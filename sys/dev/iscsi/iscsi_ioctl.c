/*	$NetBSD: iscsi_ioctl.c,v 1.27 2017/12/03 07:23:45 mlelstv Exp $	*/

/*-
 * Copyright (c) 2004,2005,2006,2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Wasabi Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "iscsi_globals.h"

#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/proc.h>

#ifndef ISCSI_MINIMAL
#include <uvm/uvm.h>
#include <uvm/uvm_pmap.h>
#endif

static kmutex_t iscsi_cleanup_mtx;
static kcondvar_t iscsi_cleanup_cv;
static kcondvar_t iscsi_event_cv;
static struct lwp *iscsi_cleanproc = NULL;

static uint16_t current_id = 0;	/* Global session ID counter */

/* list of event handlers */
static event_handler_list_t event_handlers =
	TAILQ_HEAD_INITIALIZER(event_handlers);

static connection_list_t iscsi_timeout_conn_list =
	TAILQ_HEAD_INITIALIZER(iscsi_timeout_conn_list);

static ccb_list_t iscsi_timeout_ccb_list =
	TAILQ_HEAD_INITIALIZER(iscsi_timeout_ccb_list);

static session_list_t iscsi_cleanups_list =
	TAILQ_HEAD_INITIALIZER(iscsi_cleanups_list);

static connection_list_t iscsi_cleanupc_list =
	TAILQ_HEAD_INITIALIZER(iscsi_cleanupc_list);

static uint32_t handler_id = 0;	/* Handler ID counter */

/* -------------------------------------------------------------------------- */

/* Event management functions */

/*
 * find_handler:
 *    Search the event handler list for the given ID.
 *
 *    Parameter:
 *          id    The handler ID.
 *
 *    Returns:
 *          Pointer to handler if found, else NULL.
 */


static event_handler_t *
find_handler(uint32_t id)
{
	event_handler_t *curr;

	KASSERT(mutex_owned(&iscsi_cleanup_mtx));

	TAILQ_FOREACH(curr, &event_handlers, link)
		if (curr->id == id)
			break;

	return curr;
}


/*
 * register_event:
 *    Create event handler entry, return ID.
 *
 *    Parameter:
 *          par   The parameter.
 */

static void
register_event(iscsi_register_event_parameters_t *par)
{
	event_handler_t *handler;
	int was_empty;

	handler = malloc(sizeof(event_handler_t), M_DEVBUF, M_WAITOK | M_ZERO);
	if (handler == NULL) {
		DEBOUT(("No mem for event handler\n"));
		par->status = ISCSI_STATUS_NO_RESOURCES;
		return;
	}

	TAILQ_INIT(&handler->events);

	mutex_enter(&iscsi_cleanup_mtx);
	/* create a unique ID */
	do {
		++handler_id;
	} while (!handler_id || find_handler(handler_id) != NULL);
	par->event_id = handler->id = handler_id;

	was_empty = TAILQ_FIRST(&event_handlers) == NULL;
	TAILQ_INSERT_TAIL(&event_handlers, handler, link);
	if (was_empty)
		iscsi_notify_cleanup();
	mutex_exit(&iscsi_cleanup_mtx);

	par->status = ISCSI_STATUS_SUCCESS;
	DEB(5, ("Register Event OK, ID %d\n", par->event_id));
}


/*
 * deregister_event:
 *    Destroy handler entry and any waiting events, wake up waiter.
 *
 *    Parameter:
 *          par   The parameter.
 */

static void
deregister_event(iscsi_register_event_parameters_t *par)
{
	event_handler_t *handler;
	event_t *evt;

	mutex_enter(&iscsi_cleanup_mtx);
	handler = find_handler(par->event_id);
	if (handler == NULL) {
		mutex_exit(&iscsi_cleanup_mtx);
		DEB(1, ("Deregister Event ID %d not found\n", par->event_id));
		par->status = ISCSI_STATUS_INVALID_EVENT_ID;
		return;
	}

	TAILQ_REMOVE(&event_handlers, handler, link);
	if (handler->waiter != NULL) {
		handler->waiter->status = ISCSI_STATUS_EVENT_DEREGISTERED;
		cv_broadcast(&iscsi_event_cv);
	}

	while ((evt = TAILQ_FIRST(&handler->events)) != NULL) {
		TAILQ_REMOVE(&handler->events, evt, link);
		free(evt, M_TEMP);
	}
	mutex_exit(&iscsi_cleanup_mtx);

	free(handler, M_DEVBUF);
	par->status = ISCSI_STATUS_SUCCESS;
	DEB(5, ("Deregister Event ID %d complete\n", par->event_id));
}


/*
 * check_event:
 *    Return first queued event. Optionally wait for arrival of event.
 *
 *    Parameter:
 *          par   The parameter.
 *          wait  Wait for event if true
 */

static void
check_event(iscsi_wait_event_parameters_t *par, bool wait)
{
	event_handler_t *handler;
	event_t *evt;
	int rc;

	mutex_enter(&iscsi_cleanup_mtx);
	handler = find_handler(par->event_id);
	if (handler == NULL) {
		mutex_exit(&iscsi_cleanup_mtx);
		DEBOUT(("Wait Event ID %d not found\n", par->event_id));
		par->status = ISCSI_STATUS_INVALID_EVENT_ID;
		return;
	}
	if (handler->waiter != NULL) {
		mutex_exit(&iscsi_cleanup_mtx);
		DEBOUT(("Wait Event ID %d already waiting\n", par->event_id));
		par->status = ISCSI_STATUS_EVENT_WAITING;
		return;
	}
	par->status = ISCSI_STATUS_SUCCESS;
	DEB(99, ("Wait Event ID %d\n", par->event_id));

	do {
		evt = TAILQ_FIRST(&handler->events);
		if (evt != NULL) {
			TAILQ_REMOVE(&handler->events, evt, link);
		} else {
			if (!wait) {
				par->status = ISCSI_STATUS_LIST_EMPTY;
				return;
			}
			if (par->status != ISCSI_STATUS_SUCCESS) {
				return;
			}
			handler->waiter = par;
			rc = cv_wait_sig(&iscsi_event_cv, &iscsi_cleanup_mtx);
			if (rc) {
				mutex_exit(&iscsi_cleanup_mtx);
				par->status = ISCSI_STATUS_LIST_EMPTY;
				return;
			}
		}
	} while (evt == NULL);
	mutex_exit(&iscsi_cleanup_mtx);

	par->connection_id = evt->connection_id;
	par->session_id = evt->session_id;
	par->event_kind = evt->event_kind;
	par->reason = evt->reason;

	free(evt, M_TEMP);
}

/*
 * add_event
 *    Adds an event entry to each registered handler queue.
 *    Note that events are simply duplicated because we expect the number of
 *    handlers to be very small, usually 1 (the daemon).
 *
 *    Parameters:
 *       kind     The event kind
 *       sid      The ID of the affected session
 *       cid      The ID of the affected connection
 *       reason   The reason code
 */

void
add_event(iscsi_event_t kind, uint32_t sid, uint32_t cid, uint32_t reason)
{
	event_handler_t *curr;
	event_t *evt;

	DEB(9, ("Add_event kind %d, sid %d, cid %d, reason %d\n",
		kind, sid, cid, reason));

	mutex_enter(&iscsi_cleanup_mtx);
	TAILQ_FOREACH(curr, &event_handlers, link) {
		evt = malloc(sizeof(*evt), M_TEMP, M_NOWAIT);
		if (evt == NULL) {
			DEBOUT(("Cannot allocate event\n"));
			break;
		}
		evt->event_kind = kind;
		evt->session_id = sid;
		evt->connection_id = cid;
		evt->reason = reason;

		TAILQ_INSERT_TAIL(&curr->events, evt, link);
		if (curr->waiter != NULL) {
			curr->waiter = NULL;
			cv_broadcast(&iscsi_event_cv);
		}
	}
	mutex_exit(&iscsi_cleanup_mtx);
}


/*
 * check_event_handlers
 *    Checks for dead event handlers. A dead event handler would deplete
 *    memory over time, so we have to make sure someone at the other
 *    end is actively monitoring events.
 *    This function is called every 30 seconds or so (less frequent if there
 *    is other activity for the cleanup thread to deal with) to go through
 *    the list of handlers and check whether the first element in the event
 *    list has changed at all. If not, the event is deregistered.
 *    Note that this will not detect dead handlers if no events are pending,
 *    but we don't care as long as events don't accumulate in the list.
 *
 */

static void
check_event_handlers(void)
{
	event_handler_t *curr, *next;
	event_t *evt;

	KASSERT(mutex_owned(&iscsi_cleanup_mtx));

	for (curr = TAILQ_FIRST(&event_handlers); curr != NULL; curr = next) {
		next = TAILQ_NEXT(curr, link);
		evt = TAILQ_FIRST(&curr->events);

		if (evt != NULL && evt == curr->first_in_list) {
			DEBOUT(("Found Dead Event Handler %d, removing\n", curr->id));

			TAILQ_REMOVE(&event_handlers, curr, link);
			while ((evt = TAILQ_FIRST(&curr->events)) != NULL) {
				TAILQ_REMOVE(&curr->events, evt, link);
				free(evt, M_TEMP);
			}
			free(curr, M_DEVBUF);
		} else
			curr->first_in_list = evt;
	}
}


/* -------------------------------------------------------------------------- */

/*
 * get_socket:
 *    Get the file pointer from the socket handle passed into login.
 *
 *    Parameter:
 *          fdes     IN: The socket handle
 *          fpp      OUT: The pointer to the resulting file pointer
 *
 *    Returns:    0 on success, else an error code.
 *
 */

static int
get_socket(int fdes, struct file **fpp)
{
	struct file *fp;

	if ((fp = fd_getfile(fdes)) == NULL) {
		return EBADF;
	}
	if (fp->f_type != DTYPE_SOCKET) {
		return ENOTSOCK;
	}

	/* Add the reference */
	mutex_enter(&fp->f_lock);
	fp->f_count++;
	mutex_exit(&fp->f_lock);

	*fpp = fp;
	return 0;
}

/*
 * release_socket:
 *    Release the file pointer from the socket handle passed into login.
 *
 *    Parameter:
 *          fp       IN: The pointer to the resulting file pointer
 *
 */

static void
release_socket(struct file *fp)
{
	/* Add the reference */
	mutex_enter(&fp->f_lock);
	fp->f_count--;
	mutex_exit(&fp->f_lock);
}


/*
 * find_session:
 *    Find a session by ID.
 *
 *    Parameter:  the session ID
 *
 *    Returns:    The pointer to the session (or NULL if not found)
 */

session_t *
find_session(uint32_t id)
{
	session_t *curr;

	KASSERT(mutex_owned(&iscsi_cleanup_mtx));

	TAILQ_FOREACH(curr, &iscsi_sessions, sessions)
		if (curr->id == id) {
			break;
		}
	return curr;
}


/*
 * find_connection:
 *    Find a connection by ID.
 *
 *    Parameter:  the session pointer and the connection ID
 *
 *    Returns:    The pointer to the connection (or NULL if not found)
 */

connection_t *
find_connection(session_t *session, uint32_t id)
{
	connection_t *curr;

	KASSERT(mutex_owned(&iscsi_cleanup_mtx));

	TAILQ_FOREACH(curr, &session->conn_list, connections)
		if (curr->id == id) {
			break;
		}
	return curr;
}

/*
 * ref_session:
 *    Reference a session
 *
 *    Session cannot be release until reference count reaches zero
 *
 *    Returns: 1 if reference counter would overflow
 */

int
ref_session(session_t *session)
{
	int rc = 1;

	mutex_enter(&iscsi_cleanup_mtx);
	KASSERT(session != NULL);
	if (session->refcount <= CCBS_PER_SESSION) {
		session->refcount++;
		rc = 0;
	}
	mutex_exit(&iscsi_cleanup_mtx);

	return rc;
}

/*
 * unref_session:
 *    Unreference a session
 *
 *    Release session reference, trigger cleanup
 */

void
unref_session(session_t *session)
{

	mutex_enter(&iscsi_cleanup_mtx);
	KASSERT(session != NULL);
	KASSERT(session->refcount > 0);
	if (--session->refcount == 0)
		cv_broadcast(&session->sess_cv);
	mutex_exit(&iscsi_cleanup_mtx);
}


/*
 * kill_connection:
 *    Terminate the connection as gracefully as possible.
 *
 *    Parameter:
 *		conn		The connection to terminate
 *		status		The status code for the connection's "terminating" field
 *		logout		The logout reason code
 *		recover	Attempt to recover connection
 */

void
kill_connection(connection_t *conn, uint32_t status, int logout, bool recover)
{
	session_t *sess = conn->session;
	int terminating;

	DEBC(conn, 1, ("Kill_connection: terminating=%d, status=%d, logout=%d, "
			   "state=%d\n",
			   conn->terminating, status, logout, conn->state));

	mutex_enter(&iscsi_cleanup_mtx);
	if (recover &&
	    !conn->destroy &&
	    conn->recover > MAX_RECOVERY_ATTEMPTS) {
		DEBC(conn, 1,
			  ("Kill_connection: Too many recovery attempts, destroying\n"));
		conn->destroy = TRUE;
	}

	if (!recover || conn->destroy) {

		if (conn->in_session) {
			conn->in_session = FALSE;
			TAILQ_REMOVE(&sess->conn_list, conn, connections);
			sess->mru_connection = TAILQ_FIRST(&sess->conn_list);
		}

		if (!conn->destroy) {
			DEBC(conn, 1, ("Kill_connection setting destroy flag\n"));
			conn->destroy = TRUE;
		}
	}

	terminating = conn->terminating;
	if (!terminating)
		conn->terminating = status;

	/* Don't recurse */
	if (terminating) {
		mutex_exit(&iscsi_cleanup_mtx);

		KASSERT(conn->state != ST_FULL_FEATURE);
		DEBC(conn, 1, ("Kill_connection exiting (already terminating)\n"));
		goto done;
	}

	if (conn->state == ST_FULL_FEATURE) {
		sess->active_connections--;
		conn->state = ST_WINDING_DOWN;

		/* If this is the last connection and ERL < 2, reset TSIH */
		if (!sess->active_connections && sess->ErrorRecoveryLevel < 2)
			sess->TSIH = 0;

		/* Don't try to log out if the socket is broken or we're in the middle */
		/* of logging in */
		if (logout >= 0) {
			if (sess->ErrorRecoveryLevel < 2 &&
			    logout == RECOVER_CONNECTION) {
				logout = LOGOUT_CONNECTION;
			}
			if (!sess->active_connections &&
			    logout == LOGOUT_CONNECTION) {
				logout = LOGOUT_SESSION;
			}
			mutex_exit(&iscsi_cleanup_mtx);

			connection_timeout_start(conn, CONNECTION_TIMEOUT);

			if (!send_logout(conn, conn, logout, FALSE)) {
				conn->terminating = ISCSI_STATUS_SUCCESS;
				return;
			}
			/*
			 * if the logout request was successfully sent,
			 * the logout response handler will do the rest
			 * of the termination processing. If the logout
			 * doesn't get a response, we'll get back in here
			 * once the timeout hits.
			 */

			mutex_enter(&iscsi_cleanup_mtx);
		}

	}

	conn->state = ST_SETTLING;
	mutex_exit(&iscsi_cleanup_mtx);

done:
	/* let send thread take over next step of cleanup */
	mutex_enter(&conn->lock);
	cv_broadcast(&conn->conn_cv);
	mutex_exit(&conn->lock);

	DEBC(conn, 5, ("kill_connection returns\n"));
}


/*
 * kill_session:
 *    Terminate the session as gracefully as possible.
 *
 *    Parameter:
 *		session		Session to terminate
 *		status		The status code for the termination
 *		logout		The logout reason code

 */

void
kill_session(session_t *session, uint32_t status, int logout, bool recover)
{
	connection_t *conn;

	DEB(1, ("ISCSI: kill_session %d, status %d, logout %d, recover %d\n",
			session->id, status, logout, recover));

	mutex_enter(&iscsi_cleanup_mtx);
	if (session->terminating) {
		mutex_exit(&iscsi_cleanup_mtx);

		DEB(5, ("Session is being killed with status %d\n",session->terminating));
		return;
	}

	/*
	 * don't do anything if session isn't established yet, termination will be
	 * handled elsewhere
	 */
	if (session->sessions.tqe_next == NULL && session->sessions.tqe_prev == NULL) {
		mutex_exit(&iscsi_cleanup_mtx);

		DEB(5, ("Session is being killed which is not yet established\n"));
		return;
	}

	if (recover) {
		mutex_exit(&iscsi_cleanup_mtx);

		/*
		 * Only recover when there's just one active connection left.
		 * Otherwise we get in all sorts of timing problems, and it doesn't
		 * make much sense anyway to recover when the other side has
		 * requested that we kill a multipathed session.
		 */
		if (session->active_connections == 1) {
			conn = assign_connection(session, FALSE);
			if (conn != NULL)
				kill_connection(conn, status, logout, TRUE);
		}
		return;
	}

	if (session->refcount > 0) {
		mutex_exit(&iscsi_cleanup_mtx);

		DEB(5, ("Session is being killed while in use (refcnt = %d)\n",
			session->refcount));
		return;
	}

	/* Remove session from global list */
	session->terminating = status;
	TAILQ_REMOVE(&iscsi_sessions, session, sessions);
	session->sessions.tqe_next = NULL;
	session->sessions.tqe_prev = NULL;

	mutex_exit(&iscsi_cleanup_mtx);

	/* kill all connections */
	while ((conn = TAILQ_FIRST(&session->conn_list)) != NULL) {
		kill_connection(conn, status, logout, FALSE);
		logout = NO_LOGOUT;
	}
}


/*
 * create_connection:
 *    Create and init the necessary framework for a connection:
 *       Alloc the connection structure itself
 *       Copy connection parameters
 *       Create the send and receive threads
 *       And finally, log in.
 *
 *    Parameter:
 *          par      IN/OUT: The login parameters
 *          session  IN: The owning session
 *          l        IN: The lwp pointer of the caller
 *
 *    Returns:    0 on success
 *                >0 on failure, connection structure deleted
 *                <0 on failure, connection is still terminating
 */

static int
create_connection(iscsi_login_parameters_t *par, session_t *session,
				  struct lwp *l)
{
	connection_t *connection;
	int rc;

	DEB(1, ("Create Connection for Session %d\n", session->id));

	if (session->MaxConnections &&
	    session->active_connections >= session->MaxConnections) {
		DEBOUT(("Too many connections (max = %d, curr = %d)\n",
				session->MaxConnections, session->active_connections));
		par->status = ISCSI_STATUS_MAXED_CONNECTIONS;
		return EIO;
	}

	connection = malloc(sizeof(*connection), M_DEVBUF, M_WAITOK | M_ZERO);
	if (connection == NULL) {
		DEBOUT(("No mem for connection\n"));
		par->status = ISCSI_STATUS_NO_RESOURCES;
		return EIO;
	}

	mutex_enter(&iscsi_cleanup_mtx);
	/* create a unique ID */
	do {
		++session->conn_id;
	} while (!session->conn_id ||
		 find_connection(session, session->conn_id) != NULL);
	par->connection_id = connection->id = session->conn_id;
	mutex_exit(&iscsi_cleanup_mtx);
	DEB(99, ("Connection ID = %d\n", connection->id));

	connection->session = session;

	TAILQ_INIT(&connection->ccbs_waiting);
	TAILQ_INIT(&connection->pdus_to_send);
	TAILQ_INIT(&connection->pdu_pool);

	mutex_init(&connection->lock, MUTEX_DEFAULT, IPL_BIO);
	cv_init(&connection->conn_cv, "conn");
	cv_init(&connection->pdu_cv, "pdupool");
	cv_init(&connection->ccb_cv, "ccbwait");
	cv_init(&connection->idle_cv, "idle");

	callout_init(&connection->timeout, CALLOUT_MPSAFE);
	callout_setfunc(&connection->timeout, connection_timeout_co, connection);
	connection->idle_timeout_val = CONNECTION_IDLE_TIMEOUT;

	init_sernum(&connection->StatSN_buf);
	create_pdus(connection);

	if ((rc = get_socket(par->socket, &connection->sock)) != 0) {
		DEBOUT(("Invalid socket %d\n", par->socket));

		callout_destroy(&connection->timeout);
		cv_destroy(&connection->idle_cv);
		cv_destroy(&connection->ccb_cv);
		cv_destroy(&connection->pdu_cv);
		cv_destroy(&connection->conn_cv);
		mutex_destroy(&connection->lock);
		free(connection, M_DEVBUF);
		par->status = ISCSI_STATUS_INVALID_SOCKET;
		return rc;
	}
	DEBC(connection, 1, ("get_socket: par_sock=%d, fdesc=%p\n",
			par->socket, connection->sock));

	/* close the file descriptor */
	fd_close(par->socket);

	connection->threadobj = l;
	connection->login_par = par;

	DEB(5, ("Creating receive thread\n"));
	if ((rc = kthread_create(PRI_BIO, KTHREAD_MPSAFE, NULL, iscsi_rcv_thread,
				connection, &connection->rcvproc,
				"ConnRcv")) != 0) {
		DEBOUT(("Can't create rcv thread (rc %d)\n", rc));

		release_socket(connection->sock);
		callout_destroy(&connection->timeout);
		cv_destroy(&connection->idle_cv);
		cv_destroy(&connection->ccb_cv);
		cv_destroy(&connection->pdu_cv);
		cv_destroy(&connection->conn_cv);
		mutex_destroy(&connection->lock);
		free(connection, M_DEVBUF);
		par->status = ISCSI_STATUS_NO_RESOURCES;
		return rc;
	}
	DEB(5, ("Creating send thread\n"));
	if ((rc = kthread_create(PRI_BIO, KTHREAD_MPSAFE, NULL, iscsi_send_thread,
				connection, &connection->sendproc,
				"ConnSend")) != 0) {
		DEBOUT(("Can't create send thread (rc %d)\n", rc));

		connection->terminating = ISCSI_STATUS_NO_RESOURCES;

		/*
		 * We must close the socket here to force the receive
		 * thread to wake up
		 */
		DEBC(connection, 1,
			("Closing Socket %p\n", connection->sock));
		mutex_enter(&connection->sock->f_lock);
		connection->sock->f_count += 1;
		mutex_exit(&connection->sock->f_lock);
		closef(connection->sock);

		/* give receive thread time to exit */
		kpause("settle", false, 2 * hz, NULL);

		release_socket(connection->sock);
		callout_destroy(&connection->timeout);
		cv_destroy(&connection->idle_cv);
		cv_destroy(&connection->ccb_cv);
		cv_destroy(&connection->pdu_cv);
		cv_destroy(&connection->conn_cv);
		mutex_destroy(&connection->lock);
		free(connection, M_DEVBUF);
		par->status = ISCSI_STATUS_NO_RESOURCES;
		return rc;
	}

	/*
	 * At this point, each thread will tie 'sock' into its own file descriptor
	 * tables w/o increasing the use count - they will inherit the use
	 * increments performed in get_socket().
	 */

	if ((rc = send_login(connection)) != 0) {
		DEBC(connection, 0, ("Login failed (rc %d)\n", rc));
		/* Don't attempt to recover, there seems to be something amiss */
		kill_connection(connection, rc, NO_LOGOUT, FALSE);
		par->status = rc;
		return -1;
	}

	mutex_enter(&iscsi_cleanup_mtx);
	if (session->terminating) {
		mutex_exit(&iscsi_cleanup_mtx);
		DEBC(connection, 0, ("Session terminating\n"));
		kill_connection(connection, rc, NO_LOGOUT, FALSE);
		par->status = session->terminating;
		return -1;
	}
	connection->state = ST_FULL_FEATURE;
	TAILQ_INSERT_TAIL(&session->conn_list, connection, connections);
	connection->in_session = TRUE;
	session->total_connections++;
	session->active_connections++;
	session->mru_connection = connection;
	mutex_exit(&iscsi_cleanup_mtx);

	DEBC(connection, 5, ("Connection created successfully!\n"));
	return 0;
}


/*
 * recreate_connection:
 *    Revive dead connection
 *
 *    Parameter:
 *          par      IN/OUT: The login parameters
 *          conn     IN: The connection
 *          l        IN: The lwp pointer of the caller
 *
 *    Returns:    0 on success
 *                >0 on failure, connection structure deleted
 *                <0 on failure, connection is still terminating
 */

static int
recreate_connection(iscsi_login_parameters_t *par, session_t *session,
					connection_t *connection, struct lwp *l)
{
	int rc;
	ccb_t *ccb;
	ccb_list_t old_waiting;
	pdu_t *pdu;
	uint32_t sn;

	DEB(1, ("ReCreate Connection %d for Session %d, ERL=%d\n",
		connection->id, connection->session->id,
		connection->session->ErrorRecoveryLevel));

	if (session->MaxConnections &&
	    session->active_connections >= session->MaxConnections) {
		DEBOUT(("Too many connections (max = %d, curr = %d)\n",
			session->MaxConnections, session->active_connections));
		par->status = ISCSI_STATUS_MAXED_CONNECTIONS;
		return EIO;
	}

	/* close old socket */
	if (connection->sock != NULL) {
		closef(connection->sock);
		connection->sock = NULL;
	}

	if ((rc = get_socket(par->socket, &connection->sock)) != 0) {
		DEBOUT(("Invalid socket %d\n", par->socket));
		par->status = ISCSI_STATUS_INVALID_SOCKET;
		return rc;
	}
	DEBC(connection, 1, ("get_socket: par_sock=%d, fdesc=%p\n",
			par->socket, connection->sock));

	/* close the file descriptor */
	fd_close(par->socket);

	connection->threadobj = l;
	connection->login_par = par;
	connection->terminating = ISCSI_STATUS_SUCCESS;
	connection->recover++;
	connection->num_timeouts = 0;
	connection->state = ST_SEC_NEG;
	connection->HeaderDigest = 0;
	connection->DataDigest = 0;

	session->active_connections++;

	TAILQ_INIT(&old_waiting);

	mutex_enter(&connection->lock);
	while ((ccb = TAILQ_FIRST(&connection->ccbs_waiting)) != NULL) {
		suspend_ccb(ccb, FALSE);
		TAILQ_INSERT_TAIL(&old_waiting, ccb, chain);
	}
	init_sernum(&connection->StatSN_buf);
	cv_broadcast(&connection->idle_cv);
	mutex_exit(&connection->lock);

	if ((rc = send_login(connection)) != 0) {
		DEBOUT(("Login failed (rc %d)\n", rc));
		while ((ccb = TAILQ_FIRST(&old_waiting)) != NULL) {
			TAILQ_REMOVE(&old_waiting, ccb, chain);
			wake_ccb(ccb, rc);
		}
		/* Don't attempt to recover, there seems to be something amiss */
		kill_connection(connection, rc, NO_LOGOUT, FALSE);
		par->status = rc;
		return -1;
	}

	DEBC(connection, 9, ("Re-Login successful\n"));
	par->status = ISCSI_STATUS_SUCCESS;

	connection->state = ST_FULL_FEATURE;
	session->mru_connection = connection;

	while ((ccb = TAILQ_FIRST(&old_waiting)) != NULL) {
		TAILQ_REMOVE(&old_waiting, ccb, chain);
		mutex_enter(&connection->lock);
		suspend_ccb(ccb, TRUE);
		mutex_exit(&connection->lock);

		rc = send_task_management(connection, ccb, NULL, TASK_REASSIGN);
		/* if we get an error on reassign, restart the original request */
		if (rc && ccb->pdu_waiting != NULL) {
			mutex_enter(&session->lock);
			if (sn_a_lt_b(ccb->CmdSN, session->ExpCmdSN)) {
				pdu = ccb->pdu_waiting;
				sn = get_sernum(session, pdu);

				/* update CmdSN */
				DEBC(connection, 0, ("Resend ccb %p (%d) - updating CmdSN old %u, new %u\n",
					   ccb, rc, ccb->CmdSN, sn));
				ccb->CmdSN = sn;
				pdu->pdu.p.command.CmdSN = htonl(ccb->CmdSN);
			} else {
				DEBC(connection, 0, ("Resend ccb %p (%d) - CmdSN %u\n",
					   ccb, rc, ccb->CmdSN));
			}
			mutex_exit(&session->lock);
			resend_pdu(ccb);
		} else {
			DEBC(connection, 0, ("Resend ccb %p (%d) CmdSN %u - reassigned\n",
				ccb, rc, ccb->CmdSN));
			ccb_timeout_start(ccb, COMMAND_TIMEOUT);
		}
	}

	mutex_enter(&session->lock);
	cv_broadcast(&session->sess_cv);
	mutex_exit(&session->lock);

	DEBC(connection, 0, ("Connection ReCreated successfully - status %d\n",
						 par->status));

	return 0;
}

/* -------------------------------------------------------------------------- */

/*
 * check_login_pars:
 *    Check the parameters passed into login/add_connection
 *    for validity and consistency.
 *
 *    Parameter:
 *          par      The login parameters
 *
 *    Returns:    0 on success, else an error code.
 */

static int
check_login_pars(iscsi_login_parameters_t *par)
{
	int i, n;

	if (par->is_present.auth_info) {
		/* check consistency of authentication parameters */

		if (par->auth_info.auth_number > ISCSI_AUTH_OPTIONS) {
			DEBOUT(("Auth number invalid: %d\n", par->auth_info.auth_number));
			return ISCSI_STATUS_PARAMETER_INVALID;
		}

		if (par->auth_info.auth_number > 2) {
			DEBOUT(("Auth number invalid: %d\n", par->auth_info.auth_number));
			return ISCSI_STATUS_NOTIMPL;
		}

		for (i = 0, n = 0; i < par->auth_info.auth_number; i++) {
#if 0
			if (par->auth_info.auth_type[i] < ISCSI_AUTH_None) {
				DEBOUT(("Auth type invalid: %d\n",
						par->auth_info.auth_type[i]));
				return ISCSI_STATUS_PARAMETER_INVALID;
			}
#endif
			if (par->auth_info.auth_type[i] > ISCSI_AUTH_CHAP) {
				DEBOUT(("Auth type invalid: %d\n",
						par->auth_info.auth_type[i]));
				return ISCSI_STATUS_NOTIMPL;
			}
			n = max(n, par->auth_info.auth_type[i]);
		}
		if (n) {
			if (!par->is_present.password ||
				(par->auth_info.mutual_auth &&
				 !par->is_present.target_password)) {
				DEBOUT(("Password missing\n"));
				return ISCSI_STATUS_PARAMETER_MISSING;
			}
			/* Note: Default for user-name is initiator name */
		}
	}
	if (par->login_type != ISCSI_LOGINTYPE_DISCOVERY &&
	    !par->is_present.TargetName) {
		DEBOUT(("Target name missing, login type %d\n", par->login_type));
		return ISCSI_STATUS_PARAMETER_MISSING;
	}
	if (par->is_present.MaxRecvDataSegmentLength) {
		if (par->MaxRecvDataSegmentLength < 512 ||
			par->MaxRecvDataSegmentLength > 0xffffff) {
			DEBOUT(("MaxRecvDataSegmentLength invalid: %d\n",
					par->MaxRecvDataSegmentLength));
			return ISCSI_STATUS_PARAMETER_INVALID;
		}
	}
	return 0;
}


/*
 * login:
 *    Handle the login ioctl - Create a session:
 *       Alloc the session structure
 *       Copy session parameters
 *       And call create_connection to establish the connection.
 *
 *    Parameter:
 *          par      IN/OUT: The login parameters
 *          l        IN: The lwp pointer of the caller
 */

static void
login(iscsi_login_parameters_t *par, struct lwp *l, device_t dev)
{
	session_t *session;
	int rc;

	DEB(99, ("ISCSI: login\n"));

	if (!iscsi_InitiatorName[0]) {
		DEB(1, ("No Initiator Name\n"));
		par->status = ISCSI_STATUS_NO_INITIATOR_NAME;
		return;
	}

	if ((par->status = check_login_pars(par)) != 0)
		return;

	/* alloc the session */
	session = malloc(sizeof(*session), M_DEVBUF, M_WAITOK | M_ZERO);
	if (session == NULL) {
		DEBOUT(("No mem for session\n"));
		par->status = ISCSI_STATUS_NO_RESOURCES;
		return;
	}
	TAILQ_INIT(&session->conn_list);
	TAILQ_INIT(&session->ccb_pool);

	mutex_init(&session->lock, MUTEX_DEFAULT, IPL_BIO);
	cv_init(&session->sess_cv, "session");
	cv_init(&session->ccb_cv, "ccb");

	mutex_enter(&iscsi_cleanup_mtx);
	/* create a unique ID */
	do {
		++current_id;
	} while (!current_id || find_session(current_id) != NULL);
	par->session_id = session->id = current_id;
	mutex_exit(&iscsi_cleanup_mtx);

	create_ccbs(session);
	session->login_type = par->login_type;
	session->CmdSN = 1;

	if ((rc = create_connection(par, session, l)) != 0) {
		if (rc > 0) {
			destroy_ccbs(session);
			cv_destroy(&session->ccb_cv);
			cv_destroy(&session->sess_cv);
			mutex_destroy(&session->lock);
			free(session, M_DEVBUF);
		}
		return;
	}

	mutex_enter(&iscsi_cleanup_mtx);
	TAILQ_INSERT_HEAD(&iscsi_sessions, session, sessions);
	mutex_exit(&iscsi_cleanup_mtx);

	/* Session established, map LUNs? */
	if (par->login_type == ISCSI_LOGINTYPE_MAP) {
		copyinstr(par->TargetName, session->tgtname,
		    sizeof(session->tgtname), NULL);
		DEB(1, ("Login: map session %d\n", session->id));
		if (!map_session(session, dev)) {
			DEB(1, ("Login: map session %d failed\n", session->id));
			kill_session(session, ISCSI_STATUS_MAP_FAILED,
					LOGOUT_SESSION, FALSE);
			par->status = ISCSI_STATUS_MAP_FAILED;
			return;
		}
	}
}


/*
 * logout:
 *    Handle the logout ioctl - Kill a session.
 *
 *    Parameter:
 *          par      IN/OUT: The login parameters
 */

static void
logout(iscsi_logout_parameters_t *par)
{
	session_t *session;

	DEB(5, ("ISCSI: logout session %d\n", par->session_id));

	mutex_enter(&iscsi_cleanup_mtx);
	if ((session = find_session(par->session_id)) == NULL) {
		mutex_exit(&iscsi_cleanup_mtx);
		DEBOUT(("Session %d not found\n", par->session_id));
		par->status = ISCSI_STATUS_INVALID_SESSION_ID;
		return;
	}
	mutex_exit(&iscsi_cleanup_mtx);
	/* If the session exists, this always succeeds */
	par->status = ISCSI_STATUS_SUCCESS;

	kill_session(session, ISCSI_STATUS_LOGOUT, LOGOUT_SESSION, FALSE);
}


/*
 * add_connection:
 *    Handle the add_connection ioctl.
 *
 *    Parameter:
 *          par      IN/OUT: The login parameters
 *          l        IN: The lwp pointer of the caller
 */

static void
add_connection(iscsi_login_parameters_t *par, struct lwp *l)
{
	session_t *session;

	DEB(5, ("ISCSI: add_connection to session %d\n", par->session_id));

	mutex_enter(&iscsi_cleanup_mtx);
	if ((session = find_session(par->session_id)) == NULL) {
		mutex_exit(&iscsi_cleanup_mtx);
		DEBOUT(("Session %d not found\n", par->session_id));
		par->status = ISCSI_STATUS_INVALID_SESSION_ID;
		return;
	}
	mutex_exit(&iscsi_cleanup_mtx);
	if ((par->status = check_login_pars(par)) == 0) {
		create_connection(par, session, l);
	}
}


/*
 * remove_connection:
 *    Handle the remove_connection ioctl.
 *
 *    Parameter:
 *          par      IN/OUT: The remove parameters
 */

static void
remove_connection(iscsi_remove_parameters_t *par)
{
	connection_t *conn;
	session_t *session;

	DEB(5, ("ISCSI: remove_connection %d from session %d\n",
			par->connection_id, par->session_id));

	mutex_enter(&iscsi_cleanup_mtx);
	if ((session = find_session(par->session_id)) == NULL) {
		mutex_exit(&iscsi_cleanup_mtx);
		DEBOUT(("Session %d not found\n", par->session_id));
		par->status = ISCSI_STATUS_INVALID_SESSION_ID;
		return;
	}

	if ((conn = find_connection(session, par->connection_id)) == NULL) {
		mutex_exit(&iscsi_cleanup_mtx);
		DEBOUT(("Connection %d not found in session %d\n",
				par->connection_id, par->session_id));

		par->status = ISCSI_STATUS_INVALID_CONNECTION_ID;
	} else {
		mutex_exit(&iscsi_cleanup_mtx);
		kill_connection(conn, ISCSI_STATUS_LOGOUT, LOGOUT_CONNECTION,
					FALSE);
		par->status = ISCSI_STATUS_SUCCESS;
	}
}


/*
 * restore_connection:
 *    Handle the restore_connection ioctl.
 *
 *    Parameter:
 *          par      IN/OUT: The login parameters
 *          l        IN: The lwp pointer of the caller
 */

static void
restore_connection(iscsi_login_parameters_t *par, struct lwp *l)
{
	session_t *session;
	connection_t *connection;

	DEB(1, ("ISCSI: restore_connection %d of session %d\n",
			par->connection_id, par->session_id));

	mutex_enter(&iscsi_cleanup_mtx);
	if ((session = find_session(par->session_id)) == NULL) {
		mutex_exit(&iscsi_cleanup_mtx);
		DEBOUT(("Session %d not found\n", par->session_id));
		par->status = ISCSI_STATUS_INVALID_SESSION_ID;
		return;
	}

	if ((connection = find_connection(session, par->connection_id)) == NULL) {
		mutex_exit(&iscsi_cleanup_mtx);
		DEBOUT(("Connection %d not found in session %d\n",
				par->connection_id, par->session_id));
		par->status = ISCSI_STATUS_INVALID_CONNECTION_ID;
		return;
	}
	mutex_exit(&iscsi_cleanup_mtx);

	if ((par->status = check_login_pars(par)) == 0) {
		recreate_connection(par, session, connection, l);
	}
}


#ifndef ISCSI_MINIMAL

/*
 * map_databuf:
 *    Map user-supplied data buffer into kernel space.
 *
 *    Parameter:
 *          p        IN: The proc pointer of the caller
 *          buf      IN/OUT: The virtual address of the buffer, modified
 *                   on exit to reflect kernel VA.
 *          datalen  IN: The size of the data buffer
 *
 *    Returns:
 *          An ISCSI status code on error, else 0.
 */

uint32_t
map_databuf(struct proc *p, void **buf, uint32_t datalen)
{
	vaddr_t kva, databuf, offs;
	int error;

	/* page align address */
	databuf = (vaddr_t) * buf & ~PAGE_MASK;
	/* offset of VA into page */
	offs = (vaddr_t) * buf & PAGE_MASK;
	/* round to full page including offset */
	datalen = (datalen + offs + PAGE_MASK) & ~PAGE_MASK;

	/* Do some magic to the vm space reference count (copied from "copyin_proc") */
	if ((p->p_sflag & PS_WEXIT) || (p->p_vmspace->vm_refcnt < 1)) {
		return ISCSI_STATUS_NO_RESOURCES;
	}
	p->p_vmspace->vm_refcnt++;

	/* this is lifted from uvm_io */
	error = uvm_map_extract(&p->p_vmspace->vm_map, databuf, datalen,
			kernel_map, &kva,
			UVM_EXTRACT_QREF | UVM_EXTRACT_CONTIG |
				UVM_EXTRACT_FIXPROT);
	if (error) {
		DEBOUT(("uvm_map_extract failed, error = %d\n", error));
		return ISCSI_STATUS_NO_RESOURCES;
	}
	/* add offset back into kernel VA */
	*buf = (void *) (kva + offs);

	return 0;
}


/*
 * unmap_databuf:
 *    Remove kernel space mapping of data buffer.
 *
 *    Parameter:
 *          p        IN: The proc pointer of the caller
 *          buf      IN: The kernel virtual address of the buffer
 *          datalen  IN: The size of the data buffer
 *
 *    Returns:
 *          An ISCSI status code on error, else 0.
 */

void
unmap_databuf(struct proc *p, void *buf, uint32_t datalen)
{
	struct vm_map_entry *dead_entries;
	vaddr_t databuf;

	/* round to full page */
	datalen = (datalen + ((uintptr_t) buf & PAGE_MASK) + PAGE_MASK) & ~PAGE_MASK;
	/* page align address */
	databuf = (vaddr_t) buf & ~PAGE_MASK;

	/* following code lifted almost verbatim from uvm_io.c */
	vm_map_lock(kernel_map);
	uvm_unmap_remove(kernel_map, databuf, databuf + datalen, &dead_entries,
	    0);
	vm_map_unlock(kernel_map);
	if (dead_entries != NULL) {
		uvm_unmap_detach(dead_entries, AMAP_REFALL);
	}
	/* this apparently reverses the magic to the vm ref count, from copyin_proc */
	uvmspace_free(p->p_vmspace);
}


/*
 * io_command:
 *    Handle the io_command ioctl.
 *
 *    Parameter:
 *          par      IN/OUT: The iocommand parameters
 *          l        IN: The lwp pointer of the caller
 */

static void
io_command(iscsi_iocommand_parameters_t *par, struct lwp *l)
{
	uint32_t datalen = par->req.datalen;
	void *databuf = par->req.databuf;
	session_t *session;

	DEB(9, ("ISCSI: io_command, SID=%d, lun=%" PRIu64 "\n", par->session_id, par->lun));
	mutex_enter(&iscsi_cleanup_mtx);
	if ((session = find_session(par->session_id)) == NULL) {
		mutex_exit(&iscsi_cleanup_mtx);
		DEBOUT(("Session %d not found\n", par->session_id));
		par->status = ISCSI_STATUS_INVALID_SESSION_ID;
		return;
	}
	mutex_exit(&iscsi_cleanup_mtx);

	par->req.senselen_used = 0;
	par->req.datalen_used = 0;
	par->req.error = 0;
	par->req.status = 0;
	par->req.retsts = SCCMD_UNKNOWN;	/* init to failure code */

	if (par->req.cmdlen > 16 || par->req.senselen > sizeof(par->req.sense)) {
		par->status = ISCSI_STATUS_PARAMETER_INVALID;
		return;
	}

	if (datalen && (par->status = map_databuf(l->l_proc,
			&par->req.databuf, datalen)) != 0) {
		return;
	}
	par->status = send_io_command(session, par->lun, &par->req,
								  par->options.immediate, par->connection_id);

	if (datalen) {
		unmap_databuf(l->l_proc, par->req.databuf, datalen);
		par->req.databuf = databuf;	/* restore original addr */
	}

	switch (par->status) {
	case ISCSI_STATUS_SUCCESS:
		par->req.retsts = SCCMD_OK;
		break;

	case ISCSI_STATUS_TARGET_BUSY:
		par->req.retsts = SCCMD_BUSY;
		break;

	case ISCSI_STATUS_TIMEOUT:
	case ISCSI_STATUS_SOCKET_ERROR:
		par->req.retsts = SCCMD_TIMEOUT;
		break;

	default:
		par->req.retsts = (par->req.senselen_used) ? SCCMD_SENSE
												   : SCCMD_UNKNOWN;
		break;
	}
}
#endif

/*
 * send_targets:
 *    Handle the send_targets ioctl.
 *    Note: If the passed buffer is too small to hold the complete response,
 *    the response is kept in the session structure so it can be
 *    retrieved with the next call to this function without having to go to
 *    the target again. Once the complete response has been retrieved, it
 *    is discarded.
 *
 *    Parameter:
 *          par      IN/OUT: The send_targets parameters
 */

static void
send_targets(iscsi_send_targets_parameters_t *par)
{
	int rc;
	uint32_t rlen, cplen;
	session_t *session;

	mutex_enter(&iscsi_cleanup_mtx);
	if ((session = find_session(par->session_id)) == NULL) {
		mutex_exit(&iscsi_cleanup_mtx);
		DEBOUT(("Session %d not found\n", par->session_id));
		par->status = ISCSI_STATUS_INVALID_SESSION_ID;
		return;
	}
	mutex_exit(&iscsi_cleanup_mtx);

	DEB(9, ("ISCSI: send_targets, rsp_size=%d; Saved list: %p\n",
			par->response_size, session->target_list));

	if (session->target_list == NULL) {
		rc = send_send_targets(session, par->key);
		if (rc) {
			par->status = rc;
			return;
		}
	}
	rlen = session->target_list_len;
	par->response_total = rlen;
	cplen = min(par->response_size, rlen);
	if (cplen) {
		copyout(session->target_list, par->response_buffer, cplen);
	}
	par->response_used = cplen;

	/* If all of the response was copied, don't keep it around */
	if (rlen && par->response_used == rlen) {
		free(session->target_list, M_TEMP);
		session->target_list = NULL;
	}

	par->status = ISCSI_STATUS_SUCCESS;
}


/*
 * set_node_name:
 *    Handle the set_node_name ioctl.
 *
 *    Parameter:
 *          par      IN/OUT: The set_node_name parameters
 */

static void
set_node_name(iscsi_set_node_name_parameters_t *par)
{

	if (strlen(par->InitiatorName) >= ISCSI_STRING_LENGTH ||
		strlen(par->InitiatorAlias) >= ISCSI_STRING_LENGTH) {
		DEBOUT(("*** set_node_name string too long!\n"));
		par->status = ISCSI_STATUS_PARAMETER_INVALID;
		return;
	}
	strlcpy(iscsi_InitiatorName, par->InitiatorName, sizeof(iscsi_InitiatorName));
	strlcpy(iscsi_InitiatorAlias, par->InitiatorAlias, sizeof(iscsi_InitiatorAlias));
	memcpy(&iscsi_InitiatorISID, par->ISID, 6);
	DEB(5, ("ISCSI: set_node_name, ISID A=%x, B=%x, C=%x, D=%x\n",
			iscsi_InitiatorISID.ISID_A, iscsi_InitiatorISID.ISID_B,
			iscsi_InitiatorISID.ISID_C, iscsi_InitiatorISID.ISID_D));

	if (!iscsi_InitiatorISID.ISID_A && !iscsi_InitiatorISID.ISID_B &&
		!iscsi_InitiatorISID.ISID_C && !iscsi_InitiatorISID.ISID_D) {
		iscsi_InitiatorISID.ISID_A = T_FORMAT_EN;
		iscsi_InitiatorISID.ISID_B = htons(0x1);
		iscsi_InitiatorISID.ISID_C = 0x37;
		iscsi_InitiatorISID.ISID_D = 0;
	}

	par->status = ISCSI_STATUS_SUCCESS;
}


/*
 * connection_status:
 *    Handle the connection_status ioctl.
 *
 *    Parameter:
 *          par      IN/OUT: The status parameters
 */

static void
connection_status(iscsi_conn_status_parameters_t *par)
{
	connection_t *conn;
	session_t *session;

	mutex_enter(&iscsi_cleanup_mtx);
	if ((session = find_session(par->session_id)) == NULL) {
		mutex_exit(&iscsi_cleanup_mtx);
		par->status = ISCSI_STATUS_INVALID_SESSION_ID;
		return;
	}

	if (par->connection_id) {
		conn = find_connection(session, par->connection_id);
	} else {
		conn = TAILQ_FIRST(&session->conn_list);
	}
	par->status = (conn == NULL) ? ISCSI_STATUS_INVALID_CONNECTION_ID :
					ISCSI_STATUS_SUCCESS;
	mutex_exit(&iscsi_cleanup_mtx);
	DEB(9, ("ISCSI: connection_status, session %d connection %d --> %d\n",
			par->session_id, par->connection_id, par->status));
}


/*
 * get_version:
 *    Handle the get_version ioctl.
 *
 *    Parameter:
 *          par      IN/OUT: The version parameters
 */

static void
get_version(iscsi_get_version_parameters_t *par)
{
	par->status = ISCSI_STATUS_SUCCESS;
	par->interface_version = INTERFACE_VERSION;
	par->major = VERSION_MAJOR;
	par->minor = VERSION_MINOR;
	strlcpy(par->version_string, VERSION_STRING,
		sizeof(par->version_string));
}


/* -------------------------------------------------------------------- */

/*
 * kill_all_sessions:
 *    Terminate all sessions (called when the driver unloads).
 */

int
kill_all_sessions(void)
{
	session_t *sess;
	int rc = 0;

	mutex_enter(&iscsi_cleanup_mtx);
	while ((sess = TAILQ_FIRST(&iscsi_sessions)) != NULL) {
		mutex_exit(&iscsi_cleanup_mtx);
		kill_session(sess, ISCSI_STATUS_DRIVER_UNLOAD, LOGOUT_SESSION,
				FALSE);
		mutex_enter(&iscsi_cleanup_mtx);
	}
	if (TAILQ_FIRST(&iscsi_sessions) != NULL) {
		DEBOUT(("Failed to kill all sessions\n"));
		rc = EBUSY;
	}
	mutex_exit(&iscsi_cleanup_mtx);

	return rc;
}

/*
 * handle_connection_error:
 *    Deal with a problem during send or receive.
 *
 *    Parameter:
 *       conn        The connection the problem is associated with
 *       status      The status code to insert into any unfinished CCBs
 *       dologout    Whether Logout should be attempted
 */

void
handle_connection_error(connection_t *conn, uint32_t status, int dologout)
{

	DEBC(conn, 0, ("*** Connection Error, status=%d, logout=%d, state=%d\n",
				   status, dologout, conn->state));

	if (!conn->terminating && conn->state <= ST_LOGOUT_SENT) {
		/* if we get an error while winding down, escalate it */
		if (dologout >= 0 && conn->state >= ST_WINDING_DOWN) {
			dologout = NO_LOGOUT;
		}
		kill_connection(conn, status, dologout, TRUE);
	}
}

/*
 * remove a connection from session and add to the cleanup list
 */
void
add_connection_cleanup(connection_t *conn)
{
	session_t *sess;

	mutex_enter(&iscsi_cleanup_mtx);
	if (conn->in_session) {
		sess = conn->session;
		conn->in_session = FALSE;
		conn->session = NULL;
		TAILQ_REMOVE(&sess->conn_list, conn, connections);
		sess->mru_connection = TAILQ_FIRST(&sess->conn_list);
	}
	TAILQ_INSERT_TAIL(&iscsi_cleanupc_list, conn, connections);
	iscsi_notify_cleanup();
	mutex_exit(&iscsi_cleanup_mtx);
}

/*
 * callout wrappers for timeouts, the work is done by the cleanup thread
 */
void
connection_timeout_co(void *par)
{
	connection_t *conn = par;

	mutex_enter(&iscsi_cleanup_mtx);
	conn->timedout = TOUT_QUEUED;
	TAILQ_INSERT_TAIL(&iscsi_timeout_conn_list, conn, tchain);
	iscsi_notify_cleanup();
	mutex_exit(&iscsi_cleanup_mtx);
}

void            
connection_timeout_start(connection_t *conn, int ticks)
{
	mutex_enter(&iscsi_cleanup_mtx);
	if (conn->timedout != TOUT_QUEUED) {
		conn->timedout = TOUT_ARMED;
		callout_schedule(&conn->timeout, ticks);
	}
	mutex_exit(&iscsi_cleanup_mtx);
}                           

void                    
connection_timeout_stop(connection_t *conn)
{                                                
	callout_stop(&conn->timeout);
	mutex_enter(&iscsi_cleanup_mtx);
	if (conn->timedout == TOUT_QUEUED) {
		TAILQ_REMOVE(&iscsi_timeout_conn_list, conn, tchain);
		conn->timedout = TOUT_NONE;
	}               
	if (curlwp != iscsi_cleanproc) {
		while (conn->timedout == TOUT_BUSY)
			kpause("connbusy", false, 1, &iscsi_cleanup_mtx);
	}
	mutex_exit(&iscsi_cleanup_mtx);
}                        

void
ccb_timeout_co(void *par)
{
	ccb_t *ccb = par;

	mutex_enter(&iscsi_cleanup_mtx);
	ccb->timedout = TOUT_QUEUED;
	TAILQ_INSERT_TAIL(&iscsi_timeout_ccb_list, ccb, tchain);
	iscsi_notify_cleanup();
	mutex_exit(&iscsi_cleanup_mtx);
}

void    
ccb_timeout_start(ccb_t *ccb, int ticks)
{       
	mutex_enter(&iscsi_cleanup_mtx);
	if (ccb->timedout != TOUT_QUEUED) {
		ccb->timedout = TOUT_ARMED;
		callout_schedule(&ccb->timeout, ticks);
	}
	mutex_exit(&iscsi_cleanup_mtx);
} 
 
void
ccb_timeout_stop(ccb_t *ccb)
{
	callout_stop(&ccb->timeout);
	mutex_enter(&iscsi_cleanup_mtx);
	if (ccb->timedout == TOUT_QUEUED) {
		TAILQ_REMOVE(&iscsi_timeout_ccb_list, ccb, tchain);
		ccb->timedout = TOUT_NONE;
	} 
	if (curlwp != iscsi_cleanproc) {
		while (ccb->timedout == TOUT_BUSY)
			kpause("ccbbusy", false, 1, &iscsi_cleanup_mtx);
	}
	mutex_exit(&iscsi_cleanup_mtx);
}

/*
 * iscsi_cleanup_thread
 *    Global thread to handle connection and session cleanup after termination.
 */

static void
iscsi_cleanup_thread(void *par)
{
	int s, rc;
	session_t *sess, *nxts;
	connection_t *conn, *nxtc;
	ccb_t *ccb;

	mutex_enter(&iscsi_cleanup_mtx);
	while (iscsi_num_send_threads || !iscsi_detaching ||
	       !TAILQ_EMPTY(&iscsi_cleanupc_list) || !TAILQ_EMPTY(&iscsi_cleanups_list)) {
		TAILQ_FOREACH_SAFE(conn, &iscsi_cleanupc_list, connections, nxtc) {

			TAILQ_REMOVE(&iscsi_cleanupc_list, conn, connections);
			mutex_exit(&iscsi_cleanup_mtx);

			sess = conn->session;

			/*
			 * This implies that connection cleanup only runs when
			 * the send/recv threads have been killed
			 */
			DEBC(conn, 5, ("Cleanup: Waiting for threads to exit\n"));
			while (conn->sendproc || conn->rcvproc)
				kpause("threads", false, hz, NULL);

			for (s=1; conn->usecount > 0 && s < 3; ++s)
				kpause("usecount", false, hz, NULL);

			if (conn->usecount > 0) {
				DEBC(conn, 5, ("Cleanup: %d CCBs busy\n", conn->usecount));
				/* retry later */
				mutex_enter(&iscsi_cleanup_mtx);
				TAILQ_INSERT_HEAD(&iscsi_cleanupc_list, conn, connections);
				continue;
			}

			KASSERT(!conn->in_session);

			callout_halt(&conn->timeout, &iscsi_cleanup_mtx);
			closef(conn->sock);
			callout_destroy(&conn->timeout);
			cv_destroy(&conn->idle_cv);
			cv_destroy(&conn->ccb_cv);
			cv_destroy(&conn->pdu_cv);
			cv_destroy(&conn->conn_cv);
			mutex_destroy(&conn->lock);
			free(conn, M_DEVBUF);

			mutex_enter(&iscsi_cleanup_mtx);

			if (--sess->total_connections == 0) {
				DEB(1, ("Cleanup: session %d\n", sess->id));
				if (!sess->terminating) {
					sess->terminating = ISCSI_CONNECTION_TERMINATED;
					KASSERT(sess->sessions.tqe_prev != NULL);
					TAILQ_REMOVE(&iscsi_sessions, sess, sessions);
					sess->sessions.tqe_next = NULL;
					sess->sessions.tqe_prev = NULL;
				}
				KASSERT(sess->sessions.tqe_prev == NULL);
				TAILQ_INSERT_HEAD(&iscsi_cleanups_list, sess, sessions);
			}
		}

		TAILQ_FOREACH_SAFE(sess, &iscsi_cleanups_list, sessions, nxts) {
			if (sess->refcount > 0)
				continue;
			TAILQ_REMOVE(&iscsi_cleanups_list, sess, sessions);
			sess->sessions.tqe_next = NULL;
			sess->sessions.tqe_prev = NULL;
			mutex_exit(&iscsi_cleanup_mtx);

			DEB(1, ("Cleanup: Unmap session %d\n", sess->id));
			if (unmap_session(sess) == 0) {
				DEB(1, ("Cleanup: Unmap session %d failed\n", sess->id));
				mutex_enter(&iscsi_cleanup_mtx);
				TAILQ_INSERT_HEAD(&iscsi_cleanups_list, sess, sessions);
				continue;
			}

			if (sess->target_list != NULL)
				free(sess->target_list, M_TEMP);

			/* notify event handlers of session shutdown */
			add_event(ISCSI_SESSION_TERMINATED, sess->id, 0, sess->terminating);
			DEB(1, ("Cleanup: session ended %d\n", sess->id));

			destroy_ccbs(sess);
			cv_destroy(&sess->ccb_cv);
			cv_destroy(&sess->sess_cv);
			mutex_destroy(&sess->lock);
			free(sess, M_DEVBUF);

			mutex_enter(&iscsi_cleanup_mtx);
		}

		/* handle ccb timeouts */
		while ((ccb = TAILQ_FIRST(&iscsi_timeout_ccb_list)) != NULL) {
			TAILQ_REMOVE(&iscsi_timeout_ccb_list, ccb, tchain);
			KASSERT(ccb->timedout == TOUT_QUEUED);
			ccb->timedout = TOUT_BUSY;
			mutex_exit(&iscsi_cleanup_mtx);
			ccb_timeout(ccb);
			mutex_enter(&iscsi_cleanup_mtx);
			if (ccb->timedout == TOUT_BUSY)
				ccb->timedout = TOUT_NONE;
		}

		/* handle connection timeouts */
		while ((conn = TAILQ_FIRST(&iscsi_timeout_conn_list)) != NULL) {
			TAILQ_REMOVE(&iscsi_timeout_conn_list, conn, tchain);
			KASSERT(conn->timedout == TOUT_QUEUED);
			conn->timedout = TOUT_BUSY;
			mutex_exit(&iscsi_cleanup_mtx);
			connection_timeout(conn);
			mutex_enter(&iscsi_cleanup_mtx);
			if (conn->timedout == TOUT_BUSY)
				conn->timedout = TOUT_NONE;
		}

		/* Go to sleep, but wake up every 30 seconds to
		 * check for dead event handlers */
		rc = cv_timedwait(&iscsi_cleanup_cv, &iscsi_cleanup_mtx,
			(TAILQ_FIRST(&event_handlers)) ? 120 * hz : 0);

		/* if timed out, not woken up */
		if (rc == EWOULDBLOCK)
			check_event_handlers();
	}
	mutex_exit(&iscsi_cleanup_mtx);

	add_event(ISCSI_DRIVER_TERMINATING, 0, 0, ISCSI_STATUS_DRIVER_UNLOAD);

	/*
	 * Wait for all event handlers to deregister, but don't wait more
	 * than 1 minute (assume registering app has died if it takes longer).
	 */
	mutex_enter(&iscsi_cleanup_mtx);
	for (s = 0; TAILQ_FIRST(&event_handlers) != NULL && s < 60; s++)
		kpause("waiteventclr", true, hz, &iscsi_cleanup_mtx);
	mutex_exit(&iscsi_cleanup_mtx);

	iscsi_cleanproc = NULL;
	DEB(5, ("Cleanup thread exits\n"));
	kthread_exit(0);
}

void
iscsi_init_cleanup(void)
{

	mutex_init(&iscsi_cleanup_mtx, MUTEX_DEFAULT, IPL_BIO);
	cv_init(&iscsi_cleanup_cv, "cleanup");
	cv_init(&iscsi_event_cv, "iscsievtwait");

	if (kthread_create(PRI_NONE, KTHREAD_MPSAFE, NULL, iscsi_cleanup_thread,
	    NULL, &iscsi_cleanproc, "iscsi_cleanup") != 0) {
		panic("Can't create cleanup thread!");
	}
}

int
iscsi_destroy_cleanup(void)
{
	
	iscsi_detaching = true;
	mutex_enter(&iscsi_cleanup_mtx);
	while (iscsi_cleanproc != NULL) {
		iscsi_notify_cleanup();
		kpause("detach_wait", false, hz, &iscsi_cleanup_mtx);
	}
	mutex_exit(&iscsi_cleanup_mtx);

	cv_destroy(&iscsi_event_cv);
	cv_destroy(&iscsi_cleanup_cv);
	mutex_destroy(&iscsi_cleanup_mtx);

	return 0;
}

void
iscsi_notify_cleanup(void)
{
	KASSERT(mutex_owned(&iscsi_cleanup_mtx));

	cv_signal(&iscsi_cleanup_cv);
}


/* -------------------------------------------------------------------- */

/*
 * iscsi_ioctl:
 *    Driver ioctl entry.
 *
 *    Parameter:
 *       file     File structure
 *       cmd      The ioctl Command
 *       addr     IN/OUT: The command parameter
 *       flag     Flags (ignored)
 *       l        IN: The lwp object of the caller
 */

int
iscsiioctl(struct file *fp, u_long cmd, void *addr)
{
	struct lwp *l = curlwp;
	struct iscsifd *d = fp->f_iscsi;

	DEB(1, ("ISCSI Ioctl cmd = %x\n", (int) cmd));

	switch (cmd) {
	case ISCSI_GET_VERSION:
		get_version((iscsi_get_version_parameters_t *) addr);
		break;

	case ISCSI_LOGIN:
		login((iscsi_login_parameters_t *) addr, l, d->dev);
		break;

	case ISCSI_ADD_CONNECTION:
		add_connection((iscsi_login_parameters_t *) addr, l);
		break;

	case ISCSI_RESTORE_CONNECTION:
		restore_connection((iscsi_login_parameters_t *) addr, l);
		break;

	case ISCSI_LOGOUT:
		logout((iscsi_logout_parameters_t *) addr);
		break;

	case ISCSI_REMOVE_CONNECTION:
		remove_connection((iscsi_remove_parameters_t *) addr);
		break;

#ifndef ISCSI_MINIMAL
	case ISCSI_IO_COMMAND:
		io_command((iscsi_iocommand_parameters_t *) addr, l);
		break;
#endif

	case ISCSI_SEND_TARGETS:
		send_targets((iscsi_send_targets_parameters_t *) addr);
		break;

	case ISCSI_SET_NODE_NAME:
		set_node_name((iscsi_set_node_name_parameters_t *) addr);
		break;

	case ISCSI_CONNECTION_STATUS:
		connection_status((iscsi_conn_status_parameters_t *) addr);
		break;

	case ISCSI_REGISTER_EVENT:
		register_event((iscsi_register_event_parameters_t *) addr);
		break;

	case ISCSI_DEREGISTER_EVENT:
		deregister_event((iscsi_register_event_parameters_t *) addr);
		break;

	case ISCSI_WAIT_EVENT:
		check_event((iscsi_wait_event_parameters_t *) addr, TRUE);
		break;

	case ISCSI_POLL_EVENT:
		check_event((iscsi_wait_event_parameters_t *) addr, FALSE);
		break;

	default:
		DEBOUT(("Invalid IO-Control Code\n"));
		return ENOTTY;
	}

    /*
     * NOTE: We return 0 even if the function fails as long as the ioctl code
     * is good, so the status code is copied back to the caller.
	 */
	return 0;
}
