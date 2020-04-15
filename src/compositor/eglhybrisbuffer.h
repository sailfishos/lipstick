/***************************************************************************
**
** Copyright (c) 2018 Jolla Ltd.
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

#ifndef EGLHYBRISBUFFER_H
#define EGLHYBRISBUFFER_H

#include "eglhybrisfunctions.h"
#include <QExplicitlySharedDataPointer>
#include <QSize>

class EglHybrisBuffer : public QSharedData, protected EglHybrisFunctions
{
public:
    using Pointer = QExplicitlySharedDataPointer<EglHybrisBuffer>;

    enum Format {
        RGBA = HYBRIS_PIXEL_FORMAT_RGBA_8888,   // QImage::Format_RGBA8888_Premultiplied
        RGBX = HYBRIS_PIXEL_FORMAT_RGBX_8888,   // QImage::Format_RGBX8888
        BGRA = HYBRIS_PIXEL_FORMAT_BGRA_8888    // QImage::Format_ARGB32_Premultiplied
    };

    enum UsageFlag {
        Read = HYBRIS_USAGE_SW_READ_RARELY,
        Write = HYBRIS_USAGE_SW_WRITE_RARELY,
        Texture = HYBRIS_USAGE_HW_TEXTURE,
        Render = HYBRIS_USAGE_HW_RENDER,
        Composer = HYBRIS_USAGE_HW_COMPOSER
    };
    Q_DECLARE_FLAGS(Usage, UsageFlag)

    EglHybrisBuffer(Format format, const QSize &size, Usage usage, const EglHybrisFunctions &functions);
    ~EglHybrisBuffer();

    void *handle() const;
    void bindToTexture();

    Format format() const { return m_format; }
    QSize size() const { return m_size; }
    Usage usage() const { return m_usage; }

    bool allocate();

    uchar *lock(Usage usage, int *stride);
    void unlock();

    QImage toImage();

    static void destroy(void *, void *data);

private:
    const QSize m_size;
    EGLClientBuffer m_buffer = nullptr;
    EGLImageKHR m_eglImage = nullptr;
    const Usage m_usage;
    EGLint m_bufferStride = 0;
    const Format m_format;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(EglHybrisBuffer::Usage)

#endif
