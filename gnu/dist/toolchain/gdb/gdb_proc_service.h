typedef enum {
  PS_OK,          /* generic "call succeeded" */
  PS_ERR,         /* generic. */
  PS_BADPID,      /* bad process handle */
  PS_BADLID,      /* bad lwp identifier */
  PS_BADADDR,     /* bad address */
  PS_NOSYM,       /* p_lookup() could not find given symbol */
        PS_NOFREGS
  /*
   * FPU register set not available for given
   * lwp
   */
}       ps_err_e;

#ifndef HAVE_LWPID_T
typedef unsigned int  lwpid_t;
#endif

typedef unsigned long paddr_t;

#ifndef HAVE_PSADDR_T
typedef unsigned long psaddr_t;
#endif

#ifndef HAVE_PRGREGSET_T
typedef gregset_t  prgregset_t;		/* BOGUS BOGUS BOGUS */
#endif

#ifndef HAVE_PRFPREGSET_T
typedef fpregset_t prfpregset_t;	/* BOGUS BOGUS BOGUS */
#endif

struct ps_prochandle;		/* user defined. */
