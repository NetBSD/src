# $NetBSD: Makefile,v 1.15 2002/03/25 07:43:49 lukem Exp $
#
# Stubs to kill off some things from libc:
# This save space on a boot system.
# (See Makefile.inc also.)

LIB=		hack
SRCS=		getcap.c getgrent.c getnet.c getnetgr.c getpwent.c \
		localeconv.c perror.c setlocale.c \
		strerror.c strsignal.c utmp.c yplib.c

WARNS=		1
NOLINKLIB=	# defined
NOLINT= 	# defined
NOPIC=		# defined
NOPROFILE=	# defined

realall: libhack.o

libhack.o: libhack.a
	${LD} -r -o $@ --whole-archive libhack.a

CLEANFILES+=	libhack.o

tpwent: getpwent.c
	${CC} -g -o $@.o -DTEST_MAIN -c getpwent.c
	${CC} -o $@ $@.o

.include <bsd.lib.mk>
