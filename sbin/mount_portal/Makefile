#	from: @(#)Makefile	8.1 (Berkeley) 6/5/93
#	$Id: Makefile,v 1.2 1994/01/12 20:16:59 mycroft Exp $

.if exists(${.CURDIR}/../../sys)
CFLAGS+=-I${.CURDIR}/../../sys
.else
CFLAGS+=-I/sys
.endif

PROG=	mount_portal
PTAL=	pt_conf.c pt_exec.c pt_file.c pt_tcp.c
SRCS=	mount_portal.c conf.c activate.c ${PTAL}
MAN8=	mount_portal.0

.include <bsd.prog.mk>
