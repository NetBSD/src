#	$Id: Makefile,v 1.2 1994/01/05 20:57:27 jtc Exp $

PROG=	cron
SRCS=	cron.c database.c user.c entry.c job.c do_command.c \
	misc.c env.c popen.c compat.c
CFLAGS+=-I${.CURDIR} -DCRONDIR='"/var/cron"' 
MAN8=	cron.0

.include <bsd.prog.mk>
