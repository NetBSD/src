#	$NetBSD: Makefile,v 1.2 2007/08/19 20:06:58 pooka Exp $
#

PROG=		fsconsole
RUMPFS_NOFSLIB=	# defined

LDADD+=		${RUMPFSLD_ALL}
DPADD+=		${RUMPFSDP_ALL}
CPPFLAGS+=      -I${NETBSDSRCDIR}/sys/rump/librump/rumpkern		\
		-I${NETBSDSRCDIR}/sys/rump/fs/lib/libukfs

RUMPFS_WANT=	${RUMPFSLIST} ufs ukfs

.include <bsd.prog.mk>
