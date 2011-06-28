#
# acpixtract - extract binary ACPI tables from acpidump text output
#
# NOTE: This makefile is intended to be used in the Linux environment,
# with the Linux directory structure. It will not work directly
# on the native ACPICA source tree.
#

#
# Configuration
# Notes:
#   gcc should be version 4 or greater, otherwise some of the options
#       used will not be recognized.
#   Global optimization flags (such as -O2, -Os) are not used, since
#       they cause issues on some compilers.
#   The _GNU_SOURCE symbol is required for many hosts.
#
PROG = acpixtract

HOST =          _LINUX
NOMAN =         YES
COMPILE =       $(CC) -c $(CFLAGS) $(CWARNINGFLAGS) -o$@ $?

ACPICA_COMPONENTS =
ACPICA_SRC =    ../..
ACPICA_COMMON = $(ACPICA_SRC)/common
ACPICA_CORE =   $(ACPICA_SRC)$(ACPICA_COMPONENTS)
ACPICA_TOOLS =  $(ACPICA_SRC)/tools
ACPICA_OSL =    $(ACPICA_SRC)/os_specific/service_layers
INSTALLDIR =    /usr/bin
INSTALLPROG =   cp --remove-destination $(PROG) $(INSTALLDIR)

CFLAGS+= \
    -D$(HOST) \
    -D_GNU_SOURCE \
    -I$(ACPICA_SRC)/include

CWARNINGFLAGS = \
    -ansi \
    -Wall \
    -Wbad-function-cast \
    -Wdeclaration-after-statement \
    -Werror \
    -Wformat=2 \
    -Wmissing-declarations \
    -Wmissing-prototypes \
    -Wstrict-aliasing=0 \
    -Wstrict-prototypes \
    -Wswitch-default \
    -Wpointer-arith \
    -Wundef

OBJS = \
    acpixtract.o

#
# Root rule
#
$(PROG) : $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $(PROG)

#
# acpixtract source
#
acpixtract.o :          $(ACPICA_TOOLS)/acpixtract/acpixtract.c
	$(COMPILE)


clean :
	rm -f $(PROG) $(PROG).exe $(OBJS)

install :
	$(INSTALLPROG)
