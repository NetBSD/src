# $NetBSD: _mips.mk,v 1.2 1997/10/17 18:50:45 gwr Exp $

BFD_MACHINES =	cpu-mips.c

BFD_BACKENDS =	elf32-mips.o elf32.o elf.o elflink.o ecofflink.o \
		elf64-mips.o elf64.o coff-mips.o ecoff.o

TDEFAULTS = \
 -DSELECT_ARCHITECTURES='&bfd_mips_arch' \
 -DDEFAULT_VECTOR=bfd_elf32_littlemips_vec \
 -DSELECT_VECS=' &bfd_elf32_littlemips_vec, &bfd_elf32_bigmips_vec, &bfd_elf64_bigmips_vec, &bfd_elf64_littlemips_vec, &ecoff_little_vec, &ecoff_big_vec ' \
 -DHAVE_bfd_elf32_littlemips_vec \
 -DHAVE_bfd_elf32_bigmips_vec \
 -DHAVE_bfd_elf64_bigmips_vec \
 -DHAVE_bfd_elf64_littlemips_vec \
 -DHAVE_ecoff_little_vec \
 -DHAVE_ecoff_big_vec \
 -DNETBSD_CORE

OPCODE_MACHINES =  mips-dis.c mips-opc.c mips16-opc.c
