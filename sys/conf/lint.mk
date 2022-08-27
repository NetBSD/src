# $NetBSD: lint.mk,v 1.5 2022/08/27 21:49:33 rillig Exp $

##
## lint
##

.if !target(lint)
.PATH: $S
ALLSFILES?=	${MD_SFILES} ${SFILES}
LINTSTUBS?=	${ALLSFILES:T:R:%=LintStub_%.c}
KERNLINTFLAGS?=	-bceghnxzFS
NORMAL_LN?=	${LINT} ${KERNLINTFLAGS} ${CPPFLAGS:M-[IDU]*} -o $@ -i $<

_lsrc=		${CFILES} ${LINTSTUBS} ${MI_CFILES} ${MD_CFILES}
LOBJS?=		${_lsrc:T:.c=.ln} ${LIBKERNLN} ${SYSLIBCOMPATLN}

.for sfile in ${ALLSFILES}
LintStub_${sfile:T:R}.c: ${sfile} assym.h
	${_MKTARGET_COMPILE}
	${CC} -E -C ${AFLAGS} ${CPPFLAGS} ${sfile} | \
	      ${TOOL_AWK} -f $S/kern/genlintstub.awk >${.TARGET}
.endfor

.for cfile in ${CFILES} ${LINTSTUBS} ${MI_CFILES} ${MD_CFILES}
${cfile:T:R}.ln: ${cfile}
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
