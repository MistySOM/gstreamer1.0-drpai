SUMMARY = "GStreamer DRP-AI plugin DeepPose Library"

require gstreamer1.0-drpai.inc

MESON_TARGET = "gstdrpai-deeppose"
DEPENDS = "gstreamer1.0 drpai opencv"
RDEPENDS_${PN} = "gstreamer1.0-drpai opencv"
FILES_${PN} = "${libdir}/libgstdrpai-deeppose.so"
FILES_${PN}-dbg = "${libdir}/.debug/libgstdrpai-deeppose.so"

do_install_append() {
    rm -rf ${D}${libdir}/gstreamer-1.0
}
