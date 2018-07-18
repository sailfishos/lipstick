/***************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Aaron Kennedy <aaron.kennedy@jollamobile.com>
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

#include <QtCore/qmath.h>
#include <QSGGeometryNode>
#include <QSGSimpleMaterial>
#include <QOpenGLFramebufferObject>
#include <QWaylandQuickItem>
#include "lipstickcompositorwindow.h"
#include "lipstickcompositor.h"
#include "windowpixmapitem.h"

namespace {

class SurfaceTextureState {
public:
    SurfaceTextureState() : m_texture(0), m_xScale(1), m_yScale(1) {}
    void setTexture(QSGTexture *texture) { m_texture = texture; }
    QSGTexture *texture() const { return m_texture; }
    void setXOffset(float xOffset) { m_xOffset = xOffset; }
    float xOffset() const { return m_xOffset; }
    void setYOffset(float yOffset) { m_yOffset = yOffset; }
    float yOffset() const { return m_yOffset; }
    void setXScale(float xScale) { m_xScale = xScale; }
    float xScale() const { return m_xScale; }
    void setYScale(float yScale) { m_yScale = yScale; }
    float yScale() const { return m_yScale; }

private:
    QSGTexture *m_texture;
    float m_xOffset;
    float m_yOffset;
    float m_xScale;
    float m_yScale;
};

class SurfaceTextureMaterial : public QSGSimpleMaterialShader<SurfaceTextureState>
{
    QSG_DECLARE_SIMPLE_SHADER(SurfaceTextureMaterial, SurfaceTextureState)
public:
    QList<QByteArray> attributes() const;
    void updateState(const SurfaceTextureState *newState, const SurfaceTextureState *oldState);
protected:
    void initialize();
    const char *vertexShader() const;
    const char *fragmentShader() const;
private:
    int m_id_texOffset;
    int m_id_texScale;
};

class SurfaceNode : public QObject, public QSGGeometryNode
{
    Q_OBJECT
public:
    QSGNode *sourceNode = nullptr;

    SurfaceNode();
    ~SurfaceNode();
    void setRect(const QRectF &);
    void setTextureProvider(QSGTextureProvider *, bool owned);
    void setBlending(bool);
    void setRadius(qreal radius);
    void setXOffset(qreal xOffset);
    void setYOffset(qreal yOffset);
    void setXScale(qreal xScale);
    void setYScale(qreal yScale);
    void setUpdateTexture(bool upd);

private:
    void providerDestroyed();
    void textureChanged();

    void setTexture(QSGTexture *texture);
    void updateGeometry();

    QSGSimpleMaterial<SurfaceTextureState> *m_material;
    QRectF m_rect;
    qreal m_radius;

    QPointer<QSGTextureProvider> m_provider;
    QPointer<QSGTexture> m_texture;
    QSGGeometry m_geometry;
    QRectF m_textureRect;
    bool m_providerOwned;
};

QList<QByteArray> SurfaceTextureMaterial::attributes() const
{
    QList<QByteArray> attributeList;
    attributeList << "qt_VertexPosition";
    attributeList << "qt_VertexTexCoord";
    return attributeList;
}

void SurfaceTextureMaterial::updateState(const SurfaceTextureState *newState,
                                         const SurfaceTextureState *)
{
    Q_ASSERT(newState->texture());
    if (QSGTexture *tex = newState->texture())
        tex->bind();
    program()->setUniformValue(m_id_texOffset, newState->xOffset(), newState->yOffset());
    program()->setUniformValue(m_id_texScale, newState->xScale(), newState->yScale());
}

void SurfaceTextureMaterial::initialize()
{
    QSGSimpleMaterialShader::initialize();
    m_id_texOffset = program()->uniformLocation("texOffset");
    m_id_texScale = program()->uniformLocation("texScale");
}

const char *SurfaceTextureMaterial::vertexShader() const
{
    return "uniform highp mat4 qt_Matrix;                      \n"
           "attribute highp vec4 qt_VertexPosition;            \n"
           "attribute highp vec2 qt_VertexTexCoord;            \n"
           "varying highp vec2 qt_TexCoord;                    \n"
           "uniform highp vec2 texOffset;                      \n"
           "uniform highp vec2 texScale;                       \n"
           "void main() {                                      \n"
           "    qt_TexCoord = qt_VertexTexCoord * texScale + texOffset;\n"
           "    gl_Position = qt_Matrix * qt_VertexPosition;   \n"
           "}";
}

const char *SurfaceTextureMaterial::fragmentShader() const
{
    return "varying highp vec2 qt_TexCoord;                    \n"
           "uniform sampler2D qt_Texture;                      \n"
           "uniform lowp float qt_Opacity;                     \n"
           "void main() {                                      \n"
           "    gl_FragColor = texture2D(qt_Texture, qt_TexCoord) * qt_Opacity; \n"
           "}";
}

SurfaceNode::SurfaceNode()
    : m_material(0)
    , m_radius(0)
    , m_provider(0)
    , m_texture(0)
    , m_geometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 0)
{
    setGeometry(&m_geometry);
    m_material = SurfaceTextureMaterial::createMaterial();
    setMaterial(m_material);
}

SurfaceNode::~SurfaceNode()
{
    if (m_provider && m_providerOwned)
        delete m_provider.data();

    delete sourceNode;
}

void SurfaceNode::setRect(const QRectF &r)
{
    if (m_rect == r)
        return;

    m_rect = r;

    updateGeometry();
}

void SurfaceNode::setTextureProvider(QSGTextureProvider *p, bool owned)
{
    if (p == m_provider)
        return;

    if (m_provider) {
        disconnect(m_provider.data(), &QObject::destroyed, this, &SurfaceNode::providerDestroyed);
        disconnect(m_provider.data(), &QSGTextureProvider::textureChanged, this, &SurfaceNode::textureChanged);
        m_provider = 0;
    }

    m_provider = p;
    m_providerOwned = owned;

    if (m_provider) {
        connect(m_provider.data(), &QObject::destroyed, this, &SurfaceNode::providerDestroyed);
        connect(m_provider.data(), &QSGTextureProvider::textureChanged, this, &SurfaceNode::textureChanged);

        setTexture(m_provider->texture());
    }
}

void SurfaceNode::updateGeometry()
{
    if (m_texture) {
        QSize ts = m_texture->textureSize();
        QRectF sourceRect(0, 0, ts.width(), ts.height());
        QRectF textureRect = m_texture->convertToNormalizedSourceRect(sourceRect);

        if (m_radius) {
            float radius = qMin(float(qMin(m_rect.width(), m_rect.height()) * 0.5f), float(m_radius));
            int segments = qBound(5, qCeil(radius * (M_PI / 6)), 18);
            float angle = 0.5f * float(M_PI) / segments;

            m_geometry.allocate((segments + 1) * 2 * 2);

            QSGGeometry::TexturedPoint2D *v = m_geometry.vertexDataAsTexturedPoint2D();
            QSGGeometry::TexturedPoint2D *vlast = v + (segments + 1) * 2 * 2 - 2;

            float textureXRadius = radius * textureRect.width() / m_rect.width();
            float textureYRadius = radius * textureRect.height() / m_rect.height();

            float c = 1; float cosStep = qFastCos(angle);
            float s = 0; float sinStep = qFastSin(angle);

            for (int ii = 0; ii <= segments; ++ii) {
                float px = m_rect.left() + radius - radius * c;
                float tx = textureRect.left() + textureXRadius - textureXRadius * c;

                float px2 = m_rect.right() - radius + radius * c;
                float tx2 = textureRect.right() - textureXRadius + textureXRadius * c;

                float py = m_rect.top() + radius - radius * s;
                float ty = textureRect.top() + textureYRadius - textureYRadius * s;

                float py2 = m_rect.bottom() - radius + radius * s;
                float ty2 = textureRect.bottom() - textureYRadius + textureYRadius * s;

                v[0].x = px; v[0].y = py;
                v[0].tx = tx; v[0].ty = ty;

                v[1].x = px; v[1].y = py2;
                v[1].tx = tx; v[1].ty = ty2;

                vlast[0].x = px2; vlast[0].y = py;
                vlast[0].tx = tx2; vlast[0].ty = ty;

                vlast[1].x = px2; vlast[1].y = py2;
                vlast[1].tx = tx2; vlast[1].ty = ty2;

                v += 2;
                vlast -= 2;

                float t = c;
                c = c * cosStep - s * sinStep;
                s = s * cosStep + t * sinStep;
            }
        } else {
            m_geometry.allocate(4);
            QSGGeometry::updateTexturedRectGeometry(&m_geometry, m_rect, textureRect);
        }

        markDirty(DirtyGeometry);
    }
}

void SurfaceNode::setBlending(bool b)
{
    m_material->setFlag(QSGMaterial::Blending, b);
}

void SurfaceNode::setRadius(qreal radius)
{
    if (m_radius == radius)
        return;

    m_radius = radius;

    updateGeometry();
}

void SurfaceNode::setTexture(QSGTexture *texture)
{
    m_material->state()->setTexture(texture);

    QRectF tr;
    if (texture) tr = texture->convertToNormalizedSourceRect(QRect(QPoint(0,0), texture->textureSize()));

    bool ug = !m_texture || tr != m_textureRect;

    m_texture = texture;
    m_textureRect = tr;

    if (ug) updateGeometry();

    markDirty(DirtyMaterial);
}

void SurfaceNode::setXOffset(qreal offset)
{
    m_material->state()->setXOffset(offset);

    markDirty(DirtyMaterial);
}

void SurfaceNode::setYOffset(qreal offset)
{
    m_material->state()->setYOffset(offset);

    markDirty(DirtyMaterial);
}

void SurfaceNode::setXScale(qreal xScale)
{
    m_material->state()->setXScale(xScale);

    markDirty(DirtyMaterial);
}

void SurfaceNode::setYScale(qreal yScale)
{
    m_material->state()->setYScale(yScale);

    markDirty(DirtyMaterial);
}

void SurfaceNode::textureChanged()
{
    setTexture(m_provider->texture());
}

void SurfaceNode::providerDestroyed()
{
    m_provider = 0;
    setTexture(0);
}

}

SnapshotProgram *WindowPixmapItem::s_snapshotProgram = 0;

struct SnapshotProgram
{
    QOpenGLShaderProgram program;
    int vertexLocation;
    int textureLocation;
};

class SnapshotTextureProvider : public QSGTextureProvider
{
public:
    SnapshotTextureProvider() : t(0), fbo(0) {}
    ~SnapshotTextureProvider()
    {
        delete fbo;
        delete t;
    }
    QSGTexture *texture() const Q_DECL_OVERRIDE
    {
        return t;
    }
    QSGTexture *t;
    QOpenGLFramebufferObject *fbo;
};


WindowPixmapItem::WindowPixmapItem()
{
    setSizeFollowsSurface(false);
    setInputEventsEnabled(false);
    setTouchEventsEnabled(false);
}

WindowPixmapItem::~WindowPixmapItem()
{
}

int WindowPixmapItem::windowId() const
{
    return m_id;
}

void WindowPixmapItem::setWindowId(int id)
{
    if (m_id == id)
        return;

    const QSize oldSize = m_windowSize;
    const bool hadPixmap = m_hasPixmap;

    m_hasPixmap = false;

    m_id = id;

    LipstickCompositorWindow *window = m_id != 0
                ? LipstickCompositor::instance()->windowForId(m_id)
                : nullptr;
    if (QWaylandSurface *surface = window ? window->surface() : nullptr) {
        QWaylandSurface *oldSurface = QWaylandQuickItem::surface();

        if(oldSurface) {
            disconnect(oldSurface, &QWaylandSurface::sizeChanged, this, &WindowPixmapItem::handleWindowSizeChanged);
            disconnect(oldSurface, &QWaylandSurface::hasContentChanged, this, &WindowPixmapItem::handleHasContentChanged);
        }

        connect(surface, &QWaylandSurface::sizeChanged, this, &WindowPixmapItem::handleWindowSizeChanged);
        connect(surface, &QWaylandSurface::hasContentChanged, this, &WindowPixmapItem::handleHasContentChanged);

        m_windowSize = surface->size();
        m_hasPixmap = surface->hasContent();
        m_surfaceDestroyed = false;
        m_hasBuffer = false;

        setSurface(surface);
    }

    emit windowIdChanged();

    if (m_windowSize != oldSize) {
        emit windowSizeChanged();
    }

    if (m_hasPixmap != hadPixmap) {
        emit hasPixmapChanged();
    }
}

void WindowPixmapItem::surfaceDestroyed()
{
    m_surfaceDestroyed = true;
    m_hasBuffer = false;

    update();
}

bool WindowPixmapItem::hasPixmap() const
{
    return m_hasPixmap;
}

bool WindowPixmapItem::opaque() const
{
    return m_opaque;
}

void WindowPixmapItem::setOpaque(bool o)
{
    if (m_opaque == o)
        return;

    m_opaque = o;
    update();

    emit opaqueChanged();
}

qreal WindowPixmapItem::radius() const
{
    return m_radius;
}

void WindowPixmapItem::setRadius(qreal r)
{
    if (m_radius == r)
        return;

    m_radius = r;
    update();

    emit radiusChanged();
}

qreal WindowPixmapItem::xOffset() const
{
    return m_xOffset;
}

void WindowPixmapItem::setXOffset(qreal xOffset)
{
    if (m_xOffset == xOffset)
        return;

    m_xOffset = xOffset;
    update();

    emit xOffsetChanged();
}

qreal WindowPixmapItem::yOffset() const
{
    return m_yOffset;
}

void WindowPixmapItem::setYOffset(qreal yOffset)
{
    if (m_yOffset == yOffset)
        return;

    m_yOffset = yOffset;
    update();

    emit yOffsetChanged();
}

qreal WindowPixmapItem::xScale() const
{
    return m_xScale;
}

void WindowPixmapItem::setXScale(qreal xScale)
{
    if (m_xScale == xScale)
        return;

    m_xScale = xScale;
    if (m_haveSnapshot) update();

    emit xScaleChanged();
}

qreal WindowPixmapItem::yScale() const
{
    return m_yScale;
}

void WindowPixmapItem::setYScale(qreal yScale)
{
    if (m_yScale == yScale)
        return;

    m_yScale = yScale;
    if (m_haveSnapshot) update();

    emit yScaleChanged();
}

QSize WindowPixmapItem::windowSize() const
{
    return m_windowSize;
}

void WindowPixmapItem::handleWindowSizeChanged()
{
    if (QWaylandSurface *surface = QWaylandQuickItem::surface()) {
        const QSize size = surface->size();
        if (size.isValid() && size != m_windowSize) {
            // Window size is retained even when surface is destroyed. This allows snapshots to continue
            // rendering correctly.
            m_windowSize = size;
            emit windowSizeChanged();
        }
    }
}

QSGNode *WindowPixmapItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data)
{
    SurfaceNode *node = static_cast<SurfaceNode *>(oldNode);
    QSGNode * const sourceNode = QWaylandQuickItem::updatePaintNode(node ? node->sourceNode : nullptr, data);

    if (!sourceNode) {
        if (node) {
            node->sourceNode = nullptr;
            delete node;
        }
        return nullptr;
    }

    if (!node) {
        node = new SurfaceNode;
    }

    node->sourceNode = sourceNode;
    node->setTextureProvider(textureProvider(), false);
    node->setRect(QRectF(0, 0, width(), height()));
    node->setBlending(!m_opaque);
    node->setRadius(m_radius);
    node->setXOffset(m_xOffset);
    node->setYOffset(m_yOffset);
    node->setXScale(m_xScale);
    node->setYScale(m_yScale);

    return node;

#if 0
    SurfaceNode *node = static_cast<SurfaceNode *>(oldNode);
    QSGNode *sourceNode = QWaylandQuickItem::updatePaintNode(node ? node->sourceNode : nullptr, data);

    if (!sourceNode && !m_haveSnapshot) {
        if (node)
            node->setTextureProvider(0, false);
        delete node;
        return 0;
    }

    QSGTextureProvider *provider = textureProvider();

    QSGTexture *texture = provider ? provider->texture() : nullptr;

    if (texture && !m_surfaceDestroyed) {
        m_haveSnapshot = false;
    }

    if (!m_hasBuffer && texture) {
        if (!m_textureProvider) {
            SnapshotTextureProvider *prov = new SnapshotTextureProvider;
            m_textureProvider = prov;

            if (!s_snapshotProgram) {
                s_snapshotProgram = new SnapshotProgram;
                s_snapshotProgram->program.addShaderFromSourceCode(QOpenGLShader::Vertex,
                    "attribute highp vec4 vertex;\n"
                    "varying highp vec2 texPos;\n"
                    "void main(void) {\n"
                    "   texPos = vertex.xy;\n"
                    "   gl_Position = vec4(vertex.xy * 2.0 - 1.0, 0, 1);\n"
                    "}");
                s_snapshotProgram->program.addShaderFromSourceCode(QOpenGLShader::Fragment,
                    "uniform sampler2D texture;\n"
                    "varying highp vec2 texPos;\n"
                    "void main(void) {\n"
                    "   gl_FragColor = texture2D(texture, texPos);\n"
                    "}");
                if (!s_snapshotProgram->program.link())
                    qDebug() << s_snapshotProgram->program.log();

                s_snapshotProgram->vertexLocation = s_snapshotProgram->program.attributeLocation("vertex");
                s_snapshotProgram->textureLocation = s_snapshotProgram->program.uniformLocation("texture");

                connect(window(), &QQuickWindow::sceneGraphInvalidated, this, &WindowPixmapItem::cleanupOpenGL);
            }
        }
        provider = m_textureProvider;

        if (m_surfaceDestroyed) {
            SnapshotTextureProvider *prov = static_cast<SnapshotTextureProvider *>(provider);

            if (!prov->fbo || prov->fbo->size() != QSize(width(), height())) {
                delete prov->fbo;
                delete prov->t;
                prov->t = 0;
                prov->fbo = new QOpenGLFramebufferObject(width(), height());
            }

            prov->fbo->bind();
            s_snapshotProgram->program.bind();

            texture->bind();

            static GLfloat const triangleVertices[] = {
                1.f, 0.f,
                1.f, 1.f,
                0.f, 0.f,
                0.f, 1.f,
            };
            s_snapshotProgram->program.enableAttributeArray(s_snapshotProgram->vertexLocation);
            s_snapshotProgram->program.setAttributeArray(s_snapshotProgram->vertexLocation, triangleVertices, 2);

            glViewport(0, 0, width(), height());
            glDisable(GL_BLEND);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            s_snapshotProgram->program.release();

            if (!prov->t) {
                prov->t = window()->createTextureFromId(prov->fbo->texture(), prov->fbo->size(), 0);
                emit prov->textureChanged();
            }
            prov->fbo->release();

            s_snapshotProgram->program.disableAttributeArray(s_snapshotProgram->vertexLocation);

            m_haveSnapshot = true;
            m_surfaceDestroyed = false;
        }
    } else if (!m_hasBuffer && m_textureProvider) {
        provider = m_textureProvider;
    } else if (!provider) {
        if (node)
            node->setTextureProvider(0, false);
        delete node;
        return 0;
    }
    // The else case here is no buffer and no screenshot, so no way to show a sane image.
    // It should normally not happen, though.

    if (provider != m_textureProvider) {
        delete m_textureProvider;
        m_textureProvider = 0;
    }


    if (!provider->texture()) {
        qWarning("WindowPixmapItem does not have a source texture, cover will be dropped..");
        if (node) {
            node->setTextureProvider(0, false);
            delete node;
        }
        return 0;
    }

    if (!node) node = new SurfaceNode;

    node->sourceNode = sourceNode;
    node->setTextureProvider(provider, provider == m_textureProvider);
    node->setRect(QRectF(0, 0, width(), height()));
    node->setBlending(!m_opaque);
    node->setRadius(m_radius);
    node->setXOffset(m_xOffset);
    node->setYOffset(m_yOffset);
    node->setXScale(m_xScale);
    node->setYScale(m_yScale);

    return node;
#endif
}

void WindowPixmapItem::handleHasContentChanged()
{
    const bool hadPixmap = m_hasPixmap;

    if (QWaylandSurface *surface = QWaylandQuickItem::surface()) {
        m_hasPixmap = surface->hasContent();
    }

    if (m_hasPixmap != hadPixmap) {
        emit hasPixmapChanged();
    }
}

void WindowPixmapItem::cleanupOpenGL()
{
    disconnect(window(), &QQuickWindow::sceneGraphInvalidated, this, &WindowPixmapItem::cleanupOpenGL);
    delete s_snapshotProgram;
    s_snapshotProgram = 0;
}

#include "windowpixmapitem.moc"
