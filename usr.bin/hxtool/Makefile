# $NetBSD: Makefile,v 1.1 2008/03/22 09:24:06 mlelstv Exp $

NOLINT= # defined

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH: ${DIST}/heimdal/lib/hx509

WARNS?=  1

PROG= hxtool
# man page is missing
MAN=

SRCS= \
	hxtool.c				\
	hxtool-commands.c

CPPFLAGS+= -I.					\
	-I${DIST}/heimdal/lib/hx509		\
	-I${DIST}/heimdal/lib/asn1		\
	-I${NETBSDSRCDIR}/include/heimdal	\
	-I${DESTDIR}/usr/include/krb5		\
	-DHAVE_CONFIG_H

.if (${USE_INET6} != "no")
CPPFLAGS+=-DHAVE_IPV6
.endif

VERS!=	cd ${NETBSDSRCDIR}/lib/libvers && ${PRINTOBJDIR}

LDADD+=	-lhx509 -lcrypto -lasn1 -lcom_err \
	-L${VERS} -lvers -lroken -lsl -lcrypt
DPADD+=	${LIBHX509} ${LIBCRYPTO} ${LIBASN1} ${LIBCOM_ERR} \
	${VERS}/libvers.a ${LIBROKEN} ${LIBSL} ${LIBCRYPT}

.include <bsd.prog.mk>

