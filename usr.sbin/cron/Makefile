#	$Id: Makefile,v 1.5 1994/12/22 11:33:57 cgd Exp $

PROG=	cron
SRCS=	cron.c database.c user.c entry.c job.c do_command.c \
	misc.c env.c popen.c 
CFLAGS+=-I${.CURDIR}
MAN=	cron.8

.include <bsd.prog.mk>
