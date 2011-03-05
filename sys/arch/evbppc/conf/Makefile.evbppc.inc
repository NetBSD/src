#	$NetBSD: Makefile.evbppc.inc,v 1.3.128.1 2011/03/05 20:50:15 rmind Exp $

CPPFLAGS += -D__${BOARDTYPE}__

RAMDISKDIR!=	cd ${NETBSDSRCDIR}/distrib/${THISPPC:T}/ramdisk && ${PRINTOBJDIR}

.if defined(NEEDS_MDSETIMAGE) && exists(${RAMDISKDIR}/ramdisk.fs)
SYSTEM_LD_TAIL_EXTRA+=; \
	echo ${TOOL_MDSETIMAGE} -s $@ ${RAMDISKDIR}/ramdisk.fs; \
	${TOOL_MDSETIMAGE} -s $@ ${RAMDISKDIR}/ramdisk.fs

.endif

.-include "${THISPPC}/conf/Makefile.${BOARDTYPE}.inc"
