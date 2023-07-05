#	$NetBSD: Makefile,v 1.2 2023/07/05 22:42:47 riastradh Exp $

LIB=		h_onlyctor_dynamic
SRCS=		h_onlyctor_dynamic.c

DEF_DIR_cmd=	cd ${.CURDIR}/../helper_onlydef && ${PRINTOBJDIR}
DEF_DIR=	${DEF_DIR_cmd:sh}
LDADD+=		-Wl,-rpath,${TESTSDIR} -L${DEF_DIR} -lh_onlydef

.include "../helper.mk"
