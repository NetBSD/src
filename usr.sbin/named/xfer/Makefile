#	from: @(#)Makefile	5.1 (Berkeley) 5/28/90
#	$Id: Makefile,v 1.2 1993/07/30 21:41:29 mycroft Exp $

PROG=	named-xfer
SRCS=	named-xfer.c db_glue.c
CFLAGS+=-I${.CURDIR}/..
.PATH:	${.CURDIR}/..
BINDIR=	/usr/libexec
NOMAN=	noman

.include "../../Makefile.inc"
.include <bsd.prog.mk>
