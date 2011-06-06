# $NetBSD: MALTA32.MP,v 1.1.4.1 2011/06/06 09:05:29 jruoho Exp $
#
include "arch/evbmips/conf/MALTA32"

options 	MULTIPROCESSOR
options 	LOCKDEBUG

pseudo-device	lockstat
