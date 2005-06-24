#	$NetBSD: Makefile,v 1.10 2005/06/24 23:21:09 christos Exp $
#	$OpenBSD: Makefile,v 1.4 2002/06/05 17:34:56 mickey Exp $

.include <bsd.own.mk>

WARNS=3
PROG=	systrace
DPADD+=		${LIBEVENT}
LDADD+=		-levent

SRCS=	cradle.c filter.c intercept-translate.c intercept.c \
	netbsd-syscalls.c util.c \
	policy.c systrace-errno.h systrace-error.c \
	systrace-translate.c systrace.c alias.c register.c \
	parse.y lex.l

CPPFLAGS+= -I. -I${.CURDIR} -I${NETBSDSRCDIR}/sys

YHEADER=yes

.include <bsd.prog.mk>
