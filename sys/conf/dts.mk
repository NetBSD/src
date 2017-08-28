# $NetBSD: dts.mk,v 1.2.6.2 2017/08/28 17:52:00 skrll Exp $

DTSINC?=$S/external/gpl2/dts/dist/include
DTSGNUPATH?=$S/external/gpl2/dts/dist/arch/${MACHINE_CPU}/boot/dts
DTSPATH?=$S/arch/${MACHINE_CPU}/dts
DTSPADDING?=1024

.SUFFIXES: .dtd .dtb .dts

.dts.dtd:
	(${CPP} -P -xassembler-with-cpp -I ${DTSINC} -I ${DTSPATH} \
	    -I ${DTSGNUPATH} -include ${.IMPSRC} /dev/null | \
	${TOOL_DTC} -i ${DTSINC} -i ${DTSPATH} -i ${DTSGNUPATH} -I dts -O dtb \
	    -p ${DTSPADDING} -b 0 -o /dev/null -d /dev/stdout | \
	${TOOL_SED} -e 's@/dev/null@${.TARGET:.dtd=.dtb}@' \
	    -e 's@<stdin>@${.IMPSRC}@' && \
	${CPP} -P -xassembler-with-cpp -I ${DTSINC} -I ${DTSPATH} \
	    -I ${DTSGNUPATH} -include ${.IMPSRC} -M /dev/null | \
	${TOOL_SED} -e 's@null.o@${.TARGET:.dtd=.dtb}@' \
	    -e 's@/dev/null@@') > ${.TARGET}


.dts.dtb:
	${CPP} -P -xassembler-with-cpp -I ${DTSINC} -I ${DTSPATH} \
	    -I ${DTSGNUPATH} -include ${.IMPSRC} /dev/null | \
	${TOOL_DTC} -i ${DTSINC} -i ${DTSPATH} -i ${DTSGNUPATH} -I dts -O dtb \
	    -p ${DTSPADDING} -b 0 -o ${.TARGET}

.PATH.dts: ${DTSPATH} ${DTSGNUPATH}

DEPS+= ${DTS:.dts=.dtd}
DTB= ${DTS:.dts=.dtb}

all: ${DTB}
