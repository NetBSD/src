# $NetBSD: Makefile,v 1.6 2000/12/30 14:54:40 sommerfeld Exp $
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

VERS!=	cd ${.CURDIR}/../../lib/libvers && ${PRINTOBJDIR}

LDADD=	-L${VERS} -lvers \
	-lroken \
	-lcrypt

NOMAN=	noman

CLEANFILES+=lex.c parse.c parse.h

.include <bsd.prog.mk>
