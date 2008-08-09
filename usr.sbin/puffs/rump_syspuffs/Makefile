#	$NetBSD: Makefile,v 1.3 2008/08/09 11:07:26 pooka Exp $
#

PROG=		rump_syspuffs

LDADD+=		-lrumpfs_syspuffs
DPADD+=		${LIBRUMPFS_SYSPUFFS}

ISRUMP=		# indubitably
CPPFLAGS+=	-I${.CURDIR}/../../../sys/rump/fs/lib/libsyspuffs
MAN=		rump_syspuffs.8

.include <bsd.prog.mk>
