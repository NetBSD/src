#	$NetBSD: Makefile.evbppc.inc,v 1.8 2011/06/25 02:26:53 matt Exp $

CPPFLAGS += -D__${BOARDTYPE}__

RAMDISKDIR!=	cd ${NETBSDSRCDIR}/distrib/${THISPPC:T}/ramdisk && ${PRINTOBJDIR}

.-include "${THISPPC}/conf/Makefile.${BOARDTYPE}.inc"
