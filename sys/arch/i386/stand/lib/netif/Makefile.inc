##	$NetBSD: Makefile.inc,v 1.3 1998/12/12 15:47:05 drochner Exp $

SRCS += netif_small.c
.if (${USE_NETIF} == "3c509")
SRCS += 3c509.c elink3.c
.endif
.if (${USE_NETIF} == "3c590")
SRCS += 3c590.c elink3.c
.endif
.if (${USE_NETIF} == "wd80x3")
SRCS += wd80x3.c dp8390.c
.endif
.if (${USE_NETIF} == "pcnet_pci")
SRCS += pcnet_pci.c am7990.c
.endif
.if (${USE_NETIF} == "pcnet_isapnp")
SRCS += pcnet_isapnp.c am7990.c
.endif
.if (${USE_NETIF} == "i82557")
SRCS += i82557.c
.endif
