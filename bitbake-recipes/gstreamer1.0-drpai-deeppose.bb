SUMMARY = "GStreamer DRP-AI plugin DeepPose Library"

require gstreamer1.0-drpai.inc

MESON_TARGET = "gstdrpai-deeppose"
EXTRA_OEMESON += "-Denable-deeppose=true"
DEPENDS = "gstreamer1.0 drpai opencv"
RDEPENDS_${PN} = "gstreamer1.0-drpai opencv"
FILES_${PN} = "${libdir}/libgstdrpai-deeppose.so"
FILES_${PN}-dbg = "${libdir}/.debug/libgstdrpai-deeppose.so"
