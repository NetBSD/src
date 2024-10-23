#	$NetBSD: Makefile,v 1.48 2024/10/23 04:26:04 kalvisd Exp $

S=	${.CURDIR}/../../../..

.include <bsd.own.mk>

PROG=	boot
PROGS=  ${PROG} ${PROG}.mop
DEVS=	hp.c ctu.c ra.c mfm.c if_qe.c if_le.c if_ze.c if_de.c if_ni.c
SRCS=	srt0.S boot.c devopen.c conf.c autoconf.c netio.c rom.c romread.S \
	consio.c consio2.S str.S ${DEVS} findcpu.c

# XXX SHOULD NOT NEED TO DEFINE THESE!
LIBC=
LIBCRT0=
LIBCRTI=
LIBCRTBEGIN=
LIBCRTEND=

CLEANFILES+=${PROG} ${PROG}.sym ${PROG}.mop
CPPFLAGS+=-DSUPPORT_BOOTPARAMS -DSUPPORT_BOOTP -DSUPPORT_DHCP -D_STANDALONE \
	-DNO_MID_CHECK
CPPFLAGS+=-DLIBSA_CREAD_NOCRC
#CPPFLAGS+=-DBOOTP_DEBUG -DNETIF_DEBUG -DETHER_DEBUG -DNFS_DEBUG -DDEV_DEBUG \
#	-DRPC_DEBUG -DRARP_DEBUG -DPARANOID
BINDIR=	
MDEC_DIR=/usr/mdec
MOPCOPY=${TOOL_VAXMOPCOPY}

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

${PROG}: ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	${_MKTARGET_LINK}
	${LD} -N -Ttext ${RELOC} -e nisse -o ${PROG}.sym ${OBJS} \
	    ${LIBSA} ${LIBZ} ${LIBSA} ${LIBKERN}
	${STRIP} -o ${PROG} ${PROG}.sym

${PROG}.mop: ${PROG}
	${MOPCOPY} ${PROG} ${PROG}.mop


proginstall-${PROG}:
	${INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m 555 \
		${PROG} ${DESTDIR}${MDEC_DIR}/${PROG}
	${INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m 555 \
		${PROG} ${DESTDIR}${BINDIR}/${PROG}

proginstall-${PROG}.mop:
	${INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m 555 \
		${PROG}.mop ${DESTDIR}${MDEC_DIR}/${PROG}.mop

VERSIONFLAGS+=-n
.include "${S}/conf/newvers_stand.mk"

COPTS.autoconf.c+=	${CC_WNO_ARRAY_BOUNDS}

.include <bsd.prog.mk>
