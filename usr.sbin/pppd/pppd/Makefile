#	$Id: Makefile,v 1.1 1993/08/14 06:29:56 deraadt Exp $

#DEBUG_FLAGS = -DDEBUGFSM -DDEBUGLCP -DDEBUGIPCP -DDEBUGUPAP -DDEBUGCHAP \
#	-DDEBUGMAIN
CFLAGS+=-DNO_DRAND48

PROG=	pppd
SRCS=	main.c magic.c fsm.c lcp.c ipcp.c upap.c logwtmp.c chap.c md5.c
MAN8=	pppd.0
SUBDIR=	pppstats chat
BINMODE=4555
BINOWN=	root

.include <bsd.prog.mk>
