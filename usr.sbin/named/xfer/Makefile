#	from: @(#)Makefile	5.1 (Berkeley) 5/28/90
#	$Id: Makefile,v 1.3 1994/11/29 23:54:28 glass Exp $

PROG=	named-xfer
SRCS=	named-xfer.c db_glue.c
CFLAGS+=-I${.CURDIR}/..
.PATH:	${.CURDIR}/..
BINDIR=	/usr/libexec
MAN8=	named-xfer.0

.include "../../Makefile.inc"
.include <bsd.prog.mk>
