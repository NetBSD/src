# $NetBSD: Makefile,v 1.17 2003/12/13 22:07:35 chs Exp $

NOLINT= # defined

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH: ${DIST}/heimdal/kuser

WARNS?=  1

PROG= kinit

SRCS= kinit.c

.if ${MKKERBEROS4} != "no"
CPPFLAGS+= -DKRB4
CPPFLAGS+= -I${DESTDIR}/usr/include/kerberosIV
KRB4LDADD= -lkrb -ldes
KRB4DPADD= ${LIBKRB} ${LIBDES}
.endif

CPPFLAGS+= -I.					\
	 -I${DIST}/heimdal/kuser		\
	 -I${NETBSDSRCDIR}/include/heimdal	\
	 -I${DESTDIR}/usr/include/krb5		\
	 -DHAVE_CONFIG_H

VERS!=	cd ${NETBSDSRCDIR}/lib/libvers && ${PRINTOBJDIR}

LDADD+=	-lkafs -lkrb5 -ldes ${KRB4LDADD} -lcrypto -lasn1 -lcom_err \
	-L${VERS} -lvers -lroken -lcrypt
DPADD+=	${LIBKAFS} ${LIBKRB5} ${LIBDES} ${KRB4DPADD} ${LIBCRYPTO} ${LIBASN1} ${LIBCOM_ERR} \
	${VERS}/libvers.a ${LIBROKEN} ${LIBCRYPT}

.include <bsd.prog.mk>
