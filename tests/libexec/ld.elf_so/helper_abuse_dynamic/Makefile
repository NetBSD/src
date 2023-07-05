#	$NetBSD: Makefile,v 1.2 2023/07/05 22:42:46 riastradh Exp $

LIB=		h_abuse_dynamic
SRCS=		h_abuse_dynamic.c

DEF_DIR_cmd=	cd ${.CURDIR}/../helper_def_dynamic && ${PRINTOBJDIR}
DEF_DIR=	${DEF_DIR_cmd:sh}
LDADD+=		-Wl,-rpath,${TESTSDIR} -L${DEF_DIR} -lh_def_dynamic

.include "../helper.mk"
