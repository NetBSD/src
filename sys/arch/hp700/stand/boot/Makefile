#	$NetBSD: Makefile,v 1.1.2.2 2002/06/23 17:36:30 jdolecek Exp $

#	$OpenBSD: Makefile,v 1.10 2000/05/31 19:56:51 mickey Exp $

PROGAOUT=       boot
PROGSOURCE=     srt0.S boot.c conf.c tgets.c
NEWVERSWHAT=    "LIF Boot"

CLEANFILES+=	boot.gdb boot.map boot.lif bsd

#all:	boot.lif

# enable this when SHEPHERD dies
#.if exists(${.CURDIR}/../../compile/DISKLESS/bsd)
#bsd: ${.CURDIR}/../../compile/DISKLESS/bsd
#	gzip -9 -c ${.CURDIR}/../../compile/DISKLESS/bsd > bsd
#ADDBOOT+=bsd
#.endif

.if exists(${.CURDIR}/../../compile/SHEPHERD_PIE/bsd)
bsd: ${.CURDIR}/../../compile/SHEPHERD_PIE/bsd
	gzip -9 -c ${.CURDIR}/../../compile/SHEPHERD_PIE/bsd > bsd
ADDBOOT+=bsd
.endif

.if 0
# probably we should check for 256k limit for ISL
boot.lif:	${PROG} ${ADDBOOT}
	-@cp ${PROG} ${PROG}.gdb
	${STRIP} ${PROG}
	${MKBOOT} -v ${PROG} ${ADDBOOT} boot.lif
.endif

.include "../Makefile.buildboot"
