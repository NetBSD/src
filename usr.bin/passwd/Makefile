#	$NetBSD: Makefile,v 1.25 2000/06/20 06:00:37 thorpej Exp $
#	from: @(#)Makefile    8.3 (Berkeley) 4/2/94

PROG=	passwd
SRCS=	local_passwd.c yp_passwd.c passwd.c
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

CPPFLAGS+= -DKERBEROS5 -I${DESTDIR}/usr/include/krb5
SRCS+=	krb5_passwd.c
LDADD+=	-lkrb5 -lcrypto -lasn1 -lcom_err -lroken -lcrypt

.include <bsd.prog.mk>

# getpwent.o: getpwent.c
# 	${COMPILE.c} -UYP ${.IMPSRC}
