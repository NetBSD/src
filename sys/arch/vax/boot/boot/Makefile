#	$NetBSD: Makefile,v 1.16 2000/09/23 13:53:42 simonb Exp $

S=	${.CURDIR}/../../../..

.include <bsd.own.mk>

PROG=	boot
NOMAN=	1
WARNS?=	1
DEVS=	hp.c ctu.c ra.c mfm.c if_qe.c if_le.c if_ze.c if_de.c if_ni.c
SRCS=	srt0.s boot.c devopen.c conf.c autoconf.c netio.c rom.c romread.s \
	urem.s udiv.s consio.c consio2.s str.s ${DEVS} findcpu.c \
	vers.c

CLEANFILES+=${PROG} ${PROG}.mop vers.c
CPPFLAGS+=-DSUPPORT_BOOTPARAMS -DSUPPORT_BOOTP -DSUPPORT_DHCP -D_STANDALONE
#CPPFLAGS+=-DBOOTP_DEBUG -DNETIF_DEBUG -DETHER_DEBUG -DNFS_DEBUG -DDEV_DEBUG \
#	-DRPC_DEBUG -DRARP_DEBUG -DPARANOID
BINDIR=	/
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

.if ${MACHINE} == "vax"
.BEGIN:
	@rm -f machine && ln -s ${S}/arch/${MACHINE}/include machine
	@rm -f ${MACHINE_ARCH} && ln -s ${S}/arch/${MACHINE_ARCH}/include ${MACHINE_ARCH}
.NOPATH: machine ${MACHINE_ARCH}
CLEANFILES+= machine ${MACHINE_ARCH}
.endif

.PHONY: vers.c
vers.c: ${.CURDIR}/version
	sh ${S}/conf/newvers_stand.sh -NM ${.CURDIR}/version "${MACHINE}"

${PROG}: ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	${LD} -N -Ttext ${RELOC} -e nisse -o ${PROG} ${OBJS} \
	    ${LIBSA} ${LIBZ} ${LIBSA} ${LIBKERN}
.if ${OBJECT_FMT} != "ELF"
	/usr/sbin/mopa.out ${PROG} ${PROG}.mop
.endif
	${STRIP} ${PROG}
	${SIZE} ${PROG}

install:
	${INSTALL} ${COPY} -m 555 -o ${BINOWN} -g ${BINGRP} ${PROG} \
	    ${DESTDIR}${BINDIR}
	${INSTALL} ${COPY} -m 555 -o ${BINOWN} -g ${BINGRP} ${PROG} \
	    ${DESTDIR}${MDEC_DIR}
	${INSTALL} ${COPY} -m 555 -o ${BINOWN} -g ${BINGRP} ${PROG}.mop \
	    ${DESTDIR}${MDEC_DIR}

.include <bsd.prog.mk>
