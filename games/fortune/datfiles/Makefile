#	$NetBSD: Makefile,v 1.19 1997/05/07 07:15:35 mycroft Exp $
#	@(#)Makefile	8.2 (Berkeley) 4/19/94

.include <bsd.own.mk>			# for INSTALL_OFFENSIVE_FORTUNES

SRCFILES=	fortunes fortunes2 startrek zippy
DATFILES=	fortunes.dat fortunes2.dat startrek.dat zippy.dat \
		fortunes-o fortunes-o.dat fortunes2-o fortunes2-o.dat \
		limerick limerick.dat

# TO AVOID INSTALLIND THE POTENTIALLY OFFENSIVE FORTUNES, RUN 'make' with
# "INSTALL_OFFENSIVE_FORTUNES=NO", or set "INSTALL_OFFENSIVE_FORTUNES=NO"
# IN bsd.own.mk OR /etc/mk.conf.

INSTALL_OFFENSIVE_FORTUNES?=	YES
.if (${INSTALL_OFFENSIVE_FORTUNES} == "YES")
TYPE=		real
.else
TYPE=		fake
.endif

CLEANFILES+=${DATFILES}

STRFILE!=cd $(.CURDIR)/../strfile; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/strfile\n" | ${MAKE} -s -f-

FILES=${SRCFILES} ${DATFILES}
FILESDIR=/usr/share/games/fortune

all: ${DATFILES}

# Normal fortunes: build directly from data files.

fortunes.dat fortunes2.dat startrek.dat zippy.dat:
	${STRFILE} -rs ${.CURDIR}/${.TARGET:R} ${.TARGET}
fortunes.dat:	fortunes
fortunes2.dat:	fortunes2
startrek.dat:	startrek
zippy.dat:	zippy

# Obscene fortunes: we have to build source and data files

fortunes-o: fortunes-o.${TYPE}
	tr [a-zA-Z] [n-za-mN-ZA-M] < ${.ALLSRC} > ${.TARGET}
fortunes-o.dat: fortunes-o
	${STRFILE} -rsx ${.ALLSRC} ${.TARGET}

fortunes2-o: fortunes2-o.${TYPE}
	tr [a-zA-Z] [n-za-mN-ZA-M] < ${.ALLSRC} > ${.TARGET}
fortunes2-o.dat: fortunes2-o
	${STRFILE} -rsx ${.ALLSRC} ${.TARGET}

limerick: limerick.${TYPE}
	tr [a-zA-Z] [n-za-mN-ZA-M] < ${.ALLSRC} > ${.TARGET}
limerick.dat: limerick
	${STRFILE} -rsx ${.ALLSRC} ${.TARGET}


.include <bsd.prog.mk>
