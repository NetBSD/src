# $NetBSD: Makefile,v 1.25.6.1 2019/06/10 21:42:37 christos Exp $
#
# Stubs to kill off some things from libc:
# This save space on a boot system.
# (See Makefile.inc also.)

LIB=	hack
WARNS=		1
NOLINKLIB=	# defined
NOLINT= 	# defined
NOPIC=		# defined
NOPROFILE=	# defined

HACKSRC?=${.CURDIR}
HACKOBJ?=${.OBJDIR}

.include "Makefile.inc"

OBJS=		${HACKOBJS}

realall: libhack.o

libhack.o: libhack.a
	${LD} -r -o $@ --whole-archive libhack.a

CLEANFILES+=	libhack.o

tpwent: getpwent.c
	${CC} -g -o $@.o -DTEST_MAIN -c getpwent.c
	${CC} -o $@ $@.o

.include <bsd.lib.mk>
