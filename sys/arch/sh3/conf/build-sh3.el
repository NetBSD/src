#! /bin/sh

# just helping for cross compilation.

MACHINE=evbsh3
MACHINE_ARCH=sh3el
MACHINE_CPU=sh3
# just in case you forgot to specify this when you built gcc...
CFLAGS="-D__NetBSD__"
export MACHINE MACHINE_ARCH CFLAGS
# modify following TARGET name to meet your environment
TARGET=shel-unknown-netbsdcoff

# XXX following definition is absolutely incorrect.
# Our cpp is not /usr/libexec/cpp but /usr/bin/cpp.
# True solution is:
#	1) copy /usr/bin/cpp into /usr/local/bin/shel-*-*-cpp
#	2) Edit CPP in shel-*-*-cpp correctly
#		(e.g. "CPP=`$CC -print-prog-name=cpp`").
#	3) Edit STDINCDIR in shel-*-*-cpp correctly
#		(e.g. STDINCDIR=/usr/local/shel-unknown-netbsdcoff/include)

# BROKEN
CPP=`$TARGET-gcc -print-prog-name=cpp`

# EXAMPLE
#CPP=/usr/local/bin/shel-unknown-netbsdcoff-cpp
#CPP=/usr/local/bin/shel-unknown-netbsdelf-cpp

make AR=$TARGET-ar AS="$TARGET-as -little" CC=$TARGET-gcc LD=$TARGET-ld NM=$TARGET-nm \
	RANLIB=$TARGET-ranlib SIZE=$TARGET-size \
	STRIP=$TARGET-strip OBJCOPY=$TARGET-objcopy \
	CXX=$TARGET-c++ CPP=$CPP $*
