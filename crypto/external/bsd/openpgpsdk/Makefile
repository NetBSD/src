# order of build of subdirs is important
#
SUBDIRS=src tests

all: Makefiles include/openpgpsdk/configure.h headers default

headers:
	cd include/openpgpsdk && $(MAKE) headers

#(cd $$d; echo "+++ make in $$d"; $(MAKE) -wS; echo "--- $$d"); \

default:
	set -e; for d in $(SUBDIRS); do \
	cd $$d; $(MAKE) || exit 1; cd ..;\
	done

include/openpgpsdk/configure.h: include/openpgpsdk/configure.h.template configure
	echo re-run configure && exit 1

force_depend:
	@set -e; for d in $(SUBDIRS); do \
	(cd $$d; echo "+++ make force_depend in $$d"; $(MAKE) force_depend ; echo "--- $$d"); \
	done

clean:
	@set -e; for d in $(SUBDIRS); do \
	(cd $$d; echo "+++ make clean in $$d"; $(MAKE) clean; echo "--- $$d"); \
	done
	find . -name '*.core' | xargs rm -f
	rm -rf oink-links
	rm -f lib/*

Makefiles:
	@set -e; for d in $(SUBDIRS); do \
	(cd $$d; echo "+++ make Makefile in $$d"; $(MAKE) Makefile; echo "--- $$d"); \
	done

tags:
	rm -f TAGS
	find . -name '*.[ch]' | xargs etags

test::
	make
	cd tests && $(MAKE) && ./tests 

doc::
	cd doc && $(MAKE)

coverity::
	cov-build --dir coverity make
	cd coverity && cov-analyze -e emit/ --outputdir output/ --enable VOLATILE --security --enable CHROOT --enable OPEN_ARGS --enable SECURE_CODING --enable SECURE_TEMP --enable TAINTED_POINTER --enable TOCTTOU && cov-commit-errors -e ./emit -o ./output -d /home/rachel/openpgpsdk/coverity/database/ --name ben

oink:
	rm -rf oink-links
	mkdir oink-links
	cd oink-links \
	&& find ../src ../examples -name '*.[ihc]' -exec ln -s {} \; \
	&& ln -s ../util/Makefile.oink Makefile
