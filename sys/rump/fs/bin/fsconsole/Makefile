#	$NetBSD: Makefile,v 1.1 2007/08/16 19:48:31 pooka Exp $
#

PROG=		fsconsole
RUMPFS_NOFSLIB=	# defined

LDADD+=		${RUMPFSLD_ALL} -L${NETBSDSRCDIR}/sys/rump/fs/lib/libukfs -lukfs
DPADD+=		${RUMPFSDP_ALL}
CPPFLAGS+=      -I${NETBSDSRCDIR}/sys/rump/librump/rumpkern		\
		-I${NETBSDSRCDIR}/sys/rump/fs/lib/libukfs

RUMPFS_WANT=	${RUMPFSLIST} ufs

.include <bsd.prog.mk>
