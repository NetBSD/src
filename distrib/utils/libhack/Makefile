# $NetBSD: Makefile,v 1.6.2.1 1999/06/21 03:25:36 perry Exp $
#
# Stubs to kill off some things from libc:
# This save space on a boot system.
# (See Makefile.inc also.)

LIB=	hack
SRCS=	getcap.c getgrent.c gethost.c getnet.c getnetgr.c getpwent.c \
	localeconv.c opendir.c perror.c setlocale.c \
	strerror.c strsignal.c utmp.c yplib.c

WARNS=	1
MKPIC=	no
MKLINT= no
MKPROFILE=no

all: libhack.a libhack.o

CLEANFILES+=  libhack.o
libhack.o : $(OBJS)
	$(LD) -r -o $@ $(OBJS)

tpwent: getpwent.c
	$(CC) -g -o $@.o -DTEST_MAIN -c getpwent.c
	$(CC) -o $@ $@.o

.include <bsd.lib.mk>
