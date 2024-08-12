/* Main header for the m32r.  */

#ifndef SIM_MAIN_H
#define SIM_MAIN_H

/* This is a global setting.  Different cpu families can't mix-n-match -scache
   and -pbb.  However some cpu families may use -simple while others use
   one of -scache/-pbb.  */
#define WITH_SCACHE_PBB 1

#include "symcat.h"
#include "sim-basics.h"
#include "cgen-types.h"
#include "m32r-desc.h"
#include "m32r-opc.h"
#include "arch.h"
#include "sim-base.h"
#include "cgen-sim.h"
#include "m32r-sim.h"
#include "opcode/cgen.h"

/* The _sim_cpu struct.  */

struct _sim_cpu {
  /* sim/common cpu base.  */
  sim_cpu_base base;

  /* Static parts of cgen.  */
  CGEN_CPU cgen_cpu;

  M32R_MISC_PROFILE m32r_misc_profile;
#define CPU_M32R_MISC_PROFILE(cpu) (& (cpu)->m32r_misc_profile)

  /* CPU specific parts go here.
     Note that in files that don't need to access these pieces WANT_CPU_FOO
     won't be defined and thus these parts won't appear.  This is ok in the
     sense that things work.  It is a source of bugs though.
     One has to of course be careful to not take the size of this
     struct and no structure members accessed in non-cpu specific files can
     go after here.  Oh for a better language.  */
#if defined (WANT_CPU_M32RBF)
  M32RBF_CPU_DATA cpu_data;
#endif
#if defined (WANT_CPU_M32RXF)
  M32RXF_CPU_DATA cpu_data;
#elif defined (WANT_CPU_M32R2F)
  M32R2F_CPU_DATA cpu_data;
#endif
};

/* Misc.  */

/* Catch address exceptions.  */
extern SIM_CORE_SIGNAL_FN m32r_core_signal;
#define SIM_CORE_SIGNAL(SD,CPU,CIA,MAP,NR_BYTES,ADDR,TRANSFER,ERROR) \
m32r_core_signal ((SD), (CPU), (CIA), (MAP), (NR_BYTES), (ADDR), \
		  (TRANSFER), (ERROR))

#endif /* SIM_MAIN_H */
