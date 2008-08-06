#	$NetBSD: Makefile,v 1.2 2008/08/06 20:14:09 pooka Exp $
#

PROG=		rump_syspuffs

LDADD+=		-lrumpfs_syspuffs
DPADD+=		${LIBRUMPFS_SYSPUFFS}

ISRUMP=		# indubitably
CPPFLAGS+=	-I${.CURDIR}/../../../sys/rump/fs/lib/libsyspuffs
NOMAN=

.include <bsd.prog.mk>
