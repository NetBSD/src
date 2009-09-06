#	$NetBSD: Makefile,v 1.1 2009/09/06 20:26:47 pooka Exp $
#

.include <bsd.own.mk>

MOUNTNAME=	smbfs
SRCS+=		smb_rumpops.c

LDADD+= 	-lrumpnet_netinet -lrumpnet_sockin -lrumpnet
LDADD+=		-lrumpcrypto -lrumpdev_netsmb -lrumpdev

ISRUMP=		# it is

.include "${.CURDIR}/../../../sbin/mount_smbfs/Makefile.inc"

.include <bsd.prog.mk>
