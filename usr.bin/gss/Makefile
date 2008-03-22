# $NetBSD: Makefile,v 1.1 2008/03/22 09:24:06 mlelstv Exp $

NOLINT= # defined

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH: ${DIST}/heimdal/lib/gssapi

WARNS?=  1

PROG= gss
# no man page yet
MAN=

SRCS= \
	gss.c				\
	gss-commands.c

CPPFLAGS+= -I.					\
	-I${DIST}/heimdal/lib/gssapi		\
	-I${NETBSDSRCDIR}/include/heimdal	\
	-I${DESTDIR}/usr/include/krb5		\
	-DHAVE_CONFIG_H

.if (${USE_INET6} != "no")
CPPFLAGS+=-DHAVE_IPV6
.endif

VERS!=	cd ${NETBSDSRCDIR}/lib/libvers && ${PRINTOBJDIR}

LDADD+=	-lgssapi -lsl -L${VERS} -lvers -lroken -lcrypt
DPADD+=	${LIBGSSAPI} ${LIBSL} ${VERS}/libvers.a ${LIBROKEN} ${LIBCRYPT}

.include <bsd.prog.mk>
