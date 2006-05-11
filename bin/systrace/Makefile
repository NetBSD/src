#	$NetBSD: Makefile,v 1.12 2006/05/11 23:16:28 mrg Exp $
#	$OpenBSD: Makefile,v 1.4 2002/06/05 17:34:56 mickey Exp $

.include <bsd.own.mk>

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

.if ${HAVE_GCC} == 4
COPTS.intercept.c+=   -Wno-pointer-sign
.endif

.include <bsd.prog.mk>
