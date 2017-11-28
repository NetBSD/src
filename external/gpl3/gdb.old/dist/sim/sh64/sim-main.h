/* Main header for the Hitachi SH64 architecture.  */

#ifndef SIM_MAIN_H
#define SIM_MAIN_H

/* sim-basics.h includes config.h but cgen-types.h must be included before
   sim-basics.h and cgen-types.h needs config.h.  */
#include "config.h"

/* This is a global setting.  Different cpu families can't mix-n-match -scache
   and -pbb.  However some cpu families may use -simple while others use
   one of -scache/-pbb. ???? */
#define WITH_SCACHE_PBB 1

#include "symcat.h"
#include "sim-basics.h"
#include "cgen-types.h"
#include "sh-desc.h"
#include "sh-opc.h"
#include "arch.h"

/* These must be defined before sim-base.h.  */
typedef UDI sim_cia;
#define CIA_ADDR(cia) (cia)

#include "sim-base.h"
#include "cgen-sim.h"
#include "sh64-sim.h"

/* The _sim_cpu struct.  */

struct _sim_cpu {
  /* sim/common cpu base.  */
  sim_cpu_base base;

  /* Static parts of cgen.  */
  CGEN_CPU cgen_cpu;

  /* CPU specific parts go here.
     Note that in files that don't need to access these pieces WANT_CPU_FOO
     won't be defined and thus these parts won't appear.  This is ok in the
     sense that things work.  It is a source of bugs though.
     One has to of course be careful to not take the size of this
     struct and no structure members accessed in non-cpu specific files can
     go after here.  Oh for a better language.  */
#if defined (WANT_CPU_SH64)
  SH64_CPU_DATA cpu_data;
#endif
};

/* The sim_state struct.  */

struct sim_state {
  sim_cpu *cpu[MAX_NR_PROCESSORS];

  CGEN_STATE cgen_state;

  sim_state_base base;
};

/* Misc.  */

/* Catch address exceptions.  */
extern SIM_CORE_SIGNAL_FN sh64_core_signal;
#define SIM_CORE_SIGNAL(SD,CPU,CIA,MAP,NR_BYTES,ADDR,TRANSFER,ERROR) \
sh64_core_signal ((SD), (CPU), (CIA), (MAP), (NR_BYTES), (ADDR), \
		  (TRANSFER), (ERROR))

/* Default memory size.  */
#define SH64_DEFAULT_MEM_SIZE 0x800000 /* 8M */

#endif /* SIM_MAIN_H */
