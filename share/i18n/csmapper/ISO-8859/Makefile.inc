# $NetBSD: Makefile.inc,v 1.1 2003/06/27 08:40:35 tshiozak Exp $

.PATH: ${.CURDIR}/ISO-8859

SRCS_mapper.dir+=	mapper.dir.ISO-8859
SRCS_charset.pivot+=	charset.pivot.ISO-8859
CLEANFILES+=		mapper.dir.ISO-8859 charset.pivot.ISO-8859
PART_ISO-8859!=		sed '/^\#/d;/^$$/d;' ${.CURDIR}/ISO-8859/ISO-8859.part

.for i in ${PART_ISO-8859}
.if exists(${.CURDIR}/ISO-8859/ISO-8859-$i%UCS.src)
FILES+=				ISO-8859-$i%UCS.mps
CLEANFILES+=			ISO-8859-$i%UCS.mps
FILESDIR_ISO-8859-$i%UCS.mps=	${BINDIR}/ISO-8859
.endif
.if exists(${.CURDIR}/ISO-8859/UCS%ISO-8859-$i.src)
FILES+=				UCS%ISO-8859-$i.mps
CLEANFILES+=			UCS%ISO-8859-$i.mps
FILESDIR_UCS%ISO-8859-$i.mps=	${BINDIR}/ISO-8859
.endif
.endfor

mapper.dir.ISO-8859: ${.CURDIR}/ISO-8859/ISO-8859.part
	echo "# ISO-8859" > $@
	printf '%-32s%-16s\n' 'ISO-8859-1/UCS' mapper_none >> $@
	printf '%-32s%-16s%s\n' 'UCS/ISO-8859-1' mapper_zone '0x00-0xFF' >> $@
.for i in ${PART_ISO-8859}
	printf '%-32s%-16s%s\n' 'ISO-8859-$i/UCS' mapper_std 'ISO-8859/ISO-8859-$i%UCS.mps' >> $@
	printf '%-32s%-16s%s\n' 'UCS/ISO-8859-$i' mapper_std 'ISO-8859/UCS%ISO-8859-$i.mps' >> $@
	printf '%-32s%-16s%s\n' 'ISO-8859-$i:GR/ISO-8859-$i' mapper_zone '0x00 - 0x7F : +0x80' >> $@
	printf '%-32s%-16s%s\n' 'ISO-8859-$i/ISO-8859-$i:GR' mapper_zone '0x80 - 0xFF : -0x80' >> $@
	printf '%-32s%-16s%s\n' 'ISO-8859-$i:GR/UCS' mapper_serial 'ISO-8859-$i:GR/ISO-8859-$i,ISO-8859-$i/UCS' >> $@
	printf '%-32s%-16s%s\n' 'UCS/ISO-8859-$i:GR' mapper_serial 'UCS/ISO-8859-$i,ISO-8859-$i/ISO-8859-$i:GR' >> $@
.endfor
	echo >> $@

charset.pivot.ISO-8859: ${.CURDIR}/ISO-8859/ISO-8859.part
	echo "# ISO-8859" > $@
.for i in ${PART_ISO-8859}
	printf "%-32s%-32s%d\n" 'ISO-8859-$i' 'UCS' '1' >> $@
	printf "%-32s%-32s%d\n" 'UCS' 'ISO-8859-$i' '1' >> $@
	printf "%-32s%-32s%d\n" 'ISO-8859-$i:GR' 'UCS' '1' >> $@
	printf "%-32s%-32s%d\n" 'UCS' 'ISO-8859-$i:GR' '1' >> $@
.endfor
	echo >> $@
