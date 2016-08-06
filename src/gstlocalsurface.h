/* GStreamer
 * Copyright (C) 2015 Peter Körner <peter@mazdermind.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef _GST_LOCAL_SURFACE_H_
#define _GST_LOCAL_SURFACE_H_

#include <gst/base/gstdataqueue.h>
#include <gst/audio/audio.h>
#include <gst/video/video.h>

#define GST_LOCAL_SURFACE_TYPE_UNDEFINED 0
#define GST_LOCAL_SURFACE_TYPE_VIDEO 1
#define GST_LOCAL_SURFACE_TYPE_AUDIO 2

// undefined -> waitforsrc/sink -> buffering -> running
#define GST_LOCAL_SURFACE_STATE_UNDEFINED 0
#define GST_LOCAL_SURFACE_STATE_WAITFORSRC 1
#define GST_LOCAL_SURFACE_STATE_WAITFORSINK 2
#define GST_LOCAL_SURFACE_STATE_BUFFERING 3
#define GST_LOCAL_SURFACE_STATE_RUNNING 4
#define GST_LOCAL_SURFACE_STATE_UNDERFLOW 5

G_BEGIN_DECLS typedef struct _GstLocalSurface GstLocalSurface;

struct _GstLocalSurface
{
  GMutex mutex;
  gint ref_count;

  char *name;
  guint state;
  guint type;
  guint queue_limit;

  GstVideoInfo video_info;
  GstAudioInfo audio_info;

  GstDataQueue *queue;
};

GstLocalSurface *gst_local_surface_get (const char *name);

void gst_local_surface_unref (GstLocalSurface * surface);

gboolean
gst_local_surface_queue_test_isfull (GstDataQueue * queue,
    guint visible, guint bytes, guint64 time, gpointer checkdata);

guint gst_local_surface_queue_level (GstLocalSurface * surface);

void gst_local_surface_audio_queue_free (GstDataQueueItem * item);

void
gst_local_surface_queue_push (GstLocalSurface * surface, GstBuffer * buffer);

GstBuffer *gst_local_surface_queue_pop (GstLocalSurface * surface);

G_END_DECLS
#endif
