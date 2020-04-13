#	$NetBSD: syms.mk,v 1.3.4.2 2020/04/13 07:45:52 martin Exp $

here := ${.PARSEDIR}

.SUFFIXES: .a .a.syms
.a.a.syms:
	${_MKTARGET_CREATE}
	NM=${NM:Q} AWK=${TOOL_AWK:Q} FILE=${TOOL_MKMAGIC:Q} \
		${HOST_SH} \
		${here}/gen_dynamic_list.sh \
		--extra ${SYMS_EXTRA:Q} \
		${.IMPSRC} > ${.TARGET}
