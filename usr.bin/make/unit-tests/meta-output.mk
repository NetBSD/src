#

.MAIN: all

.if make(output)
.MAKE.MODE= meta curDirOk=true nofilemon
.else
.MAKE.MODE= compat
.endif

all: output.-B output.-j1

_mf := ${.PARSEDIR}/${.PARSEFILE}

# this output should be accurately reflected in the .meta file
output: .NOPATH
	@{ echo Test ${tag} output; \
	for i in 1 2 3; do \
	printf "test$$i:  "; sleep 0; echo " Done"; \
	done; }

output.-B output.-j1:
	@{ rm -f ${TMPDIR}/output; mkdir -p ${TMPDIR}/obj; \
	MAKEFLAGS= ${.MAKE} -r -C ${TMPDIR} ${.TARGET:E} tag=${.TARGET:E} -f ${_mf} output; \
	sed '1,/^TARGET/d' ${TMPDIR}/obj/output.meta; \
	}
