#	$NetBSD: Makefile,v 1.7 2000/05/21 20:17:44 ragge Exp $

S=	${.CURDIR}/../../../../

PROG=	boot
NOMAN=	1
WARNS?=	1
DEVS=	hp.c ctu.c ra.c mfm.c if_qe.c if_le.c if_ze.c if_de.c
SRCS=	srt0.s boot.c devopen.c conf.c autoconf.c netio.c rom.c romread.s \
	urem.s udiv.s consio.c str.s ${DEVS} findcpu.c

CLEANFILES+=${PROG} ${PROG}.mop
CPPFLAGS+=-DSUPPORT_BOOTPARAMS -DSUPPORT_BOOTP -DSUPPORT_DHCP -D_STANDALONE
#CPPFLAGS+=-DBOOTP_DEBUG -DNETIF_DEBUG -DETHER_DEBUG -DNFS_DEBUG -DDEV_DEBUG \
#	-DRPC_DEBUG -DRARP_DEBUG -DPARANOID
BINDIR=	/
MDEC_DIR=/usr/mdec

SA_AS=	library
SAMISCMAKEFLAGS=SA_USE_CREAD=yes SA_INCLUDE_NET=yes
.include "${S}/lib/libsa/Makefile.inc"
LIBSA=	${SALIB}

Z_AS=	library
.include "${S}/lib/libz/Makefile.inc"
LIBZ=	${ZLIB}

KERN_AS=library
.include "${S}/lib/libkern/Makefile.inc"
LIBKERN=${KERNLIB}

${PROG}: ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	ld -N -Ttext ${RELOC} -e nisse -o ${PROG} ${OBJS} \
	    ${LIBSA} ${LIBZ} ${LIBSA} ${LIBKERN}
	/usr/sbin/mopa.out ${PROG} ${PROG}.mop
	strip ${PROG}
	size ${PROG}

install:
	${INSTALL} -c -m 555 -o ${BINOWN} -g ${BINGRP} ${PROG} ${DESTDIR}${BINDIR}
	${INSTALL} -c -m 555 -o ${BINOWN} -g ${BINGRP} ${PROG} ${DESTDIR}${MDEC_DIR}
	${INSTALL} -c -m 555 -o ${BINOWN} -g ${BINGRP} ${PROG}.mop ${DESTDIR}${MDEC_DIR}

.include <bsd.prog.mk>
