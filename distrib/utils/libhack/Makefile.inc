# $NetBSD: Makefile.inc,v 1.6 1999/05/06 18:48:23 wrstuden Exp $
# Include this fragment to build libhack.o
# It is .o and not .a to make sure these are the
# objects you get (and not the ones in libc.a)

HACKOBJS= getcap.o getgrent.o gethost.o getnet.o getnetgr.o getpwent.o \
	setlocale.o yplib.o

libhack.o : $(HACKOBJS)
	$(LD) -r -o $@ $(HACKOBJS)

getcap.o : ${HACKSRC}/getcap.c

getgrent.o : ${HACKSRC}/getgrent.c

gethost.o : ${HACKSRC}/gethost.c

getnet.o : ${HACKSRC}/getnet.c

getnetgr.o : ${HACKSRC}/getnetgr.c

getpwent.o : ${HACKSRC}/getpwent.c

setlocale.o : ${HACKSRC}/setlocale.c

yplib.o : ${HACKSRC}/yplib.c

