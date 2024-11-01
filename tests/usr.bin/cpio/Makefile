# $NetBSD: Makefile,v 1.4.10.1 2024/11/01 10:32:30 martin Exp $

NOMAN=

.include <bsd.own.mk>
LIBARCHIVE=${NETBSDSRCDIR}/external/bsd/libarchive/dist

TESTSDIR=	${TESTSBASE}/usr.bin/cpio

BINDIR=                 ${TESTSDIR}
PROGS+=	h_cpio
TESTS_SH+=	t_cpio

CPPFLAGS+=-I${LIBARCHIVE}/test_utils -I${LIBARCHIVE}/cpio -I.
CPPFLAGS+=-I${LIBARCHIVE}/cpio/test -I${LIBARCHIVE}/../include
CPPFLAGS+=-I${LIBARCHIVE}/libarchive -I${LIBARCHIVE}/libarchive_fe
CPPFLAGS+=-DPLATFORM_CONFIG_H='"config_netbsd.h"'

.PATH:  ${LIBARCHIVE}/cpio/test ${LIBARCHIVE}/test_utils ${LIBARCHIVE}/cpio \
	${LIBARCHIVE}/libarchive_fe

DPADD+=	${LIBARCHIVE} ${LIBEXPAT} ${LIBBZ2} ${LIBLZMA} ${LIBZ} \
	${LIBCRYPTO} ${LIBPTHREAD}
LDADD+=	-larchive -lexpat -lbz2 -llzma -lz -lcrypto -lpthread  

SRCS.h_cpio= \
test_main.c \
test_utils.c \
cmdline.c \
err.c \
test_0.c \
test_basic.c \
test_cmdline.c \
test_extract_cpio_Z.c \
test_extract_cpio_bz2.c \
test_extract_cpio_grz.c \
test_extract_cpio_gz.c \
test_extract_cpio_lrz.c \
test_extract_cpio_lz.c \
test_extract_cpio_lz4.c \
test_extract_cpio_lzma.c \
test_extract_cpio_lzo.c \
test_extract_cpio_xz.c \
test_extract_cpio_zstd.c \
test_format_newc.c \
test_gcpio_compat.c \
test_missing_file.c \
test_option_0.c \
test_option_B_upper.c \
test_option_C_upper.c \
test_option_J_upper.c \
test_option_L_upper.c \
test_option_Z_upper.c \
test_option_a.c \
test_option_b64encode.c \
test_option_c.c \
test_option_d.c \
test_option_f.c \
test_option_grzip.c \
test_option_help.c \
test_option_l.c \
test_option_lrzip.c \
test_option_lz4.c \
test_option_lzma.c \
test_option_lzop.c \
test_option_m.c \
test_option_passphrase.c \
test_option_t.c \
test_option_u.c \
test_option_version.c \
test_option_xz.c \
test_option_y.c \
test_option_z.c \
test_option_zstd.c \
test_owner_parse.c \
test_passthrough_dotdot.c \
test_passthrough_reverse.c

FILESDIR=       ${TESTSDIR}
FILES=\
test_extract.cpio.Z.uu \
test_extract.cpio.bz2.uu \
test_extract.cpio.grz.uu \
test_extract.cpio.gz.uu \
test_extract.cpio.lrz.uu \
test_extract.cpio.lz.uu \
test_extract.cpio.lz4.uu \
test_extract.cpio.lzma.uu \
test_extract.cpio.lzo.uu \
test_extract.cpio.xz.uu \
test_extract.cpio.zst.uu \
test_gcpio_compat_ref.bin.uu \
test_gcpio_compat_ref.crc.uu \
test_gcpio_compat_ref.newc.uu \
test_gcpio_compat_ref.ustar.uu \
test_gcpio_compat_ref_nosym.bin.uu \
test_gcpio_compat_ref_nosym.crc.uu \
test_gcpio_compat_ref_nosym.newc.uu \
test_gcpio_compat_ref_nosym.ustar.uu \
test_option_f.cpio.uu \
test_option_m.cpio.uu \
test_option_passphrase.zip.uu \
test_option_t.cpio.uu \
test_option_t.stdout.uu \
test_option_tv.stdout.uu

.include <bsd.test.mk>

test_main.o test_main.d: list.h

CLEANFILES+=list.h

list.h: ${SRCS.h_cpio} Makefile
	${TOOL_GREP} -h '^DEFINE_TEST(' ${.ALLSRC} > ${.TARGET}

COPTS.test_main.c+=	${GCC_NO_STRINGOP_OVERFLOW}
