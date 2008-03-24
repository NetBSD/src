#	$NetBSD: Makefile,v 1.2.26.1 2008/03/24 07:16:27 keiichi Exp $
#

PROG=		fsconsole
RUMPFS_NOFSLIB=	# defined

LDADD+=		${RUMPFSLD_ALL}
DPADD+=		${RUMPFSDP_ALL}
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/rump/librump/rumpkern		\
		-I${LIBRUMPDIR}						\
		-I${NETBSDSRCDIR}/sys/rump/fs/lib/libukfs

RUMPFS_WANT=	${RUMPFSLIST} ufs ukfs

.include <bsd.prog.mk>
