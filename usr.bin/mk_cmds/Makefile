# $NetBSD: Makefile,v 1.11 2002/10/23 01:25:38 itojun Exp $

NOMAN=	# defined
NOLINT= # defined

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH: ${DIST}/heimdal/lib/sl

WARNS?=  1

PROG= mk_cmds

SRCS= make_cmds.c make_cmds.h parse.y lex.l

YHEADER=	1

CFLAGS+= -I.					\
	-I${DIST}/heimdal/lib/sl		\
	-I${NETBSDSRCDIR}/include/heimdal	\
	-I${DESTDIR}/usr/include/krb5		\
	-DHAVE_CONFIG_H

VERS!=	cd ${NETBSDSRCDIR}/lib/libvers && ${PRINTOBJDIR}

LDADD+=	-L${VERS} -lvers -lroken -lcrypt
DPADD+=	${VERS}/libvers.a ${LIBROKEN} ${LIBCRYPT}

CLEANFILES+=lex.c parse.c parse.h

.include <bsd.prog.mk>
