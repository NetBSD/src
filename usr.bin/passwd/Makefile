#	$NetBSD: Makefile,v 1.31 2002/03/22 18:10:23 thorpej Exp $
#	from: @(#)Makefile    8.3 (Berkeley) 4/2/94

.include <bsd.own.mk>

PROG=	passwd
SRCS=	local_passwd.c passwd.c pwd_gensalt.c

CPPFLAGS+=-I${.CURDIR} -DLOGIN_CAP

.if (${USE_YP} != "no")
SRCS+=	yp_passwd.c
CPPFLAGS+=-DYP
DPADD+=	${LIBRPCSVC}
LDADD+=	-lrpcsvc
LINKS=	${BINDIR}/passwd ${BINDIR}/yppasswd
MLINKS=	passwd.1 yppasswd.1
.endif

DPADD+= ${LIBCRYPT} ${LIBUTIL}
LDADD+= -lcrypt -lutil

BINOWN=	root
BINMODE=4555

.if (${USE_KERBEROS} != "no")
CPPFLAGS+= -DKERBEROS5 -I${DESTDIR}/usr/include/krb5
SRCS+=	krb5_passwd.c

LDADD+=	-lkrb5 -lcrypto -lasn1 -lcom_err -lroken -lcrypt
.endif

.include <bsd.prog.mk>

# getpwent.o: getpwent.c
# 	${COMPILE.c} -UYP ${.IMPSRC}
