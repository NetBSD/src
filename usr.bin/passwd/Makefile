#	from: @(#)Makefile	5.11 (Berkeley) 2/19/91
#	$Id: Makefile,v 1.12 1996/05/15 21:55:25 jtc Exp $

PROG=	passwd
SRCS=	local_passwd.c yp_passwd.c passwd.c getpwent.c
.PATH:  ${.CURDIR}/../../lib/libc/gen
DPADD+= ${LIBRPCSVC} ${LIBCRYPT} ${LIBUTIL}
LDADD+= -lrpcsvc -lcrypt -lutil
CFLAGS+=-I${.CURDIR} -DYP

.if defined(KERBEROS5)
SRCS+= krb5_passwd.c
CFLAGS+= -DKERBEROS5
DPADD+=	${LIBKRB5} ${LIBCRYPTO} 
LDADD+=	-lkrb5 -lcrypto
.elif defined(KERBEROS)
SRCS+= krb_passwd.c des_rw.c
CFLAGS+= -DKERBEROS -DCRYPT
DPADD+=	${LIBKRB} ${LIBDES}
LDADD+=	-lkrb -ldes
.endif

LINKS=	${BINDIR}/passwd ${BINDIR}/yppasswd
MLINKS=	passwd.1 yppasswd.1

BINOWN=	root
BINMODE=4555

.include <bsd.prog.mk>

getpwent.o: getpwent.c
	${COMPILE.c} -UYP ${.IMPSRC}
