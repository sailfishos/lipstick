/***************************************************************************
**
** Copyright (c) 2014 Jolla Ltd.
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

#ifndef LIPSTICKCOMPOSITORRECORDER_H
#define LIPSTICKCOMPOSITORRECORDER_H

#include <QObject>
#include <QMultiHash>
#include <QMutex>
#include <QWaylandGlobalInterface>

#include "qwayland-server-lipstick-recorder.h"

struct wl_shm_buffer;
struct wl_client;

class QWindow;
class QQuickWindow;
class QEvent;
class LipstickRecorder;

class LipstickRecorderManager : public QWaylandGlobalInterface, public QtWaylandServer::lipstick_recorder_manager
{
public:
    LipstickRecorderManager();

    const wl_interface* interface() const;

    void recordFrame(QWindow *window);
    void requestFrame(QWindow *window, LipstickRecorder *recorder);
    void remove(QWindow *window, LipstickRecorder *recorder);

protected:
    void bind(wl_client *client, quint32 version, quint32 id) override;
    void lipstick_recorder_manager_create_recorder(Resource *resource, uint32_t id, ::wl_resource *output) override;

private:
    QMultiHash<QWindow *, LipstickRecorder *> m_requests;
    QMutex m_mutex;
};

class LipstickRecorder : public QObject, public QtWaylandServer::lipstick_recorder
{
public:
    LipstickRecorder(LipstickRecorderManager *manager, wl_client *client, quint32 id, QQuickWindow *window);
    ~LipstickRecorder();

    wl_shm_buffer *buffer() const { return m_buffer; }
    wl_client *client() const { return m_client; }

protected:
    bool event(QEvent *e) override;
    void lipstick_recorder_destroy_resource(Resource *resource) override;
    void lipstick_recorder_destroy(Resource *resource) override;
    void lipstick_recorder_record_frame(Resource *resource, ::wl_resource *buffer) override;
    void lipstick_recorder_repaint(Resource *resource) override;

private:
    LipstickRecorderManager *m_manager;
    wl_resource *m_bufferResource;
    wl_shm_buffer *m_buffer;
    wl_client *m_client;
    QQuickWindow *m_window;
};

#endif
