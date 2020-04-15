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

#ifndef EGLHYBRISFUNCTIONS_H
#define EGLHYBRISFUNCTIONS_H

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <QOpenGLFunctions>

// from hybris_nativebuffer.h in libhybris
#define HYBRIS_USAGE_SW_READ_RARELY     0x00000002
#define HYBRIS_USAGE_SW_WRITE_RARELY    0x00000020
#define HYBRIS_USAGE_HW_TEXTURE         0x00000100
#define HYBRIS_USAGE_HW_RENDER          0x00000200
#define HYBRIS_USAGE_HW_COMPOSER        0x00000800
#define HYBRIS_PIXEL_FORMAT_RGBA_8888   1
#define HYBRIS_PIXEL_FORMAT_RGBX_8888   2
#define HYBRIS_PIXEL_FORMAT_RGB_888     3
#define HYBRIS_PIXEL_FORMAT_BGRA_8888   5

#define EGL_NATIVE_BUFFER_HYBRIS        0x3140
struct EglHybrisFunctions
{
    typedef EGLBoolean (EGLAPIENTRYP _eglHybrisCreateNativeBuffer)(EGLint width, EGLint height, EGLint usage, EGLint format, EGLint *stride, EGLClientBuffer *buffer);
    typedef EGLBoolean (EGLAPIENTRYP _eglHybrisLockNativeBuffer)(EGLClientBuffer buffer, EGLint usage, EGLint l, EGLint t, EGLint w, EGLint h, void **vaddr);
    typedef EGLBoolean (EGLAPIENTRYP _eglHybrisUnlockNativeBuffer)(EGLClientBuffer buffer);
    typedef EGLBoolean (EGLAPIENTRYP _eglHybrisReleaseNativeBuffer)(EGLClientBuffer buffer);
    typedef EGLBoolean (EGLAPIENTRYP _eglHybrisNativeBufferHandle)(EGLDisplay dpy, EGLClientBuffer buffer, void **handle);

    typedef void (EGLAPIENTRYP _glEGLImageTargetTexture2DOES)(GLenum target, EGLImageKHR image);
    typedef EGLImageKHR (EGLAPIENTRYP _eglCreateImageKHR)(EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLint *attribs);
    typedef EGLBoolean (EGLAPIENTRYP _eglDestroyImageKHR)(EGLDisplay dpy, EGLImageKHR image);

    const _glEGLImageTargetTexture2DOES glEGLImageTargetTexture2DOES = reinterpret_cast<_glEGLImageTargetTexture2DOES>(eglGetProcAddress("glEGLImageTargetTexture2DOES"));
    const _eglCreateImageKHR eglCreateImageKHR = reinterpret_cast<_eglCreateImageKHR>(eglGetProcAddress("eglCreateImageKHR"));
    const _eglDestroyImageKHR eglDestroyImageKHR = reinterpret_cast<_eglDestroyImageKHR>(eglGetProcAddress("eglDestroyImageKHR"));
    const _eglHybrisCreateNativeBuffer eglHybrisCreateNativeBuffer = reinterpret_cast<_eglHybrisCreateNativeBuffer>(eglGetProcAddress("eglHybrisCreateNativeBuffer"));
    const _eglHybrisLockNativeBuffer eglHybrisLockNativeBuffer = reinterpret_cast<_eglHybrisLockNativeBuffer>(eglGetProcAddress("eglHybrisLockNativeBuffer"));
    const _eglHybrisUnlockNativeBuffer eglHybrisUnlockNativeBuffer = reinterpret_cast<_eglHybrisUnlockNativeBuffer>(eglGetProcAddress("eglHybrisUnlockNativeBuffer"));
    const _eglHybrisReleaseNativeBuffer eglHybrisReleaseNativeBuffer = reinterpret_cast<_eglHybrisReleaseNativeBuffer>(eglGetProcAddress("eglHybrisReleaseNativeBuffer"));
    const _eglHybrisNativeBufferHandle eglHybrisNativeBufferHandle = reinterpret_cast<_eglHybrisNativeBufferHandle>(eglGetProcAddress("eglHybrisNativeBufferHandle"));

    static const EglHybrisFunctions *instance();
};

#endif
