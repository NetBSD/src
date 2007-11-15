#	$NetBSD: Makefile.inc,v 1.2.8.3 2007/11/15 11:45:23 yamt Exp $
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
