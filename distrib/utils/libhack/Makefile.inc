# $NetBSD: Makefile.inc,v 1.8 1999/06/27 12:56:01 mrg Exp $
# Include this fragment to build libhack.o
# It is .o and not .a to make sure these are the
# objects you get (and not the ones in libc.a)

HACKOBJS= getcap.o getgrent.o gethost.o getnet.o getnetgr.o getpwent.o \
	localeconv.o perror.o setlocale.o \
	strerror.o strsignal.o utmp.o yplib.o
.ifndef NOLIBHACKOPENDIR
HACKOBJS+= opendir.o
.endif

libhack.o : $(HACKOBJS)
	$(LD) -r -o $@ $(HACKOBJS)

.PATH: ${HACKSRC}

