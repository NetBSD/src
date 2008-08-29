# $NetBSD: Makefile,v 1.16 2008/08/29 00:02:23 gmcgarry Exp $

NOLINT=		# defined

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH:		${DIST}/heimdal/kpasswd

WARNS?=		1

PROG=		kpasswdd
MAN=		kpasswdd.8

CPPFLAGS+= -I.					\
	-I${DIST}/heimdal/kpasswd		\
	-I${NETBSDSRCDIR}/include/heimdal	\
	-I${DESTDIR}/usr/include/kadm5		\
	-I${DESTDIR}/usr/include/krb5		\
	-DHAVE_CONFIG_H -DINETD_SUPPORT

.if (${USE_INET6} != "no")
CPPFLAGS+=-DHAVE_IPV6
.endif

.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4) || defined(HAVE_PCC)
COPTS.kpasswdd.c+=	-fno-strict-aliasing
.endif

VERS!=		cd ${NETBSDSRCDIR}/lib/libvers && ${PRINTOBJDIR}

LDADD+=		-lkadm5srv -lhdb -lkrb5 -lhx509 \
		-lcrypto -lasn1 \
		-lcom_err -L${VERS} -lvers -lroken -lcrypt \
		-lutil
DPADD+=		${LIBKADM5SRV} ${LIBHDB} ${LIBKRB5} ${LIBHX509} \
		${LIBCRYPTO} ${LIBASN1} \
		${LIBCOM_ERR} ${VERS}/libvers.a ${LIBROKEN} ${LIBCRYPT} \
		${LIBUTIL}

.include <bsd.prog.mk>
