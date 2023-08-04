#	$NetBSD: Makefile,v 1.2.4.2 2023/08/04 12:55:44 martin Exp $

LIB=		h_onlyuse_static
SRCS=		h_onlyuse_static.c

DEF_DIR_cmd=	cd ${.CURDIR}/../helper_onlydef && ${PRINTOBJDIR}
DEF_DIR=	${DEF_DIR_cmd:sh}
LDADD+=		-Wl,-rpath,${TESTSDIR} -L${DEF_DIR} -lh_onlydef

.include "../helper.mk"
