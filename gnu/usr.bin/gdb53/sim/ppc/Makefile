#	$NetBSD: Makefile,v 1.2.2.2 2004/04/06 04:34:07 jmc Exp $

NOLINT=		# defined
NOPIC=		# defined
NOPROFILE=	# defined

.include <bsd.own.mk>

TOP=	${NETBSDSRCDIR}/gnu
DIST=	${TOP}/dist/gdb

LIB=	sim
SRCS=	debug.c filter_filename.o bits.c sim-endian.c os_emul.c emul_generic.c \
	emul_bugapi.c emul_chirp.c emul_netbsd.c emul_unix.c registers.c vm.c \
	corefile.c model.c spreg.c cpu.c interrupts.c events.c cap.c device.c \
	tree.c device_table.c itable.c mon.c icache.c semantics.c idecode.c \
	support.c psim.c pk_disklabel.c hw_cpu.c hw_memory.c hw_nvram.c \
	hw_iobus.c hw_htab.c hw_disk.c hw_trace.c hw_register.c hw_vm.c \
	hw_init.c hw_core.c hw_pal.c hw_com.c hw_eeprom.c hw_opic.c hw_glue.c \
	hw_phb.c hw_ide.c options.c sim_calls.c callback.o targ-map.c

all: libsim.a

libinstall:

.include <bsd.lib.mk>
.include "../../Makefile.inc"

MIC=	${HOST_SH} ${DIST}/move-if-change

HOST_CPPFLAGS+=	-I${DIST}/sim/ppc -I${DIST}/include \
		-I${DIST}/sim/common \
		-I${DIST}/bfd \
		-I${DIST}/gdb \
		-I${DIST}/gdb/config \
		-I${DIST}/mmalloc \
		-I${TOP}/usr.bin/gdb53/arch/powerpc \
		-I.
PSIM_DEFINES=	-DHAVE_CONFIG_H \
		-DDEFAULT_INLINE=PSIM_INLINE_LOCALS \
		-DWITH_HOST_BYTE_ORDER=BYTE_ORDER \
		-DWITH_SMP=5 \
		-DHAVE_TERMIOS_STRUCTURE \
		-DHAVE_DEVZERO

# we need the same -I flags for host / target.
CPPFLAGS+=	${HOST_CPPFLAGS} ${PSIM_DEFINES}

#
# These portions run several programs to generate more code to compile.
#

# igen portion
IGEN_FLAGS=	-E    -F 32,f,o  -CSRI 1024  -N 5 \
	-o ${DIST}/sim/ppc/dc-complex \
	-k ${DIST}/sim/ppc/ppc-cache-rules \
	-i ${DIST}/sim/ppc/ppc-instructions \
	-n icache.h    -hc tmp-icache.h \
	-n icache.c    -c  tmp-icache.c \
	-n semantics.h -hs tmp-semantics.h \
	-n semantics.c -s  tmp-semantics.c \
	-n idecode.h   -hd tmp-idecode.h \
	-n idecode.c   -d  tmp-idecode.c \
	-n itable.h    -ht tmp-itable.h \
	-n itable.c    -t  tmp-itable.c \
	-n model.h     -hm tmp-model.h \
	-n model.c     -m  tmp-model.c \
	-n support.h   -hf tmp-support.h \
	-n support.c   -f  tmp-support.c
IGENED_FILES=	icache.h icache.c idecode.h idecode.c semantics.h semantics.c \
		itable.h itable.c model.h model.c support.h support.c
IGENSRCS= igen.c table.c lf.c misc.c filter_host.c ld-decode.c ld-cache.c filter.c ld-insn.c gen-model.c gen-itable.c gen-icache.c gen-semantics.c gen-idecode.c gen-support.c
IGENOBJS= ${IGENSRCS:S/.c/.lo/}

${IGENED_FILES}: run-igen
run-igen: igen ppc-instructions ppc-cache-rules
	${_MKMSG_CREATE} ${IGENED_FILES}
	rm -f ${.TARGET}
	./igen ${IGEN_FLAGS}
	${MIC} tmp-icache.h icache.h
	${MIC} tmp-icache.c icache.c
	${MIC} tmp-idecode.h idecode.h
	${MIC} tmp-idecode.c idecode.c
	${MIC} tmp-semantics.h semantics.h
	${MIC} tmp-semantics.c semantics.c
	${MIC} tmp-itable.h itable.h
	${MIC} tmp-itable.c itable.c
	${MIC} tmp-model.h model.h
	${MIC} tmp-model.c model.c
	${MIC} tmp-support.h support.h
	${MIC} tmp-support.c support.c
	touch ${.TARGET}

igen: ${IGENOBJS}
	${HOST_LINK.c} ${IGENOBJS} -o ${.TARGET}

# dgen portion
DGEN_FLAGS=	-r ${DIST}/sim/ppc/ppc-spr-table \
	-n spreg.h     -hp tmp-spreg.h \
	-n spreg.c     -p  tmp-spreg.c
DGENED_FILES=	spreg.h spreg.c
DGENSRCS= dgen.c table.c lf.c misc.c filter_host.c
DGENOBJS= ${DGENSRCS:S/.c/.lo/}

${DGENED_FILES}: run-dgen
run-dgen: dgen
	./dgen ${DGEN_FLAGS}
	${MIC} tmp-spreg.h spreg.h
	${MIC} tmp-spreg.c spreg.c
	touch run-dgen

dgen: ${DGENOBJS}
	${HOST_LINK.c} ${DGENOBJS} -o ${.TARGET}

# gentmap portion
GENTMAPSRCS= gentmap.c
GENTMAPOBJS= ${GENTMAPSRCS:S/.c/.lo/}

gentmap: ${GENTMAPOBJS}
	${HOST_LINK.c} ${GENTMAPOBJS} -o ${.TARGET}

gentmap.c: targ-vals.def

callback.o: targ-vals.h targ-vals.def

targ-vals.def: ${DIST}/sim/ppc/../common/nltvals.def
	rm -f targ-vals.def tmp-def
	cat ${DIST}/sim/ppc/../common/nltvals.def > tmp-vals.def
	${MIC} tmp-vals.def targ-vals.def

targ-vals.h: gentmap targ-vals.def
	rm -f tmp-vals.h
	./gentmap -h > tmp-vals.h
	${MIC} tmp-vals.h targ-vals.h

targ-map.c: gentmap targ-vals.def
	rm -f tmp-map.c
	./gentmap -c > tmp-map.c
	${MIC} tmp-map.c targ-map.c

# hw.c / hw.h portion
HWSRCS=	hw_cpu.c hw_memory.c hw_nvram.c hw_iobus.c hw_htab.c hw_disk.c \
	hw_trace.c hw_register.c hw_vm.c hw_init.c hw_core.c hw_pal.c \
	hw_com.c hw_eeprom.c hw_opic.c hw_glue.c hw_phb.c hw_ide.c
HWOBJS= ${HWSRCS:S/.c/.o/}

${HWSRCS}: hw.h
${HWOBJS}: ${HWSRCS}

hw.h hw.c: run-hw
run-hw: Makefile
	f=""; \
	for i in ${HWSRCS}; do \
		case " $$f " in \
			*" $$i "*) ;; \
			*) f="$$f $$i" ;; \
		esac ; \
	done ; \
	for hw in $$f; do \
		echo $$hw; \
	done | sed -e 's/^.*\(hw_.*\)\.c/\1/' \
		   -e 's/^/extern const device_descriptor /' \
		   -e 's/$$/_device_descriptor\[\];/' \
			> tmp-hw.h
	f=""; \
	for i in ${HWSRCS}; do \
		case " $$f " in \
			*" $$i "*) ;; \
			*) f="$$f $$i" ;; \
		esac ; \
	done ; \
	for hw in $$f; do \
		echo $$hw; \
	done | sed -e 's/^.*\(hw_.*\)\.c/\1/' \
		   -e 's/^/    /' \
		   -e 's/$$/_device_descriptor,/' > tmp-hw.c
	${MIC} tmp-hw.h hw.h
	${MIC} tmp-hw.c hw.c
	touch run-hw

# package portion
PACKAGE_SRC =	pk_disklabel.c
PK_H = 		pk.h

${PK_H}: run-pk
run-pk: $(PACKAGE_SRC)
	f=""; \
	for i in $(PACKAGE_SRC) ; do \
		case " $$f " in \
			*" $$i "*) ;; \
			*) f="$$f $$i" ;; \
		esac ; \
	done ; \
	for pk in $$f; do \
		echo $$pk; \
	done | sed -e 's/^.*pk_\(.*\)\.c/\1/' \
		   -e 's/^/extern package_create_instance_callback pk_/' \
		   -e 's/$$/_create_instance;/' > tmp-pk.h
	${MIC} tmp-pk.h pk.h
	touch run-pk

pk_disklabel.o: ${PK_H}

hw_disk.c: ${PK_H}

# defines portion
DEFINES_H=	defines.h
${DEFINES_H}: run-defines
run-defines:
	sed -n -e '/^#define HAVE_/s/ 1$$/",/' \
	       -e '/^#define HAVE_/s//"HAVE_/p' \
	       < ${.CURDIR}/../arch/${MACHINE_ARCH}/config.h > tmp-defines.h
	${MIC} tmp-defines.h defines.h
	touch run-defines

options.c: ${DEFINES_H}

# misc. dependancies.
corefile.o: device_table.h
device_table.h: hw.h

filter_host.lo filter_filename.o debug.c: ppc-config.h

os_emul.c: ${IGENED_FILES} ${DGENED_FILES}

filter_host.c: ${DIST}/sim/ppc/filter_filename.c
	cp ${.ALLSRC} ${.TARGET}

ppc-config.h: ${DIST}/sim/ppc/std-config.h
	cp ${.ALLSRC} ${.TARGET}

CLEANFILES+=	igen dgen gentmap *.lo \
		ppc-config.h filter_host.c \
		tmp-icache.h icache.h \
		tmp-icache.c icache.c \
		tmp-idecode.h idecode.h \
		tmp-idecode.c idecode.c \
		tmp-semantics.h semantics.h \
		tmp-semantics.c semantics.c \
		tmp-itable.h itable.h \
		tmp-itable.c itable.c \
		tmp-model.h model.h \
		tmp-model.c model.c \
		tmp-support.h support.h \
		tmp-support.c support.c \
		tmp-spreg.h spreg.h \
		tmp-spreg.c spreg.c \
		tmp-vals.h targ-vals.h \
		hw.c hw.h \
		defines.h pk.h \
		targ-map.c targ-vals.def \
		run-defines run-dgen run-hw run-igen run-pk

.SUFFIXES: .lo
.c.lo:
	${HOST_COMPILE.c} -o ${.TARGET} $<

corefile.c: hw.h
cpu.c: spreg.h idecode.h itable.h model.h

.PATH: ${DIST}/sim/ppc ${DIST}/sim/common
