# $NetBSD: Makefile,v 1.5 2000/10/26 05:57:00 garbled Exp $
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

VERS!=	cd ${.CURDIR}/../../lib/libvers && ${MAKE} print-objdir

LDADD=	-L${VERS} -lvers \
	-lroken \
	-lcrypt

NOMAN=	noman

CLEANFILES+=	lex.c parse.c parse.h

.include <bsd.prog.mk>
