#	$NetBSD: Makefile.inc,v 1.4.2.2 2007/11/02 12:47:07 joerg Exp $
#

NOMAN=		1
WARNS?=		4

CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/rump/fs/lib/libp2k -I${NETBSDSRCDIR}/sys
.ifndef RUMPFS_NOFSLIB
LDADD+=		${RUMPFSLD_P2K} ${RUMPFSLD_UKFS} ${LIBUTIL} ${LIBPUFFS}
.endif
LDADD+=		-L${LIBRUMPDIR} -lrump

RUMPKERNEL=		no
INSTALL_FILE=		: not today

.include "${.CURDIR}/../../Makefile.rumpfs"
.include "${NETBSDSRCDIR}/sys/rump/Makefile.rump"
