#	$NetBSD: Makefile.inc,v 1.8 1999/02/05 03:01:53 cjs Exp $
#
#	etc.sun3/Makefile.inc -- sun3-specific etc Makefile targets
#

# Set this for any kernels beyond GENERIC you want to include
# in the distribution.
EXTRA_KERNELS+= GENERIC3X

BIN3+= etc.sun3/ttyaction
