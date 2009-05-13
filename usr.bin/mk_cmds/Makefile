# $NetBSD: Makefile,v 1.13.12.1 2009/05/13 19:19:57 jym Exp $

WARNS?=	1	# XXX -Wextra -Wshadow

NOMAN=	# defined
NOLINT= # defined

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH: ${DIST}/heimdal/lib/sl

PROG= mk_cmds

SRCS= make_cmds.c make_cmds.h parse.y lex.l

YHEADER=	1

CPPFLAGS+= -I.					\
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
