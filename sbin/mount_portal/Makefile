#	$NetBSD: Makefile,v 1.27 2017/05/09 23:26:49 kamil Exp $
#	@(#)Makefile	8.3 (Berkeley) 3/27/94

.include <bsd.own.mk>

PROG=	mount_portal
SRCS=	puffs_portal.c activate.c conf.c pt_conf.c \
	pt_file.c pt_tcp.c pt_filter.c
SUBDIR= examples

MAN=	mount_portal.8

DPADD+=${LIBUTIL} ${LIBPUFFS}
LDADD+=-lutil -lpuffs
CPPFLAGS+=	-D_KERNTYPES

COPTS.pt_filter.c = -Wno-format-nonliteral

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
