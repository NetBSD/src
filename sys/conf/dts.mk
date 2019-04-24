# $NetBSD: dts.mk,v 1.12 2019/04/24 20:53:10 christos Exp $

DTSARCH?=${MACHINE_CPU}
DTSGNUARCH?=${DTSARCH}
DTSPADDING?=1024

.if !make(obj) && !make(clean) && !make(cleandir)
.BEGIN::
	-@mkdir -p dts
.for _arch in ${DTSGNUARCH}
	-@ln -snf ${S:S@^../@../../@}/external/gpl2/dts/dist/arch/${_arch}/boot/dts dts/${_arch}
.endfor
.endif

DTSINC?=$S/external/gpl2/dts/dist/include
.for _arch in ${DTSARCH}
DTSDIR+=$S/arch/${_arch}/dts
.endfor
.for _arch in ${DTSGNUARCH}
DTSDIR+=$S/external/gpl2/dts/dist/arch/${_arch}/boot/dts
.for _dir in ${DTSSUBDIR}
.if exists($S/external/gpl2/dts/dist/arch/${_arch}/boot/dts/${_dir})
DTSDIR+=$S/external/gpl2/dts/dist/arch/${_arch}/boot/dts/${_dir}
.endif
.endfor
.endfor

DTSPATH=${DTSINC} ${DTSDIR} ${DTS_OVERLAYDIR} dts

.SUFFIXES: .dtd .dtdo .dtb .dtbo .dts

.dts.dtd:
	(${CPP} -P -xassembler-with-cpp ${DTSPATH:@v@-I ${v}@} \
	    -include ${.IMPSRC} /dev/null | \
	${TOOL_DTC} ${DTSPATH:@v@-i ${v}@} -I dts -O dtb \
	    -p ${DTSPADDING} -b 0 -@ -o /dev/null -d /dev/stdout | \
	${TOOL_SED} -e 's@/dev/null@${.TARGET:.dtd=.dtb}@' \
	    -e 's@<stdin>@${.IMPSRC}@' && \
	${CPP} -P -xassembler-with-cpp ${DTSPATH:@v@-I ${v}@} \
	    -include ${.IMPSRC} -M /dev/null | \
	${TOOL_SED} -e 's@null.o@${.TARGET:.dtd=.dtb}@' \
	    -e 's@/dev/null@@') > ${.TARGET}

.dts.dtdo:
	(${CPP} -P -xassembler-with-cpp ${DTSPATH:@v@-I ${v}@} \
	    -include ${.IMPSRC} /dev/null | \
	${TOOL_DTC} ${DTSPATH:@v@-i ${v}@} -I dts -O dtb \
	    -@ -o /dev/null -d /dev/stdout | \
	${TOOL_SED} -e 's@/dev/null@${.TARGET:.dtdo=.dtbo}@' \
	    -e 's@<stdin>@${.IMPSRC}@' && \
	${CPP} -P -xassembler-with-cpp ${DTSPATH:@v@-I ${v}@} \
	    -include ${.IMPSRC} -M /dev/null | \
	${TOOL_SED} -e 's@null.o@${.TARGET:.dtdo=.dtbo}@' \
	    -e 's@/dev/null@@') > ${.TARGET}

.dts.dtb:
	${CPP} -P -xassembler-with-cpp ${DTSPATH:@v@-I ${v}@} \
	    -include ${.IMPSRC} /dev/null | \
	${TOOL_DTC} ${DTSPATH:@v@-i ${v}@} -I dts -O dtb \
	    -p ${DTSPADDING} -b 0 -@ -o ${.TARGET}

.dts.dtbo:
	${CPP} -P -xassembler-with-cpp ${DTSPATH:@v@-I ${v}@} \
	    -include ${.IMPSRC} /dev/null | \
	${TOOL_DTC} ${DTSPATH:@v@-i ${v}@} -I dts -O dtb \
	    -@ -o ${.TARGET}

.PATH.dts: ${DTSDIR} ${DTS_OVERLAYDIR}

DEPS+= ${DTS:.dts=.dtd}
DEPS+= ${DTS_OVERLAYS:.dts=.dtdo}
DTB=  ${DTS:.dts=.dtb}
DTBO= ${DTS_OVERLAYS:.dts=.dtbo}

all: ${DTB} ${DTBO}
