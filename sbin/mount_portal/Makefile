#	$NetBSD: Makefile,v 1.20 2003/03/22 12:44:01 jdolecek Exp $
#	@(#)Makefile	8.3 (Berkeley) 3/27/94

.include <bsd.own.mk>

PROG=	mount_portal
SRCS=	mount_portal.c activate.c conf.c pt_conf.c \
	pt_exec.c pt_file.c pt_tcp.c pt_filter.c
SUBDIR= examples

MAN=	mount_portal.8

CPPFLAGS+= -I${NETBSDSRCDIR}/sys
CPPFLAGS+= -DINET6

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
