# $NetBSD: Makefile,v 1.9 2001/11/05 15:24:49 lukem Exp $

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../crypto/dist
.PATH: ${DIST}/heimdal/lib/com_err

MKLINT= no

LIB=	com_err
SRCS=	error.c com_err.c roken_rename.h

INCS=	com_err.h com_right.h

.if ${USETOOLS} != "yes"
.PHONY: compile_et

includes: compile_et

compile_et:
	@cd ${.CURDIR}/compile_et && ${MAKE}
.endif

INCSDIR=/usr/include/krb5

CPPFLAGS+= -I.					\
	 -I${.CURDIR}/../../include/heimdal	\
	 -I${.CURDIR}/../libroken		\
	 -I${DIST}/heimdal/lib/com_err		\
	 -I${DIST}/heimdal/lib/roken		\
	 -DHAVE_CONFIG_H

.if ${USETOOLS} != "yes"
SUBDIR=	compile_et
.endif

.include <bsd.lib.mk>
.if ${USETOOLS} != "yes"
.include <bsd.subdir.mk>
.endif
