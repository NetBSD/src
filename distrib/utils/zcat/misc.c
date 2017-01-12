#include <assert.h>
#include <signal.h>

/* Avoid stdio */
__dead void __assert(const char *a, int b, const char *c) {
	kill(0, SIGQUIT);
}
__dead void __assert13(const char *a, int b, const char *c, const char *d) {
	kill(0, SIGQUIT);
}
void __diagassert(const char *a, int b, const char *x) {
	kill(0, SIGQUIT);
}
void __diagassert13(const char * a, int b, const char *c, const char *d) {
	kill(0, SIGQUIT);
}

/* Avoid mutexes environment rbree, thread stuff */
void _libc_init(void);
void _libc_init(void) {
}

/* Avoid finalizers, etc. */
int atexit(void (*)(void));

int atexit(void (*p)(void)) {
	return 0;
}

void __cxa_finalize(void *);
void __cxa_finalize(void *dso) { }

int __cxa_atexit(void (*func)(void *), void *arg, void *dso);
int
__cxa_atexit(void (*func)(void *), void *arg, void *dso)
{
	return 0;
}
