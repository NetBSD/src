#	$NetBSD: Makefile,v 1.2.8.2 2007/09/03 14:44:39 yamt Exp $
#

PROG=		fsconsole
RUMPFS_NOFSLIB=	# defined

LDADD+=		${RUMPFSLD_ALL}
DPADD+=		${RUMPFSDP_ALL}
CPPFLAGS+=      -I${NETBSDSRCDIR}/sys/rump/librump/rumpkern		\
		-I${NETBSDSRCDIR}/sys/rump/fs/lib/libukfs

RUMPFS_WANT=	${RUMPFSLIST} ufs ukfs

.include <bsd.prog.mk>
