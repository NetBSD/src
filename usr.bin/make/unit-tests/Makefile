# $NetBSD: Makefile,v 1.92 2020/08/15 01:49:07 rillig Exp $
#
# Unit tests for make(1)
#
# The main targets are:
#
# all:
#	run all the tests
# test:
#	run 'all', and compare to expected results
# accept:
#	move generated output to expected results
#
# Settable variables
#
# TEST_MAKE
#	The make program to be tested.
#
#
# Adding a test case
#
# Each feature should get its own set of tests in its own suitably
# named makefile (*.mk), with its own set of expected results (*.exp),
# and it should be added to the TESTS list.
#
# Any added files must also be added to src/distrib/sets/lists/tests/mi.
# To do that, just run "make sync-mi" in this directory.
#
# A few *.mk files are helper files for other tests (such as include-sub.mk)
# and are thus not added to TESTS.  Such files must be ignored in
# src/tests/usr.bin/make/t_make.sh.
#

# Each test is in a sub-makefile.
# Keep the list sorted.
# Any test that is commented out must be ignored in
# src/tests/usr.bin/make/t_make.sh as well.
TESTS+=		# archive	# broken on FreeBSD, enabled in t_make.sh
TESTS+=		cmdline
TESTS+=		comment
TESTS+=		cond-late
TESTS+=		cond-short
TESTS+=		cond1
TESTS+=		cond2
TESTS+=		counter
TESTS+=		dir
TESTS+=		directives
TESTS+=		dollar
TESTS+=		doterror
TESTS+=		dotwait
TESTS+=		envfirst
TESTS+=		error
TESTS+=		# escape	# broken by reverting POSIX changes
TESTS+=		export
TESTS+=		export-all
TESTS+=		export-env
TESTS+=		export-variants
TESTS+=		forloop
TESTS+=		forsubst
TESTS+=		hash
TESTS+=		impsrc
TESTS+=		include-main
TESTS+=		lint
TESTS+=		make-exported
TESTS+=		misc
TESTS+=		moderrs
TESTS+=		modmatch
TESTS+=		modmisc
TESTS+=		modorder
TESTS+=		modts
TESTS+=		modword
TESTS+=		order
TESTS+=		phony-end
TESTS+=		posix
TESTS+=		# posix1	# broken by reverting POSIX changes
TESTS+=		qequals
TESTS+=		recursive
TESTS+=		# suffixes	# runs into an endless loop (try -dA)
TESTS+=		sunshcmd
TESTS+=		sysv
TESTS+=		ternary
TESTS+=		unexport
TESTS+=		unexport-env
TESTS+=		use-inference
TESTS+=		varcmd
TESTS+=		vardebug
TESTS+=		varfind
TESTS+=		varmisc
TESTS+=		varmod-edge
TESTS+=		varparse-dynamic
TESTS+=		varquote
TESTS+=		varshell

# Override environment variables for some of the tests.
ENV.counter=		-i
ENV.envfirst=		FROM_ENV=value-from-env
ENV.export=		-i PATH=${PATH:Q}
ENV.export-variants=	-i PATH=${PATH:Q}
ENV.lint=		-i
ENV.make-exported=	-i PATH=${PATH:Q}
ENV.recursive=		-i
ENV.varmisc=		FROM_ENV=env
ENV.varmisc+=		FROM_ENV_BEFORE=env
ENV.varmisc+=		FROM_ENV_AFTER=env

# Override make flags for some of the tests; default is -k.
FLAGS.counter=		-r -dv
FLAGS.doterror=		# none
FLAGS.envfirst=		-e
FLAGS.export=		-r
FLAGS.lint=		-dL -k
FLAGS.order=		-j1
FLAGS.recursive=	-dL -r
FLAGS.vardebug=		-k -dv FROM_CMDLINE=

# Some tests need extra post-processing.
SED_CMDS.moderrs+=	-e 's,\(Regex compilation error:\).*,\1 (details omitted),'
SED_CMDS.modmisc+=	-e 's,\(Regex compilation error:\).*,\1 (details omitted),'
SED_CMDS.varmod-edge+=	-e 's, line [0-9]*:, line omitted:,'
SED_CMDS.varshell+=	-e 's,^[a-z]*sh: ,,'
SED_CMDS.varshell+=	-e '/command/s,No such.*,not found,'

# Some tests need an additional round of postprocessing.
POSTPROC.counter=	${TOOL_SED} -n -e '/:RELEVANT = yes/,/:RELEVANT = no/p'
POSTPROC.vardebug=	${TOOL_SED} -n -e '/:RELEVANT = yes/,/:RELEVANT = no/p'

export-all.rawout: export.mk
unexport.rawout: export.mk
unexport-env.rawout: export.mk

# End of the configuration section.

.MAIN: all

UNIT_TESTS:=	${.PARSEDIR}
.PATH: ${UNIT_TESTS}

OUTFILES=	${TESTS:=.out}

all: ${OUTFILES}

CLEANFILES+=		*.rawout *.out *.status *.tmp *.core *.tmp
CLEANFILES+=		obj*.[och] lib*.a	# posix1.mk
CLEANFILES+=		issue* .[ab]*		# suffixes.mk
CLEANRECURSIVE+=	dir dummy		# posix1.mk

clean:
	rm -f ${CLEANFILES}
.if !empty(CLEANRECURSIVE)
	rm -rf ${CLEANRECURSIVE}
.endif

TEST_MAKE?=	${.MAKE}
TOOL_SED?=	sed

# ensure consistent results from sort(1)
LC_ALL=		C
LANG=		C
.export LANG LC_ALL

# the tests are actually done with sub-makes.
.SUFFIXES: .mk .rawout .out
.mk.rawout:
	@echo testing ${.IMPSRC}
	@set -eu; \
	cd ${.OBJDIR}; \
	env ${ENV.${.TARGET:R}} ${TEST_MAKE} -C ${.CURDIR} \
	  ${FLAGS.${.TARGET:R}:U-k} -f ${.IMPSRC} \
	  > ${.TARGET}.tmp 2>&1 \
	&& status=$$? || status=$$?; \
	echo $$status > ${.TARGET:R}.status
	@mv ${.TARGET}.tmp ${.TARGET}

# Post-process the test output so that the results can be compared.
#
# always pretend .MAKE was called 'make'
_SED_CMDS+=	-e 's,^${TEST_MAKE:T:S,.,\\.,g}[][0-9]*:,make:,'
_SED_CMDS+=	-e 's,${TEST_MAKE:S,.,\\.,g},make,'
# replace anything after 'stopped in' with unit-tests
_SED_CMDS+=	-e '/stopped/s, /.*, unit-tests,'
# strip ${.CURDIR}/ from the output
_SED_CMDS+=	-e 's,${.CURDIR:S,.,\\.,g}/,,g'
_SED_CMDS+=	-e 's,${UNIT_TESTS:S,.,\\.,g}/,,g'

.rawout.out:
	@echo postprocess ${.TARGET}
	@${TOOL_SED} ${_SED_CMDS} ${SED_CMDS.${.TARGET:R}} \
	  < ${.IMPSRC} > ${.TARGET}.tmp1
	@${POSTPROC.${.TARGET:R}:Ucat} < ${.TARGET}.tmp1 > ${.TARGET}.tmp2
	@rm ${.TARGET}.tmp1
	@echo "exit status `cat ${.TARGET:R}.status`" >> ${.TARGET}.tmp2
	@mv ${.TARGET}.tmp2 ${.TARGET}

# Compare all output files
test:	${OUTFILES} .PHONY
	@failed= ; \
	for test in ${TESTS}; do \
	  diff -u ${UNIT_TESTS}/$${test}.exp $${test}.out \
	  || failed="$${failed}$${failed:+ }$${test}" ; \
	done ; \
	if [ -n "$${failed}" ]; then \
	  echo "Failed tests: $${failed}" ; false ; \
	else \
	  echo "All tests passed" ; \
	fi

accept:
	@for test in ${TESTS}; do \
	  cmp -s ${UNIT_TESTS}/$${test}.exp $${test}.out \
	  || { echo "Replacing $${test}.exp" ; \
	       cp $${test}.out ${UNIT_TESTS}/$${test}.exp ; } \
	done

SYNC_MI_AWK= \
	BEGIN {								\
	  testsdir = "usr.bin/make/unit-tests";				\
	  linestart = "./usr/tests/" testsdir;				\
	  fmt = linestart "/%s\ttests-usr.bin-tests\tcompattestfile,atf\\n"; \
	  cmd = "cd " testsdir " && ls *.exp *.mk | xargs printf '" fmt "'" \
	}								\
	function startswith(s, prefix) {				\
	  return substr(s, 1, length(prefix)) == prefix;		\
	}								\
	startswith($$1, linestart) && $$1 ~ /\.(exp|mk)$$/ { next }	\
	{ print $$0 }							\
	$$1 == linestart "/Makefile" { system(cmd) }

sync-mi:
	@set -eu;							\
	cd "${MAKEFILE:tA:H}/../../..";					\
	mi="distrib/sets/lists/tests/mi";				\
	awk ${SYNC_MI_AWK:Q} < "$$mi" > "$$mi.$@";			\
	mv -f "$$mi.$@" "$$mi";						\
	cvs diff "$$mi" || true

.if exists(${TEST_MAKE})
${TESTS:=.rawout}: ${TEST_MAKE}
.endif

.-include <bsd.obj.mk>
