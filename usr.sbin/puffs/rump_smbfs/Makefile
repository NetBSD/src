#	$NetBSD: Makefile,v 1.5 2009/12/11 16:46:27 pooka Exp $
#

WARNS=2

.include <bsd.own.mk>

MOUNTNAME=	smbfs
SRCS+=		smb_rumpops.c

LDADD+= 	-lrumpnet_sockin -lrumpnet
LDADD+=		-lrumpcrypto -lrumpdev_netsmb -lrumpdev

ISRUMP=		# it is

.include "${.CURDIR}/../../../sbin/mount_smbfs/Makefile.inc"

.include <bsd.prog.mk>
