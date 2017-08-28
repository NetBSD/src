# $NetBSD: SBMIPS64.MP,v 1.2.2.2 2017/08/28 17:51:36 skrll Exp $

include "arch/evbmips/conf/SBMIPS64"

options 	MULTIPROCESSOR
options 	LOCKDEBUG

pseudo-device	lockstat

no makeoptions	COPY_SYMTAB
makeoptions	COPY_SYMTAB=1

