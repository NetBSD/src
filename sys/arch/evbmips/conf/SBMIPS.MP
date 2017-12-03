# $NetBSD: SBMIPS.MP,v 1.2.6.2 2017/12/03 11:36:09 jdolecek Exp $

include "arch/evbmips/conf/SBMIPS"

options 	MULTIPROCESSOR
options 	LOCKDEBUG

pseudo-device	lockstat

no makeoptions	COPY_SYMTAB
makeoptions	COPY_SYMTAB=1

