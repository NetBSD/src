#ifndef _LINUX_ASYNC_H_
#define _LINUX_ASYNC_H_

#include <sys/param.h> /* panic */

typedef struct async_cookie_t {

} async_cookie_t;

static inline void
async_schedule(void (*func)(void *, async_cookie_t), void *cookie)
{
	panic("XXX defer function");
}

#endif /* _LINUX_ASYNC_H_ */
