# $NetBSD: Makefile,v 1.8 2001/12/12 01:48:46 tv Exp $

NOMAN=	# defined

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../crypto/dist
.PATH: ${DIST}/heimdal/lib/sl

WARNS?=  1
NOLINT= # defined

PROG= mk_cmds

SRCS= make_cmds.c make_cmds.h parse.y lex.l

YHEADER=	1

CFLAGS+= -I.					\
	-I${DIST}/heimdal/lib/sl		\
	-I${.CURDIR}/../../include/heimdal	\
	-I${DESTDIR}/usr/include/krb5		\
	-DHAVE_CONFIG_H

VERS!=	cd ${.CURDIR}/../../lib/libvers && ${PRINTOBJDIR}

LDADD=	-L${VERS} -lvers \
	-lroken \
	-lcrypt

CLEANFILES+=lex.c parse.c parse.h

.include <bsd.prog.mk>
