#	$NetBSD: Makefile,v 1.1 2002/02/19 19:49:39 thorpej Exp $

# This can also handle the 3c900 TPO / combo and 3c905 TX

USE_NETIF=	3c590

CPPFLAGS+= -DPCIROM -DPCI_VID=0x10b7 -DPCI_CLASS=0x020000
CPPFLAGS+= -DPCI_DID=0x5900 # 3c590
#CPPFLAGS+= -DPCI_DID=0x9000 # 3c900 TPO
#CPPFLAGS+= -DPCI_DID=0x9001 # 3c900 combo
#CPPFLAGS+= -DPCI_DID=0x9050 # 3c905 TX

.include "../Makefile.netboot"
