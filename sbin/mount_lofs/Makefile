#	from: @(#)Makefile	8.1 (Berkeley) 6/5/93
#	$Id: Makefile,v 1.2 1994/01/12 20:16:35 mycroft Exp $

.if exists(${.CURDIR}/../../sys)
CFLAGS+=-I${.CURDIR}/../../sys
.else
CFLAGS+=-I/sys
.endif

PROG=	mount_lofs
MAN8=	mount_lofs.0

.include <bsd.prog.mk>
