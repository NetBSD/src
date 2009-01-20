SUBDIRS = src @RPM@

DIST_SUBDIRS = src rpm

EXTRA_DIST = bootstrap README NEWS depcomp ChangeLog ChangeLog.old

MAINTAINERCLEANFILES = ChangeLog

$(srcdir)/ChangeLog:
	@if test -d "$(srcdir)/CVS"; then \
	  $(srcdir)/misc/cvs2cl.pl --follow-only ipsec-tools-0_7-branch -I ChangeLog --utc -U $(srcdir)/misc/cvsusermap --group-by-author --fsf -T -l "-d2006-09-10<now" --tag-regexp "^ipsec-tools" --stdout > $@ ; \
	  echo "For older changes see ChangeLog.old" >> $@ ; \
	else \
	  echo "A CVS checkout and perl is required to generate ChangeLog" ; \
	  exit 1 ; \
	fi

.PHONY: $(srcdir)/ChangeLog
