#	$NetBSD: Makefile,v 1.39 2005/02/28 10:37:49 wiz Exp $
#	from: @(#)Makefile    8.3 (Berkeley) 4/2/94

.include <bsd.own.mk>

PROG=	passwd
SRCS=	local_passwd.c passwd.c
MAN=	passwd.1

CPPFLAGS+=-I${.CURDIR} -DLOGIN_CAP

.if (${USE_YP} != "no")
SRCS+=	yp_passwd.c
CPPFLAGS+=-DYP
DPADD+=	${LIBRPCSVC}
LDADD+=	-lrpcsvc
LINKS+=	${BINDIR}/passwd ${BINDIR}/yppasswd
MAN+=	yppasswd.1
.endif

DPADD+= ${LIBCRYPT} ${LIBUTIL}
LDADD+= -lcrypt -lutil

BINOWN=	root
BINMODE=4555

.if (${USE_KERBEROS} != "no")
CPPFLAGS+= -DKERBEROS5 -I${DESTDIR}/usr/include/krb5
SRCS+=	krb5_passwd.c

DPADD+=	${LIBKRB5} ${LIBCRYPTO} ${LIBASN1} ${LIBCOM_ERR} ${LIBROKEN} ${LIBCRYPT}
LDADD+=	-lkrb5 -lcrypto -lasn1 -lcom_err -lroken -lcrypt
LINKS+=	${BINDIR}/passwd ${BINDIR}/kpasswd
MAN+=	kpasswd.1
.endif

.if (${USE_PAM} != "no")
CPPFLAGS+=-DUSE_PAM
SRCS+= pam_passwd.c
LDADD+=-lpam
DPADD+=${LIBPAM}
.endif

.include <bsd.prog.mk>
