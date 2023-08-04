#	$NetBSD: Makefile,v 1.2.4.2 2023/08/04 12:55:46 martin Exp $

LIB=		h_abuse_static
SRCS=		h_abuse_static.c

DEF_DIR_cmd=	cd ${.CURDIR}/../helper_def_static && ${PRINTOBJDIR}
DEF_DIR=	${DEF_DIR_cmd:sh}
LDADD+=		-Wl,-rpath,${TESTSDIR} -L${DEF_DIR} -lh_def_static

.include "../helper.mk"
