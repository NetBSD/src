#	from: @(#)Makefile	5.1 (Berkeley) 5/28/90
#	$Id: Makefile,v 1.4 1994/12/22 12:42:30 cgd Exp $

PROG=	named-xfer
SRCS=	named-xfer.c db_glue.c
CFLAGS+=-I${.CURDIR}/..
.PATH:	${.CURDIR}/..
BINDIR=	/usr/libexec
MAN=	named-xfer.8

.include "../../Makefile.inc"
.include <bsd.prog.mk>
