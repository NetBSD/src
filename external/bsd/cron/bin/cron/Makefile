#	$NetBSD: Makefile,v 1.6 2017/06/14 17:37:40 christos Exp $

.include <bsd.own.mk>

BINDIR=	/usr/sbin
PROG=	cron
SRCS=	cron.c database.c do_command.c entry.c env.c job.c \
	misc.c popen.c pw_dup.c user.c
CPPFLAGS+=-I${.CURDIR} -DLOGIN_CAP
DPADD+=	${LIBUTIL}
LDADD+=	-lutil
MAN=	cron.8

.if (${MKPAM} != "no")
SRCS+=		pam_auth.c
CPPFLAGS+=	-DUSE_PAM
DPADD+=		${LIBPAM} ${PAM_STATIC_DPADD}
LDADD+=		-lpam ${PAM_STATIC_LDADD}
.endif

CWARNFLAGS.clang+=	-Wno-string-plus-int

.include <bsd.prog.mk>
