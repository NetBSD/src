PROG=	vndcompress
SRCS=	main.c offtab.c utils.c vndcompress.c vnduncompress.c

LINKS=	${BINDIR}/vndcompress ${BINDIR}/vnduncompress
MLINKS=	vndcompress.1 vnduncompress.1

DPADD+=	${LIBZ}
LDADD+=	-lz

WARNS=	5

.include <bsd.prog.mk>

TESTFILES+=	oneblock
XFAIL+=		oneblock.in-outx
XFAIL+=		oneblock.cl2-cl2x
oneblock.in:
	head -c 512 < /usr/share/dict/words > ${.TARGET}.tmp \
	&& mv -f ${.TARGET}.tmp ${.TARGET}

TESTFILES+=	tenblock
XFAIL+=		tenblock.in-outx
XFAIL+=		tenblock.cl2-cl2x
tenblock.in:
	head -c 5120 < /usr/share/dict/words > ${.TARGET}.tmp \
	&& mv -f ${.TARGET}.tmp ${.TARGET}

TESTFILES+=	smallfile
XFAIL+=		smallfile.in-outx
XFAIL+=		smallfile.cl2-cl2x
smallfile.in:
	head -c 12345 < /usr/share/dict/words > ${.TARGET}.tmp \
	&& mv -f ${.TARGET}.tmp ${.TARGET}

CHECKS+=	check-pipe
CLEANFILES+=	smallfile.cl2pipe
check-pipe: .PHONY smallfile.cl2 smallfile.cl2pipe
	cmp ${.ALLSRC}
smallfile.cl2pipe: smallfile.in vndcompress
	head -c 54321 < /usr/share/dict/words \
	| ./vndcompress -l 12345 /dev/stdin ${.TARGET}.tmp \
	&& mv -f ${.TARGET}.tmp ${.TARGET}

TESTFILES+=	onechunk
onechunk.in:
	head -c 65536 < /usr/share/dict/words > ${.TARGET}.tmp \
	&& mv -f ${.TARGET}.tmp ${.TARGET}

TESTFILES+=	tenchunk
tenchunk.in:
	head -c 655360 < /usr/share/dict/words > ${.TARGET}.tmp \
	&& mv -f ${.TARGET}.tmp ${.TARGET}

TESTFILES+=	extrablock
XFAIL+=		extrablock.in-outx
XFAIL+=		extrablock.cl2-cl2x
extrablock.in:
	head -c $$((65536 + 512)) < /usr/share/dict/words > ${.TARGET}.tmp \
	&& mv -f ${.TARGET}.tmp ${.TARGET}

TESTFILES+=	medfile
XFAIL+=		medfile.in-outx
XFAIL+=		medfile.cl2-cl2x
medfile.in:
	head -c 123456 < /usr/share/dict/words > ${.TARGET}.tmp \
	&& mv -f ${.TARGET}.tmp ${.TARGET}

TESTFILES+=		onetinyblock
BLOCKSIZE.onetinyblock=	512
onetinyblock.in:
	head -c 512 < /usr/share/dict/words > ${.TARGET}.tmp \
	&& mv -f ${.TARGET}.tmp ${.TARGET}

TESTFILES+=		tentinyblock
BLOCKSIZE.tentinyblock=	512
tentinyblock.in:
	head -c 5120 < /usr/share/dict/words > ${.TARGET}.tmp \
	&& mv -f ${.TARGET}.tmp ${.TARGET}

# Make sure we can restart from a pipe.
CHECKS+=	check-pipe-restart
CLEANFILES+=	piperestart.in piperestart.in.tmp
CLEANFILES+=	piperestart.cl2 piperestart.cl2.tmp
CLEANFILES+=	piperestart.cl2restart piperestart.cl2restart.tmp
CLEANFILES+=	piperestart.cl2part piperestart.cl2part.tmp
check-pipe-restart: .PHONY piperestart.cl2 piperestart.cl2restart
	cmp ${.ALLSRC}
piperestart.cl2restart: piperestart.cl2part vndcompress
	cp piperestart.cl2part ${.TARGET}.tmp \
	&& head -c 700000 < /usr/share/dict/words \
	| ./vndcompress -l 655360 -k 1 -r -R /dev/stdin ${.TARGET}.tmp \
	&& mv -f ${.TARGET}.tmp ${.TARGET}
# The following rule uses ; and not && on purpose: vndcompress is
# supposed to fail (and it is even OK to interrupt!) so we can restart
# and fill in the rest.
piperestart.cl2part: vndcompress
	head -c 600000 < /usr/share/dict/words \
	| ./vndcompress -l 655360 -k 1 /dev/stdin ${.TARGET}.tmp; \
	mv -f ${.TARGET}.tmp ${.TARGET}
piperestart.in:
	head -c 655360 < /usr/share/dict/words > ${.TARGET}.tmp \
	&& mv -f ${.TARGET}.tmp ${.TARGET}

# Make sure we can restart from a pipe even if the original start was
# corrupted, as long as we don't pass -R.
CHECKS+=	check-pipe-badstart
CLEANFILES+=	pipebadstart.in pipebadstart.in.tmp
CLEANFILES+=	pipebadstart.cl2 pipebadstart.cl2.tmp
CLEANFILES+=	pipebadstart.cl2restart pipebadstart.cl2restart.tmp
CLEANFILES+=	pipebadstart.cl2part pipebadstart.cl2part.tmp
check-pipe-badstart: .PHONY pipebadstart.cl2 pipebadstart.cl2restart
	cmp ${.ALLSRC}
pipebadstart.cl2restart: pipebadstart.cl2part vndcompress
	cp pipebadstart.cl2part ${.TARGET}.tmp \
	&& head -c 700000 < /usr/share/dict/words \
	| ./vndcompress -l 655360 -k 1 -r /dev/stdin ${.TARGET}.tmp \
	&& mv -f ${.TARGET}.tmp ${.TARGET}
pipebadstart.cl2part:
	touch ${.TARGET}
pipebadstart.in:
	head -c 655360 < /usr/share/dict/words > ${.TARGET}.tmp \
	&& mv -f ${.TARGET}.tmp ${.TARGET}

# Make sure we can `restart' even if there's nothing there.
CHECKS+=	check-pipe-falsestart
CLEANFILES+=	pipefalsestart.in pipefalsestart.in.tmp
CLEANFILES+=	pipefalsestart.cl2 pipefalsestart.cl2.tmp
CLEANFILES+=	pipefalsestart.cl2restart pipefalsestart.cl2restart.tmp
check-pipe-falsestart: .PHONY pipefalsestart.cl2 pipefalsestart.cl2restart
	cmp ${.ALLSRC}
pipefalsestart.cl2restart: vndcompress
	rm -f ${.TARGET}.tmp \
	&& head -c 700000 < /usr/share/dict/words \
	| ./vndcompress -l 655360 -k 1 -r /dev/stdin ${.TARGET}.tmp \
	&& mv -f ${.TARGET}.tmp ${.TARGET}
pipefalsestart.in:
	head -c 655360 < /usr/share/dict/words > ${.TARGET}.tmp \
	&& mv -f ${.TARGET}.tmp ${.TARGET}

# Make sure we can restart from a file, simulated with `-p'.
CHECKS+=	check-part
CLEANFILES+=	part.orig part.orig.tmp
CLEANFILES+=	part.cl2part part.cl2part.tmp
CLEANFILES+=	part.cl2 part.cl2.tmp
CLEANFILES+=	part.out part.out.tmp
check-part: .PHONY part.orig part.out
	cmp part.orig part.out
part.cl2: part.orig part.cl2part vndcompress
	cp part.cl2part ${.TARGET}.tmp \
	&& ./vndcompress -b 512 -r -R part.orig ${.TARGET}.tmp \
	&& mv -f ${.TARGET}.tmp ${.TARGET}
part.cl2part: part.orig vndcompress
	./vndcompress -b 512 -p 10 part.orig ${.TARGET}.tmp \
	&& mv -f ${.TARGET}.tmp ${.TARGET}
part.orig:
	head -c 12345 < /usr/share/dict/words > ${.TARGET}.tmp \
	&& mv -f ${.TARGET}.tmp ${.TARGET}

# Make sure we can `restart' even if there's nothing there.
CHECKS+=	check-falsestart
CLEANFILES+=	falsestart.in falsestart.in.tmp
CLEANFILES+=	falsestart.cl2 falsestart.cl2.tmp
CLEANFILES+=	falsestart.cl2restart falsestart.cl2restart.tmp
check-falsestart: .PHONY falsestart.cl2 falsestart.cl2restart
	cmp ${.ALLSRC}
falsestart.cl2restart: vndcompress falsestart.in
	rm -f ${.TARGET}.tmp \
	&& ./vndcompress -r falsestart.in ${.TARGET}.tmp \
	&& mv -f ${.TARGET}.tmp ${.TARGET}
falsestart.in:
	head -c 655360 < /usr/share/dict/words > ${.TARGET}.tmp \
	&& mv -f ${.TARGET}.tmp ${.TARGET}

TESTFILES+=	smallwindow
smallwindow.in:
	head -c 655360 < /usr/share/dict/words > ${.TARGET}.tmp \
	&& mv -f ${.TARGET}.tmp ${.TARGET}
smallwindow.cl2: smallwindow.in
	./vndcompress -w 1 ${.IMPSRC} ${.TARGET}.tmp \
	&& mv -f ${.TARGET}.tmp ${.TARGET}
smallwindow.out: smallwindow.cl2
	./vndcompress -w 1 -d ${.IMPSRC} ${.TARGET}.tmp \
	&& mv -f ${.TARGET}.tmp ${.TARGET}

CHECKS+=	check-pipewindow
check-pipewindow: smallwindow.cl2
	@echo '# expecting failure...'
	if cat smallwindow.cl2 | ./vndcompress -w 1 -d /dev/stdin /dev/null; \
	then \
	  echo 'unexpected pass!' && exit 1; \
	fi

# The following two tests try to ensure a limited window size means
# limited memory allocation.  They don't work very well.  The virtual
# address space rlimit (ulimit -v, RLIMIT_AS) must cover the stack size
# that is allocated automatically for the process, which varies from
# machine architecture to machine architecture (the kernel's MAXSSIZ
# parameter), as well as any shared libraries that get loaded in and
# other auxiliary crud the loader or libc might allocate.
#
# In principle, the overhead from that and the program image should be
# constant, and the only substantial memory allocation performed by
# vndcompress should be w*8 bytes or (n/b)*8, where w is the window
# size if specified, n is the size of the input, and b is the block
# size.
#
# We could perhaps do an exponential growth and then binary search on
# the virtual address space limit to determine the overhead, but that's
# more trouble than I care to do in a makefile right now.  Currently
# this is calibrated for NetBSD/amd64 6, where 128 MB of virtual
# address space is allocated for the stack.  (Note `ulimit -v' takes a
# number of kilobytes, not a number of bytes.)  Since this is not
# reliable, however, these are commented out.

#CHECKS+=	check-ulimit
#check-ulimit:
#	@echo '# expecting failure...'
#	if head -c $$((64 * 1024 * 1024)) < /dev/zero \
#	    | (ulimit -v $$((139 * 1024)) && \
#	        ./vndcompress -w 0 -l 64m -b 512 /dev/stdin /dev/null); then \
#	  echo 'unexpected pass!' && exit 1; \
#	fi
#
#CHECKS+=	check-ulimit-window
#check-ulimit-window:
#	head -c $$((64 * 1024 * 1024)) < /dev/zero \
#	| (ulimit -v $$((139 * 1024)) && \
#	    ./vndcompress -w 8192 -l 64m -b 512 /dev/stdin /dev/null)

TESTSUFFIXES+=	in cl2 cl2x out outx

TESTFORMS+=	cl2 cl2x
TESTFORMS+=	in out
TESTFORMS+=	in outx

.for testfile in ${TESTFILES}
.  for suffix in ${TESTSUFFIXES}
CLEANFILES+=	${testfile}.${suffix}
CLEANFILES+=	${testfile}.${suffix}.tmp
.  endfor
.  for left right in ${TESTFORMS}
CHECKS.${testfile}+=	check-${testfile}.${left}-${right}
check-${testfile}.${left}-${right}: .PHONY \
  ${testfile}.${left} ${testfile}.${right}
.    if empty(XFAIL:M${testfile}.${left}-${right})
	cmp ${testfile}.${left} ${testfile}.${right}
.    else
	@echo '# expecting failure...' \
	&& echo 'cmp ${testfile}.${left} ${testfile}.${right}' \
	&& if cmp ${testfile}.${left} ${testfile}.${right}; then \
	  echo 'unexpected pass!' \
	  && exit 1; \
	fi
.    endif
.  endfor
check-${testfile}: ${CHECKS.${testfile}}
CHECKS+=	check-${testfile}
.endfor

check: .PHONY ${CHECKS}

.SUFFIXES: .cl2 .cl2x .in .out .outx

# XXX These tests should automatically try different window sizes, but
# that is tricky to express in make.

.in.cl2: vndcompress
	./vndcompress ${.IMPSRC} ${.TARGET}.tmp ${BLOCKSIZE.${.PREFIX}} \
	&& mv -f ${.TARGET}.tmp ${.TARGET}

.in.cl2x:
	vndcompress ${.IMPSRC} ${.TARGET}.tmp ${BLOCKSIZE.${.PREFIX}} \
	&& mv -f ${.TARGET}.tmp ${.TARGET}

.cl2.out: vndcompress
	./vndcompress -d ${.IMPSRC} ${.TARGET}.tmp \
	&& mv -f ${.TARGET}.tmp ${.TARGET}

.cl2.outx:
	vnduncompress ${.IMPSRC} ${.TARGET}.tmp \
	&& mv -f ${.TARGET}.tmp ${.TARGET}
