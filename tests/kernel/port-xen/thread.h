/* $NetBSD: thread.h,v 1.1.2.1 2011/06/03 13:27:44 cherry Exp $ */

#include <stdbool.h>

struct thread_ctx;

typedef void (*threadcb_t)(void *);
enum threadtype {
	TYPE_FORK,
	TYPE_PTHREAD
};

void thread_init(void);
void thread_fini(void);
inline bool thread_equal(struct thread_ctx *, struct thread_ctx *);
struct thread_ctx * thread_spawn(cpuid_t, enum threadtype, threadcb_t, void *, threadcb_t);
void prep_fault(bool);
struct thread_ctx * thread_self(void);
cpuid_t thread_cid(struct thread_ctx *);
void thread_wait(struct thread_ctx *);
