#	$NetBSD: Makefile.inc,v 1.12 2000/03/06 21:54:05 is Exp $
#
#	etc.amiga/Makefile.inc -- amiga-specific etc Makefile targets
#

# Set this for any kernels beyond GENERIC you want to include
# in the distribution.
#EXTRA_KERNELS+=

snap_md_post:
	(cd $(SRCTOP)/distrib/miniroot; ${MAKE} release)
