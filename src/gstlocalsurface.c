/* GStreamer
 * Copyright (C) 2011 David A. Schleef <ds@schleef.org>
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

#include "gstlocalsurface.h"

static GList *list;
static GMutex mutex;

GstLocalSurface *
gst_local_surface_get (const char *name)
{
  GList *g;
  GstLocalSurface *surface;

  g_mutex_lock (&mutex);
  for (g = list; g; g = g_list_next (g)) {
    surface = g->data;
    if (strcmp (name, surface->name) == 0) {
      surface->ref_count++;
      g_mutex_unlock (&mutex);
      return surface;
    }
  }

  surface = g_malloc0 (sizeof (GstLocalSurface));
  surface->ref_count = 1;
  surface->name = g_strdup (name);
  surface->queue_limit = 0;
  surface->queue =
      gst_data_queue_new (gst_local_surface_queue_test_isfull, NULL, NULL,
      surface);

  g_mutex_init (&surface->mutex);
  list = g_list_append (list, surface);
  g_mutex_unlock (&mutex);

  return surface;
}

gboolean
gst_local_surface_queue_test_isfull (GstDataQueue * queue,
    guint visible, guint bytes, guint64 time, gpointer checkdata)
{
  GstLocalSurface *surface = checkdata;
  return (surface->queue_limit != 0) && (visible >= surface->queue_limit);
}

guint
gst_local_surface_queue_level (GstLocalSurface * surface)
{
  GstDataQueueSize level;
  gst_data_queue_get_level (surface->queue, &level);
  return level.visible;
}

void
gst_local_surface_queue_push (GstLocalSurface * surface, GstBuffer * buffer)
{
  GstDataQueueItem *item = g_slice_new (GstDataQueueItem);

  item->destroy = (GDestroyNotify) gst_local_surface_audio_queue_free;

  gst_buffer_ref (buffer);
  item->object = GST_MINI_OBJECT (buffer);
  item->size = gst_buffer_get_size (buffer);
  item->duration = GST_BUFFER_DURATION (buffer);

  if (item->duration == GST_CLOCK_TIME_NONE) {
    item->duration = 0;
  }

  item->visible = TRUE;

  if (!gst_data_queue_push (surface->queue, item)) {
    item->destroy (item);
  }
}

GstBuffer *
gst_local_surface_queue_pop (GstLocalSurface * surface)
{
  GstDataQueueItem *item = NULL;

  if (gst_data_queue_is_empty (surface->queue))
    return NULL;

  if (!gst_data_queue_pop (surface->queue, &item))
    return NULL;

  if (item == NULL)
    return NULL;

  g_assert (GST_IS_BUFFER (item->object));
  return GST_BUFFER (item->object);
}

void
gst_local_surface_audio_queue_free (GstDataQueueItem * item)
{
  if (item->object) {
    gst_mini_object_unref (item->object);
  }

  g_slice_free (GstDataQueueItem, item);
}

void
gst_local_surface_unref (GstLocalSurface * surface)
{
  /* Mutex needed here, otherwise refcount might become 0
   * and someone else requests the same surface again before
   * we remove it from the list */
  g_mutex_lock (&mutex);
  if ((--surface->ref_count) == 0) {
    GList *g;

    for (g = list; g; g = g_list_next (g)) {
      GstLocalSurface *tmp = g->data;
      if (strcmp (tmp->name, surface->name) == 0) {
        list = g_list_delete_link (list, g);
        break;
      }
    }

    g_mutex_clear (&surface->mutex);
    gst_object_unref (surface->queue);
    g_free (surface->name);
    g_free (surface);
  }
  g_mutex_unlock (&mutex);
}
