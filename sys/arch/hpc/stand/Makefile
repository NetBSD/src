#	$NetBSD: Makefile,v 1.8.2.1 2002/02/11 20:07:50 jdolecek Exp $
#

PROJECT_DIRS=	hpcboot libz libsa
UUDECODE?=	uudecode

all:
	find binary -name "hpcboot*.exe.uu" -exec ${UUDECODE} {} \;

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

#
# generates project/work files for Visual C++ 5.0 Windows CE Embeded tool kit.
# (WCE100/101/200 binary for MIPS, SH3)
#
vc5:
	cd dspgen && ${MAKE} vc5 || exit 1;
	for d in ${PROJECT_DIRS}; do					\
		sh dspgen/config.sh vc5 $$d;				\
	done
	cd dspgen && ${MAKE} clean;
	sh dspgen/gen_workspace.sh vc5 hpc_stand.dsw $(PROJECT_DIRS)

clean:
	rm -f hpcboot/opt_spec_platform.h
# evc3 temporary files
	rm -f hpc_stand.vcb hpc_stand.vcw hpc_stand.vco
	find . -name "*.vcp" -print |xargs rm -f
	rm -f hpcboot/hpcboot.vcl hpcboot/HPCBOOT.VCO hpcboot/HPCBOOT.VCW
	rm -f hpcboot/HPCBOOT.vcb
# vc6 temporary files
	find . -name "*.dsp" -print |xargs rm -f
	rm -f hpc_stand.dsw hpc_stand.ncb hpc_stand.opt
	rm -f hpcboot/hpcboot.plg
# compile directory
	rm -rf compile/ARMDebug compile/ARMRelease			\
	compile/SH*Debug compile/SH*Release				\
	compile/MIPSDebug compile/MIPSRelease
	find . -name "hpcboot.exe" -print |xargs rm -f

distclean: clean
	find binary -name "hpcboot*.exe*" -print | xargs rm -f;

# WCE210 or later binary
install:
	for a in SH3 SH4 ARM MIPS; do					\
		file=`echo "compile/"$$a"Release/hpcboot.exe"`;		\
		cp $$file binary/$$a/hpcboot.exe;			\
	done
# WCE200 or earlier binary
install1:
	cp compile/MIPSRelease/hpcboot.exe binary/MIPS/hpcboot1.exe
	cp compile/SHRelease/hpcboot.exe binary/SH3/hpcboot1.exe

uuencode:
	find binary -name "hpcboot*.exe" -print | \
	awk '{ print "uuencode", $$1, $$1, ">", $$1".uu ; rm -f", $$1 }' | sh
	cp binary/build_number.h build_number.h
	awk '{if (/HPCBOOT_BUILD_NUMBER/)				\
		{ i = $$3 + 1;						\
			printf("#define HPCBOOT_BUILD_NUMBER	%d\n", i);\
		} else {						\
			printf("%s\n", $$0);				\
		}}' build_number.h > binary/build_number.h
	rm -f build_number.h
