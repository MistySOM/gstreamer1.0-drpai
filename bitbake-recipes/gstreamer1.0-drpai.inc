SECTION = "multimedia"
LICENSE = "MIT"
SRC_URI = "git://github.com/MistySOM/gstreamer1.0-drpai.git;branch=master"
SRCREV = "${AUTOREV}"
LIC_FILES_CHKSUM = "file://LICENSE.md;md5=546bb90dc9b7cbf2b99de6cc06051bf9"

do_unpack_append() {
    os.remove("{0}/gst-plugin/src/linux/drpai.h".format(d.getVar("S")))
}

inherit meson
MESON_BUILDTYPE = "release"
MESON_TARGET ?= ""

do_compile() {
    meson compile -v ${PARALLEL_MAKE} ${MESON_TARGET}
}

S = "${WORKDIR}/git"
PV = "1.0"

PACKAGES = "${PN} ${PN}-dbg"
