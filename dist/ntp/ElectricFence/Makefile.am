# Harlan Stenn <stenn@whimsy.udel.edu>
# Converted the original Makefile (now in Makefile-) to Makefile.am
# in July of '00

EXTRA_DIST = libefence.3 CHANGES
libefence_a_SOURCES = efence.h efence.c page.c print.c
check_PROGRAMS = @EF_PROGS@
EXTRA_PROGRAMS = eftest tstheap
# TESTS = eftest tstheap
noinst_LIBRARIES = @EF_LIBS@
EXTRA_LIBRARIES = libefence.a
LDADD = libefence.a

# Un-comment the following if you are running HP/UX.
# CFLAGS= -Aa -g -D_HPUX_SOURCE -DPAGE_PROTECTION_VIOLATED_SIGNAL=SIGBUS

# Un-comment the following if you are running AIX. This makes sure you won't
# get the shared-library malloc() rather than the Electric Fence malloc().
# COMPILE THE PROGRAMS YOU ARE DEBUGGING WITH THESE FLAGS, TOO.
# CFLAGS= -g -bnso -bnodelcsect -bI:/lib/syscalls.exp

# Un-comment the following if you are running SunOS 4.X
# Note the definition of PAGE_PROTECTION_VIOLATED_SIGNAL. This may vary
# depend on what version of Sun hardware you have.
# You'll probably have to link the program you are debugging with -Bstatic
# as well if using Sun's compiler, -static if using GCC.
# CFLAGS= -g -Bstatic -DPAGE_PROTECTION_VIOLATED_SIGNAL=SIGBUS

check-local:
	test ! -f eftest  || ./eftest
	test ! -f tstheap || ./tstheap 3072
