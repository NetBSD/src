#	$NetBSD: _mipsel.mk,v 1.3 1998/08/23 04:42:33 simonb Exp $

BFD_ARCH_SRCS=	cpu-mips.c \
		elf32-mips.c elf32.c elf.c elflink.c ecofflink.c \
		coff-mips.c ecoff.c \
		mips-dis.c mips-opc.c mips16-opc.c

BFD_ARCH_DEFS=	-DARCH_mips \
		-DSELECT_ARCHITECTURES='&bfd_mips_arch' \
		-DDEFAULT_VECTOR=bfd_elf32_littlemips_vec \
		-DSELECT_VECS='&bfd_elf32_littlemips_vec, &bfd_elf32_bigmips_vec, &ecoff_little_vec, &ecoff_big_vec' \
		-DHAVE_bfd_elf32_littlemips_vec \
		-DHAVE_bfd_elf32_bigmips_vec \
		-DHAVE_ecoff_little_vec \
		-DHAVE_ecoff_big_vec \
		-DNETBSD_CORE

# XXX cannot support 64-bit BFD targets with gdb 4.16.
# They assume that  BFD_ARCH_SIZE is 64, but that causes bfd_vma_addr 
# to be a 64-bit int. GDB uses bfd_vma_addr for CORE_ADDR, but also
# casts CORE_ADDRS to ints, which loses on 32-bit mips hosts.
