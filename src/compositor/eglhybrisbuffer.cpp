/***************************************************************************
**
** Copyright (c) 2018 - 2020 Jolla Ltd.
** Copyright (c) 2020 Open Mobile Platform LLC.
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

#include "eglhybrisbuffer.h"
#include <QImage>
#include "hwcrenderstage.h"
#include "logging.h"

EglHybrisBuffer::EglHybrisBuffer(
        Format format, const QSize &size, Usage usage, const EglHybrisFunctions &functions)
    : EglHybrisFunctions(functions)
    , m_size(size)
    , m_usage(usage)
    , m_format(format)
{
}

EglHybrisBuffer::~EglHybrisBuffer()
{
    if (m_eglImage) {
        eglDestroyImageKHR(eglGetDisplay(EGL_DEFAULT_DISPLAY), m_eglImage);
    }
    if (m_buffer) {
        eglHybrisReleaseNativeBuffer(m_buffer);
    }
}

void *EglHybrisBuffer::handle() const
{
    void *handle = nullptr;
    if (eglHybrisNativeBufferHandle) {
        eglHybrisNativeBufferHandle(eglGetCurrentDisplay(), m_buffer, &handle);
    }
    return handle;
}

void EglHybrisBuffer::bindToTexture()
{
    glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, m_eglImage);
}

bool EglHybrisBuffer::allocate()
{
    if (!eglHybrisCreateNativeBuffer(
                m_size.width(), m_size.height(), m_usage, m_format, &m_bufferStride, &m_buffer)
            || !m_buffer) {
        qCWarning(lcLipstickHwcLog, "EGL native buffer error");
        return false;
    } else if (!(m_eglImage = eglCreateImageKHR(
            eglGetDisplay(EGL_DEFAULT_DISPLAY),
            EGL_NO_CONTEXT,
            EGL_NATIVE_BUFFER_HYBRIS,
            m_buffer,
            0))) {
        qCWarning(lcLipstickHwcLog, "EGLImage allocation error");
        return false;
    } else {
        return true;
    }
}

uchar *EglHybrisBuffer::lock(Usage usage, int *stride)
{
    uchar *bytes = nullptr;
    if (eglHybrisLockNativeBuffer(
                m_buffer,
                usage,
                0,
                0,
                m_size.width(),
                m_size.height(),
                reinterpret_cast<void **>(&bytes))) {
        if (stride) {
            *stride = m_bufferStride * 4;
        }
        return bytes;
    }
    return nullptr;
}

void EglHybrisBuffer::unlock()
{
    eglHybrisUnlockNativeBuffer(m_buffer);
}

QImage EglHybrisBuffer::toImage()
{
    QImage image;

    QImage::Format format;
    switch (m_format) {
    case RGBA:
        format = QImage::Format_RGBA8888_Premultiplied;
        break;
    case RGBX:
        format = QImage::Format_RGBX8888;
        break;
    case BGRA:
        format = QImage::Format_ARGB32_Premultiplied;
        break;
    default:
        return image;
    }

    int stride = 0;
    if (uchar *bytes = lock(Read, &stride)) {
        image = QImage(bytes, m_size.width(), m_size.height(), stride, format).copy();
        unlock();
    }

    return image;
}

void EglHybrisBuffer::destroy(void *, void *data)
{
    EglHybrisBuffer * const buffer = static_cast<EglHybrisBuffer *>(data);

    if (!buffer->ref.deref()) {
        delete buffer;
    }
}
