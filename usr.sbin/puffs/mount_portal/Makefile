#	$NetBSD: Makefile,v 1.1 2007/07/02 18:35:14 pooka Exp $
#

NOMAN= should challenge wizd

.include <bsd.own.mk>

.PATH:	${NETBSDSRCDIR}/sbin/mount_portal

PROG=	mount_portal
SRCS=	puffs_portal.c activate.c conf.c pt_conf.c \
	pt_exec.c pt_file.c pt_tcp.c pt_filter.c

CPPFLAGS+= -I${NETBSDSRCDIR}/sys -I${NETBSDSRCDIR}/sbin/mount_portal

DPADD+=${LIBUTIL} ${LIBPUFFS}
LDADD+=-lutil -lpuffs

DBG=-g -O0

.include <bsd.prog.mk>
