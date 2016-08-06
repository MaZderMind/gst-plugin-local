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

#include "gstlocalsink.h"

static gboolean
plugin_init (GstPlugin * plugin)
{
  gst_element_register (plugin, "localsink", GST_RANK_NONE,
      GST_TYPE_LOCAL_SINK);

  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    local,
    "plugin for timestamp aware inter-pipeline communication",
    plugin_init, VERSION, "LGPL", PACKAGE_NAME, "https://github.com/MaZderMind/gst-plugin-local")
