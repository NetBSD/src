# $NetBSD: Makefile,v 1.2.2.1 2001/04/05 23:24:53 he Exp $
DIST=		${.CURDIR}/../../crypto/dist
.PATH: ${DIST}/heimdal/lib/com_err

WARNS?=  1
MKLINT= no

LIB=	com_err
SRCS=	error.c com_err.c roken_rename.h

INCS=	com_err.h com_right.h

.PHONY: compile_et

includes: compile_et

compile_et:
	@cd ${.CURDIR}/compile_et && ${MAKE}

INCSDIR=/usr/include/krb5

CPPFLAGS+= -I.					\
	 -I${.CURDIR}/../../include/heimdal	\
	 -I${.CURDIR}/../libroken		\
	 -I${DIST}/heimdal/lib/com_err		\
	 -I${DIST}/heimdal/lib/roken		\
	 -DHAVE_CONFIG_H

SUBDIR=	compile_et

.include <bsd.lib.mk>
.include <bsd.subdir.mk>
