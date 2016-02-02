
srcdir = .

.PHONY: all clean mostlyclean distclean realclean

PROGS = 

all info install-info dvi install uninstall installcheck check:
	@echo "Nothing to be done for $@..."

clean mostlyclean:
	-rm -f *.o *.diff *~ core $(PROGS)

distclean maintainer-clean realclean: clean
	-rm -f Makefile config.status config.log gdb.log gdb.sum
