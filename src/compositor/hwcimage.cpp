/***************************************************************************
**
** Copyright (C) 2015 - 2020 Jolla Ltd.
** Copyright (C) 2019 - 2020 Open Mobile Platform LLC.
**
** This file is part of lipstick.
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation
** and appearing in the file LICENSE.LGPL included in the packaging
** of this file.
**
****************************************************************************/

#include "hwcimage.h"
#include "hwcrenderstage.h"

#include "eglhybrisbuffer.h"
#include "logging.h"

#include <MGConfItem>

#include <QRunnable>
#include <QThreadPool>

#include <QQuickWindow>
#include <QSGSimpleTextureNode>
#include <QFileInfo>
#include <QImageReader>

#include <private/qquickitem_p.h>
#include <private/qmemrotate_p.h>

#define HWCIMAGE_LOAD_EVENT ((QEvent::Type) (QEvent::User + 1))
#define DEFAULT_THEME "sailfish-default"

static QRgb swizzleWithAlpha(QRgb rgb)
{
    return ((rgb << 16) & 0x00FF0000)
         | ((rgb >> 16) & 0x000000FF)
         | ( rgb        & 0xFF00FF00);
}

class HwcImageLoadRequest : public QRunnable, public QEvent
{
public:
    HwcImageLoadRequest()
        : QEvent(HWCIMAGE_LOAD_EVENT)
    {
        setAutoDelete(false);
    }

    ~HwcImageLoadRequest() {
        qCDebug(lcLipstickHwcLog, "HwcImageLoadRequest completed and destroyed...");
    }

    void execute() {
        QImageReader reader(file);

        const QSize originalSize = reader.size();


        if (!originalSize.isValid()) {
            qCWarning(lcLipstickHwcLog, "%s is not a valid file or doesn't support reading image size: %s", qPrintable(file), qPrintable(reader.errorString()));
            return;
        } else if (Q_UNLIKELY(rotation != 0 && rotation != 90 && rotation != 180 && rotation != 270)) {
            qCWarning(lcLipstickHwcLog, "%d is not a supported rotation: %s", rotation, qPrintable(file));
            return;
        }

        const bool transpose = rotation % 180 != 0;

        QSize imageSize = originalSize;

        if (textureSize.width() > 0 && textureSize.height() > 0) {
            imageSize = transpose ? textureSize.transposed() : textureSize;
        }

        if (maxTextureSize > 0) {
            imageSize = QSize(
                        qMin(imageSize.width(), maxTextureSize),
                        qMin(imageSize.height(), maxTextureSize));
        }

        QSize scaledSize = originalSize.scaled(imageSize, Qt::KeepAspectRatioByExpanding);

        if (imageSize != scaledSize) {
            QRect clipRect(
                        (scaledSize.width() - imageSize.width()) / 2,
                        (scaledSize.height() - imageSize.height()) / 2,
                        imageSize.width(),
                        imageSize.height());
            reader.setScaledClipRect(clipRect);
        }

        reader.setScaledSize(scaledSize);
        reader.setBackgroundColor(Qt::black);

        if (transpose) {
            scaledSize.transpose();
        }

        static const bool bgraBuffers = qEnvironmentVariableIntValue("QT_OPENGL_NO_BGRA") == 0;

        if (const EglHybrisFunctions * const eglFunctions = EglHybrisFunctions::instance()) {
            const EglHybrisBuffer::Usage usage = EglHybrisBuffer::Write | EglHybrisBuffer::Texture | EglHybrisBuffer::Composer;

            switch (reader.imageFormat()) {
            case QImage::Format_ARGB32:
            case QImage::Format_RGB32:
                hybrisBuffer = new EglHybrisBuffer(
                            bgraBuffers ? EglHybrisBuffer::BGRA : EglHybrisBuffer::RGBX,
                            imageSize,
                            usage,
                            *eglFunctions);
                break;
            default:
                break;
            }
        }

        int stride = 0;
        uchar * const bytes = hybrisBuffer && hybrisBuffer->allocate()
                ? hybrisBuffer->lock(EglHybrisBuffer::Write, &stride)
                : nullptr;
        if (!bytes) {
            hybrisBuffer = EglHybrisBuffer::Pointer();
        }

        if (bytes && rotation == 0 && bgraBuffers && !overlay.isValid() && effect.isEmpty()) {
            image = QImage(bytes, imageSize.width(), imageSize.height(), stride, reader.imageFormat());

            if (!reader.read(&image)) {
                qCWarning(lcLipstickHwcLog, "Error reading %s: %s", qPrintable(file), qPrintable(reader.errorString()));

                hybrisBuffer->unlock();
                hybrisBuffer = EglHybrisBuffer::Pointer();

                return;
            }
        } else {
            image = reader.read();

            if (image.isNull()) {
                qCWarning(lcLipstickHwcLog, "Error reading %s: %s", qPrintable(file), qPrintable(reader.errorString()));
                return;
            }
        }

        if ((overlay.isValid() || !effect.isEmpty())) {
            QPainter painter(&image);

            // Apply overlay
            if (overlay.isValid()) {
                if (bytes && !bgraBuffers) {
                    // Counter swizzle the background color.
                    QRgb red = overlay.red();
                    overlay.setRed(overlay.blue());
                    overlay.setBlue(red);
                }

                painter.fillRect(image.rect(), overlay);
            }

            // Apply glass..
            if (effect.contains(QStringLiteral("glass"))) {
                const auto shaderImageTemplate = QStringLiteral("/usr/share/themes/%1/meegotouch/icons/graphic-shader-texture.png");
                const auto themeName = MGConfItem("/meegotouch/theme/name").value(DEFAULT_THEME).toString();
                auto shaderImage = shaderImageTemplate.arg(themeName);
                if (themeName != QStringLiteral(DEFAULT_THEME) && !QFile::exists(shaderImage)) {
                    qCDebug(lcLipstickHwcLog, "Shader texture file does not exist: %s", qPrintable(shaderImage));
                    shaderImage = shaderImageTemplate.arg(DEFAULT_THEME);
                }
                QImage glass(shaderImage);

                if (rotation != 0) { // Counter rotate the glass effect so it is reset when the entire picture is rotated.
                    QTransform transform;
                    transform.rotate(-rotation);
                    glass = glass.transformed(transform);
                }

                painter.save();
                painter.setOpacity(0.1);
                painter.setCompositionMode(QPainter::CompositionMode_Plus);
                painter.fillRect(image.rect(), glass);
                painter.restore();
            }
        }

        if (!bytes || bytes == image.constBits()) {
            // No data copies or transforms are required.
        } else if (bgraBuffers) {
            switch (rotation) {
            case 0:
                if (image.bytesPerLine() == stride) {
                    memcpy(bytes, image.constBits(), image.byteCount());
                } else {
                    uchar *row = bytes;
                    const int length = image.width() * 4;
                    const int height = image.height();
                    for (int y = 0; y < height; ++y, row += stride) {
                        memcpy(row, image.constScanLine(y), length);
                    }
                }
                break;
            case 90:
                qt_memrotate270(
                            reinterpret_cast<const quint32 *>(image.bits()),
                            image.width(),
                            image.height(),
                            image.bytesPerLine(),
                            reinterpret_cast<quint32 *>(bytes),
                            stride);
                break;
            case 180:
                qt_memrotate180(
                            reinterpret_cast<const quint32 *>(image.bits()),
                            image.width(),
                            image.height(),
                            image.bytesPerLine(),
                            reinterpret_cast<quint32 *>(bytes),
                            stride);
                break;
            case 270:
                qt_memrotate90(
                            reinterpret_cast<const quint32 *>(image.bits()),
                            image.width(),
                            image.height(),
                            image.bytesPerLine(),
                            reinterpret_cast<quint32 *>(bytes),
                            stride);
                break;
            }
        } else {
            const int inStride = image.bytesPerLine();
            const int outStride = stride;
            const int width = image.width();
            const int height = image.height();

            switch (rotation) {
            case 0: {
                const uchar *in = image.bits();
                uchar *out = bytes;

                for (int y = 0; y < height; ++y, in += inStride, out += outStride) {
                    const QRgb * const inRow = reinterpret_cast<const QRgb *>(in);
                    QRgb * const outRow = reinterpret_cast<QRgb *>(out);
                    for (int x = 0; x < width; ++x) {
                        outRow[x] = swizzleWithAlpha(inRow[x]);
                    }
                }
                break;
            }
            // Don't worry too much about optimizing these, there's presently no known combination
            // of the hwcomposer (and therefore rotation) and no BGRA support.
            case 90: {
                const uchar *in = image.bits() + (inStride * (height - 1));
                uchar *out = bytes;

                for (int y = 0; y < height; ++y, in += 4, out += outStride) {
                    QRgb * const outRow = reinterpret_cast<QRgb *>(out);
                    for (int x = 0; x < width; ++x) {
                        outRow[x] = swizzleWithAlpha(*reinterpret_cast<const QRgb *>(in - (inStride * x)));
                    }
                }
                break;
            }
            case 180: {
                const uchar *in = image.bits() + (inStride * (height - 1));
                uchar *out = bytes;

                for (int y = 0; y < height; ++y, in -= inStride, out += outStride) {
                    const QRgb * const inRow = reinterpret_cast<const QRgb *>(in);
                    QRgb * const outRow = reinterpret_cast<QRgb *>(out);
                    for (int x = 0; x < width; ++x) {
                        outRow[x] = swizzleWithAlpha(inRow[width - x]);
                    }
                }
                break;
            }
            case 270: {
                const uchar *in = image.bits() + (4 * (width - 1));
                uchar *out = bytes;

                for (int y = 0; y < height; ++y, in -= 4, out += outStride) {
                    QRgb * const outRow = reinterpret_cast<QRgb *>(out);
                    for (int x = 0; x < width; ++x) {
                        outRow[x] = swizzleWithAlpha(*reinterpret_cast<const QRgb *>(in + (inStride * x)));
                    }
                }
                break;
            }
            }
        }

        if (hybrisBuffer) {
            hybrisBuffer->unlock();
            image = QImage();
        }
    }

    void run() {
        execute();
        mutex.lock();
        if (hwcImage && hwcImage->m_pendingRequest == this) {
            hwcImage->m_pendingRequest = 0;
            QCoreApplication::postEvent(hwcImage, this);
        } else {
            setAutoDelete(true);
        }
        mutex.unlock();
    }

    QExplicitlySharedDataPointer<EglHybrisBuffer> hybrisBuffer;
    QImage image;
    QString file;
    QString effect;
    QColor overlay;
    QSize textureSize;
    qreal pixelRatio;
    int rotation;
    int maxTextureSize;

    HwcImage *hwcImage;

    static QMutex mutex;
};

QMutex HwcImageLoadRequest::mutex;

HwcImage::HwcImage()
    : m_pendingRequest(0)
    , m_rotationHandler(0)
    , m_status(Null)
    , m_textureRotation(0)
    , m_asynchronous(true)
    , m_usedInEffect(false)
    , m_updateImage(false)
{
    setFlag(ItemHasContents, true);
    connect(this, &QQuickItem::windowChanged, this, &HwcImage::onWindowChange);
}

HwcImage::~HwcImage()
{
    HwcImageLoadRequest::mutex.lock();
    if (m_pendingRequest)
        m_pendingRequest->hwcImage = 0;
    HwcImageLoadRequest::mutex.unlock();
}


/* We want to track the item that does rotation changes explicitly. This isn't
   strictly needed, but it is easy to do and means we get away with tracking
   one single object as opposed to listening for changes in an entire parent
   hierarchy which would also be a lot slower.
  */
void HwcImage::setRotationHandler(QQuickItem *item)
{
    if (!HwcRenderStage::isHwcEnabled()) {
        qCDebug(lcLipstickHwcLog, "HwcImage ignoring rotation handler as HWC is disabled");
        return;
    }

    if (m_rotationHandler == item)
        return;
    if (m_rotationHandler)
        disconnect(m_rotationHandler, &QQuickItem::rotationChanged, this, &HwcImage::handlerRotationChanged);
    m_rotationHandler = item;
    if (m_rotationHandler)
        connect(m_rotationHandler, &QQuickItem::rotationChanged, this, &HwcImage::handlerRotationChanged);
    emit rotationHandlerChanged();
    polish();

    qCDebug(lcLipstickHwcLog) << "HwcImage" << this << "tracking rotation handler" << item;
}

void HwcImage::setAsynchronous(bool is)
{
    if (m_asynchronous == is)
        return;
    m_asynchronous = is;
    emit asynchronousChanged();
    polish();
}

void HwcImage::setSource(const QUrl &source)
{
    if (source == m_source)
        return;
    m_source = source;
    emit sourceChanged();
    reload();
}

void HwcImage::setOverlayColor(const QColor &color)
{
    if (m_overlayColor == color)
        return;
    m_overlayColor = color;
    emit overlayColorChanged();
    reload();
}

void HwcImage::setMaxTextureSize(int size)
{
    if (m_maxTextureSize == size)
        return;
    m_maxTextureSize = size;
    emit maxTextureSizeChanged();
    reload();
}

void HwcImage::setTextureSize(const QSize &size)
{
    if (m_textureSize == size)
        return;
    m_textureSize = size;
    emit textureSizeChanged();
    reload();
}

void HwcImage::setEffect(const QString &effect)
{
    if (m_effect == effect)
        return;
    m_effect = effect;
    emit effectChanged();
    reload();
}

void HwcImage::setPixelRatio(qreal ratio)
{
    if (m_pixelRatio == ratio)
        return;
    m_pixelRatio = ratio;
    emit pixelRatioChanged();
    polish();
}

qreal hwcimage_get_rotation(QQuickItem *item)
{
    qreal rotation = fmod(item->rotation(), 360.0);
    return rotation < 0 ? 360.0 + rotation : rotation;
}

void HwcImage::handlerRotationChanged()
{
    qreal rotation = hwcimage_get_rotation(m_rotationHandler);
    bool is90 = qFuzzyCompare(0.0, fmod(rotation, 90));
    qCDebug(lcLipstickHwcLog, " - rotation changed: %6.3f, 90 degree=%s", rotation, is90 ? "yes" : "no");
    if (is90 && m_textureRotation != rotation)
        polish();
}

void HwcImage::updatePolish()
{
    if (m_source.isEmpty()) {
        // Trigger deletion of the sg node and texture in case the source has been
        // removed.
        m_image = QImage();
        m_updateImage = true;
        update();
        return;
    }

    if (!QFileInfo(m_source.toLocalFile()).exists()) {
        qCDebug(lcLipstickHwcLog, "HwcImage: source file does not exist (%s)", qPrintable(m_source.toString()));
        return;
    }

    m_status = Loading;
    emit statusChanged();

    m_image = QImage();
    HwcImageLoadRequest *req = new HwcImageLoadRequest();
    req->hwcImage = this;
    req->file = m_source.toLocalFile();
    req->textureSize = m_textureSize;
    req->effect = m_effect;
    req->overlay = m_overlayColor;
    req->pixelRatio = m_pixelRatio;
    req->rotation = m_rotationHandler ? qRound(hwcimage_get_rotation(m_rotationHandler)) : 0;
    req->maxTextureSize = m_maxTextureSize;

    if (m_maxTextureSize > 0 && m_textureSize.width() > 0 && m_textureSize.height() > 0)
        qWarning() << "HwcImage: both 'textureSize' and 'maxTextureSize' are set; 'textureSize' will take presedence" << this;

    qCDebug(lcLipstickHwcLog,
            "Scheduling HwcImage request, source=%s, (%d x %d), eff=%s, olay=%s, rot=%d, pr=%f, %s",
            qPrintable(m_source.toString()),
            m_textureSize.width(), m_textureSize.height(),
            qPrintable(m_effect),
            qPrintable(m_overlayColor.name(QColor::HexArgb)),
            req->rotation,
            m_pixelRatio,
            (m_asynchronous ? "async" : "sync")
            );

    if (m_asynchronous) {
        HwcImageLoadRequest::mutex.lock();
        m_pendingRequest = req;
        QThreadPool::globalInstance()->start(m_pendingRequest);
        HwcImageLoadRequest::mutex.unlock();
    } else {
        req->execute();
        apply(req);
        delete req;
    }
}

void HwcImage::reload()
{
    if (m_status != Loading) {
        m_status = Loading;
        emit statusChanged();
    }
    polish();
}

void HwcImage::apply(HwcImageLoadRequest *req)
{

    m_hybrisBuffer = req->hybrisBuffer;
    m_image = req->image;
    QSize s = m_hybrisBuffer ? m_hybrisBuffer->size() : m_image.size();
    setSize(s);
    m_status = s.isValid() ? Ready : Error;
    emit statusChanged();
    m_textureRotation = req->rotation;
    m_updateImage = true;
    update();
}

bool HwcImage::event(QEvent *e)
{
    if (e->type() == HWCIMAGE_LOAD_EVENT) {
        HwcImageLoadRequest *req = static_cast<HwcImageLoadRequest *>(e);

        bool accept = m_source.toLocalFile() == req->file
                      && m_effect == req->effect
                      && m_textureSize == req->textureSize
                      && m_pixelRatio == req->pixelRatio
                      && m_overlayColor == req->overlay;
        qCDebug(lcLipstickHwcLog,
                "HwcImage request completed: %s, source=%s, (%d x %d), eff=%s, olay=%s, rot=%d, pr=%f",
                (accept ? "accepted" : "rejected"),
                qPrintable(req->file),
                req->textureSize.width(), req->textureSize.height(),
                qPrintable(req->effect),
                qPrintable(req->overlay.name(QColor::HexArgb)),
                req->rotation,
                req->pixelRatio);
        if (accept)
            apply(req);

        return true;
    }
    return QQuickItem::event(e);
}

void HwcImage::onWindowChange()
{
    if (m_window)
        disconnect(m_window.data(), &QQuickWindow::beforeSynchronizing, this, &HwcImage::onSync);
    m_window = window();
    if (m_window)
        connect(m_window.data(), &QQuickWindow::beforeSynchronizing, this, &HwcImage::onSync, Qt::DirectConnection);
}

void HwcImage::onSync()
{
    const bool used = hasEffectReferences(this);

    if (used != m_usedInEffect) {
        m_usedInEffect = used;
        update();
    }
}

bool HwcImage::hasEffectReferences(QQuickItem *item)
{
    for (; item; item = item->parentItem()) {
        QQuickItemPrivate *d = QQuickItemPrivate::get(item);
        if (d->extra.isAllocated() && d->extra->effectRefCount > 0) {
            return true;
        }
    }
    return false;
}


class HwcImageTexture : public QSGTexture
{
    Q_OBJECT

public:
    HwcImageTexture(const EglHybrisBuffer::Pointer &buffer, HwcRenderStage *hwc);
    ~HwcImageTexture();

    int textureId() const { return m_id; }
    QSize textureSize() const { return m_buffer->size(); }
    bool hasAlphaChannel() const { return m_buffer->format() != EglHybrisBuffer::RGBX; }
    bool hasMipmaps() const { return false; }
    void bind();

    void *handle() const { return m_buffer->handle(); }

private:
    EglHybrisBuffer::Pointer m_buffer;
    HwcRenderStage * const m_hwc;
    GLuint m_id = 0;
    bool m_bound = false;
};

class HwcImageNode : public QSGSimpleTextureNode
{
public:
    HwcImageNode() {
        qCDebug(lcLipstickHwcLog) << "HwcImageNode is created...";
        qsgnode_set_description(this, QStringLiteral("hwc-image-node"));
        setOwnsTexture(true);
    }
    ~HwcImageNode() {
        qCDebug(lcLipstickHwcLog) << "HwcImageNode is gone...";
    }
    void *handle() const {
        HwcImageTexture *t = static_cast<HwcImageTexture *>(texture());
        return t ? t->handle() : 0;
    }
};

HwcImageNode *HwcImage::updateActualPaintNode(QSGNode *old)
{
    if ((m_updateImage && m_image.isNull() && !m_hybrisBuffer) || width() <= 0 || height() <= 0) {
        delete old;
        return 0;
    }

    HwcImageNode *tn = static_cast<HwcImageNode *>(old);
    if (!tn)
        tn = new HwcImageNode();

    if (m_updateImage) {
        tn->setTexture(m_hybrisBuffer
                    ? new HwcImageTexture(
                            m_hybrisBuffer,
                            HwcRenderStage::isHwcEnabled()
                                ? static_cast<HwcRenderStage *>(QQuickWindowPrivate::get(window())->customRenderStage)
                                : nullptr)
                    : window()->createTextureFromImage(m_image));
        m_image = QImage();
        m_hybrisBuffer = EglHybrisBuffer::Pointer();
        m_updateImage = false;
    }
    tn->setRect(0, 0, width(), height());
    return tn;
}

QMatrix4x4 HwcImage::reverseTransform() const
{
    if (!m_rotationHandler)
        return QMatrix4x4();
    // We assume a center-oriented rotation with no monkey busines between us
    // and the rotationHandler and the rotationHandler must be a parent of
    // ourselves. (It wouldn't rotate otherwise)
    float w2 = width() / 2;
    float h2 = height() / 2;
    QMatrix4x4 m;
    m.translate(w2, h2);
    m.rotate(-m_textureRotation, 0, 0, 1);
    m.translate(-w2, -h2);
    return m;
}


QSGNode *HwcImage::updatePaintNode(QSGNode *old, UpdatePaintNodeData *)
{
    if (!HwcRenderStage::isHwcEnabled()) {
        qCDebug(lcLipstickHwcLog) << "HwcImage" << this << "updating paint node without HWC support";
        return updateActualPaintNode(old);
    }

    /*
        When we're using hwcomposer, we replace the image with a slightly more
        complex subtree. There is an HwcNode which is where we put the buffer
        handle which the HwcRenderStage will pick up. Then we have a
        QSGTransformNode which we use to apply reverse transforms. For different
        orientations we create a rotated texture and invert the transform
        in the scene graph.

        HwcNode             <- The hook which makes us get picked up by the HwcRenderStage
           |
        QSGTransformNode    <- The reverse transform used to get correct orientation
           |
        HwcImageNode        <- The texture node which gets rendered as a fallback.
     */
    HwcNode *hwcNode = 0;

    if (old) {
        qCDebug(lcLipstickHwcLog) << "HwcImage" << this << "updating paint existing node";
        hwcNode = static_cast<HwcNode *>(old);
        HwcImageNode *contentNode = updateActualPaintNode(hwcNode->firstChild()->firstChild());
        if (contentNode == 0) {
            delete hwcNode;
            return 0;
        } else if (contentNode != hwcNode->firstChild()->firstChild()) {
            // No need to remove the old node as the updatePaintNode call will
            // already have deleted the old content node and it would thus have
            // already been taken out.
            hwcNode->firstChild()->appendChildNode(contentNode);
        }
        static_cast<QSGTransformNode *>(hwcNode->firstChild())->setMatrix(reverseTransform());
        hwcNode->update(contentNode, contentNode->handle());
    } else if (HwcImageNode *contentNode = updateActualPaintNode(0)) {
        qCDebug(lcLipstickHwcLog) << "HwcImage" << this << "creating new node";
        hwcNode = new HwcNode(window());
        QSGTransformNode *xnode = new QSGTransformNode();
        xnode->setMatrix(reverseTransform());
        qsgnode_set_description(xnode, QStringLiteral("hwc-reverse-xform"));
        xnode->appendChildNode(contentNode);
        hwcNode->appendChildNode(xnode);
        hwcNode->update(contentNode, contentNode->handle());
    } else {
        return 0;
    }

    if (hwcNode)
        hwcNode->setForcedGLRendering(m_usedInEffect);

    return hwcNode;
}

HwcImageTexture::HwcImageTexture(const EglHybrisBuffer::Pointer &buffer, HwcRenderStage *hwc)
    : m_buffer(buffer)
    , m_hwc(hwc)
{
    glGenTextures(1, &m_id);
    qCDebug(lcLipstickHwcLog,
            "HwcImageTexture(%p) created, size=(%d x %d), texId=%d",
            this, m_buffer->size().width(), m_buffer->size().height(), m_id);
}

HwcImageTexture::~HwcImageTexture()
{
    glDeleteTextures(1, &m_id);

    if (m_hwc) {
        m_buffer->ref.ref();
        // Register for a callback so we can delete ourselves when HWC is done
        // with our buffer. Chances are the cleanup will happen on the HWC thread,
        // but this is safe as the EGL resource can be deleted from any thread..
        m_hwc->signalOnBufferRelease(EglHybrisBuffer::destroy, handle(), m_buffer.data());
    }

    qCDebug(lcLipstickHwcLog,
            "HwcImageTexture(%p) destroyed, size=(%d x %d), texId=%d",
            this, m_buffer->size().width(), m_buffer->size().height(), m_id);
}

void HwcImageTexture::bind()
{
    glBindTexture(GL_TEXTURE_2D, m_id);
    updateBindOptions(!m_bound);
    if (!m_bound) {
        m_bound = true;
        m_buffer->bindToTexture();
    }
}

#include "hwcimage.moc"

