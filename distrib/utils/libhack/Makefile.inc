# $NetBSD: Makefile.inc,v 1.4 1997/12/12 03:36:38 gwr Exp $
# Include this fragment to build libhack.o
# It is .o and not .a to make sure these are the
# objects you get (and not the ones in libc.a)

HACKOBJS= getgrent.o gethost.o getnet.o getnetgr.o getpwent.o \
	setlocale.o yplib.o

libhack.o : $(HACKOBJS)
	$(LD) -r -o $@ $(HACKOBJS)


getgrent.o : ${HACKSRC}/getgrent.c
	$(CC) -c ${HACKSRC}/getgrent.c

gethost.o : ${HACKSRC}/gethost.c
	$(CC) -c ${HACKSRC}/gethost.c

getnet.o : ${HACKSRC}/getnet.c
	$(CC) -c ${HACKSRC}/getnet.c

getnetgr.o : ${HACKSRC}/getnetgr.c
	$(CC) -c ${HACKSRC}/getnetgr.c

getpwent.o : ${HACKSRC}/getpwent.c
	$(CC) -c ${HACKSRC}/getpwent.c

setlocale.o : ${HACKSRC}/setlocale.c
	$(CC) -c ${HACKSRC}/setlocale.c

yplib.o : ${HACKSRC}/yplib.c
	$(CC) -c ${HACKSRC}/yplib.c

