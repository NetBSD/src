/*	$NetBSD: client.c,v 1.5.4.2 2019/10/17 19:34:22 martin Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#include <config.h>

#include <inttypes.h>
#include <stdbool.h>

#include <isc/aes.h>
#include <isc/formatcheck.h>
#include <isc/fuzz.h>
#include <isc/hmac.h>
#include <isc/mutex.h>
#include <isc/once.h>
#include <isc/nonce.h>
#include <isc/platform.h>
#include <isc/print.h>
#include <isc/queue.h>
#include <isc/random.h>
#include <isc/safe.h>
#include <isc/serial.h>
#include <isc/siphash.h>
#include <isc/stats.h>
#include <isc/stdio.h>
#include <isc/string.h>
#include <isc/task.h>
#include <isc/timer.h>
#include <isc/util.h>

#include <dns/adb.h>
#include <dns/badcache.h>
#include <dns/db.h>
#include <dns/dispatch.h>
#include <dns/dnstap.h>
#include <dns/cache.h>
#include <dns/edns.h>
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

#include <ns/interfacemgr.h>
#include <ns/log.h>
#include <ns/notify.h>
#include <ns/server.h>
#include <ns/stats.h>
#include <ns/update.h>

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
#define MTRACE(m)	isc_log_write(ns_lctx, \
				      NS_LOGCATEGORY_CLIENT, \
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

#define NMCTXS				100
/*%<
 * Number of 'mctx pools' for clients. (Should this be configurable?)
 * When enabling threads, we use a pool of memory contexts shared by
 * client objects, since concurrent access to a shared context would cause
 * heavy contentions.  The above constant is expected to be enough for
 * completely avoiding contentions among threads for an authoritative-only
 * server.
 */

#define COOKIE_SIZE 24U /* 8 + 4 + 4 + 8 */
#define ECS_SIZE 20U /* 2 + 1 + 1 + [0..16] */

#define WANTNSID(x) (((x)->attributes & NS_CLIENTATTR_WANTNSID) != 0)
#define WANTEXPIRE(x) (((x)->attributes & NS_CLIENTATTR_WANTEXPIRE) != 0)
#define WANTPAD(x) (((x)->attributes & NS_CLIENTATTR_WANTPAD) != 0)
#define USEKEEPALIVE(x) (((x)->attributes & NS_CLIENTATTR_USEKEEPALIVE) != 0)

/*% nameserver client manager structure */
struct ns_clientmgr {
	/* Unlocked. */
	unsigned int			magic;

	/* The queue object has its own locks */
	client_queue_t			inactive;     /*%< To be recycled */

	isc_mem_t *			mctx;
	ns_server_t *			sctx;
	isc_taskmgr_t *			taskmgr;
	isc_timermgr_t *		timermgr;
	isc_task_t *			excl;

	/* Lock covers manager state. */
	isc_mutex_t			lock;
	bool			exiting;

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

LIBNS_EXTERNAL_DATA unsigned int ns_client_requests;

static void read_settimeout(ns_client_t *client, bool newconn);
static void client_read(ns_client_t *client, bool newconn);
static void client_accept(ns_client_t *client);
static void client_udprecv(ns_client_t *client);
static void clientmgr_destroy(ns_clientmgr_t *manager);
static bool exit_check(ns_client_t *client);
static void ns_client_endrequest(ns_client_t *client);
static void client_start(isc_task_t *task, isc_event_t *event);
static void ns_client_dumpmessage(ns_client_t *client, const char *reason);
static isc_result_t get_client(ns_clientmgr_t *manager, ns_interface_t *ifp,
			       dns_dispatch_t *disp, bool tcp);
static isc_result_t get_worker(ns_clientmgr_t *manager, ns_interface_t *ifp,
			       isc_socket_t *sock, ns_client_t *oldclient);
static void compute_cookie(ns_client_t *client, uint32_t when,
			   uint32_t nonce, const unsigned char *secret,
			   isc_buffer_t *buf);

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
				 &interval, false);
	client->timerset = true;
	if (result != ISC_R_SUCCESS) {
		ns_client_log(client, NS_LOGCATEGORY_CLIENT,
			      NS_LOGMODULE_CLIENT, ISC_LOG_ERROR,
			      "setting timeout: %s",
			      isc_result_totext(result));
		/* Continue anyway. */
	}
}

static void
read_settimeout(ns_client_t *client, bool newconn) {
	isc_result_t result;
	isc_interval_t interval;
	unsigned int ds;

	if (newconn)
		ds = client->sctx->initialtimo;
	else if (USEKEEPALIVE(client))
		ds = client->sctx->keepalivetimo;
	else
		ds = client->sctx->idletimo;

	isc_interval_set(&interval, ds / 10, 100000000 * (ds % 10));
	result = isc_timer_reset(client->timer, isc_timertype_once, NULL,
				 &interval, false);
	client->timerset = true;
	if (result != ISC_R_SUCCESS) {
		ns_client_log(client, NS_LOGCATEGORY_CLIENT,
			      NS_LOGMODULE_CLIENT, ISC_LOG_ERROR,
			      "setting timeout: %s",
			      isc_result_totext(result));
		/* Continue anyway. */
	}
}

/*%
 * Allocate a reference-counted object that will maintain a single pointer to
 * the (also reference-counted) TCP client quota, shared between all the
 * clients processing queries on a single TCP connection, so that all
 * clients sharing the one socket will together consume only one slot in
 * the 'tcp-clients' quota.
 */
static isc_result_t
tcpconn_init(ns_client_t *client, bool force) {
	isc_result_t result;
	isc_quota_t *quota = NULL;
	ns_tcpconn_t *tconn = NULL;

	REQUIRE(client->tcpconn == NULL);

	/*
	 * Try to attach to the quota first, so we won't pointlessly
	 * allocate memory for a tcpconn object if we can't get one.
	 */
	if (force) {
		result = isc_quota_force(&client->sctx->tcpquota, &quota);
	} else {
		result = isc_quota_attach(&client->sctx->tcpquota, &quota);
	}
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	/*
	 * A global memory context is used for the allocation as different
	 * client structures may have different memory contexts assigned and a
	 * reference counter allocated here might need to be freed by a
	 * different client.  The performance impact caused by memory context
	 * contention here is expected to be negligible, given that this code
	 * is only executed for TCP connections.
	 */
	tconn = isc_mem_allocate(client->sctx->mctx, sizeof(*tconn));

	isc_refcount_init(&tconn->refs, 1);
	tconn->tcpquota = quota;
	quota = NULL;
	tconn->pipelined = false;

	client->tcpconn = tconn;

	return (ISC_R_SUCCESS);
}

/*%
 * Increase the count of client structures sharing the TCP connection
 * that 'source' is associated with; add a pointer to the same tcpconn
 * to 'target', thus associating it with the same TCP connection.
 */
static void
tcpconn_attach(ns_client_t *source, ns_client_t *target) {
	int old_refs;

	REQUIRE(source->tcpconn != NULL);
	REQUIRE(target->tcpconn == NULL);
	REQUIRE(source->tcpconn->pipelined);

	old_refs = isc_refcount_increment(&source->tcpconn->refs);
	INSIST(old_refs > 0);
	target->tcpconn = source->tcpconn;
}

/*%
 * Decrease the count of client structures sharing the TCP connection that
 * 'client' is associated with.  If this is the last client using this TCP
 * connection, we detach from the TCP quota and free the tcpconn
 * object. Either way, client->tcpconn is set to NULL.
 */
static void
tcpconn_detach(ns_client_t *client) {
	ns_tcpconn_t *tconn = NULL;
	int old_refs;

	REQUIRE(client->tcpconn != NULL);

	tconn = client->tcpconn;
	client->tcpconn = NULL;

	old_refs = isc_refcount_decrement(&tconn->refs);
	INSIST(old_refs > 0);

	if (old_refs == 1) {
		isc_quota_detach(&tconn->tcpquota);
		isc_mem_free(client->sctx->mctx, tconn);
	}
}

/*%
 * Mark a client as active and increment the interface's 'ntcpactive'
 * counter, as a signal that there is at least one client servicing
 * TCP queries for the interface. If we reach the TCP client quota at
 * some point, this will be used to determine whether a quota overrun
 * should be permitted.
 *
 * Marking the client active with the 'tcpactive' flag ensures proper
 * accounting, by preventing us from incrementing or decrementing
 * 'ntcpactive' more than once per client.
 */
static void
mark_tcp_active(ns_client_t *client, bool active) {
	if (active && !client->tcpactive) {
		isc_refcount_increment0(&client->interface->ntcpactive);
		client->tcpactive = active;
	} else if (!active && client->tcpactive) {
		uint32_t old =
			isc_refcount_decrement(&client->interface->ntcpactive);
		INSIST(old > 0);
		client->tcpactive = active;
	}
}

/*%
 * Check for a deactivation or shutdown request and take appropriate
 * action.  Returns true if either is in progress; in this case
 * the caller must no longer use the client object as it may have been
 * freed.
 */
static bool
exit_check(ns_client_t *client) {
	bool destroy_manager = false;
	ns_clientmgr_t *manager = NULL;

	REQUIRE(NS_CLIENT_VALID(client));
	manager = client->manager;

	if (client->state <= client->newstate)
		return (false); /* Business as usual. */

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
			return (true);

		/*
		 * We are trying to abort request processing.
		 */
		if (client->nsends > 0) {
			isc_socket_t *sock;
			if (TCP_CLIENT(client))
				sock = client->tcpsocket;
			else
				sock = client->udpsocket;
			isc_socket_cancel(sock, client->task,
					  ISC_SOCKCANCEL_SEND);
		}

		if (! (client->nsends == 0 && client->nrecvs == 0 &&
		       client->references == 0))
		{
			/*
			 * Still waiting for I/O cancel completion.
			 * or lingering references.
			 */
			return (true);
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
			INSIST(client->tcpconn != NULL);
			if (!client->tcpconn->pipelined) {
				client_read(client, false);
				client->newstate = NS_CLIENTSTATE_MAX;
				return (true); /* We're done. */
			} else if (client->mortal) {
				client->newstate = NS_CLIENTSTATE_INACTIVE;
			} else
				return (false);
		}
	}

	if (client->state == NS_CLIENTSTATE_READING) {
		/*
		 * We are trying to abort the current TCP connection,
		 * if any.
		 */
		INSIST(client->recursionquota == NULL);
		INSIST(client->newstate <= NS_CLIENTSTATE_READY);

		if (client->nreads > 0) {
			dns_tcpmsg_cancelread(&client->tcpmsg);
			/* Still waiting for read cancel completion? */
			if (client->nreads > 0) {
				return (true);
			}
		}

		if (client->tcpmsg_valid) {
			dns_tcpmsg_invalidate(&client->tcpmsg);
			client->tcpmsg_valid = false;
		}

		/*
		 * Soon the client will be ready to accept a new TCP
		 * connection or UDP request, but we may have enough
		 * clients doing that already.  Check whether this client
		 * needs to remain active and allow it go inactive if
		 * not.
		 *
		 * UDP clients always go inactive at this point, but a TCP
		 * client may need to stay active and return to READY
		 * state if no other clients are available to listen
		 * for TCP requests on this interface.
		 *
		 * Regardless, if we're going to FREED state, that means
		 * the system is shutting down and we don't need to
		 * retain clients.
		 */
		if (client->mortal && TCP_CLIENT(client) &&
		    client->newstate != NS_CLIENTSTATE_FREED &&
		    (client->sctx->options & NS_SERVER_CLIENTTEST) == 0 &&
		    isc_refcount_current(&client->interface->ntcpaccepting) == 0)
		{
			/* Nobody else is accepting */
			client->mortal = false;
			client->newstate = NS_CLIENTSTATE_READY;
		}

		/*
		 * Detach from TCP connection and TCP client quota,
		 * if appropriate. If this is the last reference to
		 * the TCP connection in our pipeline group, the
		 * TCP quota slot will be released.
		 */
		if (client->tcpconn) {
			tcpconn_detach(client);
		}

		if (client->tcpsocket != NULL) {
			CTRACE("closetcp");
			isc_socket_detach(&client->tcpsocket);
			mark_tcp_active(client, false);
		}

		if (client->timerset) {
			(void)isc_timer_reset(client->timer,
					      isc_timertype_inactive,
					      NULL, NULL, true);
			client->timerset = false;
		}

		client->peeraddr_valid = false;

		client->state = NS_CLIENTSTATE_READY;

		/*
		 * We don't need the client; send it to the inactive
		 * queue for recycling.
		 */
		if (client->mortal) {
			if (client->newstate > NS_CLIENTSTATE_INACTIVE) {
				client->newstate = NS_CLIENTSTATE_INACTIVE;
			}
		}

		if (NS_CLIENTSTATE_READY == client->newstate) {
			if (TCP_CLIENT(client)) {
				client_accept(client);
			} else {
				client_udprecv(client);
			}
			client->newstate = NS_CLIENTSTATE_MAX;
			return (true);
		}
	}

	if (client->state == NS_CLIENTSTATE_READY) {
		INSIST(client->newstate <= NS_CLIENTSTATE_INACTIVE);

		/*
		 * We are trying to enter the inactive state.
		 */
		if (client->naccepts > 0) {
			isc_socket_cancel(client->tcplistener, client->task,
					  ISC_SOCKCANCEL_ACCEPT);
			/* Still waiting for accept cancel completion? */
			if (client->naccepts > 0) {
				return (true);
			}
		}

		/* Accept cancel is complete. */
		if (client->nrecvs > 0) {
			isc_socket_cancel(client->udpsocket, client->task,
					  ISC_SOCKCANCEL_RECV);
			/* Still waiting for recv cancel completion? */
			if (client->nrecvs > 0) {
				return (true);
			}
		}

		/* Still waiting for control event to be delivered */
		if (client->nctls > 0) {
			return (true);
		}

		INSIST(client->naccepts == 0);
		INSIST(client->recursionquota == NULL);
		if (client->tcplistener != NULL) {
			isc_socket_detach(&client->tcplistener);
			mark_tcp_active(client, false);
		}
		if (client->udpsocket != NULL) {
			isc_socket_detach(&client->udpsocket);
		}

		/* Deactivate the client. */
		if (client->interface != NULL) {
			ns_interface_detach(&client->interface);
		}

		if (client->dispatch != NULL) {
			dns_dispatch_detach(&client->dispatch);
		}

		client->attributes = 0;
		client->mortal = false;
		client->sendcb = NULL;

		if (client->keytag != NULL) {
			isc_mem_put(client->mctx, client->keytag,
				    client->keytag_len);
			client->keytag_len = 0;
		}

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
			if ((client->sctx->options &
			     NS_SERVER_CLIENTTEST) == 0 &&
			    manager != NULL && !manager->exiting)
			{
				ISC_QUEUE_PUSH(manager->inactive, client,
					       ilink);
			}
			if (client->needshutdown) {
				isc_task_shutdown(client->task);
			}
			return (true);
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
				destroy_manager = true;
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
		if (client->keytag != NULL) {
			isc_mem_put(client->mctx, client->keytag,
				    client->keytag_len);
			client->keytag_len = 0;
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
		if ((client->sctx->options & NS_SERVER_CLIENTTEST) != 0 &&
		    isc_mem_references(client->mctx) != 1)
		{
			isc_mem_stats(client->mctx, stderr);
			INSIST(0);
			ISC_UNREACHABLE();
		}

		/*
		 * Destroy the fetchlock mutex that was created in
		 * ns_query_init().
		 */
		isc_mutex_destroy(&client->query.fetchlock);

		if (client->sctx != NULL)
			ns_server_detach(&client->sctx);

		isc_mem_putanddetach(&client->mctx, client, sizeof(*client));
	}

	if (destroy_manager && manager != NULL)
		clientmgr_destroy(manager);

	return (true);
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
		if (client->tcpconn != NULL) {
			client_read(client, false);
		} else {
			client_accept(client);
		}
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
	client->needshutdown = false;
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

	if (client->view != NULL) {
#ifdef ENABLE_AFL
		if (client->sctx->fuzztype == isc_fuzz_resolver) {
			dns_cache_clean(client->view->cache, INT_MAX);
			dns_adb_flush(client->view->adb);
		}
#endif
		dns_view_detach(&client->view);
	}
	if (client->opt != NULL) {
		INSIST(dns_rdataset_isassociated(client->opt));
		dns_rdataset_disassociate(client->opt);
		dns_message_puttemprdataset(client->message, &client->opt);
	}

	client->signer = NULL;
	client->udpsize = 512;
	client->extflags = 0;
	client->ednsversion = -1;
	dns_ecs_init(&client->ecs);
	dns_message_reset(client->message, DNS_MESSAGE_INTENTPARSE);

	if (client->recursionquota != NULL) {
		isc_quota_detach(&client->recursionquota);
		ns_stats_decrement(client->sctx->nsstats,
				   ns_statscounter_recursclients);
	}

	/*
	 * Clear all client attributes that are specific to
	 * the request; that's all except the TCP flag.
	 */
	client->attributes &= NS_CLIENTATTR_TCP;
#ifdef ENABLE_AFL
	if (client->sctx->fuzznotify != NULL &&
	    (client->sctx->fuzztype == isc_fuzz_client ||
	     client->sctx->fuzztype == isc_fuzz_tcpclient ||
	     client->sctx->fuzztype == isc_fuzz_resolver))
	{
		client->sctx->fuzznotify();
	}
#endif /* ENABLE_AFL */

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
		    isc_buffer_t *tcpbuffer, uint32_t length,
		    unsigned char *sendbuf, unsigned char **datap)
{
	unsigned char *data;
	uint32_t bufsize;
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
			isc_buffer_putuint16(buffer, (uint16_t)length);
		}
	} else {
		data = sendbuf;
		if ((client->attributes & NS_CLIENTATTR_HAVECOOKIE) == 0) {
			if (client->view != NULL)
				bufsize = client->view->nocookieudp;
			else
				bufsize = 512;
		} else
			bufsize = client->udpsize;
		if (bufsize > client->udpsize)
			bufsize = client->udpsize;
		if (bufsize > SEND_BUFFER_SIZE)
			bufsize = SEND_BUFFER_SIZE;
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
	isc_socket_t *sock;
	isc_netaddr_t netaddr;
	int match;
	unsigned int sockflags = ISC_SOCKFLAG_IMMEDIATE;

	if (TCP_CLIENT(client)) {
		sock = client->tcpsocket;
		address = NULL;
	} else {
		dns_aclenv_t *env =
			ns_interfacemgr_getaclenv(client->interface->mgr);

		sock = client->udpsocket;
		address = &client->peeraddr;

		isc_netaddr_fromsockaddr(&netaddr, &client->peeraddr);
		if (client->sctx->blackholeacl != NULL &&
		    (dns_acl_match(&netaddr, NULL, client->sctx->blackholeacl,
				   env, &match, NULL) == ISC_R_SUCCESS) &&
		    match > 0)
		{
			return (DNS_R_BLACKHOLED);
		}
		sockflags |= ISC_SOCKFLAG_NORETRY;
	}

	if ((client->attributes & NS_CLIENTATTR_PKTINFO) != 0 &&
	    (client->attributes & NS_CLIENTATTR_MULTICAST) == 0)
		pktinfo = &client->pktinfo;
	else
		pktinfo = NULL;

	if (client->dispatch != NULL) {
		isc_dscp_t dscp = dns_dispatch_getdscp(client->dispatch);
		if (dscp != -1) {
			client->dscp = dscp;
		}
	}

	if (client->dscp == -1) {
		client->sendevent->attributes &= ~ISC_SOCKEVENTATTR_DSCP;
		client->sendevent->dscp = 0;
	} else {
		client->sendevent->attributes |= ISC_SOCKEVENTATTR_DSCP;
		client->sendevent->dscp = client->dscp;
	}

	isc_buffer_usedregion(buffer, &r);

	/*
	 * If this is a UDP client and the IPv6 packet can't be
	 * encapsulated without generating a PTB on a 1500 octet
	 * MTU link force fragmentation at 1280 if it is a IPv6
	 * response.
	 */
	client->sendevent->attributes &= ~ISC_SOCKEVENTATTR_USEMINMTU;
	if (!TCP_CLIENT(client) && r.length > 1432)
		client->sendevent->attributes |= ISC_SOCKEVENTATTR_USEMINMTU;

	CTRACE("sendto");

	result = isc_socket_sendto2(sock, &r, client->task,
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
	bool cleanup_cctx = false;
	unsigned char sendbuf[SEND_BUFFER_SIZE];
	unsigned int render_opts;
	unsigned int preferred_glue;
	bool opt_included = false;
	size_t respsize;
	dns_aclenv_t *env = ns_interfacemgr_getaclenv(client->interface->mgr);
#ifdef HAVE_DNSTAP
	unsigned char zone[DNS_NAME_MAXWIRE];
	dns_dtmsgtype_t dtmsgtype;
	isc_region_t zr;
#endif /* HAVE_DNSTAP */

	REQUIRE(NS_CLIENT_VALID(client));

	CTRACE("send");

	if (client->message->opcode == dns_opcode_query &&
	    (client->attributes & NS_CLIENTATTR_RA) != 0)
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
	if (preferred_glue == 0) {
		if (isc_sockaddr_pf(&client->peeraddr) == AF_INET)
			preferred_glue = DNS_MESSAGERENDER_PREFER_A;
		else
			preferred_glue = DNS_MESSAGERENDER_PREFER_AAAA;
	}

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
		    !dns_acl_allowed(&netaddr, name,
				     client->view->nocasecompress, env))
		{
			dns_compress_setsensitive(&cctx, true);
		}

		if (client->view->msgcompression == false) {
			dns_compress_disable(&cctx);
		}
	}
	cleanup_cctx = true;

	result = dns_message_renderbegin(client->message, &cctx, &buffer);
	if (result != ISC_R_SUCCESS)
		goto done;

	if (client->opt != NULL) {
		result = dns_message_setopt(client->message, client->opt);
		opt_included = true;
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

#ifdef HAVE_DNSTAP
	memset(&zr, 0, sizeof(zr));
	if (((client->message->flags & DNS_MESSAGEFLAG_AA) != 0) &&
	    (client->query.authzone != NULL))
	{
		isc_buffer_t b;
		dns_name_t *zo =
			dns_zone_getorigin(client->query.authzone);

		isc_buffer_init(&b, zone, sizeof(zone));
		dns_compress_setmethods(&cctx, DNS_COMPRESS_NONE);
		result = dns_name_towire(zo, &cctx, &b);
		if (result == ISC_R_SUCCESS)
			isc_buffer_usedregion(&b, &zr);
	}

	if (client->message->opcode == dns_opcode_update) {
		dtmsgtype = DNS_DTTYPE_UR;
	} else if ((client->message->flags & DNS_MESSAGEFLAG_RD) != 0) {
		dtmsgtype = DNS_DTTYPE_CR;
	} else {
		dtmsgtype = DNS_DTTYPE_AR;
	}
#endif /* HAVE_DNSTAP */

	if (cleanup_cctx) {
		dns_compress_invalidate(&cctx);
		cleanup_cctx = false;
	}

	if (client->sendcb != NULL) {
		client->sendcb(&buffer);
	} else if (TCP_CLIENT(client)) {
		isc_buffer_usedregion(&buffer, &r);
		isc_buffer_putuint16(&tcpbuffer, (uint16_t) r.length);
		isc_buffer_add(&tcpbuffer, r.length);
#ifdef HAVE_DNSTAP
		if (client->view != NULL) {
			dns_dt_send(client->view, dtmsgtype,
				    &client->peeraddr, &client->destsockaddr,
				    true, &zr, &client->requesttime, NULL,
				    &buffer);
		}
#endif /* HAVE_DNSTAP */

		/* don't count the 2-octet length header */
		respsize = isc_buffer_usedlength(&tcpbuffer) - 2;
		result = client_sendpkg(client, &tcpbuffer);

		switch (isc_sockaddr_pf(&client->peeraddr)) {
		case AF_INET:
			isc_stats_increment(client->sctx->tcpoutstats4,
					    ISC_MIN((int)respsize / 16, 256));
			break;
		case AF_INET6:
			isc_stats_increment(client->sctx->tcpoutstats6,
					    ISC_MIN((int)respsize / 16, 256));
			break;
		default:
			INSIST(0);
			ISC_UNREACHABLE();
		}
	} else {
#ifdef HAVE_DNSTAP
		/*
		 * Log dnstap data first, because client_sendpkg() may
		 * leave client->view set to NULL.
		 */
		if (client->view != NULL) {
			dns_dt_send(client->view, dtmsgtype,
				    &client->peeraddr,
				    &client->destsockaddr,
				    false, &zr,
				    &client->requesttime, NULL, &buffer);
		}
#endif /* HAVE_DNSTAP */

		respsize = isc_buffer_usedlength(&buffer);
		result = client_sendpkg(client, &buffer);

		switch (isc_sockaddr_pf(&client->peeraddr)) {
		case AF_INET:
			isc_stats_increment(client->sctx->udpoutstats4,
					    ISC_MIN((int)respsize / 16, 256));
			break;
		case AF_INET6:
			isc_stats_increment(client->sctx->udpoutstats6,
					    ISC_MIN((int)respsize / 16, 256));
			break;
		default:
			INSIST(0);
			ISC_UNREACHABLE();
		}
	}

	/* update statistics (XXXJT: is it okay to access message->xxxkey?) */
	ns_stats_increment(client->sctx->nsstats, ns_statscounter_response);

	dns_rcodestats_increment(client->sctx->rcodestats,
				 client->message->rcode);
	if (opt_included) {
		ns_stats_increment(client->sctx->nsstats,
				   ns_statscounter_edns0out);
	}
	if (client->message->tsigkey != NULL) {
		ns_stats_increment(client->sctx->nsstats,
				   ns_statscounter_tsigout);
	}
	if (client->message->sig0key != NULL) {
		ns_stats_increment(client->sctx->nsstats,
				   ns_statscounter_sig0out);
	}
	if ((client->message->flags & DNS_MESSAGEFLAG_TC) != 0)
		ns_stats_increment(client->sctx->nsstats,
				   ns_statscounter_truncatedresp);

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
	 * Delay the response according to the -T delay option
	 */
	if (client->sctx->delay != 0) {
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
		if (client->sctx->delay >= 1000)
			isc_interval_set(&interval, client->sctx->delay / 1000,
				 (client->sctx->delay % 1000) * 1000000);
		else
			isc_interval_set(&interval, 0,
					 client->sctx->delay * 1000000);
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

	if (client->rcode_override == -1) {
		rcode = dns_result_torcode(result);
	} else {
		rcode = (dns_rcode_t)(client->rcode_override & 0xfff);
	}

#if NS_CLIENT_DROPPORT
	/*
	 * Don't send FORMERR to ports on the drop port list.
	 */
	if (rcode == dns_rcode_formerr &&
	    ns_client_dropport(isc_sockaddr_getport(&client->peeraddr)) !=
	    DROPPORT_NO)
	{
		char buf[64];
		isc_buffer_t b;

		isc_buffer_init(&b, buf, sizeof(buf) - 1);
		if (dns_rcode_totext(rcode, &b) != ISC_R_SUCCESS) {
			isc_buffer_putstr(&b, "UNKNOWN RCODE");
		}
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
		bool wouldlog;
		char log_buf[DNS_RRL_LOG_BUF_LEN];
		dns_rrl_result_t rrl_result;
		int loglevel;

		INSIST(rcode != dns_rcode_noerror &&
		       rcode != dns_rcode_nxdomain);
		if ((client->sctx->options & NS_SERVER_LOGQUERIES) != 0) {
			loglevel = DNS_RRL_LOG_DROP;
		} else {
			loglevel = ISC_LOG_DEBUG(1);
		}
		wouldlog = isc_log_wouldlog(ns_lctx, loglevel);
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
					      NS_LOGCATEGORY_QUERY_ERRORS,
					      NS_LOGMODULE_CLIENT,
					      loglevel,
					      "%s", log_buf);
			}
			/*
			 * Some error responses cannot be 'slipped',
			 * so don't try to slip any error responses.
			 */
			if (!client->view->rrl->log_only) {
				ns_stats_increment(client->sctx->nsstats,
						   ns_statscounter_ratedropped);
				ns_stats_increment(client->sctx->nsstats,
						   ns_statscounter_dropped);
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
	result = dns_message_reply(message, true);
	if (result != ISC_R_SUCCESS) {
		/*
		 * It could be that we've got a query with a good header,
		 * but a bad question section, so we try again with
		 * want_question_section set to false.
		 */
		result = dns_message_reply(message, false);
		if (result != ISC_R_SUCCESS) {
			ns_client_next(client, result);
			return;
		}
	}
	message->rcode = rcode;

	if (rcode == dns_rcode_formerr) {
		/*
		 * FORMERR loop avoidance:  If we sent a FORMERR message
		 * with the same ID to the same client less than two
		 * seconds ago, assume that we are in an infinite error
		 * packet dialog with a server for some protocol whose
		 * error responses look enough like DNS queries to
		 * elicit a FORMERR response.  Drop a packet to break
		 * the loop.
		 */
		if (isc_sockaddr_equal(&client->peeraddr,
				       &client->formerrcache.addr) &&
		    message->id == client->formerrcache.id &&
		    (isc_time_seconds(&client->requesttime) -
		     client->formerrcache.time) < 2)
		{
			/* Drop packet. */
			ns_client_log(client, NS_LOGCATEGORY_CLIENT,
				      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(1),
				      "possible error packet loop, "
				      "FORMERR dropped");
			ns_client_next(client, result);
			return;
		}
		client->formerrcache.addr = client->peeraddr;
		client->formerrcache.time =
			isc_time_seconds(&client->requesttime);
		client->formerrcache.id = message->id;
	} else if (rcode == dns_rcode_servfail && client->query.qname != NULL &&
		   client->view != NULL && client->view->fail_ttl != 0 &&
		   ((client->attributes & NS_CLIENTATTR_NOSETFC) == 0))
	{
		/*
		 * SERVFAIL caching: store qname/qtype of failed queries
		 */
		isc_time_t expire;
		isc_interval_t i;
		uint32_t flags = 0;

		if ((message->flags & DNS_MESSAGEFLAG_CD) != 0)
			flags = NS_FAILCACHE_CD;

		isc_interval_set(&i, client->view->fail_ttl, 0);
		result = isc_time_nowplusinterval(&expire, &i);
		if (result == ISC_R_SUCCESS) {
			dns_badcache_add(client->view->failcache,
					 client->query.qname,
					 client->query.qtype,
					 true, flags, &expire);
		}
	}

	ns_client_send(client);
}

isc_result_t
ns_client_addopt(ns_client_t *client, dns_message_t *message,
		 dns_rdataset_t **opt)
{
	unsigned char ecs[ECS_SIZE];
	char nsid[BUFSIZ], *nsidp;
	unsigned char cookie[COOKIE_SIZE];
	isc_result_t result;
	dns_view_t *view;
	dns_resolver_t *resolver;
	uint16_t udpsize;
	dns_ednsopt_t ednsopts[DNS_EDNSOPTIONS];
	int count = 0;
	unsigned int flags;
	unsigned char expire[4];
	unsigned char advtimo[2];
	dns_aclenv_t *env = ns_interfacemgr_getaclenv(client->interface->mgr);

	REQUIRE(NS_CLIENT_VALID(client));
	REQUIRE(opt != NULL && *opt == NULL);
	REQUIRE(message != NULL);

	view = client->view;
	resolver = (view != NULL) ? view->resolver : NULL;
	if (resolver != NULL)
		udpsize = dns_resolver_getudpsize(resolver);
	else
		udpsize = client->sctx->udpsize;

	flags = client->extflags & DNS_MESSAGEEXTFLAG_REPLYPRESERVE;

	/* Set EDNS options if applicable */
	if (WANTNSID(client)) {
		if (client->sctx->server_id != NULL) {
			nsidp = client->sctx->server_id;
		} else if (client->sctx->gethostname != NULL) {
			result = client->sctx->gethostname(nsid, sizeof(nsid));
			if (result != ISC_R_SUCCESS) {
				goto no_nsid;
			}
			nsidp = nsid;
		} else {
			goto no_nsid;
		}

		INSIST(count < DNS_EDNSOPTIONS);
		ednsopts[count].code = DNS_OPT_NSID;
		ednsopts[count].length = (uint16_t)strlen(nsidp);
		ednsopts[count].value = (unsigned char *)nsidp;
		count++;
	}
 no_nsid:
	if ((client->attributes & NS_CLIENTATTR_WANTCOOKIE) != 0) {
		isc_buffer_t buf;
		isc_stdtime_t now;
		uint32_t nonce;

		isc_buffer_init(&buf, cookie, sizeof(cookie));
		isc_stdtime_get(&now);

		isc_random_buf(&nonce, sizeof(nonce));

		compute_cookie(client, now, nonce, client->sctx->secret, &buf);

		INSIST(count < DNS_EDNSOPTIONS);
		ednsopts[count].code = DNS_OPT_COOKIE;
		ednsopts[count].length = COOKIE_SIZE;
		ednsopts[count].value = cookie;
		count++;
	}
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
	if (((client->attributes & NS_CLIENTATTR_HAVEECS) != 0) &&
	    (client->ecs.addr.family == AF_INET ||
	     client->ecs.addr.family == AF_INET6 ||
	     client->ecs.addr.family == AF_UNSPEC))
	{
		isc_buffer_t buf;
		uint8_t addr[16];
		uint32_t plen, addrl;
		uint16_t family = 0;

		/* Add CLIENT-SUBNET option. */

		plen = client->ecs.source;

		/* Round up prefix len to a multiple of 8 */
		addrl = (plen + 7) / 8;

		switch (client->ecs.addr.family) {
		case AF_UNSPEC:
			INSIST(plen == 0);
			family = 0;
			break;
		case AF_INET:
			INSIST(plen <= 32);
			family = 1;
			memmove(addr, &client->ecs.addr.type, addrl);
			break;
		case AF_INET6:
			INSIST(plen <= 128);
			family = 2;
			memmove(addr, &client->ecs.addr.type, addrl);
			break;
		default:
			INSIST(0);
			ISC_UNREACHABLE();
		}

		isc_buffer_init(&buf, ecs, sizeof(ecs));
		/* family */
		isc_buffer_putuint16(&buf, family);
		/* source prefix-length */
		isc_buffer_putuint8(&buf, client->ecs.source);
		/* scope prefix-length */
		isc_buffer_putuint8(&buf, client->ecs.scope);

		/* address */
		if (addrl > 0) {
			/* Mask off last address byte */
			if ((plen % 8) != 0)
				addr[addrl - 1] &=
					~0U << (8 - (plen % 8));
			isc_buffer_putmem(&buf, addr,
					  (unsigned) addrl);
		}

		ednsopts[count].code = DNS_OPT_CLIENT_SUBNET;
		ednsopts[count].length = addrl + 4;
		ednsopts[count].value = ecs;
		count++;
	}
	if (TCP_CLIENT(client) && USEKEEPALIVE(client)) {
		isc_buffer_t buf;

		INSIST(count < DNS_EDNSOPTIONS);

		isc_buffer_init(&buf, advtimo, sizeof(advtimo));
		isc_buffer_putuint16(&buf,
			     (uint16_t) client->sctx->advertisedtimo);
		ednsopts[count].code = DNS_OPT_TCP_KEEPALIVE;
		ednsopts[count].length = 2;
		ednsopts[count].value = advtimo;
		count++;
	}

	/* Padding must be added last */
	if ((view != NULL) && (view->padding > 0) && WANTPAD(client) &&
	    (TCP_CLIENT(client) ||
	     ((client->attributes & NS_CLIENTATTR_HAVECOOKIE) != 0)))
	{
		isc_netaddr_t netaddr;
		int match;

		isc_netaddr_fromsockaddr(&netaddr, &client->peeraddr);
		result = dns_acl_match(&netaddr, NULL, view->pad_acl,
				       env, &match, NULL);
		if (result == ISC_R_SUCCESS && match > 0) {
			INSIST(count < DNS_EDNSOPTIONS);

			ednsopts[count].code = DNS_OPT_PAD;
			ednsopts[count].length = 0;
			ednsopts[count].value = NULL;
			count++;

			dns_message_setpadding(message, view->padding);
		}
	}

	result = dns_message_buildopt(message, opt, 0, udpsize, flags,
				      ednsopts, count);
	return (result);
}

static void
compute_cookie(ns_client_t *client, uint32_t when, uint32_t nonce,
	       const unsigned char *secret, isc_buffer_t *buf)
{
	unsigned char digest[ISC_MAX_MD_SIZE] ISC_NONSTRING = { 0 };
	STATIC_ASSERT(ISC_MAX_MD_SIZE >= ISC_SIPHASH24_TAG_LENGTH,
		      "You need to increase the digest buffer.");
	STATIC_ASSERT(ISC_MAX_MD_SIZE >= ISC_AES_BLOCK_LENGTH,
		      "You need to increase the digest buffer.");

	switch (client->sctx->cookiealg) {
	case ns_cookiealg_siphash24: {
		unsigned char input[16 + 16] ISC_NONSTRING = { 0 };
		size_t inputlen = 0;
		isc_netaddr_t netaddr;
		unsigned char *cp;

		cp = isc_buffer_used(buf);
		isc_buffer_putmem(buf, client->cookie, 8);
		isc_buffer_putuint8(buf, NS_COOKIE_VERSION_1);
		isc_buffer_putuint24(buf, 0); /* Reserved */
		isc_buffer_putuint32(buf, when);

		memmove(input, cp, 16);

		isc_netaddr_fromsockaddr(&netaddr, &client->peeraddr);
		switch (netaddr.family) {
		case AF_INET:
			cp = (unsigned char *)&netaddr.type.in;
			memmove(input + 16, cp, 4);
			inputlen = 20;
			break;
		case AF_INET6:
			cp = (unsigned char *)&netaddr.type.in6;
			memmove(input + 16, cp, 16);
			inputlen = 32;
			break;
		default:
			INSIST(0);
			ISC_UNREACHABLE();
		}

		isc_siphash24(secret, input, inputlen, digest);
		isc_buffer_putmem(buf, digest, 8);
		break;
	}
	case ns_cookiealg_aes: {
		unsigned char input[4 + 4 + 16] ISC_NONSTRING = { 0 };
		isc_netaddr_t netaddr;
		unsigned char *cp;
		unsigned int i;

		cp = isc_buffer_used(buf);
		isc_buffer_putmem(buf, client->cookie, 8);
		isc_buffer_putuint32(buf, nonce);
		isc_buffer_putuint32(buf, when);
		memmove(input, cp, 16);
		isc_aes128_crypt(secret, input, digest);
		for (i = 0; i < 8; i++) {
			input[i] = digest[i] ^ digest[i + 8];
		}
		isc_netaddr_fromsockaddr(&netaddr, &client->peeraddr);
		switch (netaddr.family) {
		case AF_INET:
			cp = (unsigned char *)&netaddr.type.in;
			memmove(input + 8, cp, 4);
			memset(input + 12, 0, 4);
			isc_aes128_crypt(secret, input, digest);
			break;
		case AF_INET6:
			cp = (unsigned char *)&netaddr.type.in6;
			memmove(input + 8, cp, 16);
			isc_aes128_crypt(secret, input, digest);
			for (i = 0; i < 8; i++) {
				input[i + 8] = digest[i] ^ digest[i + 8];
			}
			isc_aes128_crypt(client->sctx->secret, input + 8,
					 digest);
			break;
		default:
			INSIST(0);
			ISC_UNREACHABLE();
		}
		for (i = 0; i < 8; i++) {
			digest[i] ^= digest[i + 8];
		}
		isc_buffer_putmem(buf, digest, 8);
		break;
	}

	case ns_cookiealg_sha1:
	case ns_cookiealg_sha256: {
		unsigned char input[8 + 4 + 4 + 16];
		isc_netaddr_t netaddr;
		unsigned char *cp;
		unsigned int length = 0;
		isc_md_type_t md_type =
			(client->sctx->cookiealg == ns_cookiealg_sha1)
			? ISC_MD_SHA1
			: ISC_MD_SHA256;
		unsigned int secret_len = isc_md_type_get_size(md_type);

		cp = isc_buffer_used(buf);
		isc_buffer_putmem(buf, client->cookie, 8);
		isc_buffer_putuint32(buf, nonce);
		isc_buffer_putuint32(buf, when);
		memmove(input, cp, 16);

		isc_netaddr_fromsockaddr(&netaddr, &client->peeraddr);
		switch (netaddr.family) {
		case AF_INET:
			memmove(input + 16,
				(unsigned char *)&netaddr.type.in, 4);
			length = 16 + 4;
			break;
		case AF_INET6:
			memmove(input + 16,
				(unsigned char *)&netaddr.type.in6, 16);
			length = 16 + 16;
			break;
		default:
			INSIST(0);
			ISC_UNREACHABLE();
		}

		/*
		 * XXXOND: Feels wrong to assert on cookie calculation failure
		 */
		RUNTIME_CHECK(isc_hmac(md_type, secret, secret_len,
				       input, length,
				       digest, NULL) == ISC_R_SUCCESS);

		isc_buffer_putmem(buf, digest, 8);
		break;
	}

	default:
		INSIST(0);
		ISC_UNREACHABLE();
	}
}

static void
process_cookie(ns_client_t *client, isc_buffer_t *buf, size_t optlen) {
	ns_altsecret_t *altsecret;
	unsigned char dbuf[COOKIE_SIZE];
	unsigned char *old;
	isc_stdtime_t now;
	uint32_t when;
	uint32_t nonce;
	isc_buffer_t db;

	/*
	 * If we have already seen a cookie option skip this cookie option.
	 */
	if ((!client->sctx->answercookie) ||
	    (client->attributes & NS_CLIENTATTR_WANTCOOKIE) != 0)
	{
		isc_buffer_forward(buf, (unsigned int)optlen);
		return;
	}

	client->attributes |= NS_CLIENTATTR_WANTCOOKIE;

	ns_stats_increment(client->sctx->nsstats, ns_statscounter_cookiein);

	if (optlen != COOKIE_SIZE) {
		/*
		 * Not our token.
		 */
		INSIST(optlen >= 8U);
		memmove(client->cookie, isc_buffer_current(buf), 8);
		isc_buffer_forward(buf, (unsigned int)optlen);

		if (optlen == 8U)
			ns_stats_increment(client->sctx->nsstats,
					   ns_statscounter_cookienew);
		else
			ns_stats_increment(client->sctx->nsstats,
					   ns_statscounter_cookiebadsize);
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
	 * Only accept COOKIE if we have talked to the client in the last hour.
	 */
	isc_stdtime_get(&now);
	if (isc_serial_gt(when, (now + 300)) ||		/* In the future. */
	    isc_serial_lt(when, (now - 3600))) {	/* In the past. */
		ns_stats_increment(client->sctx->nsstats,
				   ns_statscounter_cookiebadtime);
		return;
	}

	isc_buffer_init(&db, dbuf, sizeof(dbuf));
	compute_cookie(client, when, nonce, client->sctx->secret, &db);

	if (isc_safe_memequal(old, dbuf, COOKIE_SIZE)) {
		ns_stats_increment(client->sctx->nsstats,
				   ns_statscounter_cookiematch);
		client->attributes |= NS_CLIENTATTR_HAVECOOKIE;
		return;
	}

	for (altsecret = ISC_LIST_HEAD(client->sctx->altsecrets);
	     altsecret != NULL;
	     altsecret = ISC_LIST_NEXT(altsecret, link))
	{
		isc_buffer_init(&db, dbuf, sizeof(dbuf));
		compute_cookie(client, when, nonce, altsecret->secret, &db);
		if (isc_safe_memequal(old, dbuf, COOKIE_SIZE)) {
			ns_stats_increment(client->sctx->nsstats,
					   ns_statscounter_cookiematch);
			client->attributes |= NS_CLIENTATTR_HAVECOOKIE;
			return;
		}
	}

	ns_stats_increment(client->sctx->nsstats,
			   ns_statscounter_cookienomatch);
}

static isc_result_t
process_ecs(ns_client_t *client, isc_buffer_t *buf, size_t optlen) {
	uint16_t family;
	uint8_t addrlen, addrbytes, scope, *paddr;
	isc_netaddr_t caddr;

	/*
	 * If we have already seen a ECS option skip this ECS option.
	 */
	if ((client->attributes & NS_CLIENTATTR_HAVEECS) != 0) {
		isc_buffer_forward(buf, (unsigned int)optlen);
		return (ISC_R_SUCCESS);
	}

	/*
	 * XXXMUKS: Is there any need to repeat these checks here
	 * (except query's scope length) when they are done in the OPT
	 * RDATA fromwire code?
	 */

	if (optlen < 4U) {
		ns_client_log(client, NS_LOGCATEGORY_CLIENT,
			      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(2),
			      "EDNS client-subnet option too short");
		return (DNS_R_FORMERR);
	}

	family = isc_buffer_getuint16(buf);
	addrlen = isc_buffer_getuint8(buf);
	scope = isc_buffer_getuint8(buf);
	optlen -= 4;

	if (scope != 0U) {
		ns_client_log(client, NS_LOGCATEGORY_CLIENT,
			      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(2),
			      "EDNS client-subnet option: invalid scope");
		return (DNS_R_OPTERR);
	}

	memset(&caddr, 0, sizeof(caddr));
	switch (family) {
	case 0:
		/*
		 * XXXMUKS: In queries, if FAMILY is set to 0, SOURCE
		 * PREFIX-LENGTH must be 0 and ADDRESS should not be
		 * present as the address and prefix lengths don't make
		 * sense because the family is unknown.
		 */
		if (addrlen != 0U) {
			ns_client_log(client, NS_LOGCATEGORY_CLIENT,
				      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(2),
				      "EDNS client-subnet option: invalid "
				      "address length (%u) for FAMILY=0",
				      addrlen);
			return (DNS_R_OPTERR);
		}
		caddr.family = AF_UNSPEC;
		break;
	case 1:
		if (addrlen > 32U) {
			ns_client_log(client, NS_LOGCATEGORY_CLIENT,
				      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(2),
				      "EDNS client-subnet option: invalid "
				      "address length (%u) for IPv4",
				      addrlen);
			return (DNS_R_OPTERR);
		}
		caddr.family = AF_INET;
		break;
	case 2:
		if (addrlen > 128U) {
			ns_client_log(client, NS_LOGCATEGORY_CLIENT,
				      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(2),
				      "EDNS client-subnet option: invalid "
				      "address length (%u) for IPv6",
				      addrlen);
			return (DNS_R_OPTERR);
		}
		caddr.family = AF_INET6;
		break;
	default:
		ns_client_log(client, NS_LOGCATEGORY_CLIENT,
			      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(2),
			      "EDNS client-subnet option: invalid family");
		return (DNS_R_OPTERR);
	}

	addrbytes = (addrlen + 7) / 8;
	if (isc_buffer_remaininglength(buf) < addrbytes) {
		ns_client_log(client, NS_LOGCATEGORY_CLIENT,
			      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(2),
			      "EDNS client-subnet option: address too short");
		return (DNS_R_OPTERR);
	}

	paddr = (uint8_t *) &caddr.type;
	if (addrbytes != 0U) {
		memmove(paddr, isc_buffer_current(buf), addrbytes);
		isc_buffer_forward(buf, addrbytes);
		optlen -= addrbytes;

		if ((addrlen % 8) != 0) {
			uint8_t bits = ~0U << (8 - (addrlen % 8));
			bits &= paddr[addrbytes - 1];
			if (bits != paddr[addrbytes - 1])
				return (DNS_R_OPTERR);
		}
	}

	memmove(&client->ecs.addr, &caddr, sizeof(caddr));
	client->ecs.source = addrlen;
	client->ecs.scope = 0;
	client->attributes |= NS_CLIENTATTR_HAVEECS;

	isc_buffer_forward(buf, (unsigned int)optlen);
	return (ISC_R_SUCCESS);
}

static isc_result_t
process_keytag(ns_client_t *client, isc_buffer_t *buf, size_t optlen) {

	if (optlen == 0 || (optlen % 2) != 0) {
		isc_buffer_forward(buf, (unsigned int)optlen);
		return (DNS_R_OPTERR);
	}

	/* Silently drop additional keytag options. */
	if (client->keytag != NULL) {
		isc_buffer_forward(buf, (unsigned int)optlen);
		return (ISC_R_SUCCESS);
	}

	client->keytag = isc_mem_get(client->mctx, optlen);
	if (client->keytag != NULL) {
		client->keytag_len = (uint16_t)optlen;
		memmove(client->keytag, isc_buffer_current(buf), optlen);
	}
	isc_buffer_forward(buf, (unsigned int)optlen);
	return (ISC_R_SUCCESS);
}

static isc_result_t
process_opt(ns_client_t *client, dns_rdataset_t *opt) {
	dns_rdata_t rdata;
	isc_buffer_t optbuf;
	isc_result_t result;
	uint16_t optcode;
	uint16_t optlen;

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
	client->extflags = (uint16_t)(opt->ttl & 0xFFFF);

	/*
	 * Do we understand this version of EDNS?
	 *
	 * XXXRTH need library support for this!
	 */
	client->ednsversion = (opt->ttl & 0x00FF0000) >> 16;
	if (client->ednsversion > DNS_EDNS_VERSION) {
		ns_stats_increment(client->sctx->nsstats,
				   ns_statscounter_badednsver);
		result = ns_client_addopt(client, client->message,
					  &client->opt);
		if (result == ISC_R_SUCCESS)
			result = DNS_R_BADVERS;
		ns_client_error(client, result);
		return (result);
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
				if (!WANTNSID(client))
					ns_stats_increment(
						   client->sctx->nsstats,
						   ns_statscounter_nsidopt);
				client->attributes |= NS_CLIENTATTR_WANTNSID;
				isc_buffer_forward(&optbuf, optlen);
				break;
			case DNS_OPT_COOKIE:
				process_cookie(client, &optbuf, optlen);
				break;
			case DNS_OPT_EXPIRE:
				if (!WANTEXPIRE(client))
					ns_stats_increment(
						   client->sctx->nsstats,
						   ns_statscounter_expireopt);
				client->attributes |= NS_CLIENTATTR_WANTEXPIRE;
				isc_buffer_forward(&optbuf, optlen);
				break;
			case DNS_OPT_CLIENT_SUBNET:
				result = process_ecs(client, &optbuf, optlen);
				if (result != ISC_R_SUCCESS) {
					ns_client_error(client, result);
					return (result);
				}
				ns_stats_increment(client->sctx->nsstats,
						   ns_statscounter_ecsopt);
				break;
			case DNS_OPT_TCP_KEEPALIVE:
				if (!USEKEEPALIVE(client))
					ns_stats_increment(
						   client->sctx->nsstats,
						   ns_statscounter_keepaliveopt);
				client->attributes |=
					NS_CLIENTATTR_USEKEEPALIVE;
				isc_buffer_forward(&optbuf, optlen);
				break;
			case DNS_OPT_PAD:
				client->attributes |= NS_CLIENTATTR_WANTPAD;
				ns_stats_increment(client->sctx->nsstats,
						   ns_statscounter_padopt);
				isc_buffer_forward(&optbuf, optlen);
				break;
			case DNS_OPT_KEY_TAG:
				result = process_keytag(client, &optbuf,
							optlen);
				if (result != ISC_R_SUCCESS) {
					ns_client_error(client, result);
					return (result);
				}
				ns_stats_increment(client->sctx->nsstats,
						   ns_statscounter_keytagopt);
				break;
			default:
				ns_stats_increment(client->sctx->nsstats,
						   ns_statscounter_otheropt);
				isc_buffer_forward(&optbuf, optlen);
				break;
			}
		}
	}

	ns_stats_increment(client->sctx->nsstats, ns_statscounter_edns0in);
	client->attributes |= NS_CLIENTATTR_WANTOPT;

	return (result);
}

/*
 * Handle an incoming request event from the socket (UDP case)
 * or tcpmsg (TCP case).
 */
void
ns__client_request(isc_task_t *task, isc_event_t *event) {
	ns_client_t *client;
	isc_socketevent_t *sevent;
	isc_result_t result;
	isc_result_t sigresult = ISC_R_SUCCESS;
	isc_buffer_t *buffer;
	isc_buffer_t tbuffer;
	dns_rdataset_t *opt;
	const dns_name_t *signame;
	bool ra;	/* Recursion available. */
	isc_netaddr_t netaddr;
	int match;
	dns_messageid_t id;
	unsigned int flags;
	bool notimp;
	size_t reqsize;
	dns_aclenv_t *env;
#ifdef HAVE_DNSTAP
	dns_dtmsgtype_t dtmsgtype;
#endif

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
			client->peeraddr_valid = true;
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
		INSIST(client->tcpconn != NULL);
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

	reqsize = isc_buffer_usedlength(buffer);
	/* don't count the length header */
	if (TCP_CLIENT(client))
		reqsize -= 2;

	if (exit_check(client)) {
		return;
	}
	client->state = client->newstate = NS_CLIENTSTATE_WORKING;

	isc_task_getcurrenttimex(task, &client->requesttime);
	client->tnow = client->requesttime;
	client->now = isc_time_seconds(&client->tnow);

	if (result != ISC_R_SUCCESS) {
		if (TCP_CLIENT(client)) {
			ns_client_next(client, result);
		} else {
			if  (result != ISC_R_CANCELED)
				isc_log_write(ns_lctx, NS_LOGCATEGORY_CLIENT,
					      NS_LOGMODULE_CLIENT,
					      ISC_LOG_ERROR,
					      "UDP client handler shutting "
					      "down due to fatal receive "
					      "error: %s",
					      isc_result_totext(result));
			isc_task_shutdown(client->task);
		}
		return;
	}

	isc_netaddr_fromsockaddr(&netaddr, &client->peeraddr);

#if NS_CLIENT_DROPPORT
	if (ns_client_dropport(isc_sockaddr_getport(&client->peeraddr)) ==
	    DROPPORT_REQUEST) {
		ns_client_log(client, DNS_LOGCATEGORY_SECURITY,
			      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(10),
			      "dropped request: suspicious port");
		ns_client_next(client, ISC_R_SUCCESS);
		return;
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
	env = ns_interfacemgr_getaclenv(client->interface->mgr);
	if (!TCP_CLIENT(client)) {
		if (client->sctx->blackholeacl != NULL &&
		    (dns_acl_match(&netaddr, NULL, client->sctx->blackholeacl,
				   env, &match, NULL) == ISC_R_SUCCESS) &&
		    match > 0)
		{
			ns_client_log(client, DNS_LOGCATEGORY_SECURITY,
				      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(10),
				      "blackholed UDP datagram");
			ns_client_next(client, ISC_R_SUCCESS);
			return;
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
		return;
	}

	result = dns_message_peekheader(buffer, &id, &flags);
	if (result != ISC_R_SUCCESS) {
		/*
		 * There isn't enough header to determine whether
		 * this was a request or a response.  Drop it.
		 */
		ns_client_next(client, result);
		return;
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
			return;
		} else {
			dns_dispatch_importrecv(client->dispatch, event);
			ns_client_next(client, ISC_R_SUCCESS);
			return;
		}
	}

	/*
	 * Update some statistics counters.  Don't count responses.
	 */
	if (isc_sockaddr_pf(&client->peeraddr) == PF_INET) {
		ns_stats_increment(client->sctx->nsstats,
				   ns_statscounter_requestv4);
	} else {
		ns_stats_increment(client->sctx->nsstats,
				   ns_statscounter_requestv6);
	}
	if (TCP_CLIENT(client)) {
		ns_stats_increment(client->sctx->nsstats,
				   ns_statscounter_requesttcp);
		switch (isc_sockaddr_pf(&client->peeraddr)) {
		case AF_INET:
			isc_stats_increment(client->sctx->tcpinstats4,
					    ISC_MIN((int)reqsize / 16, 18));
			break;
		case AF_INET6:
			isc_stats_increment(client->sctx->tcpinstats6,
					    ISC_MIN((int)reqsize / 16, 18));
			break;
		default:
			INSIST(0);
			ISC_UNREACHABLE();
		}
	} else {
		switch (isc_sockaddr_pf(&client->peeraddr)) {
		case AF_INET:
			isc_stats_increment(client->sctx->udpinstats4,
					    ISC_MIN((int)reqsize / 16, 18));
			break;
		case AF_INET6:
			isc_stats_increment(client->sctx->udpinstats6,
					    ISC_MIN((int)reqsize / 16, 18));
			break;
		default:
			INSIST(0);
			ISC_UNREACHABLE();
		}
	}

	/*
	 * It's a request.  Parse it.
	 */
	result = dns_message_parse(client->message, buffer, 0);
	if (result != ISC_R_SUCCESS) {
		/*
		 * Parsing the request failed.  Send a response
		 * (typically FORMERR or SERVFAIL).
		 */
		if (result == DNS_R_OPTERR) {
			(void)ns_client_addopt(client, client->message,
					       &client->opt);
		}

		ns_client_log(client, NS_LOGCATEGORY_CLIENT,
			      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(1),
			      "message parsing failed: %s",
			      isc_result_totext(result));
		if (result == ISC_R_NOSPACE) {
			result = DNS_R_FORMERR;
		}
		ns_client_error(client, result);
		return;
	}

	/*
	 * Pipeline TCP query processing.
	 */
	if (TCP_CLIENT(client) &&
	    client->message->opcode != dns_opcode_query)
	{
		client->tcpconn->pipelined = false;
	}
	if (TCP_CLIENT(client) && client->tcpconn->pipelined) {
		/*
		 * We're pipelining. Replace the client; the
		 * replacement can read the TCP socket looking
		 * for new messages and this one can process the
		 * current message asynchronously.
		 *
		 * There will now be at least three clients using this
		 * TCP socket - one accepting new connections,
		 * one reading an existing connection to get new
		 * messages, and one answering the message already
		 * received.
		 */
		result = ns_client_replace(client);
		if (result != ISC_R_SUCCESS) {
			client->tcpconn->pipelined = false;
		}
	}

	dns_opcodestats_increment(client->sctx->opcodestats,
				  client->message->opcode);
	switch (client->message->opcode) {
	case dns_opcode_query:
	case dns_opcode_update:
	case dns_opcode_notify:
		notimp = false;
		break;
	case dns_opcode_iquery:
	default:
		notimp = true;
		break;
	}

	client->message->rcode = dns_rcode_noerror;

	/* RFC1123 section 6.1.3.2 */
	if ((client->attributes & NS_CLIENTATTR_MULTICAST) != 0)
		client->message->flags &= ~DNS_MESSAGEFLAG_RD;

	/*
	 * Deal with EDNS.
	 */
	if ((client->sctx->options & NS_SERVER_NOEDNS) != 0)
		opt = NULL;
	else
		opt = dns_message_getopt(client->message);

	client->ecs.source = 0;
	client->ecs.scope = 0;

	if (opt != NULL) {
		/*
		 * Are returning FORMERR to all EDNS queries?
		 * Simulate a STD13 compliant server.
		 */
		if ((client->sctx->options & NS_SERVER_EDNSFORMERR) != 0) {
			ns_client_error(client, DNS_R_FORMERR);
			return;
		}

		/*
		 * Are returning NOTIMP to all EDNS queries?
		 */
		if ((client->sctx->options & NS_SERVER_EDNSNOTIMP) != 0) {
			ns_client_error(client, DNS_R_NOTIMP);
			return;
		}

		/*
		 * Are returning REFUSED to all EDNS queries?
		 */
		if ((client->sctx->options & NS_SERVER_EDNSREFUSED) != 0) {
			ns_client_error(client, DNS_R_REFUSED);
			return;
		}

		/*
		 * Are we dropping all EDNS queries?
		 */
		if ((client->sctx->options & NS_SERVER_DROPEDNS) != 0) {
			ns_client_next(client, ISC_R_SUCCESS);
			return;
		}

		result = process_opt(client, opt);
		if (result != ISC_R_SUCCESS)
			return;
	}

	if (client->message->rdclass == 0) {
		if ((client->attributes & NS_CLIENTATTR_WANTCOOKIE) != 0 &&
		    client->message->opcode == dns_opcode_query &&
		    client->message->counts[DNS_SECTION_QUESTION] == 0U)
		{
			result = dns_message_reply(client->message, true);
			if (result != ISC_R_SUCCESS) {
				ns_client_error(client, result);
				return;
			}
			if (notimp)
				client->message->rcode = dns_rcode_notimp;
			ns_client_send(client);
			return;
		}
		ns_client_log(client, NS_LOGCATEGORY_CLIENT,
			      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(1),
			      "message class could not be determined");
		ns_client_dumpmessage(client,
				      "message class could not be determined");
		ns_client_error(client, notimp ? DNS_R_NOTIMP : DNS_R_FORMERR);
		return;
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
			return;
		}
	}

	isc_sockaddr_fromnetaddr(&client->destsockaddr, &client->destaddr, 0);

	result = client->sctx->matchingview(&netaddr, &client->destaddr,
					    client->message, env,
					    &sigresult, &client->view);
	if (result != ISC_R_SUCCESS) {
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
		return;
	}

	ns_client_log(client, NS_LOGCATEGORY_CLIENT,
		      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(5),
		      "using view '%s'", client->view->name);

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
			ns_stats_increment(client->sctx->nsstats,
					   ns_statscounter_tsigin);
		} else {
			ns_stats_increment(client->sctx->nsstats,
					   ns_statscounter_sig0in);
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
		ns_stats_increment(client->sctx->nsstats,
				   ns_statscounter_invalidsig);
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
			return;
		}
	}

	/*
	 * Decide whether recursive service is available to this client.
	 * We do this here rather than in the query code so that we can
	 * set the RA bit correctly on all kinds of responses, not just
	 * responses to ordinary queries.  Note if you can't query the
	 * cache there is no point in setting RA.
	 */
	ra = false;
	if (client->view->resolver != NULL &&
	    client->view->recursion == true &&
	    ns_client_checkaclsilent(client, NULL,
				     client->view->recursionacl,
				     true) == ISC_R_SUCCESS &&
	    ns_client_checkaclsilent(client, NULL,
				     client->view->cacheacl,
				     true) == ISC_R_SUCCESS &&
	    ns_client_checkaclsilent(client, &client->destaddr,
				     client->view->recursiononacl,
				     true) == ISC_R_SUCCESS &&
	    ns_client_checkaclsilent(client, &client->destaddr,
				     client->view->cacheonacl,
				     true) == ISC_R_SUCCESS)
		ra = true;

	if (ra == true) {
		client->attributes |= NS_CLIENTATTR_RA;
	}

	ns_client_log(client, DNS_LOGCATEGORY_SECURITY, NS_LOGMODULE_CLIENT,
		      ISC_LOG_DEBUG(3), ra ? "recursion available" :
					     "recursion not available");

	/*
	 * Adjust maximum UDP response size for this client.
	 */
	if (client->udpsize > 512) {
		dns_peer_t *peer = NULL;
		uint16_t udpsize = client->view->maxudp;
		(void) dns_peerlist_peerbyaddr(client->view->peers,
					       &netaddr, &peer);
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
#ifdef HAVE_DNSTAP
		if (ra && (client->message->flags & DNS_MESSAGEFLAG_RD) != 0) {
			dtmsgtype = DNS_DTTYPE_CQ;
		} else {
			dtmsgtype = DNS_DTTYPE_AQ;
		}

		dns_dt_send(client->view, dtmsgtype, &client->peeraddr,
			    &client->destsockaddr, TCP_CLIENT(client), NULL,
			    &client->requesttime, NULL, buffer);
#endif /* HAVE_DNSTAP */

		ns_query_start(client);
		break;
	case dns_opcode_update:
		CTRACE("update");
#ifdef HAVE_DNSTAP
		dns_dt_send(client->view, DNS_DTTYPE_UQ, &client->peeraddr,
			    &client->destsockaddr, TCP_CLIENT(client), NULL,
			    &client->requesttime, NULL, buffer);
#endif /* HAVE_DNSTAP */
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
	if ((manager->sctx->options & NS_SERVER_CLIENTTEST) != 0) {
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

	client->sctx = NULL;
	ns_server_attach(manager->sctx, &client->sctx);

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
	client->timerset = false;

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
						   ns__client_request, client);
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
	client->tcpmsg_valid = false;
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
	client->mortal = false;
	client->sendcb = NULL;
	client->tcpconn = NULL;
	client->recursionquota = NULL;
	client->interface = NULL;
	client->peeraddr_valid = false;
	dns_ecs_init(&client->ecs);
	client->needshutdown = ((client->sctx->options &
				 NS_SERVER_CLIENTTEST) != 0);
	client->tcpactive = false;

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
	client->keytag = NULL;
	client->keytag_len = 0;
	client->rcode_override = -1; 	/* not set */

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
	if (client->sctx != NULL)
		ns_server_detach(&client->sctx);
	isc_mem_putanddetach(&client->mctx, client, sizeof(*client));

	return (result);
}

static void
client_read(ns_client_t *client, bool newconn) {
	isc_result_t result;

	CTRACE("read");

	result = dns_tcpmsg_readmessage(&client->tcpmsg, client->task,
					ns__client_request, client);
	if (result != ISC_R_SUCCESS)
		goto fail;

	/*
	 * Set a timeout to limit the amount of time we will wait
	 * for a request on this TCP connection.
	 */
	read_settimeout(client, newconn);

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
	isc_result_t result;
	ns_client_t *client = event->ev_arg;
	isc_socket_newconnev_t *nevent = (isc_socket_newconnev_t *)event;
	dns_aclenv_t *env = ns_interfacemgr_getaclenv(client->interface->mgr);
	uint32_t old;

	REQUIRE(event->ev_type == ISC_SOCKEVENT_NEWCONN);
	REQUIRE(NS_CLIENT_VALID(client));
	REQUIRE(client->task == task);

	UNUSED(task);

	INSIST(client->state == NS_CLIENTSTATE_READY);

	/*
	 * The accept() was successful and we're now establishing a new
	 * connection. We need to make note of it in the client and
	 * interface objects so client objects can do the right thing
	 * when going inactive in exit_check() (see comments in
	 * client_accept() for details).
	 */
	INSIST(client->naccepts == 1);
	client->naccepts--;

	old = isc_refcount_decrement(&client->interface->ntcpaccepting);
	INSIST(old > 0);

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
		client->peeraddr_valid = true;
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
		tcpconn_detach(client);
	}

	if (exit_check(client))
		goto freeevent;

	if (nevent->result == ISC_R_SUCCESS) {
		int match;
		isc_netaddr_t netaddr;

		isc_netaddr_fromsockaddr(&netaddr, &client->peeraddr);

		if (client->sctx->blackholeacl != NULL &&
		    (dns_acl_match(&netaddr, NULL, client->sctx->blackholeacl,
				   env, &match, NULL) == ISC_R_SUCCESS) &&
		    match > 0)
		{
			ns_client_log(client, DNS_LOGCATEGORY_SECURITY,
				      NS_LOGMODULE_CLIENT, ISC_LOG_DEBUG(10),
				      "blackholed connection attempt");
			client->newstate = NS_CLIENTSTATE_READY;
			(void)exit_check(client);
			goto freeevent;
		}

		INSIST(client->tcpmsg_valid == false);
		dns_tcpmsg_init(client->mctx, client->tcpsocket,
				&client->tcpmsg);
		client->tcpmsg_valid = true;

		/*
		 * Let a new client take our place immediately, before
		 * we wait for a request packet.  If we don't,
		 * telnetting to port 53 (once per CPU) will
		 * deny service to legitimate TCP clients.
		 */
		result = ns_client_replace(client);
		if (result == ISC_R_SUCCESS &&
		    (client->sctx->keepresporder == NULL ||
		     !dns_acl_allowed(&netaddr, NULL,
				     client->sctx->keepresporder, env)))
		{
			client->tcpconn->pipelined = true;
		}

		client_read(client, true);
	}

 freeevent:
	isc_event_free(&event);
}

static void
client_accept(ns_client_t *client) {
	isc_result_t result;

	CTRACE("accept");

	/*
	 * Set up a new TCP connection. This means try to attach to the
	 * TCP client quota (tcp-clients), but fail if we're over quota.
	 */
	result = tcpconn_init(client, false);
	if (result != ISC_R_SUCCESS) {
		bool exit;

		ns_client_log(client, NS_LOGCATEGORY_CLIENT,
			      NS_LOGMODULE_CLIENT, ISC_LOG_WARNING,
			      "TCP client quota reached: %s",
			      isc_result_totext(result));

		/*
		 * We have exceeded the system-wide TCP client quota.  But,
		 * we can't just block this accept in all cases, because if
		 * we did, a heavy TCP load on other interfaces might cause
		 * this interface to be starved, with no clients able to
		 * accept new connections.
		 *
		 * So, we check here to see if any other clients are
		 * already servicing TCP queries on this interface (whether
		 * accepting, reading, or processing). If we find that at
		 * least one client other than this one is active, then
		 * it's okay *not* to call accept - we can let this
		 * client go inactive and another will take over when it's
		 * done.
		 *
		 * If there aren't enough active clients on the interface,
		 * then we can be a little bit flexible about the quota.
		 * We'll allow *one* extra client through to ensure we're
		 * listening on every interface; we do this by setting the
		 * 'force' option to tcpconn_init().
		 *
		 * (Note: In practice this means that the real TCP client
		 * quota is tcp-clients plus the number of listening
		 * interfaces plus 1.)
		 */
		exit = (isc_refcount_current(&client->interface->ntcpactive) >
			(client->tcpactive ? 1U : 0U));
		if (exit) {
			client->newstate = NS_CLIENTSTATE_INACTIVE;
			(void)exit_check(client);
			return;
		}

		result = tcpconn_init(client, true);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
	}

	/*
	 * If this client was set up using get_client() or get_worker(),
	 * then TCP is already marked active. However, if it was restarted
	 * from exit_check(), it might not be, so we take care of it now.
	 */
	mark_tcp_active(client, true);

	result = isc_socket_accept(client->tcplistener, client->task,
				   client_newconn, client);
	if (result != ISC_R_SUCCESS) {
		/*
		 * XXXRTH  What should we do?  We're trying to accept but
		 *	   it didn't work.  If we just give up, then TCP
		 *	   service may eventually stop.
		 *
		 *	   For now, we just go idle.
		 */
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "isc_socket_accept() failed: %s",
				 isc_result_totext(result));

		tcpconn_detach(client);
		mark_tcp_active(client, false);
		return;
	}

	/*
	 * The client's 'naccepts' counter indicates that this client has
	 * called accept() and is waiting for a new connection. It should
	 * never exceed 1.
	 */
	INSIST(client->naccepts == 0);
	client->naccepts++;

	/*
	 * The interface's 'ntcpaccepting' counter is incremented when
	 * any client calls accept(), and decremented in client_newconn()
	 * once the connection is established.
	 *
	 * When the client object is shutting down after handling a TCP
	 * request (see exit_check()), if this value is at least one, that
	 * means another client has called accept() and is waiting to
	 * establish the next connection. That means the client may be
	 * be free to become inactive; otherwise it may need to start
	 * listening for connections itself to prevent the interface
	 * going dead.
	 */
	isc_refcount_increment0(&client->interface->ntcpaccepting);
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

bool
ns_client_shuttingdown(ns_client_t *client) {
	return (client->newstate == NS_CLIENTSTATE_FREED);
}

isc_result_t
ns_client_replace(ns_client_t *client) {
	isc_result_t result;
	bool tcp;

	CTRACE("replace");

	REQUIRE(client != NULL);
	REQUIRE(client->manager != NULL);

	tcp = TCP_CLIENT(client);
	if (tcp && client->tcpconn != NULL && client->tcpconn->pipelined) {
		result = get_worker(client->manager, client->interface,
				    client->tcpsocket, client);
	} else {
		result = get_client(client->manager, client->interface,
				    client->dispatch, tcp);

	}
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	/*
	 * The responsibility for listening for new requests is hereby
	 * transferred to the new client.  Therefore, the old client
	 * should refrain from listening for any more requests.
	 */
	client->mortal = true;

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

	isc_mutex_destroy(&manager->lock);
	isc_mutex_destroy(&manager->listlock);
	isc_mutex_destroy(&manager->reclock);

	if (manager->excl != NULL)
		isc_task_detach(&manager->excl);

	ns_server_detach(&manager->sctx);

	manager->magic = 0;
	isc_mem_put(manager->mctx, manager, sizeof(*manager));
}

isc_result_t
ns_clientmgr_create(isc_mem_t *mctx, ns_server_t *sctx, isc_taskmgr_t *taskmgr,
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

	isc_mutex_init(&manager->lock);
	isc_mutex_init(&manager->listlock);
	isc_mutex_init(&manager->reclock);

	manager->excl = NULL;
	result = isc_taskmgr_excltask(taskmgr, &manager->excl);
	if (result != ISC_R_SUCCESS) {
		goto cleanup_reclock;
	}

	manager->mctx = mctx;
	manager->taskmgr = taskmgr;
	manager->timermgr = timermgr;
	manager->exiting = false;

	manager->sctx = NULL;
	ns_server_attach(sctx, &manager->sctx);

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

 cleanup_reclock:
	isc_mutex_destroy(&manager->reclock);
	isc_mutex_destroy(&manager->listlock);
	isc_mutex_destroy(&manager->lock);

	isc_mem_put(manager->mctx, manager, sizeof(*manager));

	return (result);
}

void
ns_clientmgr_destroy(ns_clientmgr_t **managerp) {
	isc_result_t result;
	ns_clientmgr_t *manager;
	ns_client_t *client;
	bool need_destroy = false, unlock = false;

	REQUIRE(managerp != NULL);
	manager = *managerp;
	REQUIRE(VALID_MANAGER(manager));

	MTRACE("destroy");

	/*
	 * Check for success because we may already be task-exclusive
	 * at this point.  Only if we succeed at obtaining an exclusive
	 * lock now will we need to relinquish it later.
	 */
	result = isc_task_beginexclusive(manager->excl);
	if (result == ISC_R_SUCCESS)
		unlock = true;

	manager->exiting = true;

	for (client = ISC_LIST_HEAD(manager->clients);
	     client != NULL;
	     client = ISC_LIST_NEXT(client, link))
		isc_task_shutdown(client->task);

	if (ISC_LIST_EMPTY(manager->clients))
		need_destroy = true;

	if (unlock)
		isc_task_endexclusive(manager->excl);

	if (need_destroy)
		clientmgr_destroy(manager);

	*managerp = NULL;
}

static isc_result_t
get_client(ns_clientmgr_t *manager, ns_interface_t *ifp,
	   dns_dispatch_t *disp, bool tcp)
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
	if ((manager->sctx->options & NS_SERVER_CLIENTTEST) == 0) {
		ISC_QUEUE_POP(manager->inactive, ilink, client);
	}

	if (client != NULL) {
		MTRACE("recycle");
	} else {
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
	client->sctx = manager->sctx;
	INSIST(client->recursionquota == NULL);

	client->dscp = ifp->dscp;
	client->rcode_override = -1;	/* not set */

	if (tcp) {
		mark_tcp_active(client, true);

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

	return (result);
}

static isc_result_t
get_worker(ns_clientmgr_t *manager, ns_interface_t *ifp, isc_socket_t *sock,
	   ns_client_t *oldclient)
{
	isc_result_t result = ISC_R_SUCCESS;
	isc_event_t *ev;
	ns_client_t *client;
	MTRACE("get worker");

	REQUIRE(manager != NULL);
	REQUIRE(oldclient != NULL);

	if (manager->exiting)
		return (ISC_R_SHUTTINGDOWN);

	/*
	 * Allocate a client.  First try to get a recycled one;
	 * if that fails, make a new one.
	 */
	client = NULL;
	if ((manager->sctx->options & NS_SERVER_CLIENTTEST) == 0)
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
	client->newstate = client->state = NS_CLIENTSTATE_WORKING;
	INSIST(client->recursionquota == NULL);
	client->sctx = manager->sctx;

	client->dscp = ifp->dscp;

	client->attributes |= NS_CLIENTATTR_TCP;
	client->mortal = true;
	client->sendcb = NULL;
	client->rcode_override = -1;	/* not set */

	tcpconn_attach(oldclient, client);
	mark_tcp_active(client, true);

	isc_socket_attach(ifp->tcpsocket, &client->tcplistener);
	isc_socket_attach(sock, &client->tcpsocket);
	isc_socket_setname(client->tcpsocket, "worker-tcp", NULL);
	(void)isc_socket_getpeername(client->tcpsocket, &client->peeraddr);
	client->peeraddr_valid = true;

	INSIST(client->tcpmsg_valid == false);
	dns_tcpmsg_init(client->mctx, client->tcpsocket, &client->tcpmsg);
	client->tcpmsg_valid = true;

	INSIST(client->nctls == 0);
	client->nctls++;
	ev = &client->ctlevent;
	isc_task_send(client->task, &ev);

	return (result);
}

isc_result_t
ns__clientmgr_getclient(ns_clientmgr_t *manager, ns_interface_t *ifp,
			bool tcp, ns_client_t **clientp)
{
	isc_result_t result = ISC_R_SUCCESS;
	ns_client_t *client;
	MTRACE("getclient");

	REQUIRE(VALID_MANAGER(manager));
	REQUIRE(clientp != NULL && *clientp == NULL);

	if (manager->exiting)
		return (ISC_R_SHUTTINGDOWN);

	client = NULL;
	ISC_QUEUE_POP(manager->inactive, ilink, client);
	if (client != NULL)
		MTRACE("getclient (recycle)");
	else {
		MTRACE("getclient (create)");

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
	client->references++;

	if (tcp) {
		client->attributes |= NS_CLIENTATTR_TCP;
	}

	*clientp = client;

	return (result);
}

isc_result_t
ns_clientmgr_createclients(ns_clientmgr_t *manager, unsigned int n,
			   ns_interface_t *ifp, bool tcp)
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

isc_sockaddr_t *
ns_client_getdestaddr(ns_client_t *client) {
	return (&client->destsockaddr);
}

isc_result_t
ns_client_checkaclsilent(ns_client_t *client, isc_netaddr_t *netaddr,
			 dns_acl_t *acl, bool default_allow)
{
	isc_result_t result;
	dns_aclenv_t *env = ns_interfacemgr_getaclenv(client->interface->mgr);
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
			       env, &match, NULL);
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
		   bool default_allow, int log_level)
{
	isc_result_t result;
	isc_netaddr_t netaddr;

	if (sockaddr != NULL)
		isc_netaddr_fromsockaddr(&netaddr, sockaddr);

	result = ns_client_checkaclsilent(client, sockaddr ? &netaddr : NULL,
					  acl, default_allow);

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
	char signerbuf[DNS_NAME_FORMATSIZE], qnamebuf[DNS_NAME_FORMATSIZE];
	char peerbuf[ISC_SOCKADDR_FORMATSIZE];
	const char *viewname = "";
	const char *sep1 = "", *sep2 = "", *sep3 = "", *sep4 = "";
	const char *signer = "", *qname = "";
	dns_name_t *q = NULL;

	REQUIRE(client != NULL);

	vsnprintf(msgbuf, sizeof(msgbuf), fmt, ap);

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

	if (client->peeraddr_valid) {
		isc_sockaddr_format(&client->peeraddr,
				    peerbuf, sizeof(peerbuf));
	} else {
		snprintf(peerbuf, sizeof(peerbuf), "(no-peer)");
	}

	isc_log_write(ns_lctx, category, module, level,
		      "client @%p %s%s%s%s%s%s%s%s: %s",
		      client, peerbuf, sep1, signer, sep2, qname, sep3,
		      sep4, viewname, msgbuf);
}

void
ns_client_log(ns_client_t *client, isc_logcategory_t *category,
	   isc_logmodule_t *module, int level, const char *fmt, ...)
{
	va_list ap;

	if (! isc_log_wouldlog(ns_lctx, level))
		return;

	va_start(ap, fmt);
	ns_client_logv(client, category, module, level, fmt, ap);
	va_end(ap);
}

void
ns_client_aclmsg(const char *msg, const dns_name_t *name, dns_rdatatype_t type,
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

	if (!isc_log_wouldlog(ns_lctx, ISC_LOG_DEBUG(1)))
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
			ns_client_log(client, NS_LOGCATEGORY_CLIENT,
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
			strlcpy(typebuf, "-", sizeof(typebuf));
			strlcpy(classbuf, "-", sizeof(classbuf));
		}
		UNLOCK(&client->query.fetchlock);
		fprintf(f, "; client %s%s%s: id %u '%s/%s/%s'%s%s "
			"requesttime %u\n", peerbuf, sep, name,
			client->message->id, namebuf, typebuf, classbuf,
			origfor, original,
			isc_time_seconds(&client->requesttime));
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
	client->query.attributes &= ~NS_QUERYATTR_REDIRECT;
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

dns_rdataset_t *
ns_client_newrdataset(ns_client_t *client) {
	dns_rdataset_t *rdataset;
	isc_result_t result;

	REQUIRE(NS_CLIENT_VALID(client));

	rdataset = NULL;
	result = dns_message_gettemprdataset(client->message, &rdataset);
	if (result != ISC_R_SUCCESS) {
		return (NULL);
	}

	return (rdataset);
}

void
ns_client_putrdataset(ns_client_t *client, dns_rdataset_t **rdatasetp) {
	dns_rdataset_t *rdataset;

	REQUIRE(NS_CLIENT_VALID(client));
	REQUIRE(rdatasetp != NULL);

	rdataset = *rdatasetp;

	if (rdataset != NULL) {
		if (dns_rdataset_isassociated(rdataset)) {
			dns_rdataset_disassociate(rdataset);
		}
		dns_message_puttemprdataset(client->message, rdatasetp);
	}
}

isc_result_t
ns_client_newnamebuf(ns_client_t *client) {
	isc_buffer_t *dbuf;
	isc_result_t result;

	CTRACE("ns_client_newnamebuf");

	dbuf = NULL;
	result = isc_buffer_allocate(client->mctx, &dbuf, 1024);
	if (result != ISC_R_SUCCESS) {
		CTRACE("ns_client_newnamebuf: "
		       "isc_buffer_allocate failed: done");
		return (result);
	}
	ISC_LIST_APPEND(client->query.namebufs, dbuf, link);

	CTRACE("ns_client_newnamebuf: done");
	return (ISC_R_SUCCESS);
}

dns_name_t *
ns_client_newname(ns_client_t *client, isc_buffer_t *dbuf, isc_buffer_t *nbuf) {
	dns_name_t *name;
	isc_region_t r;
	isc_result_t result;

	REQUIRE((client->query.attributes & NS_QUERYATTR_NAMEBUFUSED) == 0);

	CTRACE("ns_client_newname");

	name = NULL;
	result = dns_message_gettempname(client->message, &name);
	if (result != ISC_R_SUCCESS) {
		CTRACE("ns_client_newname: "
		       "dns_message_gettempname failed: done");
		return (NULL);
	}
	isc_buffer_availableregion(dbuf, &r);
	isc_buffer_init(nbuf, r.base, r.length);
	dns_name_init(name, NULL);
	dns_name_setbuffer(name, nbuf);
	client->query.attributes |= NS_QUERYATTR_NAMEBUFUSED;

	CTRACE("ns_client_newname: done");
	return (name);
}

isc_buffer_t *
ns_client_getnamebuf(ns_client_t *client) {
	isc_buffer_t *dbuf;
	isc_result_t result;
	isc_region_t r;

	CTRACE("ns_client_getnamebuf");

	/*%
	 * Return a name buffer with space for a maximal name, allocating
	 * a new one if necessary.
	 */
	if (ISC_LIST_EMPTY(client->query.namebufs)) {
		result = ns_client_newnamebuf(client);
		if (result != ISC_R_SUCCESS) {
		    CTRACE("ns_client_getnamebuf: "
			   "ns_client_newnamebuf failed: done");
			return (NULL);
		}
	}

	dbuf = ISC_LIST_TAIL(client->query.namebufs);
	INSIST(dbuf != NULL);
	isc_buffer_availableregion(dbuf, &r);
	if (r.length < DNS_NAME_MAXWIRE) {
		result = ns_client_newnamebuf(client);
		if (result != ISC_R_SUCCESS) {
		    CTRACE("ns_client_getnamebuf: "
			   "ns_client_newnamebuf failed: done");
			return (NULL);

		}
		dbuf = ISC_LIST_TAIL(client->query.namebufs);
		isc_buffer_availableregion(dbuf, &r);
		INSIST(r.length >= 255);
	}
	CTRACE("ns_client_getnamebuf: done");
	return (dbuf);
}

void
ns_client_keepname(ns_client_t *client, dns_name_t *name, isc_buffer_t *dbuf) {
	isc_region_t r;

	CTRACE("ns_client_keepname");

	/*%
	 * 'name' is using space in 'dbuf', but 'dbuf' has not yet been
	 * adjusted to take account of that.  We do the adjustment.
	 */
	REQUIRE((client->query.attributes & NS_QUERYATTR_NAMEBUFUSED) != 0);

	dns_name_toregion(name, &r);
	isc_buffer_add(dbuf, r.length);
	dns_name_setbuffer(name, NULL);
	client->query.attributes &= ~NS_QUERYATTR_NAMEBUFUSED;
}

void
ns_client_releasename(ns_client_t *client, dns_name_t **namep) {
	dns_name_t *name = *namep;

	/*%
	 * 'name' is no longer needed.  Return it to our pool of temporary
	 * names.  If it is using a name buffer, relinquish its exclusive
	 * rights on the buffer.
	 */

	CTRACE("ns_client_releasename");
	if (dns_name_hasbuffer(name)) {
		INSIST((client->query.attributes & NS_QUERYATTR_NAMEBUFUSED)
		       != 0);
		client->query.attributes &= ~NS_QUERYATTR_NAMEBUFUSED;
	}
	dns_message_puttempname(client->message, namep);
	CTRACE("ns_client_releasename: done");
}

isc_result_t
ns_client_newdbversion(ns_client_t *client, unsigned int n) {
	unsigned int i;
	ns_dbversion_t *dbversion;

	for (i = 0; i < n; i++) {
		dbversion = isc_mem_get(client->mctx, sizeof(*dbversion));
		if (dbversion != NULL) {
			dbversion->db = NULL;
			dbversion->version = NULL;
			ISC_LIST_INITANDAPPEND(client->query.freeversions,
					      dbversion, link);
		} else {
			/*
			 * We only return ISC_R_NOMEMORY if we couldn't
			 * allocate anything.
			 */
			if (i == 0) {
				return (ISC_R_NOMEMORY);
			} else {
				return (ISC_R_SUCCESS);
			}
		}
	}

	return (ISC_R_SUCCESS);
}

static inline ns_dbversion_t *
client_getdbversion(ns_client_t *client) {
	isc_result_t result;
	ns_dbversion_t *dbversion;

	if (ISC_LIST_EMPTY(client->query.freeversions)) {
		result = ns_client_newdbversion(client, 1);
		if (result != ISC_R_SUCCESS) {
			return (NULL);
		}
	}
	dbversion = ISC_LIST_HEAD(client->query.freeversions);
	INSIST(dbversion != NULL);
	ISC_LIST_UNLINK(client->query.freeversions, dbversion, link);

	return (dbversion);
}

ns_dbversion_t *
ns_client_findversion(ns_client_t *client, dns_db_t *db) {
	ns_dbversion_t *dbversion;

	for (dbversion = ISC_LIST_HEAD(client->query.activeversions);
	     dbversion != NULL;
	     dbversion = ISC_LIST_NEXT(dbversion, link))
	{
		if (dbversion->db == db) {
			break;
		}
	}

	if (dbversion == NULL) {
		/*
		 * This is a new zone for this query.  Add it to
		 * the active list.
		 */
		dbversion = client_getdbversion(client);
		if (dbversion == NULL) {
			return (NULL);
		}
		dns_db_attach(db, &dbversion->db);
		dns_db_currentversion(db, &dbversion->version);
		dbversion->acl_checked = false;
		dbversion->queryok = false;
		ISC_LIST_APPEND(client->query.activeversions,
				dbversion, link);
	}

	return (dbversion);
}
