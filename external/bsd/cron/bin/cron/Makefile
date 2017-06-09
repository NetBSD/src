#	$NetBSD: Makefile,v 1.4 2017/06/09 17:36:29 christos Exp $

BINDIR=	/usr/sbin
PROG=	cron
SRCS=	cron.c database.c do_command.c entry.c env.c job.c \
	misc.c pam_auth.c popen.c pw_dup.c user.c
CPPFLAGS+=-I${.CURDIR} -DLOGIN_CAP -DUSE_PAM
DPADD+=${LIBPAM} ${LIBUTIL}
LDADD+=-lpam -lutil
MAN=	cron.8

CWARNFLAGS.clang+=	-Wno-string-plus-int

.include <bsd.prog.mk>
