# $NetBSD: Makefile.inc,v 1.1 2000/05/18 02:12:43 matt Exp $

PRELABEL=${DESTDIR}/sbin/disklabel -w -r ${VND_RDEV} miniroot
# To install the boot block...
BOOTINSTALL=${DESTDIR}/sbin/disklabel -B -b ${DESTDIR}/usr/mdec/xxboot ${VND_RDEV}	
