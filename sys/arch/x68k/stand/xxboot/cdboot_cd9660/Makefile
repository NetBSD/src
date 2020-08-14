#	$NetBSD: Makefile,v 1.3 2020/08/14 03:40:47 isaki Exp $

FS=		cd9660
BOOTCPPFLAGS=	-DCDBOOT

TOUCHPROG=								\
	( ( echo -n X68SCSI1 | dd bs=2048 conv=sync ) ;			\
	  ( cat ${PROG} | dd bs=2048 conv=sync ) ) > ${PROG}.n ;	\
	mv ${PROG}.n ${PROG}

.include "../Makefile.xxboot"
