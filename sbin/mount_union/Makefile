#	from: @(#)Makefile	8.3 (Berkeley) 3/27/94
#	$Id: Makefile,v 1.4 1995/01/09 10:20:11 mycroft Exp $

PROG=	mount_union
SRCS=	mount_union.c getmntopts.c
MAN=	mount_union.8

MOUNT=	${.CURDIR}/../mount
CFLAGS+= -I${.CURDIR}/../../sys -I${MOUNT}
.PATH:	${MOUNT}

.include <bsd.prog.mk>
