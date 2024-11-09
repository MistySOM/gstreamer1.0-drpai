SUMMARY = "GStreamer DRP-AI plugin"

require gstreamer1.0-drpai.inc

MESON_TARGET = "gstdrpai"
MESONOPTS += " -Dtvm=enabled"
DEPENDS += " gstreamer1.0-plugins-base"
RDEPENDS_${PN} = "\
  gstreamer1.0 \
  gstreamer1.0-plugins-base \
  kernel-module-udmabuf \
  libtvm_runtime.so()(64bit) \
"
FILES_${PN} = "\
  ${libdir}/gstreamer-1.0/libgstdrpai.so \
  ${libdir}/libtvm_runtime.so \
"
FILES_${PN}-dbg = "${libdir}/gstreamer-1.0/.debug/libgstdrpai.so"
RPROVIDES_${PN} += " libtvm_runtime.so()(64bit)"
