# $NetBSD: Makefile.inc,v 1.1 2003/06/27 08:40:11 tshiozak Exp $

.PATH: ${.CURDIR}/CP

SRCS_mapper.dir+=	mapper.dir.CP
SRCS_charset.pivot+=	charset.pivot.CP
CLEANFILES+=		mapper.dir.CP charset.pivot.CP
PART_CP!=		sed '/^\#/d;/^$$/d;' ${.CURDIR}/CP/CP.part

mapper.dir.CP: ${.CURDIR}/CP/CP.part ${.CURDIR}/CP/mapper.dir.CP.src
	echo "# CP" > $@
.for i in ${PART_CP}
	printf '%-32s%-16s%s\n' 'CP$i/UCS' 'mapper_std' \
		'CP/CP${i:S/:/@/}%UCS.mps' >> $@
	printf '%-32s%-16s%s\n' 'UCS/CP$i' 'mapper_std' \
		'CP/UCS%CP${i:S/:/@/}.mps' >> $@
.endfor
	cat ${.CURDIR}/CP/mapper.dir.CP.src >> $@
	echo >> $@

charset.pivot.CP: ${.CURDIR}/CP/CP.part ${.CURDIR}/CP/charset.pivot.CP.src
	echo "# CP" > $@
.for i in ${PART_CP}
	printf "%-32s%-32s%d\n" 'CP$i' 'UCS' '1' >> $@
	printf "%-32s%-32s%d\n" 'UCS' 'CP$i' '1' >> $@
.endfor
	cat ${.CURDIR}/CP/charset.pivot.CP.src >> $@
	echo >> $@

.for i in ${PART_CP}
FILES+=		CP${i:S/:/@/}%UCS.mps UCS%CP${i:S/:/@/}.mps
CLEANFILES+=	CP${i:S/:/@/}%UCS.mps UCS%CP${i:S/:/@/}.mps
FILESDIR_CP${i:S/:/@/}%UCS.mps=	${BINDIR}/CP
FILESDIR_UCS%CP${i:S/:/@/}.mps=	${BINDIR}/CP
.endfor
