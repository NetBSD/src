#	$NetBSD: Makefile.inc,v 1.7 2000/05/17 21:40:45 matt Exp $
#
#	etc.vax/Makefile.inc -- vax-specific etc Makefile targets
#

# 
# We need this to build the sysinst kernel.
BUILD_KERNELS+=INSTALL

distribution: pcs750.bin

pcs750.bin: etc.vax/pcs750.bin.uu
	(cd etc.vax; uudecode pcs750.bin.uu)
	(cd etc.vax; install -o ${BINOWN} -g ${BINGRP} -m 444 ${PCS} \
	    ${DESTDIR}/)
	

