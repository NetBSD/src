#	$NetBSD: Makefile,v 1.2 2012/11/17 19:10:46 tsutsui Exp $

DEV=		xx
DEVDRV=		xx
FS=		cd9660
BOOTCPPFLAGS=	-DCDBOOT

TOUCHPROG=								\
	( ( echo -n X68SCSI1 | dd bs=2048 conv=sync ) ;			\
	  ( cat ${PROG} | dd bs=2048 conv=sync ) ) > ${PROG}.n ;	\
	mv ${PROG}.n ${PROG}

.include "../Makefile.xxboot"
