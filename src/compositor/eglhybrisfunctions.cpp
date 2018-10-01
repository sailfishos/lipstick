/***************************************************************************
**
** Copyright (C) 2018 Jolla Ltd.
** Contact: Andrew den Exter <andrew.den.exter@jolla.com>
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

#include "eglhybrisfunctions.h"

#include <QGlobalStatic>

struct SupportedEglHybrisFunctions : public EglHybrisFunctions
{
    const bool supported = strstr(eglQueryString(eglGetDisplay(EGL_DEFAULT_DISPLAY), EGL_EXTENSIONS), "EGL_HYBRIS_native_buffer2") != 0
            && glEGLImageTargetTexture2DOES
            && eglCreateImageKHR
            && eglDestroyImageKHR
            && eglHybrisCreateNativeBuffer
            && eglHybrisLockNativeBuffer
            && eglHybrisUnlockNativeBuffer
            && eglHybrisReleaseNativeBuffer
            && eglHybrisNativeBufferHandle;
};

Q_GLOBAL_STATIC(SupportedEglHybrisFunctions, eglFunctions)

const EglHybrisFunctions *EglHybrisFunctions::instance()
{
    SupportedEglHybrisFunctions * const functions = eglFunctions();
    return functions->supported ? functions : nullptr;
}
