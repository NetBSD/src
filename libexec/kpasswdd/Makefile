# $NetBSD: Makefile,v 1.14 2006/05/11 23:16:29 mrg Exp $

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

.if ${HAVE_GCC} == 4
COPTS.kpasswdd.c+=	-fno-strict-aliasing
.endif

VERS!=		cd ${NETBSDSRCDIR}/lib/libvers && ${PRINTOBJDIR}

LDADD+=		-lkadm5srv -lhdb -lkrb5 -lcrypto -lasn1 \
		-lcom_err -L${VERS} -lvers -lroken -lcrypt \
		-lutil
DPADD+=		${LIBKADM5SRV} ${LIBHDB} ${LIBKRB5} ${LIBCRYPTO} ${LIBASN1} \
		${LIBCOM_ERR} ${VERS}/libvers.a ${LIBROKEN} ${LIBCRYPT} \
		${LIBUTIL}

.include <bsd.prog.mk>
