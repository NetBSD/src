#	$Id: Makefile,v 1.4 1994/01/20 02:56:55 jtc Exp $

PROG=	cron
SRCS=	cron.c database.c user.c entry.c job.c do_command.c \
	misc.c env.c popen.c 
CFLAGS+=-I${.CURDIR}
MAN8=	cron.0

.include <bsd.prog.mk>
