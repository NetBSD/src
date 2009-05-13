# $NetBSD: Makefile,v 1.2.10.1 2009/05/13 19:19:51 jym Exp $

WARNS?=	2	# XXX -Wextra -Wcast-qual

NOLINT= # defined
NOMAN=	# defined

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH: ${DIST}/heimdal/lib/gssapi

PROG= gss

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

LDADD+=	-lgssapi -lkrb5 -lheimntlm -lhx509 -lsl \
	-lasn1 -lcrypto -lcom_err -L${VERS} -lvers \
	-lroken -lcrypt -lutil -ledit -ltermcap
DPADD+=	${LIBGSSAPI} ${LIBKRB5} ${LIBHEIMNTLM} ${LIBHX509} ${LIBSL} \
	${LIBASN1} ${LIBCRYPTO} ${LIBCOM_ERR} ${VERS}/libvers.a \
	${LIBROKEN} ${LIBCRYPT} ${LIBUTIL} ${LIBEDIT} ${LIBTERMCAP}

.include <bsd.prog.mk>
