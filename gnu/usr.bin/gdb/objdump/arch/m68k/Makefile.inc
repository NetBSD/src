# $Id: Makefile.inc,v 1.1 1994/01/28 16:38:57 pk Exp $

SRCS+=		m68k-pinsn.c

CFLAGS+=	-DSELECT_ARCHITECTURES=bfd_arch_m68k \
		-DPRINT_INSN=print_insn_m68k


