#	$NetBSD: Makefile,v 1.28.2.1 2002/06/11 16:02:11 lukem Exp $

NOOBJ=	# defined

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

TZDIR=	${DESTDIR}/usr/share/zoneinfo

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

ZIC?=zic

.PHONY:	posix_only
posix_only: ${TDATA}
	${ZIC} -y ${YEARISTYPE} -d ${TZDIR} -L /dev/null ${TDATA}

.PHONY:	right_only
right_only: leapseconds ${TDATA}
	${ZIC} -y ${YEARISTYPE} -d ${TZDIR} -L leapseconds ${TDATA}

.PHONY:	other_two
other_two: leapseconds ${TDATA}
	${ZIC} -y ${YEARISTYPE} -d ${TZDIR}/posix -L /dev/null ${TDATA}
	${ZIC} -y ${YEARISTYPE} -d ${TZDIR}/right -L leapseconds ${TDATA}

.PHONY:	posix_right
posix_right: posix_only other_two

.PHONY:	right_posix
right_posix: right_only other_two

.if ${MKSHARE} != "no"
afterinstall: ${DATA} ${REDO}
	${ZIC} -y ${YEARISTYPE} -d ${TZDIR} -p ${POSIXRULES}
	find ${TZDIR} -type d | xargs chmod u=rwX,go=rX
	find ${TZDIR} -type f | xargs chmod a=r
.if !defined(UNPRIVED)
	chown -R ${BINOWN}:${BINGRP} ${TZDIR}
.else
	find ${TZDIR} -type d | xargs -n 1 printf \
		"%s type=dir mode=0755 uname=${BINOWN} gname=${BINGRP}\n" | \
		sed -e 's|^/|./|g' -e 's|//|/|g' | ${CAT} -l >> ${METALOG}
	find ${TZDIR} -type f | xargs -n 1 printf \
		"%s type=file mode=0444 uname=${BINOWN} gname=${BINGRP}\n" | \
		sed -e 's|^/|./|g' -e 's|//|/|g' | ${CAT} -l >> ${METALOG}
.endif
.else
afterinstall:
.endif

.include <bsd.prog.mk>
