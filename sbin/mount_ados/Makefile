#	$Id: Makefile,v 1.3 1995/01/18 08:37:48 mycroft Exp $

PROG=	mount_ados
SRCS=	mount_ados.c getmntopts.c
MAN=	mount_ados.8

MOUNT=	${.CURDIR}/../mount
CFLAGS+= -I${MOUNT}
.PATH:	${MOUNT}

.include <bsd.prog.mk>
