#	from: @(#)Makefile	5.1 (Berkeley) 5/11/90
#	$Id: Makefile,v 1.2 1993/07/30 22:12:04 mycroft Exp $

PROG=	pac
CFLAGS+=-I${.CURDIR}/../common_source
MAN8=	pac.0
SRCS=	pac.c printcap.c
.PATH:	${.CURDIR}/../common_source

.include "../../Makefile.inc"
.include <bsd.prog.mk>
