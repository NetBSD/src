#	$NetBSD: Makefile,v 1.16 1999/08/16 06:55:27 bgrayson Exp $
#	@(#)Makefile	8.3 (Berkeley) 3/27/94

PROG=	mount_portal
SRCS=	mount_portal.c activate.c conf.c getmntopts.c pt_conf.c \
	pt_exec.c pt_file.c pt_tcp.c pt_filter.c
SUBDIR= examples

DPADD+=	${LIBCOMPAT}
LDADD+=	-lcompat

MAN=	mount_portal.8

MOUNT=	${.CURDIR}/../mount
CPPFLAGS+= -I${.CURDIR}/../../sys -I${MOUNT}
.PATH:	${MOUNT}

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
