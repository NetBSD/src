#	from: @(#)Makefile	5.11 (Berkeley) 2/19/91
#	$Id: Makefile,v 1.6 1993/10/07 02:19:49 cgd Exp $

PROG=	passwd
SRCS=	local_passwd.c yp_passwd.c passwd.c pw_copy.c pw_util.c getpwent.c
.PATH:  ${.CURDIR}/../../usr.bin/chpass ${.CURDIR}/../../usr.sbin/vipw \
	${.CURDIR}/../rlogin ${.CURDIR}/../../lib/libc/gen
CFLAGS+=-I${.CURDIR} -DYP
BINOWN=	root
BINMODE=4555
DPADD+= ${LIBRPCSVC} ${LIBCRYPT}
LDADD+= -lrpcsvc -lcrypt

.include <bsd.prog.mk>

getpwent.o: getpwent.c
	${CC} ${CFLAGS} -UYP -c ${.IMPSRC}
