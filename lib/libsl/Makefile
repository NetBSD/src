# $NetBSD: Makefile,v 1.3 2001/11/05 15:24:49 lukem Exp $
DIST=		${.CURDIR}/../../crypto/dist
.PATH: ${DIST}/heimdal/lib/sl

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
