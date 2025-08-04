#

.MAIN: all

.if make(output)
.MAKE.MODE= meta curDirOk=true nofilemon
x!= echo -n ok; echo
.if ${x:M-n} == ""
ECHO_SCRIPT= Echo() { echo "$$@"; }
.else
ECHO_SCRIPT= Echo() { case "$$1" in -n) shift; echo "$$@\c";; \
	*) echo "$$@";; esac; }
.endif
.else
.MAKE.MODE= compat
.endif

all: output.-B output.-j1

_mf := ${.PARSEDIR}/${.PARSEFILE}

# this output should be accurately reflected in the .meta file
output: .NOPATH
	@${ECHO_SCRIPT}; { echo Test ${tag} output; \
	for i in 1 2 3; do \
	Echo -n "test$$i:  "; sleep 0; echo " Done"; \
	done; }

output.-B output.-j1:
	@{ rm -f ${TMPDIR}/output; mkdir -p ${TMPDIR}/obj; \
	MAKEFLAGS= ${.MAKE} -r -C ${TMPDIR} ${.TARGET:E} tag=${.TARGET:E} -f ${_mf} output; \
	sed '1,/^TARGET/d' ${TMPDIR}/obj/output.meta; \
	}
