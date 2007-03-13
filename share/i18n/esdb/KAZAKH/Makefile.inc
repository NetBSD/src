# $NetBSD: Makefile.inc,v 1.1 2007/03/13 16:34:39 tnozaki Exp $

.PATH: ${.CURDIR}/KAZAKH

CODESETS_KAZAKH!=	sed '/^\#/d;/^ *$$/d;/^[ 	]*/s///;s/[ 	].*//' \
			${.CURDIR}/KAZAKH/esdb.dir.KAZAKH.src
ESDB_KAZAKH:=	${CODESETS_KAZAKH:C/$/.esdb/:S/:/@/}

SRC_esdb.dir+=		esdb.dir.KAZAKH.src
SRC_esdb.alias+=	esdb.alias.KAZAKH.src
FILES+=			${ESDB_KAZAKH}
CLEANFILES+=		${ESDB_KAZAKH}
.for i in ${ESDB_KAZAKH}
FILESDIR_$i:=		${BINDIR}/KAZAKH
.endfor
