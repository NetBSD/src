# $NetBSD: Makefile,v 1.22.14.2 2014/08/19 23:45:45 tls Exp $
#
# Stubs to kill off some things from libc:
# This save space on a boot system.
# (See Makefile.inc also.)

.PATH.c: ${.CURDIR}/../../../lib/libc/gen ${.CURDIR}/../../../lib/libc/locale

HACKSRC?=${.CURDIR}
HACKOBJ?=${.OBJDIR}

CPPFLAGS+=	-DSMALL
CPPFLAGS.runetable.c+=	-I${HACKSRC}/../../../lib/libc/citrus \
			-DALL_80_TO_FF_SW1
CPPFLAGS.syslog.c+=	-I${HACKSRC}/../../../lib/libc/include
CPPFLAGS.gethost.c+=	-I${HACKSRC}/../../../lib/libc/net

LIB=		hack
SRCS=		getcap.c getgrent.c getnet.c getnetgr.c getpwent.c \
		localeconv.c multibyte.c perror.c runetable.c setlocale.c \
		strerror.c strsignal.c syslog.c utmp.c yplib.c gethost.c

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
