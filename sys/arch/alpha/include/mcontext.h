
#ifndef _ALPHA_MCONTEXT_H_
#define _ALPHA_MCONTEXT_H_

/* In Digital Unix, mcontext_t is also struct sigcontext */

#include <machine/signal.h>

typedef struct sigcontext mcontext_t;

#define _UC_MACHINE_PAD 1 /* XXX check */

#endif	/* !_ALPHA_MCONTEXT_H_ */
