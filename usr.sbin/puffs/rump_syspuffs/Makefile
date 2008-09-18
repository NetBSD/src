#	$NetBSD: Makefile,v 1.3.2.2 2008/09/18 04:30:13 wrstuden Exp $
#

PROG=		rump_syspuffs

LDADD+=		-lrumpfs_syspuffs
DPADD+=		${LIBRUMPFS_SYSPUFFS}

ISRUMP=		# indubitably
CPPFLAGS+=	-I${.CURDIR}/../../../sys/rump/fs/lib/libsyspuffs
MAN=		rump_syspuffs.8

.include <bsd.prog.mk>
