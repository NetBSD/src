# $Id: Makefile.inc,v 1.1 1994/01/28 16:38:51 pk Exp $

SRCS+=		i386-pinsn.c

CFLAGS+=	-DSELECT_ARCHITECTURES=bfd_arch_i386 \
		-DPRINT_INSN=print_insn_i386


