#	$NetBSD: Makefile,v 1.27 2000/07/06 11:16:50 ad Exp $
#	from: @(#)Makefile    8.3 (Berkeley) 4/2/94

.include <bsd.own.mk>

PROG=	passwd
SRCS=	local_passwd.c passwd.c pwd_gensalt.c yp_passwd.c
DPADD+= ${LIBRPCSVC} ${LIBCRYPT} ${LIBUTIL}
LDADD+= -lrpcsvc -lcrypt -lutil
CPPFLAGS+=-I${.CURDIR} -DYP -DLOGIN_CAP

LINKS=	${BINDIR}/passwd ${BINDIR}/yppasswd
MLINKS=	passwd.1 yppasswd.1

BINOWN=	root
BINMODE=4555
.if defined(notyet)
INSTALLFLAGS=-fschg
.endif

.if (${MKKERBEROS} != "no")
CPPFLAGS+= -DKERBEROS5 -I${DESTDIR}/usr/include/krb5
SRCS+=	krb5_passwd.c
LDADD+=	-lkrb5 -lcrypto -lasn1 -lcom_err -lroken -lcrypt
.endif

.include <bsd.prog.mk>

# getpwent.o: getpwent.c
# 	${COMPILE.c} -UYP ${.IMPSRC}
