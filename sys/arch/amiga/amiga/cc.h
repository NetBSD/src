/* -------------------------------------------------- 
 |  NAME
 |    cc
 |  PURPOSE
 |    
 |  NOTES
 |    
 |  HISTORY
 |    chopps - Oct 22, 1993: Created.
 +--------------------------------------------------- */

#if ! defined (_CC_H)
#define _CC_H

#include <sys/types.h>
#include "cc_types.h"
#include "dlists.h"
#include "custom.h"

#include "cc_vbl.h"
#include "cc_audio.h"
#include "cc_chipmem.h"
#include "cc_copper.h"
#include "cc_blitter.h"

void custom_chips_init (void);
#if ! defined (HIADDR)
#define HIADDR(x) \
        (u_word)((((unsigned long)(x))>>16)&0xffff)
#endif 
#if ! defined (LOADDR)
#define LOADDR(x) \
        (u_word)(((unsigned long)(x))&0xffff)
#endif

#if defined (AMIGA_TEST)
void cc_deinit (void);
int  nothing (void);
void amiga_test_panic (char *,...);
#define splhigh nothing
#define splx(x) 
#define panic amiga_test_panic
#endif

#endif /* _CC_H */

