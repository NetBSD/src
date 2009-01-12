#	$NetBSD: Makefile,v 1.4 2009/01/12 20:43:29 pooka Exp $
#

NOMAN= should challenge wizd

.include <bsd.own.mk>

.PATH:	${NETBSDSRCDIR}/sbin/mount_portal

PROG=	mount_puffsportal
SRCS=	puffs_portal.c activate.c conf.c pt_conf.c \
	pt_exec.c pt_file.c pt_tcp.c pt_filter.c

CPPFLAGS+= -I${NETBSDSRCDIR}/sbin/mount_portal

DPADD+=${LIBUTIL} ${LIBPUFFS}
LDADD+=-lutil -lpuffs

.include <bsd.prog.mk>
