#	$NetBSD: Makefile,v 1.3 2008/03/12 05:11:51 lukem Exp $
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
