# $NetBSD: Makefile,v 1.3 2000/08/03 04:02:29 assar Exp $
DIST=		${.CURDIR}/../../crypto/dist
.PATH: ${DIST}/heimdal/lib/com_err

WARNS?=  1
MKLINT= no

PROG= compile_et

SRCS= compile_et.c parse.y lex.l

YHEADER=	1

CPPFLAGS+= -I.					\
	-I${DIST}/heimdal/lib/com_err		\
	-I${.CURDIR}/../../include/heimdal	\
	-I${DESTDIR}/usr/include/krb5		\
	-DHAVE_CONFIG_H

LDADD=	-lvers \
	-lroken \
	-lcrypt

NOMAN=	noman

CLEANFILES+=	lex.c parse.c parse.h

.include <bsd.prog.mk>
