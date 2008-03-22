# $NetBSD: Makefile,v 1.1 2008/03/22 09:29:56 mlelstv Exp $

NOLINT= # defined

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH: ${DIST}/heimdal/kuser

WARNS?=  1

PROG= kdigest
# no man-page yet
MAN=

SRCS+= 	kdigest.c				\
	kdigest-commands.c

CPPFLAGS+= -I.					\
	-I${DIST}/heimdal/kuser			\
	-I${NETBSDSRCDIR}/include/heimdal	\
	-I${DIST}/heimdal/lib/asn1		\
	-I${DIST}/heimdal/lib/krb5		\
	-I${DESTDIR}/usr/include/krb5		\
	-DHAVE_CONFIG_H

.if (${USE_INET6} != "no")
CPPFLAGS+=-DHAVE_IPV6
.endif

VERS!=	cd ${NETBSDSRCDIR}/lib/libvers && ${PRINTOBJDIR}

LDADD+=	-lkrb5 -lheimntlm -lcrypto -lasn1 -lcom_err -lsl \
	-L${VERS} -lvers -lroken -lcrypt
DPADD+=	${LIBKRB5} ${LIBHEIMNTLM} ${LIBCRYPTO} ${LIBASN1} \
	${LIBCOM_ERR} ${LIBSL} ${VERS}/libvers.a ${LIBROKEN} ${LIBCRYPT}

.include <bsd.prog.mk>
