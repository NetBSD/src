# $NetBSD: dts.mk,v 1.8.2.1 2018/06/25 07:25:48 pgoyette Exp $

DTSARCH?=${MACHINE_CPU}
DTSGNUARCH?=${DTSARCH}
DTSPADDING?=1024

.if !make(obj) && !make(clean) && !make(cleandir)
.BEGIN::
	-@mkdir -p dts
.for _arch in ${DTSGNUARCH}
	-@ln -sf ${S:S@^../@../../@}/external/gpl2/dts/dist/arch/${_arch}/boot/dts dts/${_arch}
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

DTSPATH=${DTSINC} ${DTSDIR} dts

.SUFFIXES: .dtd .dtb .dts

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


.dts.dtb:
	${CPP} -P -xassembler-with-cpp ${DTSPATH:@v@-I ${v}@} \
	    -include ${.IMPSRC} /dev/null | \
	${TOOL_DTC} ${DTSPATH:@v@-i ${v}@} -I dts -O dtb \
	    -p ${DTSPADDING} -b 0 -@ -o ${.TARGET}

.PATH.dts: ${DTSDIR}

DEPS+= ${DTS:.dts=.dtd}
DTB= ${DTS:.dts=.dtb}

all: ${DTB}
