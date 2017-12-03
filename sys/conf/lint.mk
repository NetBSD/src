# $NetBSD: lint.mk,v 1.1.18.2 2017/12/03 11:36:57 jdolecek Exp $

##
## lint
##

.if !target(lint)
ALLSFILES?=	${MD_SFILES} ${SFILES}
LINTSTUBS?=	${ALLSFILES:T:R:C/^.*$/LintStub_&.c/g}
KERNLINTFLAGS?=	-bcehnxzFS
NORMAL_LN?=	${LINT} ${KERNLINTFLAGS} ${CPPFLAGS:M-[IDU]*} -i $< -o $@

_lsrc=${CFILES} ${LINTSTUBS} ${MI_CFILES} ${MD_CFILES}
LOBJS?= ${_lsrc:T:S/.c$/.ln/g} ${LIBKERNLN} ${SYSLIBCOMPATLN}

.for _sfile in ${ALLSFILES}
LintStub_${_sfile:T:R}.c: ${_sfile} assym.h
	${_MKTARGET_COMPILE}
	${CC} -E -C ${AFLAGS} ${CPPFLAGS} ${_sfile} | \
	      ${TOOL_AWK} -f $S/kern/genlintstub.awk >${.TARGET}
.endfor

.for _cfile in ${CFILES} ${LINTSTUBS} ${MI_CFILES} ${MD_CFILES}
${_cfile:T:R}.ln: ${_cfile}
	${_MKTARGET_COMPILE}
	${NORMAL_LN}
.endfor

lint: ${LOBJS}
	${LINT} ${KERNLINTFLAGS} ${CPPFLAGS:M-[IDU]*} ${LOBJS}
.endif

# XXX who uses this?
# Attempt to do a syntax-only compile of the entire kernel as one entity.
# Alas, bugs in the GCC C frontend prevent this from completely effective
# but information can be gleaned from the output.
syntax-only: ${CFILES} ${MD_CFILES}
	${CC} -fsyntax-only -combine ${CFLAGS} ${CPPFLAGS} \
		${CFILES} ${MD_CFILES}
