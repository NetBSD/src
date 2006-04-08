# $NetBSD: Makefile.inc,v 1.1 2006/04/08 14:15:48 tnozaki Exp $

.PATH: ${.CURDIR}/KOI

SRCS_mapper.dir+=	mapper.dir.KOI
SRCS_charset.pivot+=	charset.pivot.KOI
CLEANFILES+=		mapper.dir.KOI charset.pivot.KOI
PART_KOI8!=		sed '/^\#/d;/^$$/d;' ${.CURDIR}/KOI/KOI8.part

mapper.dir.KOI: ${.CURDIR}/KOI/KOI8.part
	${_MKTARGET_CREATE}
	( \
	    echo "# KOI"; \
	    for i in 7 8-BASE ${PART_KOI8}; do \
		printf "%-31s %-31s %s\n" KOI$$i"/UCS" \
			mapper_std KOI/KOI$$i"%UCS.mps"; \
		printf "%-31s %-31s %s\n" UCS/KOI$$i \
			mapper_std KOI/UCS%KOI$$i".mps"; \
	    done; \
	    for i in ${PART_KOI8}; do \
		printf "%-31s %-31s %s\n" KOI$$i"-PARALLEL/UCS" \
			mapper_parallel KOI8-BASE/UCS,KOI$$i"/UCS"; \
		printf "%-31s %-31s %s\n" UCS/KOI$$i"-PARALLEL" \
			mapper_parallel UCS/KOI$$i",UCS/KOI8-BASE"; \
	    done; \
	    echo; \
	) > ${.TARGET}

charset.pivot.KOI: ${.CURDIR}/KOI/KOI8.part
	${_MKTARGET_CREATE}
	( \
	    echo "# KOI"; \
	    for i in 7 8-BASE ${PART_KOI8}; do \
		printf "%-31s %-31s%d\n" KOI$$i UCS 1; \
		printf "%-31s %-31s%d\n" UCS KOI$$i 1; \
	    done; \
	    for i in ${PART_KOI8}; do \
		printf "%-31s %-31s%d\n" KOI$$i"-PARALLEL" UCS 1; \
		printf "%-31s %-31s%d\n" UCS KOI$$i"-PARALLEL" 1; \
	    done; \
	    echo; \
	) > ${.TARGET}

.for i in 7 8-BASE ${PART_KOI8}
FILES+=		KOI${i:S/:/@/}%UCS.mps UCS%KOI${i:S/:/@/}.mps
CLEANFILES+=	KOI${i:S/:/@/}%UCS.mps UCS%KOI${i:S/:/@/}.mps
FILESDIR_KOI${i:S/:/@/}%UCS.mps=	${BINDIR}/KOI
FILESDIR_UCS%KOI${i:S/:/@/}.mps=	${BINDIR}/KOI
.endfor
