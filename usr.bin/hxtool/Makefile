# $NetBSD: Makefile,v 1.4 2010/02/03 15:34:45 roy Exp $

WARNS?=	1	# XXX -Wextra -Wshadow

NOLINT= # defined
NOMAN=	# defined

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH: ${DIST}/heimdal/lib/hx509

PROG= hxtool

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
	-L${VERS} -lvers -lroken -lsl -lcrypt -lutil \
	-ledit -lterminfo
DPADD+=	${LIBHX509} ${LIBCRYPTO} ${LIBASN1} ${LIBCOM_ERR} \
	${VERS}/libvers.a ${LIBROKEN} ${LIBSL} ${LIBCRYPT} ${LIBUTIL} \
	${LIBEDIT} ${LIBTERMINFO}

.include <bsd.prog.mk>

