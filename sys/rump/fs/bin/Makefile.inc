#	$NetBSD: Makefile.inc,v 1.1.2.2 2007/08/05 22:28:02 pooka Exp $
#

NOMAN=		1
WARNS?=		4

CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/rump/fs/lib/libp2k -I${NETBSDSRCDIR}/sys
LDADD+=		${RUMPFSLD_P2K} -L${LIBRUMPDIR} -lrump ${LIBUTIL} ${LIBPUFFS}

RUMPKERNEL=		no
INSTALL_FILE=		: not today

.include "${.CURDIR}/../../Makefile.rumpfs"
.include "${NETBSDSRCDIR}/sys/rump/Makefile.rump"
