#!/bin/sh
GST_PLUGIN_PATH=`dirname $0`/../src/ gst-inspect-1.0 | grep local
