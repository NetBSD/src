#	$NetBSD: Makefile,v 1.15 1998/09/18 22:14:46 thorpej Exp $
#	@(#)Makefile	8.3 (Berkeley) 3/27/94

PROG=	mount_portal
SRCS=	mount_portal.c activate.c conf.c getmntopts.c pt_conf.c \
	pt_exec.c pt_file.c pt_tcp.c

DPADD+=	${LIBCOMPAT}
LDADD+=	-lcompat

MAN=	mount_portal.8

MOUNT=	${.CURDIR}/../mount
CPPFLAGS+= -I${.CURDIR}/../../sys -I${MOUNT}
.PATH:	${MOUNT}

.include <bsd.prog.mk>
