# $NetBSD: Makefile.inc,v 1.7 1999/05/19 03:58:13 gwr Exp $
# Include this fragment to build libhack.o
# It is .o and not .a to make sure these are the
# objects you get (and not the ones in libc.a)

HACKOBJS= getcap.o getgrent.o gethost.o getnet.o getnetgr.o getpwent.o \
	localeconv.o opendir.o perror.o setlocale.o \
	strerror.o strsignal.o utmp.o yplib.o

libhack.o : $(HACKOBJS)
	$(LD) -r -o $@ $(HACKOBJS)

.PATH: ${HACKSRC}

