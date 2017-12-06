/*	$NetBSD: iscsi_ioctl.c,v 1.29 2017/12/06 04:29:58 ozaki-r Exp $	*/

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

	TAILQ_FOREACH(curr, &event_handlers, evh_link)
		if (curr->evh_id == id)
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

	TAILQ_INIT(&handler->evh_events);

	mutex_enter(&iscsi_cleanup_mtx);
	/* create a unique ID */
	do {
		++handler_id;
	} while (!handler_id || find_handler(handler_id) != NULL);
	par->event_id = handler->evh_id = handler_id;

	was_empty = TAILQ_FIRST(&event_handlers) == NULL;
	TAILQ_INSERT_TAIL(&event_handlers, handler, evh_link);
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

	TAILQ_REMOVE(&event_handlers, handler, evh_link);
	if (handler->evh_waiter != NULL) {
		handler->evh_waiter->status = ISCSI_STATUS_EVENT_DEREGISTERED;
		cv_broadcast(&iscsi_event_cv);
	}

	while ((evt = TAILQ_FIRST(&handler->evh_events)) != NULL) {
		TAILQ_REMOVE(&handler->evh_events, evt, ev_link);
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
	if (handler->evh_waiter != NULL) {
		mutex_exit(&iscsi_cleanup_mtx);
		DEBOUT(("Wait Event ID %d already waiting\n", par->event_id));
		par->status = ISCSI_STATUS_EVENT_WAITING;
		return;
	}
	par->status = ISCSI_STATUS_SUCCESS;
	DEB(99, ("Wait Event ID %d\n", par->event_id));

	do {
		evt = TAILQ_FIRST(&handler->evh_events);
		if (evt != NULL) {
			TAILQ_REMOVE(&handler->evh_events, evt, ev_link);
		} else {
			if (!wait) {
				par->status = ISCSI_STATUS_LIST_EMPTY;
				return;
			}
			if (par->status != ISCSI_STATUS_SUCCESS) {
				return;
			}
			handler->evh_waiter = par;
			rc = cv_wait_sig(&iscsi_event_cv, &iscsi_cleanup_mtx);
			if (rc) {
				mutex_exit(&iscsi_cleanup_mtx);
				par->status = ISCSI_STATUS_LIST_EMPTY;
				return;
			}
		}
	} while (evt == NULL);
	mutex_exit(&iscsi_cleanup_mtx);

	par->connection_id = evt->ev_connection_id;
	par->session_id = evt->ev_session_id;
	par->event_kind = evt->ev_event_kind;
	par->reason = evt->ev_reason;

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
	TAILQ_FOREACH(curr, &event_handlers, evh_link) {
		evt = malloc(sizeof(*evt), M_TEMP, M_NOWAIT);
		if (evt == NULL) {
			DEBOUT(("Cannot allocate event\n"));
			break;
		}
		evt->ev_event_kind = kind;
		evt->ev_session_id = sid;
		evt->ev_connection_id = cid;
		evt->ev_reason = reason;

		TAILQ_INSERT_TAIL(&curr->evh_events, evt, ev_link);
		if (curr->evh_waiter != NULL) {
			curr->evh_waiter = NULL;
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
		next = TAILQ_NEXT(curr, evh_link);
		evt = TAILQ_FIRST(&curr->evh_events);

		if (evt != NULL && evt == curr->evh_first_in_list) {
			DEBOUT(("Found Dead Event Handler %d, removing\n", curr->evh_id));

			TAILQ_REMOVE(&event_handlers, curr, evh_link);
			while ((evt = TAILQ_FIRST(&curr->evh_events)) != NULL) {
				TAILQ_REMOVE(&curr->evh_events, evt, ev_link);
				free(evt, M_TEMP);
			}
			free(curr, M_DEVBUF);
		} else
			curr->evh_first_in_list = evt;
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
	session_t *sess;

	KASSERT(mutex_owned(&iscsi_cleanup_mtx));

	TAILQ_FOREACH(sess, &iscsi_sessions, s_sessions)
		if (sess->s_id == id) {
			break;
		}
	return sess;
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
find_connection(session_t *sess, uint32_t id)
{
	connection_t *conn;

	KASSERT(mutex_owned(&iscsi_cleanup_mtx));

	TAILQ_FOREACH(conn, &sess->s_conn_list, c_connections)
		if (conn->c_id == id) {
			break;
		}
	return conn;
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
ref_session(session_t *sess)
{
	int rc = 1;

	mutex_enter(&iscsi_cleanup_mtx);
	KASSERT(sess != NULL);
	if (sess->s_refcount <= CCBS_PER_SESSION) {
		sess->s_refcount++;
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
	KASSERT(session->s_refcount > 0);
	if (--session->s_refcount == 0)
		cv_broadcast(&session->s_sess_cv);
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
	session_t *sess = conn->c_session;
	int terminating;

	DEBC(conn, 1, ("Kill_connection: terminating=%d, status=%d, logout=%d, "
			   "state=%d\n",
			   conn->c_terminating, status, logout, conn->c_state));

	mutex_enter(&iscsi_cleanup_mtx);
	if (recover &&
	    !conn->c_destroy &&
	    conn->c_recover > MAX_RECOVERY_ATTEMPTS) {
		DEBC(conn, 1,
			  ("Kill_connection: Too many recovery attempts, destroying\n"));
		conn->c_destroy = TRUE;
	}

	if (!recover || conn->c_destroy) {

		if (conn->c_in_session) {
			conn->c_in_session = FALSE;
			TAILQ_REMOVE(&sess->s_conn_list, conn, c_connections);
			sess->s_mru_connection = TAILQ_FIRST(&sess->s_conn_list);
		}

		if (!conn->c_destroy) {
			DEBC(conn, 1, ("Kill_connection setting destroy flag\n"));
			conn->c_destroy = TRUE;
		}
	}

	terminating = conn->c_terminating;
	if (!terminating)
		conn->c_terminating = status;

	/* Don't recurse */
	if (terminating) {
		mutex_exit(&iscsi_cleanup_mtx);

		KASSERT(conn->c_state != ST_FULL_FEATURE);
		DEBC(conn, 1, ("Kill_connection exiting (already terminating)\n"));
		goto done;
	}

	if (conn->c_state == ST_FULL_FEATURE) {
		sess->s_active_connections--;
		conn->c_state = ST_WINDING_DOWN;

		/* If this is the last connection and ERL < 2, reset TSIH */
		if (!sess->s_active_connections && sess->s_ErrorRecoveryLevel < 2)
			sess->s_TSIH = 0;

		/* Don't try to log out if the socket is broken or we're in the middle */
		/* of logging in */
		if (logout >= 0) {
			if (sess->s_ErrorRecoveryLevel < 2 &&
			    logout == RECOVER_CONNECTION) {
				logout = LOGOUT_CONNECTION;
			}
			if (!sess->s_active_connections &&
			    logout == LOGOUT_CONNECTION) {
				logout = LOGOUT_SESSION;
			}
			mutex_exit(&iscsi_cleanup_mtx);

			connection_timeout_start(conn, CONNECTION_TIMEOUT);

			if (!send_logout(conn, conn, logout, FALSE)) {
				conn->c_terminating = ISCSI_STATUS_SUCCESS;
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

	conn->c_state = ST_SETTLING;
	mutex_exit(&iscsi_cleanup_mtx);

done:
	/* let send thread take over next step of cleanup */
	mutex_enter(&conn->c_lock);
	cv_broadcast(&conn->c_conn_cv);
	mutex_exit(&conn->c_lock);

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
kill_session(session_t *sess, uint32_t status, int logout, bool recover)
{
	connection_t *conn;

	DEB(1, ("ISCSI: kill_session %d, status %d, logout %d, recover %d\n",
			sess->s_id, status, logout, recover));

	mutex_enter(&iscsi_cleanup_mtx);
	if (sess->s_terminating) {
		mutex_exit(&iscsi_cleanup_mtx);

		DEB(5, ("Session is being killed with status %d\n",sess->s_terminating));
		return;
	}

	/*
	 * don't do anything if session isn't established yet, termination will be
	 * handled elsewhere
	 */
	if (sess->s_sessions.tqe_next == NULL && sess->s_sessions.tqe_prev == NULL) {
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
		if (sess->s_active_connections == 1) {
			conn = assign_connection(sess, FALSE);
			if (conn != NULL)
				kill_connection(conn, status, logout, TRUE);
		}
		return;
	}

	if (sess->s_refcount > 0) {
		mutex_exit(&iscsi_cleanup_mtx);

		DEB(5, ("Session is being killed while in use (refcnt = %d)\n",
			sess->s_refcount));
		return;
	}

	/* Remove session from global list */
	sess->s_terminating = status;
	TAILQ_REMOVE(&iscsi_sessions, sess, s_sessions);
	sess->s_sessions.tqe_next = NULL;
	sess->s_sessions.tqe_prev = NULL;

	mutex_exit(&iscsi_cleanup_mtx);

	/* kill all connections */
	while ((conn = TAILQ_FIRST(&sess->s_conn_list)) != NULL) {
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
create_connection(iscsi_login_parameters_t *par, session_t *sess,
				  struct lwp *l)
{
	connection_t *conn;
	int rc;

	DEB(1, ("Create Connection for Session %d\n", sess->s_id));

	if (sess->s_MaxConnections &&
	    sess->s_active_connections >= sess->s_MaxConnections) {
		DEBOUT(("Too many connections (max = %d, curr = %d)\n",
				sess->s_MaxConnections, sess->s_active_connections));
		par->status = ISCSI_STATUS_MAXED_CONNECTIONS;
		return EIO;
	}

	conn = malloc(sizeof(*conn), M_DEVBUF, M_WAITOK | M_ZERO);
	if (conn == NULL) {
		DEBOUT(("No mem for connection\n"));
		par->status = ISCSI_STATUS_NO_RESOURCES;
		return EIO;
	}

	mutex_enter(&iscsi_cleanup_mtx);
	/* create a unique ID */
	do {
		++sess->s_conn_id;
	} while (!sess->s_conn_id ||
		 find_connection(sess, sess->s_conn_id) != NULL);
	par->connection_id = conn->c_id = sess->s_conn_id;
	mutex_exit(&iscsi_cleanup_mtx);
	DEB(99, ("Connection ID = %d\n", conn->c_id));

	conn->c_session = sess;

	TAILQ_INIT(&conn->c_ccbs_waiting);
	TAILQ_INIT(&conn->c_pdus_to_send);
	TAILQ_INIT(&conn->c_pdu_pool);

	mutex_init(&conn->c_lock, MUTEX_DEFAULT, IPL_BIO);
	cv_init(&conn->c_conn_cv, "conn");
	cv_init(&conn->c_pdu_cv, "pdupool");
	cv_init(&conn->c_ccb_cv, "ccbwait");
	cv_init(&conn->c_idle_cv, "idle");

	callout_init(&conn->c_timeout, CALLOUT_MPSAFE);
	callout_setfunc(&conn->c_timeout, connection_timeout_co, conn);
	conn->c_idle_timeout_val = CONNECTION_IDLE_TIMEOUT;

	init_sernum(&conn->c_StatSN_buf);
	create_pdus(conn);

	if ((rc = get_socket(par->socket, &conn->c_sock)) != 0) {
		DEBOUT(("Invalid socket %d\n", par->socket));

		callout_destroy(&conn->c_timeout);
		cv_destroy(&conn->c_idle_cv);
		cv_destroy(&conn->c_ccb_cv);
		cv_destroy(&conn->c_pdu_cv);
		cv_destroy(&conn->c_conn_cv);
		mutex_destroy(&conn->c_lock);
		free(conn, M_DEVBUF);
		par->status = ISCSI_STATUS_INVALID_SOCKET;
		return rc;
	}
	DEBC(conn, 1, ("get_socket: par_sock=%d, fdesc=%p\n",
			par->socket, conn->c_sock));

	/* close the file descriptor */
	fd_close(par->socket);

	conn->c_threadobj = l;
	conn->c_login_par = par;

	DEB(5, ("Creating receive thread\n"));
	if ((rc = kthread_create(PRI_BIO, KTHREAD_MPSAFE, NULL, iscsi_rcv_thread,
				conn, &conn->c_rcvproc,
				"ConnRcv")) != 0) {
		DEBOUT(("Can't create rcv thread (rc %d)\n", rc));

		release_socket(conn->c_sock);
		callout_destroy(&conn->c_timeout);
		cv_destroy(&conn->c_idle_cv);
		cv_destroy(&conn->c_ccb_cv);
		cv_destroy(&conn->c_pdu_cv);
		cv_destroy(&conn->c_conn_cv);
		mutex_destroy(&conn->c_lock);
		free(conn, M_DEVBUF);
		par->status = ISCSI_STATUS_NO_RESOURCES;
		return rc;
	}
	DEB(5, ("Creating send thread\n"));
	if ((rc = kthread_create(PRI_BIO, KTHREAD_MPSAFE, NULL, iscsi_send_thread,
				conn, &conn->c_sendproc,
				"ConnSend")) != 0) {
		DEBOUT(("Can't create send thread (rc %d)\n", rc));

		conn->c_terminating = ISCSI_STATUS_NO_RESOURCES;

		/*
		 * We must close the socket here to force the receive
		 * thread to wake up
		 */
		DEBC(conn, 1, ("Closing Socket %p\n", conn->c_sock));
		mutex_enter(&conn->c_sock->f_lock);
		conn->c_sock->f_count += 1;
		mutex_exit(&conn->c_sock->f_lock);
		closef(conn->c_sock);

		/* give receive thread time to exit */
		kpause("settle", false, 2 * hz, NULL);

		release_socket(conn->c_sock);
		callout_destroy(&conn->c_timeout);
		cv_destroy(&conn->c_idle_cv);
		cv_destroy(&conn->c_ccb_cv);
		cv_destroy(&conn->c_pdu_cv);
		cv_destroy(&conn->c_conn_cv);
		mutex_destroy(&conn->c_lock);
		free(conn, M_DEVBUF);
		par->status = ISCSI_STATUS_NO_RESOURCES;
		return rc;
	}

	/*
	 * At this point, each thread will tie 'sock' into its own file descriptor
	 * tables w/o increasing the use count - they will inherit the use
	 * increments performed in get_socket().
	 */

	if ((rc = send_login(conn)) != 0) {
		DEBC(conn, 0, ("Login failed (rc %d)\n", rc));
		/* Don't attempt to recover, there seems to be something amiss */
		kill_connection(conn, rc, NO_LOGOUT, FALSE);
		par->status = rc;
		return -1;
	}

	mutex_enter(&iscsi_cleanup_mtx);
	if (sess->s_terminating) {
		mutex_exit(&iscsi_cleanup_mtx);
		DEBC(conn, 0, ("Session terminating\n"));
		kill_connection(conn, rc, NO_LOGOUT, FALSE);
		par->status = sess->s_terminating;
		return -1;
	}
	conn->c_state = ST_FULL_FEATURE;
	TAILQ_INSERT_TAIL(&sess->s_conn_list, conn, c_connections);
	conn->c_in_session = TRUE;
	sess->s_total_connections++;
	sess->s_active_connections++;
	sess->s_mru_connection = conn;
	mutex_exit(&iscsi_cleanup_mtx);

	DEBC(conn, 5, ("Connection created successfully!\n"));
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
recreate_connection(iscsi_login_parameters_t *par, session_t *sess,
					connection_t *conn, struct lwp *l)
{
	int rc;
	ccb_t *ccb;
	ccb_list_t old_waiting;
	pdu_t *pdu;
	uint32_t sn;

	DEB(1, ("ReCreate Connection %d for Session %d, ERL=%d\n",
		conn->c_id, conn->c_session->s_id,
		conn->c_session->s_ErrorRecoveryLevel));

	if (sess->s_MaxConnections &&
	    sess->s_active_connections >= sess->s_MaxConnections) {
		DEBOUT(("Too many connections (max = %d, curr = %d)\n",
			sess->s_MaxConnections, sess->s_active_connections));
		par->status = ISCSI_STATUS_MAXED_CONNECTIONS;
		return EIO;
	}

	/* close old socket */
	if (conn->c_sock != NULL) {
		closef(conn->c_sock);
		conn->c_sock = NULL;
	}

	if ((rc = get_socket(par->socket, &conn->c_sock)) != 0) {
		DEBOUT(("Invalid socket %d\n", par->socket));
		par->status = ISCSI_STATUS_INVALID_SOCKET;
		return rc;
	}
	DEBC(conn, 1, ("get_socket: par_sock=%d, fdesc=%p\n",
			par->socket, conn->c_sock));

	/* close the file descriptor */
	fd_close(par->socket);

	conn->c_threadobj = l;
	conn->c_login_par = par;
	conn->c_terminating = ISCSI_STATUS_SUCCESS;
	conn->c_recover++;
	conn->c_num_timeouts = 0;
	conn->c_state = ST_SEC_NEG;
	conn->c_HeaderDigest = 0;
	conn->c_DataDigest = 0;

	sess->s_active_connections++;

	TAILQ_INIT(&old_waiting);

	mutex_enter(&conn->c_lock);
	while ((ccb = TAILQ_FIRST(&conn->c_ccbs_waiting)) != NULL) {
		suspend_ccb(ccb, FALSE);
		TAILQ_INSERT_TAIL(&old_waiting, ccb, ccb_chain);
	}
	init_sernum(&conn->c_StatSN_buf);
	cv_broadcast(&conn->c_idle_cv);
	mutex_exit(&conn->c_lock);

	if ((rc = send_login(conn)) != 0) {
		DEBOUT(("Login failed (rc %d)\n", rc));
		while ((ccb = TAILQ_FIRST(&old_waiting)) != NULL) {
			TAILQ_REMOVE(&old_waiting, ccb, ccb_chain);
			wake_ccb(ccb, rc);
		}
		/* Don't attempt to recover, there seems to be something amiss */
		kill_connection(conn, rc, NO_LOGOUT, FALSE);
		par->status = rc;
		return -1;
	}

	DEBC(conn, 9, ("Re-Login successful\n"));
	par->status = ISCSI_STATUS_SUCCESS;

	conn->c_state = ST_FULL_FEATURE;
	sess->s_mru_connection = conn;

	while ((ccb = TAILQ_FIRST(&old_waiting)) != NULL) {
		TAILQ_REMOVE(&old_waiting, ccb, ccb_chain);
		mutex_enter(&conn->c_lock);
		suspend_ccb(ccb, TRUE);
		mutex_exit(&conn->c_lock);

		rc = send_task_management(conn, ccb, NULL, TASK_REASSIGN);
		/* if we get an error on reassign, restart the original request */
		if (rc && ccb->ccb_pdu_waiting != NULL) {
			mutex_enter(&sess->s_lock);
			if (sn_a_lt_b(ccb->ccb_CmdSN, sess->s_ExpCmdSN)) {
				pdu = ccb->ccb_pdu_waiting;
				sn = get_sernum(sess, pdu);

				/* update CmdSN */
				DEBC(conn, 0, ("Resend ccb %p (%d) - updating CmdSN old %u, new %u\n",
					   ccb, rc, ccb->ccb_CmdSN, sn));
				ccb->ccb_CmdSN = sn;
				pdu->pdu_hdr.pduh_p.command.CmdSN = htonl(ccb->ccb_CmdSN);
			} else {
				DEBC(conn, 0, ("Resend ccb %p (%d) - CmdSN %u\n",
					   ccb, rc, ccb->ccb_CmdSN));
			}
			mutex_exit(&sess->s_lock);
			resend_pdu(ccb);
		} else {
			DEBC(conn, 0, ("Resend ccb %p (%d) CmdSN %u - reassigned\n",
				ccb, rc, ccb->ccb_CmdSN));
			ccb_timeout_start(ccb, COMMAND_TIMEOUT);
		}
	}

	mutex_enter(&sess->s_lock);
	cv_broadcast(&sess->s_sess_cv);
	mutex_exit(&sess->s_lock);

	DEBC(conn, 0, ("Connection ReCreated successfully - status %d\n",
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
	session_t *sess;
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
	sess = malloc(sizeof(*sess), M_DEVBUF, M_WAITOK | M_ZERO);
	if (sess == NULL) {
		DEBOUT(("No mem for session\n"));
		par->status = ISCSI_STATUS_NO_RESOURCES;
		return;
	}
	TAILQ_INIT(&sess->s_conn_list);
	TAILQ_INIT(&sess->s_ccb_pool);

	mutex_init(&sess->s_lock, MUTEX_DEFAULT, IPL_BIO);
	cv_init(&sess->s_sess_cv, "session");
	cv_init(&sess->s_ccb_cv, "ccb");

	mutex_enter(&iscsi_cleanup_mtx);
	/* create a unique ID */
	do {
		++current_id;
	} while (!current_id || find_session(current_id) != NULL);
	par->session_id = sess->s_id = current_id;
	mutex_exit(&iscsi_cleanup_mtx);

	create_ccbs(sess);
	sess->s_login_type = par->login_type;
	sess->s_CmdSN = 1;

	if ((rc = create_connection(par, sess, l)) != 0) {
		if (rc > 0) {
			destroy_ccbs(sess);
			cv_destroy(&sess->s_ccb_cv);
			cv_destroy(&sess->s_sess_cv);
			mutex_destroy(&sess->s_lock);
			free(sess, M_DEVBUF);
		}
		return;
	}

	mutex_enter(&iscsi_cleanup_mtx);
	TAILQ_INSERT_HEAD(&iscsi_sessions, sess, s_sessions);
	mutex_exit(&iscsi_cleanup_mtx);

	/* Session established, map LUNs? */
	if (par->login_type == ISCSI_LOGINTYPE_MAP) {
		copyinstr(par->TargetName, sess->s_tgtname,
		    sizeof(sess->s_tgtname), NULL);
		DEB(1, ("Login: map session %d\n", sess->s_id));
		if (!map_session(sess, dev)) {
			DEB(1, ("Login: map session %d failed\n", sess->s_id));
			kill_session(sess, ISCSI_STATUS_MAP_FAILED,
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
	session_t *sess;
	connection_t *conn;

	DEB(1, ("ISCSI: restore_connection %d of session %d\n",
			par->connection_id, par->session_id));

	mutex_enter(&iscsi_cleanup_mtx);
	if ((sess = find_session(par->session_id)) == NULL) {
		mutex_exit(&iscsi_cleanup_mtx);
		DEBOUT(("Session %d not found\n", par->session_id));
		par->status = ISCSI_STATUS_INVALID_SESSION_ID;
		return;
	}

	if ((conn = find_connection(sess, par->connection_id)) == NULL) {
		mutex_exit(&iscsi_cleanup_mtx);
		DEBOUT(("Connection %d not found in session %d\n",
				par->connection_id, par->session_id));
		par->status = ISCSI_STATUS_INVALID_CONNECTION_ID;
		return;
	}
	mutex_exit(&iscsi_cleanup_mtx);

	if ((par->status = check_login_pars(par)) == 0) {
		recreate_connection(par, sess, conn, l);
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
	session_t *sess;

	mutex_enter(&iscsi_cleanup_mtx);
	if ((sess = find_session(par->session_id)) == NULL) {
		mutex_exit(&iscsi_cleanup_mtx);
		DEBOUT(("Session %d not found\n", par->session_id));
		par->status = ISCSI_STATUS_INVALID_SESSION_ID;
		return;
	}
	mutex_exit(&iscsi_cleanup_mtx);

	DEB(9, ("ISCSI: send_targets, rsp_size=%d; Saved list: %p\n",
			par->response_size, sess->s_target_list));

	if (sess->s_target_list == NULL) {
		rc = send_send_targets(sess, par->key);
		if (rc) {
			par->status = rc;
			return;
		}
	}
	rlen = sess->s_target_list_len;
	par->response_total = rlen;
	cplen = min(par->response_size, rlen);
	if (cplen) {
		copyout(sess->s_target_list, par->response_buffer, cplen);
	}
	par->response_used = cplen;

	/* If all of the response was copied, don't keep it around */
	if (rlen && par->response_used == rlen) {
		free(sess->s_target_list, M_TEMP);
		sess->s_target_list = NULL;
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
	session_t *sess;

	mutex_enter(&iscsi_cleanup_mtx);
	if ((sess = find_session(par->session_id)) == NULL) {
		mutex_exit(&iscsi_cleanup_mtx);
		par->status = ISCSI_STATUS_INVALID_SESSION_ID;
		return;
	}

	if (par->connection_id) {
		conn = find_connection(sess, par->connection_id);
	} else {
		conn = TAILQ_FIRST(&sess->s_conn_list);
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
				   status, dologout, conn->c_state));

	if (!conn->c_terminating && conn->c_state <= ST_LOGOUT_SENT) {
		/* if we get an error while winding down, escalate it */
		if (dologout >= 0 && conn->c_state >= ST_WINDING_DOWN) {
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
	if (conn->c_in_session) {
		sess = conn->c_session;
		conn->c_in_session = FALSE;
		conn->c_session = NULL;
		TAILQ_REMOVE(&sess->s_conn_list, conn, c_connections);
		sess->s_mru_connection = TAILQ_FIRST(&sess->s_conn_list);
	}
	TAILQ_INSERT_TAIL(&iscsi_cleanupc_list, conn, c_connections);
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
	conn->c_timedout = TOUT_QUEUED;
	TAILQ_INSERT_TAIL(&iscsi_timeout_conn_list, conn, c_tchain);
	iscsi_notify_cleanup();
	mutex_exit(&iscsi_cleanup_mtx);
}

void            
connection_timeout_start(connection_t *conn, int ticks)
{
	mutex_enter(&iscsi_cleanup_mtx);
	if (conn->c_timedout != TOUT_QUEUED) {
		conn->c_timedout = TOUT_ARMED;
		callout_schedule(&conn->c_timeout, ticks);
	}
	mutex_exit(&iscsi_cleanup_mtx);
}                           

void                    
connection_timeout_stop(connection_t *conn)
{                                                
	callout_stop(&conn->c_timeout);
	mutex_enter(&iscsi_cleanup_mtx);
	if (conn->c_timedout == TOUT_QUEUED) {
		TAILQ_REMOVE(&iscsi_timeout_conn_list, conn, c_tchain);
		conn->c_timedout = TOUT_NONE;
	}               
	if (curlwp != iscsi_cleanproc) {
		while (conn->c_timedout == TOUT_BUSY)
			kpause("connbusy", false, 1, &iscsi_cleanup_mtx);
	}
	mutex_exit(&iscsi_cleanup_mtx);
}                        

void
ccb_timeout_co(void *par)
{
	ccb_t *ccb = par;

	mutex_enter(&iscsi_cleanup_mtx);
	ccb->ccb_timedout = TOUT_QUEUED;
	TAILQ_INSERT_TAIL(&iscsi_timeout_ccb_list, ccb, ccb_tchain);
	iscsi_notify_cleanup();
	mutex_exit(&iscsi_cleanup_mtx);
}

void    
ccb_timeout_start(ccb_t *ccb, int ticks)
{       
	mutex_enter(&iscsi_cleanup_mtx);
	if (ccb->ccb_timedout != TOUT_QUEUED) {
		ccb->ccb_timedout = TOUT_ARMED;
		callout_schedule(&ccb->ccb_timeout, ticks);
	}
	mutex_exit(&iscsi_cleanup_mtx);
} 
 
void
ccb_timeout_stop(ccb_t *ccb)
{
	callout_stop(&ccb->ccb_timeout);
	mutex_enter(&iscsi_cleanup_mtx);
	if (ccb->ccb_timedout == TOUT_QUEUED) {
		TAILQ_REMOVE(&iscsi_timeout_ccb_list, ccb, ccb_tchain);
		ccb->ccb_timedout = TOUT_NONE;
	} 
	if (curlwp != iscsi_cleanproc) {
		while (ccb->ccb_timedout == TOUT_BUSY)
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
		TAILQ_FOREACH_SAFE(conn, &iscsi_cleanupc_list, c_connections, nxtc) {

			TAILQ_REMOVE(&iscsi_cleanupc_list, conn, c_connections);
			mutex_exit(&iscsi_cleanup_mtx);

			sess = conn->c_session;

			/*
			 * This implies that connection cleanup only runs when
			 * the send/recv threads have been killed
			 */
			DEBC(conn, 5, ("Cleanup: Waiting for threads to exit\n"));
			while (conn->c_sendproc || conn->c_rcvproc)
				kpause("threads", false, hz, NULL);

			for (s=1; conn->c_usecount > 0 && s < 3; ++s)
				kpause("usecount", false, hz, NULL);

			if (conn->c_usecount > 0) {
				DEBC(conn, 5, ("Cleanup: %d CCBs busy\n", conn->c_usecount));
				/* retry later */
				mutex_enter(&iscsi_cleanup_mtx);
				TAILQ_INSERT_HEAD(&iscsi_cleanupc_list, conn, c_connections);
				continue;
			}

			KASSERT(!conn->c_in_session);

			callout_halt(&conn->c_timeout, &iscsi_cleanup_mtx);
			closef(conn->c_sock);
			callout_destroy(&conn->c_timeout);
			cv_destroy(&conn->c_idle_cv);
			cv_destroy(&conn->c_ccb_cv);
			cv_destroy(&conn->c_pdu_cv);
			cv_destroy(&conn->c_conn_cv);
			mutex_destroy(&conn->c_lock);
			free(conn, M_DEVBUF);

			mutex_enter(&iscsi_cleanup_mtx);

			if (--sess->s_total_connections == 0) {
				DEB(1, ("Cleanup: session %d\n", sess->s_id));
				if (!sess->s_terminating) {
					sess->s_terminating = ISCSI_CONNECTION_TERMINATED;
					KASSERT(sess->s_sessions.tqe_prev != NULL);
					TAILQ_REMOVE(&iscsi_sessions, sess, s_sessions);
					sess->s_sessions.tqe_next = NULL;
					sess->s_sessions.tqe_prev = NULL;
				}
				KASSERT(sess->s_sessions.tqe_prev == NULL);
				TAILQ_INSERT_HEAD(&iscsi_cleanups_list, sess, s_sessions);
			}
		}

		TAILQ_FOREACH_SAFE(sess, &iscsi_cleanups_list, s_sessions, nxts) {
			if (sess->s_refcount > 0)
				continue;
			TAILQ_REMOVE(&iscsi_cleanups_list, sess, s_sessions);
			sess->s_sessions.tqe_next = NULL;
			sess->s_sessions.tqe_prev = NULL;
			mutex_exit(&iscsi_cleanup_mtx);

			DEB(1, ("Cleanup: Unmap session %d\n", sess->s_id));
			if (unmap_session(sess) == 0) {
				DEB(1, ("Cleanup: Unmap session %d failed\n", sess->s_id));
				mutex_enter(&iscsi_cleanup_mtx);
				TAILQ_INSERT_HEAD(&iscsi_cleanups_list, sess, s_sessions);
				continue;
			}

			if (sess->s_target_list != NULL)
				free(sess->s_target_list, M_TEMP);

			/* notify event handlers of session shutdown */
			add_event(ISCSI_SESSION_TERMINATED, sess->s_id, 0, sess->s_terminating);
			DEB(1, ("Cleanup: session ended %d\n", sess->s_id));

			destroy_ccbs(sess);
			cv_destroy(&sess->s_ccb_cv);
			cv_destroy(&sess->s_sess_cv);
			mutex_destroy(&sess->s_lock);
			free(sess, M_DEVBUF);

			mutex_enter(&iscsi_cleanup_mtx);
		}

		/* handle ccb timeouts */
		while ((ccb = TAILQ_FIRST(&iscsi_timeout_ccb_list)) != NULL) {
			TAILQ_REMOVE(&iscsi_timeout_ccb_list, ccb, ccb_tchain);
			KASSERT(ccb->ccb_timedout == TOUT_QUEUED);
			ccb->ccb_timedout = TOUT_BUSY;
			mutex_exit(&iscsi_cleanup_mtx);
			ccb_timeout(ccb);
			mutex_enter(&iscsi_cleanup_mtx);
			if (ccb->ccb_timedout == TOUT_BUSY)
				ccb->ccb_timedout = TOUT_NONE;
		}

		/* handle connection timeouts */
		while ((conn = TAILQ_FIRST(&iscsi_timeout_conn_list)) != NULL) {
			TAILQ_REMOVE(&iscsi_timeout_conn_list, conn, c_tchain);
			KASSERT(conn->c_timedout == TOUT_QUEUED);
			conn->c_timedout = TOUT_BUSY;
			mutex_exit(&iscsi_cleanup_mtx);
			connection_timeout(conn);
			mutex_enter(&iscsi_cleanup_mtx);
			if (conn->c_timedout == TOUT_BUSY)
				conn->c_timedout = TOUT_NONE;
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
		login((iscsi_login_parameters_t *) addr, l, d->fd_dev);
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
