#	$NetBSD: Makefile,v 1.2 2001/02/21 16:01:52 uch Exp $
#

PROJECT_DIRS	= hpcboot libz libsa
MAKE	= make

all:
	find binary -name hpcboot.exe.uu -exec uudecode {} \;

#
# generates project/work files for Embeded Visual C++ 3.0
#
evc3:
	cd dspgen && ${MAKE} evc3 || exit 1;
	for d in ${PROJECT_DIRS}; do					\
		sh dspgen/config.sh evc3 $$d;				\
	done
	sh dspgen/gen_workspace.sh evc3 hpc_stand.vcw $(PROJECT_DIRS)
	cd dspgen && ${MAKE} clean;
#
# generates project/work files for Visual C++ 6.0 + Windows CE tool kit.
#
vc6:
	cd dspgen && ${MAKE} vc6 || exit 1;
	for d in ${PROJECT_DIRS}; do					\
		sh dspgen/config.sh vc6 $$d;				\
	done
	cd dspgen && ${MAKE} clean;
	sh dspgen/gen_workspace.sh vc6 hpc_stand.dsw $(PROJECT_DIRS)

clean:
# evc3
	rm -f hpc_stand.vcb hpc_stand.vcw hpc_stand.vco
	find . -name "*.vcp" -print |xargs rm -f
	rm -f hpcboot/hpcboot.vcl hpcboot/HPCBOOT.VCO hpcboot/HPCBOOT.VCW
	rm -f hpcboot/HPCBOOT.vcb
# vc6
	find . -name "*.dsp" -print |xargs rm -f
	rm -f hpc_stand.dsw hpc_stand.ncb hpc_stand.opt
	rm -f hpcboot/hpcboot.plg
# 
	rm -rf compile/ARMDebug compile/ARMRelease			\
	compile/SH3Debug compile/SH3Release				\
	compile/MIPSDebug compile/MIPSRelease
	find . -name "hpcboot.exe" -print |xargs rm -f

distclean: clean
	find binary -name hpcboot.exe* -print | xargs rm -f;

install:
	for a in SH3 ARM MIPS; do					\
		file=`echo "compile/"$$a"Release/hpcboot.exe"`;		\
		cp $$file binary/$$a/hpcboot.exe;			\
	done
uuencode:
	find binary -name hpcboot.exe -print | \
	awk '{ print "uuencode", $$1, $$1, ">", $$1".uu ; rm -f", $$1 }' | sh



