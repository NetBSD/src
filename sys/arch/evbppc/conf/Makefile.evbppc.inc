#	$NetBSD: Makefile.evbppc.inc,v 1.4 2011/01/04 08:58:59 matt Exp $

CPPFLAGS += -D__${BOARDTYPE}__

RAMDISKDIR!=	cd ${NETBSDSRCDIR}/distrib/${THISPPC:T}/ramdisk && ${PRINTOBJDIR}

.if defined(NEEDS_MDSETIMAGE) && exists(${RAMDISKDIR}/ramdisk.fs)
SYSTEM_LD_TAIL_EXTRA+=; \
	echo ${TOOL_MDSETIMAGE} -s $@ ${RAMDISKDIR}/ramdisk.fs; \
	${TOOL_MDSETIMAGE} -s $@ ${RAMDISKDIR}/ramdisk.fs

.endif

.-include "${THISPPC}/conf/Makefile.${BOARDTYPE}.inc"
