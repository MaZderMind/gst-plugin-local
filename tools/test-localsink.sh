#!/bin/sh
GST_PLUGIN_PATH=`dirname $0`/../src/ GST_DEBUG=4 gst-launch-1.0 \
	videotestsrc !\
	video/x-raw,width=800,height=450,format=UYVY !\
	localsink channel=lala
