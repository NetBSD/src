# $NetBSD: Makefile,v 1.19 2007/02/03 19:55:51 christos Exp $
#
# Stubs to kill off some things from libc:
# This save space on a boot system.
# (See Makefile.inc also.)

.PATH.c: ${.CURDIR}/../../../lib/libc/gen

CPPFLAGS+=	-DSMALL

LIB=		hack
SRCS=		getcap.c getgrent.c getnet.c getnetgr.c getpwent.c \
		localeconv.c perror.c setlocale.c \
		mbrtowc.c wcrtomb.c \
		strerror.c strsignal.c syslog.c utmp.c yplib.c

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
