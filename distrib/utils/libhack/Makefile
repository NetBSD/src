# $NetBSD: Makefile,v 1.11 2001/08/20 12:19:43 martin Exp $
#
# Stubs to kill off some things from libc:
# This save space on a boot system.
# (See Makefile.inc also.)

LIB=	hack
SRCS=	getcap.c getgrent.c gethost.c getnet.c getnetgr.c getpwent.c \
	localeconv.c perror.c setlocale.c \
	strerror.c strsignal.c utmp.c yplib.c
.ifndef NOLIBHACKOPENDIR
SRCS+=        opendir.c
.endif


HACKOBJS= getcap.o getgrent.o gethost.o getnet.o getnetgr.o getpwent.o \
	localeconv.o perror.o setlocale.o \
	strerror.o strsignal.o utmp.o yplib.o
.ifndef NOLIBHACKOPENDIR
HACKOBJS+= opendir.o
.endif

.if ${MACHINE_ARCH} != "sparc64"
COPTS+= -Os
.endif
WARNS=	1
MKPIC=	no
MKLINT= no
MKPROFILE=no

all: libhack.a libhack.o

libhack.o : $(HACKOBJS)
	$(LD) -r -o $@ $(HACKOBJS)

CLEANFILES+=  libhack.o

tpwent: getpwent.c
	$(CC) -g -o $@.o -DTEST_MAIN -c getpwent.c
	$(CC) -o $@ $@.o

.include <bsd.lib.mk>
