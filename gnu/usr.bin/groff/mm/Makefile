#	$Id: Makefile,v 1.7 1994/08/22 19:54:44 pk Exp $

# XXX -- this depends on ../include/defs.h
TMACDIR?=	/usr/share/tmac

TMACOWN?=	bin
TMACGRP?=	bin
TMACMODE?=	444

FILES=		0.MT 5.MT 4.MT ms.cov se_ms.cov 
LOCALE=		locale se_locale

MAN7=		groff_mm.0 groff_mmse.0
MLINKS=		groff_mm.7 mm.7 groff_mmse.7 mmse.7

afterinstall:
	install -c -o $(TMACOWN) -g $(TMACGRP) -m $(TMACMODE) \
		$(.CURDIR)/tmac.m $(DESTDIR)$(TMACDIR)/tmac.m
	install -c -o $(TMACOWN) -g $(TMACGRP) -m $(TMACMODE) \
		$(.CURDIR)/tmac.m $(DESTDIR)$(TMACDIR)/tmac.mse

	for f in $(FILES); do \
		rm -f $(DESTDIR)$(TMACDIR)/mm/$$f; \
		install -c -o $(TMACOWN) -g $(TMACGRP) -m $(TMACMODE) \
			$(.CURDIR)/mm/$$f $(DESTDIR)$(TMACDIR)/mm/$$f; \
	done
	for f in $(LOCALE); do \
		if test ! -f $(DESTDIR)$(TMACDIR)/mm/$$f; then \
			install -c -o $(TMACOWN) -g $(TMACGRP) -m $(TMACMODE) \
				/dev/null $(DESTDIR)$(TMACDIR)/mm/$$f; \
		fi; \
	done

.include "../Makefile.cfg"
.include <bsd.prog.mk>
