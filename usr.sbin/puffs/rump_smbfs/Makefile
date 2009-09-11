#	$NetBSD: Makefile,v 1.2 2009/09/11 06:48:50 skrll Exp $
#

.include <bsd.own.mk>

MOUNTNAME=	smbfs
SRCS+=		smb_rumpops.c

LDADD+= 	-lrumpnet_netinet -lrumpnet_sockin -lrumpnet
LDADD+=		-lrumpcrypto -lrumpdev_netsmb -lrumpdev

ISRUMP=		# it is

LDFLAGS+=      -Wl,--ignore-unresolved-symbol,__start_link_set_domains
LDFLAGS+=      -Wl,--ignore-unresolved-symbol,__stop_link_set_domains

.include "${.CURDIR}/../../../sbin/mount_smbfs/Makefile.inc"

.include <bsd.prog.mk>
