# $NetBSD: Makefile,v 1.3 2000/08/03 04:02:32 assar Exp $
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

LDADD=	-lvers \
	-lroken \
	-lcrypt

NOMAN=	noman

CLEANFILES+=lex.c parse.c parse.h

.include <bsd.prog.mk>
