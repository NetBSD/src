#	$NetBSD: Makefile,v 1.21 1999/07/12 22:11:38 thorpej Exp $
#	from: @(#)Makefile    8.3 (Berkeley) 4/2/94

SRCTOP=	../..
.include <bsd.crypto.mk>

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

.if defined(CRYPTOPATH)
.include "${CRYPTOPATH}/usr.bin/passwd/Makefile.frag"
.endif

.include <bsd.prog.mk>

getpwent.o: getpwent.c
	${COMPILE.c} -UYP ${.IMPSRC}
