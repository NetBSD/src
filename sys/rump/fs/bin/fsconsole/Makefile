#	$NetBSD: Makefile,v 1.2.6.2 2007/09/03 10:23:52 skrll Exp $
#

PROG=		fsconsole
RUMPFS_NOFSLIB=	# defined

LDADD+=		${RUMPFSLD_ALL}
DPADD+=		${RUMPFSDP_ALL}
CPPFLAGS+=      -I${NETBSDSRCDIR}/sys/rump/librump/rumpkern		\
		-I${NETBSDSRCDIR}/sys/rump/fs/lib/libukfs

RUMPFS_WANT=	${RUMPFSLIST} ufs ukfs

.include <bsd.prog.mk>
