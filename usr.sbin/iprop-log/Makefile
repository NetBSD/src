# $NetBSD: Makefile,v 1.1 2008/03/22 09:29:56 mlelstv Exp $

NOLINT= # defined

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH: ${DIST}/heimdal/lib/kadm5

WARNS?=  1

PROG= iprop-log

SRCS+= 	iprop-log.c				\
	iprop-commands.c

CPPFLAGS+= -I.					\
	-I${DIST}/heimdal/lib/hdb		\
	-I${DIST}/heimdal/lib/krb5		\
	-I${NETBSDSRCDIR}/include/heimdal	\
	-I${DIST}/heimdal/lib/asn1		\
	-I${DESTDIR}/usr/include/krb5		\
	-I${DESTDIR}/usr/include/kadm5		\
	-DHAVE_CONFIG_H

.if (${USE_INET6} != "no")
CPPFLAGS+=-DHAVE_IPV6
.endif

.if ${HAVE_GCC} == 4
COPTS.mit_dump.c+=	-Wno-pointer-sign
.endif

VERS!=	cd ${NETBSDSRCDIR}/lib/libvers && ${PRINTOBJDIR}

LDADD+=	-lkadm5srv -lhdb -lkrb5 -lcrypto -lasn1 \
	-lsl -L${VERS} -lvers -lroken -lcrypt
DPADD+=	${LIBKADM5SRV} ${LIBHDB} ${LIBKRB5} ${LIBCRYPTO} ${LIBASN1} \
	${LIBSL} ${VERS}/libvers.a ${LIBROKEN} ${LIBCRYPT}

MAN=	iprop-log.8

.include <bsd.prog.mk>
