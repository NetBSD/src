/*	$NetBSD: client.c,v 1.13 2015/07/08 17:28:55 christos Exp $	*/

/*
 * Copyright (C) 2004-2015  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 1999-2003  Internet Software Consortium.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/* Id: client.c,v 1.286 2012/01/31 23:47:30 tbox Exp  */

#include <config.h>

#include <isc/formatcheck.h>
#include <isc/mutex.h>
#include <isc/once.h>
#include <isc/platform.h>
#include <isc/print.h>
#include <isc/queue.h>
#include <isc/random.h>
#include <isc/serial.h>
#include <isc/stats.h>
#include <isc/stdio.h>
#include <isc/string.h>
#include <isc/task.h>
#include <isc/timer.h>
#include <isc/util.h>

#ifdef AES_SIT
#include <isc/aes.h>
#else
#include <isc/hmacsha.h>
#endif

#include <dns/db.h>
#include <dns/dispatch.h>
#include <dns/events.h>
#include <dns/message.h>
#include <dns/peer.h>
#include <dns/rcode.h>
#include <dns/rdata.h>
#include <dns/rdataclass.h>
#include <dns/rdatalist.h>
#include <dns/rdataset.h>
#include <dns/resolver.h>
#include <dns/stats.h>
#include <dns/tsig.h>
#include <dns/view.h>
#include <dns/zone.h>

#include <named/interfacemgr.h>
#include <named/log.h>
#include <named/notify.h>
#include <named/os.h>
#include <named/server.h>
#include <named/update.h>

#include "pfilter.h"

/***
 *** Client
 ***/

/*! \file
 * Client Routines
 *
 * Important note!
 *
 * All client state changes, other than that from idle to listening, occur
 * as a result of events.  This guarantees serialization and avoids the
 * need for locking.
 *
 * If a routine is ever created that allows someone other than the client's
 * task to change the client, then the client will have to be locked.
 */

#define NS_CLIENT_TRACE
#ifdef NS_CLIENT_TRACE
#define CTRACE(m)	ns_client_log(client, \
				      NS_LOGCATEGORY_CLIENT, \
				      NS_LOGMODULE_CLIENT, \
				      ISC_LOG_DEBUG(3), \
				      "%s", (m))
#define MTRACE(m)	isc_log_write(ns_g_lctx, \
				      NS_LOGCATEGORY_GENERAL, \
				      NS_LOGMODULE_CLIENT, \
				      ISC_LOG_DEBUG(3), \
				      "clientmgr @%p: %s", manager, (m))
#else
#define CTRACE(m)	((void)(m))
#define MTRACE(m)	((void)(m))
#endif

#define TCP_CLIENT(c)	(((c)->attributes & NS_CLIENTATTR_TCP) != 0)

#define TCP_BUFFER_SIZE			(65535 + 2)
#define SEND_BUFFER_SIZE		4096
#define RECV_BUFFER_SIZE		4096

#ifdef ISC_PLATFORM_USETHREADS
#define NMCTXS				100
/*%<
 * Number of 'mctx pools' for clients. (Should this be configurable?)
 * When enabling threads, we use a pool of memory contexts shared by
 * client objects, since concurrent access to a shared context would cause
 * heavy contentions.  The above constant is expected to be enough for
 * completely avoiding contentions among threads for an authoritative-only
 * server.
 */
#else
#define NMCTXS				0
/*%<
 * If named with built without thread, simply share manager's context.  Using
 * a separate context in this case would simply waste memory.
 */
#endif

#define SIT_SIZE 24U /* 8 + 4 + 4 + 8 */

/*% nameserver client manager structure */
struct ns_clientmgr {
	/* Unlocked. */
	unsigned int			magic;

	/* The queue object has its own locks */
	client_queue_t			inactive;     /*%< To be recycled */

	isc_mem_t *			mctx;
	isc_taskmgr_t *			taskmgr;
	isc_timermgr_t *		timermgr;

	/* Lock covers manager state. */
	isc_mutex_t			lock;
	isc_boolean_t			exiting;

	/* Lock covers the clients list */
	isc_mutex_t			listlock;
	client_list_t			clients;      /*%< All active clients */

	/* Lock covers the recursing list */
	isc_mutex_t			reclock;
	client_list_t			recursing;    /*%< Recursing clients */

#if NMCTXS > 0
	/*%< mctx pool for clients. */
	unsigned int			nextmctx;
	isc_mem_t *			mctxpool[NMCTXS];
#endif
};

#define MANAGER_MAGIC			ISC_MAGIC('N', 'S', 'C', 'm')
#define VALID_MANAGER(m)		ISC_MAGIC_VALID(m, MANAGER_MAGIC)

/*!
 * Client object states.  Ordering is significant: higher-numbered
 * states are generally "more active", meaning that the client can
 * have more dynamically allocated data, outstanding events, etc.
 * In the list below, any such properties listed for state N
 * also apply to any state > N.
 *
 * To force the client into a less active state, set client->newstate
 * to that state and call exit_check().  This will cause any
 * activities defined for higher-numbered states to be aborted.
 */

#define NS_CLIENTSTATE_FREED    0
/*%<
 * The client object no longer exists.
 */

#define NS_CLIENTSTATE_INACTIVE 1
/*%<
 * The client object exists and has a task and timer.
 * Its "query" struct and sendbuf are initialized.
 * It is on the client manager's list of inactive clients.
 * It has a message and OPT, both in the reset state.
 */

#define NS_CLIENTSTATE_READY    2
/*%<
 * The client object is either a TCP or a UDP one, and
 * it is associated with a network interface.  It is on the
 * client manager's list of active clients.
 *
 * If it is a TCP client object, it has a TCP listener socket
 * and an outstanding TCP listen request.
 *
 * If it is a UDP client object, it has a UDP listener socket
 * and an outstanding UDP receive request.
 */

#define NS_CLIENTSTATE_READING  3
/*%<
 * The client object is a TCP client object that has received
 * a connection.  It has a tcpsocket, tcpmsg, TCP quota, and an
 * outstanding TCP read request.  This state is not used for
 * UDP client objects.
 */

#define NS_CLIENTSTATE_WORKING  4
/*%<
 * The client object has received a request and is working
 * on it.  It has a view, and it may have any of a non-reset OPT,
 * recursion quota, and an outstanding write request.
 */

#define NS_CLIENTSTATE_RECURSING  5
/*%<
 * The client object is recursing.  It will be on the 'recursing'
 * list.
 */

#define NS_CLIENTSTATE_MAX      9
/*%<
 * Sentinel value used to indicate "no state".  When client->newstate
 * has this value, we are not attempting to exit the current state.
 * Must be greater than any valid state.
 */

/*
 * Enable ns_client_dropport() by default.
 */
#ifndef NS_CLIENT_DROPPORT
#define NS_CLIENT_DROPPORT 1
#endif

unsigned int ns_client_requests;

static void client_read(ns_client_t *client);
static void client_accept(ns_client_t *client);
static void client_udprecv(ns_client_t *client);
static void clientmgr_destroy(ns_clientmgr_t *manager);
static isc_boolean_t exit_check(ns_client_t *client);
static void ns_client_endrequest(ns_client_t *client);
static void client_start(isc_task_t *task, isc_event_t *event);
static void client_request(isc_task_t *task, isc_event_t *event);
static void ns_client_dumpmessage(ns_client_t *client, const char *reason);
static isc_result_t get_client(ns_clientmgr_t *manager, ns_interface_t *ifp,
			       dns_dispatch_t *disp, isc_boolean_t tcp);
static inline isc_boolean_t
allowed(isc_netaddr_t *addr, dns_name_t *signer, dns_acl_t *acl);
#ifdef ISC_PLATFORM_USESIT
static void compute_sit(ns_client_t *client, isc_uint32_t when,
			isc_uint32_t nonce, isc_buffer_t *buf);
#endif

void
ns_client_recursing(ns_client_t *client) {
	REQUIRE(NS_CLIENT_VALID(client));
	REQUIRE(client->state == NS_CLIENTSTATE_WORKING);

	LOCK(&client->manager->reclock);
	client->newstate = client->state = NS_CLIENTSTATE_RECURSING;
	ISC_LIST_APPEND(client->manager->recursing, client, rlink);
	UNLOCK(&client->manager->reclock);
}

void
ns_client_killoldestquery(ns_client_t *client) {
	ns_client_t *oldest;
	REQUIRE(NS_CLIENT_VALID(client));

	LOCK(&client->manager->reclock);
	oldest = ISC_LIST_HEAD(client->manager->recursing);
	if (oldest != NULL) {
		ISC_LIST_UNLINK(client->manager->recursing, oldest, rlink);
		UNLOCK(&client->manager->reclock);
		ns_query_cancel(oldest);
	} else
		UNLOCK(&client->manager->reclock);
}

void
ns_client_settimeout(ns_client_t *client, unsigned int seconds) {
	isc_result_t result;
	isc_interval_t interval;

	isc_interval_set(&interval, seconds, 0);
	result = isc_timer_reset(client->timer, isc_timertype_once, NULL,
				 &interval, ISC_FALSE);
	client->timerset = ISC_TRUE;
	if (result != ISC_R_SUCCESS) {
		ns_client_log(client, NS_LOGCATEGORY_CLIENT,
			      NS_LOGMODULE_CLIENT, ISC_LOG_ERROR,
			      "setting timeout: %s",
			      isc_result_totext(result));
		/* Continue anyway. */
	}
}

/*%
 * Check for a deactivation or shutdown request and take appropriate
 * action.  Returns ISC_TRUE if either is in progress; in this case
 * the caller must no longer use the client object as it may have been
 * freed.
 */
static isc_boolean_t
exit_check(ns_client_t *client) {
	isc_boolean_t destroy_manager = ISC_FALSE;
	ns_clientmgr_t *manager = NULL;

	REQUIRE(NS_CLIENT_VALID(client));
	manager = client->manager;

	if (client->state <= client->newstate)
		return (ISC_FALSE); /* Business as usual. */

	INSIST(client->newstate < NS_CLIENTSTATE_RECURSING);

	/*
	 * We need to detach from the view early when shutting down
	 * the server to break the following vicious circle:
	 *
	 *  - The resolver will not shut down until the view refcount is zero
	 *  - The view refcount does not go to zero until all clients detach
	 *  - The client does not detach from the view until references is zero
	 *  - references does not go to zero until the resolver has shut down
	 *
	 * Keep the view attached until any outstanding updates complete.
	 */
	if (client->nupdates == 0 &&
	    client->newstate == NS_CLIENTSTATE_FREED && client->view != NULL)
		dns_view_detach(&client->view);

	if (client->state == NS_CLIENTSTATE_WORKING ||
	    client->state == NS_CLIENTSTATE_RECURSING)
	{
		INSIST(client->newstate <= NS_CLIENTSTATE_READING);
		/*
		 * Let the update processing complete.
		 */
		if (client->nupdates > 0)
			return (ISC_TRUE);

		/*
		 * We are trying to abort request processing.
		 */
		if (client->nsends > 0) {
			isc_socket_t *socket;
			if (TCP_CLIENT(client))
				socket = client->tcpsocket;
			else
				socket = client->udpsocket;
			isc_socket_cancel(socket, client->task,
					  ISC_SOCKCANCEL_SEND);
		}

		if (! (client->nsends == 0 && client->nrecvs == 0 &&
		       client->references == 0))
		{
			/*
			 * Still waiting for I/O cancel completion.
			 * or lingering references.
			 */
			return (ISC_TRUE);
		}

		/*
		 * I/O cancel is complete.  Burn down all state
		 * related to the current request.  Ensure that
		 * the client is no longer on the recursing list.
		 *
		 * We need to check whether the client is still linked,
		 * because it may already have been removed from the
		 * recursing list by ns_client_killoldestquery()
		 */
		if (client->state == NS_CLIENTSTATE_RECURSING) {
			LOCK(&manager->reclock);
			if (ISC_LINK_LINKED(client, rlink))
				ISC_LIST_UNLINK(manager->recursing,
						client, rlink);
			UNLOCK(&manager->reclock);
		}
		ns_client_endrequest(client);

		client->state = NS_CLIENTSTATE_READING;
		INSIST(client->recursionquota == NULL);

		if (NS_CLIENTSTATE_READING == client->newstate) {
			client_read(client);
			client->newstate = NS_CLIENTSTATE_MAX;
			return (ISC_TRUE); /* We're done. */
		}
	}

	if (client->state == NS_CLIENTSTATE_READING) {
		/*
		 * We are trying to abort the current TCP connection,
		 * if any.
		 */
		INSIST(client->recursionquota == NULL);
		INSIST(client->newstate <= NS_CLIENTSTATE_READY);
		if (client->nreads > 0)
			dns_tcpmsg_cancelread(&client->tcpmsg);
		if (! client->nreads == 0) {
			/* Still waiting for read cancel completion. */
			return (ISC_TRUE);
		}

		if (client->tcpmsg_valid) {
			dns_tcpmsg_invalidate(&client->tcpmsg);
			client->tcpmsg_valid = ISC_FALSE;
		}
		if (client->tcpsocket != NULL) {
			CTRACE("closetcp");
			isc_socket_detach(&client->tcpsocket);
		}

		if (client->tcpquota != NULL)
			isc_quota_detach(&client->tcpquota);

		if (client->timerset) {
			(void)isc_timer_reset(client->timer,
					      isc_timertype_inactive,
					      NULL, NULL, ISC_TRUE);
			client->timerset = ISC_FALSE;
		}

		client->peeraddr_valid = ISC_FALSE;

		client->state = NS_CLIENTSTATE_READY;
		INSIST(client->recursionquota == NULL);

		/*
		 * Now the client is ready to accept a new TCP connection
		 * or UDP request, but we may have enough clients doing
		 * that already.  Check whether this client needs to remain
		 * active and force it to go inactive if not.
		 *
		 * UDP clients go inactive at this point, but TCP clients
		 * may remain active if we have fewer active TCP client
		 * objects than desired due to an earlier quota exhaustion.
		 */
		if (client->mortal && TCP_CLIENT(client) && !ns_g_clienttest) {
			LOCK(&client->interface->lock);
			if (client->interface->ntcpcurrent <
				    client->interface->ntcptarget)
				client->mortal = ISC_FALSE;
			UNLOCK(&client->interface->lock);
		}

		/*
		 * We don't need the client; send it to the inactive
		 * queue for recycling.
		 */
		if (client->mortal) {
			if (client->newstate > NS_CLIENTSTATE_INACTIVE)
				client->newstate = NS_CLIENTSTATE_INACTIVE;
		}

		if (NS_CLIENTSTATE_READY == client->newstate) {
			if (TCP_CLIENT(client)) {
				client_accept(client);
			} else
				client_udprecv(client);
			client->newstate = NS_CLIENTSTATE_MAX;
			return (ISC_TRUE);
		}
	}

	if (client->state == NS_CLIENTSTATE_READY) {
		INSIST(client->newstate <= NS_CLIENTSTATE_INACTIVE);

		/*
		 * We are trying to enter the inactive state.
		 */
		if (client->naccepts > 0)
			isc_socket_cancel(client->tcplistener, client->task,
					  ISC_SOCKCANCEL_ACCEPT);

		/* Still waiting for accept cancel completion. */
		if (! (client->naccepts == 0))
			return (ISC_TRUE);

		/* Accept cancel is complete. */
		if (client->nrecvs > 0)
			isc_socket_cancel(client->udpsocket, client->task,
					  ISC_SOCKCANCEL_RECV);

		/* Still waiting for recv cancel completion. */
		if (! (client->nrecvs == 0))
			return (ISC_TRUE);

		/* Still waiting for control event to be delivered */
		if (client->nctls > 0)
			return (ISC_TRUE);

		/* Deactivate the client. */
		if (client->interface)
			ns_interface_detach(&client->interface);

		INSIST(client->naccepts == 0);
		INSIST(client->recursionquota == NULL);
		if (client->tcplistener != NULL)
			isc_socket_detach(&client->tcplistener);

		if (client->udpsocket != NULL)
			isc_socket_detach(&client->udpsocket);

		if (client->dispatch != NULL)
			dns_dispatch_detach(&client->dispatch);

		client->attributes = 0;
		client->mortal = ISC_FALSE;

		/*
		 * Put the client on the inactive list.  If we are aiming for
		 * the "freed" state, it will be removed from the inactive
		 * list shortly, and we need to keep the manager locked until
		 * that has been done, lest the manager decide to reactivate
		 * the dying client inbetween.
		 */
		client->state = NS_CLIENTSTATE_INACTIVE;
		INSIST(client->recursionquota == NULL);

		if (client->state == client->newstate) {
			client->newstate = NS_CLIENTSTATE_MAX;
			if (!ns_g_clienttest && manager != NULL &&
			    !manager->exiting)
				ISC_QUEUE_PUSH(manager->inactive, client,
					       ilink);
			if (client->needshutdown)
				isc_task_shutdown(client->task);
			return (ISC_TRUE);
		}
	}

	if (client->state == NS_CLIENTSTATE_INACTIVE) {
		INSIST(client->newstate == NS_CLIENTSTATE_FREED);
		/*
		 * We are trying to free the client.
		 *
		 * When "shuttingdown" is true, either the task has received
		 * its shutdown event or no shutdown event has ever been
		 * set up.  Thus, we have no outstanding shutdown
		 * event at this point.
		 */
		REQUIRE(client->state == NS_CLIENTSTATE_INACTIVE);

		INSIST(client->recursionquota == NULL);
		INSIST(!ISC_QLINK_LINKED(client, ilink));

		if (manager != NULL) {
			LOCK(&manager->listlock);
			ISC_LIST_UNLINK(manager->clients, client, link);
			LOCK(&manager->lock);
			if (manager->exiting &&
			    ISC_LIST_EMPTY(manager->clients))
				destroy_manager = ISC_TRUE;
			UNLOCK(&manager->lock);
			UNLOCK(&manager->listlock);
		}

		ns_query_free(client);
		isc_mem_put(client->mctx, client->recvbuf, RECV_BUFFER_SIZE);
		isc_event_free((isc_event_t **)&client->sendevent);
		isc_event_free((isc_event_t **)&client->recvevent);
		isc_timer_detach(&client->timer);
		if (client->delaytimer != NULL)
			isc_timer_detach(&client->delaytimer);

		if (client->tcpbuf != NULL)
			isc_mem_put(client->mctx, client->tcpbuf,
				    TCP_BUFFER_SIZE);
		if (client->opt != NULL) {
			INSIST(dns_rdataset_isassociated(client->opt));
			dns_rdataset_disassociate(client->opt);
			dns_message_puttemprdataset(client->message,
						    &client->opt);
		}

		dns_message_destroy(&client->message);

		/*
		 * Detaching the task must be done after unlinking from
		 * the manager's lists because the manager accesses
		 * client->task.
		 */
		if (client->task != NULL)
			isc_task_detach(&client->task);

		CTRACE("free");
		client->magic = 0;

		/*
		 * Check that there are no other external references to
		 * the memory context.
		 */
		if (ns_g_clienttest && isc_mem_references(client->mctx) != 1) {
			isc_mem_stats(client->mctx, stderr);
			INSIST(0);
		}

		/*
		 * Destroy the fetchlock mutex that was created in
		 * ns_query_init().
		 */
		DESTROYLOCK(&client->query.fetchlock);

		isc_mem_putanddetach(&client->mctx, client, sizeof(*client));
	}

	if (destroy_manager && manager != NULL)
		clientmgr_destroy(manager);

	return (ISC_TRUE);
}

/*%
 * The client's task has received the client's control event
 * as part of the startup process.
 */
static void
client_start(isc_task_t *task, isc_event_t *event) {
	ns_client_t *client = (ns_client_t *) event->ev_arg;

	INSIST(task == client->task);

	UNUSED(task);

	INSIST(client->nctls == 1);
	client->nctls--;

	if (exit_check(client))
		return;

	if (TCP_CLIENT(client)) {
		client_accept(client);
	} else {
		client_udprecv(client);
	}
}


/*%
 * The client's task has received a shutdown event.
 */
static void
client_shutdown(isc_task_t *task, isc_event_t *event) {
	ns_client_t *client;

	REQUIRE(event != NULL);
	REQUIRE(event->ev_type == ISC_TASKEVENT_SHUTDOWN);
	client = event->ev_arg;
	REQUIRE(NS_CLIENT_VALID(client));
	REQUIRE(task == client->task);

	UNUSED(task);

	CTRACE("shutdown");

	isc_event_free(&event);

	if (client->shutdown != NULL) {
		(client->shutdown)(client->shutdown_arg, ISC_R_SHUTTINGDOWN);
		client->shutdown = NULL;
		client->shutdown_arg = NULL;
	}

	if (ISC_QLINK_LINKED(client, ilink))
		ISC_QUEUE_UNLINK(client->manager->inactive, client, ilink);

	client->newstate = NS_CLIENTSTATE_FREED;
	client->needshutdown = ISC_FALSE;
	(void)exit_check(client);
}

static void
ns_client_endrequest(ns_client_t *client) {
	INSIST(client->naccepts == 0);
	INSIST(client->nreads == 0);
	INSIST(client->nsends == 0);
	INSIST(client->nrecvs == 0);
	INSIST(client->nupdates == 0);
	INSIST(client->state == NS_CLIENTSTATE_WORKING ||
	       client->state == NS_CLIENTSTATE_RECURSING);

	CTRACE("endrequest");

	if (client->next != NULL) {
		(client->next)(client);
		client->next = NULL;
	}

	if (client->view != NULL)
		dns_view_detach(&client->view);
	if (client->opt != NULL) {
		INSIST(dns_rdataset_isassociated(client->opt));
		dns_rdataset_disassociate(client->opt);
		dns_message_puttemprdataset(client->message, &client->opt);
	}

	client->signer = NULL;
	client->udpsize = 512;
	client->extflags = 0;
	client->ednsversion = -1;
	dns_message_reset(client->message, DNS_MESSAGE_INTENTPARSE);

	if (client->recursionquota != NULL) {
		isc_quota_detach(&client->recursionquota);
		isc_stats_decrement(ns_g_server->nsstats,
				    dns_nsstatscounter_recursclients);
	}

	/*
	 * Clear all client attributes that are specific to
	 * the request; that's all except the TCP flag.
	 */
	client->attributes &= NS_CLIENTATTR_TCP;
}

void
ns_client_next(ns_client_t *client, isc_result_t result) {
	int newstate;

	REQUIRE(NS_CLIENT_VALID(client));
	REQUIRE(client->state == NS_CLIENTSTATE_WORKING ||
		client->state == NS_CLIENTSTATE_RECURSING ||
		client->state == NS_CLIENTSTATE_READING);

	CTRACE("next");

	if (result != ISC_R_SUCCESS)
		ns_client_log(client, DNS_LOGCATEGORY_SECURITY,
			      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(3),
			      "request failed: %s", isc_result_totext(result));

	/*
	 * An error processing a TCP request may have left
	 * the connection out of sync.  To be safe, we always
	 * sever the connection when result != ISC_R_SUCCESS.
	 */
	if (result == ISC_R_SUCCESS && TCP_CLIENT(client))
		newstate = NS_CLIENTSTATE_READING;
	else
		newstate = NS_CLIENTSTATE_READY;

	if (client->newstate > newstate)
		client->newstate = newstate;
	(void)exit_check(client);
}


static void
client_senddone(isc_task_t *task, isc_event_t *event) {
	ns_client_t *client;
	isc_socketevent_t *sevent = (isc_socketevent_t *) event;

	REQUIRE(sevent != NULL);
	REQUIRE(sevent->ev_type == ISC_SOCKEVENT_SENDDONE);
	client = sevent->ev_arg;
	REQUIRE(NS_CLIENT_VALID(client));
	REQUIRE(task == client->task);
	REQUIRE(sevent == client->sendevent);

	UNUSED(task);

	CTRACE("senddone");

	if (sevent->result != ISC_R_SUCCESS)
		ns_client_log(client, NS_LOGCATEGORY_CLIENT,
			      NS_LOGMODULE_CLIENT, ISC_LOG_WARNING,
			      "error sending response: %s",
			      isc_result_totext(sevent->result));

	INSIST(client->nsends > 0);
	client->nsends--;

	if (client->tcpbuf != NULL) {
		INSIST(TCP_CLIENT(client));
		isc_mem_put(client->mctx, client->tcpbuf, TCP_BUFFER_SIZE);
		client->tcpbuf = NULL;
	}

	ns_client_next(client, ISC_R_SUCCESS);
}

/*%
 * We only want to fail with ISC_R_NOSPACE when called from
 * ns_client_sendraw() and not when called from ns_client_send(),
 * tcpbuffer is NULL when called from ns_client_sendraw() and
 * length != 0.  tcpbuffer != NULL when called from ns_client_send()
 * and length == 0.
 */

static isc_result_t
client_allocsendbuf(ns_client_t *client, isc_buffer_t *buffer,
		    isc_buffer_t *tcpbuffer, isc_uint32_t length,
		    unsigned char *sendbuf, unsigned char **datap)
{
	unsigned char *data;
	isc_uint32_t bufsize;
	isc_result_t result;

	INSIST(datap != NULL);
	INSIST((tcpbuffer == NULL && length != 0) ||
	       (tcpbuffer != NULL && length == 0));

	if (TCP_CLIENT(client)) {
		INSIST(client->tcpbuf == NULL);
		if (length + 2 > TCP_BUFFER_SIZE) {
			result = ISC_R_NOSPACE;
			goto done;
		}
		client->tcpbuf = isc_mem_get(client->mctx, TCP_BUFFER_SIZE);
		if (client->tcpbuf == NULL) {
			result = ISC_R_NOMEMORY;
			goto done;
		}
		data = client->tcpbuf;
		if (tcpbuffer != NULL) {
			isc_buffer_init(tcpbuffer, data, TCP_BUFFER_SIZE);
			isc_buffer_init(buffer, data + 2, TCP_BUFFER_SIZE - 2);
		} else {
			isc_buffer_init(buffer, data, TCP_BUFFER_SIZE);
			INSIST(length <= 0xffff);
			isc_buffer_putuint16(buffer, (isc_uint16_t)length);
		}
	} else {
		data = sendbuf;
#ifdef ISC_PLATFORM_USESIT
		if ((client->attributes & NS_CLIENTATTR_HAVESIT) == 0) {
			if (client->view != NULL)
				bufsize = client->view->situdp;
			else
				bufsize = 512;
		} else
			bufsize = client->udpsize;
		if (bufsize > client->udpsize)
			bufsize = client->udpsize;
		if (bufsize > SEND_BUFFER_SIZE)
			bufsize = SEND_BUFFER_SIZE;
#else
		if (client->udpsize < SEND_BUFFER_SIZE)
			bufsize = client->udpsize;
		else
			bufsize = SEND_BUFFER_SIZE;
#endif
		if (length > bufsize) {
			result = ISC_R_NOSPACE;
			goto done;
		}
		isc_buffer_init(buffer, data, bufsize);
	}
	*datap = data;
	result = ISC_R_SUCCESS;

 done:
	return (result);
}

static isc_result_t
client_sendpkg(ns_client_t *client, isc_buffer_t *buffer) {
	struct in6_pktinfo *pktinfo;
	isc_result_t result;
	isc_region_t r;
	isc_sockaddr_t *address;
	isc_socket_t *socket;
	isc_netaddr_t netaddr;
	int match;
	unsigned int sockflags = ISC_SOCKFLAG_IMMEDIATE;
	isc_dscp_t dispdscp = -1;

	if (TCP_CLIENT(client)) {
		socket = client->tcpsocket;
		address = NULL;
	} else {
		socket = client->udpsocket;
		address = &client->peeraddr;

		isc_netaddr_fromsockaddr(&netaddr, &client->peeraddr);
		if (ns_g_server->blackholeacl != NULL &&
		    dns_acl_match(&netaddr, NULL,
				  ns_g_server->blackholeacl,
				  &ns_g_server->aclenv,
				  &match, NULL) == ISC_R_SUCCESS &&
		    match > 0)
			return (DNS_R_BLACKHOLED);
		sockflags |= ISC_SOCKFLAG_NORETRY;
	}

	if ((client->attributes & NS_CLIENTATTR_PKTINFO) != 0 &&
	    (client->attributes & NS_CLIENTATTR_MULTICAST) == 0)
		pktinfo = &client->pktinfo;
	else
		pktinfo = NULL;

	if (client->dispatch != NULL) {
		dispdscp = dns_dispatch_getdscp(client->dispatch);
		if (dispdscp != -1)
			client->dscp = dispdscp;
	}

	if (client->dscp == -1) {
		client->sendevent->attributes &= ~ISC_SOCKEVENTATTR_DSCP;
		client->sendevent->dscp = 0;
	} else {
		client->sendevent->attributes |= ISC_SOCKEVENTATTR_DSCP;
		client->sendevent->dscp = client->dscp;
	}

	isc_buffer_usedregion(buffer, &r);

	CTRACE("sendto");

	result = isc_socket_sendto2(socket, &r, client->task,
				    address, pktinfo,
				    client->sendevent, sockflags);
	if (result == ISC_R_SUCCESS || result == ISC_R_INPROGRESS) {
		client->nsends++;
		if (result == ISC_R_SUCCESS)
			client_senddone(client->task,
					(isc_event_t *)client->sendevent);
		result = ISC_R_SUCCESS;
	}
	return (result);
}

void
ns_client_sendraw(ns_client_t *client, dns_message_t *message) {
	isc_result_t result;
	unsigned char *data;
	isc_buffer_t buffer;
	isc_region_t r;
	isc_region_t *mr;
	unsigned char sendbuf[SEND_BUFFER_SIZE];

	REQUIRE(NS_CLIENT_VALID(client));

	CTRACE("sendraw");

	mr = dns_message_getrawmessage(message);
	if (mr == NULL) {
		result = ISC_R_UNEXPECTEDEND;
		goto done;
	}

	result = client_allocsendbuf(client, &buffer, NULL, mr->length,
				     sendbuf, &data);
	if (result != ISC_R_SUCCESS)
		goto done;

	/*
	 * Copy message to buffer and fixup id.
	 */
	isc_buffer_availableregion(&buffer, &r);
	result = isc_buffer_copyregion(&buffer, mr);
	if (result != ISC_R_SUCCESS)
		goto done;
	r.base[0] = (client->message->id >> 8) & 0xff;
	r.base[1] = client->message->id & 0xff;

	result = client_sendpkg(client, &buffer);
	if (result == ISC_R_SUCCESS)
		return;

 done:
	if (client->tcpbuf != NULL) {
		isc_mem_put(client->mctx, client->tcpbuf, TCP_BUFFER_SIZE);
		client->tcpbuf = NULL;
	}
	ns_client_next(client, result);
}

static void
client_send(ns_client_t *client) {
	isc_result_t result;
	unsigned char *data;
	isc_buffer_t buffer;
	isc_buffer_t tcpbuffer;
	isc_region_t r;
	dns_compress_t cctx;
	isc_boolean_t cleanup_cctx = ISC_FALSE;
	unsigned char sendbuf[SEND_BUFFER_SIZE];
	unsigned int render_opts;
	unsigned int preferred_glue;
	isc_boolean_t opt_included = ISC_FALSE;

	REQUIRE(NS_CLIENT_VALID(client));

	CTRACE("send");

	if ((client->attributes & NS_CLIENTATTR_RA) != 0)
		client->message->flags |= DNS_MESSAGEFLAG_RA;

	if ((client->attributes & NS_CLIENTATTR_WANTDNSSEC) != 0)
		render_opts = 0;
	else
		render_opts = DNS_MESSAGERENDER_OMITDNSSEC;

	preferred_glue = 0;
	if (client->view != NULL) {
		if (client->view->preferred_glue == dns_rdatatype_a)
			preferred_glue = DNS_MESSAGERENDER_PREFER_A;
		else if (client->view->preferred_glue == dns_rdatatype_aaaa)
			preferred_glue = DNS_MESSAGERENDER_PREFER_AAAA;
	}

#ifdef ALLOW_FILTER_AAAA
	/*
	 * filter-aaaa-on-v4 yes or break-dnssec option to suppress
	 * AAAA records.
	 *
	 * We already know that request came via IPv4,
	 * that we have both AAAA and A records,
	 * and that we either have no signatures that the client wants
	 * or we are supposed to break DNSSEC.
	 *
	 * Override preferred glue if necessary.
	 */
	if ((client->attributes & NS_CLIENTATTR_FILTER_AAAA) != 0) {
		render_opts |= DNS_MESSAGERENDER_FILTER_AAAA;
		if (preferred_glue == DNS_MESSAGERENDER_PREFER_AAAA)
			preferred_glue = DNS_MESSAGERENDER_PREFER_A;
	}
#endif

	/*
	 * Create an OPT for our reply.
	 */
	if ((client->attributes & NS_CLIENTATTR_WANTOPT) != 0) {
		result = ns_client_addopt(client, client->message,
					  &client->opt);
		if (result != ISC_R_SUCCESS)
			goto done;
	}

	/*
	 * XXXRTH  The following doesn't deal with TCP buffer resizing.
	 */
	result = client_allocsendbuf(client, &buffer, &tcpbuffer, 0,
				     sendbuf, &data);
	if (result != ISC_R_SUCCESS)
		goto done;

	result = dns_compress_init(&cctx, -1, client->mctx);
	if (result != ISC_R_SUCCESS)
		goto done;
	if (client->peeraddr_valid && client->view != NULL) {
		isc_netaddr_t netaddr;
		dns_name_t *name = NULL;

		isc_netaddr_fromsockaddr(&netaddr, &client->peeraddr);
		if (client->message->tsigkey != NULL)
			name = &client->message->tsigkey->name;
		if (client->view->nocasecompress == NULL ||
		    !allowed(&netaddr, name, client->view->nocasecompress))
		{
			dns_compress_setsensitive(&cctx, ISC_TRUE);
		}
	}
	cleanup_cctx = ISC_TRUE;

	result = dns_message_renderbegin(client->message, &cctx, &buffer);
	if (result != ISC_R_SUCCESS)
		goto done;

	if (client->opt != NULL) {
		result = dns_message_setopt(client->message, client->opt);
		opt_included = ISC_TRUE;
		client->opt = NULL;
		if (result != ISC_R_SUCCESS)
			goto done;
	}
	result = dns_message_rendersection(client->message,
					   DNS_SECTION_QUESTION, 0);
	if (result == ISC_R_NOSPACE) {
		client->message->flags |= DNS_MESSAGEFLAG_TC;
		goto renderend;
	}
	if (result != ISC_R_SUCCESS)
		goto done;
	/*
	 * Stop after the question if TC was set for rate limiting.
	 */
	if ((client->message->flags & DNS_MESSAGEFLAG_TC) != 0)
		goto renderend;
	result = dns_message_rendersection(client->message,
					   DNS_SECTION_ANSWER,
					   DNS_MESSAGERENDER_PARTIAL |
					   render_opts);
	if (result == ISC_R_NOSPACE) {
		client->message->flags |= DNS_MESSAGEFLAG_TC;
		goto renderend;
	}
	if (result != ISC_R_SUCCESS)
		goto done;
	result = dns_message_rendersection(client->message,
					   DNS_SECTION_AUTHORITY,
					   DNS_MESSAGERENDER_PARTIAL |
					   render_opts);
	if (result == ISC_R_NOSPACE) {
		client->message->flags |= DNS_MESSAGEFLAG_TC;
		goto renderend;
	}
	if (result != ISC_R_SUCCESS)
		goto done;
	result = dns_message_rendersection(client->message,
					   DNS_SECTION_ADDITIONAL,
					   preferred_glue | render_opts);
	if (result != ISC_R_SUCCESS && result != ISC_R_NOSPACE)
		goto done;
 renderend:
	result = dns_message_renderend(client->message);

	if (result != ISC_R_SUCCESS)
		goto done;

	if (cleanup_cctx) {
		dns_compress_invalidate(&cctx);
		cleanup_cctx = ISC_FALSE;
	}

	if (TCP_CLIENT(client)) {
		isc_buffer_usedregion(&buffer, &r);
		isc_buffer_putuint16(&tcpbuffer, (isc_uint16_t) r.length);
		isc_buffer_add(&tcpbuffer, r.length);
		result = client_sendpkg(client, &tcpbuffer);
	} else
		result = client_sendpkg(client, &buffer);

	/* update statistics (XXXJT: is it okay to access message->xxxkey?) */
	isc_stats_increment(ns_g_server->nsstats, dns_nsstatscounter_response);
	if (opt_included) {
		isc_stats_increment(ns_g_server->nsstats,
				    dns_nsstatscounter_edns0out);
	}
	if (client->message->tsigkey != NULL) {
		isc_stats_increment(ns_g_server->nsstats,
				    dns_nsstatscounter_tsigout);
	}
	if (client->message->sig0key != NULL) {
		isc_stats_increment(ns_g_server->nsstats,
				    dns_nsstatscounter_sig0out);
	}
	if ((client->message->flags & DNS_MESSAGEFLAG_TC) != 0)
		isc_stats_increment(ns_g_server->nsstats,
				    dns_nsstatscounter_truncatedresp);

	if (result == ISC_R_SUCCESS)
		return;

 done:
	if (client->tcpbuf != NULL) {
		isc_mem_put(client->mctx, client->tcpbuf, TCP_BUFFER_SIZE);
		client->tcpbuf = NULL;
	}

	if (cleanup_cctx)
		dns_compress_invalidate(&cctx);

	ns_client_next(client, result);
}

/*
 * Completes the sending of a delayed client response.
 */
static void
client_delay(isc_task_t *task, isc_event_t *event) {
	ns_client_t *client;

	REQUIRE(event != NULL);
	REQUIRE(event->ev_type == ISC_TIMEREVENT_LIFE ||
		event->ev_type == ISC_TIMEREVENT_IDLE);
	client = event->ev_arg;
	REQUIRE(NS_CLIENT_VALID(client));
	REQUIRE(task == client->task);
	REQUIRE(client->delaytimer != NULL);

	UNUSED(task);

	CTRACE("client_delay");

	isc_event_free(&event);
	isc_timer_detach(&client->delaytimer);

	client_send(client);
	ns_client_detach(&client);
}

void
ns_client_send(ns_client_t *client) {

	/*
	 * Delay the response by ns_g_delay ms.
	 */
	if (ns_g_delay != 0) {
		ns_client_t *dummy = NULL;
		isc_result_t result;
		isc_interval_t interval;

		/*
		 * Replace ourselves if we have not already been replaced.
		 */
		if (!client->mortal) {
			result = ns_client_replace(client);
			if (result != ISC_R_SUCCESS)
				goto nodelay;
		}

		ns_client_attach(client, &dummy);
		if (ns_g_delay >= 1000)
			isc_interval_set(&interval, ns_g_delay / 1000,
					 (ns_g_delay % 1000) * 1000000);
		else
			isc_interval_set(&interval, 0, ns_g_delay * 1000000);
		result = isc_timer_create(client->manager->timermgr,
					  isc_timertype_once, NULL, &interval,
					  client->task, client_delay,
					  client, &client->delaytimer);
		if (result == ISC_R_SUCCESS)
			return;

		ns_client_detach(&dummy);
	}

 nodelay:
	client_send(client);
}

#if NS_CLIENT_DROPPORT
#define DROPPORT_NO		0
#define DROPPORT_REQUEST	1
#define DROPPORT_RESPONSE	2
/*%
 * ns_client_dropport determines if certain requests / responses
 * should be dropped based on the port number.
 *
 * Returns:
 * \li	0:	Don't drop.
 * \li	1:	Drop request.
 * \li	2:	Drop (error) response.
 */
static int
ns_client_dropport(in_port_t port) {
	switch (port) {
	case 7: /* echo */
	case 13: /* daytime */
	case 19: /* chargen */
	case 37: /* time */
		return (DROPPORT_REQUEST);
	case 464: /* kpasswd */
		return (DROPPORT_RESPONSE);
	}
	return (DROPPORT_NO);
}
#endif

void
ns_client_error(ns_client_t *client, isc_result_t result) {
	dns_rcode_t rcode;
	dns_message_t *message;

	REQUIRE(NS_CLIENT_VALID(client));

	CTRACE("error");

	message = client->message;
	rcode = dns_result_torcode(result);

#if NS_CLIENT_DROPPORT
	/*
	 * Don't send FORMERR to ports on the drop port list.
	 */
	if (rcode == dns_rcode_formerr &&
	    ns_client_dropport(isc_sockaddr_getport(&client->peeraddr)) !=
	    DROPPORT_NO) {
		char buf[64];
		isc_buffer_t b;

		isc_buffer_init(&b, buf, sizeof(buf) - 1);
		if (dns_rcode_totext(rcode, &b) != ISC_R_SUCCESS)
			isc_buffer_putstr(&b, "UNKNOWN RCODE");
		ns_client_log(client, DNS_LOGCATEGORY_SECURITY,
			      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(10),
			      "dropped error (%.*s) response: suspicious port",
			      (int)isc_buffer_usedlength(&b), buf);
		ns_client_next(client, ISC_R_SUCCESS);
		return;
	}
#endif

	/*
	 * Try to rate limit error responses.
	 */
	if (client->view != NULL && client->view->rrl != NULL) {
		isc_boolean_t wouldlog;
		char log_buf[DNS_RRL_LOG_BUF_LEN];
		dns_rrl_result_t rrl_result;

		INSIST(rcode != dns_rcode_noerror &&
		       rcode != dns_rcode_nxdomain);
		wouldlog = isc_log_wouldlog(ns_g_lctx, DNS_RRL_LOG_DROP);
		rrl_result = dns_rrl(client->view, &client->peeraddr,
				     TCP_CLIENT(client),
				     dns_rdataclass_in, dns_rdatatype_none,
				     NULL, result, client->now,
				     wouldlog, log_buf, sizeof(log_buf));
		if (rrl_result != DNS_RRL_RESULT_OK) {
			/*
			 * Log dropped errors in the query category
			 * so that they are not lost in silence.
			 * Starts of rate-limited bursts are logged in
			 * NS_LOGCATEGORY_RRL.
			 */
			if (wouldlog) {
				ns_client_log(client,
					      NS_LOGCATEGORY_QUERY_EERRORS,
					      NS_LOGMODULE_CLIENT,
					      DNS_RRL_LOG_DROP,
					      "%s", log_buf);
			}
			/*
			 * Some error responses cannot be 'slipped',
			 * so don't try to slip any error responses.
			 */
			if (!client->view->rrl->log_only) {
				isc_stats_increment(ns_g_server->nsstats,
						dns_nsstatscounter_ratedropped);
				isc_stats_increment(ns_g_server->nsstats,
						dns_nsstatscounter_dropped);
				ns_client_next(client, DNS_R_DROP);
				return;
			}
		}
	}

	/*
	 * Message may be an in-progress reply that we had trouble
	 * with, in which case QR will be set.  We need to clear QR before
	 * calling dns_message_reply() to avoid triggering an assertion.
	 */
	message->flags &= ~DNS_MESSAGEFLAG_QR;
	/*
	 * AA and AD shouldn't be set.
	 */
	message->flags &= ~(DNS_MESSAGEFLAG_AA | DNS_MESSAGEFLAG_AD);
	result = dns_message_reply(message, ISC_TRUE);
	if (result != ISC_R_SUCCESS) {
		/*
		 * It could be that we've got a query with a good header,
		 * but a bad question section, so we try again with
		 * want_question_section set to ISC_FALSE.
		 */
		result = dns_message_reply(message, ISC_FALSE);
		if (result != ISC_R_SUCCESS) {
			ns_client_next(client, result);
			return;
		}
	}
	message->rcode = rcode;

	/*
	 * FORMERR loop avoidance:  If we sent a FORMERR message
	 * with the same ID to the same client less than two
	 * seconds ago, assume that we are in an infinite error
	 * packet dialog with a server for some protocol whose
	 * error responses look enough like DNS queries to
	 * elicit a FORMERR response.  Drop a packet to break
	 * the loop.
	 */
	if (rcode == dns_rcode_formerr) {
		if (isc_sockaddr_equal(&client->peeraddr,
				       &client->formerrcache.addr) &&
		    message->id == client->formerrcache.id &&
		    client->requesttime - client->formerrcache.time < 2) {
			/* Drop packet. */
			ns_client_log(client, NS_LOGCATEGORY_CLIENT,
				      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(1),
				      "possible error packet loop, "
				      "FORMERR dropped");
			ns_client_next(client, result);
			return;
		}
		client->formerrcache.addr = client->peeraddr;
		client->formerrcache.time = client->requesttime;
		client->formerrcache.id = message->id;
	}
	ns_client_send(client);
}

isc_result_t
ns_client_addopt(ns_client_t *client, dns_message_t *message,
		 dns_rdataset_t **opt)
{
	char nsid[BUFSIZ], *nsidp;
#ifdef ISC_PLATFORM_USESIT
	unsigned char sit[SIT_SIZE];
#endif
	isc_result_t result;
	dns_view_t *view;
	dns_resolver_t *resolver;
	isc_uint16_t udpsize;
	dns_ednsopt_t ednsopts[DNS_EDNSOPTIONS];
	int count = 0;
	unsigned int flags;
	unsigned char expire[4];

	REQUIRE(NS_CLIENT_VALID(client));
	REQUIRE(opt != NULL && *opt == NULL);
	REQUIRE(message != NULL);

	view = client->view;
	resolver = (view != NULL) ? view->resolver : NULL;
	if (resolver != NULL)
		udpsize = dns_resolver_getudpsize(resolver);
	else
		udpsize = ns_g_udpsize;

	flags = client->extflags & DNS_MESSAGEEXTFLAG_REPLYPRESERVE;

	/* Set EDNS options if applicable */
	if ((client->attributes & NS_CLIENTATTR_WANTNSID) != 0 &&
	    (ns_g_server->server_id != NULL ||
	     ns_g_server->server_usehostname)) {
		if (ns_g_server->server_usehostname) {
			result = ns_os_gethostname(nsid, sizeof(nsid));
			if (result != ISC_R_SUCCESS) {
				goto no_nsid;
			}
			nsidp = nsid;
		} else
			nsidp = ns_g_server->server_id;

		INSIST(count < DNS_EDNSOPTIONS);
		ednsopts[count].code = DNS_OPT_NSID;
		ednsopts[count].length = strlen(nsidp);
		ednsopts[count].value = (unsigned char *)nsidp;
		count++;
	}
 no_nsid:
#ifdef ISC_PLATFORM_USESIT
	if ((client->attributes & NS_CLIENTATTR_WANTSIT) != 0) {
		isc_buffer_t buf;
		isc_stdtime_t now;
		isc_uint32_t nonce;

		isc_buffer_init(&buf, sit, sizeof(sit));
		isc_stdtime_get(&now);
		isc_random_get(&nonce);

		compute_sit(client, now, nonce, &buf);

		INSIST(count < DNS_EDNSOPTIONS);
		ednsopts[count].code = DNS_OPT_SIT;
		ednsopts[count].length = SIT_SIZE;
		ednsopts[count].value = sit;
		count++;
	}
#endif
	if ((client->attributes & NS_CLIENTATTR_HAVEEXPIRE) != 0) {
		isc_buffer_t buf;

		INSIST(count < DNS_EDNSOPTIONS);

		isc_buffer_init(&buf, expire, sizeof(expire));
		isc_buffer_putuint32(&buf, client->expire);
		ednsopts[count].code = DNS_OPT_EXPIRE;
		ednsopts[count].length = 4;
		ednsopts[count].value = expire;
		count++;
	}

	result = dns_message_buildopt(message, opt, 0, udpsize, flags,
				      ednsopts, count);
	return (result);
}

static inline isc_boolean_t
allowed(isc_netaddr_t *addr, dns_name_t *signer, dns_acl_t *acl) {
	int match;
	isc_result_t result;

	if (acl == NULL)
		return (ISC_TRUE);
	result = dns_acl_match(addr, signer, acl, &ns_g_server->aclenv,
			       &match, NULL);
	if (result == ISC_R_SUCCESS && match > 0)
		return (ISC_TRUE);
	return (ISC_FALSE);
}

/*
 * Callback to see if a non-recursive query coming from 'srcaddr' to
 * 'destaddr', with optional key 'mykey' for class 'rdclass' would be
 * delivered to 'myview'.
 *
 * We run this unlocked as both the view list and the interface list
 * are updated when the appropriate task has exclusivity.
 */
isc_boolean_t
ns_client_isself(dns_view_t *myview, dns_tsigkey_t *mykey,
		 isc_sockaddr_t *srcaddr, isc_sockaddr_t *dstaddr,
		 dns_rdataclass_t rdclass, void *arg)
{
	dns_view_t *view;
	dns_tsigkey_t *key = NULL;
	dns_name_t *tsig = NULL;
	isc_netaddr_t netsrc;
	isc_netaddr_t netdst;

	UNUSED(arg);

	/*
	 * ns_g_server->interfacemgr is task exclusive locked.
	 */
	if (ns_g_server->interfacemgr == NULL)
		return (ISC_TRUE);

	if (!ns_interfacemgr_listeningon(ns_g_server->interfacemgr, dstaddr))
		return (ISC_FALSE);

	isc_netaddr_fromsockaddr(&netsrc, srcaddr);
	isc_netaddr_fromsockaddr(&netdst, dstaddr);

	for (view = ISC_LIST_HEAD(ns_g_server->viewlist);
	     view != NULL;
	     view = ISC_LIST_NEXT(view, link)) {

		if (view->matchrecursiveonly)
			continue;

		if (rdclass != view->rdclass)
			continue;

		if (mykey != NULL) {
			isc_boolean_t match;
			isc_result_t result;

			result = dns_view_gettsig(view, &mykey->name, &key);
			if (result != ISC_R_SUCCESS)
				continue;
			match = dst_key_compare(mykey->key, key->key);
			dns_tsigkey_detach(&key);
			if (!match)
				continue;
			tsig = dns_tsigkey_identity(mykey);
		}

		if (allowed(&netsrc, tsig, view->matchclients) &&
		    allowed(&netdst, tsig, view->matchdestinations))
			break;
	}
	return (ISC_TF(view == myview));
}

#ifdef ISC_PLATFORM_USESIT
static void
compute_sit(ns_client_t *client, isc_uint32_t when, isc_uint32_t nonce,
	    isc_buffer_t *buf)
{
#ifdef AES_SIT
	unsigned char digest[ISC_AES_BLOCK_LENGTH];
	unsigned char input[4 + 4 + 16];
	isc_netaddr_t netaddr;
	unsigned char *cp;
	unsigned int i;

	memset(input, 0, sizeof(input));
	cp = isc_buffer_used(buf);
	isc_buffer_putmem(buf, client->cookie, 8);
	isc_buffer_putuint32(buf, nonce);
	isc_buffer_putuint32(buf, when);
	memmove(input, cp, 16);
	isc_aes128_crypt(ns_g_server->secret, input, digest);
	for (i = 0; i < 8; i++)
		input[i] = digest[i] ^ digest[i + 8];
	isc_netaddr_fromsockaddr(&netaddr, &client->peeraddr);
	switch (netaddr.family) {
	case AF_INET:
		memmove(input + 8, (unsigned char *)&netaddr.type.in, 4);
		memset(input + 12, 0, 4);
		isc_aes128_crypt(ns_g_server->secret, input, digest);
		break;
	case AF_INET6:
		memmove(input + 8, (unsigned char *)&netaddr.type.in6, 16);
		isc_aes128_crypt(ns_g_server->secret, input, digest);
		for (i = 0; i < 8; i++)
			input[i + 8] = digest[i] ^ digest[i + 8];
		isc_aes128_crypt(ns_g_server->secret, input + 8, digest);
		break;
	}
	for (i = 0; i < 8; i++)
		digest[i] ^= digest[i + 8];
	isc_buffer_putmem(buf, digest, 8);
#endif
#ifdef HMAC_SHA1_SIT
	unsigned char digest[ISC_SHA1_DIGESTLENGTH];
	isc_netaddr_t netaddr;
	unsigned char *cp;
	isc_hmacsha1_t hmacsha1;

	cp = isc_buffer_used(buf);
	isc_buffer_putmem(buf, client->cookie, 8);
	isc_buffer_putuint32(buf, nonce);
	isc_buffer_putuint32(buf, when);

	isc_hmacsha1_init(&hmacsha1,
			  ns_g_server->secret,
			  ISC_SHA1_DIGESTLENGTH);
	isc_hmacsha1_update(&hmacsha1, cp, 16);
	isc_netaddr_fromsockaddr(&netaddr, &client->peeraddr);
	switch (netaddr.family) {
	case AF_INET:
		isc_hmacsha1_update(&hmacsha1,
				    (unsigned char *)&netaddr.type.in, 4);
		break;
	case AF_INET6:
		isc_hmacsha1_update(&hmacsha1,
				    (unsigned char *)&netaddr.type.in6, 16);
		break;
	}
	isc_hmacsha1_update(&hmacsha1, client->cookie, sizeof(client->cookie));
	isc_hmacsha1_sign(&hmacsha1, digest, sizeof(digest));
	isc_buffer_putmem(buf, digest, 8);
	isc_hmacsha1_invalidate(&hmacsha1);
#endif
#ifdef HMAC_SHA256_SIT
	unsigned char digest[ISC_SHA256_DIGESTLENGTH];
	isc_netaddr_t netaddr;
	unsigned char *cp;
	isc_hmacsha256_t hmacsha256;

	cp = isc_buffer_used(buf);
	isc_buffer_putmem(buf, client->cookie, 8);
	isc_buffer_putuint32(buf, nonce);
	isc_buffer_putuint32(buf, when);

	isc_hmacsha256_init(&hmacsha256,
			    ns_g_server->secret,
			    ISC_SHA256_DIGESTLENGTH);
	isc_hmacsha256_update(&hmacsha256, cp, 16);
	isc_netaddr_fromsockaddr(&netaddr, &client->peeraddr);
	switch (netaddr.family) {
	case AF_INET:
		isc_hmacsha256_update(&hmacsha256,
				      (unsigned char *)&netaddr.type.in, 4);
		break;
	case AF_INET6:
		isc_hmacsha256_update(&hmacsha256,
				      (unsigned char *)&netaddr.type.in6, 16);
		break;
	}
	isc_hmacsha256_update(&hmacsha256, client->cookie,
			      sizeof(client->cookie));
	isc_hmacsha256_sign(&hmacsha256, digest, sizeof(digest));
	isc_buffer_putmem(buf, digest, 8);
	isc_hmacsha256_invalidate(&hmacsha256);
#endif
}

static void
process_sit(ns_client_t *client, isc_buffer_t *buf, size_t optlen) {
	unsigned char dbuf[SIT_SIZE];
	unsigned char *old;
	isc_stdtime_t now;
	isc_uint32_t when;
	isc_uint32_t nonce;
	isc_buffer_t db;

	client->attributes |= NS_CLIENTATTR_WANTSIT;

	isc_stats_increment(ns_g_server->nsstats,
			    dns_nsstatscounter_sitopt);

	if (optlen != SIT_SIZE) {
		/*
		 * Not our token.
		 */
		if (optlen >= 8U)
			memmove(client->cookie, isc_buffer_current(buf), 8);
		else
			memset(client->cookie, 0, 8);
		isc_buffer_forward(buf, (unsigned int)optlen);

		if (optlen == 8U)
			isc_stats_increment(ns_g_server->nsstats,
					    dns_nsstatscounter_sitnew);
		else
			isc_stats_increment(ns_g_server->nsstats,
					    dns_nsstatscounter_sitbadsize);
		return;
	}

	/*
	 * Process all of the incoming buffer.
	 */
	old = isc_buffer_current(buf);
	memmove(client->cookie, old, 8);
	isc_buffer_forward(buf, 8);
	nonce = isc_buffer_getuint32(buf);
	when = isc_buffer_getuint32(buf);
	isc_buffer_forward(buf, 8);

	/*
	 * Allow for a 5 minute clock skew between servers sharing a secret.
	 * Only accept SIT if we have talked to the client in the last hour.
	 */
	isc_stdtime_get(&now);
	if (isc_serial_gt(when, (now + 300)) ||		/* In the future. */
	    isc_serial_lt(when, (now - 3600))) {	/* In the past. */
		isc_stats_increment(ns_g_server->nsstats,
				    dns_nsstatscounter_sitbadtime);
		return;
	}

	isc_buffer_init(&db, dbuf, sizeof(dbuf));
	compute_sit(client, when, nonce, &db);

	if (memcmp(old, dbuf, SIT_SIZE) != 0) {
		isc_stats_increment(ns_g_server->nsstats,
				    dns_nsstatscounter_sitnomatch);
		return;
	}
	isc_stats_increment(ns_g_server->nsstats,
			    dns_nsstatscounter_sitmatch);

	client->attributes |= NS_CLIENTATTR_HAVESIT;
}
#endif

static isc_result_t
process_opt(ns_client_t *client, dns_rdataset_t *opt) {
	dns_rdata_t rdata;
	isc_buffer_t optbuf;
	isc_result_t result;
	isc_uint16_t optcode;
	isc_uint16_t optlen;

	/*
	 * Set the client's UDP buffer size.
	 */
	client->udpsize = opt->rdclass;

	/*
	 * If the requested UDP buffer size is less than 512,
	 * ignore it and use 512.
	 */
	if (client->udpsize < 512)
		client->udpsize = 512;

	/*
	 * Get the flags out of the OPT record.
	 */
	client->extflags = (isc_uint16_t)(opt->ttl & 0xFFFF);

	/*
	 * Do we understand this version of EDNS?
	 *
	 * XXXRTH need library support for this!
	 */
	client->ednsversion = (opt->ttl & 0x00FF0000) >> 16;
	if (client->ednsversion > 0) {
		isc_stats_increment(ns_g_server->nsstats,
				    dns_nsstatscounter_badednsver);
		result = ns_client_addopt(client, client->message,
					  &client->opt);
		if (result == ISC_R_SUCCESS)
			result = DNS_R_BADVERS;
		ns_client_error(client, result);
		goto cleanup;
	}

	/* Check for NSID request */
	result = dns_rdataset_first(opt);
	if (result == ISC_R_SUCCESS) {
		dns_rdata_init(&rdata);
		dns_rdataset_current(opt, &rdata);
		isc_buffer_init(&optbuf, rdata.data, rdata.length);
		isc_buffer_add(&optbuf, rdata.length);
		while (isc_buffer_remaininglength(&optbuf) >= 4) {
			optcode = isc_buffer_getuint16(&optbuf);
			optlen = isc_buffer_getuint16(&optbuf);
			switch (optcode) {
			case DNS_OPT_NSID:
				isc_stats_increment(ns_g_server->nsstats,
						    dns_nsstatscounter_nsidopt);
				client->attributes |= NS_CLIENTATTR_WANTNSID;
				isc_buffer_forward(&optbuf, optlen);
				break;
#ifdef ISC_PLATFORM_USESIT
			case DNS_OPT_SIT:
				process_sit(client, &optbuf, optlen);
				break;
#endif
			case DNS_OPT_EXPIRE:
				isc_stats_increment(ns_g_server->nsstats,
						  dns_nsstatscounter_expireopt);
				client->attributes |= NS_CLIENTATTR_WANTEXPIRE;
				isc_buffer_forward(&optbuf, optlen);
				break;
			default:
				isc_stats_increment(ns_g_server->nsstats,
						  dns_nsstatscounter_otheropt);
				isc_buffer_forward(&optbuf, optlen);
				break;
			}
		}
	}

	isc_stats_increment(ns_g_server->nsstats, dns_nsstatscounter_edns0in);
	client->attributes |= NS_CLIENTATTR_WANTOPT;

 cleanup:
	return (result);
}

/*
 * Handle an incoming request event from the socket (UDP case)
 * or tcpmsg (TCP case).
 */
static void
client_request(isc_task_t *task, isc_event_t *event) {
	ns_client_t *client;
	isc_socketevent_t *sevent;
	isc_result_t result;
	isc_result_t sigresult = ISC_R_SUCCESS;
	isc_buffer_t *buffer;
	isc_buffer_t tbuffer;
	dns_view_t *view;
	dns_rdataset_t *opt;
	dns_name_t *signame;
	isc_boolean_t ra;	/* Recursion available. */
	isc_netaddr_t netaddr;
	int match;
	dns_messageid_t id;
	unsigned int flags;
	isc_boolean_t notimp;

	REQUIRE(event != NULL);
	client = event->ev_arg;
	REQUIRE(NS_CLIENT_VALID(client));
	REQUIRE(task == client->task);

	INSIST(client->recursionquota == NULL);

	INSIST(client->state == (TCP_CLIENT(client) ?
				       NS_CLIENTSTATE_READING :
				       NS_CLIENTSTATE_READY));

	ns_client_requests++;

	if (event->ev_type == ISC_SOCKEVENT_RECVDONE) {
		INSIST(!TCP_CLIENT(client));
		sevent = (isc_socketevent_t *)event;
		REQUIRE(sevent == client->recvevent);
		isc_buffer_init(&tbuffer, sevent->region.base, sevent->n);
		isc_buffer_add(&tbuffer, sevent->n);
		buffer = &tbuffer;
		result = sevent->result;
		if (result == ISC_R_SUCCESS) {
			client->peeraddr = sevent->address;
			client->peeraddr_valid = ISC_TRUE;
		}
		if ((sevent->attributes & ISC_SOCKEVENTATTR_DSCP) != 0) {
			ns_client_log(client, NS_LOGCATEGORY_CLIENT,
			      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(90),
			      "received DSCP %d", sevent->dscp);
			if (client->dscp == -1)
				client->dscp = sevent->dscp;
		}
		if ((sevent->attributes & ISC_SOCKEVENTATTR_PKTINFO) != 0) {
			client->attributes |= NS_CLIENTATTR_PKTINFO;
			client->pktinfo = sevent->pktinfo;
		}
		if ((sevent->attributes & ISC_SOCKEVENTATTR_MULTICAST) != 0)
			client->attributes |= NS_CLIENTATTR_MULTICAST;
		client->nrecvs--;
	} else {
		INSIST(TCP_CLIENT(client));
		REQUIRE(event->ev_type == DNS_EVENT_TCPMSG);
		REQUIRE(event->ev_sender == &client->tcpmsg);
		buffer = &client->tcpmsg.buffer;
		result = client->tcpmsg.result;
		INSIST(client->nreads == 1);
		/*
		 * client->peeraddr was set when the connection was accepted.
		 */
		client->nreads--;
	}

	if (exit_check(client))
		goto cleanup;
	client->state = client->newstate = NS_CLIENTSTATE_WORKING;

	isc_task_getcurrenttime(task, &client->requesttime);
	client->now = client->requesttime;

	if (result != ISC_R_SUCCESS) {
		if (TCP_CLIENT(client)) {
			ns_client_next(client, result);
		} else {
			if  (result != ISC_R_CANCELED)
				isc_log_write(ns_g_lctx, NS_LOGCATEGORY_CLIENT,
					      NS_LOGMODULE_CLIENT,
					      ISC_LOG_ERROR,
					      "UDP client handler shutting "
					      "down due to fatal receive "
					      "error: %s",
					      isc_result_totext(result));
			isc_task_shutdown(client->task);
		}
		goto cleanup;
	}

	isc_netaddr_fromsockaddr(&netaddr, &client->peeraddr);

#if NS_CLIENT_DROPPORT
	if (ns_client_dropport(isc_sockaddr_getport(&client->peeraddr)) ==
	    DROPPORT_REQUEST) {
		ns_client_log(client, DNS_LOGCATEGORY_SECURITY,
			      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(10),
			      "dropped request: suspicious port");
		ns_client_next(client, ISC_R_SUCCESS);
		goto cleanup;
	}
#endif

	ns_client_log(client, NS_LOGCATEGORY_CLIENT,
		      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(3),
		      "%s request",
		      TCP_CLIENT(client) ? "TCP" : "UDP");

	/*
	 * Check the blackhole ACL for UDP only, since TCP is done in
	 * client_newconn.
	 */
	if (!TCP_CLIENT(client)) {

		if (ns_g_server->blackholeacl != NULL &&
		    dns_acl_match(&netaddr, NULL, ns_g_server->blackholeacl,
				  &ns_g_server->aclenv,
				  &match, NULL) == ISC_R_SUCCESS &&
		    match > 0)
		{
			ns_client_log(client, DNS_LOGCATEGORY_SECURITY,
				      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(10),
				      "blackholed UDP datagram");
			ns_client_next(client, ISC_R_SUCCESS);
			goto cleanup;
		}
	}

	/*
	 * Silently drop multicast requests for the present.
	 * XXXMPA revisit this as mDNS spec was published.
	 */
	if ((client->attributes & NS_CLIENTATTR_MULTICAST) != 0) {
		ns_client_log(client, NS_LOGCATEGORY_CLIENT,
			      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(2),
			      "dropping multicast request");
		ns_client_next(client, DNS_R_REFUSED);
		goto cleanup;
	}

	result = dns_message_peekheader(buffer, &id, &flags);
	if (result != ISC_R_SUCCESS) {
		/*
		 * There isn't enough header to determine whether
		 * this was a request or a response.  Drop it.
		 */
		ns_client_next(client, result);
		goto cleanup;
	}

	/*
	 * The client object handles requests, not responses.
	 * If this is a UDP response, forward it to the dispatcher.
	 * If it's a TCP response, discard it here.
	 */
	if ((flags & DNS_MESSAGEFLAG_QR) != 0) {
		if (TCP_CLIENT(client)) {
			CTRACE("unexpected response");
			ns_client_next(client, DNS_R_FORMERR);
			goto cleanup;
		} else {
			dns_dispatch_importrecv(client->dispatch, event);
			ns_client_next(client, ISC_R_SUCCESS);
			goto cleanup;
		}
	}

	/*
	 * Update some statistics counters.  Don't count responses.
	 */
	if (isc_sockaddr_pf(&client->peeraddr) == PF_INET) {
		isc_stats_increment(ns_g_server->nsstats,
				    dns_nsstatscounter_requestv4);
	} else {
		isc_stats_increment(ns_g_server->nsstats,
				    dns_nsstatscounter_requestv6);
	}
	if (TCP_CLIENT(client))
		isc_stats_increment(ns_g_server->nsstats,
				    dns_nsstatscounter_requesttcp);

	/*
	 * It's a request.  Parse it.
	 */
	result = dns_message_parse(client->message, buffer, 0);
	if (result != ISC_R_SUCCESS) {
		/*
		 * Parsing the request failed.  Send a response
		 * (typically FORMERR or SERVFAIL).
		 */
		ns_client_error(client, result);
		goto cleanup;
	}

	dns_opcodestats_increment(ns_g_server->opcodestats,
				  client->message->opcode);
	switch (client->message->opcode) {
	case dns_opcode_query:
	case dns_opcode_update:
	case dns_opcode_notify:
		notimp = ISC_FALSE;
		break;
	case dns_opcode_iquery:
	default:
		notimp = ISC_TRUE;
		break;
	}

	client->message->rcode = dns_rcode_noerror;

	/* RFC1123 section 6.1.3.2 */
	if ((client->attributes & NS_CLIENTATTR_MULTICAST) != 0)
		client->message->flags &= ~DNS_MESSAGEFLAG_RD;

	/*
	 * Deal with EDNS.
	 */
	if (ns_g_noedns)
		opt = NULL;
	else
		opt = dns_message_getopt(client->message);
	if (opt != NULL) {
		/*
		 * Are we dropping all EDNS queries?
		 */
		if (ns_g_dropedns) {
			ns_client_next(client, ISC_R_SUCCESS);
			goto cleanup;
		}
		result = process_opt(client, opt);
		if (result != ISC_R_SUCCESS)
			goto cleanup;
	}

	if (client->message->rdclass == 0) {
		ns_client_log(client, NS_LOGCATEGORY_CLIENT,
			      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(1),
			      "message class could not be determined");
		ns_client_dumpmessage(client,
				      "message class could not be determined");
		ns_client_error(client, notimp ? DNS_R_NOTIMP : DNS_R_FORMERR);
		goto cleanup;
	}

	/*
	 * Determine the destination address.  If the receiving interface is
	 * bound to a specific address, we simply use it regardless of the
	 * address family.  All IPv4 queries should fall into this case.
	 * Otherwise, if this is a TCP query, get the address from the
	 * receiving socket (this needs a system call and can be heavy).
	 * For IPv6 UDP queries, we get this from the pktinfo structure (if
	 * supported).
	 * If all the attempts fail (this can happen due to memory shortage,
	 * etc), we regard this as an error for safety.
	 */
	if ((client->interface->flags & NS_INTERFACEFLAG_ANYADDR) == 0)
		isc_netaddr_fromsockaddr(&client->destaddr,
					 &client->interface->addr);
	else {
		isc_sockaddr_t sockaddr;
		result = ISC_R_FAILURE;

		if (TCP_CLIENT(client))
			result = isc_socket_getsockname(client->tcpsocket,
							&sockaddr);
		if (result == ISC_R_SUCCESS)
			isc_netaddr_fromsockaddr(&client->destaddr, &sockaddr);
		if (result != ISC_R_SUCCESS &&
		    client->interface->addr.type.sa.sa_family == AF_INET6 &&
		    (client->attributes & NS_CLIENTATTR_PKTINFO) != 0) {
			/*
			 * XXXJT technically, we should convert the receiving
			 * interface ID to a proper scope zone ID.  However,
			 * due to the fact there is no standard API for this,
			 * we only handle link-local addresses and use the
			 * interface index as link ID.  Despite the assumption,
			 * it should cover most typical cases.
			 */
			isc_netaddr_fromin6(&client->destaddr,
					    &client->pktinfo.ipi6_addr);
			if (IN6_IS_ADDR_LINKLOCAL(&client->pktinfo.ipi6_addr))
				isc_netaddr_setzone(&client->destaddr,
						client->pktinfo.ipi6_ifindex);
			result = ISC_R_SUCCESS;
		}
		if (result != ISC_R_SUCCESS) {
			UNEXPECTED_ERROR(__FILE__, __LINE__,
					 "failed to get request's "
					 "destination: %s",
					 isc_result_totext(result));
			ns_client_next(client, ISC_R_SUCCESS);
			goto cleanup;
		}
	}

	/*
	 * Find a view that matches the client's source address.
	 */
	for (view = ISC_LIST_HEAD(ns_g_server->viewlist);
	     view != NULL;
	     view = ISC_LIST_NEXT(view, link)) {
		if (client->message->rdclass == view->rdclass ||
		    client->message->rdclass == dns_rdataclass_any)
		{
			dns_name_t *tsig = NULL;

			sigresult = dns_message_rechecksig(client->message,
							   view);
			if (sigresult == ISC_R_SUCCESS)
				tsig = dns_tsigkey_identity(client->message->tsigkey);

			if (allowed(&netaddr, tsig, view->matchclients) &&
			    allowed(&client->destaddr, tsig,
				    view->matchdestinations) &&
			    !((client->message->flags & DNS_MESSAGEFLAG_RD)
			      == 0 && view->matchrecursiveonly))
			{
				dns_view_attach(view, &client->view);
				break;
			}
		}
	}

	if (view == NULL) {
		char classname[DNS_RDATACLASS_FORMATSIZE];

		/*
		 * Do a dummy TSIG verification attempt so that the
		 * response will have a TSIG if the query did, as
		 * required by RFC2845.
		 */
		isc_buffer_t b;
		isc_region_t *r;

		dns_message_resetsig(client->message);

		r = dns_message_getrawmessage(client->message);
		isc_buffer_init(&b, r->base, r->length);
		isc_buffer_add(&b, r->length);
		(void)dns_tsig_verify(&b, client->message, NULL, NULL);

		dns_rdataclass_format(client->message->rdclass, classname,
				      sizeof(classname));
		ns_client_log(client, NS_LOGCATEGORY_CLIENT,
			      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(1),
			      "no matching view in class '%s'", classname);
		ns_client_dumpmessage(client, "no matching view in class");
		ns_client_error(client, notimp ? DNS_R_NOTIMP : DNS_R_REFUSED);
		goto cleanup;
	}

	ns_client_log(client, NS_LOGCATEGORY_CLIENT,
		      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(5),
		      "using view '%s'", view->name);

	/*
	 * Check for a signature.  We log bad signatures regardless of
	 * whether they ultimately cause the request to be rejected or
	 * not.  We do not log the lack of a signature unless we are
	 * debugging.
	 */
	client->signer = NULL;
	dns_name_init(&client->signername, NULL);
	result = dns_message_signer(client->message, &client->signername);
	if (result != ISC_R_NOTFOUND) {
		signame = NULL;
		if (dns_message_gettsig(client->message, &signame) != NULL) {
			isc_stats_increment(ns_g_server->nsstats,
					    dns_nsstatscounter_tsigin);
		} else {
			isc_stats_increment(ns_g_server->nsstats,
					    dns_nsstatscounter_sig0in);
		}

	}
	if (result == ISC_R_SUCCESS) {
		char namebuf[DNS_NAME_FORMATSIZE];
		dns_name_format(&client->signername, namebuf, sizeof(namebuf));
		ns_client_log(client, DNS_LOGCATEGORY_SECURITY,
			      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(3),
			      "request has valid signature: %s", namebuf);
		client->signer = &client->signername;
	} else if (result == ISC_R_NOTFOUND) {
		ns_client_log(client, DNS_LOGCATEGORY_SECURITY,
			      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(3),
			      "request is not signed");
	} else if (result == DNS_R_NOIDENTITY) {
		ns_client_log(client, DNS_LOGCATEGORY_SECURITY,
			      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(3),
			      "request is signed by a nonauthoritative key");
	} else {
		char tsigrcode[64];
		isc_buffer_t b;
		dns_rcode_t status;
		isc_result_t tresult;

		/* There is a signature, but it is bad. */
		isc_stats_increment(ns_g_server->nsstats,
				    dns_nsstatscounter_invalidsig);
		signame = NULL;
		if (dns_message_gettsig(client->message, &signame) != NULL) {
			char namebuf[DNS_NAME_FORMATSIZE];
			char cnamebuf[DNS_NAME_FORMATSIZE];
			dns_name_format(signame, namebuf, sizeof(namebuf));
			status = client->message->tsigstatus;
			isc_buffer_init(&b, tsigrcode, sizeof(tsigrcode) - 1);
			tresult = dns_tsigrcode_totext(status, &b);
			INSIST(tresult == ISC_R_SUCCESS);
			tsigrcode[isc_buffer_usedlength(&b)] = '\0';
			if (client->message->tsigkey->generated) {
				dns_name_format(client->message->tsigkey->creator,
						cnamebuf, sizeof(cnamebuf));
				ns_client_log(client, DNS_LOGCATEGORY_SECURITY,
					      NS_LOGMODULE_CLIENT,
					      ISC_LOG_ERROR,
					      "request has invalid signature: "
					      "TSIG %s (%s): %s (%s)", namebuf,
					      cnamebuf,
					      isc_result_totext(result),
					      tsigrcode);
			} else {
				ns_client_log(client, DNS_LOGCATEGORY_SECURITY,
					      NS_LOGMODULE_CLIENT,
					      ISC_LOG_ERROR,
					      "request has invalid signature: "
					      "TSIG %s: %s (%s)", namebuf,
					      isc_result_totext(result),
					      tsigrcode);
			}
		} else {
			status = client->message->sig0status;
			isc_buffer_init(&b, tsigrcode, sizeof(tsigrcode) - 1);
			tresult = dns_tsigrcode_totext(status, &b);
			INSIST(tresult == ISC_R_SUCCESS);
			tsigrcode[isc_buffer_usedlength(&b)] = '\0';
			ns_client_log(client, DNS_LOGCATEGORY_SECURITY,
				      NS_LOGMODULE_CLIENT, ISC_LOG_ERROR,
				      "request has invalid signature: %s (%s)",
				      isc_result_totext(result), tsigrcode);
		}
		/*
		 * Accept update messages signed by unknown keys so that
		 * update forwarding works transparently through slaves
		 * that don't have all the same keys as the master.
		 */
		if (!(client->message->tsigstatus == dns_tsigerror_badkey &&
		      client->message->opcode == dns_opcode_update)) {
			ns_client_error(client, sigresult);
			goto cleanup;
		}
	}

	/*
	 * Decide whether recursive service is available to this client.
	 * We do this here rather than in the query code so that we can
	 * set the RA bit correctly on all kinds of responses, not just
	 * responses to ordinary queries.  Note if you can't query the
	 * cache there is no point in setting RA.
	 */
	ra = ISC_FALSE;
	if (client->view->resolver != NULL &&
	    client->view->recursion == ISC_TRUE &&
	    ns_client_checkaclsilent(client, NULL,
				     client->view->recursionacl,
				     ISC_TRUE) == ISC_R_SUCCESS &&
	    ns_client_checkaclsilent(client, NULL,
				     client->view->cacheacl,
				     ISC_TRUE) == ISC_R_SUCCESS &&
	    ns_client_checkaclsilent(client, &client->destaddr,
				     client->view->recursiononacl,
				     ISC_TRUE) == ISC_R_SUCCESS &&
	    ns_client_checkaclsilent(client, &client->destaddr,
				     client->view->cacheonacl,
				     ISC_TRUE) == ISC_R_SUCCESS)
		ra = ISC_TRUE;

	if (ra == ISC_TRUE)
		client->attributes |= NS_CLIENTATTR_RA;

	ns_client_log(client, DNS_LOGCATEGORY_SECURITY, NS_LOGMODULE_CLIENT,
		      ISC_LOG_DEBUG(3), ra ? "recursion available" :
					     "recursion not available");

	/*
	 * Adjust maximum UDP response size for this client.
	 */
	if (client->udpsize > 512) {
		dns_peer_t *peer = NULL;
		isc_uint16_t udpsize = view->maxudp;
		(void) dns_peerlist_peerbyaddr(view->peers, &netaddr, &peer);
		if (peer != NULL)
			dns_peer_getmaxudp(peer, &udpsize);
		if (client->udpsize > udpsize)
			client->udpsize = udpsize;
	}

	/*
	 * Dispatch the request.
	 */
	switch (client->message->opcode) {
	case dns_opcode_query:
		CTRACE("query");
		ns_query_start(client);
		break;
	case dns_opcode_update:
		CTRACE("update");
		ns_client_settimeout(client, 60);
		ns_update_start(client, sigresult);
		break;
	case dns_opcode_notify:
		CTRACE("notify");
		ns_client_settimeout(client, 60);
		ns_notify_start(client);
		break;
	case dns_opcode_iquery:
		CTRACE("iquery");
		ns_client_error(client, DNS_R_NOTIMP);
		break;
	default:
		CTRACE("unknown opcode");
		ns_client_error(client, DNS_R_NOTIMP);
	}

 cleanup:
	return;
}

static void
client_timeout(isc_task_t *task, isc_event_t *event) {
	ns_client_t *client;

	REQUIRE(event != NULL);
	REQUIRE(event->ev_type == ISC_TIMEREVENT_LIFE ||
		event->ev_type == ISC_TIMEREVENT_IDLE);
	client = event->ev_arg;
	REQUIRE(NS_CLIENT_VALID(client));
	REQUIRE(task == client->task);
	REQUIRE(client->timer != NULL);

	UNUSED(task);

	CTRACE("timeout");

	isc_event_free(&event);

	if (client->shutdown != NULL) {
		(client->shutdown)(client->shutdown_arg, ISC_R_TIMEDOUT);
		client->shutdown = NULL;
		client->shutdown_arg = NULL;
	}

	if (client->newstate > NS_CLIENTSTATE_READY)
		client->newstate = NS_CLIENTSTATE_READY;
	(void)exit_check(client);
}

static isc_result_t
get_clientmctx(ns_clientmgr_t *manager, isc_mem_t **mctxp) {
	isc_mem_t *clientmctx;
	isc_result_t result;
#if NMCTXS > 0
	unsigned int nextmctx;
#endif

	MTRACE("clientmctx");

	/*
	 * Caller must be holding the manager lock.
	 */
	if (ns_g_clienttest) {
		result = isc_mem_create(0, 0, mctxp);
		if (result == ISC_R_SUCCESS)
			isc_mem_setname(*mctxp, "client", NULL);
		return (result);
	}
#if NMCTXS > 0
	nextmctx = manager->nextmctx++;
	if (manager->nextmctx == NMCTXS)
		manager->nextmctx = 0;

	INSIST(nextmctx < NMCTXS);

	clientmctx = manager->mctxpool[nextmctx];
	if (clientmctx == NULL) {
		result = isc_mem_create(0, 0, &clientmctx);
		if (result != ISC_R_SUCCESS)
			return (result);
		isc_mem_setname(clientmctx, "client", NULL);

		manager->mctxpool[nextmctx] = clientmctx;
	}
#else
	clientmctx = manager->mctx;
#endif

	isc_mem_attach(clientmctx, mctxp);

	return (ISC_R_SUCCESS);
}

static isc_result_t
client_create(ns_clientmgr_t *manager, ns_client_t **clientp) {
	ns_client_t *client;
	isc_result_t result;
	isc_mem_t *mctx = NULL;

	/*
	 * Caller must be holding the manager lock.
	 *
	 * Note: creating a client does not add the client to the
	 * manager's client list or set the client's manager pointer.
	 * The caller is responsible for that.
	 */

	REQUIRE(clientp != NULL && *clientp == NULL);

	result = get_clientmctx(manager, &mctx);
	if (result != ISC_R_SUCCESS)
		return (result);

	client = isc_mem_get(mctx, sizeof(*client));
	if (client == NULL) {
		isc_mem_detach(&mctx);
		return (ISC_R_NOMEMORY);
	}
	client->mctx = mctx;

	client->task = NULL;
	result = isc_task_create(manager->taskmgr, 0, &client->task);
	if (result != ISC_R_SUCCESS)
		goto cleanup_client;
	isc_task_setname(client->task, "client", client);

	client->timer = NULL;
	result = isc_timer_create(manager->timermgr, isc_timertype_inactive,
				  NULL, NULL, client->task, client_timeout,
				  client, &client->timer);
	if (result != ISC_R_SUCCESS)
		goto cleanup_task;
	client->timerset = ISC_FALSE;

	client->delaytimer = NULL;

	client->message = NULL;
	result = dns_message_create(client->mctx, DNS_MESSAGE_INTENTPARSE,
				    &client->message);
	if (result != ISC_R_SUCCESS)
		goto cleanup_timer;

	/* XXXRTH  Hardwired constants */

	client->sendevent = isc_socket_socketevent(client->mctx, client,
						   ISC_SOCKEVENT_SENDDONE,
						   client_senddone, client);
	if (client->sendevent == NULL) {
		result = ISC_R_NOMEMORY;
		goto cleanup_message;
	}

	client->recvbuf = isc_mem_get(client->mctx, RECV_BUFFER_SIZE);
	if  (client->recvbuf == NULL) {
		result = ISC_R_NOMEMORY;
		goto cleanup_sendevent;
	}

	client->recvevent = isc_socket_socketevent(client->mctx, client,
						   ISC_SOCKEVENT_RECVDONE,
						   client_request, client);
	if (client->recvevent == NULL) {
		result = ISC_R_NOMEMORY;
		goto cleanup_recvbuf;
	}

	client->magic = NS_CLIENT_MAGIC;
	client->manager = NULL;
	client->state = NS_CLIENTSTATE_INACTIVE;
	client->newstate = NS_CLIENTSTATE_MAX;
	client->naccepts = 0;
	client->nreads = 0;
	client->nsends = 0;
	client->nrecvs = 0;
	client->nupdates = 0;
	client->nctls = 0;
	client->references = 0;
	client->attributes = 0;
	client->view = NULL;
	client->dispatch = NULL;
	client->udpsocket = NULL;
	client->tcplistener = NULL;
	client->tcpsocket = NULL;
	client->tcpmsg_valid = ISC_FALSE;
	client->tcpbuf = NULL;
	client->opt = NULL;
	client->udpsize = 512;
	client->dscp = -1;
	client->extflags = 0;
	client->ednsversion = -1;
	client->next = NULL;
	client->shutdown = NULL;
	client->shutdown_arg = NULL;
	client->signer = NULL;
	dns_name_init(&client->signername, NULL);
	client->mortal = ISC_FALSE;
	client->tcpquota = NULL;
	client->recursionquota = NULL;
	client->interface = NULL;
	client->peeraddr_valid = ISC_FALSE;
#ifdef ALLOW_FILTER_AAAA
	client->filter_aaaa = dns_aaaa_ok;
#endif
	client->needshutdown = ns_g_clienttest;

	ISC_EVENT_INIT(&client->ctlevent, sizeof(client->ctlevent), 0, NULL,
		       NS_EVENT_CLIENTCONTROL, client_start, client, client,
		       NULL, NULL);
	/*
	 * Initialize FORMERR cache to sentinel value that will not match
	 * any actual FORMERR response.
	 */
	isc_sockaddr_any(&client->formerrcache.addr);
	client->formerrcache.time = 0;
	client->formerrcache.id = 0;
	ISC_LINK_INIT(client, link);
	ISC_LINK_INIT(client, rlink);
	ISC_QLINK_INIT(client, ilink);

	/*
	 * We call the init routines for the various kinds of client here,
	 * after we have created an otherwise valid client, because some
	 * of them call routines that REQUIRE(NS_CLIENT_VALID(client)).
	 */
	result = ns_query_init(client);
	if (result != ISC_R_SUCCESS)
		goto cleanup_recvevent;

	result = isc_task_onshutdown(client->task, client_shutdown, client);
	if (result != ISC_R_SUCCESS)
		goto cleanup_query;

	CTRACE("create");

	*clientp = client;

	return (ISC_R_SUCCESS);

 cleanup_query:
	ns_query_free(client);

 cleanup_recvevent:
	isc_event_free((isc_event_t **)&client->recvevent);

 cleanup_recvbuf:
	isc_mem_put(client->mctx, client->recvbuf, RECV_BUFFER_SIZE);

 cleanup_sendevent:
	isc_event_free((isc_event_t **)&client->sendevent);

	client->magic = 0;

 cleanup_message:
	dns_message_destroy(&client->message);

 cleanup_timer:
	isc_timer_detach(&client->timer);

 cleanup_task:
	isc_task_detach(&client->task);

 cleanup_client:
	isc_mem_putanddetach(&client->mctx, client, sizeof(*client));

	return (result);
}

static void
client_read(ns_client_t *client) {
	isc_result_t result;

	CTRACE("read");

	result = dns_tcpmsg_readmessage(&client->tcpmsg, client->task,
					client_request, client);
	if (result != ISC_R_SUCCESS)
		goto fail;

	/*
	 * Set a timeout to limit the amount of time we will wait
	 * for a request on this TCP connection.
	 */
	ns_client_settimeout(client, 30);

	client->state = client->newstate = NS_CLIENTSTATE_READING;
	INSIST(client->nreads == 0);
	INSIST(client->recursionquota == NULL);
	client->nreads++;

	return;
 fail:
	ns_client_next(client, result);
}

static void
client_newconn(isc_task_t *task, isc_event_t *event) {
	ns_client_t *client = event->ev_arg;
	isc_socket_newconnev_t *nevent = (isc_socket_newconnev_t *)event;
	isc_result_t result;

	REQUIRE(event->ev_type == ISC_SOCKEVENT_NEWCONN);
	REQUIRE(NS_CLIENT_VALID(client));
	REQUIRE(client->task == task);

	UNUSED(task);

	INSIST(client->state == NS_CLIENTSTATE_READY);

	INSIST(client->naccepts == 1);
	client->naccepts--;

	LOCK(&client->interface->lock);
	INSIST(client->interface->ntcpcurrent > 0);
	client->interface->ntcpcurrent--;
	UNLOCK(&client->interface->lock);

	/*
	 * We must take ownership of the new socket before the exit
	 * check to make sure it gets destroyed if we decide to exit.
	 */
	if (nevent->result == ISC_R_SUCCESS) {
		client->tcpsocket = nevent->newsocket;
		isc_socket_setname(client->tcpsocket, "client-tcp", NULL);
		client->state = NS_CLIENTSTATE_READING;
		INSIST(client->recursionquota == NULL);

		(void)isc_socket_getpeername(client->tcpsocket,
					     &client->peeraddr);
		client->peeraddr_valid = ISC_TRUE;
		ns_client_log(client, NS_LOGCATEGORY_CLIENT,
			   NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(3),
			   "new TCP connection");
	} else {
		/*
		 * XXXRTH  What should we do?  We're trying to accept but
		 *	   it didn't work.  If we just give up, then TCP
		 *	   service may eventually stop.
		 *
		 *	   For now, we just go idle.
		 *
		 *	   Going idle is probably the right thing if the
		 *	   I/O was canceled.
		 */
		ns_client_log(client, NS_LOGCATEGORY_CLIENT,
			      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(3),
			      "accept failed: %s",
			      isc_result_totext(nevent->result));
	}

	if (exit_check(client))
		goto freeevent;

	if (nevent->result == ISC_R_SUCCESS) {
		int match;
		isc_netaddr_t netaddr;

		isc_netaddr_fromsockaddr(&netaddr, &client->peeraddr);

		if (ns_g_server->blackholeacl != NULL &&
		    dns_acl_match(&netaddr, NULL,
				  ns_g_server->blackholeacl,
				  &ns_g_server->aclenv,
				  &match, NULL) == ISC_R_SUCCESS &&
		    match > 0)
		{
			ns_client_log(client, DNS_LOGCATEGORY_SECURITY,
				      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(10),
				      "blackholed connection attempt");
			client->newstate = NS_CLIENTSTATE_READY;
			(void)exit_check(client);
			goto freeevent;
		}

		INSIST(client->tcpmsg_valid == ISC_FALSE);
		dns_tcpmsg_init(client->mctx, client->tcpsocket,
				&client->tcpmsg);
		client->tcpmsg_valid = ISC_TRUE;

		/*
		 * Let a new client take our place immediately, before
		 * we wait for a request packet.  If we don't,
		 * telnetting to port 53 (once per CPU) will
		 * deny service to legitimate TCP clients.
		 */
		result = isc_quota_attach(&ns_g_server->tcpquota,
					  &client->tcpquota);
		if (result == ISC_R_SUCCESS)
			result = ns_client_replace(client);
		if (result != ISC_R_SUCCESS) {
			ns_client_log(client, NS_LOGCATEGORY_CLIENT,
				      NS_LOGMODULE_CLIENT, ISC_LOG_WARNING,
				      "no more TCP clients: %s",
				      isc_result_totext(result));
		}

		client_read(client);
	}

 freeevent:
	isc_event_free(&event);
}

static void
client_accept(ns_client_t *client) {
	isc_result_t result;

	CTRACE("accept");

	result = isc_socket_accept(client->tcplistener, client->task,
				   client_newconn, client);
	if (result != ISC_R_SUCCESS) {
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "isc_socket_accept() failed: %s",
				 isc_result_totext(result));
		/*
		 * XXXRTH  What should we do?  We're trying to accept but
		 *	   it didn't work.  If we just give up, then TCP
		 *	   service may eventually stop.
		 *
		 *	   For now, we just go idle.
		 */
		return;
	}
	INSIST(client->naccepts == 0);
	client->naccepts++;
	LOCK(&client->interface->lock);
	client->interface->ntcpcurrent++;
	UNLOCK(&client->interface->lock);
}

static void
client_udprecv(ns_client_t *client) {
	isc_result_t result;
	isc_region_t r;

	CTRACE("udprecv");

	r.base = client->recvbuf;
	r.length = RECV_BUFFER_SIZE;
	result = isc_socket_recv2(client->udpsocket, &r, 1,
				  client->task, client->recvevent, 0);
	if (result != ISC_R_SUCCESS) {
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "isc_socket_recv2() failed: %s",
				 isc_result_totext(result));
		/*
		 * This cannot happen in the current implementation, since
		 * isc_socket_recv2() cannot fail if flags == 0.
		 *
		 * If this does fail, we just go idle.
		 */
		return;
	}
	INSIST(client->nrecvs == 0);
	client->nrecvs++;
}

void
ns_client_attach(ns_client_t *source, ns_client_t **targetp) {
	REQUIRE(NS_CLIENT_VALID(source));
	REQUIRE(targetp != NULL && *targetp == NULL);

	source->references++;
	ns_client_log(source, NS_LOGCATEGORY_CLIENT,
		      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(10),
		      "ns_client_attach: ref = %d", source->references);
	*targetp = source;
}

void
ns_client_detach(ns_client_t **clientp) {
	ns_client_t *client = *clientp;

	client->references--;
	INSIST(client->references >= 0);
	*clientp = NULL;
	ns_client_log(client, NS_LOGCATEGORY_CLIENT,
		      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(10),
		      "ns_client_detach: ref = %d", client->references);
	(void)exit_check(client);
}

isc_boolean_t
ns_client_shuttingdown(ns_client_t *client) {
	return (ISC_TF(client->newstate == NS_CLIENTSTATE_FREED));
}

isc_result_t
ns_client_replace(ns_client_t *client) {
	isc_result_t result;

	CTRACE("replace");

	REQUIRE(client != NULL);
	REQUIRE(client->manager != NULL);

	result = get_client(client->manager, client->interface,
			    client->dispatch, TCP_CLIENT(client));
	if (result != ISC_R_SUCCESS)
		return (result);

	/*
	 * The responsibility for listening for new requests is hereby
	 * transferred to the new client.  Therefore, the old client
	 * should refrain from listening for any more requests.
	 */
	client->mortal = ISC_TRUE;

	return (ISC_R_SUCCESS);
}

/***
 *** Client Manager
 ***/

static void
clientmgr_destroy(ns_clientmgr_t *manager) {
#if NMCTXS > 0
	int i;
#endif

	REQUIRE(ISC_LIST_EMPTY(manager->clients));

	MTRACE("clientmgr_destroy");

#if NMCTXS > 0
	for (i = 0; i < NMCTXS; i++) {
		if (manager->mctxpool[i] != NULL)
			isc_mem_detach(&manager->mctxpool[i]);
	}
#endif

	ISC_QUEUE_DESTROY(manager->inactive);
	DESTROYLOCK(&manager->lock);
	DESTROYLOCK(&manager->listlock);
	DESTROYLOCK(&manager->reclock);
	manager->magic = 0;
	isc_mem_put(manager->mctx, manager, sizeof(*manager));
}

isc_result_t
ns_clientmgr_create(isc_mem_t *mctx, isc_taskmgr_t *taskmgr,
		    isc_timermgr_t *timermgr, ns_clientmgr_t **managerp)
{
	ns_clientmgr_t *manager;
	isc_result_t result;
#if NMCTXS > 0
	int i;
#endif

	manager = isc_mem_get(mctx, sizeof(*manager));
	if (manager == NULL)
		return (ISC_R_NOMEMORY);

	result = isc_mutex_init(&manager->lock);
	if (result != ISC_R_SUCCESS)
		goto cleanup_manager;

	result = isc_mutex_init(&manager->listlock);
	if (result != ISC_R_SUCCESS)
		goto cleanup_lock;

	result = isc_mutex_init(&manager->reclock);
	if (result != ISC_R_SUCCESS)
		goto cleanup_listlock;

	manager->mctx = mctx;
	manager->taskmgr = taskmgr;
	manager->timermgr = timermgr;
	manager->exiting = ISC_FALSE;
	ISC_LIST_INIT(manager->clients);
	ISC_LIST_INIT(manager->recursing);
	ISC_QUEUE_INIT(manager->inactive, ilink);
#if NMCTXS > 0
	manager->nextmctx = 0;
	for (i = 0; i < NMCTXS; i++)
		manager->mctxpool[i] = NULL; /* will be created on-demand */
#endif
	manager->magic = MANAGER_MAGIC;

	MTRACE("create");

	*managerp = manager;

	return (ISC_R_SUCCESS);

 cleanup_listlock:
	(void) isc_mutex_destroy(&manager->listlock);

 cleanup_lock:
	(void) isc_mutex_destroy(&manager->lock);

 cleanup_manager:
	isc_mem_put(manager->mctx, manager, sizeof(*manager));

	return (result);
}

void
ns_clientmgr_destroy(ns_clientmgr_t **managerp) {
	isc_result_t result;
	ns_clientmgr_t *manager;
	ns_client_t *client;
	isc_boolean_t need_destroy = ISC_FALSE, unlock = ISC_FALSE;

	REQUIRE(managerp != NULL);
	manager = *managerp;
	REQUIRE(VALID_MANAGER(manager));

	MTRACE("destroy");

	/*
	 * Check for success because we may already be task-exclusive
	 * at this point.  Only if we succeed at obtaining an exclusive
	 * lock now will we need to relinquish it later.
	 */
	result = isc_task_beginexclusive(ns_g_server->task);
	if (result == ISC_R_SUCCESS)
		unlock = ISC_TRUE;

	manager->exiting = ISC_TRUE;

	for (client = ISC_LIST_HEAD(manager->clients);
	     client != NULL;
	     client = ISC_LIST_NEXT(client, link))
		isc_task_shutdown(client->task);

	if (ISC_LIST_EMPTY(manager->clients))
		need_destroy = ISC_TRUE;

	if (unlock)
		isc_task_endexclusive(ns_g_server->task);

	if (need_destroy)
		clientmgr_destroy(manager);

	*managerp = NULL;
}

static isc_result_t
get_client(ns_clientmgr_t *manager, ns_interface_t *ifp,
	   dns_dispatch_t *disp, isc_boolean_t tcp)
{
	isc_result_t result = ISC_R_SUCCESS;
	isc_event_t *ev;
	ns_client_t *client;
	MTRACE("get client");

	REQUIRE(manager != NULL);

	if (manager->exiting)
		return (ISC_R_SHUTTINGDOWN);

	/*
	 * Allocate a client.  First try to get a recycled one;
	 * if that fails, make a new one.
	 */
	client = NULL;
	if (!ns_g_clienttest)
		ISC_QUEUE_POP(manager->inactive, ilink, client);

	if (client != NULL)
		MTRACE("recycle");
	else {
		MTRACE("create new");

		LOCK(&manager->lock);
		result = client_create(manager, &client);
		UNLOCK(&manager->lock);
		if (result != ISC_R_SUCCESS)
			return (result);

		LOCK(&manager->listlock);
		ISC_LIST_APPEND(manager->clients, client, link);
		UNLOCK(&manager->listlock);
	}

	client->manager = manager;
	ns_interface_attach(ifp, &client->interface);
	client->state = NS_CLIENTSTATE_READY;
	INSIST(client->recursionquota == NULL);

	client->dscp = ifp->dscp;

	if (tcp) {
		client->attributes |= NS_CLIENTATTR_TCP;
		isc_socket_attach(ifp->tcpsocket,
				  &client->tcplistener);
	} else {
		isc_socket_t *sock;

		dns_dispatch_attach(disp, &client->dispatch);
		sock = dns_dispatch_getsocket(client->dispatch);
		isc_socket_attach(sock, &client->udpsocket);
	}

	INSIST(client->nctls == 0);
	client->nctls++;
	ev = &client->ctlevent;
	isc_task_send(client->task, &ev);

	return (ISC_R_SUCCESS);
}

isc_result_t
ns_clientmgr_createclients(ns_clientmgr_t *manager, unsigned int n,
			   ns_interface_t *ifp, isc_boolean_t tcp)
{
	isc_result_t result = ISC_R_SUCCESS;
	unsigned int disp;

	REQUIRE(VALID_MANAGER(manager));
	REQUIRE(n > 0);

	MTRACE("createclients");

	for (disp = 0; disp < n; disp++) {
		result = get_client(manager, ifp, ifp->udpdispatch[disp], tcp);
		if (result != ISC_R_SUCCESS)
			break;
	}

	return (result);
}

isc_sockaddr_t *
ns_client_getsockaddr(ns_client_t *client) {
	return (&client->peeraddr);
}

isc_result_t
ns_client_checkaclsilent(ns_client_t *client, isc_netaddr_t *netaddr,
			 dns_acl_t *acl, isc_boolean_t default_allow)
{
	isc_result_t result;
	isc_netaddr_t tmpnetaddr;
	int match;

	if (acl == NULL) {
		if (default_allow)
			goto allow;
		else
			goto deny;
	}

	if (netaddr == NULL) {
		isc_netaddr_fromsockaddr(&tmpnetaddr, &client->peeraddr);
		netaddr = &tmpnetaddr;
	}

	result = dns_acl_match(netaddr, client->signer, acl,
			       &ns_g_server->aclenv, &match, NULL);

	if (result != ISC_R_SUCCESS)
		goto deny; /* Internal error, already logged. */
	if (match > 0)
		goto allow;
	goto deny; /* Negative match or no match. */

 allow:
	return (ISC_R_SUCCESS);

 deny:
	return (DNS_R_REFUSED);
}

isc_result_t
ns_client_checkacl(ns_client_t *client, isc_sockaddr_t *sockaddr,
		   const char *opname, dns_acl_t *acl,
		   isc_boolean_t default_allow, int log_level)
{
	isc_result_t result;
	isc_netaddr_t netaddr;

	if (sockaddr != NULL)
		isc_netaddr_fromsockaddr(&netaddr, sockaddr);

	result = ns_client_checkaclsilent(client, sockaddr ? &netaddr : NULL,
					  acl, default_allow);

	pfilter_notify(result, client, opname);
	if (result == ISC_R_SUCCESS)
		ns_client_log(client, DNS_LOGCATEGORY_SECURITY,
			      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(3),
			      "%s approved", opname);
	else
		ns_client_log(client, DNS_LOGCATEGORY_SECURITY,
			      NS_LOGMODULE_CLIENT,
			      log_level, "%s denied", opname);
	return (result);
}

static void
ns_client_name(ns_client_t *client, char *peerbuf, size_t len) {
	if (client->peeraddr_valid)
		isc_sockaddr_format(&client->peeraddr, peerbuf,
				    (unsigned int)len);
	else
		snprintf(peerbuf, len, "@%p", client);
}

void
ns_client_logv(ns_client_t *client, isc_logcategory_t *category,
	       isc_logmodule_t *module, int level, const char *fmt, va_list ap)
{
	char msgbuf[4096];
	char peerbuf[ISC_SOCKADDR_FORMATSIZE];
	char signerbuf[DNS_NAME_FORMATSIZE], qnamebuf[DNS_NAME_FORMATSIZE];
	const char *viewname = "";
	const char *sep1 = "", *sep2 = "", *sep3 = "", *sep4 = "";
	const char *signer = "", *qname = "";
	dns_name_t *q = NULL;

	vsnprintf(msgbuf, sizeof(msgbuf), fmt, ap);

	ns_client_name(client, peerbuf, sizeof(peerbuf));

	if (client->signer != NULL) {
		dns_name_format(client->signer, signerbuf, sizeof(signerbuf));
		sep1 = "/key ";
		signer = signerbuf;
	}

	q = client->query.origqname != NULL
		? client->query.origqname : client->query.qname;
	if (q != NULL) {
		dns_name_format(q, qnamebuf, sizeof(qnamebuf));
		sep2 = " (";
		sep3 = ")";
		qname = qnamebuf;
	}

	if (client->view != NULL && strcmp(client->view->name, "_bind") != 0 &&
	    strcmp(client->view->name, "_default") != 0) {
		sep4 = ": view ";
		viewname = client->view->name;
	}

	isc_log_write(ns_g_lctx, category, module, level,
		      "client %s%s%s%s%s%s%s%s: %s",
		      peerbuf, sep1, signer, sep2, qname, sep3,
		      sep4, viewname, msgbuf);
}

void
ns_client_log(ns_client_t *client, isc_logcategory_t *category,
	   isc_logmodule_t *module, int level, const char *fmt, ...)
{
	va_list ap;

	if (! isc_log_wouldlog(ns_g_lctx, level))
		return;

	va_start(ap, fmt);
	ns_client_logv(client, category, module, level, fmt, ap);
	va_end(ap);
}

void
ns_client_aclmsg(const char *msg, dns_name_t *name, dns_rdatatype_t type,
		 dns_rdataclass_t rdclass, char *buf, size_t len)
{
	char namebuf[DNS_NAME_FORMATSIZE];
	char typebuf[DNS_RDATATYPE_FORMATSIZE];
	char classbuf[DNS_RDATACLASS_FORMATSIZE];

	dns_name_format(name, namebuf, sizeof(namebuf));
	dns_rdatatype_format(type, typebuf, sizeof(typebuf));
	dns_rdataclass_format(rdclass, classbuf, sizeof(classbuf));
	(void)snprintf(buf, len, "%s '%s/%s/%s'", msg, namebuf, typebuf,
		       classbuf);
}

static void
ns_client_dumpmessage(ns_client_t *client, const char *reason) {
	isc_buffer_t buffer;
	char *buf = NULL;
	int len = 1024;
	isc_result_t result;

	if (!isc_log_wouldlog(ns_g_lctx, ISC_LOG_DEBUG(1)))
		return;

	/*
	 * Note that these are multiline debug messages.  We want a newline
	 * to appear in the log after each message.
	 */

	do {
		buf = isc_mem_get(client->mctx, len);
		if (buf == NULL)
			break;
		isc_buffer_init(&buffer, buf, len);
		result = dns_message_totext(client->message,
					    &dns_master_style_debug,
					    0, &buffer);
		if (result == ISC_R_NOSPACE) {
			isc_mem_put(client->mctx, buf, len);
			len += 1024;
		} else if (result == ISC_R_SUCCESS)
			ns_client_log(client, NS_LOGCATEGORY_UNMATCHED,
				      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(1),
				      "%s\n%.*s", reason,
				       (int)isc_buffer_usedlength(&buffer),
				       buf);
	} while (result == ISC_R_NOSPACE);

	if (buf != NULL)
		isc_mem_put(client->mctx, buf, len);
}

void
ns_client_dumprecursing(FILE *f, ns_clientmgr_t *manager) {
	ns_client_t *client;
	char namebuf[DNS_NAME_FORMATSIZE];
	char original[DNS_NAME_FORMATSIZE];
	char peerbuf[ISC_SOCKADDR_FORMATSIZE];
	char typebuf[DNS_RDATATYPE_FORMATSIZE];
	char classbuf[DNS_RDATACLASS_FORMATSIZE];
	const char *name;
	const char *sep;
	const char *origfor;
	dns_rdataset_t *rdataset;

	REQUIRE(VALID_MANAGER(manager));

	LOCK(&manager->reclock);
	client = ISC_LIST_HEAD(manager->recursing);
	while (client != NULL) {
		INSIST(client->state == NS_CLIENTSTATE_RECURSING);

		ns_client_name(client, peerbuf, sizeof(peerbuf));
		if (client->view != NULL &&
		    strcmp(client->view->name, "_bind") != 0 &&
		    strcmp(client->view->name, "_default") != 0) {
			name = client->view->name;
			sep = ": view ";
		} else {
			name = "";
			sep = "";
		}

		LOCK(&client->query.fetchlock);
		INSIST(client->query.qname != NULL);
		dns_name_format(client->query.qname, namebuf, sizeof(namebuf));
		if (client->query.qname != client->query.origqname &&
		    client->query.origqname != NULL) {
			origfor = " for ";
			dns_name_format(client->query.origqname, original,
					sizeof(original));
		} else {
			origfor = "";
			original[0] = '\0';
		}
		rdataset = ISC_LIST_HEAD(client->query.qname->list);
		if (rdataset == NULL && client->query.origqname != NULL)
			rdataset = ISC_LIST_HEAD(client->query.origqname->list);
		if (rdataset != NULL) {
			dns_rdatatype_format(rdataset->type, typebuf,
					     sizeof(typebuf));
			dns_rdataclass_format(rdataset->rdclass, classbuf,
					      sizeof(classbuf));
		} else {
			strcpy(typebuf, "-");
			strcpy(classbuf, "-");
		}
		UNLOCK(&client->query.fetchlock);
		fprintf(f, "; client %s%s%s: id %u '%s/%s/%s'%s%s "
			"requesttime %d\n", peerbuf, sep, name,
			client->message->id, namebuf, typebuf, classbuf,
			origfor, original, client->requesttime);
		client = ISC_LIST_NEXT(client, rlink);
	}
	UNLOCK(&manager->reclock);
}

void
ns_client_qnamereplace(ns_client_t *client, dns_name_t *name) {
	LOCK(&client->query.fetchlock);
	if (client->query.restarts > 0) {
		/*
		 * client->query.qname was dynamically allocated.
		 */
		dns_message_puttempname(client->message,
					&client->query.qname);
	}
	client->query.qname = name;
	UNLOCK(&client->query.fetchlock);
}

isc_result_t
ns_client_sourceip(dns_clientinfo_t *ci, isc_sockaddr_t **addrp) {
	ns_client_t *client = (ns_client_t *) ci->data;

	REQUIRE(NS_CLIENT_VALID(client));
	REQUIRE(addrp != NULL);

	*addrp = &client->peeraddr;
	return (ISC_R_SUCCESS);
}
