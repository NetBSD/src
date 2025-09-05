sp := $(sp).x
dirstack_$(sp) := $(d)
d := $(dir)

.PHONY: test

CLEAN += clients servers tests/progs tests/schema tests/testdata tests/testrun

test: all clients servers tests/progs

test:
	cd tests; \
		SRCDIR=$(abspath $(LDAP_SRC)) \
		LDAP_BUILD=$(abspath $(LDAP_BUILD)) \
		TOPDIR=$(abspath $(SRCDIR)) \
		LIBTOOL=$(abspath $(LIBTOOL)) \
			$(abspath $(SRCDIR))/tests/run test001-ciboolean

servers clients tests/progs:
	ln -s $(abspath $(LDAP_BUILD))/$@ $@

d := $(dirstack_$(sp))
sp := $(basename $(sp))
