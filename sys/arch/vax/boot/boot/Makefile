#	$NetBSD: Makefile,v 1.24 2002/02/24 00:17:42 matt Exp $

S=	${.CURDIR}/../../../..
NOMAN=	# defined

.include <bsd.own.mk>

PROG=	boot
WARNS?=	1
DEVS=	hp.c ctu.c ra.c mfm.c if_qe.c if_le.c if_ze.c if_de.c if_ni.c
SRCS=	srt0.s boot.c devopen.c conf.c autoconf.c netio.c rom.c romread.s \
	urem.s udiv.s consio.c consio2.s str.s ${DEVS} findcpu.c \
	vers.c

CLEANFILES+=${PROG} ${PROG}.mop vers.c
CPPFLAGS+=-DSUPPORT_BOOTPARAMS -DSUPPORT_BOOTP -DSUPPORT_DHCP -D_STANDALONE \
	-DNO_MID_CHECK
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
.PHONY: machine-links
beforedepend: machine-links
machine-links:
	@rm -f machine && ln -s ${S}/arch/${MACHINE}/include machine
	@rm -f ${MACHINE_ARCH} && ln -s ${S}/arch/${MACHINE_ARCH}/include ${MACHINE_ARCH}
.NOPATH: machine ${MACHINE_ARCH}
CLEANFILES+= machine ${MACHINE_ARCH}
.endif


.if ${OBJECT_FMT} == "ELF"
START=start
.else
START=nisse
.endif

.PHONY: vers.c
vers.c: ${.CURDIR}/version
	sh ${S}/conf/newvers_stand.sh -NM ${.CURDIR}/version "${MACHINE}"

${PROG}: machine-links ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	${LD} -N -Ttext ${RELOC} -e ${START} -o ${PROG}.sym ${OBJS} \
	    ${LIBSA} ${LIBZ} ${LIBSA} ${LIBKERN}
.if ${OBJECT_FMT} != "ELF"
	/usr/sbin/mopa.out ${PROG}.sym ${PROG}.mop
.endif
	${STRIP} -o ${PROG} ${PROG}.sym
	${SIZE} ${PROG}

install:
	${INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m 555 \
		${PROG} ${DESTDIR}${BINDIR}
	${INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m 555 \
		${PROG} ${DESTDIR}${MDEC_DIR}
.if ${OBJECT_FMT} != "ELF"
	${INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m 555 \
		${PROG}.mop ${DESTDIR}${MDEC_DIR}
.endif

.include <bsd.prog.mk>
