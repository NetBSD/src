#	$NetBSD: des.inc,v 1.1 2002/09/20 23:59:40 thorpej Exp $

# XXX WE NEED A WAY TO DETERMINE THESE AT COMPILE-TIME BASED ON
# XXX COMPILER OPTIONS.

# These are good for StrongARM with -mcpu=strongarm
#CPPFLAGS+=	-DDES_RISC1

# These are good for XScale with -mcpu=xscale
#CPPFLAGS+=	-DDES_RISC1 -DDES_PTR

.include "${.CURDIR}/des.inc"
