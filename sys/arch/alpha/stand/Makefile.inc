#	$NetBSD: Makefile.inc,v 1.6 1997/02/16 21:27:06 cgd Exp $

.ifndef __INCLUDED_STAND_MAKEFILE_INC
__INCLUDED_STAND_MAKEFILE_INC=

.include <bsd.own.mk>			# for ELF_TOOLCHAIN definition

BINDIR=			/usr/mdec

# For descriptions of regions available to bootstrap programs, see
# section 3.4.1.2 (pp. III 3-14 - III 3-18) of the second edition of
# the Alpha AXP Architecture Reference Manual.

PRIMARY_LOAD_ADDRESS=	20000000	# "Region 1 start"
SECONDARY_LOAD_ADDRESS=	20004000	# "Region 1 start" + 32k
HEAP_LIMIT=		20040000	# "Region 1 start" + 256k

CPPFLAGS+=		-DPRIMARY_LOAD_ADDRESS="0x${PRIMARY_LOAD_ADDRESS}"
CPPFLAGS+=		-DSECONDARY_LOAD_ADDRESS="0x${SECONDARY_LOAD_ADDRESS}"

SAMISCCPPFLAGS+=	-DHEAP_LIMIT="0x${HEAP_LIMIT}"

.if !defined(ELF_TOOLCHAIN)
CPPFLAGS+=	-DECOFF_COMPAT
.endif

.endif
