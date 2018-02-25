#	$NetBSD: Makefile,v 1.46 2018/02/25 00:16:49 mrg Exp $
#	from: @(#)Makefile    8.3 (Berkeley) 4/2/94

.include <bsd.own.mk>

USE_FORT?= yes	# setuid
PROG=	passwd
SRCS=	local_passwd.c passwd.c
MAN=	passwd.1

CPPFLAGS+=-I${.CURDIR} -DLOGIN_CAP

.if (${USE_YP} != "no")
SRCS+=	yp_passwd.c
CPPFLAGS+=-DYP
LDADD+=	-lrpcsvc
DPADD+=	${LIBRPCSVC}
LINKS+=	${BINDIR}/passwd ${BINDIR}/yppasswd
MAN+=	yppasswd.1
.endif

LDADD+= -lcrypt -lutil
DPADD+= ${LIBCRYPT} ${LIBUTIL}

BINOWN=	root
BINMODE=4555

.if (${USE_KERBEROS} != "no")
CPPFLAGS+= -DKERBEROS5
SRCS+=	krb5_passwd.c

LDADD+=	${LIBKRB5_LDADD}
DPADD+=	${LIBKRB5_DPADD}
.ifdef OVERRIDE_HEIMDAL_KPASSWD
LINKS+=	${BINDIR}/passwd ${BINDIR}/kpasswd
MAN+=	kpasswd.1
.endif
.endif

.if (${USE_PAM} != "no")
CPPFLAGS+=-DUSE_PAM
SRCS+= pam_passwd.c
LDADD+=-lpam ${PAM_STATIC_LDADD}
DPADD+=${LIBPAM} ${PAM_STATIC_DPADD}
.endif

.include <bsd.prog.mk>
