
.includes: ${top_builddir}/config.status
	ln -snf $(KERNEL_INCLUDE)/linux
	touch .includes

all: .includes

EXTRA_DIST = \
  glibc-bugs.h \
  net/pfkeyv2.h \
  netinet/ipsec.h \
  sys/queue.h

DISTCLEANFILES = .includes linux
