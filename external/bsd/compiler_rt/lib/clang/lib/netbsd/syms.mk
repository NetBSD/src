#	$NetBSD: syms.mk,v 1.2 2019/08/26 04:49:45 kamil Exp $

here := ${.PARSEDIR}

.SUFFIXES: .a .a.syms
.a.a.syms:
	${_MKTARGET_CREATE}
	NM=${NM:Q} AWK=${TOOL_AWK:Q} FILE=${TOOL_MKMAGIC:Q} \
		${HOST_SH} \
		${here}/gen_dynamic_list.sh \
		${.IMPSRC} > ${.TARGET}
