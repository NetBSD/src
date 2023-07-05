#	$NetBSD: Makefile,v 1.3 2023/07/05 22:42:48 riastradh Exp $

LIB=		h_use_static
SRCS=		h_use_static.c

DEF_DIR_cmd=	cd ${.CURDIR}/../helper_def_static && ${PRINTOBJDIR}
DEF_DIR=	${DEF_DIR_cmd:sh}
LDADD+=		-Wl,-rpath,${TESTSDIR} -L${DEF_DIR} -lh_def_static

.include "../helper.mk"
