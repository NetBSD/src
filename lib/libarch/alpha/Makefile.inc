#	$NetBSD: Makefile.inc,v 1.3 2000/02/26 19:00:08 thorpej Exp $

.if (${MACHINE_ARCH} == "alpha")
SRCS+=	alpha_bus_window.c alpha_pci_conf.c alpha_pci_io.c alpha_pci_mem.c
.endif
