#include <config.h>

#include <isc/platform.h>
#include <isc/util.h>
#include <named/types.h>
#include <named/client.h>

#include <blacklist.h>

#include "pfilter.h"

static struct blacklist *blstate;

void
pfilter_open(void)
{
	if (blstate == NULL)
		blstate = blacklist_open();
}

#define TCP_CLIENT(c)  (((c)->attributes & NS_CLIENTATTR_TCP) != 0)

void
pfilter_notify(isc_result_t res, ns_client_t *client, const char *msg)
{
	int rv;
	isc_socket_t *socket;

	pfilter_open();

	if (TCP_CLIENT(client))
		socket = client->tcpsocket;
	else {
		socket = client->udpsocket;
		if (!client->peeraddr_valid) {
			syslog(LOG_ERR, "no peer res=%d\n", res);
			return;
		}
	}
	if (socket == NULL) {
		syslog(LOG_ERR, "null socket res=%d\n", res);
		return;
	}
	if (blstate == NULL) {
		syslog(LOG_ERR, "null blstate res=%d\n", res);
		return;
	}
	rv = blacklist_sa_r(blstate, 
	    res != ISC_R_SUCCESS, isc_socket_getfd(socket),
	    &client->peeraddr.type.sa, client->peeraddr.length, msg);
	if (rv || res != ISC_R_SUCCESS)
		syslog(LOG_ERR, "blacklist rv=%d res=%d\n", rv, res);
}
