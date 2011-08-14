#	$NetBSD: Makefile,v 1.25 2011/08/14 11:34:11 christos Exp $
#	@(#)Makefile	8.3 (Berkeley) 3/27/94

.include <bsd.own.mk>

PROG=	mount_portal
SRCS=	puffs_portal.c activate.c conf.c pt_conf.c \
	pt_exec.c pt_file.c pt_tcp.c pt_filter.c
SUBDIR= examples

MAN=	mount_portal.8

DPADD+=${LIBUTIL} ${LIBPUFFS}
LDADD+=-lutil -lpuffs

COPTS.pt_filter.c = -Wno-format-nonliteral

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
