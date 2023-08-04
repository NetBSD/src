# $NetBSD: Makefile,v 1.1.46.1 2023/08/04 12:55:45 martin Exp $

LIB=		h_helper_dso2
SRCS=		h_helper_dso2.c

DSO1DIR_cmd=	cd ${.CURDIR}/../helper_dso1 && ${PRINTOBJDIR}
DSO1DIR=	${DSO1DIR_cmd:sh}
LDADD+=		-Wl,-rpath,${TESTSDIR} -L${DSO1DIR} -lh_helper_dso1

.include "../helper.mk"
