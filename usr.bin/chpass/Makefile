#	from: @(#)Makefile	5.5 (Berkeley) 2/19/91
#	$Id: Makefile,v 1.4 1995/01/07 23:14:32 mycroft Exp $

PROG=	chpass
SRCS=	chpass.c edit.c field.c pw_copy.c pw_scan.c pw_util.c pw_yp.c table.c util.c getpwent.c
BINOWN=	root
BINMODE=4555
.PATH:	${.CURDIR}/../../usr.sbin/pwd_mkdb ${.CURDIR}/../../usr.sbin/vipw \
	${.CURDIR}/../../lib/libc/gen
LINKS=	${BINDIR}/chpass ${BINDIR}/chfn ${BINDIR}/chpass ${BINDIR}/chsh
MLINKS=	chpass.1 chfn.1 chpass.1 chsh.1
CFLAGS+=-DYP
DPADD+= ${LIBRPCSVC}
LDADD+= -lrpcsvc

.include <bsd.prog.mk>

getpwent.o: getpwent.c
	${COMPILE.c} -UYP ${.IMPSRC}
