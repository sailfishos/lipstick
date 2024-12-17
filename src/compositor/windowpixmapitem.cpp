/***************************************************************************
**
** Copyright (c) 2013 Jolla Ltd.
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
#include <QSGMaterial>
#include <QSGMaterialShader>
#include <QSGTexture>
#include <QSGTextureProvider>
#include <QOpenGLFramebufferObject>
#include <QOpenGLShaderProgram>
#include <QWaylandSurfaceItem>
#include "lipstickcompositorwindow.h"
#include "lipstickcompositor.h"
#include "windowpixmapitem.h"

namespace {

class OpaqueSurfaceTextureMaterial;

struct CornerVertex
{
    float m_x;
    float m_y;
    float m_tx;
    float m_ty;
    float m_cx;
    float m_cy;

    void set(float x, float y, float tx, float ty, float cx, float cy)
    {
        m_x = x; m_y = y; m_tx = tx; m_ty = ty; m_cx = cx; m_cy = cy;
    }
};

const QSGGeometry::AttributeSet &cornerAttributes()
{
    static QSGGeometry::Attribute data[] = {
        QSGGeometry::Attribute::create(0, 2, GL_FLOAT, true),
        QSGGeometry::Attribute::create(1, 2, GL_FLOAT),
        QSGGeometry::Attribute::create(2, 2, GL_FLOAT)
    };
    static QSGGeometry::AttributeSet attributes = { 3, sizeof(CornerVertex), data };
    return attributes;
}

class OpaqueSurfaceTextureShader : public QSGMaterialShader
{
public:
    const char *vertexShader() const override;
    const char *fragmentShader() const override;
    char const *const *attributeNames() const override;
    void updateState(
            const RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
    void initialize() override;

private:
    int m_id_qt_Matrix = -1;
    int m_qt_Texture = -1;
};

class SurfaceTextureShader : public OpaqueSurfaceTextureShader
{
public:
    const char *fragmentShader() const override;
    void updateState(
            const RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
    void initialize() override;

private:
    int m_id_qt_Opacity = -1;
};

class CornerSurfaceTextureShader : public SurfaceTextureShader
{
public:
    const char *vertexShader() const override;
    const char *fragmentShader() const override;
    char const *const *attributeNames() const override;
    void updateState(
            const RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
    void initialize() override;

private:
    int m_id_radius;
};

class OpaqueSurfaceTextureMaterial : public QSGMaterial
{
public:
    void setTexture(QSGTexture *texture) { m_texture = texture; }
    QSGTexture *texture() const { return m_texture; }

    QSGMaterialType *type() const override;
    QSGMaterialShader *createShader() const override;

    int compare(const QSGMaterial *other) const override;

private:
    QSGTexture *m_texture = nullptr;
};

class SurfaceTextureMaterial : public QSGMaterial
{
public:
    SurfaceTextureMaterial(OpaqueSurfaceTextureMaterial &opaqueMaterial)
        : opaqueMaterial(opaqueMaterial)
    {
    }

    QSGMaterialType *type() const override;
    QSGMaterialShader *createShader() const override;

    int compare(const QSGMaterial *other) const override;

    OpaqueSurfaceTextureMaterial &opaqueMaterial;
};

class CornerSurfaceTextureMaterial : public SurfaceTextureMaterial
{
public:
    CornerSurfaceTextureMaterial(OpaqueSurfaceTextureMaterial &opaqueMaterial)
        : SurfaceTextureMaterial(opaqueMaterial)
    {
    }

    float radius() const { return m_radius; }
    void setRadius(float radius) { m_radius = radius; }

    QSGMaterialType *type() const override;
    QSGMaterialShader *createShader() const override;

    int compare(const QSGMaterial *other) const override;

private:
    qreal m_radius = 0;
};

class SurfaceNode : public QObject, public QSGGeometryNode
{
    Q_OBJECT
public:
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

    void updateGeometry();

private slots:
    void providerDestroyed();
    void textureChanged();

private:
    void setTexture(QSGTexture *texture);
    OpaqueSurfaceTextureMaterial m_opaqueMaterial;
    SurfaceTextureMaterial m_material { m_opaqueMaterial };
    CornerSurfaceTextureMaterial m_cornerMaterial { m_opaqueMaterial };
    QSGGeometry m_geometry { QSGGeometry::defaultAttributes_TexturedPoint2D(), 0 };
    QSGGeometry m_cornerGeometry { cornerAttributes(), 0 };
    QSGGeometryNode m_cornerNode;

    QRectF m_rect;
    QRectF m_textureRect;
    qreal m_radius = 0;
    qreal m_xOffset = 0;
    qreal m_yOffset = 0;
    qreal m_xScale = 1;
    qreal m_yScale = 1;

    QSGTextureProvider *m_provider = nullptr;
    QSGTexture *m_texture = nullptr;
    bool m_providerOwned = false;
    bool m_geometryChanged = true;
};

const char *OpaqueSurfaceTextureShader::vertexShader() const
{
    return "uniform highp mat4 qt_Matrix;                      \n"
           "attribute highp vec4 qt_VertexPosition;            \n"
           "attribute highp vec2 qt_VertexTexCoord;            \n"
           "varying highp vec2 qt_TexCoord;                    \n"
           "void main() {                                      \n"
           "    qt_TexCoord = qt_VertexTexCoord;\n"
           "    gl_Position = qt_Matrix * qt_VertexPosition;   \n"
           "}";
}

const char *OpaqueSurfaceTextureShader::fragmentShader() const
{
    return "varying highp vec2 qt_TexCoord;                    \n"
           "uniform sampler2D qt_Texture;                      \n"
           "void main() {                                      \n"
           "    gl_FragColor = texture2D(qt_Texture, qt_TexCoord); \n"
           "}";
}

char const *const *OpaqueSurfaceTextureShader::attributeNames() const
{
    static const char *attributes[] = {
        "qt_VertexPosition",
        "qt_VertexTexCoord",
        nullptr
    };
    return attributes;
}

void OpaqueSurfaceTextureShader::updateState(
        const RenderState &state, QSGMaterial *newMaterial, QSGMaterial *)
{
    QOpenGLShaderProgram * const program = QSGMaterialShader::program();
    OpaqueSurfaceTextureMaterial *newSurface = static_cast<OpaqueSurfaceTextureMaterial *>(newMaterial);

    if (QSGTexture *texture = newSurface->texture()) {
        texture->bind();
    }

    if (state.isMatrixDirty()) {
        program->setUniformValue(m_id_qt_Matrix, state.combinedMatrix());
    }
}

void OpaqueSurfaceTextureShader::initialize()
{
    QOpenGLShaderProgram * const program = QSGMaterialShader::program();

    m_id_qt_Matrix = program->uniformLocation("qt_Matrix");
    m_qt_Texture = program->uniformLocation("qt_Texture");
}

const char *SurfaceTextureShader::fragmentShader() const
{
    return "varying highp vec2 qt_TexCoord;                    \n"
           "uniform sampler2D qt_Texture;                      \n"
           "uniform lowp float qt_Opacity;                     \n"
           "void main() {                                      \n"
           "    gl_FragColor = texture2D(qt_Texture, qt_TexCoord) * qt_Opacity; \n"
           "}";
}

void SurfaceTextureShader::updateState(
        const RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    QOpenGLShaderProgram * const program = QSGMaterialShader::program();
    SurfaceTextureMaterial *newSurface = static_cast<SurfaceTextureMaterial *>(newMaterial);
    SurfaceTextureMaterial *oldSurface = static_cast<SurfaceTextureMaterial *>(oldMaterial);

    OpaqueSurfaceTextureShader::updateState(
                state,
                &newSurface->opaqueMaterial,
                oldSurface ? &oldSurface->opaqueMaterial : nullptr);

    if (state.isOpacityDirty()) {
        program->setUniformValue(m_id_qt_Opacity, state.opacity());
    }
}

void SurfaceTextureShader::initialize()
{
    OpaqueSurfaceTextureShader::initialize();

    QOpenGLShaderProgram * const program = QSGMaterialShader::program();

    m_id_qt_Opacity = program->uniformLocation("qt_Opacity");
}

const char *CornerSurfaceTextureShader::vertexShader() const
{
    return "uniform highp mat4 qt_Matrix;                      \n"
           "attribute highp vec4 qt_VertexPosition;            \n"
           "attribute highp vec2 qt_VertexTexCoord;            \n"
           "attribute highp vec2 vertexCorner;                 \n"
           "varying highp vec2 qt_TexCoord;                    \n"
           "varying highp vec2 corner;                         \n"
           "void main() {                                      \n"
           "    qt_TexCoord = qt_VertexTexCoord;               \n"
           "    gl_Position = qt_Matrix * qt_VertexPosition;   \n"
           "    corner = vertexCorner;                         \n"
           "}";
}

const char *CornerSurfaceTextureShader::fragmentShader() const
{
    return "varying highp vec2 qt_TexCoord;                    \n"
           "varying highp vec2 corner;                         \n"
           "uniform sampler2D qt_Texture;                      \n"
           "uniform lowp float qt_Opacity;                     \n"
           "uniform lowp vec2 radius;                          \n"
           "void main() {                                      \n"
           "    gl_FragColor = texture2D(qt_Texture, qt_TexCoord)\n"
           "                 * smoothstep(radius.x, radius.y, dot(corner, corner))\n"
           "                 * qt_Opacity;                     \n"
           "}";
}

char const *const *CornerSurfaceTextureShader::attributeNames() const
{
    static const char *attributes[] = {
        "qt_VertexPosition",
        "qt_VertexTexCoord",
        "vertexCorner",
        nullptr
    };
    return attributes;
}

void CornerSurfaceTextureShader::updateState(
        const RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    SurfaceTextureShader::updateState(state, newMaterial, oldMaterial);

    QOpenGLShaderProgram * const program = QSGMaterialShader::program();
    CornerSurfaceTextureMaterial *newSurface = static_cast<CornerSurfaceTextureMaterial *>(newMaterial);

    const float radius = newSurface->radius();

    program->setUniformValue(m_id_radius, (radius + 0.5f) * (radius + 0.5f), (radius - 0.5f) * (radius - 0.5f));
}

void CornerSurfaceTextureShader::initialize()
{
    SurfaceTextureShader::initialize();

    QOpenGLShaderProgram * const program = QSGMaterialShader::program();

    m_id_radius = program->uniformLocation("radius");
}

QSGMaterialType *OpaqueSurfaceTextureMaterial::type() const
{
    static QSGMaterialType type;

    return &type;
}

QSGMaterialShader *OpaqueSurfaceTextureMaterial::createShader() const
{
    return new OpaqueSurfaceTextureShader;
}

int OpaqueSurfaceTextureMaterial::compare(const QSGMaterial *other) const
{
    const OpaqueSurfaceTextureMaterial * const surface = static_cast<const OpaqueSurfaceTextureMaterial *>(other);

    return (m_texture ? m_texture->textureId() : 0)
            - (surface->m_texture ? surface->m_texture->textureId() : 0);
}

QSGMaterialType *SurfaceTextureMaterial::type() const
{
    static QSGMaterialType type;

    return &type;
}

QSGMaterialShader *SurfaceTextureMaterial::createShader() const
{
    return new SurfaceTextureShader;
}

int SurfaceTextureMaterial::compare(const QSGMaterial *other) const
{
    return opaqueMaterial.compare(&static_cast<const SurfaceTextureMaterial *>(other)->opaqueMaterial);
}


QSGMaterialType *CornerSurfaceTextureMaterial::type() const
{
    static QSGMaterialType type;

    return &type;
}

QSGMaterialShader *CornerSurfaceTextureMaterial::createShader() const
{
    return new CornerSurfaceTextureShader;
}

int CornerSurfaceTextureMaterial::compare(const QSGMaterial *other) const
{
    const CornerSurfaceTextureMaterial * const surface = static_cast<const CornerSurfaceTextureMaterial *>(other);

    const int result = SurfaceTextureMaterial::compare(other);
    if (result != 0) {
        return result;
    } else if (m_radius < surface->m_radius) {
        return -1;
    } else if (m_radius > surface->m_radius) {
        return 1;
    } else {
        return 0;
    }
}

SurfaceNode::SurfaceNode()
{
    setGeometry(&m_geometry);
    setMaterial(&m_material);
    setOpaqueMaterial(&m_opaqueMaterial);

    m_cornerNode.setGeometry(&m_cornerGeometry);
    m_cornerNode.setMaterial(&m_cornerMaterial);

    m_material.setFlag(QSGMaterial::Blending, true);
    m_cornerMaterial.setFlag(QSGMaterial::Blending, true);

    m_cornerGeometry.setDrawingMode(GL_TRIANGLES);
}

SurfaceNode::~SurfaceNode()
{
    if (m_provider && m_providerOwned)
        delete m_provider;
}

void SurfaceNode::setRect(const QRectF &r)
{
    m_geometryChanged |= m_rect != r;

    m_rect = r;
}

void SurfaceNode::setTextureProvider(QSGTextureProvider *p, bool owned)
{
    if (p == m_provider)
        return;

    if (m_provider) {
        QObject::disconnect(m_provider, SIGNAL(destroyed(QObject *)), this, SLOT(providerDestroyed()));
        QObject::disconnect(m_provider, SIGNAL(textureChanged()), this, SLOT(textureChanged()));
        m_provider = 0;
    }

    m_provider = p;
    m_providerOwned = owned;

    if (m_provider) {
        QObject::connect(m_provider, SIGNAL(destroyed(QObject *)), this, SLOT(providerDestroyed()));
        QObject::connect(m_provider, SIGNAL(textureChanged()), this, SLOT(textureChanged()));

        setTexture(m_provider->texture());
    }
}

void SurfaceNode::updateGeometry()
{
    if (m_geometryChanged && m_texture) {
        m_geometryChanged = false;

        const QSize ts = m_texture->textureSize();
        const QRectF textureRect = m_texture->convertToNormalizedSourceRect(QRectF(
                    ts.width() * m_xOffset,
                    ts.height() * m_yOffset,
                    ts.width() * m_xScale,
                    ts.height() * m_yScale));

        if (m_radius) {
            qreal radius = std::min({ m_rect.width() / 2, m_rect.height() / 2, m_radius });

            m_geometry.allocate(8);
            m_cornerGeometry.allocate(12);

            const float outerL = m_rect.left();
            const float innerL = m_rect.left() + radius;
            const float innerR = m_rect.right() - radius;
            const float outerR = m_rect.right();

            const float outerT = m_rect.top();
            const float innerT = m_rect.top() + radius;
            const float innerB = m_rect.bottom() - radius;
            const float outerB = m_rect.bottom();

            float textureXRadius = radius * textureRect.width() / m_rect.width();
            float textureYRadius = radius * textureRect.height() / m_rect.height();

            const float outerTL = textureRect.left();
            const float innerTL = textureRect.left() + textureXRadius;
            const float innerTR = textureRect.right() - textureXRadius;
            const float outerTR = textureRect.right();

            const float outerTT = textureRect.top();
            const float innerTT = textureRect.top() + textureYRadius;
            const float innerTB = textureRect.bottom() - textureYRadius;
            const float outerTB = textureRect.bottom();

            // Item rectangle with the corners clipped
            QSGGeometry::TexturedPoint2D *vertices = m_geometry.vertexDataAsTexturedPoint2D();

            vertices[0].set(outerL, innerB, outerTL, innerTB); // Outer left, inner bottom
            vertices[1].set(outerL, innerT, outerTL, innerTT); // Outer left, inner top
            vertices[2].set(innerL, outerB, innerTL, outerTB); // Inner left, outer bottom
            vertices[3].set(innerL, outerT, innerTL, outerTT); // Inner left, outer top
            vertices[4].set(innerR, outerB, innerTR, outerTB); // Inner right, outer botton
            vertices[5].set(innerR, outerT, innerTR, outerTT); // Inner right, outer top
            vertices[6].set(outerR, innerB, outerTR, innerTB); // Outer right, inner bottom
            vertices[7].set(outerR, innerT, outerTR, innerTT); // Outer right, inner top

            // Corners
            CornerVertex *corners = static_cast<CornerVertex *>(m_cornerGeometry.vertexData());

            // Bottom left
            corners[0].set(outerL, outerB, outerTL, outerTB, radius, radius);
            corners[1].set(outerL, innerB, outerTL, innerTB, radius, 0);
            corners[2].set(innerL, outerB, innerTL, outerTB, 0, radius);

            // Top left
            corners[3].set(outerL, outerT, outerTL, outerTT, radius, radius);
            corners[4].set(outerL, innerT, outerTL, innerTT, radius, 0);
            corners[5].set(innerL, outerT, innerTL, outerTT, 0, radius);

            // Bottom right
            corners[6].set(outerR, outerB, outerTR, outerTB, radius, radius);
            corners[7].set(outerR, innerB, outerTR, innerTB, radius, 0);
            corners[8].set(innerR, outerB, innerTR, outerTB, 0, radius);

            // Top right
            corners[9].set(outerR, outerT, outerTR, outerTT, radius, radius);
            corners[10].set(outerR, innerT, outerTR, innerTT, radius, 0);
            corners[11].set(innerR, outerT, innerTR, outerTT, 0, radius);

            m_cornerNode.markDirty(DirtyGeometry);
        } else {
            m_geometry.allocate(4);
            QSGGeometry::updateTexturedRectGeometry(&m_geometry, m_rect, textureRect);
        }

        markDirty(DirtyGeometry);
    }
}

void SurfaceNode::setBlending(bool b)
{
    m_opaqueMaterial.setFlag(QSGMaterial::Blending, b);
}

void SurfaceNode::setRadius(qreal radius)
{
    if (m_radius == radius)
        return;

    if (m_radius == 0 && radius != 0) {
        appendChildNode(&m_cornerNode);
    } else if (m_radius != 0 && radius == 0) {
        removeChildNode(&m_cornerNode);
    }

    m_radius = radius;
    m_geometryChanged = true;
    m_cornerMaterial.setRadius(m_radius);

    m_cornerNode.markDirty(DirtyMaterial);
}

void SurfaceNode::setTexture(QSGTexture *texture)
{
    m_opaqueMaterial.setTexture(texture);

    QRectF tr;
    if (texture) tr = texture->convertToNormalizedSourceRect(QRect(QPoint(0,0), texture->textureSize()));

    m_geometryChanged |= !m_texture || tr != m_textureRect;

    m_texture = texture;
    m_textureRect = tr;

    m_texture = texture;

    markDirty(DirtyMaterial);
    if (m_radius > 0) {
        m_cornerNode.markDirty(DirtyMaterial);
    }
}

void SurfaceNode::setXOffset(qreal offset)
{
    m_geometryChanged |= m_xOffset != offset;

    m_xOffset = offset;
}

void SurfaceNode::setYOffset(qreal offset)
{
    m_geometryChanged |= m_yOffset != offset;

    m_yOffset = offset;
}

void SurfaceNode::setXScale(qreal xScale)
{
    m_geometryChanged |= m_xScale != xScale;

    m_xScale = xScale;
}

void SurfaceNode::setYScale(qreal yScale)
{
    m_geometryChanged |= m_yScale != yScale;

    m_yScale = yScale;
}

void SurfaceNode::textureChanged()
{
    setTexture(m_provider->texture());
    updateGeometry();
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
    : m_item(0), m_id(0), m_opaque(false), m_radius(0), m_xOffset(0), m_yOffset(0)
    , m_xScale(1), m_yScale(1), m_unmapLock(0), m_hasBuffer(false), m_hasPixmap(false)
    , m_surfaceDestroyed(false), m_haveSnapshot(false)
    , m_textureProvider(0)
{
    setFlag(ItemHasContents);
}

WindowPixmapItem::~WindowPixmapItem()
{
    setWindowId(0);
}

int WindowPixmapItem::windowId() const
{
    return m_id;
}

void WindowPixmapItem::setWindowId(int id)
{
    if (m_id == id)
        return;

    QSize oldSize = windowSize();
    if (m_item) {
        disconnect(m_item.data(), &QObject::destroyed, this, &WindowPixmapItem::itemDestroyed);
        if (m_item->surface()) {
            disconnect(m_item->surface(), &QWaylandSurface::sizeChanged, this, &WindowPixmapItem::handleWindowSizeChanged);
            disconnect(m_item->surface(), &QWaylandSurface::configure, this, &WindowPixmapItem::configure);
            disconnect(m_item.data(), &QWaylandSurfaceItem::surfaceDestroyed, this, &WindowPixmapItem::surfaceDestroyed);
        }
        if (!m_surfaceDestroyed)
            m_item->imageRelease(this);
        m_item->setDelayRemove(false);
        m_item = 0;
        delete m_unmapLock;
        m_unmapLock = 0;
    }

    m_surfaceDestroyed = false;
    m_hasBuffer = false;
    m_id = id;
    updateItem();

    emit windowIdChanged();
    if (windowSize() != oldSize)
        emit windowSizeChanged();
}

void WindowPixmapItem::surfaceDestroyed()
{
    m_surfaceDestroyed = true;
    m_hasBuffer = false;
    m_unmapLock = new QWaylandUnmapLock(m_item->surface());
    m_item->imageRelease(this);
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
    if (m_item) update();

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
    if (m_item) update();

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
    if (m_item) update();

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
    if (m_item) update();

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
    if (m_item || m_haveSnapshot) update();

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
    if (m_item || m_haveSnapshot) update();

    emit yScaleChanged();
}

QSize WindowPixmapItem::windowSize() const
{
    return m_windowSize;
}

void WindowPixmapItem::setWindowSize(const QSize &s)
{
    if (!m_item || !m_item->surface()) {
        return;
    }

    m_item->surface()->requestSize(s);
}

void WindowPixmapItem::handleWindowSizeChanged()
{
    if (m_item->surface()->size().isValid()) {
        // Window size is retained even when surface is destroyed. This allows snapshots to continue
        // rendering correctly.
        m_windowSize = m_item->surface()->size();
        emit windowSizeChanged();
    }
}

void WindowPixmapItem::itemDestroyed(QObject *)
{
    m_item = 0;
    if (!m_haveSnapshot) {
        m_hasPixmap = false;
        emit hasPixmapChanged();
    }
}

QSGNode *WindowPixmapItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    SurfaceNode *node = static_cast<SurfaceNode *>(oldNode);

    if (m_item == 0 && !m_haveSnapshot) {
        if (node)
            node->setTextureProvider(0, false);
        delete node;
        return 0;
    }

    QSGTextureProvider *provider = 0;
    QSGTexture *texture = 0;
    if (m_item) {
        provider = m_item->textureProvider();
        texture = provider->texture();
        if (texture && !m_surfaceDestroyed) {
            m_haveSnapshot = false;
        }
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

        if (m_unmapLock) {
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
            delete m_unmapLock;
            m_unmapLock = 0;
            s_snapshotProgram->program.disableAttributeArray(s_snapshotProgram->vertexLocation);

            m_haveSnapshot = true;
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

    if (m_surfaceDestroyed && m_item) {
        m_item->setDelayRemove(false);
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

    node->setTextureProvider(provider, provider == m_textureProvider);
    node->setRect(QRectF(0, 0, width(), height()));
    node->setBlending(!m_opaque);
    node->setRadius(m_radius);
    node->setXOffset(m_xOffset);
    node->setYOffset(m_yOffset);
    node->setXScale(m_xScale);
    node->setYScale(m_yScale);
    node->updateGeometry();

    return node;
}

void WindowPixmapItem::updateItem()
{
    LipstickCompositor *c = LipstickCompositor::instance();
    Q_ASSERT(m_item == 0);

    if (c && m_id) {
        LipstickCompositorWindow *w = static_cast<LipstickCompositorWindow *>(c->windowForId(m_id));

        if (!w) {
            if (m_hasPixmap && !m_haveSnapshot) {
                m_hasPixmap = false;
                emit hasPixmapChanged();
            }
            return;
        } else if (w->surface()) {
            m_item = w;
            m_item->setDelayRemove(true);
            connect(m_item->surface(), &QWaylandSurface::sizeChanged, this, &WindowPixmapItem::handleWindowSizeChanged);
            connect(m_item->surface(), &QWaylandSurface::configure, this, &WindowPixmapItem::configure);
            connect(m_item.data(), &QWaylandSurfaceItem::surfaceDestroyed, this, &WindowPixmapItem::surfaceDestroyed);
            connect(m_item.data(), &QObject::destroyed, this, &WindowPixmapItem::itemDestroyed);
            m_windowSize = m_item->surface()->size();
            m_unmapLock = new QWaylandUnmapLock(m_item->surface());
        }

        w->imageAddref(this);

        update();
    }

    const bool hadPixmap = m_hasPixmap;
    m_hasPixmap = m_item || m_haveSnapshot;
    if (m_hasPixmap != hadPixmap) {
        emit hasPixmapChanged();
    }
}

void WindowPixmapItem::configure(bool hasBuffer)
{
    if (hasBuffer != m_hasBuffer) {
        m_hasBuffer = hasBuffer;
        if (m_hasBuffer && !m_unmapLock)
            m_unmapLock = new QWaylandUnmapLock(m_item->surface());

        update();
    }
}

void WindowPixmapItem::cleanupOpenGL()
{
    disconnect(window(), &QQuickWindow::sceneGraphInvalidated, this, &WindowPixmapItem::cleanupOpenGL);
    delete s_snapshotProgram;
    s_snapshotProgram = 0;
}

#include "windowpixmapitem.moc"
