#	$NetBSD: Makefile,v 1.1.4.2 2002/02/28 04:10:29 nathanw Exp $

USE_NETIF=	pcnet_pci

CPPFLAGS+= -DPCIROM -DPCI_VID=0x1022 -DPCI_DID=0x2000 -DPCI_CLASS=0x020000

.include "../Makefile.netboot"
