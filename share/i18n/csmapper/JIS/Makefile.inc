# $NetBSD: Makefile.inc,v 1.2 2003/07/19 20:20:41 tshiozak Exp $

.PATH: ${.CURDIR}/JIS

SRCS_mapper.dir+=	mapper.dir.JIS
SRCS_charset.pivot+=	charset.pivot.JIS
CLEANFILES+=		mapper.dir.JIS charset.pivot.JIS
MAPS_JIS=		JISX0201-KANA%UCS.mps \
			JISX0208@1990%UCS.mps \
			JISX0212%UCS.mps \
			JISX0208@MS%UCS.mps \
			JISX0208VDC@NEC%UCS.mps \
			JISX0208UDC%UCS.mps \
			JISX0212@MS%UCS.mps \
			JISX0212VDC@IBM%UCS.mps \
			JISX0212UDC%UCS.mps \
			UCS%JISX0201-KANA.mps \
			UCS%JISX0208@1990.mps \
			UCS%JISX0212.mps \
			UCS%JISX0208@MS.mps \
			UCS%JISX0208VDC@NEC.mps \
			UCS%JISX0208UDC.mps \
			UCS%JISX0212@MS.mps \
			UCS%JISX0212VDC@IBM.mps \
			UCS%JISX0212UDC.mps

mapper.dir.JIS: ${.CURDIR}/JIS/mapper.dir.JIS.src
	echo "# JIS" > $@
	cat $> >> $@
	echo >> $@

charset.pivot.JIS: ${.CURDIR}/JIS/charset.pivot.JIS.src
	echo "# JIS" > $@
	cat $> >> $@
	echo >> $@

FILES+= ${MAPS_JIS}
CLEANFILES+= ${MAPS_JIS}
.for i in ${MAPS_JIS}
FILESDIR_$i= ${BINDIR}/JIS
.endfor
