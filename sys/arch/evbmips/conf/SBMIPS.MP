# $NetBSD: SBMIPS.MP,v 1.2 2017/07/24 09:56:45 mrg Exp $

include "arch/evbmips/conf/SBMIPS"

options 	MULTIPROCESSOR
options 	LOCKDEBUG

pseudo-device	lockstat

no makeoptions	COPY_SYMTAB
makeoptions	COPY_SYMTAB=1

