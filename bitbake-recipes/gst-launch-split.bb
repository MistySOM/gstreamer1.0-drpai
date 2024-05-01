SUMMARY = "GStreamer Pipeline Splitter"

require gstreamer1.0-drpai.inc

MESON_TARGET = "gst-launch-split"
RDEPENDS_${PN} = "gstreamer1.0 gstreamer1.0-plugins-bad"
FILES_${PN} = "${bindir}/gst-launch-split"
FILES_${PN}-dbg = "${bindir}/.debug/gst-launch-split"
