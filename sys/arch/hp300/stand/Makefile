#	$NetBSD: Makefile,v 1.20 1996/06/26 17:44:21 thorpej Exp $

#	@(#)Makefile	8.1 (Berkeley) 6/10/93

NOPROG=	noprog
NOMAN=	noman

SUBDIR=	libsa

# RELOC=FFF00000 allows for boot prog up to FF000 (1044480) bytes long
RELOC=	FFF00000

CONS=	-DDCACONSOLE -DITECONSOLE -DDCMCONSOLE
DEFS=	-DSTANDALONE -DCOMPAT_NOLABEL ${CONS} -Dhp300 # -DROMPRF
#DEFS+=	-DNETIF_DEBUG -DRPC_DEBUG -DNFS_DEBUG -DRARP_DEBUG -DNET_DEBUG
#DEFS+=	-DLE_DEBUG
CFLAGS=	-O3 ${INCPATH} ${DEFS}

SRCS=   autoconf.c cons.c devopen.c machdep.c prf.c
OBJS=	${SRCS:N*.h:R:S/$/.o/g}
DSRCS=	ct.c dca.c dcm.c fhpib.c hil.c hpib.c if_le.c \
	ite.c ite_dv.c ite_gb.c ite_rb.c ite_subr.c ite_tc.c ite_hy.c \
	nhpib.c rd.c scsi.c sd.c
DOBJS=	${DSRCS:N*.h:R:S/$/.o/g}

S=	${.CURDIR}/../../..

.PATH: ${S}/arch/${MACHINE_ARCH}/${MACHINE_ARCH}
.PATH: ${S}/stand

INCPATH=-I${.CURDIR} -I${.CURDIR}/../.. -I${S} -I${S}/lib/libsa
### find out what to use for libkern
KERNREL=
.include "$S/lib/libkern/Makefile.inc"
LIBKERN=	${KERNLIB}

.include "${.CURDIR}/libsa/Makefile.inc"
LIBSA=	${SA_LIB}

LIBS=	${OBJS} libdrive.a ${LIBSA} ${LIBKERN}

BOOTS=	uboot inst
ALL=	${BOOTS} mkboot installboot

all: ${ALL}

${BOOTS}: ${LIBS}

libdrive.a: ${DOBJS}
	rm -f $@
	${AR} cq $@ ${DOBJS}
	${RANLIB} $@

# depend on DEFS

devopen.o machdep.o srt0.o: Makefile
cons.o dca.o hil.o: Makefile
ite.o ite_subr.o ite_dv.o ite_gb.o ite_hy.o ite_rb.o ite_tc.o: Makefile

# startups

srt0.o: ${.CURDIR}/srt0.s
	${CC} ${INCPATH} ${DEFS} -c ${.CURDIR}/srt0.s

# unified boot program (disk, network, tape)
uboot:	srt0.o uboot.o tgets.o netio.o clock.o conf.o ${LIBS}
	${LD} -N -T ${RELOC} -e begin srt0.o uboot.o tgets.o netio.o clock.o \
	    conf.o ${LIBS} -o $@
	@size $@
	@echo $@ total size should not exceed 1044480 bytes

# miniroot installation program
inst: srt0.o inst.o clock.o conf.o netio.o tgets.o ${LIBS}
	${LD} -N -T ${RELOC} -e begin srt0.o inst.o clock.o conf.o netio.o \
	    tgets.o ${LIBS} -o $@
	@size $@
	@echo $@ total size should not exceed 1044480 bytes

# helper program ... turns OMAGIC into LIF
mkboot: ${.CURDIR}/mkboot.c
	${CC} ${CFLAGS} ${.CURDIR}/mkboot.c -o $@

installboot: ${.CURDIR}/installboot.sh
	@rm -f installboot
	cp -p ${.CURDIR}/installboot.sh installboot

# utilities

clean::
	rm -f *.o *.i
	rm -f a.out ${BOOTS}
	rm -f libdrive.a mkboot installboot *.lif

install: mkboot installboot ${ALL}
	./mkboot uboot uboot.lif
	./mkboot inst inst.lif
	install -d -m 755 -o ${BINOWN} -g ${BINGRP} ${DESTDIR}/usr/mdec/rbootd
	install -c -o ${BINOWN} -g ${BINGRP} -m 555 installboot \
	    ${DESTDIR}/usr/mdec
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 uboot.lif \
	    ${DESTDIR}/usr/mdec
	rm -f ${DESTDIR}/usr/mdec/rdboot
	ln ${DESTDIR}/usr/mdec/uboot.lif ${DESTDIR}/usr/mdec/rdboot
	rm -f ${DESTDIR}/usr/mdec/bootrd
	ln ${DESTDIR}/usr/mdec/uboot.lif ${DESTDIR}/usr/mdec/bootrd
	rm -f ${DESTDIR}/usr/mdec/sdboot
	ln ${DESTDIR}/usr/mdec/uboot.lif ${DESTDIR}/usr/mdec/sdboot
	rm -f ${DESTDIR}/usr/mdec/bootsd
	ln ${DESTDIR}/usr/mdec/uboot.lif ${DESTDIR}/usr/mdec/bootsd
	rm -f ${DESTDIR}/usr/mdec/ctboot
	ln ${DESTDIR}/usr/mdec/uboot.lif ${DESTDIR}/usr/mdec/ctboot
	rm -f ${DESTDIR}/usr/mdec/bootct
	ln ${DESTDIR}/usr/mdec/uboot.lif ${DESTDIR}/usr/mdec/bootct
	rm -f ${DESTDIR}/usr/mdec/rbootd/SYS_UBOOT
	ln ${DESTDIR}/usr/mdec/uboot.lif ${DESTDIR}/usr/mdec/rbootd/SYS_UBOOT
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 inst.lif \
	    ${DESTDIR}/usr/mdec/rbootd/SYS_INST

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
