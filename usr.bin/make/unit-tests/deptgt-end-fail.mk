# $NetBSD: deptgt-end-fail.mk,v 1.5 2020/12/07 00:53:30 rillig Exp $
#
# Tests for an errors in the main target, its dependencies,
# the .END node and its dependencies.
#
# Before 2020-11-25, an error in the .END target did not print the "Stop.",
# even though this was intended.  The cause for this was a missing condition
# in Compat_Run, in the code handling the .END node.

test: .PHONY

# The default stop-on-error mode is not as interesting to test since it
# stops right after the first error.
.MAKEFLAGS: -k

.for all in ok ERR
.  for all-dep in ok ERR
.    for end in ok ERR
.      for end-dep in ok ERR
.        for target in ${all}-${all-dep}-${end}-${end-dep}
test: ${target}
${target}: .PHONY .SILENT
	echo Test case all=${all} all-dep=${all-dep} end=${end} end-dep=${end-dep}.
	${MAKE} -r -f ${MAKEFILE} \
		all=${all} all-dep=${all-dep} \
		end=${end} end-dep=${end-dep} \
		all; \
	echo "exit status $$?"
	echo
	echo
.        endfor
.      endfor
.    endfor
.  endfor
.endfor

.if make(all)

all all-dep end-dep: .PHONY

CMD.ok=		true
CMD.ERR=	false

all: all-dep
	: Making ${.TARGET} from ${.ALLSRC}.
	@${CMD.${all}}

all-dep:
	: Making ${.TARGET} out of nothing.
	@${CMD.${all-dep}}

.END: end-dep
	: Making ${.TARGET} from ${.ALLSRC}.
	@${CMD.${end}}

end-dep:
	: Making ${.TARGET} out of nothing.
	@${CMD.${end-dep}}

.endif

# Until 2020-12-07, several of the test cases printed "`all' not remade
# because of errors.", followed by "exit status 0", which contradicted
# each other.

# XXX: As of 2020-12-06, '.END' is made if 'all' fails, but if a dependency
# of 'all' fails, it is skipped.  This is inconsistent.
