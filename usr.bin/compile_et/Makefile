# $NetBSD: Makefile,v 1.2.4.2 2000/06/23 16:30:17 minoura Exp $
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

LDADD=	-lroken \
	-lcrypt

NOMAN=	noman

CLEANFILES+=	lex.c parse.c parse.h

.include <bsd.prog.mk>
