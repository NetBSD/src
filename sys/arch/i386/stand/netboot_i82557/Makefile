#	$NetBSD: Makefile,v 1.1 2002/02/17 20:03:15 thorpej Exp $

USE_NETIF=	i82557

CPPFLAGS+= -DPCIROM -DPCI_VID=0x8086 -DPCI_DID=0x1229 -DPCI_CLASS=0x020000

.include "../netboot/Makefile"
