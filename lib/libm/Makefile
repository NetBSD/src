#	from: @(#)Makefile	5.11 (Berkeley) 12/2/92
#	$Id: Makefile,v 1.10 1993/12/06 12:11:42 cgd Exp $
#
# ieee		- for most IEEE machines, we hope.
# mc68881	- the, ahem, mc68881.
# national	- NATIONAL 32016 with 32081.
# i386		- i387 NPX
# mips		- for MIPS achitecture machines (and also pmax)
# tahoe		- for the tahoe double format.
# vax		- for the vax D_floating format

LIB=	m
CFLAGS+=-I${.CURDIR}/common_source
NOPIC=	nopic

.if (${MACHINE_ARCH} == "ieee")

HARDWARE=${MACHINE_ARCH}
.PATH:	${.CURDIR}/common_source ${.CURDIR}/common ${.CURDIR}/ieee
# common_source
SRCS+=	acosh.c asincos.c asinh.c atan.c atanh.c cosh.c erf.c exp.c exp__D.c \
	exp__E.c expm1.c floor.c fmod.c gamma.c lgamma.c j0.c j1.c jn.c log.c \
	log10.c log1p.c log__D.c log__L.c logtab.c pow.c sinh.c tanh.c
# common
SRCS+=	atan2.c sincos.c tan.c
# ieee
SRCS+=	cabs.c cbrt.c support.c

.elif (${MACHINE_ARCH} == "m68k")

HARDWARE=mc68881
.PATH:	${.CURDIR}/mc68881 ${.CURDIR}/common_source ${.CURDIR}/ieee
# common_source
SRCS+=	acosh.c asinh.c erf.c exp__D.c exp__E.c fmod.c gamma.c lgamma.c j0.c \
	j1.c log__D.c log__L.c logtab.c pow.c
# mc68881
SRCS+=	asincos.S atan.S atan2.c atanh.S cosh.S exp.S expm1.S floor.S \
	log.S log10.S log1p.S sincos.S sinh.S sqrt.S support.S tan.S tanh.S
# ieee
SRCS+=	cabs.c cbrt.c

.elif (${MACHINE_ARCH} == "i386")

HARDWARE=i387
.PATH:	${.CURDIR}/common_source ${.CURDIR}/common ${.CURDIR}/ieee
# common_source
SRCS+=	acosh.c asincos.c asinh.c atan.c atanh.c cosh.c erf.c exp.c exp__D.c \
	exp__E.c expm1.c floor.c fmod.c gamma.c lgamma.c j0.c j1.c jn.c log.c \
	log10.c log1p.c log__D.c log__L.c logtab.c pow.c sinh.c tanh.c
# common
SRCS+=	atan2.c sincos.c tan.c
# ieee
SRCS+=	cabs.c cbrt.c support.c

.elif (${MACHINE_ARCH} == "mips" || ${MACHINE_ARCH} == "pmax")

HARDWARE=${MACHINE_ARCH}
.PATH:	${.CURDIR}/common_source ${.CURDIR}/common ${.CURDIR}/ieee
# common_source
SRCS+=	acosh.c asincos.c asinh.c atan.c atanh.c cosh.c erf.c exp.c exp__D.c \
	exp__E.c expm1.c floor.c fmod.c gamma.c lgamma.c j0.c j1.c jn.c log.c \
	log10.c log1p.c log__D.c log__L.c logtab.c pow.c sinh.c tanh.c
# common
SRCS+=	atan2.c sincos.c tan.c
# ieee
SRCS+=	cabs.c cbrt.c support.c

.elif (${MACHINE_ARCH} == "national" || ${MACHINE_ARCH} == "ns32k")
HARDWARE=national
.PATH:	${.CURDIR}/common_source ${.CURDIR}/common ${.CURDIR}/national \
	${.CURDIR}/ieee
# common_source
SRCS+=	acosh.c asincos.c asinh.c atan.c atanh.c cosh.c erf.c exp.c exp__D.c \
	exp__E.c expm1.c floor.c fmod.c gamma.c lgamma.c j0.c j1.c jn.c log.c \
	log10.c log1p.c log__D.c log__L.c logtab.c pow.c sinh.c tanh.c
# common
SRCS+=	atan2.c sincos.c tan.c
# national
SRCS+=	sqrt.S support.S
# ieee
SRCS+=	cabs.c cbrt.c

.elif (${MACHINE_ARCH} == "sparc")

HARDWARE=${MACHINE_ARCH}
.PATH:  ${.CURDIR}/common_source ${.CURDIR}/common ${.CURDIR}/ieee
# common_source
SRCS+=	acosh.c asincos.c asinh.c atan.c atanh.c cosh.c erf.c exp.c exp__D.c \
	exp__E.c expm1.c floor.c fmod.c gamma.c lgamma.c j0.c j1.c jn.c log.c \
	log10.c log1p.c log__D.c log__L.c logtab.c pow.c sinh.c tanh.c
# XXX should do sqrt & support functions in assembly
# common
SRCS+=	atan2.c sincos.c tan.c
# ieee
SRCS+=	cabs.c cbrt.c support.c

.elif (${MACHINE_ARCH} == "tahoe")

HARDWARE=${MACHINE_ARCH}
.PATH:	${.CURDIR}/common_source ${.CURDIR}/common ${.CURDIR}/tahoe \
# common_source
SRCS+=	acosh.c asincos.c asinh.c atan.c atanh.c cosh.c erf.c exp.c exp__D.c \
	exp__E.c expm1.c floor.c fmod.c gamma.c lgamma.c j0.c j1.c jn.c log.c \
	log10.c log1p.c log__D.c log__L.c logtab.c pow.c sinh.c tanh.c
# common
SRCS+=	atan2.c sincos.c tan.c
# tahoe
SRCS+=	cabs.S cbrt.S sqrt.S support.S infnan.S

.elif (${MACHINE_ARCH} == "vax")

HARDWARE=${MACHINE_ARCH}
.PATH:	${.CURDIR}/common_source ${.CURDIR}/vax
# common_source
SRCS+=	acosh.c asincos.c asinh.c atan.c atanh.c cosh.c erf.c exp.c exp__D.c \
	exp__E.c expm1.c floor.c fmod.c gamma.c lgamma.c j0.c j1.c jn.c log.c \
	log10.c log1p.c log__D.c log__L.c logtab.c pow.c sinh.c tanh.c
# vax
SRCS+=	atan2.S cabs.S cbrt.S sqrt.S sincos.S tan.S argred.S support.S \
	infnan.S

.endif

MAN3+=	acos.0 acosh.0 asin.0 asinh.0 atan.0 atan2.0 atanh.0 ceil.0 \
	cos.0 cosh.0 erf.0 exp.0 fabs.0 floor.0 fmod.0 hypot.0 ieee.0 \
	infnan.0 j0.0 lgamma.0 math.0 rint.0 sin.0 sinh.0 sqrt.0 \
	tan.0 tanh.0

MLINKS+=erf.3 erfc.3
MLINKS+=exp.3 expm1.3 exp.3 log.3 exp.3 log10.3 exp.3 log1p.3 exp.3 pow.3
MLINKS+=hypot.3 cabs.3
MLINKS+=ieee.3 copysign.3 ieee.3 drem.3 ieee.3 finite.3 ieee.3 logb.3 \
	ieee.3 scalb.3
MLINKS+=j0.3 j1.3 j0.3 jn.3 j0.3 y0.3 j0.3 y1.3 j0.3 yn.3
MLINKS+=lgamma.3 gamma.3
MLINKS+=sqrt.3 cbrt.3

# can't use the standard mkdep, because there are some .S files that
# are using '#' as a comment indicator and cpp thinks it's an undefined
# control.

depend: .depend
.depend: ${SRCS}
	mkdep ${CFLAGS:M-[ID]*} ${.ALLSRC:M*.c}

.include <bsd.lib.mk>

.S.o:
	${AS} -o ${.TARGET} ${.IMPSRC}
	@${LD} -x -r ${.TARGET}
	@mv a.out ${.TARGET}

.S.po:
	sed -f ${.CURDIR}/${HARDWARE}/mcount.sed ${.IMPSRC} | \
	    ${AS} -o ${.TARGET}
	@${LD} -X -r ${.TARGET}
	@mv a.out ${.TARGET}
