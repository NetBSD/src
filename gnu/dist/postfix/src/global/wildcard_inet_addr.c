/* System library. */

#include <sys_defs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#ifdef INET6
#include <sys/socket.h>
#endif
#include <netdb.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <inet_addr_list.h>
#include <inet_addr_local.h>
#include <inet_addr_host.h>
#include <stringops.h>

/* Global library. */

#include <mail_params.h>
#include <wildcard_inet_addr.h>

/* Application-specific. */
static INET_ADDR_LIST addr_list;

/* wildcard_inet_addr_init - initialize my own address list */

static void wildcard_inet_addr_init(INET_ADDR_LIST *addr_list)
{
#ifdef INET6
    struct addrinfo hints, *res, *res0;
    char hbuf[NI_MAXHOST];
    int error;
#ifdef NI_WITHSCOPEID
    const int niflags = NI_NUMERICHOST | NI_WITHSCOPEID;
#else
    const int niflags = NI_NUMERICHOST;
#endif

    inet_addr_list_init(addr_list);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    error = getaddrinfo(NULL, "0", &hints, &res0);
    if (error)
	msg_fatal("could not get list of wildcard addresses");
    for (res = res0; res; res = res->ai_next) {
	if (getnameinfo(res->ai_addr, res->ai_addrlen, hbuf, sizeof(hbuf),
	    NULL, 0, niflags) != 0)
	    continue;
	if (inet_addr_host(addr_list, hbuf) == 0)
	    msg_fatal("config variable %s: host not found: %s",
		      VAR_INET_INTERFACES, hbuf);
    }
    freeaddrinfo(res0);
#else
    if (inet_addr_host(addr_list, "0.0.0.0") == 0)
	msg_fatal("config variable %s: host not found: %s",
		  VAR_INET_INTERFACES, "0.0.0.0");
#endif
}

/* wildcard_inet_addr_list - return list of addresses */

struct INET_ADDR_LIST *wildcard_inet_addr_list(void)
{
    if (addr_list.used == 0)
	wildcard_inet_addr_init(&addr_list);

    return (&addr_list);
}
