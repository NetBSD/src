#	from: @(#)Makefile	8.1 (Berkeley) 6/10/93
#	     $Id: Makefile,v 1.8 1994/06/27 04:00:52 hpeyerl Exp $

# RELOC=FFF00000 allows for boot prog up to FF000 (1044480) bytes long
RELOC=	FFF00000

CONS= -DDCACONSOLE -DITECONSOLE -DDCMCONSOLE
DEFS= -DSTANDALONE -DCOMPAT_NOLABEL ${CONS} -Dhp300 # -DROMPRF
CFLAGS=	-O ${INCPATH} ${DEFS}

SRCS=   autoconf.c cons.c ct.c devopen.c dca.c dcm.c fhpib.c hil.c hpib.c \
	ite.c ite_dv.c ite_gb.c ite_rb.c ite_subr.c ite_tc.c ite_hy.c \
	nhpib.c rd.c scsi.c sd.c
SRCS+=	conf.c machdep.c prf.c tgets.c

S=	${.CURDIR}/../../..

.PATH: ${S}/arch/${MACHINE_ARCH}/${MACHINE_ARCH}
.PATH: ${S}/stand ${S}/lib/libsa

INCPATH=-I${.CURDIR} -I${.CURDIR}/../.. -I${S} -I${S}/lib/libsa

### find out what to use for libkern
.include "$S/lib/libkern/Makefile.inc"
LIBKERN=	${KERNLIB}

.include "$S/lib/libsa/Makefile.inc"
LIBSA=	${SA_LIB}

LIBS=	${LIBSA} ${.OBJDIR}/libdrive.a \
	${LIBSA} ${LIBKERN}

BOOTS=	pboot dboot nboot dcopy tboot tcopy cat #ls
ALL=	${BOOTS} mkboot installboot

all: ${ALL}

${BOOTS}: ${LIBS}

OBJS=	${SRCS:N*.h:R:S/$/.o/g}
${.OBJDIR}/libdrive.a: ${OBJS}
	ar crv $@ $?
	ranlib $@

# depend on DEFS

devopen.o machdep.o srt0.o: Makefile
cons.o dca.o hil.o: Makefile
ite.o ite_subr.o ite_dv.o ite_gb.o ite_hy.o ite_rb.o ite_tc.o: Makefile

# startups

srt0.o: ${.CURDIR}/srt0.s
	${CC} ${INCPATH} ${DEFS} -c ${.CURDIR}/srt0.s

tpsrt0.o: ${.CURDIR}/srt0.s
	${CC} ${INCPATH} ${DEFS} -DTP -c ${.CURDIR}/srt0.s -o tpsrt0.o

# new boot
pboot:	pboot.o srt0.o ${LIBS}
	ld -N -T ${RELOC} -e begin srt0.o pboot.o ${LIBS} -o $@
	@size pboot
	@echo pboot total size should not exceed 1044480 bytes

ls:	ls.o srt0.o ${LIBS}
	ld -N -T ${RELOC} -e begin srt0.o ls.o ${LIBS} -o $@
	@size ls
	@echo ls total size should not exceed 1044480 bytes

cat:	cat.o srt0.o ${LIBS}
	ld -N -T ${RELOC} -e begin srt0.o cat.o ${LIBS} -o $@
	@size cat
	@echo cat total size should not exceed 1044480 bytes

# bootable from tape

tboot:	tboot.o srt0.o ${LIBS}
	ld -N -T ${RELOC} -e begin srt0.o tboot.o ${LIBS} -o $@
	@size tboot
	@echo tboot total size should not exceed 1044480 bytes

tboot.o: boot.c
	cp ${.CURDIR}/boot.c tboot.c; chmod +w tboot.c
	${CC} -c ${CFLAGS} -DJUSTASK tboot.c
	rm -f tboot.c

tcopy:	copy.o tpsrt0.o ${LIBS}
	ld -N -T ${RELOC} -e begin tpsrt0.o copy.o ${LIBS} -o $@
	@size tcopy
	@echo tcopy total size should not exceed 1044480 bytes

# bootable from floppy or real disks

dboot:	boot.o srt0.o bootconf.o ${LIBS}
	ld -N -T ${RELOC} -e begin srt0.o boot.o bootconf.o ${LIBS} -o $@ 
	@size dboot
	@echo dboot text+data size should not exceed 57344 bytes
	@echo dboot total size should not exceed 1044480 bytes

bootconf.o: ${.CURDIR}/conf.c
	rm -f bootconf.c
	ln -s ${.CURDIR}/conf.c bootconf.c
	${CC} -c ${CFLAGS} -DBOOT bootconf.c
	rm -f bootconf.c

# non-secure boot (allows booting non-root owned or world writable kernels)

nboot:	nboot.o srt0.o bootconf.o ${LIBS}
	ld -N -T ${RELOC} -e begin -o nboot srt0.o nboot.o bootconf.o ${LIBS}
	@size nboot
	@echo nboot text+data size should not exceed 57344 bytes
	@echo nboot total size should not exceed 1044480 bytes

nboot.o: ${.CURDIR}/boot.c
	rm -f nboot.c
	ln -s ${.CURDIR}/boot.c nboot.c
	${CC} -c ${CFLAGS} -DINSECURE nboot.c
	rm -f nboot.c

dcopy:	copy.o srt0.o conf.o ${LIBS}
	ld -N -T ${RELOC} -e begin -o dcopy srt0.o copy.o conf.o ${LIBS}

mkboot: ${.CURDIR}/mkboot.c
	${CC} ${CFLAGS} -o mkboot ${.CURDIR}/mkboot.c

installboot: ${.CURDIR}/installboot.c
	${CC} ${CFLAGS} -o installboot ${.CURDIR}/installboot.c

# utilities

clean cleandir:
	rm -f *.o *.exe *.i errs make.out
	rm -f a.out pboot cat ls tboot tcopy
	rm -f boot[a-z]? boot[a-wyz][a-z].c conf[a-wyz][a-z].c
	rm -f core sboot bootconf.c nboot.c
	rm -f libdrive.a mkboot dboot dcopy nboot installboot *.lif

install: mkboot installboot ${ALL}
	./mkboot pboot netbsdboot.lif
	./mkboot dboot diskboot.lif
	./mkboot nboot ndiskboot.lif
	./mkboot tboot tcopy tapeboot.lif
	install -c -o bin -g bin -m 555 installboot ${DESTDIR}/usr/mdec
	install -c -o bin -g bin -m 444 netbsdboot.lif ${DESTDIR}/usr/mdec/rdboot
	rm -f ${DESTDIR}/usr/mdec/bootrd
	ln ${DESTDIR}/usr/mdec/rdboot ${DESTDIR}/usr/mdec/bootrd
	rm -f ${DESTDIR}/usr/mdec/sdboot
	ln ${DESTDIR}/usr/mdec/rdboot ${DESTDIR}/usr/mdec/sdboot
	rm -f ${DESTDIR}/usr/mdec/bootsd
	ln ${DESTDIR}/usr/mdec/sdboot ${DESTDIR}/usr/mdec/bootsd
	install -c -o bin -g bin -m 644 tapeboot.lif ${DESTDIR}/usr/mdec/bootct
	install -o bin -g bin -m 755 -d ${DESTDIR}/sys/hpdist/tp
	install -o bin -g bin -m 444 netbsdboot.lif ${DESTDIR}/sys/hpdist/tp
	install -o bin -g bin -m 444 tapeboot.lif ${DESTDIR}/sys/hpdist/tp
	install -o bin -g bin -m 444 diskboot.lif ${DESTDIR}/sys/hpdist/tp
	install -o bin -g bin -m 444 ndiskboot.lif ${DESTDIR}/sys/hpdist/tp

.include <bsd.dep.mk>
.include <bsd.obj.mk>

FRC:
