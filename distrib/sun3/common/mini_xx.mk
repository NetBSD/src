#
# $NetBSD: mini_xx.mk,v 1.3 1997/12/12 22:39:07 gwr Exp $
# Hacks for re-linking some programs -static
#

MINI_XX = grep tip vi
mini_xx : ${MINI_XX}

clean_xx:
	-rm -f ${MINI_XX}

grep :
	cd ${BSDSRCDIR}/gnu/usr.bin/grep ;\
	$(MAKE) -f Makefile -f ${TOP}/common/Make.static \
	    OUTDIR=${.CURDIR} ${.CURDIR}/grep

tip :
	cd ${BSDSRCDIR}/usr.bin/tip ;\
	$(MAKE) -f Makefile -f ${TOP}/common/Make.static \
	    OUTDIR=${.CURDIR} ${.CURDIR}/tip

vi :
	cd ${BSDSRCDIR}/usr.bin/vi/build ;\
	$(MAKE) -f Makefile -f ${TOP}/common/Make.static \
	    OUTDIR=${.CURDIR} ${.CURDIR}/vi
