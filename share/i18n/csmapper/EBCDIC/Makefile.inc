# $NetBSD: Makefile.inc,v 1.1 2003/06/27 08:40:28 tshiozak Exp $

.PATH: ${.CURDIR}/EBCDIC

SRCS_mapper.dir+=	mapper.dir.EBCDIC
SRCS_charset.pivot+=	charset.pivot.EBCDIC
CLEANFILES+=		mapper.dir.EBCDIC charset.pivot.EBCDIC
PART_EBCDIC!=		sed '/^\#/d;/^$$/d;' ${.CURDIR}/EBCDIC/EBCDIC.part

mapper.dir.EBCDIC: ${.CURDIR}/EBCDIC/EBCDIC.part
	echo "# EBCDIC" > $@
.for i in ${PART_EBCDIC}
	printf '%-32s%-16s%s\n' 'EBCDIC-$i/UCS' 'mapper_std' \
		'EBCDIC/EBCDIC-${i:S/:/@/}%UCS.mps' >> $@
	printf '%-32s%-16s%s\n' 'UCS/EBCDIC-$i' 'mapper_std' \
		'EBCDIC/EBCDIC-${i:S/:/@/}%UCS.mps' >> $@
.endfor
	echo >> $@

charset.pivot.EBCDIC: ${.CURDIR}/EBCDIC/EBCDIC.part
	echo "# EBCDIC" > $@
.for i in ${PART_EBCDIC}
	printf "%-32s%-32s%d\n" 'EBCDIC-$i' 'UCS' '1' >> $@
	printf "%-32s%-32s%d\n" 'UCS' 'EBCDIC-$i' '1' >> $@
.endfor
	echo >> $@

.for i in ${PART_EBCDIC}
FILES+=					EBCDIC-${i:S/:/@/}%UCS.mps
FILESDIR_EBCDIC-${i:S/:/@/}%UCS.mps=	${BINDIR}/EBCDIC
CLEANFILES+=				EBCDIC-${i:S/:/@/}%UCS.mps
FILES+=					UCS%EBCDIC-${i:S/:/@/}.mps
FILESDIR_UCS%EBCDIC-${i:S/:/@/}.mps=	${BINDIR}/EBCDIC
CLEANFILES+=				UCS%EBCDIC-${i:S/:/@/}.mps
.endfor
