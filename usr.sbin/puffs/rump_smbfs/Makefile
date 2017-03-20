#	$NetBSD: Makefile,v 1.7.8.1 2017/03/20 06:58:09 pgoyette Exp $
#

SMBROOT=${.CURDIR}/../../../external/bsd/smbfs

MOUNTNAME=	smbfs
SRCS+=		smb_rumpops.c

PROGDPLIBS+=	smb ${SMBROOT}/lib/libsmb
LDADD+= 	-lrumpnet_sockin -lrumpnet_net -lrumpnet
LDADD+=		-lrumpkern_crypto -lrumpdev_netsmb -lrumpdev

ISRUMP=		# it is

.include "${SMBROOT}/Makefile.inc"

CPPFLAGS+=-I${SMBDIST}/mount_smbfs
.PATH: ${SMBDIST}/mount_smbfs

.include <bsd.prog.mk>
