#	$NetBSD: syms.mk,v 1.3 2019/08/30 23:36:40 kamil Exp $

here := ${.PARSEDIR}

.SUFFIXES: .a .a.syms
.a.a.syms:
	${_MKTARGET_CREATE}
	NM=${NM:Q} AWK=${TOOL_AWK:Q} FILE=${TOOL_MKMAGIC:Q} \
		${HOST_SH} \
		${here}/gen_dynamic_list.sh \
		--extra ${SYMS_EXTRA:Q} \
		${.IMPSRC} > ${.TARGET}
