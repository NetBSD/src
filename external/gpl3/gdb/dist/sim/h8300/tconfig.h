/* h8300 target configuration file.  */

/* Define this if the simulator uses an instruction cache.
   See the h8/300 simulator for an example.
   This enables the `-c size' option to set the size of the cache.
   The target is required to provide sim_set_simcache_size.  */
#define SIM_HAVE_SIMCACHE

/* FIXME: This is a quick hack for run.c so it can support the `-h' option.
   It will eventually be replaced by a more general facility.  */
#define SIM_H8300
