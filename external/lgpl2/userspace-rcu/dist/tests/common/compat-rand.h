// SPDX-FileCopyrightText: 1996 Ulrich Drepper <drepper@cygnus.com>
// SPDX-FileCopyrightText: 2013 Pierre-Luc St-Charles <pierre-luc.st-charles@polymtl.ca>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef _COMPAT_RAND_H
#define _COMPAT_RAND_H

/*
 * Userspace RCU library - rand/rand_r Compatibility Header
 *
 * Note: this file is only used to simplify the code required to
 * use the 'rand_r(...)' system function across multiple platforms,
 * which might not always be referenced the same way.
 */

#ifndef HAVE_RAND_R
/*
 * Reentrant random function from POSIX.1c.
 * Copyright (C) 1996, 1999 Free Software Foundation, Inc.
 * This file is part of the GNU C Library.
 * Contributed by Ulrich Drepper <drepper@cygnus.com <mailto:drepper@cygnus.com>>, 1996.
 */
static inline int rand_r(unsigned int *seed)
{
       unsigned int next = *seed;
       int result;

       next *= 1103515245;
       next += 12345;
       result = (unsigned int) (next / 65536) % 2048;

       next *= 1103515245;
       next += 12345;
       result <<= 10;
       result ^= (unsigned int) (next / 65536) % 1024;

       next *= 1103515245;
       next += 12345;
       result <<= 10;
       result ^= (unsigned int) (next / 65536) % 1024;

       *seed = next;

       return result;
}
#endif /* HAVE_RAND_R */

#endif /* _COMPAT_RAND_H */
