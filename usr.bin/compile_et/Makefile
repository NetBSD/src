# $NetBSD: Makefile,v 1.16.36.1 2009/05/13 19:19:46 jym Exp $

WARNS?=	2	# XXX -Wextra -Wsign-compare

NOLINT= # defined

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH: ${DIST}/heimdal/lib/com_err

PROG= compile_et

SRCS= compile_et.c parse.y lex.l

YHEADER=	1

CPPFLAGS+= -I.					\
	-I${DIST}/heimdal/lib/com_err		\
	-I${NETBSDSRCDIR}/include/heimdal	

.ifndef HOSTPROG
CPPFLAGS+=	-I${DESTDIR}/usr/include/krb5
.endif
CPPFLAGS+=	-DHAVE_CONFIG_H

.if (${USE_INET6} != "no")
CPPFLAGS+=-DHAVE_IPV6
.endif

.ifndef HOSTPROG
VERS!=	cd ${NETBSDSRCDIR}/lib/libvers && ${PRINTOBJDIR}

LDADD+=	-L${VERS} -lvers -lroken -lcrypt
DPADD+=	${VERS}/libvers.a ${LIBROKEN} ${LIBCRYPT}
.endif

MAN=	compile_et.1

CLEANFILES+=	lex.c parse.c parse.h

.include <bsd.prog.mk>
