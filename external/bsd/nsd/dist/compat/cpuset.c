/*
 * cpuset.c -- CPU affinity.
 *
 * Copyright (c) 2020, NLnet Labs. All rights reserved.
 *
 * See LICENSE for the license.
 *
 */
#include "config.h"
#include "cpuset.h"

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef HAVE_CPUSET_CREATE
cpuset_t *cpuset_create(void)
{
  cpuset_t *set = calloc(1, sizeof(*set));
  return set;
}
#endif /* !HAVE_CPUSET_CREATE */

#ifndef HAVE_CPUSET_DESTROY
void cpuset_destroy(cpuset_t *set)
{
  free(set);
}
#endif /* !HAVE_CPUSET_DESTROY */

#ifndef HAVE_CPUSET_ZERO
void cpuset_zero(cpuset_t *set)
{
  CPU_ZERO(set);
}
#endif /* !HAVE_CPUSET_ZERO */

#ifndef HAVE_CPUSET_SET
int cpuset_set(cpuid_t cpu, cpuset_t *set)
{
  CPU_SET(cpu, set);
  return 0;
}
#endif /* !HAVE_CPUSET_SET */

#ifndef HAVE_CPUSET_CLR
int cpuset_clr(cpuid_t cpu, cpuset_t *set)
{
  CPU_CLR(cpu, set);
  return 0;
}
#endif /* !HAVE_CPUSET_CLR */

#ifndef HAVE_CPUSET_ISSET
int cpuset_isset(cpuid_t cpu, const cpuset_t *set)
{
  return CPU_ISSET(cpu, set);
}
#endif /* !HAVE_CPUSET_ISSET */

#ifndef HAVE_CPUSET_SIZE
size_t cpuset_size(const cpuset_t *set)
{
  return sizeof(*set);
}
#endif /* !HAVE_CPUSET_SIZE */

#ifdef CPU_OR_THREE_ARGS
/* for Linux, use three arguments */
void cpuset_or(cpuset_t *destset, const cpuset_t *srcset)
{
  CPU_OR(destset, destset, srcset);
}
#else
/* for FreeBSD, use two arguments */
void cpuset_or(cpuset_t *destset, const cpuset_t *srcset)
{
  CPU_OR(destset, srcset);
}
#endif
