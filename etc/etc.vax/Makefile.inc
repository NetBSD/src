#	$NetBSD: Makefile.inc,v 1.6 1999/07/25 04:23:49 abs Exp $
#
#	etc.vax/Makefile.inc -- vax-specific etc Makefile targets
#

# Set this for any kernels beyond GENERIC you want to include
# in the distribution.
#EXTRA_KERNELS+=

distribution: pcs750.bin

pcs750.bin:
	(cd etc.vax;uudecode pcs750.bin.uu)
	(cd etc.vax; install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${PCS} \
	    ${DESTDIR}/)

