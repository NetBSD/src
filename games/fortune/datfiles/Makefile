#	$NetBSD: Makefile,v 1.30 2000/12/30 17:51:26 sommerfeld Exp $
#	@(#)Makefile	8.2 (Berkeley) 4/19/94

.include <bsd.own.mk>			# for INSTALL_OFFENSIVE_FORTUNES

SRCFILES=	fortunes fortunes2 startrek zippy unamerican-o limerick \
		netbsd
DATFILES=	fortunes.dat fortunes2.dat startrek.dat zippy.dat \
		fortunes-o fortunes-o.dat fortunes2-o fortunes2-o.dat \
		limerick.dat limerick-o limerick-o.dat unamerican-o \
		unamerican-o.dat netbsd.dat netbsd-o netbsd-o.dat

# TO AVOID INSTALLING THE POTENTIALLY OFFENSIVE FORTUNES, RUN 'make' with
# "INSTALL_OFFENSIVE_FORTUNES=NO", or set "INSTALL_OFFENSIVE_FORTUNES=NO"
# IN bsd.own.mk OR /etc/mk.conf.

INSTALL_OFFENSIVE_FORTUNES?=	YES
.if (${INSTALL_OFFENSIVE_FORTUNES} == "YES")
TYPE=		real
.else
TYPE=		fake
.endif

CLEANFILES+=${DATFILES} limerick-o

STRFILEDIR!=cd $(.CURDIR)/../strfile; ${PRINTOBJDIR}
STRFILE=${STRFILEDIR}/strfile

.if ${MKSHARE} != "no"
FILES=${SRCFILES} ${DATFILES}
FILESDIR=/usr/share/games/fortune
.endif

realall: ${DATFILES}

# Normal fortunes: build directly from data files.

fortunes.dat fortunes2.dat startrek.dat zippy.dat limerick.dat netbsd.dat:
	${STRFILE} -rs ${.CURDIR}/${.TARGET:R} ${.TARGET}
fortunes.dat:	fortunes
fortunes2.dat:	fortunes2
startrek.dat:	startrek
zippy.dat:	zippy
limerick.dat:	limerick
netbsd.dat:	netbsd

# Obscene fortunes: we have to build source and data files

fortunes-o: fortunes-o.${TYPE}
	tr [a-zA-Z] [n-za-mN-ZA-M] < ${.ALLSRC} > ${.TARGET}
fortunes-o.dat: fortunes-o
	${STRFILE} -rsx ${.ALLSRC} ${.TARGET}

fortunes2-o: fortunes2-o.${TYPE}
	tr [a-zA-Z] [n-za-mN-ZA-M] < ${.ALLSRC} > ${.TARGET}
fortunes2-o.dat: fortunes2-o
	${STRFILE} -rsx ${.ALLSRC} ${.TARGET}

limerick-o: limerick-o.${TYPE}
	tr [a-zA-Z] [n-za-mN-ZA-M] < ${.ALLSRC} > ${.TARGET}
limerick-o.dat: limerick-o
	${STRFILE} -rsx ${.ALLSRC} ${.TARGET}

unamerican-o: unamerican-o.${TYPE}
	tr [a-zA-Z] [n-za-mN-ZA-M] < ${.ALLSRC} > ${.TARGET}
unamerican-o.dat: unamerican-o
	${STRFILE} -rsx ${.ALLSRC} ${.TARGET}

netbsd-o: netbsd-o.${TYPE}
	tr [a-zA-Z] [n-za-mN-ZA-M] < ${.ALLSRC} > ${.TARGET}
netbsd-o.dat: netbsd-o
	${STRFILE} -rsx ${.ALLSRC} ${.TARGET}


.include <bsd.prog.mk>
