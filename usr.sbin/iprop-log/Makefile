# $NetBSD: Makefile,v 1.5.4.1 2011/06/23 14:20:47 cherry Exp $

WARNS?=	2	# XXX -Wextra -Wsign-compare
NOLINT= # defined

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH: ${DIST}/heimdal/lib/kadm5

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

.if defined(HAVE_GCC)
COPTS.mit_dump.c+=	-Wno-pointer-sign
.endif

VERS!=	cd ${NETBSDSRCDIR}/lib/libvers && ${PRINTOBJDIR}

LDADD+=	-lkadm5srv -lhdb -lkrb5 -lhx509 -lcrypto \
	-lasn1 -lsl -lcom_err -L${VERS} -lvers \
	-lroken -lcrypt -ledit -lterminfo
DPADD+=	${LIBKADM5SRV} ${LIBHDB} ${LIBKRB5} ${LIBHX509} ${LIBCRYPTO} \
	${LIBASN1} ${LIBSL} ${LIBCOM_ERR} ${VERS}/libvers.a \
	${LIBROKEN} ${LIBCRYPT} ${LIBEDIT} ${LIBTERMINFO}

MAN=	iprop-log.8

.include <bsd.prog.mk>
