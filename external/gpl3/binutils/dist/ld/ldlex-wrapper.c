/* The flex output (ldlex.c) includes stdio.h before any of the C code
   in ldlex.l.  Make sure we include sysdep.h first, so that config.h
   can select the correct value of things like _FILE_OFFSET_BITS and
   _LARGE_FILES.  */
#include "sysdep.h"
#include "ldlex.c"
