#	$NetBSD: Makefile,v 1.36.6.1 2006/04/22 11:38:06 simonb Exp $

S=	${.CURDIR}/../../../..

.include <bsd.own.mk>

PROG=	boot
DEVS=	hp.c ctu.c ra.c mfm.c if_qe.c if_le.c if_ze.c if_de.c if_ni.c
SRCS=	srt0.S boot.c devopen.c conf.c autoconf.c netio.c rom.c romread.S \
	consio.c consio2.S str.S ${DEVS} findcpu.c vers.c
LIBC=

CLEANFILES+=${PROG} ${PROG}.sym vers.c
CPPFLAGS+=-DSUPPORT_BOOTPARAMS -DSUPPORT_BOOTP -DSUPPORT_DHCP -D_STANDALONE \
	-DNO_MID_CHECK
#CPPFLAGS+=-DBOOTP_DEBUG -DNETIF_DEBUG -DETHER_DEBUG -DNFS_DEBUG -DDEV_DEBUG \
#	-DRPC_DEBUG -DRARP_DEBUG -DPARANOID
BINDIR=	
MDEC_DIR=/usr/mdec

SA_AS=	library
SAMISCMAKEFLAGS=SA_USE_CREAD=yes SA_INCLUDE_NET=yes SA_USE_LOADFILE=yes
.include "${S}/lib/libsa/Makefile.inc"
LIBSA=	${SALIB}

Z_AS=	library
.include "${S}/lib/libz/Makefile.inc"
LIBZ=	${ZLIB}

KERN_AS=library
.include "${S}/lib/libkern/Makefile.inc"
LIBKERN=${KERNLIB}

.PHONY: vers.c
vers.c: ${.CURDIR}/version
	${HOST_SH} ${S}/conf/newvers_stand.sh -NM ${.CURDIR}/version "${MACHINE}"

${PROG}: machine-links ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	${_MKTARGET_LINK}
	${LD} -N -Ttext ${RELOC} -e nisse -o ${PROG}.sym ${OBJS} \
	    ${LIBSA} ${LIBZ} ${LIBSA} ${LIBKERN}
	${STRIP} -o ${PROG} ${PROG}.sym

install:
	${INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m 555 \
		${PROG} ${DESTDIR}${MDEC_DIR}/${PROG}

.include <bsd.prog.mk>
