#	$NetBSD: Makefile,v 1.20 1999/02/18 21:22:52 scottr Exp $
#	from: @(#)Makefile    8.3 (Berkeley) 4/2/94


PROG=	passwd
SRCS=	local_passwd.c yp_passwd.c passwd.c getpwent.c
.PATH:  ${.CURDIR}/../../lib/libc/gen
DPADD+= ${LIBRPCSVC} ${LIBCRYPT} ${LIBUTIL}
LDADD+= -lrpcsvc -lcrypt -lutil
CPPFLAGS+=-I${.CURDIR} -I${.CURDIR}/../../lib/libc/include -DYP

LINKS=	${BINDIR}/passwd ${BINDIR}/yppasswd
MLINKS=	passwd.1 yppasswd.1

BINOWN=	root
BINMODE=4555
.if defined(notyet)
INSTALLFLAGS=-fschg
.endif

.include <bsd.prog.mk>

getpwent.o: getpwent.c
	${COMPILE.c} -UYP ${.IMPSRC}
