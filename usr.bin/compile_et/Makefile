# $NetBSD: Makefile,v 1.13 2002/10/23 01:25:35 itojun Exp $

NOLINT= # defined

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH: ${DIST}/heimdal/lib/com_err

WARNS?=  1

PROG= compile_et

SRCS= compile_et.c parse.y lex.l

YHEADER=	1

CPPFLAGS+= -I.					\
	-I${DIST}/heimdal/lib/com_err		\
	-I${NETBSDSRCDIR}/include/heimdal	\
	-I${DESTDIR}/usr/include/krb5		\
	-DHAVE_CONFIG_H

.ifndef HOSTPROG
VERS!=	cd ${NETBSDSRCDIR}/lib/libvers && ${PRINTOBJDIR}

LDADD+=	-L${VERS} -lvers -lroken -lcrypt
DPADD+=	${VERS}/libvers.a ${LIBROKEN} ${LIBCRYPT}
.endif

MAN=	compile_et.1

CLEANFILES+=	lex.c parse.c parse.h

.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif
