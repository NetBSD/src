#	$NetBSD: Makefile,v 1.22 2005/02/09 13:57:57 xtraeme Exp $
#	@(#)Makefile	8.3 (Berkeley) 3/27/94

.include <bsd.own.mk>

PROG=	mount_portal
SRCS=	mount_portal.c activate.c conf.c pt_conf.c \
	pt_exec.c pt_file.c pt_tcp.c pt_filter.c
SUBDIR= examples

MAN=	mount_portal.8

WARNS=	3

CPPFLAGS+= -I${NETBSDSRCDIR}/sys

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
