/*	$NetBSD: osf1_util.h,v 1.3 1998/05/20 16:34:29 chs Exp $	*/

#ifndef _OSF1_OSF1_UTIL_H_
#define _OSF1_OSF1_UTIL_H_

#include <compat/common/compat_util.h>

extern const char osf1_emul_path[];

#define OSF1_CHECK_ALT_EXIST(p, sgp, path) \
    CHECK_ALT_EXIST(p, sgp, osf1_emul_path, path)

#endif /* _OSF1_OSF1_UTIL_H_ */
