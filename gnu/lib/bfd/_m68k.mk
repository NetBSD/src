#	$NetBSD: _m68k.mk,v 1.5 1998/08/23 20:57:45 frueauf Exp $

BFD_ARCH_SRCS=	cpu-m68k.c \
		m68knetbsd.c m68k4knetbsd.c sunos.c aout32.c \
		elf32-m68k.c elf32.c elf.c elflink.c \
		m68k-dis.c m68k-opc.c

BFD_ARCH_DEFS=	-DARCH_m68k \
		-DSELECT_ARCHITECTURES='&bfd_m68k_arch' \
		-DDEFAULT_VECTOR=m68knetbsd_vec \
		-DSELECT_VECS='&m68knetbsd_vec, &m68k4knetbsd_vec, &bfd_elf32_m68k_vec, &sunos_big_vec' \
		-DHAVE_m68knetbsd_vec \
		-DHAVE_m68k4knetbsd_vec \
		-DHAVE_bfd_elf32_m68k_vec \
		-DHAVE_sunos_big_vec \
		-DNETBSD_CORE
