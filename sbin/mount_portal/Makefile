#	$NetBSD: Makefile,v 1.24 2009/12/05 20:11:02 pooka Exp $
#	@(#)Makefile	8.3 (Berkeley) 3/27/94

.include <bsd.own.mk>

PROG=	mount_portal
SRCS=	puffs_portal.c activate.c conf.c pt_conf.c \
	pt_exec.c pt_file.c pt_tcp.c pt_filter.c
SUBDIR= examples

MAN=	mount_portal.8

DPADD+=${LIBUTIL} ${LIBPUFFS}
LDADD+=-lutil -lpuffs

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
