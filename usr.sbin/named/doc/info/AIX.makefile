Received: by gw.home.vix.com id AA16867; Tue, 12 Jul 94 06:57:41 -0700
Received: (from ben@localhost) by mercure.inserm.fr (8.6.8/8.6.6) id PAA09612; Tue, 12 Jul 1994 15:57:09 +0200
Date: Tue, 12 Jul 1994 15:57:09 +0200
From: Benoit Grange <ben@tolbiac.inserm.fr>
Message-Id: <199407121357.PAA09612@mercure.inserm.fr>
To: paul@vix.com
Subject: Re: Make fails building bind on AIX


> what do you mean "separate" the all and clean targets?

In the Makefile, instead of :

----
all clean depend:: FRC
	@for x in $(SUBDIRS); do \
		(cd $$x; pwd; $(MAKE) $(MARGS) $@); \
	done

clean:: FRC
	-test -d doc/bog && (cd doc/bog; pwd; $(MAKE) $(MARGS) $@)
	(cd conf; rm -f *~ *.CKP *.BAK *.orig)
	rm -f *~ *.CKP *.BAK *.orig
...
----
I write :
----

all :: FRC
	@for x in $(SUBDIRS); do \
		(cd $$x; pwd; $(MAKE) $(MARGS) $@); \
	done

clean:: FRC
	@for x in $(SUBDIRS); do \
		(cd $$x; pwd; $(MAKE) $(MARGS) $@); \
	done
	-test -d doc/bog && (cd doc/bog; pwd; $(MAKE) $(MARGS) $@)
	(cd conf; rm -f *~ *.CKP *.BAK *.orig)
	rm -f *~ *.CKP *.BAK *.orig

depend:: FRC
	@for x in $(SUBDIRS); do \
		(cd $$x; pwd; $(MAKE) $(MARGS) $@); \
	done

--------------------

Anyway, all this is because of the buggy make supplied
with AIX.

Benoit Grange.
