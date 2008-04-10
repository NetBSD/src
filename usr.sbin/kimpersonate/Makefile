# $NetBSD: Makefile,v 1.2 2008/04/10 15:42:06 tsutsui Exp $

NOLINT= # defined

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH: ${DIST}/heimdal/kuser

WARNS?=  1

PROG= kimpersonate

SRCS+= 	kimpersonate.c

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

VERS!=  cd ${NETBSDSRCDIR}/lib/libvers && ${PRINTOBJDIR}

LDADD+=	-lkafs -lkrb5 -lhx509 -lheimntlm -lcrypto \
	-lasn1 -lcom_err -L${VERS} -lvers -lroken -lcrypt
DPADD+=	${LIBKAFS} ${LIBKRB5} ${LIBHX509} ${LIBHEIMNTLM} ${LIBCRYPTO} \
	${LIBASN1} ${LIBCOM_ERR} ${VERS}/libvers.a ${LIBROKEN} ${LIBCRYPT}

.include <bsd.prog.mk>
