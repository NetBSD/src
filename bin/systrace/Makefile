#	$NetBSD: Makefile,v 1.3 2002/06/18 02:55:19 thorpej Exp $
#	$OpenBSD: Makefile,v 1.4 2002/06/05 17:34:56 mickey Exp $

PROG=	systrace
SRCS=	filter.c intercept-translate.c intercept.c \
	netbsd-syscalls.c util.c \
	policy.c systrace-errno.h systrace-error.c \
	systrace-translate.c systrace.c \
	parse.y lex.l

CPPFLAGS+= -I. -I${.CURDIR}/../../sys

YHEADER=yes

.include <bsd.prog.mk>
