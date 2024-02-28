SUMMARY = "GStreamer DRP-AI plugin Yolo Library"

require gstreamer1.0-drpai.inc

MESON_TARGET = "gstdrpai-yolo"
DEPENDS = "gstreamer1.0 drpai"
RDEPENDS_${PN} = "gstreamer1.0-drpai"
FILES_${PN} = "${libdir}/libgstdrpai-yolo.so -${libdir}/gstreamer-1.0"
FILES_${PN}-dbg = "${libdir}/.debug/libgstdrpai-yolo.so -${libdir}/gstreamer-1.0"

do_install_append() {
    rm -rf ${D}${libdir}/gstreamer-1.0
}
