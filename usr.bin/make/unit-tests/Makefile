# $Id: Makefile,v 1.2 2003/07/29 06:35:09 sjg Exp $
#
# Unit tests for make(1)
# The main targets are:
# 
# all:	run all the tests
# test:	run 'all', capture output and compare to expected results
# accept: move generated output to expected results
#
# Adding a test case.  
# Each feature should get its own set of tests which should be hooked
# into the 'all' target.
# 

.MAIN: all

all: mod-ts

LIST= one two three
LIST+= four five six

FU_mod-ts = a / b / cool

mod-ts:
	@echo LIST='${LIST}'
	@echo LIST:ts,='${LIST:ts,}'
	@echo LIST:ts/:tu='${LIST:ts/:tu}'
	@echo LIST:tu:ts/='${LIST:tu:ts/}'
	@echo LIST:ts:='${LIST:ts:}'
	@echo LIST:ts='${LIST:ts}'
	@echo LIST:ts:S/two/2/='${LIST:ts:S/two/2/}'
	@echo LIST:S/two/2/:ts='${LIST:S/two/2/:ts}'
	@echo LIST:ts/:S/two/2/='${LIST:ts/:S/two/2/}'
	@echo "LIST:ts\n='${LIST:ts\n}'"
	@echo "LIST:ts\t='${LIST:ts\t}'"
	@echo "LIST:ts\012:tu='${LIST:ts\012:tu}'"
	@echo "LIST:tx='${LIST:tx}'"
	@echo "LIST:ts\a:tu='${LIST:ts\a:tu}'"
	@echo "FU_$@='${FU_${@:ts}:ts}'"
	@echo "FU_$@:ts:T='${FU_${@:ts}:ts:T}' == cool?"

clean:
	rm -f *.out *.fail *.core

.include <bsd.obj.mk>

TEST_MAKE?= ${MAKE}

# here is the driver
test:
	@echo "${TEST_MAKE} -f ${MAKEFILE} > ${.TARGET}.out 2>&1"
	@${TEST_MAKE} -f ${MAKEFILE} 2>&1 | \
	sed 's,^${TEST_MAKE:T}:,make:,' > ${.TARGET}.out || { \
	tail ${.TARGET}.out; mv ${.TARGET}.out ${.TARGET}.fail; exit 1; }
	diff -u ${.CURDIR}/${.TARGET}.exp ${.TARGET}.out

accept:
	mv test.out ${.CURDIR}/test.exp

