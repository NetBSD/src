#	$NetBSD: Makefile,v 1.34 2003/07/18 08:26:10 lukem Exp $

.include <bsd.own.mk>

# Change the line below for your time zone (after finding the zone you want in
# the time zone files, or adding it to a time zone file).
# Alternately, if you discover you've got the wrong time zone, you can just
#	zic -l rightzone

# If you want something other than Eastern United States time as a template
# for handling POSIX-style time zone environment variables,
# change the line below (after finding the zone you want in the
# time zone files, or adding it to a time zone file).
# Alternately, if you discover you've got the wrong time zone, you can just
#	zic -p rightzone

POSIXRULES=	US/Pacific

# Use an absolute path name for TZDIR unless you're just testing the software.
# Note: ${DESTDIR} is prepended to this for the actual copy.

TZDIR=	/usr/share/zoneinfo

# If you always want time values interpreted as "seconds since the epoch
# (not counting leap seconds)", use
# 	REDO=		posix_only
# below.  If you always want right time values interpreted as "seconds since
# the epoch" (counting leap seconds)", use
#	REDO=		right_only
# below.  If you want both sets of data available, with leap seconds not
# counted normally, use
#	REDO=		posix_right
# below.  If you want both sets of data available, with leap seconds counted
# normally, use
#	REDO=		right_posix
# below.

REDO=		posix_only

# Since "." may not be in PATH...
YEARISTYPE=	"sh ${.CURDIR}/yearistype.sh"

YDATA=		africa antarctica asia australasia \
		europe northamerica southamerica pacificnew etcetera factory \
		backward
NDATA=		systemv
SDATA=		solar87 solar88 solar89
TDATA=		$(YDATA) $(NDATA) $(SDATA)
DATA=		$(YDATA) $(NDATA) $(SDATA) leapseconds # yearistype.sh
USNO=		usno1988 usno1989

TZBUILDDIR=	${.OBJDIR}/builddir
TZBUILDSPEC=	${.OBJDIR}/builddir.spec

.PHONY:	posix_only
posix_only: ${TDATA}
	mkdir -p ${TZBUILDDIR}
	cd ${.CURDIR} && \
	    ${TOOL_ZIC} -y ${YEARISTYPE} -d ${TZBUILDDIR} -L /dev/null ${TDATA}

.PHONY:	right_only
right_only: leapseconds ${TDATA}
	mkdir -p ${TZBUILDDIR}
	cd ${.CURDIR} && \
	    ${TOOL_ZIC} -y ${YEARISTYPE} -d ${TZBUILDDIR} -L leapseconds ${TDATA}

.PHONY:	other_two
other_two: leapseconds ${TDATA}
	mkdir -p ${TZBUILDDIR}
	cd ${.CURDIR} && \
	    ${TOOL_ZIC} -y ${YEARISTYPE} -d ${TZBUILDDIR}/posix -L /dev/null ${TDATA}
	cd ${.CURDIR} && \
	    ${TOOL_ZIC} -y ${YEARISTYPE} -d ${TZBUILDDIR}/right -L leapseconds ${TDATA}

.PHONY:	posix_right
posix_right: posix_only other_two

.PHONY:	right_posix
right_posix: right_only other_two

.if ${MKUNPRIVED} == "no"
TOOL_PAX.unpriv=-pe
.else
TOOL_PAX.unpriv=
.endif

.if ${MKSHARE} != "no"
afterinstall: ${DATA} ${REDO}
	mkdir -p ${TZBUILDDIR}
	cd ${.CURDIR} && \
	    ${TOOL_ZIC} -y ${YEARISTYPE} -d ${TZBUILDDIR} -p ${POSIXRULES}
	cd ${TZBUILDDIR} && \
	(   find . -type d | xargs -n 1 printf \
		"%s type=dir mode=0755 uname=${BINOWN} gname=${BINGRP}\n" ; \
	    find . -type f | xargs -n 1 printf \
		"%s type=file mode=0444 uname=${BINOWN} gname=${BINGRP}\n" ; \
	) > ${TZBUILDSPEC}
	cd ${TZBUILDDIR} && \
	  ${TOOL_PAX} -O -rw ${TOOL_PAX.unpriv} -M -N ${NETBSDSRCDIR}/etc \
	  ${DESTDIR}${TZDIR} < ${TZBUILDSPEC}
.if ${MKUNPRIVED} != "no"
	sed -e "s|^\.|./${TZDIR}|g" -e "s|//|/|g" < ${TZBUILDSPEC} | \
	    ${METALOG.add}
.endif

.else	# ${MKSHARE} == "no"
afterinstall:
.endif	# ${MKSHARE} == "no"

clean:
	-rm -rf ${TZBUILDDIR} ${TZBUILDSPEC}

.include <bsd.prog.mk>
