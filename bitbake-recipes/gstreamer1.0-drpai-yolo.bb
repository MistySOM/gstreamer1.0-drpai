SUMMARY = "GStreamer DRP-AI plugin Yolo Library"

require gstreamer1.0-drpai.inc

MESON_TARGET = "gstdrpai-yolo"
RDEPENDS_${PN} = "gstreamer1.0-drpai"
FILES_${PN} = "${libdir}/libgstdrpai-yolo.so"
FILES_${PN}-dbg = "${libdir}/.debug/libgstdrpai-yolo.so"
