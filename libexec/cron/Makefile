#	$Id: Makefile,v 1.3 1994/01/05 21:10:29 jtc Exp $

PROG=	cron
SRCS=	cron.c database.c user.c entry.c job.c do_command.c \
	misc.c env.c popen.c compat.c
CFLAGS+=-I${.CURDIR}
MAN8=	cron.0

.include <bsd.prog.mk>
