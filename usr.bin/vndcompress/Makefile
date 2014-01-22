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
	| ./vndcompress -l 655360 -k 1 -rR /dev/stdin ${.TARGET}.tmp \
	&& mv -f ${.TARGET}.tmp ${.TARGET}
piperestart.cl2part: vndcompress
	head -c 600000 < /usr/share/dict/words \
	| ./vndcompress -l 655360 -k 1 /dev/stdin ${.TARGET}.tmp; \
	mv -f ${.TARGET}.tmp ${.TARGET}
piperestart.in:
	head -c 655360 < /usr/share/dict/words > ${.TARGET}.tmp \
	&& mv -f ${.TARGET}.tmp ${.TARGET}

CHECKS+=	check-part
CLEANFILES+=	part.orig part.cl2part part.cl2 part.out
check-part: .PHONY part.orig part.out
	cmp part.orig part.out
part.cl2: part.orig part.cl2part vndcompress
	cp part.cl2part ${.TARGET}.tmp \
	&& ./vndcompress -s 512 -rR part.orig ${.TARGET}.tmp \
	&& mv -f ${.TARGET}.tmp ${.TARGET}
part.cl2part: part.orig vndcompress
	./vndcompress -s 512 -p 10 part.orig ${.TARGET}.tmp \
	&& mv -f ${.TARGET}.tmp ${.TARGET}
part.orig:
	head -c 12345 < /usr/share/dict/words > ${.TARGET}.tmp \
	&& mv -f ${.TARGET}.tmp ${.TARGET}

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
