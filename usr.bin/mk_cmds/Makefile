# $NetBSD: Makefile,v 1.12 2005/01/10 03:11:18 lukem Exp $

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

.if (${USE_INET6} != "no")
CPPFLAGS+=-DHAVE_IPV6
.endif

VERS!=	cd ${NETBSDSRCDIR}/lib/libvers && ${PRINTOBJDIR}

LDADD+=	-L${VERS} -lvers -lroken -lcrypt
DPADD+=	${VERS}/libvers.a ${LIBROKEN} ${LIBCRYPT}

CLEANFILES+=lex.c parse.c parse.h

.include <bsd.prog.mk>
