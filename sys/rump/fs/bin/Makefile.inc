#	$NetBSD: Makefile.inc,v 1.4 2007/10/31 17:53:38 pooka Exp $
#

NOMAN=		1
WARNS?=		4

CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/rump/fs/lib/libp2k -I${NETBSDSRCDIR}/sys
.ifndef RUMPFS_NOFSLIB
LDADD+=		${RUMPFSLD_P2K} ${RUMPFSLD_UKFS} ${LIBUTIL} ${LIBPUFFS}
.endif
LDADD+=		-L${LIBRUMPDIR} -Wl,--whole-archive -lrump \
		-Wl,--no-whole-archive -lpthread

RUMPKERNEL=		no
INSTALL_FILE=		: not today

.include "${.CURDIR}/../../Makefile.rumpfs"
.include "${NETBSDSRCDIR}/sys/rump/Makefile.rump"
