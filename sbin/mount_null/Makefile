#	from: @(#)Makefile	8.3 (Berkeley) 3/27/94
#	$Id: Makefile,v 1.3 1994/08/03 21:07:14 deraadt Exp $

PROG=	mount_null
SRCS=	mount_null.c getmntopts.c
MAN8=	mount_null.0

MOUNT=	${.CURDIR}/../mount
CFLAGS+= -I${.CURDIR}/../../sys -I${MOUNT}
.PATH:	${MOUNT}

.include <bsd.prog.mk>
