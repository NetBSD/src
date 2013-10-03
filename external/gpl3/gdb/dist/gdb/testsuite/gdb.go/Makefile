
srcdir = .

EXECUTABLES = \
	chan handcall hello integers methods package \
	strings types unsafe

all info install-info dvi install uninstall installcheck check:
	@echo "Nothing to be done for $@..."

clean mostlyclean:
	-find . -name '*.o' -print | xargs rm -f
	-find . -name '*.ali' -print | xargs rm -f
	-rm -f *~ a.out
	-rm -f *.dwo *.dwp
	-rm -f core core.coremaker coremaker.core corefile $(EXECUTABLES)

distclean maintainer-clean realclean: clean
	-rm -f Makefile config.status config.log
	-rm -f *-init.exp gdb.log gdb.sum
	-rm -fr *.log summary detail *.plog *.sum *.psum site.*
