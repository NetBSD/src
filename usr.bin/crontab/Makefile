#	$Id: Makefile,v 1.6 1994/01/05 21:03:46 jtc Exp $

PROG=	crontab
SRCS=	crontab.c misc.c entry.c env.c compat.c
CFLAGS+=-I${.CURDIR} -I${.CURDIR}/../../libexec/cron -DDEBUGGING=1 
BINOWN =root
BINMODE=4111

.include <bsd.prog.mk>
.PATH: ${.CURDIR}/../../libexec/cron
