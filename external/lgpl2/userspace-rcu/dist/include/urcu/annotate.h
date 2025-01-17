// SPDX-FileCopyrightText: 2023 Olivier Dion <odion@efficios.com>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef _URCU_ANNOTATE_H
#define _URCU_ANNOTATE_H

/*
 * urcu/annotate.h
 *
 * Userspace RCU - annotation header.
 */

/*
 * WARNING!
 *
 * This API is highly experimental. There is zero guarantees of stability
 * between releases.
 *
 * You have been warned.
 */

#include <stdio.h>
#include <stdlib.h>

#include <urcu/compiler.h>

enum cmm_annotate {
	CMM_ANNOTATE_VOID,
	CMM_ANNOTATE_LOAD,
	CMM_ANNOTATE_STORE,
	CMM_ANNOTATE_MB,
};

typedef enum cmm_annotate cmm_annotate_t __attribute__((unused));

#define cmm_annotate_define(name)		\
	cmm_annotate_t name = CMM_ANNOTATE_VOID

#ifdef CMM_SANITIZE_THREAD

# ifdef __cplusplus
extern "C" {
# endif
extern void __tsan_acquire(void *);
extern void __tsan_release(void *);
# ifdef __cplusplus
}
# endif

# define cmm_annotate_die(msg)						\
	do {								\
		fprintf(stderr,						\
			"(" __FILE__ ":%s@%u) Annotation ERROR: %s\n",	\
			__func__, __LINE__, msg);			\
		abort();						\
	} while (0)

/* Only used for typechecking in macros. */
static inline cmm_annotate_t cmm_annotate_dereference(cmm_annotate_t *group)
{
	return *group;
}

# define cmm_annotate_group_mb_acquire(group)				\
	do {								\
		switch (cmm_annotate_dereference(group)) {		\
		case CMM_ANNOTATE_VOID:					\
			break;						\
		case CMM_ANNOTATE_LOAD:					\
			break;						\
		case CMM_ANNOTATE_STORE:				\
			cmm_annotate_die("store for acquire group");	\
			break;						\
		case CMM_ANNOTATE_MB:					\
			cmm_annotate_die(				\
				"redundant mb for acquire group"	\
					);				\
			break;						\
		}							\
		*(group) = CMM_ANNOTATE_MB;				\
	} while (0)

# define cmm_annotate_group_mb_release(group)				\
	do {								\
		switch (cmm_annotate_dereference(group)) {		\
		case CMM_ANNOTATE_VOID:					\
			break;						\
		case CMM_ANNOTATE_LOAD:					\
			cmm_annotate_die("load before release group");	\
			break;						\
		case CMM_ANNOTATE_STORE:				\
			cmm_annotate_die(				\
				"store before release group"		\
					);				\
			break;						\
		case CMM_ANNOTATE_MB:					\
			cmm_annotate_die(				\
				"redundant mb of release group"		\
					);				\
			break;						\
		}							\
		*(group) = CMM_ANNOTATE_MB;				\
	} while (0)

# define cmm_annotate_group_mem_acquire(group, mem)			\
	do {								\
		__tsan_acquire((void*)(mem));				\
		switch (cmm_annotate_dereference(group)) {		\
		case CMM_ANNOTATE_VOID:					\
			*(group) = CMM_ANNOTATE_LOAD;			\
			break;						\
		case CMM_ANNOTATE_MB:					\
			cmm_annotate_die(				\
				"load after mb for acquire group"	\
					);				\
			break;						\
		default:						\
			break;						\
		}							\
	} while (0)

# define cmm_annotate_group_mem_release(group, mem)		\
	do {							\
		__tsan_release((void*)(mem));			\
		switch (cmm_annotate_dereference(group)) {	\
		case CMM_ANNOTATE_MB:				\
			break;					\
		default:					\
			cmm_annotate_die(			\
				"missing mb for release group"	\
					);			\
		}						\
	} while (0)

# define cmm_annotate_mem_acquire(mem)		\
	__tsan_acquire((void*)(mem))

# define cmm_annotate_mem_release(mem)		\
	__tsan_release((void*)(mem))
#else

# define cmm_annotate_group_mb_acquire(group)	\
	(void) (group)

# define cmm_annotate_group_mb_release(group)	\
	(void) (group)

# define cmm_annotate_group_mem_acquire(group, mem)	\
	(void) (group)

# define cmm_annotate_group_mem_release(group, mem)	\
	(void) (group)

# define cmm_annotate_mem_acquire(mem)		\
	do { } while (0)

# define cmm_annotate_mem_release(mem)		\
	do { } while (0)

#endif  /* CMM_SANITIZE_THREAD */

#endif	/* _URCU_ANNOTATE_H */
