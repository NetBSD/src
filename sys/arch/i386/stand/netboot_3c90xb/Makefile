#	$NetBSD: Makefile,v 1.1 2002/02/17 20:03:14 thorpej Exp $

USE_NETIF=	3c90xb

#CPPFLAGS+= -DPCIROM -DPCI_VID=0x10b7 -DPCI_CLASS=0x020000
#CPPFLAGS+= -DPCI_DID=0x9005 # 3c900b combo
CPPFLAGS+= -DPCI_DID=0x9055 # 3c905b TX 
#CPPFLAGS+= -DPCI_DID=0x9058 # 3c905b combo

.include "../netboot/Makefile"
