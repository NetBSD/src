# $NetBSD: Makefile,v 1.1.2.2 2026/07/19 16:17:13 martin Exp $

LIB=		h_helper_recurdso2
SRCS=		h_helper_recurdso2.c

RECURDSODIR_cmd=cd ${.CURDIR}/../helper_recurdso && ${PRINTOBJDIR}
RECURDSODIR=	${RECURDSODIR_cmd:sh}
LDADD+=		-Wl,-rpath,${TESTSDIR} -L${RECURDSODIR} -lh_helper_recurdso

.include "../helper.mk"
