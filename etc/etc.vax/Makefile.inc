#	$NetBSD: Makefile.inc,v 1.2 1996/10/26 08:12:12 ragge Exp $
#
#	etc.vax/Makefile.inc -- vax-specific etc Makefile targets
#

.ifdef DESTDIR
distribution: pcs750.bin

pcs750.bin:
	(cd etc.vax;uudecode pcs750.bin.uu)
	(cd etc.vax; install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${PCS} \
            ${DESTDIR}/)

.endif
