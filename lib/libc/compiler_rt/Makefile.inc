# $NetBSD: Makefile.inc,v 1.2 2013/12/03 02:28:51 joerg Exp $

COMPILER_RT_SRCDIR=	${NETBSDSRCDIR}/sys/external/bsd/compiler_rt/dist

.if ${MACHINE_ARCH} == "powerpc"
COMPILER_RT_CPU_DIR=	${COMPILER_RT_SRCDIR}/lib/ppc
COMPILER_RT_ARCH_DIR=	${COMPILER_RT_SRCDIR}/lib/ppc
.else
COMPILER_RT_CPU_DIR=	${COMPILER_RT_SRCDIR}/lib/${MACHINE_CPU}
COMPILER_RT_ARCH_DIR=	${COMPILER_RT_SRCDIR}/lib/${MACHINE_ARCH}
.endif

.PATH: ${COMPILER_RT_CPU_DIR}
.PATH: ${COMPILER_RT_ARCH_DIR}
.PATH: ${COMPILER_RT_SRCDIR}/lib

# Complex support needs parts of libm
#GENERIC_SRCS+=
#	mulxc3.c \
#	mulsc3.c \
#	divxc3.c \
#	divdc3.c \
#	divsc3.c

.if ${HAVE_LLVM:Uno} == "yes"
# Requires C11 support
GENERIC_SRCS+= atomic.c
.endif

.if ${HAVE_LIBGCC} == "no"
# Requires unwind support
GENERIC_SRCS+= \
	gcc_personality_v0.c

# Conflicts with soft-float
GENERIC_SRCS+= \
	comparedf2.c \
	comparesf2.c
.endif

GENERIC_SRCS+= \
	absvdi2.c \
	absvsi2.c \
	absvti2.c \
	adddf3.c \
	addsf3.c \
	addvdi3.c \
	addvsi3.c \
	addvti3.c \
	ashldi3.c \
	ashlti3.c \
	ashrdi3.c \
	ashrti3.c \
	clzdi2.c \
	clzsi2.c \
	clzti2.c \
	cmpdi2.c \
	cmpti2.c \
	ctzdi2.c \
	ctzsi2.c \
	ctzti2.c \
	divdf3.c \
	divdi3.c \
	divmoddi4.c \
	divmodsi4.c \
	divsf3.c \
	divsi3.c \
	divti3.c \
	extendsfdf2.c \
	ffsdi2.c \
	ffsti2.c \
	fixdfdi.c \
	fixdfsi.c \
	fixdfti.c \
	fixsfdi.c \
	fixsfsi.c \
	fixsfti.c \
	fixunsdfdi.c \
	fixunsdfsi.c \
	fixunsdfti.c \
	fixunssfdi.c \
	fixunssfsi.c \
	fixunssfti.c \
	fixunsxfdi.c \
	fixunsxfsi.c \
	fixunsxfti.c \
	fixxfdi.c \
	fixxfti.c \
	floatdidf.c \
	floatdisf.c \
	floatdixf.c \
	floatsidf.c \
	floatsisf.c \
	floattidf.c \
	floattisf.c \
	floattixf.c \
	floatundidf.c \
	floatundisf.c \
	floatundixf.c \
	floatunsidf.c \
	floatunsisf.c \
	floatuntidf.c \
	floatuntisf.c \
	floatuntixf.c \
	int_util.c \
	lshrdi3.c \
	lshrti3.c \
	moddi3.c \
	modsi3.c \
	modti3.c \
	muldc3.c \
	muldf3.c \
	muldi3.c \
	mulodi4.c \
	mulosi4.c \
	muloti4.c \
	mulsf3.c \
	multi3.c \
	mulvdi3.c \
	mulvsi3.c \
	mulvti3.c \
	negdf2.c \
	negdi2.c \
	negsf2.c \
	negti2.c \
	negvdi2.c \
	negvsi2.c \
	negvti2.c \
	paritydi2.c \
	paritysi2.c \
	parityti2.c \
	popcountdi2.c \
	popcountsi2.c \
	popcountti2.c \
	powidf2.c \
	powisf2.c \
	powitf2.c \
	powixf2.c \
	subdf3.c \
	subsf3.c \
	subvdi3.c \
	subvsi3.c \
	subvti3.c \
	truncdfsf2.c \
	ucmpdi2.c \
	ucmpti2.c \
	udivdi3.c \
	udivmoddi4.c \
	udivmodsi4.c \
	udivmodti4.c \
	udivsi3.c \
	udivti3.c \
	umoddi3.c \
	umodsi3.c \
	umodti3.c

.if ${MACHINE_ARCH} == "powerpc"
GENERIC_SRCS+= \
	fixtfdi.c \
	fixunstfdi.c \
	floatditf.c \
	floatunditf.c \
	gcc_qadd.c \
	gcc_qdiv.c \
	gcc_qmul.c \
	gcc_qsub.c
.endif

.if ${MACHINE_CPU} == "arm"
.if !empty(LIBC_MACHINE_ARCH:Mearm*)
GENERIC_SRCS+= \
	aeabi_dcmp.S \
	aeabi_fcmp.S \
	aeabi_idivmod.S \
	aeabi_ldivmod.S \
	aeabi_uidivmod.S \
	aeabi_uldivmod.S
.endif

GENERIC_SRCS+= \
	bswapdi2.S \
	bswapsi2.S
.endif

.for src in ${GENERIC_SRCS}
.  if exists(${COMPILER_RT_CPU_DIR}/${src:R}.S) || \
      exists(${COMPILER_RT_ARCH_DIR}/${src:R}.S)
SRCS+=	${src:R}.S
.  else
SRCS+=	${src}
COPTS.${src}+=	-Wno-missing-prototypes \
		-Wno-old-style-definition \
		-Wno-strict-prototypes \
		-Wno-uninitialized
.  endif
.endfor
