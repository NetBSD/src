# $NetBSD: Makefile.inc,v 1.5 1999/03/13 19:08:43 sommerfe Exp $
# Include this fragment to build libhack.o
# It is .o and not .a to make sure these are the
# objects you get (and not the ones in libc.a)

HACKOBJS= getcap.o getgrent.o gethost.o getnet.o getnetgr.o getpwent.o \
	setlocale.o yplib.o

libhack.o : $(HACKOBJS)
	$(LD) -r -o $@ $(HACKOBJS)

getcap.o : ${HACKSRC}/getcap.c
	${CC} ${CFLAGS} -c ${HACKSRC}/getcap.c

getgrent.o : ${HACKSRC}/getgrent.c
	${CC} ${CFLAGS} -c ${HACKSRC}/getgrent.c

gethost.o : ${HACKSRC}/gethost.c
	${CC} ${CFLAGS} -c ${HACKSRC}/gethost.c

getnet.o : ${HACKSRC}/getnet.c
	${CC} ${CFLAGS} -c ${HACKSRC}/getnet.c

getnetgr.o : ${HACKSRC}/getnetgr.c
	${CC} ${CFLAGS} -c ${HACKSRC}/getnetgr.c

getpwent.o : ${HACKSRC}/getpwent.c
	${CC} ${CFLAGS} -c ${HACKSRC}/getpwent.c

setlocale.o : ${HACKSRC}/setlocale.c
	${CC} ${CFLAGS} -c ${HACKSRC}/setlocale.c

yplib.o : ${HACKSRC}/yplib.c
	${CC} ${CFLAGS} -c ${HACKSRC}/yplib.c

