# $NetBSD: Makefile.inc,v 1.12 2001/10/05 01:17:28 jmc Exp $
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

.if ${MACHINE_ARCH} != "sparc64"
DBG= -Os
.endif

# XXX .PATH won't work here, because of crunchgen used in the i386 build

getcap.o: ${HACKSRC}/getcap.c
getgrent.o: ${HACKSRC}/getgrent.c
gethost.o: ${HACKSRC}/gethost.c
getnet.o: ${HACKSRC}/getnet.c
getnetgr.o: ${HACKSRC}/getnetgr.c
getpwent.o: ${HACKSRC}/getpwent.c
localeconv.o: ${HACKSRC}/localeconv.c
perror.o: ${HACKSRC}/perror.c
setlocale.o: ${HACKSRC}/setlocale.c
strerror.o: ${HACKSRC}/strerror.c
strsignal.o: ${HACKSRC}/strsignal.c
utmp.o: ${HACKSRC}/utmp.c
yplib.o: ${HACKSRC}/yplib.c
opendir.o: ${HACKSRC}/opendir.c
