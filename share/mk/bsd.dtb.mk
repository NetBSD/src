#	$NetBSD: bsd.dtb.mk,v 1.3 2021/06/02 10:28:21 jmcneill Exp $

.include <bsd.init.mk>
.include <bsd.own.mk>

##### Default values
.if !defined(S)
.   if defined(NETBSDSRCDIR)
S=      ${NETBSDSRCDIR}/sys
.   elif defined(BSDSRCDIR)
S=      ${BSDSRCDIR}/sys
.   else
S=      /sys
.   endif
.endif

##### Basic targets
.PHONY:		dtbinstall dtblist dtb
realinstall:	dtbinstall
realall:	dtb

DTSPADDING?=	1024

.if !make(obj) && !make(clean) && !make(cleandir)
.BEGIN::
	-@mkdir -p ${.OBJDIR}/dts
.for _arch in ${DTSGNUARCH}
	-@ln -snf ${S:S@^../@../../@}/external/gpl2/dts/dist/arch/${_arch}/boot/dts ${.OBJDIR}/dts/${_arch}
.endfor
.endif

DTSINC?=$S/external/gpl2/dts/dist/include
.for _arch in ${DTSARCH}
DTSDIR+=$S/arch/${_arch}/dts
.endfor
.for _arch in ${DTSGNUARCH}
DTSDIR+=$S/external/gpl2/dts/dist/arch/${_arch}/boot/dts
.if defined(DTSSUBDIR)
DTSDIR+=$S/external/gpl2/dts/dist/arch/${_arch}/boot/dts/${DTSSUBDIR}
.endif
.endfor

DTSPATH=${DTSINC} ${DTSDIR} ${.OBJDIR}/dts

.SUFFIXES: .dtb .dts

.dts.dtb:
	${CPP} -P -xassembler-with-cpp ${DTSPATH:@v@-I ${v}@} \
	    -include ${.IMPSRC} /dev/null | \
	${TOOL_DTC} ${DTSPATH:@v@-i ${v}@} -I dts -O dtb \
	    -p ${DTSPADDING} -b 0 -@ -o ${.TARGET}

.PATH.dts: ${DTSDIR}

DTB= 		 ${DTS:.dts=.dtb}

dtb:		${DTB}

.if defined(DTSSUBDIR)
DTBINSTDIR=	${DTBDIR}/${DTSSUBDIR}
.else
DTBINSTDIR=	${DTBDIR}
.endif

dtbinstall:	dtb
	${INSTALL_DIR} ${DESTDIR}${DTBINSTDIR}
.for _dtb in ${DTB}
	${_MKSHMSG_INSTALL} ${_dtb}
	${_MKSHECHO} "${INSTALL_FILE} -o ${DTBOWN} -g ${DTBGRP} -m ${DTBMODE} \
	    ${.OBJDIR}/${_dtb} ${DESTDIR}${DTBINSTDIR}"
	${INSTALL_FILE} -o ${DTBOWN} -g ${DTBGRP} -m ${DTBMODE} \
	    ${.OBJDIR}/${_dtb} ${DESTDIR}${DTBINSTDIR}
.endfor
.if defined(DTSSUBDIR)
.for _dtb in ${DTB_NOSUBDIR}
	${_MKSHMSG_INSTALL} ${_dtb}
	${_MKSHECHO} "${INSTALL_FILE} -o ${DTBOWN} -g ${DTBGRP} -m ${DTBMODE} \
	    ${.OBJDIR}/${_dtb} ${DESTDIR}${DTBDIR}"
	${INSTALL_FILE} -o ${DTBOWN} -g ${DTBGRP} -m ${DTBMODE} \
	    ${.OBJDIR}/${_dtb} ${DESTDIR}${DTBDIR}
.endfor
.endif

dtblist:
.if defined(DTSSUBDIR)
	@echo ".${DTBINSTDIR}\t\tdtb-base-boot\tdtb" | \
	    ${TOOL_SED} 's/\\t/	/g'
.for _dtb in ${DTB_NOSUBDIR}
	@echo ".${DTBDIR}/${_dtb}\t\tdtb-base-boot\tdtb" | \
	    ${TOOL_SED} 's/\\t/	/g'
.endfor
.endif
.for _dtb in ${DTB}
	@echo ".${DTBINSTDIR}/${_dtb}\t\tdtb-base-boot\tdtb" | \
	    ${TOOL_SED} 's/\\t/	/g'
.endfor

clean:  .PHONY
	rm -f ${DTB}
.for _arch in ${DTSGNUARCH}
	rm -f dts/${_arch}
.endfor
	test -d dts && rmdir dts || true


##### Pull in related .mk logic
.include <bsd.obj.mk>
.include <bsd.kinc.mk>
