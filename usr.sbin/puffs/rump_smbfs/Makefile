#	$NetBSD: Makefile,v 1.6 2010/12/05 20:13:26 pooka Exp $
#

WARNS=2

.include <bsd.own.mk>

MOUNTNAME=	smbfs
SRCS+=		smb_rumpops.c

LDADD+= 	-lrumpnet_sockin -lrumpnet
LDADD+=		-lrumpkern_crypto -lrumpdev_netsmb -lrumpdev

ISRUMP=		# it is

.include "${.CURDIR}/../../../sbin/mount_smbfs/Makefile.inc"

.include <bsd.prog.mk>
