#	$NetBSD: Makefile,v 1.15 1995/02/21 09:16:34 mycroft Exp $

#	@(#)Makefile	8.1 (Berkeley) 6/10/93

# RELOC=FFF00000 allows for boot prog up to FF000 (1044480) bytes long
RELOC=	FFF00000

CONS=	-DDCACONSOLE -DITECONSOLE -DDCMCONSOLE
DEFS=	-DSTANDALONE -DCOMPAT_NOLABEL ${CONS} -Dhp300 # -DROMPRF
DEFS+=	-DNETIF_DEBUG
CFLAGS=	-O3 ${INCPATH} ${DEFS}

SRCS=   autoconf.c cons.c devopen.c machdep.c prf.c
OBJS=	${SRCS:N*.h:R:S/$/.o/g}
DSRCS=	ct.c dca.c dcm.c fhpib.c hil.c hpib.c if_le.c \
	ite.c ite_dv.c ite_gb.c ite_rb.c ite_subr.c ite_tc.c ite_hy.c \
	nhpib.c rd.c scsi.c sd.c
DOBJS=	${DSRCS:N*.h:R:S/$/.o/g}

S=	${.CURDIR}/../../..

.PATH: ${S}/arch/${MACHINE_ARCH}/${MACHINE_ARCH}
.PATH: ${S}/stand ${S}/lib/libsa

INCPATH=-I${.CURDIR} -I${.CURDIR}/../.. -I${S} -I${S}/lib/libsa

### find out what to use for libkern
.include "$S/lib/libkern/Makefile.inc"
LIBKERN=	${KERNLIB}

.include "$S/lib/libsa/Makefile.inc"
LIBSA=	${SA_LIB}

LIBS=	${OBJS} libdrive.a ${LIBSA} ${LIBKERN}

BOOTS=	pboot netboot dboot dcopy tboot tcopy cat #ls
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

tpsrt0.o: ${.CURDIR}/srt0.s
	${CC} ${INCPATH} ${DEFS} -DTP -c ${.CURDIR}/srt0.s -o $@

# new boot
pboot:	srt0.o pboot.o tgets.o conf.o ${LIBS}
	${LD} -N -T ${RELOC} -e begin srt0.o pboot.o tgets.o conf.o ${LIBS} -o $@
	@size $@
	@echo $@ total size should not exceed 1044480 bytes

ls:	srt0.o ls.o conf.o ${LIBS}
	${LD} -N -T ${RELOC} -e begin srt0.o ls.o conf.o ${LIBS} -o $@
	@size $@
	@echo $@ total size should not exceed 1044480 bytes

cat:	srt0.o cat.o conf.o ${LIBS}
	${LD} -N -T ${RELOC} -e begin srt0.o cat.o conf.o ${LIBS} -o $@
	@size $@
	@echo $@ total size should not exceed 1044480 bytes

# bootable from network

netboot:	srt0.o netboot.o clock.o netconf.o ${LIBS}
	${LD} -N -T ${RELOC} -e begin srt0.o netboot.o clock.o netconf.o ${LIBS} -o $@
	@size $@
	@echo $@ total size should not exceed 1044480 bytes
	
netconf.o: ${.CURDIR}/conf.c
	${CC} ${CFLAGS} -DNETBOOT -c ${.CURDIR}/conf.c -o $@

# bootable from tape

tboot:	srt0.o tboot.o conf.o ${LIBS}
	${LD} -N -T ${RELOC} -e begin srt0.o tboot.o conf.o ${LIBS} -o $@
	@size $@
	@echo $@ total size should not exceed 1044480 bytes

tboot.o: ${.CURDIR}/boot.c
	${CC} ${CFLAGS} -DJUSTASK -c ${.CURDIR}/boot.c -o $@

tcopy:	tpsrt0.o copy.o conf.o ${LIBS}
	${LD} -N -T ${RELOC} -e begin tpsrt0.o copy.o conf.o ${LIBS} -o $@
	@size $@
	@echo $@ total size should not exceed 1044480 bytes

# bootable from floppy or real disks

dboot:	srt0.o boot.o tapeconf.o ${LIBS}
	${LD} -N -T ${RELOC} -e begin srt0.o boot.o tapeconf.o ${LIBS} -o $@
	@size $@
	@echo $@ text+data size should not exceed 57344 bytes
	@echo $@ total size should not exceed 1044480 bytes

tapeconf.o: ${.CURDIR}/conf.c
	${CC} ${CFLAGS} -DTAPEBOOT -c ${.CURDIR}/conf.c -o $@

dcopy:	srt0.o copy.o conf.o ${LIBS}
	${LD} -N -T ${RELOC} -e begin srt0.o copy.o conf.o ${LIBS} -o $@

mkboot: ${.CURDIR}/mkboot.c
	${CC} ${CFLAGS} ${.CURDIR}/mkboot.c -o $@

installboot: ${.CURDIR}/installboot.sh
	@rm -f installboot
	cp -p ${.CURDIR}/installboot.sh installboot

# utilities

clean cleandir:
	rm -f *.o *.exe *.i errs make.out
	rm -f a.out pboot cat ls tboot tcopy
	rm -f boot[a-z]? boot[a-wyz][a-z].c conf[a-wyz][a-z].c
	rm -f core sboot
	rm -f libdrive.a mkboot dboot dcopy installboot *.lif

install: mkboot installboot ${ALL}
	./mkboot pboot netbsdboot.lif
	./mkboot netboot netboot.lif
	./mkboot dboot diskboot.lif
	./mkboot tboot tcopy tapeboot.lif
	install -c -o bin -g bin -m 555 installboot ${DESTDIR}/usr/mdec
	install -c -o bin -g bin -m 444 netbsdboot.lif ${DESTDIR}/usr/mdec/rdboot
	rm -f ${DESTDIR}/usr/mdec/bootrd
	ln ${DESTDIR}/usr/mdec/rdboot ${DESTDIR}/usr/mdec/bootrd
	rm -f ${DESTDIR}/usr/mdec/sdboot
	ln ${DESTDIR}/usr/mdec/rdboot ${DESTDIR}/usr/mdec/sdboot
	rm -f ${DESTDIR}/usr/mdec/bootsd
	ln ${DESTDIR}/usr/mdec/sdboot ${DESTDIR}/usr/mdec/bootsd
	install -c -o bin -g bin -m 444 netboot.lif ${DESTDIR}/usr/mdec/rbootd/SYS_NBOOT
	install -c -o bin -g bin -m 644 tapeboot.lif ${DESTDIR}/usr/mdec/bootct
#	install -o bin -g bin -m 755 -d ${DESTDIR}/sys/hpdist/tp
#	install -o bin -g bin -m 444 netbsdboot.lif ${DESTDIR}/sys/hpdist/tp
#	install -o bin -g bin -m 444 tapeboot.lif ${DESTDIR}/sys/hpdist/tp
#	install -o bin -g bin -m 444 diskboot.lif ${DESTDIR}/sys/hpdist/tp

.include <bsd.dep.mk>
.include <bsd.obj.mk>
# XXX This should not be necessary.
.include <bsd.subdir.mk>
