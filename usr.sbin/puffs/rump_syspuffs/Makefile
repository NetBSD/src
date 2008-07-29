#	$NetBSD: Makefile,v 1.1 2008/07/29 13:17:48 pooka Exp $
#

PROG=		rump_syspuffs

LDADD+=		-lrumpfs_syspuffs
DPADD+=		${LIBRUMPFS_SYSPUFFS}

ISRUMP=		# indubitably
CPPFLAGS+=	-I${.CURDIR}/../../../sys/rump/fs/lib/libsyspuffs

.include <bsd.prog.mk>
