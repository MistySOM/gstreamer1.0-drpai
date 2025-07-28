SECTION = "multimedia"
LICENSE = "MIT"
SRC_URI = "git://github.com/MistySOM/gstreamer1.0-drpai.git;branch=master"
SRCREV = "${AUTOREV}"
LIC_FILES_CHKSUM = "file://LICENSE.md;md5=546bb90dc9b7cbf2b99de6cc06051bf9"
DEPENDS = "gstreamer1.0 drpai"

inherit meson
MESON_BUILDTYPE = "release"

S = "${WORKDIR}/git"
PV = "1.0"

PACKAGES = "${PN} ${PN}-dbg"

MESONOPTS += " -Dtvm=enabled"
DEPENDS = "gstreamer1.0-plugins-base"
RDEPENDS_${PN} = "\
  gstreamer1.0 \
  gstreamer1.0-plugins-base \
  kernel-module-udmabuf \
  libtvm_runtime \
"
FILES_${PN} = "\
  ${libdir}/gstreamer-1.0/libgstdrpai.so \
  ${libdir}/libtvm_runtime.so \
"
FILES_${PN}-dbg = "${libdir}/gstreamer-1.0/.debug/libgstdrpai.so"
RPROVIDES_${PN} += " libtvm_runtime"


PACKAGES += " ${PN}-yolo ${PN}-yolo-dbg"
PROVIDES += " ${PN}-yolo"
RDEPENDS_${PN}-yolo = "${PN}"
FILES_${PN}-yolo = "${libdir}/libgstdrpai-yolo.so"
FILES_${PN}-yolo-dbg = "${libdir}/.debug/libgstdrpai-yolo.so"

PACKAGES += " ${PN}-dummy ${PN}-dummy-dbg"
PROVIDES += " ${PN}-dummy"
RDEPENDS_${PN}-dummy = "${PN}"
FILES_${PN}-dummy = "${libdir}/libgstdrpai-dummy.so"
FILES_${PN}-dummy-dbg = "${libdir}/.debug/libgstdrpai-dummy.so"

PACKAGES += " ${PN}-mobilenet ${PN}-mobilenet-dbg"
PROVIDES += " ${PN}-mobilenet"
RDEPENDS_${PN}-mobilenet = "${PN}"
FILES_${PN}-mobilenet = "${libdir}/libgstdrpai-mobilenet.so"
FILES_${PN}-mobilenet-dbg = "${libdir}/.debug/libgstdrpai-mobilenet.so"


PACKAGES += " gst-launch-split gst-launch-split-dbg"
PROVIDES += " gst-launch-split"
RDEPENDS_gst-launch-split = "gstreamer1.0 gstreamer1.0-plugins-bad"
FILES_gst-launch-split = "${bindir}/gst-launch-split"
FILES_gst-launch-split-dbg = "${bindir}/.debug/gst-launch-split"
