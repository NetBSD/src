NOMAN=noman

.include "../../Makefile.inc"

DIFF      = /usr/bin/diff
DIFF3     = /usr/bin/diff3

DEFINES = -DV4_2BSD -DSIGNAL_TYPE=void -DSTRICT_LOCKING=1 -DDIFF='"${DIFF}"' -DCO='"${BINDIR}/co"' -DMERGE='"${BINDIR}/merge"' -DVFPRINTF
CFLAGS += ${DEFINES} -I${.CURDIR}
LINTFLAGS = -c -u -Dlint ${DEFINES}

NORMAL_LINK = ${CC} ${LDFLAGS} -o ${.TARGET} ${.ALLSRC}
NORMAL_LINT = ${LINT} ${LINTFLAGS} ${.ALLSRC}

all:

ci.o co.o ident.o rcs.o rcsdiff.o rcsedit.o rcsfcmp.o rcsfnms.o \
rcsgen.o rcskeep.o rcskeys.o rcslex.o rcsmerge.o rcsrev.o rcssyn.o \
rcsutil.o rlog.o snoop.o rcssynTST.o rcsrevTST.o rcsfnmsTST.o \
rcsfcmpTST.o rcskeepTST.o: rcsbase.h

partime.o maketime.o co.o ci.o rcs.o rlog.o rcsutil.o: time.h

INSTALL = install

install:	install-exec install-script

install-exec:
	${INSTALL} ${COPY} -m ${BINMODE} -o ${BINOWN} -g ${BINGRP} ${STRIP} ${.ALLSRC} ${BINDIR}
install-script:
	${INSTALL} ${COPY} -m ${BINMODE} -o ${BINOWN} -g ${BINGRP} ${.ALLSRC} ${BINDIR}

installog:	installsnoop install

installsnoop:	snoop
	install ${COPY} -m 4775 -o ${BINOWN} -g ${BINGRP} ${STRIP} snoop ${BINDIR}${SNOOP}
	chmod u+s ${BINDIR}${SNOOP}
	touch ${BINDIR}${SNOOPFILE}
	chown bin.bin ${BINDIR}${SNOOPFILE}
	chmod 644 ${BINDIR}${SNOOPFILE}
# CAUTION: You may have to do a chown of SNOOP and SNOOPFILE (if not owned by root).
lint: ci.lint co.lint ident.lint rlog.lint rcs.lint rcsdiff.lint rcsmerge.lint snoop.lint

clean:
	rm -f *.o ci co ident merge rlog rcs rcsclean rcsdiff rcsmerge snoop \
	      sccstorcs rcsfreeze

cleandir: clean
	rm -f tags .depend


all install-exec: ci

ci:	ci.o rcslex.o rcssyn.o rcsgen.o rcsedit.o rcskeys.o rcsrev.o rcsutil.o rcsfnms.o partime.o maketime.o localzone.o rcskeep.o rcsfcmp.o
	${NORMAL_LINK}

ci.lint: ci.c rcslex.c rcssyn.c rcsgen.c rcsedit.c rcskeys.c rcsrev.c rcsutil.c rcsfnms.c partime.c maketime.c localzone.c rcskeep.c rcsfcmp.c
	${NORMAL_LINT}
	

all install-exec: co

co:	co.o rcslex.o rcssyn.o rcsgen.o rcsedit.o rcskeys.o rcsrev.o rcsutil.o rcsfnms.o partime.o maketime.o localzone.o
	${NORMAL_LINK}

co.lint: co.c rcslex.c rcssyn.c rcsgen.c rcsedit.c rcskeys.c rcsrev.c rcsutil.c rcsfnms.c partime.c maketime.c localzone.c
	${NORMAL_LINT}


all install-exec: ident

ident:	ident.o rcskeys.o
	${NORMAL_LINK}

ident.lint: ident.c rcskeys.c
	${NORMAL_LINT}


all install-script: merge

.SUFFIXES: .sh
merge:	merge.sh
	sed -e '/^#/d' \
		-e 's:DIFF=.*$$:DIFF=${DIFF}:' \
		-e 's:DIFF3=.*$$:DIFF3=${DIFF3}:' \
		${.ALLSRC} > ${.TARGET}
#	This takes out the comment lines and substitutes in DIFF and DIFF3.
#	(Comments are not permitted in some older shells.)
	chmod 755 ${.TARGET}


all install-exec: rlog

rlog:	rlog.o rcslex.o rcssyn.o rcsrev.o rcsutil.o partime.o maketime.o localzone.o rcsfnms.o
	${NORMAL_LINK}

rlog.lint: rlog.c rcslex.c rcssyn.c rcsrev.c rcsutil.c partime.c maketime.c localzone.c rcsfnms.c
	${NORMAL_LINT}


all install-exec: rcs

rcs:	rcs.o rcslex.o rcssyn.o rcsrev.o rcsutil.o rcsgen.o rcsedit.o rcskeys.o rcsfnms.o
	${NORMAL_LINK}

rcs.lint: rcs.c rcslex.c rcssyn.c rcsrev.c rcsutil.c rcsgen.c rcsedit.c rcskeys.c rcsfnms.c
	${NORMAL_LINT}


all install-script: rcsclean

rcsclean:	rcsclean.sh
	sed -e '/^#/d' ${.ALLSRC} > ${.TARGET}
#	This takes out the comments (not permitted in older shells).
	chmod 755 ${.TARGET}


all install-exec: rcsdiff

rcsdiff:	rcsdiff.o rcsutil.o rcsfnms.o rcsrev.o rcssyn.o rcslex.o
	${NORMAL_LINK}

rcsdiff.lint: rcsdiff.c rcsutil.c rcsfnms.c rcsrev.c rcssyn.c rcslex.c
	${NORMAL_LINT}


all install-exec: rcsmerge

rcsmerge:	rcsmerge.o rcsutil.o rcsfnms.o rcsrev.o rcssyn.o rcslex.o
	${NORMAL_LINK}

rcsmerge.lint: rcsmerge.c rcsutil.c rcsfnms.c rcsrev.c rcssyn.c rcslex.c
	${NORMAL_LINT}


all install-exec: sccstorcs

sccstorcs:	sccstorcs.o
	${NORMAL_LINK}

sccstorcs.lint:	sccstorcs.c
	${NORMAL_LINT}


all install-script: rcsfreeze

rcsfreeze: rcsfreeze.sh
	cat ${.ALLSRC} > ${.TARGET}


#all install-exec: snoop

snoop:	snoop.o
	${NORMAL_LINK}

snoop.lint: snoop.c
	${NORMAL_LINT}


SOURCE=	ci.c co.c curdir.c ident.c maketime.c localzone.c partime.c rcs.c \
	rcsclean.c rcsdiff.c rcsedit.c rcsfcmp.c rcsfnms.c rcsgen.c \
	rcskeep.c rcskeys.c rcslex.c rcsmerge.c rcsrev.c rcssyn.c rcsutil.c \
	rlog.c snoop.c

HFILES=	rcsbase.h time.h

depend:	${SOURCE} ${HFILES}
	mkdep ${CFLAGS:S/-O//} ${.ALLSRC:M*.c}

obj:
	@cd ${.CURDIR}; \
        here=`pwd`; subdir=`echo $$here | sed 's,^/usr/src/,,'`; \
        if test $$here != $$subdir ; then \
                rm -rf obj; \
                dest=/usr/obj/$$subdir ; \
                echo "$$here -> $$dest"; ln -s $$dest obj; \
                if test -d /usr/obj -a ! -d $$dest; then \
                        mkdir -p $$dest; \
                else \
                        true; \
                fi; \
        else \
                true ; \
                dest=$$here/obj ; \
                /bin/rm -f $$dest 2> /dev/null || true; \
                echo "making $$here/obj" ; \
                if test ! -d obj ; then \
                        mkdir $$here/obj; \
                fi ; \
        fi;

.include <bsd.prog.mk>
