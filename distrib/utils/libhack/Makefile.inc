# $NetBSD: Makefile.inc,v 1.17 2004/04/25 06:45:02 christos Exp $
#
# Include this fragment to build libhack.o
# It is .o and not .a to make sure these are the
# objects you get (and not the ones in libc.a)
#

#
# Required variables:
#	HACKSRC		This directory
#
# Optional variables:
#	HACKOBJS	Extra objects to build.
#			The following extra objects are available:
#			gethost.o	cut-down gethostby{name,addr}()
#					that only supports /etc/hosts
#			

CPPFLAGS+=	-DSMALL
HACKOBJS+=	getcap.o getgrent.o getnet.o getnetgr.o getpwent.o \
		localeconv.o perror.o setlocale.o \
		strerror.o strsignal.o utmp.o yplib.o

libhack.o: ${HACKOBJS}
	${LD} -r -o $@ ${HACKOBJS}


# XXX .PATH won't work here, because of crunchgen used by various builds

getcap.o:	${HACKSRC}/../../../lib/libc/gen/getcap.c
getgrent.o:	${HACKSRC}/getgrent.c
gethost.o:	${HACKSRC}/gethost.c
getnet.o:	${HACKSRC}/getnet.c
getnetgr.o:	${HACKSRC}/getnetgr.c
getpwent.o:	${HACKSRC}/getpwent.c
localeconv.o:	${HACKSRC}/localeconv.c
perror.o:	${HACKSRC}/perror.c
setlocale.o:	${HACKSRC}/setlocale.c
strerror.o:	${HACKSRC}/strerror.c
strsignal.o:	${HACKSRC}/strsignal.c
utmp.o:		${HACKSRC}/utmp.c
yplib.o:	${HACKSRC}/yplib.c
