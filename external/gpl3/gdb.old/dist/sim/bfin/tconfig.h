/* Blackfin target configuration file.  -*- C -*- */

/* See sim-hload.c.  We properly handle LMA. -- TODO: check this */
#define SIM_HANDLES_LMA 1

/* We use this so that we are passed the requesting CPU for HW acesses.
   Common sim core by default sets hw_system_cpu to NULL for WITH_HW.  */
#define WITH_DEVICES 1

/* ??? Temporary hack until model support unified.  */
#define SIM_HAVE_MODEL

/* Allows us to do the memory aliasing that some bfroms have:
   {0xef000000 - 0xef100000} => {0xef000000 - 0xef000800}  */
#define WITH_MODULO_MEMORY 1
