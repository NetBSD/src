#include "ieeefp.h"

fp_rnd    fpgetround(void);
fp_rnd    fpsetround(fp_rnd);
fp_except fpgetmask(void);
fp_except fpsetmask(fp_except);
fp_except fpgetsticky(void);
fp_except fpsetsticky(fp_except);
