SUBDIRS = suse

EXTRA_DIST = \
  ipsec-tools.spec.in

all-local: ipsec-tools.spec

## We borrow guile's convention and use @-...-@ as the substitution
## brackets here, instead of the usual @...@.  This prevents autoconf
## from substituting the values directly into the left-hand sides of
## the sed substitutions.  *sigh*
ipsec-tools.spec: ipsec-tools.spec.in Makefile
	rm -f $@.tmp
	sed < $< > $@.tmp \
	    -e 's:@-VERSION-@:${VERSION}:'
	mv $@.tmp $@

CLEANFILES = ipsec-tools.spec
