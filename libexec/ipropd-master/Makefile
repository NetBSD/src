# $NetBSD: Makefile,v 1.2.4.1 2008/09/18 04:39:17 wrstuden Exp $

NOLINT=		# defined

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH:		${DIST}/heimdal/lib/kadm5

WARNS?=		1

PROG=		ipropd-master
MAN=		iprop.8
MLINKS=	\
	iprop.8		ipropd-master.8		\
	iprop.8		ipropd-slave.8

SRCS+= 	ipropd_master.c				\
	ipropd_common.c

CPPFLAGS+= -I.					\
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

LDADD+=		-lkadm5srv -lhdb -lkrb5 -lhx509 -lasn1 \
		-lcrypto -L${VERS} -lvers -lcom_err -lroken \
		-lcrypt -lutil
DPADD+=		${LIBKADM5SRV} ${LIBHDB} ${LIBKRB5} ${LIBHX509} ${LIBASN1} \
		${LIBCRYPTO} ${VERS}/libvers.a ${LIBCOM_ERR} ${LIBROKEN} \
		${LIBCRYPT} ${LIBUTIL}

.include <bsd.prog.mk>
