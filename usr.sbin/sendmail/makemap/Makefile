#	@(#)Makefile	8.4 (Berkeley) 6/10/97

PROG=	makemap
MAN=	makemap.8
CFLAGS+=-I${.CURDIR}/../src -DNEWDB -DNOT_SENDMAIL

SRCS=	makemap.c safefile.c

.PATH: ${.CURDIR}/../src
.include "../../Makefile.inc"
.include <bsd.prog.mk>
