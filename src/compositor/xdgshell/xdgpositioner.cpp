/***************************************************************************
**
** Copyright (c) 2026 Affe Null <affenull2345@gmail.com>
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

#include "lipstickcompositor.h"
#include "xdgshell.h"
#include "xdgpositioner.h"
#include "xdgsurface.h"
#include "xdgtoplevel.h"

XdgPositioner::XdgPositioner(XdgShell *mgr, wl_client *client, uint32_t version, uint32_t id)
    : QObject(mgr)
    , QtWaylandServer::xdg_positioner(client, id, version)
    , m_anchor(0)
    , m_gravity(0)
    , m_adjustment(XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_NONE)
{
}

XdgPositioner::~XdgPositioner()
{
    wl_resource_set_implementation(resource()->handle, nullptr, nullptr, nullptr);
}

XdgPositioner *XdgPositioner::fromResource(::wl_resource *resource)
{
    return static_cast<XdgPositioner *>(Resource::fromResource(resource)->xdg_positioner_object);
}

QRect XdgPositioner::toRect(const XdgSurface *surface) const
{
    if (m_size.isNull() || m_anchorRect.isNull())
        return QRect();

    QPointF offset(0.0, 0.0);
    XdgToplevel *top = surface->findToplevel(&offset);
    if (!top)
        return QRect();

    LipstickCompositor *compositor = LipstickCompositor::instance();
    QSizeF size = QSizeF(compositor->width(), compositor->height()) / top->scale();
    QRect bounds = QRectF(offset, size).toRect();

    uint32_t adjustment = m_adjustment;
    Qt::Edges anchor, gravity;
    Qt::Orientations flip = 0;

    QRect rect;

    for (;;) {
        int x = m_offset.x(), y = m_offset.y();

        anchor = flipEdges(m_anchor, flip);
        gravity = flipEdges(m_gravity, flip);

        if (anchor & Qt::TopEdge) {
            y += m_anchorRect.top();
        } else if (anchor & Qt::BottomEdge) {
            y += m_anchorRect.bottom() + 1;
        } else {
            y += m_anchorRect.top() + m_anchorRect.height() / 2;
        }

        if (anchor & Qt::LeftEdge) {
            x += m_anchorRect.left();
        } else if (anchor & Qt::RightEdge) {
            x += m_anchorRect.right() + 1;
        } else {
            x += m_anchorRect.left() + m_anchorRect.width() / 2;
        }

        if (gravity & Qt::TopEdge) {
            y -= m_size.height();
        } else if (!(m_gravity & Qt::BottomEdge)) {
            y -= m_size.height() / 2;
        }

        if (gravity & Qt::LeftEdge) {
            x -= m_size.width();
        } else if (!(gravity & Qt::RightEdge)) {
            x -= m_size.width() / 2;
        }

        rect.setRect(x, y, m_size.width(), m_size.height());

        // If the surface is contrained on a flippable axis, invert the anchor
        // and gravity on that axis and try again. If this is the second
        // attempt, revert to the original position.

        if (rect.left() < bounds.left() || rect.right() > bounds.right()) {
            if (adjustment & XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_FLIP_X) {
                adjustment &= ~XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_FLIP_X;
                flip |= Qt::Horizontal;
                continue;
            } else if (flip & Qt::Horizontal) {
                flip &= ~Qt::Horizontal;
                continue;
            }
        }

        if (rect.top() < bounds.top() || rect.bottom() > bounds.bottom()) {
            if (adjustment & XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_FLIP_Y) {
                adjustment &= ~XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_FLIP_Y;
                flip |= Qt::Vertical;
                continue;
            } else if (flip & Qt::Vertical) {
                flip &= ~Qt::Vertical;
                continue;
            }
        }

        // If no flips are left to try, proceed with other options.
        break;
    }

    // Try to move the surface so that at least one edge is unconstrained.
    // Prefer movement in direction of gravity.

    if (adjustment & XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_SLIDE_X) {
        if (gravity & Qt::LeftEdge) {
            if (rect.left() < bounds.left()) {
                rect.moveLeft(bounds.left());
            }
        }
        if (rect.right() > bounds.right()) {
            rect.moveRight(bounds.right());
        }
        if (!(gravity & Qt::LeftEdge)) {
            if (rect.left() < bounds.left()) {
                rect.moveLeft(bounds.left());
            }
        }
    }

    if (adjustment & XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_SLIDE_Y) {
        if (gravity & Qt::TopEdge) {
            if (rect.top() < bounds.top()) {
                rect.moveTop(bounds.top());
            }
        }
        if (rect.bottom() > bounds.bottom()) {
            rect.moveBottom(bounds.bottom());
        }
        if (!(gravity & Qt::TopEdge)) {
            if (rect.top() < bounds.top()) {
                rect.moveTop(bounds.top());
            }
        }
    }

    // If allowed, resize the surface to make it unconstrained.
    QRect resized = bounds & rect;
    if (adjustment & XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_RESIZE_X) {
        rect.setLeft(resized.left());
        rect.setRight(resized.right());
    }
    if (adjustment & XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_RESIZE_Y) {
        rect.setTop(resized.top());
        rect.setBottom(resized.bottom());
    }

    return rect;
}

void XdgPositioner::xdg_positioner_destroy_resource(Resource *resource)
{
    Q_UNUSED(resource);
    delete this;
}

void XdgPositioner::xdg_positioner_destroy(Resource *resource)
{
    wl_resource_destroy(resource->handle);
}

void XdgPositioner::xdg_positioner_set_size(Resource *resource, int32_t width, int32_t height)
{
    Q_UNUSED(resource);
    m_size = QSize(width, height);
}

void XdgPositioner::xdg_positioner_set_anchor_rect(Resource *resource, int32_t x, int32_t y, int32_t width, int32_t height)
{
    Q_UNUSED(resource);
    m_anchorRect.setRect(x, y, width, height);
}

void XdgPositioner::xdg_positioner_set_anchor(Resource *resource, uint32_t anchor)
{
    Q_UNUSED(resource);
    m_anchor = toEdges(anchor);
}

void XdgPositioner::xdg_positioner_set_gravity(Resource *resource, uint32_t gravity)
{
    Q_UNUSED(resource);
    m_gravity = toEdges(gravity);
}

void XdgPositioner::xdg_positioner_set_constraint_adjustment(Resource *resource, uint32_t adjustment)
{
    Q_UNUSED(resource);
    m_adjustment = adjustment;
}

void XdgPositioner::xdg_positioner_set_offset(Resource *resource, int32_t x, int32_t y)
{
    Q_UNUSED(resource);
    m_offset = QPoint(x, y);
}

Qt::Edges XdgPositioner::toEdges(uint32_t anchor)
{
    Qt::Edges edges = 0;

    if (anchor == XDG_POSITIONER_ANCHOR_TOP ||
            anchor == XDG_POSITIONER_ANCHOR_TOP_LEFT ||
            anchor == XDG_POSITIONER_ANCHOR_TOP_RIGHT) {
        edges |= Qt::TopEdge;
    }

    if (anchor == XDG_POSITIONER_ANCHOR_BOTTOM ||
            anchor == XDG_POSITIONER_ANCHOR_BOTTOM_LEFT ||
            anchor == XDG_POSITIONER_ANCHOR_BOTTOM_RIGHT) {
        edges |= Qt::BottomEdge;
    }

    if (anchor == XDG_POSITIONER_ANCHOR_LEFT ||
            anchor == XDG_POSITIONER_ANCHOR_TOP_LEFT ||
            anchor == XDG_POSITIONER_ANCHOR_BOTTOM_LEFT) {
        edges |= Qt::LeftEdge;
    }

    if (anchor == XDG_POSITIONER_ANCHOR_RIGHT ||
            anchor == XDG_POSITIONER_ANCHOR_TOP_RIGHT ||
            anchor == XDG_POSITIONER_ANCHOR_BOTTOM_RIGHT) {
        edges |= Qt::RightEdge;
    }

    return edges;
}

Qt::Edges XdgPositioner::flipEdges(Qt::Edges edges, Qt::Orientations flip)
{
    Qt::Edges transformed = 0;

    if (flip & Qt::Vertical) {
        if (edges & Qt::TopEdge)
            transformed |= Qt::BottomEdge;
        if (edges & Qt::BottomEdge)
            transformed |= Qt::TopEdge;
    } else {
        transformed |= edges & (Qt::TopEdge | Qt::BottomEdge);
    }

    if (flip & Qt::Horizontal) {
        if (edges & Qt::LeftEdge)
            transformed |= Qt::RightEdge;
        if (edges & Qt::RightEdge)
            transformed |= Qt::LeftEdge;
    } else {
        transformed |= edges & (Qt::LeftEdge | Qt::RightEdge);
    }

    return transformed;
}
