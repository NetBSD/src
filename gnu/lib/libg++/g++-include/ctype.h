//	$Id: ctype.h,v 1.2 1993/08/02 17:22:03 mycroft Exp $

#include <_G_config.h>
extern "C" {
#include_next <ctype.h>
#ifndef toupper
extern int toupper _G_ARGS((int));
#endif
#ifndef tolower
extern int tolower _G_ARGS((int));
#endif
}
