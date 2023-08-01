#	$NetBSD: Makefile,v 1.2.2.2 2023/08/01 16:35:00 martin Exp $

LIB=		h_onlyuse_dynamic
SRCS=		h_onlyuse_dynamic.c

DEF_DIR_cmd=	cd ${.CURDIR}/../helper_onlydef && ${PRINTOBJDIR}
DEF_DIR=	${DEF_DIR_cmd:sh}
LDADD+=		-Wl,-rpath,${TESTSDIR} -L${DEF_DIR} -lh_onlydef

.include "../helper.mk"
