# $NetBSD: Makefile,v 1.20.4.1 2008/09/18 04:39:17 wrstuden Exp $

NOLINT=		# defined

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH:		${DIST}/heimdal/kadmin

WARNS?=		1

PROG=		kadmind
MAN=		kadmind.8

SRCS+= 	kadmind.c				\
	server.c				\
	kadm_conn.c

CPPFLAGS+= -I.					\
	-I${DIST}/heimdal/kadmin		\
	-I${DIST}/heimdal/lib/krb5		\
	-I${DIST}/heimdal/lib/asn1		\
	-I${NETBSDSRCDIR}/include/heimdal	\
	-I${DESTDIR}/usr/include/kadm5		\
	-I${DESTDIR}/usr/include/krb5		\
	-DHAVE_CONFIG_H

.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4) || defined(HAVE_PCC)
COPTS.server.c+=	-Wno-pointer-sign
.endif

.if (${USE_INET6} != "no")
CPPFLAGS+=-DHAVE_IPV6
.endif

VERS!=		cd ${NETBSDSRCDIR}/lib/libvers && ${PRINTOBJDIR}

LDADD+=		-lkadm5srv -lhdb -lkrb5 -lhx509 \
		-lcrypto -lasn1 -lcom_err -L${VERS} -lvers \
		-lroken -lcrypt -lutil
DPADD+=		${LIBKADM5SRV} ${LIBHDB} ${LIBKRB5} ${LIBHX509} \
		${LIBCRYPTO} ${LIBASN1} ${LIBCOM_ERR} ${VERS}/libvers.a \
		${LIBROKEN} ${LIBCRYPT} ${LIBUTIL}

.include <bsd.prog.mk>
