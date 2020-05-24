
#include <isc/platform.h>
#include <isc/util.h>
#include <ns/types.h>
#include <ns/client.h>

#include <blacklist.h>

#include <ns/pfilter.h>

static struct blacklist *blstate;
static int blenable;

void
pfilter_enable(void) {
	blenable = 1;
}

#define TCP_CLIENT(c)  (((c)->attributes & NS_CLIENTATTR_TCP) != 0)

void
pfilter_notify(isc_result_t res, ns_client_t *client, const char *msg)
{
	int fd;

	if (!blenable)
		return;

	if (blstate == NULL)
		blstate = blacklist_open();

	if (blstate == NULL)
		return;

	if (!TCP_CLIENT(client) && !client->peeraddr_valid)
		return;

	if ((fd = isc_nmhandle_getfd(client->handle)) == -1)
		return;

	blacklist_sa_r(blstate, 
	    res != ISC_R_SUCCESS, fd,
	    &client->peeraddr.type.sa, client->peeraddr.length, msg);
}
