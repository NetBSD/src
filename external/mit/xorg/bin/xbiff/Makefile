#	$NetBSD: Makefile,v 1.1.1.1.6.2 2008/10/19 22:41:00 haad Exp $

.include <bsd.own.mk>

PROG=	xbiff
SRCS=	xbiff.c Mailbox.c

LDADD+=	-lXext -lXaw -lXmu -lXt -lX11 -lSM -lICE -lXau
DPADD+=	${LIBXEXT} ${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBX11} ${LIBSM} ${LIBICE} \
	${LIBXAU}

#cc  -O2 -I/usr/pkg/include  -L/usr/pkg/lib -Wl,-R/usr/pkg/lib -o xbiff  xbiff-Mailbox.o xbiff-xbiff.o -Wl,-R/usr/pkg/lib -L/usr/pkg/lib -lXext   -Wl,-R/usr/pkg/lib -L/usr/pkg/lib -lXaw8 -lXmu -lXt -lX11 -lSM -lICE -lXau

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
