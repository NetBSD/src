#	$NetBSD: Makefile,v 1.6.6.1 2014/05/22 11:43:09 yamt Exp $
#

SMBROOT=${.CURDIR}/../../../external/bsd/smbfs

MOUNTNAME=	smbfs
SRCS+=		smb_rumpops.c

PROGDPLIBS+=	smb ${SMBROOT}/lib/libsmb
LDADD+= 	-lrumpnet_sockin -lrumpnet
LDADD+=		-lrumpkern_crypto -lrumpdev_netsmb -lrumpdev

ISRUMP=		# it is

.include "${SMBROOT}/Makefile.inc"

CPPFLAGS+=-I${SMBDIST}/mount_smbfs
.PATH: ${SMBDIST}/mount_smbfs

.include <bsd.prog.mk>
