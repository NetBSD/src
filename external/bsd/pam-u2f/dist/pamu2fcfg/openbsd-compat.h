/*
 * Copyright (C) 2021 Yubico AB - See COPYING
 */

#ifndef _OPENBSD_COMPAT_H
#define _OPENBSD_COMPAT_H

#include <string.h>

#ifndef HAVE_STRLCPY
size_t strlcpy(char *, const char *, size_t);
#endif /* HAVE_STRLCPY */

#ifndef HAVE_READPASSPHRASE
#include "_readpassphrase.h"
#else
#include <readpassphrase.h>
#endif /* HAVE_READPASSPHRASE */

#endif /* !_STRLCPY_H_ */
