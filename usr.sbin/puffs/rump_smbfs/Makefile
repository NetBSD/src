#	$NetBSD: Makefile,v 1.4 2009/11/05 13:26:02 pooka Exp $
#

WARNS=2

.include <bsd.own.mk>

MOUNTNAME=	smbfs
SRCS+=		smb_rumpops.c

LDADD+= 	-lrumpnet_netinet -lrumpnet_sockin -lrumpnet
LDADD+=		-lrumpcrypto -lrumpdev_netsmb -lrumpdev

ISRUMP=		# it is

.include "${.CURDIR}/../../../sbin/mount_smbfs/Makefile.inc"

.include <bsd.prog.mk>
