#	$Id: Makefile,v 1.4 1993/08/02 17:50:18 mycroft Exp $

PROG=	crond
SRCS=	crond.c database.c user.c entry.c misc.c job.c do_command.c env.c
CFLAGS+=-I${.CURDIR} -DBSD -DCRONDIR='"/var/cron"' 
MAN8=	crond.0

.include <bsd.prog.mk>
