#	from: @(#)Makefile	8.3 (Berkeley) 3/27/94
#	$Id: Makefile,v 1.2 1994/06/10 17:34:53 pk Exp $

PROG=	mount_null
SRCS=	mount_null.c getmntopts.c
MAN8=	mount_null.0

MOUNT=	${.CURDIR}/../mount
CFLAGS+= -I${DESTDIR}/sys -I${MOUNT}
.PATH:	${MOUNT}

.include <bsd.prog.mk>
