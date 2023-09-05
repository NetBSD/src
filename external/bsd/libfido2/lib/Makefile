# $NetBSD: Makefile,v 1.8.2.2 2023/09/05 17:11:34 martin Exp $

NOLINT=
.include <bsd.own.mk>
.include <bsd.init.mk>

.PATH: ${DIST}/src ${DIST}/man ${DIST}/openbsd-compat

CPPFLAGS+= -D_FIDO_INTERNAL -I${DIST}/src

LDADD+=-lusbhid -lcbor -lz
DPADD+=${LIBUSBHID} ${LIBCBOR} ${LIBZ}

LDFLAGS+=-Wl,--version-script=${DIST}/src/export.gnu

LIB=    fido2

SRCS+= \
aes256.c \
assert.c \
authkey.c \
bio.c \
blob.c \
buf.c \
cbor.c \
compress.c \
config.c \
cred.c \
credman.c \
dev.c \
ecdh.c \
eddsa.c \
err.c \
es256.c \
es384.c \
hid.c \
hid_netbsd.c \
hid_unix.c \
info.c \
io.c \
iso7816.c \
largeblob.c \
log.c \
pin.c \
random.c \
reset.c \
rs1.c \
rs256.c \
time.c \
tpm.c \
touch.c \
types.c \
u2f.c

SRCS+= \
explicit_bzero.c \
freezero.c \
recallocarray.c \
timingsafe_bcmp.c

INCS+= \
fido.h \
fido/bio.h \
fido/config.h \
fido/credman.h \
fido/eddsa.h \
fido/err.h \
fido/es256.h \
fido/es384.h \
fido/param.h \
fido/rs256.h \
fido/types.h

INCSDIR=/usr/include

MAN+= \
eddsa_pk_new.3 \
es256_pk_new.3 \
es384_pk_new.3 \
fido_assert_allow_cred.3 \
fido_assert_new.3 \
fido_assert_set_authdata.3 \
fido_assert_verify.3 \
fido_bio_dev_get_info.3 \
fido_bio_enroll_new.3 \
fido_bio_info_new.3 \
fido_bio_template.3 \
fido_cbor_info_new.3 \
fido_cred_exclude.3 \
fido_cred_new.3 \
fido_cred_set_authdata.3 \
fido_cred_verify.3 \
fido_credman_metadata_new.3 \
fido_dev_enable_entattest.3 \
fido_dev_get_assert.3 \
fido_dev_get_touch_begin.3 \
fido_dev_info_manifest.3 \
fido_dev_largeblob_get.3 \
fido_dev_make_cred.3 \
fido_dev_open.3 \
fido_dev_set_io_functions.3 \
fido_dev_set_pin.3 \
fido_init.3 \
fido_strerr.3 \
rs256_pk_new.3

SHLIB_MAJOR=5
SHLIB_MINOR=0

.SUFFIXES: .in
.in:
	${TOOL_SED} \
		-e s%@CMAKE_INSTALL_PREFIX@%/usr% \
		-e s%@CMAKE_INSTALL_LIBDIR@%lib% \
		-e s%@PROJECT_NAME@%libfido2% \
		-e s%@FIDO_VERSION@%${FIDO_VERSION}% \
		< ${.ALLSRC} > ${.TARGET}

FILESDIR=/usr/lib/pkgconfig
FILES+=libfido2.pc
FILESBUILD_libfido2.pc=yes

COPTS.assert.c+=-Wno-error=deprecated-declarations
COPTS.cbor.c+=-Wno-error=deprecated-declarations
COPTS.cred.c+=-Wno-error=deprecated-declarations
COPTS.ecdh.c+=-Wno-error=deprecated-declarations
COPTS.es256.c+=-Wno-error=deprecated-declarations
COPTS.es384.c+=-Wno-error=deprecated-declarations
COPTS.rs256.c+=-Wno-error=deprecated-declarations

.include <bsd.lib.mk>
