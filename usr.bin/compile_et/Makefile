# $NetBSD: Makefile,v 1.10 2001/12/12 01:48:44 tv Exp $

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../crypto/dist
.PATH: ${DIST}/heimdal/lib/com_err

WARNS?=  1
NOLINT= # defined

PROG= compile_et

SRCS= compile_et.c parse.y lex.l

YHEADER=	1

CPPFLAGS+= -I.					\
	-I${DIST}/heimdal/lib/com_err		\
	-I${.CURDIR}/../../include/heimdal	\
	-I${DESTDIR}/usr/include/krb5		\
	-DHAVE_CONFIG_H

.ifndef HOSTPROG
VERS!=	cd ${.CURDIR}/../../lib/libvers && ${PRINTOBJDIR}

LDADD=	-L${VERS} -lvers \
	-lroken \
	-lcrypt
.endif

MAN=	compile_et.1

CLEANFILES+=	lex.c parse.c parse.h

.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif
