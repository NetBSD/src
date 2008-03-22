# $NetBSD: Makefile,v 1.1 2008/03/22 09:29:56 mlelstv Exp $

NOLINT=		# defined

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH:		${DIST}/heimdal/lib/kadm5

WARNS?=		1

PROG=		ipropd-slave

# man page included with ipropd-master
MAN=

SRCS+= 	ipropd_slave.c				\
	ipropd_common.c

CPPFLAGS+= -I.					\
	-I${DIST}/heimdal/lib/krb5		\
	-I${DIST}/heimdal/lib/asn1		\
	-I${NETBSDSRCDIR}/include/heimdal	\
	-I${DESTDIR}/usr/include/kadm5		\
	-I${DESTDIR}/usr/include/krb5		\
	-DHAVE_CONFIG_H

.if ${HAVE_GCC} == 4
COPTS.server.c+=	-Wno-pointer-sign
.endif

.if (${USE_INET6} != "no")
CPPFLAGS+=-DHAVE_IPV6
.endif

VERS!=		cd ${NETBSDSRCDIR}/lib/libvers && ${PRINTOBJDIR}

LDADD+=		-lkadm5srv -lhdb -lkrb5 -lasn1 \
		-lcrypto -L${VERS} -lvers -lroken \
		-lcrypt
DPADD+=		${LIBKADM5SRV} ${LIBHDB} ${LIBKRB5} ${LIBASN1} \
		${LIBCRYPTO} ${VERS}/libvers.a ${LIBROKEN} \
		${LIBCRYPT}

.include <bsd.prog.mk>
