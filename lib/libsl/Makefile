# $NetBSD: Makefile,v 1.2.4.2 2000/06/23 16:16:51 minoura Exp $
DIST=		${.CURDIR}/../../crypto/dist
.PATH: ${DIST}/heimdal/lib/sl

WARNS?=  1
MKLINT= no

LIB=	sl

SRCS=	sl.c

INCS=	sl.h

INCSDIR= /usr/include/krb5

CPPFLAGS+= -I.					\
	 -I${DIST}/heimdal/lib/sl		\
	 -I${.CURDIR}/../../include/heimdal	\
	 -I${.CURDIR}/../../lib/libroken	\
	 -I${DIST}/heimdal/lib/roken		\
	 -DHAVE_CONFIG_H

.include <bsd.lib.mk>
