SUMMARY = "GStreamer DRP-AI plugin"

require gstreamer1.0-drpai.inc

MESON_TARGET = "gstdrpai"
DEPENDS = "gstreamer1.0 gstreamer1.0-plugins-base drpai"
RDEPENDS_${PN} = "gstreamer1.0 gstreamer1.0-plugins-base kernel-module-udmabuf"
FILES_${PN} = "${libdir}/gstreamer-1.0/libgstdrpai.so"
FILES_${PN}-dbg = "${libdir}/gstreamer-1.0/.debug/libgstdrpai.so"

do_install_append() {
    rm -rf ${D}${libdir}/libgstdrpai-*.so
    rm -rf ${D}${libdir}/.debug/libgstdrpai-*.so
}
