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
 * Free Software Foundation, Inc., 51 Franklin Street, Suite 500,
 * Boston, MA 02110-1335, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include "gstlocalsink.h"

GST_DEBUG_CATEGORY_STATIC (gst_local_sink_debug_category);
#define GST_CAT_DEFAULT gst_local_sink_debug_category


/* prototypes */
static void gst_local_sink_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_local_sink_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
static void gst_local_sink_finalize (GObject * object);

static void gst_local_sink_get_times (GstBaseSink * sink,
    GstBuffer * buffer, GstClockTime * start, GstClockTime * end);
static gboolean gst_local_sink_start (GstBaseSink * sink);
static gboolean gst_local_sink_stop (GstBaseSink * sink);
static gboolean gst_local_sink_set_caps (GstBaseSink * sink, GstCaps * caps);
static GstFlowReturn gst_local_sink_render (GstBaseSink * sink,
    GstBuffer * buffer);

enum
{
  PROP_0,
  PROP_CHANNEL,
  PROP_JITTERBUFFER
};

#define DEFAULT_CHANNEL ("default")
#define DEFAULT_JITTERBUFFER (50)
#define AVCAPS (GST_VIDEO_CAPS_MAKE (GST_VIDEO_FORMATS_ALL) ";" \
  GST_AUDIO_CAPS_MAKE (GST_AUDIO_FORMATS_ALL))


/* pad templates */
static GstStaticPadTemplate gst_local_sink_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (AVCAPS));

/* class initialization */
G_DEFINE_TYPE (GstLocalSink, gst_local_sink, GST_TYPE_BASE_SINK);


/* element boilerplate */
static void
gst_local_sink_class_init (GstLocalSinkClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);
  GstBaseSinkClass *base_sink_class = GST_BASE_SINK_CLASS (klass);

  GST_DEBUG_CATEGORY_INIT (gst_local_sink_debug_category,
      "localsink", 0, "debug category for localsink element");

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_local_sink_sink_template));

  gst_element_class_set_static_metadata (element_class,
      "inter-pipeline communication sink",
      "Sink",
      "virtual sink for timestamp aware inter-pipeline communication",
      "Peter Körner <peter@mazdermind.de>");

  gobject_class->set_property = gst_local_sink_set_property;
  gobject_class->get_property = gst_local_sink_get_property;
  gobject_class->finalize = gst_local_sink_finalize;
  base_sink_class->start = GST_DEBUG_FUNCPTR (gst_local_sink_start);
  base_sink_class->set_caps = GST_DEBUG_FUNCPTR (gst_local_sink_set_caps);
  base_sink_class->get_times = GST_DEBUG_FUNCPTR (gst_local_sink_get_times);
  base_sink_class->render = GST_DEBUG_FUNCPTR (gst_local_sink_render);
  base_sink_class->stop = GST_DEBUG_FUNCPTR (gst_local_sink_stop);

  g_object_class_install_property (gobject_class, PROP_CHANNEL,
      g_param_spec_string ("channel", "Channel",
          "Channel name to match local src and sink elements",
          DEFAULT_CHANNEL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_JITTERBUFFER,
      g_param_spec_uint ("jitter-buffer", "Jitter-Buffer",
          "Number of Buffers to keep in the Jitter-Buffer between the sink and the source, 0 for infinity",
          0, G_MAXUINT, DEFAULT_JITTERBUFFER,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
gst_local_sink_init (GstLocalSink * localsink)
{
  localsink->channel = g_strdup (DEFAULT_CHANNEL);
  GST_DEBUG_OBJECT (localsink, "init with default channel-name \"%s\"",
      localsink->channel);
  localsink->queue_limit = DEFAULT_JITTERBUFFER;
  GST_DEBUG_OBJECT (localsink, "init with default jitter-buffer-limit %u",
      localsink->queue_limit);
}

void
gst_local_sink_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstLocalSink *localsink = GST_LOCAL_SINK (object);

  switch (property_id) {
    case PROP_CHANNEL:
      g_free (localsink->channel);
      localsink->channel = g_value_dup_string (value);
      GST_DEBUG_OBJECT (localsink, "setting channel-name to %s",
          localsink->channel);
      break;
    case PROP_JITTERBUFFER:
      localsink->queue_limit = g_value_get_uint (value);
      GST_DEBUG_OBJECT (localsink, "setting jitter-buffer-limit to %u",
          localsink->queue_limit);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_local_sink_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstLocalSink *localsink = GST_LOCAL_SINK (object);

  switch (property_id) {
    case PROP_CHANNEL:
      g_value_set_string (value, localsink->channel);
      break;
    case PROP_JITTERBUFFER:
      g_value_set_uint (value, localsink->queue_limit);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_local_sink_finalize (GObject * object)
{
  GstLocalSink *localsink = GST_LOCAL_SINK (object);
  GST_DEBUG_OBJECT (localsink, "finalize");

  /* clean up object here */
  g_free (localsink->channel);

  G_OBJECT_CLASS (gst_local_sink_parent_class)->finalize (object);
}


/* basesink function implementations */
static gboolean
gst_local_sink_start (GstBaseSink * sink)
{
  GstLocalSink *localsink = GST_LOCAL_SINK (sink);
  GST_DEBUG_OBJECT (localsink, "start");

  localsink->surface = gst_local_surface_get (localsink->channel);
  GST_DEBUG_OBJECT (localsink,
      "selected surface at %p for channel-name %s", localsink->surface,
      localsink->channel);

  g_mutex_lock (&localsink->surface->mutex);
  localsink->surface->queue_limit = localsink->queue_limit;
  if (localsink->surface->state == GST_LOCAL_SURFACE_STATE_UNDEFINED) {
    GST_DEBUG_OBJECT (localsink,
        "surface is in state UNDEFINED, changing state to WAITFORSRC");

    localsink->surface->state = GST_LOCAL_SURFACE_STATE_WAITFORSRC;
  } else if (localsink->surface->state == GST_LOCAL_SURFACE_STATE_WAITFORSINK) {
    GST_DEBUG_OBJECT (localsink,
        "surface is in state WAITFORSINK, changing state to BUFFERING");

    localsink->surface->state = GST_LOCAL_SURFACE_STATE_BUFFERING;
  } else {
    GST_ERROR_OBJECT (localsink,
        "surface is already connected, failing to connect another sink to it");
    g_mutex_unlock (&localsink->surface->mutex);
    return FALSE;
  }

  GST_DEBUG_OBJECT (localsink,
      "successfully connected to surface at %p for channel-name %s",
      localsink->surface, localsink->channel);

  g_mutex_unlock (&localsink->surface->mutex);
  return TRUE;
}

static gboolean
gst_local_sink_set_caps (GstBaseSink * sink, GstCaps * caps)
{
  GstLocalSink *localsink = GST_LOCAL_SINK (sink);

  GstStructure *structure = gst_caps_get_structure (caps, 0);

  if (g_str_has_prefix (gst_structure_get_name (structure), "video/")) {
    GST_DEBUG_OBJECT (sink, "detected incoming caps as video");
    localsink->type = GST_LOCAL_SURFACE_TYPE_VIDEO;

    if (!gst_video_info_from_caps (&localsink->video_info, caps)) {
      GST_ERROR_OBJECT (sink,
          "failed to parse caps to video_info: %" GST_PTR_FORMAT, caps);
      return FALSE;
    }
  }

  else if (g_str_has_prefix (gst_structure_get_name (structure), "audio/")) {
    GST_DEBUG_OBJECT (sink, "detected incoming caps as audio");
    localsink->type = GST_LOCAL_SURFACE_TYPE_AUDIO;

    if (!gst_audio_info_from_caps (&localsink->audio_info, caps)) {
      GST_ERROR_OBJECT (sink,
          "failed to parse caps to audio_info: %" GST_PTR_FORMAT, caps);
      return FALSE;
    }
  }

  else {
    GST_ERROR_OBJECT (sink, "detected totally bogus caps: %" GST_PTR_FORMAT,
        caps);
    return FALSE;
  }

  g_mutex_lock (&localsink->surface->mutex);
  localsink->surface->video_info = localsink->video_info;
  localsink->surface->audio_info = localsink->audio_info;
  localsink->surface->type = localsink->type;
  g_mutex_unlock (&localsink->surface->mutex);

  return TRUE;
}

static void
gst_local_sink_get_times (GstBaseSink * sink, GstBuffer * buffer,
    GstClockTime * start, GstClockTime * end)
{
  GstLocalSink *localsink = GST_LOCAL_SINK (sink);

  if (GST_BUFFER_TIMESTAMP_IS_VALID (buffer)) {
    *start = GST_BUFFER_TIMESTAMP (buffer);

    if (GST_BUFFER_DURATION_IS_VALID (buffer)) {
      *end = *start + GST_BUFFER_DURATION (buffer);
    }

    else if (localsink->type == GST_LOCAL_SURFACE_TYPE_VIDEO) {
      GST_WARNING_OBJECT (sink,
          "estimating buffer duration based on video-framerate");

      if (localsink->video_info.fps_n > 0) {
        *end = *start +
            gst_util_uint64_scale_int (GST_SECOND, localsink->video_info.fps_d,
            localsink->video_info.fps_n);
      }
    }

    else if (localsink->type == GST_LOCAL_SURFACE_TYPE_AUDIO) {
      GST_WARNING_OBJECT (sink,
          "estimating buffer duration based on audio-samplerate");

      if (localsink->audio_info.rate > 0) {
        *end = *start +
            gst_util_uint64_scale_int (gst_buffer_get_size (buffer), GST_SECOND,
            localsink->audio_info.rate * localsink->audio_info.bpf);
      }
    }
  }
}

static GstFlowReturn
gst_local_sink_render (GstBaseSink * sink, GstBuffer * buffer)
{
  GstLocalSink *localsink = GST_LOCAL_SINK (sink);

  // maybe that lock is not needed and may harm the performance a lot
  g_mutex_lock (&localsink->surface->mutex);

  if (localsink->surface->state == GST_LOCAL_SURFACE_STATE_BUFFERING ||
      localsink->surface->state == GST_LOCAL_SURFACE_STATE_RUNNING) {

    GST_DEBUG_OBJECT (localsink,
        "pushing a buffer onto the jitter-buffer. level is currently at %u buffers, limit is %u",
        gst_local_surface_queue_level (localsink->surface),
        localsink->queue_limit);

    gst_local_surface_queue_push (localsink->surface, buffer);
  } else {

    GST_INFO_OBJECT (localsink,
        "surface it not connected to a source, throwing away buffer.");

  }

  g_mutex_unlock (&localsink->surface->mutex);

  return GST_FLOW_OK;
}

static gboolean
gst_local_sink_stop (GstBaseSink * sink)
{
  GstLocalSink *localsink = GST_LOCAL_SINK (sink);

  g_mutex_lock (&localsink->surface->mutex);

  if (localsink->surface->state == GST_LOCAL_SURFACE_STATE_BUFFERING) {
    GST_DEBUG_OBJECT (localsink,
        "surface is in state BUFFERING, changing state to WAITFORSINK");

    localsink->surface->state = GST_LOCAL_SURFACE_STATE_WAITFORSINK;
  } else if (localsink->surface->state == GST_LOCAL_SURFACE_STATE_WAITFORSRC) {
    GST_DEBUG_OBJECT (localsink,
        "surface is in state WAITFORSRC, changing state to UNDEFINED");

    localsink->surface->state = GST_LOCAL_SURFACE_STATE_UNDEFINED;
  }

  g_mutex_unlock (&localsink->surface->mutex);
  gst_local_surface_unref (localsink->surface);
  localsink->surface = NULL;

  return TRUE;
}
