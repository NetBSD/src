#	from: @(#)Makefile	5.11 (Berkeley) 2/19/91
#	$Id: Makefile,v 1.7 1994/07/27 03:28:11 brezak Exp $

PROG=	passwd
SRCS=	local_passwd.c yp_passwd.c passwd.c pw_copy.c pw_util.c getpwent.c
.PATH:  ${.CURDIR}/../../usr.bin/chpass ${.CURDIR}/../../usr.sbin/vipw \
	${.CURDIR}/../rlogin ${.CURDIR}/../../lib/libc/gen
DPADD+= ${LIBRPCSVC} ${LIBCRYPT}
LDADD+= -lrpcsvc -lcrypt
CFLAGS+=-I${.CURDIR} -DYP

.if defined(KERBEROS5)
SRCS+= krb5_passwd.c
CFLAGS+= -DKERBEROS5
DPADD+=	${LIBKRB5} ${LIBCRYPTO}
LDADD+=	-lkrb5 -lcrypto
.elif defined(KERBEROS)
SRCS+= krb_passwd.c
CFLAGS+= -DKERBEROS
DPADD+=	${LIBKRB} ${LIBDES}
LDADD+=	-lkrb -ldes
.endif

BINOWN=	root
BINMODE=4555

.include <bsd.prog.mk>

getpwent.o: getpwent.c
	${CC} ${CFLAGS} -UYP -c ${.IMPSRC}
