/* IQ2000 target configuration file.  -*- C -*- */

/* Define this if the simulator can vary the size of memory.
   See the xxx simulator for an example.
   This enables the `-m size' option.
   The memory size is stored in STATE_MEM_SIZE.  */
/* Not used for IQ2000 since we use the memory module. TODO -- check this  */
/* #define SIM_HAVE_MEM_SIZE */

/* See sim-hload.c.  We properly handle LMA. -- TODO: check this */
#define SIM_HANDLES_LMA 1

/* For MSPR support.  FIXME: revisit.  */
#define WITH_DEVICES 0

#if 0
/* Enable watchpoints.  */
#define WITH_WATCHPOINTS 1
#endif

/* ??? Temporary hack until model support unified.  */
#define SIM_HAVE_MODEL

/* Define this to enable the intrinsic breakpoint mechanism. */
/* FIXME: may be able to remove SIM_HAVE_BREAKPOINTS since it essentially
   duplicates ifdef SIM_BREAKPOINT (right?) */
#if 1
#define SIM_HAVE_BREAKPOINTS
#define SIM_BREAKPOINT { 0, 0, 0, 0xD }
#define SIM_BREAKPOINT_SIZE 4
#endif

/* This is a global setting.  Different cpu families can't mix-n-match -scache
   and -pbb.  However some cpu families may use -simple while others use
   one of -scache/-pbb. ???? */
#define WITH_SCACHE_PBB 1
