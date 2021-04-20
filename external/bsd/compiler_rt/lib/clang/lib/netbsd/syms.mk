#	$NetBSD: syms.mk,v 1.4 2021/04/20 23:19:53 rillig Exp $

here := ${.PARSEDIR}

.if !make(install)		# allow both .a and .a.syms to be installed
.SUFFIXES: .a .a.syms
.a.a.syms:
	${_MKTARGET_CREATE}
	NM=${NM:Q} AWK=${TOOL_AWK:Q} FILE=${TOOL_MKMAGIC:Q} \
		${HOST_SH} \
		${here}/gen_dynamic_list.sh \
		--extra ${SYMS_EXTRA:Q} \
		${.IMPSRC} > ${.TARGET}
.endif
