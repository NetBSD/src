# $NetBSD: Makefile,v 1.1.1.1 2000/06/16 22:50:34 thorpej Exp $
DIST=		${.CURDIR}/../../dist
.PATH: ${DIST}/heimdal/lib/kafs

WARNS?=  1
MKLINT= no

LIB=	kafs

SRCS=						\
	afssys.c				\
	afskrb.c				\
	afskrb5.c				\
	common.c				\

INCSDIR= /usr/include/krb5

INCS=	kafs.h

CPPFLAGS+= -I.					\
	-I${DIST}/heimdal/lib/kafs		\
	-I${DESTDIR}/usr/include/krb5		\
	-I${DESTDIR}/usr/include/openssl	\
	-I${.CURDIR}/../../include/heimdal	\
	-I${.CURDIR}/../libroken		\
	-I${DIST}/heimdal/lib/roken		\
	-DHAVE_CONFIG_H

.include <bsd.lib.mk>
