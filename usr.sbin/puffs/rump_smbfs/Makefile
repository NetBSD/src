#	$NetBSD: Makefile,v 1.3 2009/09/14 17:24:21 skrll Exp $
#

.include <bsd.own.mk>

MOUNTNAME=	smbfs
SRCS+=		smb_rumpops.c

LDADD+= 	-lrumpnet_netinet -lrumpnet_sockin -lrumpnet
LDADD+=		-lrumpcrypto -lrumpdev_netsmb -lrumpdev

ISRUMP=		# it is

.include "${.CURDIR}/../../../sbin/mount_smbfs/Makefile.inc"

.include <bsd.prog.mk>
