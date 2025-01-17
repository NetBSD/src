// SPDX-FileCopyrightText: 2012 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef _URCU_TLS_COMPAT_H
#define _URCU_TLS_COMPAT_H

/*
 * Userspace RCU library - Thread-Local Storage Compatibility Header
 */

#include <stdlib.h>
#include <urcu/config.h>
#include <urcu/compiler.h>
#include <urcu/arch.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_RCU_TLS

/*
 * Default to '__thread' on all C and C++ compilers except MSVC. While C11 has
 * '_Thread_local' and C++11 has 'thread_local', only '__thread' seems to have
 * a compatible implementation when linking public extern symbols across
 * language boundaries.
 *
 * For more details, see 'https://gcc.gnu.org/onlinedocs/gcc/Thread-Local.html'.
 */
#if defined(_MSC_VER)
# define URCU_TLS_STORAGE_CLASS	__declspec(thread)
#else
# define URCU_TLS_STORAGE_CLASS	__thread
#endif

/*
 * Hint: How to define/declare TLS variables of compound types
 *       such as array or function pointers?
 *
 * Answer: Use typedef to assign a type_name to the compound type.
 * Example: Define a TLS variable which is an int array with len=4:
 *
 * 	typedef int my_int_array_type[4];
 * 	DEFINE_URCU_TLS(my_int_array_type, var_name);
 *
 * Another example:
 * 	typedef void (*call_rcu_flavor)(struct rcu_head *, XXXX);
 * 	DECLARE_URCU_TLS(call_rcu_flavor, p_call_rcu);
 *
 * NOTE: URCU_TLS() is NOT async-signal-safe, you can't use it
 * inside any function which can be called from signal handler.
 *
 * But if pthread_getspecific() is async-signal-safe in your
 * platform, you can make URCU_TLS() async-signal-safe via:
 * ensuring the first call to URCU_TLS() of a given TLS variable of
 * all threads is called earliest from a non-signal handler function.
 *
 * Example: In any thread, the first call of URCU_TLS(rcu_reader)
 * is called from rcu_register_thread(), so we can ensure all later
 * URCU_TLS(rcu_reader) in any thread is async-signal-safe.
 *
 * Moreover, URCU_TLS variables should not be touched from signal
 * handlers setup with with sigaltstack(2).
 */

# define DECLARE_URCU_TLS(type, name)	\
	URCU_TLS_STORAGE_CLASS type name

# define DEFINE_URCU_TLS(type, name)	\
	URCU_TLS_STORAGE_CLASS type name

# define DEFINE_URCU_TLS_INIT(type, name, init)	\
	URCU_TLS_STORAGE_CLASS type name = (init)

# define URCU_TLS(name)		(name)

#else /* #ifndef CONFIG_RCU_TLS */

/*
 * The *_1() macros ensure macro parameters are expanded.
 */

# include <pthread.h>

struct urcu_tls {
	pthread_key_t key;
	pthread_mutex_t init_mutex;
	int init_done;
};

# define DECLARE_URCU_TLS_1(type, name)				\
	type *__tls_access_ ## name(void)
# define DECLARE_URCU_TLS(type, name)				\
	DECLARE_URCU_TLS_1(type, name)

/*
 * Note: we don't free memory at process exit, since it will be dealt
 * with by the OS.
 */
# define DEFINE_URCU_TLS_INIT_1(type, name, do_init)		\
	type *__tls_access_ ## name(void)			\
	{							\
		static struct urcu_tls __tls_ ## name = {	\
			.key = 0,				\
			.init_mutex = PTHREAD_MUTEX_INITIALIZER,\
			.init_done = 0,				\
		};						\
		__typeof__(type) *__tls_p;			\
		if (!__tls_ ## name.init_done) {		\
			/* Mutex to protect concurrent init */	\
			pthread_mutex_lock(&__tls_ ## name.init_mutex); \
			if (!__tls_ ## name.init_done) {	\
				(void) pthread_key_create(&__tls_ ## name.key, \
					free);			\
				cmm_smp_wmb();	/* create key before write init_done */ \
				__tls_ ## name.init_done = 1;	\
			}					\
			pthread_mutex_unlock(&__tls_ ## name.init_mutex); \
		}						\
		cmm_smp_rmb();	/* read init_done before getting key */ \
		__tls_p = (__typeof__(type) *) pthread_getspecific(__tls_ ## name.key); \
		if (caa_unlikely(__tls_p == NULL)) {		\
			__tls_p = (__typeof__(type) *) calloc(1, sizeof(type));	\
			do_init					\
			(void) pthread_setspecific(__tls_ ## name.key,	\
				__tls_p);			\
		}						\
		return __tls_p;					\
	}

# define _URCU_TLS_INIT(init)					\
	*__tls_p = (init);

# define DEFINE_URCU_TLS_INIT(type, name, init)			\
	DEFINE_URCU_TLS_INIT_1(type, name, _URCU_TLS_INIT(init))

# define DEFINE_URCU_TLS(type, name)				\
	DEFINE_URCU_TLS_INIT_1(type, name, /* empty */)

# define URCU_TLS_1(name)	(*__tls_access_ ## name())

# define URCU_TLS(name)		URCU_TLS_1(name)

#endif	/* #else #ifndef CONFIG_RCU_TLS */

#ifdef __cplusplus
}
#endif

#endif /* _URCU_TLS_COMPAT_H */
