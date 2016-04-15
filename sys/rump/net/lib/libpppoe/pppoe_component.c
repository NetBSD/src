
#include <sys/cdefs.h>

#include <sys/param.h>

#include <rump-sys/kern.h>

int pppoeattach(int);

RUMP_COMPONENT(RUMP_COMPONENT_NET_IF)
{
	pppoeattach(0);
}
