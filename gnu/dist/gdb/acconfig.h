
/* Whether malloc must be declared even if <stdlib.h> is included.  */
#undef NEED_DECLARATION_MALLOC

/* Whether realloc must be declared even if <stdlib.h> is included.  */
#undef NEED_DECLARATION_REALLOC

/* Whether free must be declared even if <stdlib.h> is included.  */
#undef NEED_DECLARATION_FREE

/* Whether strerror must be declared even if <string.h> is included.  */
#undef NEED_DECLARATION_STRERROR
@TOP@

/* Define if fpregset_t type is available. */
#undef HAVE_FPREGSET_T

/* Define if gregset_t type is available. */
#undef HAVE_GREGSET_T

/* Define if the `long long' type works.  */
#undef CC_HAS_LONG_LONG

/* Define if the "ll" format works to print long long ints. */
#undef PRINTF_HAS_LONG_LONG

/* Define if the "%Lg" format works to print long doubles. */
#undef PRINTF_HAS_LONG_DOUBLE

/* Define if the "%Lg" format works to scan long doubles. */
#undef SCANF_HAS_LONG_DOUBLE

/* Define if using Solaris thread debugging.  */
#undef HAVE_THREAD_DB_LIB

/* Define on a GNU/Linux system to work around problems in sys/procfs.h.  */
#undef START_INFERIOR_TRAPS_EXPECTED
#undef sys_quotactl

/* Define if you have HPUX threads */
#undef HAVE_HPUX_THREAD_SUPPORT

/* Define if you want to use the memory mapped malloc package (mmalloc). */
#undef USE_MMALLOC

/* Define if the runtime uses a routine from mmalloc before gdb has a chance
   to initialize mmalloc, and we want to force checking to be used anyway.
   This may cause spurious memory corruption messages if the runtime tries
   to explicitly deallocate that memory when gdb calls exit. */
#undef MMCHECK_FORCE
