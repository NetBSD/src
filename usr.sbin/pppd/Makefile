#	$Id: Makefile,v 1.2 1993/10/11 10:52:31 cgd Exp $

#DEBUG_FLAGS = -DDEBUGFSM -DDEBUGLCP -DDEBUGIPCP -DDEBUGUPAP -DDEBUGCHAP \
#	-DDEBUGMAIN
CFLAGS+=-DNO_DRAND48

PROG=	pppd
SRCS=	main.c magic.c fsm.c lcp.c ipcp.c upap.c logwtmp.c chap.c md5.c
MAN8=	pppd.0
SUBDIR=	pppstats chat
BINMODE=4555
BINOWN=	root

LDADD=	-lcrypt
DPADD=	${LIBCRYPT}

.include <bsd.prog.mk>
