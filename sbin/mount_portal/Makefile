#	$NetBSD: Makefile,v 1.18 2000/12/31 06:03:52 itojun Exp $
#	@(#)Makefile	8.3 (Berkeley) 3/27/94

PROG=	mount_portal
SRCS=	mount_portal.c activate.c conf.c getmntopts.c pt_conf.c \
	pt_exec.c pt_file.c pt_tcp.c pt_filter.c
SUBDIR= examples

MAN=	mount_portal.8

MOUNT=	${.CURDIR}/../mount
CPPFLAGS+= -I${.CURDIR}/../../sys -I${MOUNT}
.PATH:	${MOUNT}
CPPFLAGS+= -DINET6

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
