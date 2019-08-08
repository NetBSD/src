.SUFFIXES: .a .syms
.a.syms:
	${SCRIPT_ENV} \
		NM=${NM} \
		AWK=${AWK} \
		FILE=${FILE} \
		${HOST_SH} \
		${.PARSEDIR}/gen_dynamic_list.sh \
		${.IMPSRC} > ${.TARGET}
