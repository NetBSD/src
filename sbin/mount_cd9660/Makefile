#	from: @(#)Makefile	8.3 (Berkeley) 3/27/94
#	$Id: Makefile,v 1.1 1994/06/08 19:08:27 mycroft Exp $

PROG=	mount_cd9660
SRCS=	mount_cd9660.c getmntopts.c
MAN8=	mount_cd9660.0

MOUNT=	${.CURDIR}/../mount
CFLAGS+= -I${MOUNT}
.PATH:	${MOUNT}

.include <bsd.prog.mk>
