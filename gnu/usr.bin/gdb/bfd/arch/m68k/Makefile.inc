# $Id: Makefile.inc,v 1.4 1995/04/16 03:09:24 gwr Exp $

SRCS+=		netbsdm68k.c netbsdm68k4k.c cpu-m68k.c trad-core.c

# Note: DEFAULT_VECTOR is actually ignored when SELECT_VECS is set.
VECTORS=	-DDEFAULT_VECTOR=netbsd_m68k_vec \
		-DSELECT_ARCHITECTURES=bfd_m68k_arch \
		-DSELECT_VECS=' &netbsd_m68k_vec, &netbsd_m68k4k_vec, \
				&netbsd_core_vec, &trad_core_vec '

