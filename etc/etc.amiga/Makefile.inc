#	$NetBSD: Makefile.inc,v 1.13 2000/08/29 21:14:31 is Exp $
#
#	etc.amiga/Makefile.inc -- amiga-specific etc Makefile targets
#

# Set this for any kernels beyond GENERIC you want to include
# in the distribution.
EXTRA_KERNELS+=INSTALL INSTWDCPCM

snap_md_post:
	(cd $(SRCTOP)/distrib/miniroot; ${MAKE} release)
