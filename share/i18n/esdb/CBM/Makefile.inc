# $NetBSD: Makefile.inc,v 1.1.2.2 2011/05/19 21:06:57 bouyer Exp $

.PATH: ${.CURDIR}/CBM

CODESETS_CBM!=	${TOOL_SED} \
		'/^\#/d;/^ *$$/d;/^[ 	]*/s///;s/[ 	].*//' \
		${.CURDIR}/CBM/esdb.dir.CBM.src
ESDB_CBM:=	${CODESETS_CBM:C/$/.esdb/:S/:/@/}

SRC_esdb.dir+=		esdb.dir.CBM.src
SRC_esdb.alias+=	esdb.alias.CBM.src
FILES+=			${ESDB_CBM}
CLEANFILES+=		${ESDB_CBM}
.for i in ${ESDB_CBM}
FILESDIR_$i:=		${BINDIR}/CBM
.endfor
