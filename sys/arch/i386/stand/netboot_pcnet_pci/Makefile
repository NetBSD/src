#	$NetBSD: Makefile,v 1.1 2002/02/17 20:03:17 thorpej Exp $

USE_NETIF=	pcnet_pci

CPPFLAGS+= -DPCIROM -DPCI_VID=0x1022 -DPCI_DID=0x2000 -DPCI_CLASS=0x020000

.include "../netboot/Makefile"
