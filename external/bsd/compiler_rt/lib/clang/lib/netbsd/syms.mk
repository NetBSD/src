#	$NetBSD: syms.mk,v 1.1.2.3 2019/09/01 10:36:26 martin Exp $

here := ${.PARSEDIR}

.SUFFIXES: .a .a.syms
.a.a.syms:
	${_MKTARGET_CREATE}
	NM=${NM:Q} AWK=${TOOL_AWK:Q} FILE=${TOOL_MKMAGIC:Q} \
		${HOST_SH} \
		${here}/gen_dynamic_list.sh \
		${.IMPSRC} > ${.TARGET}
