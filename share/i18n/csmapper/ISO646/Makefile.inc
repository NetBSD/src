# $NetBSD: Makefile.inc,v 1.1 2003/06/27 08:40:44 tshiozak Exp $

.PATH: ${.CURDIR}/ISO646

SRCS_mapper.dir+=	mapper.dir.ISO646
SRCS_charset.pivot+=	charset.pivot.ISO646
CLEANFILES+=		mapper.dir.ISO646 charset.pivot.ISO646
PART_ISO646!=		sed '/^\#/d;/^$$/d;' ${.CURDIR}/ISO646/ISO646.part

mapper.dir.ISO646: ${.CURDIR}/ISO646/ISO646.part
	echo "# ISO646" > $@
.for i in ${PART_ISO646}
	printf '%-32s%-16s%s\n' 'ISO646-$i/UCS' 'mapper_646' \
		'ISO646/ISO646-${i:S/:/@/}%UCS.646' >> $@
	printf '%-32s%-16s%s\n' 'UCS/ISO646-$i' 'mapper_646' \
		'!ISO646/ISO646-${i:S/:/@/}%UCS.646' >> $@
.endfor
	echo >> $@

charset.pivot.ISO646: ${.CURDIR}/ISO646/ISO646.part
	echo "# ISO646" > $@
.for i in ${PART_ISO646}
	printf "%-32s%-32s%d\n" 'ISO646-$i' 'UCS' '1' >> $@
	printf "%-32s%-32s%d\n" 'UCS' 'ISO646-$i' '1' >> $@
.endfor
	echo >> $@

.for i in ${PART_ISO646}
FILES+=	ISO646-${i:S/:/@/}%UCS.646
FILESDIR_ISO646-${i:S/:/@/}%UCS.646=	${BINDIR}/ISO646
.endfor
