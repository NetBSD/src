#	$Id: Makefile,v 1.4 1993/11/09 04:53:15 paulus Exp $

#DEBUG_FLAGS = -DDEBUGALL

PROG=	pppd
SRCS=	main.c magic.c fsm.c lcp.c ipcp.c upap.c chap.c md5.c \
	auth.c options.c sys-bsd.c
MAN8=	pppd.0
SUBDIR=	pppstats chat
BINMODE=4555
BINOWN=	root

LDADD=	-lcrypt -lutil
DPADD=	${LIBCRYPT} ${LIBUTIL}

.include <bsd.prog.mk>
