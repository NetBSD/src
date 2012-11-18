#	$NetBSD: Makefile,v 1.2.2.2 2012/11/18 19:05:23 riz Exp $

DEV=		xx
DEVDRV=		xx
FS=		cd9660
BOOTCPPFLAGS=	-DCDBOOT -DFDBOOT -DSDBOOT

TOUCHPROG=								\
	( ( echo -n X68SCSI1 | dd bs=2048 conv=sync ) ;			\
	  ( cat ${PROG} | dd bs=2048 conv=sync ) ) > ${PROG}.n ;	\
	mv ${PROG}.n ${PROG}

.include "../Makefile.xxboot"
