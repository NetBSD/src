#	$NetBSD: Makefile.arc,v 1.38 2000/05/21 02:50:10 soren Exp $

# Makefile for NetBSD
#
# This makefile is constructed from a machine description:
#	config machineid
# Most changes should be made in the machine description
#	/sys/arch/arc/conf/``machineid''
# after which you should do
#	config machineid
# Machine generic makefile changes should be made in
#	/sys/arch/arc/conf/Makefile.arc
# after which config should be rerun for all machines of that type.
#
# N.B.: NO DEPENDENCIES ON FOLLOWING FLAGS ARE VISIBLE TO MAKEFILE
#	IF YOU CHANGE THE DEFINITION OF ANY OF THESE RECOMPILE EVERYTHING
#
# -DTRACE	compile in kernel tracing hooks
# -DQUOTA	compile in file system quotas

# DEBUG is set to -g if debugging.
# PROF is set to -pg if profiling.

AR?=	ar
AS?=	as
CC?=	cc
CPP?=	cpp
LD?=	ld
LORDER?=lorder
MKDEP?=	mkdep
NM?=	nm
RANLIB?=ranlib
STRIP?=	strip
SIZE?=	size
OBJCOPY?=objcopy
ELF2ECOFF?=${OBJCOPY} -O ecoff-littlemips
TOUCH?=	touch -f -c
TSORT?=	tsort -q

COPTS?= 	-O2 # -mmemcpy # XXX: - profile this

TEXTADDR?=	80100000

# source tree is located via $S relative to the compilation directory
.ifndef S
#S!=	cd ../../../..; pwd
S=	../../../..
.endif
ARC=	$S/arch/arc
MIPS=	$S/arch/mips

HAVE_EGCS!=	${CC} --version | egrep "^(2\.[89]|egcs)" ; echo 
INCLUDES=	-I. -I$S/arch -I$S -nostdinc
CPPFLAGS=	${INCLUDES} ${IDENT} ${PARAM} -D_KERNEL -Darc
CWARNFLAGS?=	-Werror -Wall -Wmissing-prototypes -Wstrict-prototypes \
		-Wpointer-arith
# XXX Delete -Wuninitialized for now, since the compiler doesn't
# XXX always get it right.  --thorpej 
CWARNFLAGS+=	-Wno-uninitialized
.if (${HAVE_EGCS} != "")
CWARNFLAGS+=	-Wno-main
.endif
GP?=		-G 0 # -G 30 # XXX: check this
CFLAGS=		${DEBUG} ${COPTS} ${CWARNFLAGS} ${GP} \
		-mips2 -mcpu=r4000 -mno-abicalls -mno-half-pic
AFLAGS=		-x assembler-with-cpp -traditional-cpp -mips2 -D_LOCORE
LINKFLAGS=	-T${MIPS}/conf/kern.ldscript.le -Ttext ${TEXTADDR} \
		-e start ${GP}
STRIPFLAGS=	-g -X -x

%INCLUDES

### find out what to use for libkern
.include "$S/lib/libkern/Makefile.inc"
.ifndef PROF
LIBKERN=	${KERNLIB}
.else
LIBKERN=	${KERNLIB_PROF}
.endif

### find out what to use for libcompat
.include "$S/compat/common/Makefile.inc"
.ifndef PROF
LIBCOMPAT=	${COMPATLIB}
.else
LIBCOMPAT=	${COMPATLIB_PROF}
.endif

# compile rules: rules are named ${TYPE}_${SUFFIX} where TYPE is NORMAL or
# HOSTED}, and SUFFIX is the file suffix, capitalized (e.g. C for a .c file).

NORMAL_C=	${CC} ${CFLAGS} ${CPPFLAGS} ${PROF} -c $<
NOPROF_C=	${CC} ${CFLAGS} ${CPPFLAGS} -c $<
NORMAL_S=	${CC} ${AFLAGS} ${CPPFLAGS} -c $<

%OBJS

%CFILES

%SFILES

# load lines for config "xxx" will be emitted as:
# xxx: ${SYSTEM_DEP} swapxxx.o
#	${SYSTEM_LD_HEAD}
#	${SYSTEM_LD} swapxxx.o
#	${SYSTEM_LD_TAIL}
SYSTEM_OBJ=	locore.o fp.o locore_machdep.o \
		param.o ioconf.o ${OBJS} ${LIBCOMPAT} ${LIBKERN}
.if !empty(IDENT:M-DMIPS3)
SYSTEM_OBJ+=	locore_mips3.o
.endif
SYSTEM_DEP=	Makefile ${SYSTEM_OBJ}
SYSTEM_LD_HEAD=	@rm -f $@
SYSTEM_LD=	@echo ${LD} ${LINKFLAGS} -o $@ '$${SYSTEM_OBJ}' vers.o ; \
		${LD} ${LINKFLAGS} -o $@ ${SYSTEM_OBJ} vers.o
SYSTEM_LD_TAIL=	@${SIZE} $@; chmod 755 $@

DEBUG?=
.if ${DEBUG} == "-g"
LINKFLAGS+=	-X
SYSTEM_LD_TAIL+=; \
		echo mv -f $@ $@.gdb; mv -f $@ $@.gdb; \
		echo ${STRIP} ${STRIPFLAGS} -o $@ $@.gdb; \
		${STRIP} ${STRIPFLAGS} -o $@ $@.gdb
.else
LINKFLAGS+=	-x
.endif

SYSTEM_LD_TAIL+=;\
		${ELF2ECOFF} $@ $@.ecoff

%LOAD

assym.h: $S/kern/genassym.sh ${MIPS}/mips/genassym.cf
	sh $S/kern/genassym.sh ${CC} ${CFLAGS} ${CPPFLAGS} ${PROF} \
	    < ${MIPS}/mips/genassym.cf > assym.h.tmp && \
	mv -f assym.h.tmp assym.h

param.c: $S/conf/param.c
	rm -f param.c
	cp $S/conf/param.c .

param.o: param.c Makefile
	${NORMAL_C}

ioconf.o: ioconf.c
	${NORMAL_C}

newvers: ${SYSTEM_DEP} ${SYSTEM_SWAP_DEP}
	sh $S/conf/newvers.sh
	${CC} ${CFLAGS} ${CPPFLAGS} ${PROF} -c vers.c

__CLEANKERNEL: .USE
	@echo "${.TARGET}ing the kernel objects"
	rm -f eddep *netbsd netbsd.gdb netbsd.ecoff netbsd.elf \
	    tags *.[io] [a-z]*.s [Ee]rrs linterrs makelinks assym.h.tmp assym.h

__CLEANDEPEND: .USE
	rm -f .depend

clean: __CLEANKERNEL

cleandir distclean: __CLEANKERNEL __CLEANDEPEND

lint:
	@lint -hbxncez -Dvolatile= ${CPPFLAGS} -UKGDB \
	    ${ARC}/arc/Locore.c ${CFILES} \
	    ioconf.c param.c | \
	    grep -v 'static function .* unused'

tags:
	@echo "see $S/kern/Makefile for tags"

links:
	egrep '#if' ${CFILES} | sed -f $S/conf/defines | \
	  sed -e 's/:.*//' -e 's/\.c/.o/' | sort -u > dontlink
	echo ${CFILES} | tr -s ' ' '\12' | sed 's/\.c/.o/' | \
	  sort -u | comm -23 - dontlink | \
	  sed 's,../.*/\(.*.o\),rm -f \1; ln -s ../GENERIC/\1 \1,' > makelinks
	sh makelinks && rm -f dontlink

SRCS=	${MIPS}/mips/locore.S ${MIPS}/mips/fp.S \
	${ARC}/arc/locore_machdep.S \
	param.c ioconf.c ${CFILES} ${SFILES}
depend: .depend
.depend: ${SRCS} assym.h param.c
	${MKDEP} ${AFLAGS} ${CPPFLAGS} ${MIPS}/mips/locore.S ${MIPS}/mips/fp.S
	${MKDEP} -a ${AFLAGS} ${CPPFLAGS} ${ARC}/arc/locore_machdep.S 
	${MKDEP} -a ${CFLAGS} ${CPPFLAGS} param.c ioconf.c ${CFILES}
	[ "${SFILES}" = "" ] || \
	${MKDEP} -a ${AFLAGS} ${CPPFLAGS} ${SFILES}
	sh $S/kern/genassym.sh ${MKDEP} -f assym.dep ${CFLAGS} \
	  ${CPPFLAGS} ${PROF} < ${MIPS}/mips/genassym.cf
	@sed -e 's/.*\.o:.*\.c/assym.h:/' < assym.dep >> .depend
	@rm -f assym.dep

dependall: depend all


# depend on root or device configuration
autoconf.o conf.o: Makefile

# depend on network or filesystem configuration
uipc_proto.o vfs_conf.o: Makefile

# depend on maxusers
machdep.o: Makefile

# depend on CPU configuration
machdep.o mainbus.o trap.o: Makefile

# depend on System V IPC/shmem options
mips_machdep.o pmap.o: Makefile

locore.o: ${MIPS}/mips/locore.S assym.h
	${NORMAL_S}

locore_mips1.o: ${MIPS}/mips/locore_mips1.S assym.h
	${NORMAL_S}

locore_mips3.o: ${MIPS}/mips/locore_mips3.S assym.h
	${NORMAL_S}

fp.o: ${MIPS}/mips/fp.S assym.h
	${NORMAL_S}

locore_machdep.o: ${ARC}/arc/locore_machdep.S assym.h
	${NORMAL_S}

# The install target can be redefined by putting a
# install-kernel-${MACHINE_NAME} target into /etc/mk.conf
MACHINE_NAME!=  uname -n
install: install-kernel-${MACHINE_NAME}
.if !target(install-kernel-${MACHINE_NAME}})
install-kernel-${MACHINE_NAME}:
	rm -f /onetbsd
	ln /netbsd /onetbsd
	cp netbsd /nnetbsd
	mv /nnetbsd /netbsd
.endif

%RULES
