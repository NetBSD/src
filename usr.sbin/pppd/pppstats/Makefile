#	$Id: Makefile,v 1.1 1993/08/14 06:31:04 deraadt Exp $

PROG=	pppstats
SRCS=	pppstats.c
LDADD=	-lutil
CFLAGS+=-DKVMLIB
NOMAN=	noman
BINMODE=2555
GRPOWN=	kmem
BINDIR=	/usr/sbin

.include <bsd.prog.mk>
