# $Id: Makefile.inc,v 1.1 1994/01/28 16:39:01 pk Exp $

SRCS+=		sparc-pinsn.c sparc-opc.c

CFLAGS+=	-DSELECT_ARCHITECTURES=bfd_arch_sparc \
		-DPRINT_INSN=print_insn_sparc


