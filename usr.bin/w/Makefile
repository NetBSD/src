#	from: @(#)Makefile	5.6 (Berkeley) 4/23/91
#	$Id: Makefile,v 1.3 1993/07/30 22:30:00 mycroft Exp $

PROG=	w
SRCS=	attime.c w.c proc_compare.c
MAN1=	w.0 uptime.0
DPADD=	${LIBUTIL}
LDADD=	-lutil
BINGRP=	kmem
BINMODE=2555
LINKS=	${BINDIR}/w ${BINDIR}/uptime

.include <bsd.prog.mk>
