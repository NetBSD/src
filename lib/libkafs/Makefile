# $NetBSD: Makefile,v 1.2 2000/06/16 23:22:58 thorpej Exp $
DIST=		${.CURDIR}/../../crypto/dist
.PATH: ${DIST}/heimdal/lib/kafs

WARNS?=  1
MKLINT= no

LIB=	kafs

SRCS=						\
	afssys.c				\
	afskrb.c				\
	afskrb5.c				\
	common.c				\

INCSDIR= /usr/include/kerberosIV

INCS=	kafs.h

CPPFLAGS+= -I.					\
	-I${DIST}/heimdal/lib/kafs		\
	-I${DESTDIR}/usr/include/krb5		\
	-I${DESTDIR}/usr/include/kerberosIV	\
	-I${DESTDIR}/usr/include/openssl	\
	-I${.CURDIR}/../../include/heimdal	\
	-I${.CURDIR}/../libroken		\
	-I${DIST}/heimdal/lib/roken		\
	-DHAVE_CONFIG_H

.include <bsd.lib.mk>
