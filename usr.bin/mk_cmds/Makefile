# $NetBSD: Makefile,v 1.2 2000/06/16 23:57:19 thorpej Exp $
DIST=		${.CURDIR}/../../crypto/dist
.PATH: ${DIST}/heimdal/lib/sl

WARNS?=  1
MKLINT= no

PROG= mk_cmds

SRCS= make_cmds.c make_cmds.h parse.y lex.l

YHEADER=	1

CFLAGS+= -I.					\
	-I${DIST}/heimdal/lib/sl		\
	-I${.CURDIR}/../../include/heimdal	\
	-I${DESTDIR}/usr/include/krb5		\
	-DHAVE_CONFIG_H

LDADD=	-lroken \
	-lcrypt

NOMAN=	noman

CLEANFILES+=lex.c parse.c parse.h

.include <bsd.prog.mk>
