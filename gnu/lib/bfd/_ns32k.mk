# $NetBSD: _ns32k.mk,v 1.1.1.1 1997/09/26 02:38:49 gwr Exp $

BFD_BACKENDS =	ns32knetbsd.c aout-ns32k.c
BFD_MACHINES =	cpu-ns32k.c

# Note: DEFAULT_VECTOR is actually ignored when SELECT_VECS is set.
TDEFAULTS = \
 -DSELECT_ARCHITECTURES='&bfd_ns32k_arch' \
 -DDEFAULT_VECTOR=ns32knetbsd_vec \
 -DSELECT_VECS=' &ns32knetbsd_vec ' \
 -DHAVE_ns32knetbsd_vec \
 -DNETBSD_CORE

OPCODE_MACHINES =  ns32k-dis.c
