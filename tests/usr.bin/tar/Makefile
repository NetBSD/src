# $NetBSD: Makefile,v 1.4.10.1 2024/11/01 10:32:30 martin Exp $

NOMAN=

.include <bsd.own.mk>
LIBARCHIVE=${NETBSDSRCDIR}/external/bsd/libarchive/dist

TESTSDIR=	${TESTSBASE}/usr.bin/tar

BINDIR=                 ${TESTSDIR}
PROGS+=	h_tar
TESTS_SH+=	t_tar

CPPFLAGS+=-I${LIBARCHIVE}/test_utils -I${LIBARCHIVE}/tar -I.
CPPFLAGS+=-I${LIBARCHIVE}/tar/test -I${LIBARCHIVE}/../include
CPPFLAGS+=-I${LIBARCHIVE}/libarchive

.PATH: ${LIBARCHIVE}/tar/test ${LIBARCHIVE}/test_utils

DPADD+=	${LIBARCHIVE} ${LIBEXPAT} ${LIBBZ2} ${LIBLZMA} ${LIBZ} \
	${LIBCRYPTO} ${LIBPTHREAD}
LDADD+=	-larchive -lexpat -lbz2 -llzma -lz -lcrypto -lpthread  

SRCS.h_tar= \
test_main.c \
test_utils.c \
test_0.c \
test_basic.c \
test_copy.c \
test_empty_mtree.c \
test_extract_tar_Z.c \
test_extract_tar_bz2.c \
test_extract_tar_grz.c \
test_extract_tar_gz.c \
test_extract_tar_lrz.c \
test_extract_tar_lz.c \
test_extract_tar_lz4.c \
test_extract_tar_lzma.c \
test_extract_tar_lzo.c \
test_extract_tar_xz.c \
test_extract_tar_zstd.c \
test_format_newc.c \
test_help.c \
test_leading_slash.c \
test_missing_file.c \
test_option_C_mtree.c \
test_option_C_upper.c \
test_option_H_upper.c \
test_option_L_upper.c \
test_option_O_upper.c \
test_option_T_upper.c \
test_option_U_upper.c \
test_option_X_upper.c \
test_option_a.c \
test_option_acls.c \
test_option_b.c \
test_option_b64encode.c \
test_option_exclude.c \
test_option_exclude_vcs.c \
test_option_fflags.c \
test_option_gid_gname.c \
test_option_grzip.c \
test_option_j.c \
test_option_k.c \
test_option_keep_newer_files.c \
test_option_lrzip.c \
test_option_lz4.c \
test_option_lzma.c \
test_option_lzop.c \
test_option_n.c \
test_option_newer_than.c \
test_option_nodump.c \
test_option_older_than.c \
test_option_passphrase.c \
test_option_q.c \
test_option_r.c \
test_option_s.c \
test_option_uid_uname.c \
test_option_uuencode.c \
test_option_xattrs.c \
test_option_xz.c \
test_option_z.c \
test_option_zstd.c \
test_patterns.c \
test_print_longpath.c \
test_stdio.c \
test_strip_components.c \
test_symlink_dir.c \
test_version.c \
test_windows.c

FILESDIR=       ${TESTSDIR}
FILES=\
test_extract.tar.Z.uu \
test_extract.tar.bz2.uu \
test_extract.tar.grz.uu \
test_extract.tar.gz.uu \
test_extract.tar.lrz.uu \
test_extract.tar.lz.uu \
test_extract.tar.lz4.uu \
test_extract.tar.lzma.uu \
test_extract.tar.lzo.uu \
test_extract.tar.xz.uu \
test_extract.tar.zst.uu \
test_leading_slash.tar.uu \
test_option_keep_newer_files.tar.Z.uu \
test_option_passphrase.zip.uu \
test_option_s.tar.Z.uu \
test_patterns_2.tar.uu \
test_patterns_3.tar.uu \
test_patterns_4.tar.uu \
test_print_longpath.tar.Z.uu

.include <bsd.test.mk>

test_main.o test_main.d: list.h

CLEANFILES+=list.h

list.h: ${SRCS.h_tar} Makefile
	${TOOL_GREP} -h '^DEFINE_TEST(' ${.ALLSRC} > ${.TARGET}

COPTS.test_main.c+=	${GCC_NO_STRINGOP_OVERFLOW}
COPTS.test_option_b.c+=	${GCC_NO_STRINGOP_OVERFLOW}
