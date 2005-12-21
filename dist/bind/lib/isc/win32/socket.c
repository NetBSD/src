/*	$NetBSD: socket.c,v 1.1.1.4 2005/12/21 23:17:47 christos Exp $	*/

/*
 * Copyright (C) 2004, 2005  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 2000-2003  Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
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

/* Id: socket.c,v 1.5.2.13.2.16 2005/09/01 03:16:12 marka Exp */

/* This code has been rewritten to take advantage of Windows Sockets
 * I/O Completion Ports and Events. I/O Completion Ports is ONLY
 * available on Windows NT, Windows 2000 and Windows XP series of
 * the Windows Operating Systems. In CANNOT run on Windows 95, Windows 98
 * or the follow-ons to those Systems.
 *
 * This code is by nature multithreaded and takes advantage of various
 * features to pass on information through the completion port for
 * when I/O is completed.  All sends and receives are completed through
 * the completion port. Due to an implementation bug in Windows 2000,
 * Service Pack 2 must installed on the system for this code to run correctly.
 * For details on this problem see Knowledge base article Q263823.
 * The code checks for this. The number of Completion Port Worker threads
 * used is the total number of CPU's + 1. This increases the likelihood that
 * a Worker Thread is available for processing a completed request.
 *
 * All accepts and connects are accomplished through the WSAEventSelect()
 * function and the event_wait loop. Events are added to and deleted from
 * each event_wait thread via a common event_update stack owned by the socket
 * manager. If the event_wait thread runs out of array space in the events
 * array it will look for another event_wait thread to add the event. If it
 * fails to find another one it will create a new thread to handle the
 * outstanding event.
 *
 * A future enhancement is to use AcceptEx to take avantage of Overlapped
 * I/O which allows for enhanced performance of TCP connections.
 * This will also reduce the number of events that are waited on by the
 * event_wait threads to just the connect sockets and reduce the number
 * additional threads required.
 *
 * XXXPDM 5 August, 2002
 */

#define MAKE_EXTERNAL 1
#include <config.h>

#include <sys/types.h>

#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_   /* Prevent inclusion of winsock.h in windows.h */
#endif

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <io.h>
#include <fcntl.h>
#include <process.h>

#include <isc/buffer.h>
#include <isc/bufferlist.h>
#include <isc/condition.h>
#include <isc/list.h>
#include <isc/log.h>
#include <isc/mem.h>
#include <isc/msgs.h>
#include <isc/mutex.h>
#include <isc/net.h>
#include <isc/os.h>
#include <isc/platform.h>
#include <isc/print.h>
#include <isc/region.h>
#include <isc/socket.h>
#include <isc/strerror.h>
#include <isc/syslog.h>
#include <isc/task.h>
#include <isc/thread.h>
#include <isc/util.h>
#include <isc/win32os.h>

#include "errno2result.h"

/*
 * Define this macro to control the behavior of connection
 * resets on UDP sockets.  See Microsoft KnowledgeBase Article Q263823
 * for details.
 * NOTE: This requires that Windows 2000 systems install Service Pack 2
 * or later.
 */
#ifndef SIO_UDP_CONNRESET
#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR,12)
#endif

/*
 * Some systems define the socket length argument as an int, some as size_t,
 * some as socklen_t.  This is here so it can be easily changed if needed.
 */
#ifndef ISC_SOCKADDR_LEN_T
#define ISC_SOCKADDR_LEN_T unsigned int
#endif

/*
 * Define what the possible "soft" errors can be.  These are non-fatal returns
 * of various network related functions, like recv() and so on.
 */
#define SOFT_ERROR(e)	((e) == WSAEINTR || \
			 (e) == WSAEWOULDBLOCK || \
			 (e) == EWOULDBLOCK || \
			 (e) == EINTR || \
			 (e) == EAGAIN || \
			 (e) == 0)

/*
 * Pending errors are not really errors and should be
 * kept separate
 */
#define PENDING_ERROR(e) ((e) == WSA_IO_PENDING || (e) == 0)

#define DOIO_SUCCESS	  0       /* i/o ok, event sent */
#define DOIO_SOFT	     1       /* i/o ok, soft error, no event sent */
#define DOIO_HARD	     2       /* i/o error, event sent */
#define DOIO_EOF	      3       /* EOF, no event sent */
#define DOIO_PENDING	  4       /* status when i/o is in process */

#define DLVL(x) ISC_LOGCATEGORY_GENERAL, ISC_LOGMODULE_SOCKET, ISC_LOG_DEBUG(x)

/*
 * DLVL(90)  --  Function entry/exit and other tracing.
 * DLVL(70)  --  Socket "correctness" -- including returning of events, etc.
 * DLVL(60)  --  Socket data send/receive
 * DLVL(50)  --  Event tracing, including receiving/sending completion events.
 * DLVL(20)  --  Socket creation/destruction.
 */
#define TRACE_LEVEL		90
#define CORRECTNESS_LEVEL	70
#define IOEVENT_LEVEL		60
#define EVENT_LEVEL		50
#define CREATION_LEVEL		20

#define TRACE		DLVL(TRACE_LEVEL)
#define CORRECTNESS	DLVL(CORRECTNESS_LEVEL)
#define IOEVENT		DLVL(IOEVENT_LEVEL)
#define EVENT		DLVL(EVENT_LEVEL)
#define CREATION	DLVL(CREATION_LEVEL)

typedef isc_event_t intev_t;

#define SOCKET_MAGIC		ISC_MAGIC('I', 'O', 'i', 'o')
#define VALID_SOCKET(t)		ISC_MAGIC_VALID(t, SOCKET_MAGIC)

/*
 * IPv6 control information.  If the socket is an IPv6 socket we want
 * to collect the destination address and interface so the client can
 * set them on outgoing packets.
 */
#ifdef ISC_PLATFORM_HAVEIPV6
#ifndef USE_CMSG
#define USE_CMSG	1
#endif
#endif

/*
 * We really  don't want to try and use these control messages. Win32
 * doesn't have this mechanism before XP.
 */
#undef USE_CMSG

/*
 * Message header for recvmsg and sendmsg calls.
 * Used value-result for recvmsg, value only for sendmsg.
 */


struct msghdr {
	void	*msg_name;		/* optional address */
	u_int   msg_namelen;		/* size of address */
	WSABUF	*msg_iov;		/* scatter/gather array */
	u_int   msg_iovlen;		/* # elements in msg_iov */
	void	*msg_control;		/* ancillary data, see below */
	u_int   msg_controllen;		/* ancillary data buffer len */
	int     msg_flags;		/* flags on received message */
	int	msg_totallen;		/* total length of this message */
} msghdr;

/*
 * The number of times a send operation is repeated if the result
 * is WSAEINTR.
 */
#define NRETRIES 10

struct isc_socket {
	/* Not locked. */
	unsigned int		magic;
	isc_socketmgr_t	       *manager;
	isc_mutex_t		lock;
	isc_sockettype_t	type;
	OVERLAPPED		overlapped;
	/* Pointers to scatter/gather buffers */
	WSABUF			iov[ISC_SOCKET_MAXSCATTERGATHER];
	WSAEVENT		hEvent;		/* Event Handle */
	long			wait_type;	/* Events to wait on */
	WSAEVENT		hAlert;		/* Alert Event Handle */
	DWORD			evthread_id;	/* Event Thread Id for socket */

	/* Locked by socket lock. */
	ISC_LINK(isc_socket_t)	link;
	unsigned int		references;
	SOCKET			fd;
	int			pf;

	ISC_LIST(isc_socketevent_t)		send_list;
	ISC_LIST(isc_socketevent_t)		recv_list;
	ISC_LIST(isc_socket_newconnev_t)	accept_list;
	isc_socket_connev_t		       *connect_ev;

	/*
	 * Internal events.  Posted when a descriptor is readable or
	 * writable.  These are statically allocated and never freed.
	 * They will be set to non-purgable before use.
	 */
	intev_t			readable_ev;
	intev_t			writable_ev;

	isc_sockaddr_t		address;  /* remote address */

	unsigned int		pending_close : 1,
				pending_accept : 1,
				iocp : 1,	/* I/O Completion Port */
				listener : 1,	/* listener socket */
				connected : 1,
				connecting : 1, /* connect pending */
				bound : 1,	/* bound to local addr */
				pending_free: 1;
	unsigned int		pending_recv;
	unsigned int		pending_send;
};

/*
 * I/O Completion ports Info structures
 */

static HANDLE hHeapHandle = NULL;
static int iocp_total = 0;
typedef struct IoCompletionInfo {
	OVERLAPPED			overlapped;
	isc_socketevent_t		*dev;
	int				request_type;
	struct msghdr			messagehdr;
} IoCompletionInfo;

/*
 * Define a maximum number of I/O Completion Port worker threads
 * to handle the load on the Completion Port. The actual number
 * used is the number of CPU's + 1.
 */
#define MAX_IOCPTHREADS 20

/*
 * event_change structure to handle adds and deletes from the list of
 * events in the Wait
 */
typedef struct event_change event_change_t;

struct event_change {
	isc_socket_t			*sock;
	WSAEVENT			hEvent;
	DWORD				evthread_id;
	SOCKET				fd;
	unsigned int			action;
	ISC_LINK(event_change_t)	link;
};

/*
 * Note: We are using an array here since *WaitForMultiple* wants an array
 * WARNING: This value may not be greater than 64 since the
 * WSAWaitForMultipleEvents function is limited to 64 events.
 */

#define MAX_EVENTS 64

/*
 * List of events being waited on and their associated sockets
 */
typedef struct sock_event_list {
	int max_event;
	int total_events;
	isc_socket_t			*aSockList[MAX_EVENTS];
	WSAEVENT			aEventList[MAX_EVENTS];
} sock_event_list;

/*
 * Thread Event structure for managing the threads handling events
 */
typedef struct events_thread events_thread_t;

struct events_thread {
	isc_thread_t			thread_handle;	/* Thread's handle */
	DWORD				thread_id;	/* Thread's id */
	sock_event_list			sockev_list;
	isc_socketmgr_t			*manager;
	ISC_LINK(events_thread_t)	link;
};

#define SOCKET_MANAGER_MAGIC	ISC_MAGIC('I', 'O', 'm', 'g')
#define VALID_MANAGER(m)	ISC_MAGIC_VALID(m, SOCKET_MANAGER_MAGIC)

struct isc_socketmgr {
	/* Not locked. */
	unsigned int			magic;
	isc_mem_t		       *mctx;
	isc_mutex_t			lock;
	/* Locked by manager lock. */
	ISC_LIST(event_change_t)	event_updates;
	ISC_LIST(isc_socket_t)		socklist;
	int				event_written;
	WSAEVENT			prime_alert;
	isc_boolean_t			bShutdown;
	ISC_LIST(events_thread_t)	ev_threads;
	isc_condition_t			shutdown_ok;
	HANDLE				hIoCompletionPort;
	int				maxIOCPThreads;
	HANDLE				hIOCPThreads[MAX_IOCPTHREADS];
	DWORD				dwIOCPThreadIds[MAX_IOCPTHREADS];
};

/*
 * send() and recv() iovec counts
 */
#define MAXSCATTERGATHER_SEND	(ISC_SOCKET_MAXSCATTERGATHER)
#define MAXSCATTERGATHER_RECV	(ISC_SOCKET_MAXSCATTERGATHER)

static isc_threadresult_t WINAPI event_wait(void *uap);
static isc_threadresult_t WINAPI SocketIoThread(LPVOID ThreadContext);
static void free_socket(isc_socket_t **);

enum {
	SOCKET_RECV,
	SOCKET_SEND,
};

enum {
	EVENT_ADD,
	EVENT_DELETE
};

#if defined(ISC_SOCKET_DEBUG)
/*
 * This is used to dump the contents of the sock structure
 * You should make sure that the sock is locked before
 * dumping it. Since the code uses simple printf() statements
 * it should only be used interactively.
 */
void
sock_dump(isc_socket_t *sock) {
	isc_socketevent_t *ldev;
	isc_socket_newconnev_t *ndev;
	isc_sockaddr_t addr;
	char socktext[256];


	isc_socket_getpeername(sock, &addr);
	isc_sockaddr_format(&addr, socktext, sizeof(socktext));
	printf("Remote Socket: %s\n", socktext);
	isc_socket_getsockname(sock, &addr);
	isc_sockaddr_format(&addr, socktext, sizeof(socktext));
	printf("This Socket: %s\n", socktext);

	printf("\n\t\tSock Dump\n");
	printf("\t\tfd: %u\n", sock->fd);
	printf("\t\treferences: %d\n", sock->references);
	printf("\t\tpending_accept: %d\n", sock->pending_accept);
	printf("\t\tpending_close: %d\n", sock->pending_close);
	printf("\t\tconnecting: %d\n", sock->connecting);
	printf("\t\tconnected: %d\n", sock->connected);
	printf("\t\tbound: %d\n", sock->bound);
	printf("\t\tiocp: %d\n", sock->iocp);
	printf("\t\tsocket type: %d\n", sock->type);

	printf("\n\t\tSock Recv List\n");
	ldev = ISC_LIST_HEAD(sock->recv_list);
	while (ldev != NULL) {
		printf("\t\tdev: %p\n", ldev);
		ldev = ISC_LIST_NEXT(ldev, ev_link);
	}
	printf("\n\t\tSock Send List\n");
	ldev = ISC_LIST_HEAD(sock->send_list);
	while (ldev != NULL) {
		printf("\t\tdev: %p\n", ldev);
		ldev = ISC_LIST_NEXT(ldev, ev_link);
	}
	printf("\n\t\tSock Accept List\n");
	ndev = ISC_LIST_HEAD(sock->accept_list);
	while (ndev != NULL) {
		printf("\t\tdev: %p\n", ldev);
		ndev = ISC_LIST_NEXT(ndev, ev_link);
	}
}
#endif

/*  This function will add an entry to the I/O completion port
 *  that will signal the I/O thread to exit (gracefully)
 */
static void
signal_iocompletionport_exit(isc_socketmgr_t *manager) {
	int i;
	int errval;
	char strbuf[ISC_STRERRORSIZE];

	REQUIRE(VALID_MANAGER(manager));
	for (i = 0; i < manager->maxIOCPThreads; i++) {
		if (!PostQueuedCompletionStatus(manager->hIoCompletionPort,
						0, 0, 0)) {
			errval = GetLastError();
			isc__strerror(errval, strbuf, sizeof(strbuf));
			FATAL_ERROR(__FILE__, __LINE__,
				isc_msgcat_get(isc_msgcat, ISC_MSGSET_SOCKET,
				ISC_MSG_FAILED,
				"Can't request service thread to exit: %s"),
				strbuf);
		}
	}
}

/*
 * Create the worker threads for the I/O Completion Port
 */
void
iocompletionport_createthreads(int total_threads, isc_socketmgr_t *manager) {
	int errval;
	char strbuf[ISC_STRERRORSIZE];
	int i;

	INSIST(total_threads > 0);
	REQUIRE(VALID_MANAGER(manager));
	/*
	 * We need at least one
	 */
	for (i = 0; i < total_threads; i++) {
		manager->hIOCPThreads[i] = CreateThread(NULL, 0, SocketIoThread,
						manager, 0,
						&manager->dwIOCPThreadIds[i]);
		if(manager->hIOCPThreads[i] == NULL) {
			errval = GetLastError();
			isc__strerror(errval, strbuf, sizeof(strbuf));
			FATAL_ERROR(__FILE__, __LINE__,
				isc_msgcat_get(isc_msgcat, ISC_MSGSET_SOCKET,
				ISC_MSG_FAILED,
				"Can't create IOCP thread: %s"),
				strbuf);
		}
	}
}

/*
 *  Create/initialise the I/O completion port
 */
void
iocompletionport_init(isc_socketmgr_t *manager) {
	int errval;
	char strbuf[ISC_STRERRORSIZE];

	REQUIRE(VALID_MANAGER(manager));
	/*
	 * Create a private heap to handle the socket overlapped structure
	 * The miniumum number of structures is 10, there is no maximum
	 */
	hHeapHandle = HeapCreate(0, 10*sizeof(IoCompletionInfo), 0);
	manager->maxIOCPThreads = min(isc_os_ncpus() + 1,
					MAX_IOCPTHREADS);

	/* Now Create the Completion Port */
	manager->hIoCompletionPort = CreateIoCompletionPort(
				     INVALID_HANDLE_VALUE, NULL,
				     0, manager->maxIOCPThreads);
	if (manager->hIoCompletionPort == NULL) {
		errval = GetLastError();
		isc__strerror(errval, strbuf, sizeof(strbuf));
		FATAL_ERROR(__FILE__, __LINE__,
				isc_msgcat_get(isc_msgcat, ISC_MSGSET_SOCKET,
				ISC_MSG_FAILED,
				"CreateIoCompletionPort() failed "
				"during initialization: %s"),
				strbuf);
		exit(1);
	}

	/*
	 * Worker threads for servicing the I/O
 	 */
	iocompletionport_createthreads(manager->maxIOCPThreads, manager);
}


void
iocompletionport_exit(isc_socketmgr_t *manager) {

	REQUIRE(VALID_MANAGER(manager));
	if (manager->hIoCompletionPort != NULL) {
		/*  Get each of the service threads to exit
		*/
		signal_iocompletionport_exit(manager);
	}
}

/*
 * Add sockets in here and pass the sock data in as part of the
 * information needed.
 */
void
iocompletionport_update(isc_socket_t *sock) {
	HANDLE hiocp;

	REQUIRE(sock != NULL);
	if(sock->iocp == 0) {
		sock->iocp = 1;
		hiocp = CreateIoCompletionPort((HANDLE) sock->fd,
			sock->manager->hIoCompletionPort, (DWORD) sock,
			sock->manager->maxIOCPThreads);
		InterlockedIncrement(&iocp_total);

	}
}

void
socket_event_minit(sock_event_list *evlist) {
	BOOL bReset;
	int i;

	REQUIRE(evlist != NULL);
	/* Initialize the Event List */
	evlist->max_event = 0;
	evlist->total_events = 0;
	for (i = 0; i < MAX_EVENTS; i++) {
		evlist->aSockList[i] = NULL;
		evlist->aEventList[i] = (WSAEVENT) 0;
	}

	evlist->aEventList[0] = WSACreateEvent();
	(evlist->max_event)++;
	bReset = WSAResetEvent(evlist->aEventList[0]);
}
/*
 * Event Thread Initialization
 */
isc_result_t
event_thread_create(events_thread_t **evthreadp, isc_socketmgr_t *manager) {
	events_thread_t *evthread;

	REQUIRE(VALID_MANAGER(manager));
	REQUIRE(evthreadp != NULL && *evthreadp == NULL);

	evthread = isc_mem_get(manager->mctx, sizeof(*evthread));
	socket_event_minit(&evthread->sockev_list);
	ISC_LINK_INIT(evthread, link);
	evthread->manager = manager;

	ISC_LIST_APPEND(manager->ev_threads, evthread, link);

	/*
	 * Start up the event wait thread.
	 */
	if (isc_thread_create(event_wait, evthread, &evthread->thread_handle) !=
	    ISC_R_SUCCESS) {
		isc_mem_put(manager->mctx, evthread, sizeof(*evthread));
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "isc_thread_create() %s",
				 isc_msgcat_get(isc_msgcat, ISC_MSGSET_GENERAL,
						ISC_MSG_FAILED, "failed"));
		return (ISC_R_UNEXPECTED);
	}
	*evthreadp = evthread;
	return (ISC_R_SUCCESS);
}
/*
 * Locate a thread with space for additional events or create one if
 * necessary. The manager is locked at this point so the information
 * cannot be changed by another thread while we are searching.
 */
void
locate_available_thread(isc_socketmgr_t *manager) {
	events_thread_t *evthread;
	DWORD threadid = GetCurrentThreadId();

	evthread = ISC_LIST_HEAD(manager->ev_threads);
	while (evthread != NULL) {
		/*
		 * We need to find a thread with space to add an event
		 * If we find it, alert it to process the event change
		 * list
		 */
		if(threadid != evthread->thread_id &&
			evthread->sockev_list.max_event < MAX_EVENTS) {
			WSASetEvent(evthread->sockev_list.aEventList[0]);
			return;
		}
		evthread = ISC_LIST_NEXT(evthread, link);
	}
	/*
	 * We need to create a new thread as other threads are full.
	 * If we succeed in creating the thread, alert it to
	 * process the event change list since it will have space.
	 * If we are unable to create one, the event will stay on the
	 * list and the next event_wait thread will try again to add
	 * the event. It will call here again if it has no space.
	 */
	if (event_thread_create(&evthread, manager) == ISC_R_SUCCESS) {
		WSASetEvent(evthread->sockev_list.aEventList[0]);
	}

}

isc_boolean_t
socket_eventlist_add(event_change_t *evchange, sock_event_list *evlist,
		     isc_socketmgr_t *manager) {
	int max_event;
	isc_socket_t *sock;
	REQUIRE(evchange != NULL);

	sock = evchange->sock;
	REQUIRE(sock != NULL);
	REQUIRE(sock->hEvent != NULL);
	REQUIRE(evlist != NULL);

	max_event = evlist->max_event;
	if(max_event >= MAX_EVENTS) {
		locate_available_thread(manager);
		return (ISC_FALSE);
	}

	evlist->aSockList[max_event] = sock;
	evlist->aEventList[max_event] = sock->hEvent;
	evlist->max_event++;
	evlist->total_events++;
	sock->hAlert = evlist->aEventList[0];
	sock->evthread_id = GetCurrentThreadId();
	return (ISC_TRUE);
}

/*
 * Note that the eventLock is locked before calling this function.
 * All Events and associated sockets are closed here.
 */
isc_boolean_t
socket_eventlist_delete(event_change_t *evchange, sock_event_list *evlist) {
	int i;
	WSAEVENT hEvent;
	int iEvent = -1;

	REQUIRE(evchange != NULL);
	/*  Make sure this is the right thread from which to delete the event */
	if (evchange->evthread_id != GetCurrentThreadId())
		return (ISC_FALSE);

	REQUIRE(evlist != NULL);
	REQUIRE(evchange->hEvent != NULL);
	hEvent = evchange->hEvent;

	/* Find the Event */
	for (i = 1; i < evlist->max_event; i++) {
		if (evlist->aEventList[i] == hEvent) {
			iEvent = i;
			break;
		}
	}

	/* Actual event start at 1 */
	if (iEvent < 1)
		return (ISC_FALSE);

	for(i = iEvent; i < (evlist->max_event - 1); i++) {
		evlist->aEventList[i] = evlist->aEventList[i + 1];
		evlist->aSockList[i] = evlist->aSockList[i + 1];
	}

	evlist->aEventList[evlist->max_event - 1] = 0;
	evlist->aSockList[evlist->max_event - 1] = NULL;

	/* Cleanup */
	WSACloseEvent(hEvent);
	if (evchange->fd >= 0)
		closesocket(evchange->fd);
	evlist->max_event--;
	evlist->total_events--;

	return (ISC_TRUE);
}

/*
 * Get the event changes off of the list and apply the
 * requested changes. The manager lock is taken out at
 * the start of this function to prevent other event_wait
 * threads processing the same information at the same
 * time. The queue may not be empty on exit since other
 * threads may be involved in processing the queue.
 *
 * The deletes are done first in order that there be space
 * available for the events being added in the same thread
 * in case the event list is almost full. This reduces the
 * probability of having to create another thread which would
 * increase overhead costs.
 */
isc_result_t
process_eventlist(sock_event_list *evlist, isc_socketmgr_t *manager) {
	event_change_t *evchange;
	event_change_t *next;
	isc_boolean_t del;

	REQUIRE(evlist != NULL);

	LOCK(&manager->lock);

	/*
	 * First the deletes.
	 */
	evchange = ISC_LIST_HEAD(manager->event_updates);
	while (evchange != NULL) {
		next = ISC_LIST_NEXT(evchange, link);
		del = ISC_FALSE;
		if (evchange->action == EVENT_DELETE) {
			del = socket_eventlist_delete(evchange, evlist);

			/*
			 * Delete only if this thread's socket list was
			 * updated.
			 */
			if (del) {
				ISC_LIST_DEQUEUE(manager->event_updates,
						 evchange, link);
				HeapFree(hHeapHandle, 0, evchange);
				manager->event_written--;
			}
		}
		evchange = next;
	}

	/*
	 * Now the adds.
	 */
	evchange = ISC_LIST_HEAD(manager->event_updates);
	while (evchange != NULL) {
		next = ISC_LIST_NEXT(evchange, link);
		del = ISC_FALSE;
		if (evchange->action == EVENT_ADD) {
			del = socket_eventlist_add(evchange, evlist, manager);

			/*
			 * Delete only if this thread's socket list was
			 * updated.
			 */
			if (del) {
				ISC_LIST_DEQUEUE(manager->event_updates,
						 evchange, link);
				HeapFree(hHeapHandle, 0, evchange);
				manager->event_written--;
			}
		}
		evchange = next;
	}
	UNLOCK(&manager->lock);
	return (ISC_R_SUCCESS);
}

/*
 * Add the event list changes to the queue and notify the
 * event loop
 */
static void
notify_eventlist(isc_socket_t *sock, isc_socketmgr_t *manager,
		 unsigned int action)
{

	event_change_t *evchange;

	REQUIRE(VALID_MANAGER(manager));
	REQUIRE(sock != NULL);

	evchange = HeapAlloc(hHeapHandle, HEAP_ZERO_MEMORY,
			     sizeof(event_change_t));
	evchange->sock = sock;
	evchange->action = action;
	evchange->hEvent = sock->hEvent;
	evchange->fd = sock->fd;
	evchange->evthread_id = sock->evthread_id;

	LOCK(&manager->lock);
	ISC_LIST_APPEND(manager->event_updates, evchange, link);
	sock->manager->event_written++;
	UNLOCK(&manager->lock);

	/* Alert the Wait List */
	if (sock->hAlert != NULL)
		WSASetEvent(sock->hAlert);
	else
		WSASetEvent(manager->prime_alert);
}

/*
 * Note that the socket is already locked before calling this function
 */
isc_result_t
socket_event_add(isc_socket_t *sock, long type) {
	int stat;
	WSAEVENT hEvent;
	char strbuf[ISC_STRERRORSIZE];
	const char *msg;

	REQUIRE(sock != NULL);

	hEvent = WSACreateEvent();
	if (hEvent == WSA_INVALID_EVENT) {
		stat = WSAGetLastError();
		isc__strerror(stat, strbuf, sizeof(strbuf));
		msg = isc_msgcat_get(isc_msgcat, ISC_MSGSET_GENERAL,
				     ISC_MSG_FAILED, "failed"),
		UNEXPECTED_ERROR(__FILE__, __LINE__, "WSACreateEvent: %s: %s",
				 msg, strbuf);
		return (ISC_R_UNEXPECTED);
	}
	if (WSAEventSelect(sock->fd, hEvent, type) != 0) {
		stat = WSAGetLastError();
		isc__strerror(stat, strbuf, sizeof(strbuf));
		msg = isc_msgcat_get(isc_msgcat, ISC_MSGSET_GENERAL,
				     ISC_MSG_FAILED, "failed");
		UNEXPECTED_ERROR(__FILE__, __LINE__, "WSAEventSelect: %s: %s",
				 msg, strbuf);
		return (ISC_R_UNEXPECTED);
	}
	sock->hEvent = hEvent;

	sock->wait_type = type;
	notify_eventlist(sock, sock->manager, EVENT_ADD);
	return (ISC_R_SUCCESS);
}

/*
 * Note that the socket is not locked before calling this function
 */
void
socket_event_delete(isc_socket_t *sock) {

	REQUIRE(sock != NULL);
	REQUIRE(sock->hEvent != NULL);

	if (sock->hEvent != NULL) {
		sock->wait_type = 0;
		sock->pending_close = 1;
		notify_eventlist(sock, sock->manager, EVENT_DELETE);
		sock->hEvent = NULL;
		sock->hAlert = NULL;
		sock->evthread_id = 0;
	}
}

/*
 * Routine to cleanup and then close the socket.
 * Only close the socket here if it is NOT associated
 * with an event, otherwise the WSAWaitForMultipleEvents
 * may fail due to the fact that the the Wait should not
 * be running while closing an event or a socket.
 */
void
socket_close(isc_socket_t *sock) {

	REQUIRE(sock != NULL);
	sock->pending_close = 1;
	if (sock->hEvent != NULL)
		socket_event_delete(sock);
	else {
		closesocket(sock->fd);
	}
	if (sock->iocp) {
		sock->iocp = 0;
		InterlockedDecrement(&iocp_total);
	}

}

/*
 * Initialize socket services
 */
BOOL InitSockets() {
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	/* Need Winsock 2.0 or better */
	wVersionRequested = MAKEWORD(2, 0);

	err = WSAStartup(wVersionRequested, &wsaData);
	if ( err != 0 ) {
		/* Tell the user that we could not find a usable Winsock DLL */
		return(FALSE);
	}
	return(TRUE);
}

int
internal_sendmsg(isc_socket_t *sock, IoCompletionInfo *lpo,
		 struct msghdr *messagehdr, int flags, int *Error)
{
	int Result;
	DWORD BytesSent;
	DWORD Flags = flags;
	int total_sent;

	*Error = 0;
	Result = WSASendTo((SOCKET) sock->fd, messagehdr->msg_iov,
			   messagehdr->msg_iovlen, &BytesSent,
			   Flags, messagehdr->msg_name,
			   messagehdr->msg_namelen, (LPOVERLAPPED) lpo,
			   NULL);

	total_sent = (int) BytesSent;

	/* Check for errors.*/
	if (Result == SOCKET_ERROR) {

		*Error = WSAGetLastError();

		switch (*Error) {
		case WSA_IO_INCOMPLETE :
		case WSA_WAIT_IO_COMPLETION :
		case WSA_IO_PENDING :
			sock->pending_send++;
		case NO_ERROR :
			break;

		default :
			return (-1);
			break;
		}
	} else
		sock->pending_send++;
	if (lpo != NULL)
		return (0);
	else
		return (total_sent);
}

int
internal_recvmsg(isc_socket_t *sock, IoCompletionInfo *lpo,
		 struct msghdr *messagehdr, int flags, int *Error)
{
	DWORD Flags = 0;
	DWORD NumBytes = 0;
	int total_bytes = 0;
	int Result;

	*Error = 0;
	Result = WSARecvFrom((SOCKET) sock->fd,
			     messagehdr->msg_iov,
			     messagehdr->msg_iovlen,
			     &NumBytes,
			     &Flags,
			     messagehdr->msg_name,
			     (int *)&(messagehdr->msg_namelen),
			     (LPOVERLAPPED) lpo,
			     NULL);

	total_bytes = (int) NumBytes;

	/* Check for errors. */
	if (Result == SOCKET_ERROR) {

		*Error = WSAGetLastError();

		switch (*Error) {
		case WSA_IO_INCOMPLETE:
		case WSA_WAIT_IO_COMPLETION:
		case WSA_IO_PENDING:
			sock->pending_recv++;
		case NO_ERROR:
			break;

		default :
			return (-1);
			break;
		}
	} else
		sock->pending_recv++;

	/* Return the flags received in header */
	messagehdr->msg_flags = Flags;
	if (lpo != NULL)
		return (-1);
	else
		return (total_bytes);
}

static void
manager_log(isc_socketmgr_t *sockmgr, isc_logcategory_t *category,
	    isc_logmodule_t *module, int level, const char *fmt, ...)
{
	char msgbuf[2048];
	va_list ap;

	if (!isc_log_wouldlog(isc_lctx, level))
		return;

	va_start(ap, fmt);
	vsnprintf(msgbuf, sizeof(msgbuf), fmt, ap);
	va_end(ap);

	isc_log_write(isc_lctx, category, module, level,
		      "sockmgr %p: %s", sockmgr, msgbuf);
}

static void
socket_log(isc_socket_t *sock, isc_sockaddr_t *address,
	   isc_logcategory_t *category, isc_logmodule_t *module, int level,
	   isc_msgcat_t *msgcat, int msgset, int message,
	   const char *fmt, ...) ISC_FORMAT_PRINTF(9, 10);

static void
socket_log(isc_socket_t *sock, isc_sockaddr_t *address,
	   isc_logcategory_t *category, isc_logmodule_t *module, int level,
	   isc_msgcat_t *msgcat, int msgset, int message,
	   const char *fmt, ...)
{
	char msgbuf[2048];
	char peerbuf[256];
	va_list ap;

	if (! isc_log_wouldlog(isc_lctx, level))
		return;

	va_start(ap, fmt);
	vsnprintf(msgbuf, sizeof(msgbuf), fmt, ap);
	va_end(ap);

	if (address == NULL) {
		isc_log_iwrite(isc_lctx, category, module, level,
			       msgcat, msgset, message,
			       "socket %p: %s", sock, msgbuf);
	} else {
		isc_sockaddr_format(address, peerbuf, sizeof(peerbuf));
		isc_log_iwrite(isc_lctx, category, module, level,
			       msgcat, msgset, message,
			       "socket %p %s: %s", sock, peerbuf, msgbuf);
	}
}

/*
 * Make an fd SOCKET non-blocking.
 */
static isc_result_t
make_nonblock(SOCKET fd) {
	int ret;
	unsigned long flags = 1;
	char strbuf[ISC_STRERRORSIZE];

	/* Set the socket to non-blocking */
	ret = ioctlsocket(fd, FIONBIO, &flags);

	if (ret == -1) {
		isc__strerror(errno, strbuf, sizeof(strbuf));
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "ioctlsocket(%d, FIOBIO, %d): %s",
				 fd, flags, strbuf);

		return (ISC_R_UNEXPECTED);
	}

	return (ISC_R_SUCCESS);
}

/*
 * Windows 2000 systems incorrectly cause UDP sockets using WASRecvFrom
 * to not work correctly, returning a WSACONNRESET error when a WSASendTo
 * fails with an "ICMP port unreachable" response and preventing the
 * socket from using the WSARecvFrom in subsequent operations.
 * The function below fixes this, but requires that Windows 2000
 * Service Pack 2 or later be installed on the system.  NT 4.0
 * systems are not affected by this and work correctly.
 * See Microsoft Knowledge Base Article Q263823 for details of this.
 */
isc_result_t
connection_reset_fix(SOCKET fd) {
	DWORD dwBytesReturned = 0;
	BOOL  bNewBehavior = FALSE;
	DWORD status;

	if(isc_win32os_majorversion() < 5)
		return (ISC_R_SUCCESS); /*  NT 4.0 has no problem */

	/* disable bad behavior using IOCTL: SIO_UDP_CONNRESET */
	status = WSAIoctl(fd, SIO_UDP_CONNRESET, &bNewBehavior,
			  sizeof(bNewBehavior), NULL, 0,
			  &dwBytesReturned, NULL, NULL);
	if (status != SOCKET_ERROR)
		return (ISC_R_SUCCESS);
	else {
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "WSAIoctl(SIO_UDP_CONNRESET, oldBehaviour) %s",
				 isc_msgcat_get(isc_msgcat, ISC_MSGSET_GENERAL,
						ISC_MSG_FAILED, "failed"));
		return (ISC_R_UNEXPECTED);
	}
}

/*
 * Construct an iov array and attach it to the msghdr passed in.  This is
 * the SEND constructor, which will use the used region of the buffer
 * (if using a buffer list) or will use the internal region (if a single
 * buffer I/O is requested).
 *
 * Nothing can be NULL, and the done event must list at least one buffer
 * on the buffer linked list for this function to be meaningful.
 */
static void
build_msghdr_send(isc_socket_t *sock, isc_socketevent_t *dev,
		  struct msghdr *msg, char *cmsg, WSABUF *iov)
{
	unsigned int iovcount;
	isc_buffer_t *buffer;
	isc_region_t used;
	size_t write_count;
	size_t skip_count;

	memset(msg, 0, sizeof(*msg));

	if (sock->type == isc_sockettype_udp) {
		msg->msg_name = (void *)&dev->address.type.sa;
		msg->msg_namelen = dev->address.length;
	} else {
		msg->msg_name = NULL;
		msg->msg_namelen = 0;
	}

	buffer = ISC_LIST_HEAD(dev->bufferlist);
	write_count = 0;
	iovcount = 0;

	/*
	 * Single buffer I/O?  Skip what we've done so far in this region.
	 */
	if (buffer == NULL) {
		write_count = dev->region.length - dev->n;
		iov[0].buf = (void *)(dev->region.base + dev->n);
		iov[0].len = write_count;
		iovcount = 1;

		goto config;
	}

	/*
	 * Multibuffer I/O.
	 * Skip the data in the buffer list that we have already written.
	 */
	skip_count = dev->n;
	while (buffer != NULL) {
		REQUIRE(ISC_BUFFER_VALID(buffer));
		if (skip_count < isc_buffer_usedlength(buffer))
			break;
		skip_count -= isc_buffer_usedlength(buffer);
		buffer = ISC_LIST_NEXT(buffer, link);
	}

	while (buffer != NULL) {
		INSIST(iovcount < MAXSCATTERGATHER_SEND);

		isc_buffer_usedregion(buffer, &used);

		if (used.length > 0) {
			iov[iovcount].buf = (void *)(used.base
							  + skip_count);
			iov[iovcount].len = used.length - skip_count;
			write_count += (used.length - skip_count);
			skip_count = 0;
			iovcount++;
		}
		buffer = ISC_LIST_NEXT(buffer, link);
	}

	INSIST(skip_count == 0);

 config:
	msg->msg_iov = iov;
	msg->msg_iovlen = iovcount;
	msg->msg_totallen = write_count;
}

/*
 * Construct an iov array and attach it to the msghdr passed in.  This is
 * the RECV constructor, which will use the available region of the buffer
 * (if using a buffer list) or will use the internal region (if a single
 * buffer I/O is requested).
 *
 * Nothing can be NULL, and the done event must list at least one buffer
 * on the buffer linked list for this function to be meaningful.
 */
static void
build_msghdr_recv(isc_socket_t *sock, isc_socketevent_t *dev,
		  struct msghdr *msg, char *cmsg, WSABUF *iov)
{
	unsigned int iovcount;
	isc_buffer_t *buffer;
	isc_region_t available;
	size_t read_count;

	memset(msg, 0, sizeof(struct msghdr));

	if (sock->type == isc_sockettype_udp) {
		memset(&dev->address, 0, sizeof(dev->address));
		msg->msg_name = (void *)&dev->address.type.sa;
		msg->msg_namelen = sizeof(dev->address.type);
	} else { /* TCP */
		msg->msg_name = NULL;
		msg->msg_namelen = 0;
		dev->address = sock->address;
	}

	buffer = ISC_LIST_HEAD(dev->bufferlist);
	read_count = 0;

	/*
	 * Single buffer I/O?  Skip what we've done so far in this region.
	 */
	if (buffer == NULL) {
		read_count = dev->region.length - dev->n;
		iov[0].buf = (void *)(dev->region.base + dev->n);
		iov[0].len = read_count;
		iovcount = 1;
	} else {
		/*
		 * Multibuffer I/O.
		 * Skip empty buffers.
		 */
		while (buffer != NULL) {
			REQUIRE(ISC_BUFFER_VALID(buffer));
			if (isc_buffer_availablelength(buffer) != 0)
				break;
			buffer = ISC_LIST_NEXT(buffer, link);
		}

		iovcount = 0;
		while (buffer != NULL) {
			INSIST(iovcount < MAXSCATTERGATHER_RECV);

			isc_buffer_availableregion(buffer, &available);

			if (available.length > 0) {
				iov[iovcount].buf = (void *)(available.base);
				iov[iovcount].len = available.length;
				read_count += available.length;
				iovcount++;
			}
			buffer = ISC_LIST_NEXT(buffer, link);
		}
	}

	/*
	 * If needed, set up to receive that one extra byte.  Note that
	 * we know there is at least one iov left, since we stole it
	 * at the top of this function.
	 */

	msg->msg_iov = iov;
	msg->msg_iovlen = iovcount;
	msg->msg_totallen = read_count;
}

static void
set_dev_address(isc_sockaddr_t *address, isc_socket_t *sock,
		isc_socketevent_t *dev)
{
	if (sock->type == isc_sockettype_udp) {
		if (address != NULL)
			dev->address = *address;
		else
			dev->address = sock->address;
	} else if (sock->type == isc_sockettype_tcp) {
		INSIST(address == NULL);
		dev->address = sock->address;
	}
}

static isc_socketevent_t *
allocate_socketevent(isc_socket_t *sock, isc_eventtype_t eventtype,
		     isc_taskaction_t action, const void *arg)
{
	isc_socketevent_t *ev;

	ev = (isc_socketevent_t *)isc_event_allocate(sock->manager->mctx,
						     sock, eventtype,
						     action, arg,
						     sizeof(*ev));
	if (ev == NULL)
		return (NULL);

	ev->result = ISC_R_UNEXPECTED;
	ISC_LINK_INIT(ev, ev_link);
	ISC_LIST_INIT(ev->bufferlist);
	ev->region.base = NULL;
	ev->n = 0;
	ev->offset = 0;
	ev->attributes = 0;

	return (ev);
}

#if defined(ISC_SOCKET_DEBUG)
static void
dump_msg(struct msghdr *msg, isc_socket_t *sock) {
	unsigned int i;

	printf("MSGHDR %p, Socket #: %u\n", msg, sock->fd);
	printf("\tname %p, namelen %d\n", msg->msg_name, msg->msg_namelen);
	printf("\tiov %p, iovlen %d\n", msg->msg_iov, msg->msg_iovlen);
	for (i = 0; i < (unsigned int)msg->msg_iovlen; i++)
		printf("\t\t%d\tbase %p, len %d\n", i,
		       msg->msg_iov[i].buf,
		       msg->msg_iov[i].len);
}
#endif

static int
completeio_recv(isc_socket_t *sock, isc_socketevent_t *dev,
		struct msghdr *messagehdr, int cc, int recv_errno)
{
	size_t actual_count;
	isc_buffer_t *buffer;

#define SOFT_OR_HARD(_system, _isc) \
	if (recv_errno == _system) { \
		if (sock->connected) { \
			dev->result = _isc; \
			return (DOIO_HARD); \
		} \
		return (DOIO_SOFT); \
	}

#define ALWAYS_HARD(_system, _isc) \
	if (recv_errno == _system) { \
		dev->result = _isc; \
		return (DOIO_HARD); \
	}

	if (recv_errno != 0) {

		if (SOFT_ERROR(recv_errno))
			return (DOIO_SOFT);

		SOFT_OR_HARD(WSAECONNREFUSED, ISC_R_CONNREFUSED);
		SOFT_OR_HARD(WSAENETUNREACH, ISC_R_NETUNREACH);
		SOFT_OR_HARD(WSAEHOSTUNREACH, ISC_R_HOSTUNREACH);
		SOFT_OR_HARD(WSAECONNRESET, ISC_R_CONNECTIONRESET);
		SOFT_OR_HARD(WSAENETRESET, ISC_R_CONNECTIONRESET);
		SOFT_OR_HARD(WSAECONNABORTED, ISC_R_CONNECTIONRESET);
		SOFT_OR_HARD(WSAEDISCON, ISC_R_CONNECTIONRESET);
		SOFT_OR_HARD(WSAENETDOWN, ISC_R_NETDOWN);
		ALWAYS_HARD(ERROR_OPERATION_ABORTED, ISC_R_CONNECTIONRESET);
		ALWAYS_HARD(ERROR_NETNAME_DELETED, ISC_R_CONNECTIONRESET);
		ALWAYS_HARD(ERROR_PORT_UNREACHABLE, ISC_R_HOSTUNREACH);
		ALWAYS_HARD(ERROR_HOST_UNREACHABLE, ISC_R_HOSTUNREACH);
		ALWAYS_HARD(ERROR_NETWORK_UNREACHABLE, ISC_R_NETUNREACH);
		ALWAYS_HARD(ERROR_NETNAME_DELETED, ISC_R_NETUNREACH);
		ALWAYS_HARD(WSAENOBUFS, ISC_R_NORESOURCES);

#undef SOFT_OR_HARD
#undef ALWAYS_HARD

		dev->result = isc__errno2result(recv_errno);
		return (DOIO_HARD);
	}

	/*
	 * On TCP, zero length reads indicate EOF, while on
	 * UDP, zero length reads are perfectly valid, although
	 * strange.
	 */
	if ((sock->type == isc_sockettype_tcp) && (cc == 0))
		return (DOIO_EOF);

	if (sock->type == isc_sockettype_udp) {
		dev->address.length = messagehdr->msg_namelen;
		if (isc_sockaddr_getport(&dev->address) == 0) {
			if (isc_log_wouldlog(isc_lctx, IOEVENT_LEVEL)) {
				socket_log(sock, &dev->address, IOEVENT,
					   isc_msgcat, ISC_MSGSET_SOCKET,
					   ISC_MSG_ZEROPORT,
					   "dropping source port zero packet");
			}
			return (DOIO_SOFT);
		}
	}

	socket_log(sock, &dev->address, IOEVENT,
		   isc_msgcat, ISC_MSGSET_SOCKET, ISC_MSG_PKTRECV,
		   "packet received correctly");

	/*
	 * Overflow bit detection.  If we received MORE bytes than we should,
	 * this indicates an overflow situation.  Set the flag in the
	 * dev entry and adjust how much we read by one.
	 */
#ifdef ISC_NET_RECVOVERFLOW
	if ((sock->type == isc_sockettype_udp) && ((size_t)cc > read_count)) {
		dev->attributes |= ISC_SOCKEVENTATTR_TRUNC;
		cc--;
	}
#endif

	/*
	 * update the buffers (if any) and the i/o count
	 */
	dev->n += cc;
	actual_count = cc;
	buffer = ISC_LIST_HEAD(dev->bufferlist);
	while (buffer != NULL && actual_count > 0) {
		REQUIRE(ISC_BUFFER_VALID(buffer));
		if (isc_buffer_availablelength(buffer) <= actual_count) {
			actual_count -= isc_buffer_availablelength(buffer);
			isc_buffer_add(buffer,
				       isc_buffer_availablelength(buffer));
		} else {
			isc_buffer_add(buffer, actual_count);
			actual_count = 0;
			break;
		}
		buffer = ISC_LIST_NEXT(buffer, link);
		if (buffer == NULL) {
			INSIST(actual_count == 0);
		}
	}

	/*
	 * If we read less than we expected, update counters,
	 * and let the upper layer handle it.
	 */
	if ((cc != messagehdr->msg_totallen) && (dev->n < dev->minimum))
		return (DOIO_SOFT);

	/*
	 * Full reads are posted, or partials if partials are ok.
	 */
	dev->result = ISC_R_SUCCESS;
	return (DOIO_SUCCESS);
}

static int
startio_recv(isc_socket_t *sock, isc_socketevent_t *dev, int *nbytes,
	     int *recv_errno)
{
	char *cmsg = NULL;
	char strbuf[ISC_STRERRORSIZE];
	IoCompletionInfo *lpo;
	int status;
	struct msghdr *msghdr;

	lpo = (IoCompletionInfo *) HeapAlloc(hHeapHandle,
					     HEAP_ZERO_MEMORY,
					     sizeof(IoCompletionInfo));
	lpo->request_type = SOCKET_RECV;
	lpo->dev = dev;
	msghdr = &lpo->messagehdr;
	memset(msghdr, 0, sizeof(struct msghdr));

	build_msghdr_recv(sock, dev, msghdr, cmsg, sock->iov);

#if defined(ISC_SOCKET_DEBUG)
	dump_msg(msghdr, sock);
#endif

	*nbytes = internal_recvmsg(sock, lpo, msghdr, 0, recv_errno);

	if (*nbytes < 0) {
		/*
		 * I/O has been initiated
		 * return will be via the completion port
		 */
		if (PENDING_ERROR(*recv_errno)) {
			status = DOIO_PENDING;
			goto done;
		}
		if (SOFT_ERROR(*recv_errno)) {
			status = DOIO_SOFT;
			goto done;
		}

		/*
		 * If we got this far something is wrong
		 */
		if (isc_log_wouldlog(isc_lctx, IOEVENT_LEVEL)) {
			isc__strerror(*recv_errno, strbuf, sizeof(strbuf));
			socket_log(sock, NULL, IOEVENT,
				   isc_msgcat, ISC_MSGSET_SOCKET,
				   ISC_MSG_DOIORECV,
				  "startio_recv: recvmsg(%d) %d bytes, "
				  "err %d/%s",
				   sock->fd, *nbytes, *recv_errno, strbuf);
		}
		status = DOIO_HARD;
		goto done;
	}
	dev->result = ISC_R_SUCCESS;
	status = DOIO_SOFT;
done:
	return (status);
}

/*
 * Returns:
 *	DOIO_SUCCESS	The operation succeeded.  dev->result contains
 *			ISC_R_SUCCESS.
 *
 *	DOIO_HARD	A hard or unexpected I/O error was encountered.
 *			dev->result contains the appropriate error.
 *
 *	DOIO_SOFT	A soft I/O error was encountered.  No senddone
 *			event was sent.  The operation should be retried.
 *
 *	No other return values are possible.
 */
static int
completeio_send(isc_socket_t *sock, isc_socketevent_t *dev,
		struct msghdr *messagehdr, int cc, int send_errno)
{
	char addrbuf[ISC_SOCKADDR_FORMATSIZE];
	char strbuf[ISC_STRERRORSIZE];

	if(send_errno != 0) {


		if (SOFT_ERROR(send_errno))
			return (DOIO_SOFT);

#define SOFT_OR_HARD(_system, _isc) \
	if (send_errno == _system) { \
		if (sock->connected) { \
			dev->result = _isc; \
			return (DOIO_HARD); \
		} \
		return (DOIO_SOFT); \
	}
#define ALWAYS_HARD(_system, _isc) \
	if (send_errno == _system) { \
		dev->result = _isc; \
		return (DOIO_HARD); \
	}

		SOFT_OR_HARD(WSAEACCES, ISC_R_NOPERM);
		SOFT_OR_HARD(WSAEAFNOSUPPORT, ISC_R_ADDRNOTAVAIL);
		SOFT_OR_HARD(WSAECONNREFUSED, ISC_R_CONNREFUSED);
		SOFT_OR_HARD(WSAENOTCONN, ISC_R_CONNREFUSED);
		SOFT_OR_HARD(WSAECONNRESET, ISC_R_CONNECTIONRESET);
		SOFT_OR_HARD(WSAECONNABORTED, ISC_R_CONNECTIONRESET);
		SOFT_OR_HARD(WSAENETRESET, ISC_R_CONNECTIONRESET);
		SOFT_OR_HARD(WSAEDISCON, ISC_R_CONNECTIONRESET);
		SOFT_OR_HARD(WSAENETDOWN, ISC_R_NETDOWN);
		ALWAYS_HARD(ERROR_OPERATION_ABORTED, ISC_R_CONNECTIONRESET);
		ALWAYS_HARD(ERROR_NETNAME_DELETED, ISC_R_CONNECTIONRESET);
		ALWAYS_HARD(ERROR_PORT_UNREACHABLE, ISC_R_HOSTUNREACH);
		ALWAYS_HARD(ERROR_HOST_UNREACHABLE, ISC_R_HOSTUNREACH);
		ALWAYS_HARD(ERROR_NETWORK_UNREACHABLE, ISC_R_NETUNREACH);
		ALWAYS_HARD(WSAEADDRNOTAVAIL, ISC_R_ADDRNOTAVAIL);
		ALWAYS_HARD(WSAEHOSTUNREACH, ISC_R_HOSTUNREACH);
		ALWAYS_HARD(WSAEHOSTDOWN, ISC_R_HOSTUNREACH);
		ALWAYS_HARD(WSAENETUNREACH, ISC_R_NETUNREACH);
		ALWAYS_HARD(WSAENOBUFS, ISC_R_NORESOURCES);
		ALWAYS_HARD(EPERM, ISC_R_HOSTUNREACH);
		ALWAYS_HARD(EPIPE, ISC_R_NOTCONNECTED);

#undef SOFT_OR_HARD
#undef ALWAYS_HARD

		/*
		 * The other error types depend on whether or not the
		 * socket is UDP or TCP.  If it is UDP, some errors
		 * that we expect to be fatal under TCP are merely
		 * annoying, and are really soft errors.
		 *
		 * However, these soft errors are still returned as
		 * a status.
		 */
		isc_sockaddr_format(&dev->address, addrbuf, sizeof(addrbuf));
		isc__strerror(send_errno, strbuf, sizeof(strbuf));
		UNEXPECTED_ERROR(__FILE__, __LINE__, "completeio_send: %s: %s",
				 addrbuf, strbuf);
		dev->result = isc__errno2result(send_errno);
	return (DOIO_HARD);
	}

	/*
	 * If we write less than we expected, update counters, poke.
	 */
	dev->n += cc;
	if (cc != messagehdr->msg_totallen)
		return (DOIO_SOFT);

	/*
	 * Exactly what we wanted to write.  We're done with this
	 * entry.  Post its completion event.
	 */
	dev->result = ISC_R_SUCCESS;
	return (DOIO_SUCCESS);
}

static int
startio_send(isc_socket_t *sock, isc_socketevent_t *dev, int *nbytes,
	     int *send_errno)
{
	char *cmsg = NULL;
	char strbuf[ISC_STRERRORSIZE];
	IoCompletionInfo *lpo;
	int status;
	struct msghdr *msghdr;

	lpo = (IoCompletionInfo *) HeapAlloc(hHeapHandle,
					     HEAP_ZERO_MEMORY,
					     sizeof(IoCompletionInfo));
	lpo->request_type = SOCKET_SEND;
	lpo->dev = dev;
	msghdr = &lpo->messagehdr;
	memset(msghdr, 0, sizeof(struct msghdr));

	build_msghdr_send(sock, dev, msghdr, cmsg, sock->iov);

	*nbytes = internal_sendmsg(sock, lpo, msghdr, 0, send_errno);


	if (*nbytes < 0) {
		/*
		 * I/O has been initiated
		 * completion will be through the completion port
		 */
		if (PENDING_ERROR(*send_errno)) {
			status = DOIO_PENDING;
			goto done;
		}

		if (SOFT_ERROR(*send_errno)) {
			status = DOIO_SOFT;
			goto done;
		}

		/*
		 * If we got this far then something is wrong
		 */
		if (isc_log_wouldlog(isc_lctx, IOEVENT_LEVEL)) {
			isc__strerror(*send_errno, strbuf, sizeof(strbuf));
			socket_log(sock, NULL, IOEVENT,
				   isc_msgcat, ISC_MSGSET_SOCKET,
				   ISC_MSG_INTERNALSEND,
				   "startio_send: internal_sendmsg(%d) %d "
				   "bytes, err %d/%s",
				   sock->fd, *nbytes, *send_errno, strbuf);
		}
		goto done;
	}
	dev->result = ISC_R_SUCCESS;
	status = DOIO_SOFT;
done:
	return (status);
}

/*
 * Kill.
 *
 * Caller must ensure that the socket is not locked and no external
 * references exist.
 */
static void
destroy_socket(isc_socket_t **sockp) {
	isc_socket_t *sock = *sockp;
	isc_socketmgr_t *manager = sock->manager;
	isc_boolean_t dofree = ISC_TRUE;

	REQUIRE(sock != NULL);

	socket_log(sock, NULL, CREATION, isc_msgcat, ISC_MSGSET_SOCKET,
		   ISC_MSG_DESTROYING, "destroying socket %d", sock->fd);

	INSIST(ISC_LIST_EMPTY(sock->accept_list));
	INSIST(ISC_LIST_EMPTY(sock->recv_list));
	INSIST(ISC_LIST_EMPTY(sock->send_list));
	INSIST(sock->connect_ev == NULL);

	LOCK(&manager->lock);

	LOCK(&sock->lock);
	socket_close(sock);
	if (sock->pending_recv != 0 || sock->pending_send != 0) {
		dofree = ISC_FALSE;
		sock->pending_free = 1;
	}
	ISC_LIST_UNLINK(manager->socklist, sock, link);
	UNLOCK(&sock->lock);

	if (ISC_LIST_EMPTY(manager->socklist))
		SIGNAL(&manager->shutdown_ok);

	/*
	 * XXX should reset manager->maxfd here
	 */
	UNLOCK(&manager->lock);

	if (dofree)
		free_socket(sockp);
}

static isc_result_t
allocate_socket(isc_socketmgr_t *manager, isc_sockettype_t type,
		isc_socket_t **socketp) {
	isc_socket_t *sock;
	isc_result_t ret;

	sock = isc_mem_get(manager->mctx, sizeof(*sock));

	if (sock == NULL)
		return (ISC_R_NOMEMORY);

	ret = ISC_R_UNEXPECTED;

	sock->magic = 0;
	sock->references = 0;

	sock->manager = manager;
	sock->type = type;
	sock->fd = INVALID_SOCKET;

	ISC_LINK_INIT(sock, link);

	/*
	 * set up list of readers and writers to be initially empty
	 */
	ISC_LIST_INIT(sock->recv_list);
	ISC_LIST_INIT(sock->send_list);
	ISC_LIST_INIT(sock->accept_list);
	sock->connect_ev = NULL;
	sock->pending_accept = 0;
	sock->pending_close = 0;
	sock->pending_recv = 0;
	sock->pending_send = 0;
	sock->pending_free = 0;
	sock->iocp = 0;
	sock->listener = 0;
	sock->connected = 0;
	sock->connecting = 0;
	sock->bound = 0;
	sock->hEvent = NULL;
	sock->hAlert = NULL;
	sock->evthread_id = 0;
	sock->wait_type = 0;

	/*
	 * initialize the lock
	 */
	if (isc_mutex_init(&sock->lock) != ISC_R_SUCCESS) {
		sock->magic = 0;
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "isc_mutex_init() %s",
				 isc_msgcat_get(isc_msgcat, ISC_MSGSET_GENERAL,
						ISC_MSG_FAILED, "failed"));
		ret = ISC_R_UNEXPECTED;
		goto error;
	}

	/*
	 * Initialize readable and writable events
	 */
	ISC_EVENT_INIT(&sock->readable_ev, sizeof(intev_t),
		       ISC_EVENTATTR_NOPURGE, NULL, ISC_SOCKEVENT_INTR,
		       NULL, sock, sock, NULL, NULL);
	ISC_EVENT_INIT(&sock->writable_ev, sizeof(intev_t),
		       ISC_EVENTATTR_NOPURGE, NULL, ISC_SOCKEVENT_INTW,
		       NULL, sock, sock, NULL, NULL);

	sock->magic = SOCKET_MAGIC;
	*socketp = sock;

	return (ISC_R_SUCCESS);

 error:
	isc_mem_put(manager->mctx, sock, sizeof(*sock));

	return (ret);
}

/*
 * This event requires that the various lists be empty, that the reference
 * count be 1, and that the magic number is valid.  The other socket bits,
 * like the lock, must be initialized as well.  The fd associated must be
 * marked as closed, by setting it to INVALID_SOCKET on close, or this
 * routine will also close the socket.
 */
static void
free_socket(isc_socket_t **socketp) {
	isc_socket_t *sock = *socketp;

	INSIST(sock->references == 0);
	INSIST(VALID_SOCKET(sock));
	INSIST(!sock->connecting);
	INSIST(!sock->pending_accept);
	INSIST(ISC_LIST_EMPTY(sock->recv_list));
	INSIST(ISC_LIST_EMPTY(sock->send_list));
	INSIST(ISC_LIST_EMPTY(sock->accept_list));
	INSIST(!ISC_LINK_LINKED(sock, link));

	sock->magic = 0;

	DESTROYLOCK(&sock->lock);

	isc_mem_put(sock->manager->mctx, sock, sizeof(*sock));

	*socketp = NULL;
}

/*
 * Create a new 'type' socket managed by 'manager'.  Events
 * will be posted to 'task' and when dispatched 'action' will be
 * called with 'arg' as the arg value.  The new socket is returned
 * in 'socketp'.
 */
isc_result_t
isc_socket_create(isc_socketmgr_t *manager, int pf, isc_sockettype_t type,
		  isc_socket_t **socketp) {
	isc_socket_t *sock = NULL;
	isc_result_t result;
#if defined(USE_CMSG) || defined(SO_BSDCOMPAT)
	int on = 1;
#endif
	int socket_errno;
	char strbuf[ISC_STRERRORSIZE];

	REQUIRE(VALID_MANAGER(manager));
	REQUIRE(socketp != NULL && *socketp == NULL);

	result = allocate_socket(manager, type, &sock);
	if (result != ISC_R_SUCCESS)
		return (result);

	sock->pf = pf;
	switch (type) {
	case isc_sockettype_udp:
		sock->fd = socket(pf, SOCK_DGRAM, IPPROTO_UDP);
		if (sock->fd != INVALID_SOCKET) {
			result = connection_reset_fix(sock->fd);
			if (result != ISC_R_SUCCESS) {
				closesocket(sock->fd);
				free_socket(&sock);
				return (result);
			}
		}
		break;
	case isc_sockettype_tcp:
		sock->fd = socket(pf, SOCK_STREAM, IPPROTO_TCP);
		break;
	}

	if (sock->fd == INVALID_SOCKET) {
		socket_errno = WSAGetLastError();
		free_socket(&sock);

		switch (socket_errno) {
		case WSAEMFILE:
		case WSAENOBUFS:
			return (ISC_R_NORESOURCES);

		case WSAEPROTONOSUPPORT:
		case WSAEPFNOSUPPORT:
		case WSAEAFNOSUPPORT:
			return (ISC_R_FAMILYNOSUPPORT);

		default:
			isc__strerror(socket_errno, strbuf, sizeof(strbuf));
			UNEXPECTED_ERROR(__FILE__, __LINE__,
					 "socket() %s: %s",
					 isc_msgcat_get(isc_msgcat,
							ISC_MSGSET_GENERAL,
							ISC_MSG_FAILED,
							"failed"),
					 strbuf);
			return (ISC_R_UNEXPECTED);
		}
	}

	result = make_nonblock(sock->fd);
	if (result != ISC_R_SUCCESS) {
		free_socket(&sock);
		return (result);
	}


#if defined(USE_CMSG)
	if (type == isc_sockettype_udp) {

#if defined(ISC_PLATFORM_HAVEIPV6)
#ifdef IPV6_RECVPKTINFO
		/* 2292bis */
		if ((pf == AF_INET6)
		    && (setsockopt(sock->fd, IPPROTO_IPV6, IPV6_RECVPKTINFO,
				   (void *)&on, sizeof(on)) < 0)) {
			isc__strerror(WSAGetLastError(), strbuf, sizeof(strbuf));
			UNEXPECTED_ERROR(__FILE__, __LINE__,
					 "setsockopt(%d, IPV6_RECVPKTINFO) "
					 "%s: %s", sock->fd,
					 isc_msgcat_get(isc_msgcat,
							ISC_MSGSET_GENERAL,
							ISC_MSG_FAILED,
							"failed"),
					 strbuf);
		}
#else
		/* 2292 */
		if ((pf == AF_INET6)
		    && (setsockopt(sock->fd, IPPROTO_IPV6, IPV6_PKTINFO,
				   (void *)&on, sizeof(on)) < 0)) {
			isc__strerror(WSAGetLastError(), strbuf, sizeof(strbuf));
			UNEXPECTED_ERROR(__FILE__, __LINE__,
					 "setsockopt(%d, IPV6_PKTINFO) %s: %s",
					 sock->fd,
					 isc_msgcat_get(isc_msgcat,
							ISC_MSGSET_GENERAL,
							ISC_MSG_FAILED,
							"failed"),
					 strbuf);
		}
#endif /* IPV6_RECVPKTINFO */
#ifdef IPV6_USE_MIN_MTU	/*2292bis, not too common yet*/
		/* use minimum MTU */
		if (pf == AF_INET6) {
			(void)setsockopt(sock->fd, IPPROTO_IPV6,
					 IPV6_USE_MIN_MTU,
					 (void *)&on, sizeof(on));
		}
#endif
#endif /* ISC_PLATFORM_HAVEIPV6 */

	}
#endif /* USE_CMSG */

	sock->references = 1;
	*socketp = sock;

	LOCK(&manager->lock);

	/*
	 * Note we don't have to lock the socket like we normally would because
	 * there are no external references to it yet.
	 */

	ISC_LIST_APPEND(manager->socklist, sock, link);
	UNLOCK(&manager->lock);

	socket_log(sock, NULL, CREATION, isc_msgcat, ISC_MSGSET_SOCKET,
		   ISC_MSG_CREATED, "created %u", sock->fd);

	return (ISC_R_SUCCESS);
}

/*
 * Attach to a socket.  Caller must explicitly detach when it is done.
 */
void
isc_socket_attach(isc_socket_t *sock, isc_socket_t **socketp) {
	REQUIRE(VALID_SOCKET(sock));
	REQUIRE(socketp != NULL && *socketp == NULL);

	LOCK(&sock->lock);
	sock->references++;
	UNLOCK(&sock->lock);

	*socketp = sock;
}

/*
 * Dereference a socket.  If this is the last reference to it, clean things
 * up by destroying the socket.
 */
void
isc_socket_detach(isc_socket_t **socketp) {
	isc_socket_t *sock;
	isc_boolean_t kill_socket = ISC_FALSE;

	REQUIRE(socketp != NULL);
	sock = *socketp;
	REQUIRE(VALID_SOCKET(sock));

	LOCK(&sock->lock);
	REQUIRE(sock->references > 0);
	sock->references--;
	if (sock->references == 0)
		kill_socket = ISC_TRUE;
	UNLOCK(&sock->lock);

	if (kill_socket)
		destroy_socket(&sock);

	*socketp = NULL;
}

/*
 * Dequeue an item off the given socket's read queue, set the result code
 * in the done event to the one provided, and send it to the task it was
 * destined for.
 *
 * If the event to be sent is on a list, remove it before sending.  If
 * asked to, send and detach from the socket as well.
 *
 * Caller must have the socket locked if the event is attached to the socket.
 */
static void
send_recvdone_event(isc_socket_t *sock, isc_socketevent_t **dev) {
	isc_task_t *task;

	task = (*dev)->ev_sender;

	(*dev)->ev_sender = sock;

	if (ISC_LINK_LINKED(*dev, ev_link)) {
		ISC_LIST_DEQUEUE(sock->recv_list, *dev, ev_link);
	}

	if (((*dev)->attributes & ISC_SOCKEVENTATTR_ATTACHED)
	    == ISC_SOCKEVENTATTR_ATTACHED)
		isc_task_sendanddetach(&task, (isc_event_t **)dev);
	else
		isc_task_send(task, (isc_event_t **)dev);
}

/*
 * See comments for send_recvdone_event() above.
 *
 * Caller must have the socket locked if the event is attached to the socket.
 */
static void
send_senddone_event(isc_socket_t *sock, isc_socketevent_t **dev) {
	isc_task_t *task;

	INSIST(dev != NULL && *dev != NULL);

	task = (*dev)->ev_sender;
	(*dev)->ev_sender = sock;

	if (ISC_LINK_LINKED(*dev, ev_link)) {
		ISC_LIST_DEQUEUE(sock->send_list, *dev, ev_link);
	}

	if (((*dev)->attributes & ISC_SOCKEVENTATTR_ATTACHED)
	    == ISC_SOCKEVENTATTR_ATTACHED)
		isc_task_sendanddetach(&task, (isc_event_t **)dev);
	else
		isc_task_send(task, (isc_event_t **)dev);
}

/*
 * Call accept() on a socket, to get the new file descriptor.  The listen
 * socket is used as a prototype to create a new isc_socket_t.  The new
 * socket has one outstanding reference.  The task receiving the event
 * will be detached from just after the event is delivered.
 *
 * On entry to this function, the event delivered is the internal
 * readable event, and the first item on the accept_list should be
 * the done event we want to send.  If the list is empty, this is a no-op,
 * so just unlock and return.
 */
static void
internal_accept(isc_socket_t *sock, int accept_errno) {
	isc_socketmgr_t *manager;
	isc_socket_newconnev_t *dev;
	isc_task_t *task;
	ISC_SOCKADDR_LEN_T addrlen;
	SOCKET fd;
	isc_result_t result = ISC_R_SUCCESS;
	char strbuf[ISC_STRERRORSIZE];

	INSIST(VALID_SOCKET(sock));

	LOCK(&sock->lock);
	socket_log(sock, NULL, TRACE,
		   isc_msgcat, ISC_MSGSET_SOCKET, ISC_MSG_ACCEPTLOCK,
		   "internal_accept called, locked socket");

	manager = sock->manager;
	INSIST(VALID_MANAGER(manager));

	INSIST(sock->listener);
	INSIST(sock->hEvent != NULL);
	INSIST(sock->pending_accept == 1);
	sock->pending_accept = 0;

	/*
	 * Check any possible error status from the event notification here.
	 * Note that we don't take any action since it was only
	 * Windows that was notifying about a network event, not the
	 * application.
	 * PDMXXX: Should we care about any of the possible event errors
	 *	   signalled? The only possible valid errors are:
	 *	   WSAENETDOWN, WSAECONNRESET, & WSAECONNABORTED
	 */
	if (accept_errno != 0) {
		switch (accept_errno) {
		case WSAENETDOWN:
		case WSAECONNRESET:
		case WSAECONNABORTED:
			break;		/* Expected errors */
		default:
			isc__strerror(accept_errno, strbuf, sizeof(strbuf));
			UNEXPECTED_ERROR(__FILE__, __LINE__,
					 "internal_accept: from event wait: %s",
					 strbuf);
			break;
		}
		UNLOCK(&sock->lock);
		return;
	}

	/*
	 * Get the first item off the accept list.
	 * If it is empty, unlock the socket and return.
	 */
	dev = ISC_LIST_HEAD(sock->accept_list);
	if (dev == NULL) {
		isc_sockaddr_t from;
		/*
		 * This should only happen if WSAEventSelect() fails
		 * below or in isc_socket_cancel().
		 */
		addrlen = sizeof(from.type);
		fd = accept(sock->fd, &from.type.sa, &addrlen);
		if (fd != INVALID_SOCKET) {
			char addrbuf[ISC_SOCKADDR_FORMATSIZE];
			isc_sockaddr_format(&from, addrbuf, sizeof(addrbuf));
			UNEXPECTED_ERROR(__FILE__, __LINE__,
					 "sock->accept_list empty: "
					 "dropping TCP request from %s",
					 addrbuf);
			(void)closesocket(fd);
		}
		UNLOCK(&sock->lock);
		return;
	}

	/*
	 * Try to accept the new connection.  If the accept fails with
	 * WSAEINTR, the event wait will be notified again since
	 * the event will be reset on return to caller.
	 */
	addrlen = sizeof(dev->newsocket->address.type);
	memset(&dev->newsocket->address.type.sa, 0, addrlen);
	fd = accept(sock->fd, &dev->newsocket->address.type.sa,
		    (void *)&addrlen);
	if (fd == INVALID_SOCKET) {
		accept_errno = WSAGetLastError();
		if (SOFT_ERROR(accept_errno) || accept_errno == WSAECONNRESET) {
			goto soft_error;
		} else {
			isc__strerror(accept_errno, strbuf, sizeof(strbuf));
			UNEXPECTED_ERROR(__FILE__, __LINE__,
					 "internal_accept: accept() %s: %s",
					 isc_msgcat_get(isc_msgcat,
							ISC_MSGSET_GENERAL,
							ISC_MSG_FAILED,
							"failed"),
					 strbuf);
			fd = INVALID_SOCKET;
			result = ISC_R_UNEXPECTED;
		}
	} else {
		if (addrlen == 0) {
			UNEXPECTED_ERROR(__FILE__, __LINE__,
					 "internal_accept(): "
					 "accept() failed to return "
					 "remote address");

			(void)closesocket(fd);
			goto soft_error;
		} else if (dev->newsocket->address.type.sa.sa_family !=
			   sock->pf)
		{
			UNEXPECTED_ERROR(__FILE__, __LINE__,
					 "internal_accept(): "
					 "accept() returned peer address "
					 "family %u (expected %u)",
					 dev->newsocket->address.
					 type.sa.sa_family,
					 sock->pf);
			(void)closesocket(fd);
			goto soft_error;
		}
	}

	if (fd != INVALID_SOCKET) {
		dev->newsocket->address.length = addrlen;
		dev->newsocket->pf = sock->pf;
	}

	/*
	 * Pull off the done event.
	 */
	ISC_LIST_UNLINK(sock->accept_list, dev, ev_link);

	/*
	 * Stop listing for connects.
	 */
	if (ISC_LIST_EMPTY(sock->accept_list) &&
	    WSAEventSelect(sock->fd, sock->hEvent, FD_CLOSE) != 0) {
		int stat;
		const char *msg;
		stat = WSAGetLastError();
		isc__strerror(stat, strbuf, sizeof(strbuf));
		msg = isc_msgcat_get(isc_msgcat, ISC_MSGSET_GENERAL,
				     ISC_MSG_FAILED, "failed");
		UNEXPECTED_ERROR(__FILE__, __LINE__, "WSAEventSelect: %s: %s",
				 msg, strbuf);
	}

	UNLOCK(&sock->lock);

	if (fd != INVALID_SOCKET) {
		isc_result_t tresult;
		tresult = make_nonblock(fd);
		if (tresult != ISC_R_SUCCESS) {
			closesocket(fd);
			fd = INVALID_SOCKET;
			result = tresult;
		}
	}

	/*
	 * INVALID_SOCKET means the new socket didn't happen.
	 */
	if (fd != INVALID_SOCKET) {
		LOCK(&manager->lock);
		ISC_LIST_APPEND(manager->socklist, dev->newsocket, link);

		dev->newsocket->fd = fd;
		dev->newsocket->bound = 1;
		dev->newsocket->connected = 1;

		/*
		 * The accept socket inherits the listen socket's
		 * selected events. Remove this socket from all events
		 * as it is handled by IOCP. (Joe Quanaim, lucent.com)
		 */
		if (WSAEventSelect(dev->newsocket->fd, 0, 0) != 0) {
			/* this is an unlikely but non-fatal error */
			int stat;
			const char *msg;
			stat = WSAGetLastError();
			isc__strerror(stat, strbuf, sizeof(strbuf));
			msg = isc_msgcat_get(isc_msgcat, ISC_MSGSET_GENERAL,
					     ISC_MSG_FAILED, "failed");
			UNEXPECTED_ERROR(__FILE__, __LINE__,
					 "WSAEventSelect: %s: %s", msg, strbuf);
		}

		/*
		 * Save away the remote address
		 */
		dev->address = dev->newsocket->address;

		socket_log(sock, &dev->newsocket->address, CREATION,
			   isc_msgcat, ISC_MSGSET_SOCKET, ISC_MSG_ACCEPTEDCXN,
			   "accepted connection, new socket %p",
			   dev->newsocket);

		UNLOCK(&manager->lock);
	} else {
		dev->newsocket->references--;
		free_socket(&dev->newsocket);
	}

	/*
	 * Fill in the done event details and send it off.
	 */
	dev->result = result;
	task = dev->ev_sender;
	dev->ev_sender = sock;

	isc_task_sendanddetach(&task, (isc_event_t **)&dev);
	return;

 soft_error:
	UNLOCK(&sock->lock);
	return;
}

/*
 * Called when a socket with a pending connect() finishes.
 */
static void
internal_connect(isc_socket_t *sock, int connect_errno) {
	isc_socket_connev_t *dev;
	isc_task_t *task;
	char strbuf[ISC_STRERRORSIZE];

	INSIST(VALID_SOCKET(sock));

	LOCK(&sock->lock);

	/*
	 * Has this event been canceled?
	 */
	dev = sock->connect_ev;
	if (dev == NULL) {
		INSIST(!sock->connecting);
		UNLOCK(&sock->lock);
		return;
	}

	INSIST(sock->connecting);
	sock->connecting = 0;

	/*
	 * Check possible Windows network event error status here.
	 */
	if (connect_errno != 0) {
		/*
		 * If the error is SOFT, just try again on this
		 * fd and pretend nothing strange happened.
		 */
		if (SOFT_ERROR(connect_errno) ||
		    connect_errno == WSAEINPROGRESS)
		{
			sock->connecting = 1;
			UNLOCK(&sock->lock);
			return;
		}

		/*
		 * Translate other errors into ISC_R_* flavors.
		 */
		switch (connect_errno) {
#define ERROR_MATCH(a, b) case a: dev->result = b; break;
			ERROR_MATCH(WSAEACCES, ISC_R_NOPERM);
			ERROR_MATCH(WSAEADDRNOTAVAIL, ISC_R_ADDRNOTAVAIL);
			ERROR_MATCH(WSAEAFNOSUPPORT, ISC_R_ADDRNOTAVAIL);
			ERROR_MATCH(WSAECONNREFUSED, ISC_R_CONNREFUSED);
			ERROR_MATCH(WSAEHOSTUNREACH, ISC_R_HOSTUNREACH);
			ERROR_MATCH(WSAEHOSTDOWN, ISC_R_HOSTDOWN);
			ERROR_MATCH(WSAENETUNREACH, ISC_R_NETUNREACH);
			ERROR_MATCH(WSAENETDOWN, ISC_R_NETDOWN);
			ERROR_MATCH(WSAENOBUFS, ISC_R_NORESOURCES);
			ERROR_MATCH(WSAECONNRESET, ISC_R_CONNECTIONRESET);
			ERROR_MATCH(WSAECONNABORTED, ISC_R_CONNECTIONRESET);
			ERROR_MATCH(WSAETIMEDOUT, ISC_R_TIMEDOUT);
#undef ERROR_MATCH
		default:
			dev->result = ISC_R_UNEXPECTED;
			isc__strerror(connect_errno, strbuf, sizeof(strbuf));
			UNEXPECTED_ERROR(__FILE__, __LINE__,
					 "internal_connect: connect() %s",
					 strbuf);
		}
	} else {
		dev->result = ISC_R_SUCCESS;
		sock->connected = 1;
		sock->bound = 1;
	}

	sock->connect_ev = NULL;

	UNLOCK(&sock->lock);

	task = dev->ev_sender;
	dev->ev_sender = sock;
	isc_task_sendanddetach(&task, (isc_event_t **)&dev);
}

static void
internal_recv(isc_socket_t *sock, isc_socketevent_t *dev,
	      struct msghdr *messagehdr, int nbytes, int recv_errno)
{
	isc_socketevent_t *ldev;
	int io_state;
	int cc;

	INSIST(VALID_SOCKET(sock));

	LOCK(&sock->lock);
	socket_log(sock, NULL, IOEVENT,
		   isc_msgcat, ISC_MSGSET_SOCKET, ISC_MSG_INTERNALRECV,
		   "internal_recv: task got socket event %p", dev);

	INSIST(sock->pending_recv > 0);
	sock->pending_recv--;
	/* If the event is no longer in the list we can just return */
	ldev = ISC_LIST_HEAD(sock->recv_list);
	while (ldev != NULL && ldev != dev) {
		ldev = ISC_LIST_NEXT(ldev, ev_link);
	}
	if (ldev == NULL)
		goto done;

	/*
	 * Try to do as much I/O as possible on this socket.  There are no
	 * limits here, currently.
	 */
	switch (completeio_recv(sock, dev, messagehdr, nbytes, recv_errno)) {
	case DOIO_SOFT:
		cc = 0;
		recv_errno = 0;
		io_state = startio_recv(sock, dev, &cc, &recv_errno);
		goto done;

	case DOIO_EOF:
		/*
		 * read of 0 means the remote end was closed.
		 * Run through the event queue and dispatch all
		 * the events with an EOF result code.
		 */
		dev->result = ISC_R_EOF;
		send_recvdone_event(sock, &dev);
		goto done;

	case DOIO_SUCCESS:
	case DOIO_HARD:
		send_recvdone_event(sock, &dev);
		break;
	}
 done:
	UNLOCK(&sock->lock);
}

static void
internal_send(isc_socket_t *sock, isc_socketevent_t *dev,
	      struct msghdr *messagehdr, int nbytes, int send_errno)
{
	isc_socketevent_t *ldev;

	/*
	 * Find out what socket this is and lock it.
	 */
	INSIST(VALID_SOCKET(sock));

	LOCK(&sock->lock);
	socket_log(sock, NULL, IOEVENT,
		   isc_msgcat, ISC_MSGSET_SOCKET, ISC_MSG_INTERNALSEND,
		   "internal_send: task got socket event %p", dev);

	INSIST(sock->pending_send > 0);
	sock->pending_send--;

	/* If the event is no longer in the list we can just return */
	ldev = ISC_LIST_HEAD(sock->send_list);
	while (ldev != NULL && ldev != dev) {
		ldev = ISC_LIST_NEXT(ldev, ev_link);
	}
	if (ldev == NULL)
		goto done;
	/*
	 * Try to do as much I/O as possible on this socket.  There are no
	 * limits here, currently.
	 */
	switch (completeio_send(sock, dev, messagehdr, nbytes, send_errno)) {
	case DOIO_SOFT:
		break;
	case DOIO_HARD:
	case DOIO_SUCCESS:
		send_senddone_event(sock, &dev);
		break;
	}


 done:
	UNLOCK(&sock->lock);
}

/*
 * This is the I/O Completion Port Worker Function. It loops forever
 * waiting for I/O to complete and then forwards them for further
 * processing. There are a number of these in separate threads.
 */
static isc_threadresult_t WINAPI
SocketIoThread(LPVOID ThreadContext) {
	isc_socketmgr_t *manager = ThreadContext;
	BOOL bSuccess = FALSE;
	DWORD nbytes;
	IoCompletionInfo *lpo = NULL;
	isc_socket_t *sock = NULL;
	int request;
	isc_socketevent_t *dev = NULL;
	struct msghdr *messagehdr = NULL;
	int errval;
	char strbuf[ISC_STRERRORSIZE];
	int errstatus;

	REQUIRE(VALID_MANAGER(manager));

	/*	Set the thread priority high enough so I/O will
	 *	preempt normal recv packet processing, but not
	 * 	higher than the timer sync thread.
	 */
	if (!SetThreadPriority(GetCurrentThread(),
			       THREAD_PRIORITY_ABOVE_NORMAL))
	{
		errval = GetLastError();
		isc__strerror(errval, strbuf, sizeof(strbuf));
		FATAL_ERROR(__FILE__, __LINE__,
				isc_msgcat_get(isc_msgcat, ISC_MSGSET_SOCKET,
				ISC_MSG_FAILED,
				"Can't set thread priority: %s"),
				strbuf);
	}

	/*
	 * Loop forever waiting on I/O Completions and then processing them
	 */
	while (TRUE) {
		bSuccess = GetQueuedCompletionStatus(manager->hIoCompletionPort,
						     &nbytes, (LPDWORD) &sock,
						     (LPOVERLAPPED *)&lpo,
						     INFINITE);
		if (lpo == NULL) {
			/*
			 * Received request to exit
			 */
			break;
		}
		errstatus = 0;
		if (!bSuccess) {
			isc_boolean_t dofree = ISC_FALSE;
			REQUIRE(VALID_SOCKET(sock));
			/*
			 * Was this the socket closed under us?
			 */
			errstatus = GetLastError();
			if (nbytes == 0 && errstatus == WSA_OPERATION_ABORTED) {
				LOCK(&sock->lock);
				switch (lpo->request_type) {
				case SOCKET_RECV:
					INSIST(sock->pending_recv > 0);
					sock->pending_recv--;
					break;
				case SOCKET_SEND:
					INSIST(sock->pending_send > 0);
					sock->pending_send--;
					break;
				}
				if (sock->pending_recv == 0 &&
				    sock->pending_send == 0 &&
				    sock->pending_free)
					dofree = ISC_TRUE;
				UNLOCK(&sock->lock);
				if (dofree)
					free_socket(&sock);
				if (lpo != NULL)
					HeapFree(hHeapHandle, 0, lpo);
				continue;
			}
		}

		request = lpo->request_type;
		dev = lpo->dev;
		messagehdr = &lpo->messagehdr;

		switch (request) {
		case SOCKET_RECV:
			internal_recv(sock, dev, messagehdr, nbytes, errstatus);
			break;
		case SOCKET_SEND:
			internal_send(sock, dev, messagehdr, nbytes, errstatus);
			break;
		}
		if (lpo != NULL)
			HeapFree(hHeapHandle, 0, lpo);
	}

	/*
	 * Exit Completion Port Thread
	 */
	manager_log(manager, TRACE,
		    isc_msgcat_get(isc_msgcat, ISC_MSGSET_GENERAL,
				   ISC_MSG_EXITING, "SocketIoThread exiting"));
	return ((isc_threadresult_t)0);
}

/*
 * This is the thread that will loop forever, waiting for an event to
 * happen.
 *
 * When the wait returns something to do, find the signaled event
 * and issue the request for the given socket
 */
static isc_threadresult_t WINAPI
event_wait(void *uap) {
	events_thread_t *evthread = uap;
	isc_socketmgr_t *manager = evthread->manager;
	int cc;
	int event_errno;
	char strbuf[ISC_STRERRORSIZE];
	isc_socket_t *wsock;
	int iEvent;
	int max_event;
	sock_event_list *evlist;
	WSANETWORKEVENTS NetworkEvents;
	int err;

	REQUIRE(evthread != NULL);
	REQUIRE(VALID_MANAGER(manager));

	/* We need to know the Id of the thread */
	evthread->thread_id = GetCurrentThreadId();

	evlist = &(evthread->sockev_list);

	/* See if there's anything waiting to add to the event list */
	if (manager->event_written > 0)
		process_eventlist(evlist, manager);

	while (!manager->bShutdown) {
		do {

			max_event = evlist->max_event;
			event_errno = 0;

			WSAResetEvent(evlist->aEventList[0]);
			cc = WSAWaitForMultipleEvents(max_event,
					evlist->aEventList, FALSE, WSA_INFINITE,
					FALSE);
			if (cc == WSA_WAIT_FAILED) {
				event_errno = WSAGetLastError();
				if (!SOFT_ERROR(event_errno)) {
					isc__strerror(event_errno, strbuf,
					      sizeof(strbuf));
					FATAL_ERROR(__FILE__, __LINE__,
					   "WSAWaitForMultipleEvents() %s: %s",
					    isc_msgcat_get(isc_msgcat,
						    ISC_MSGSET_GENERAL,
						    ISC_MSG_FAILED,
						    "failed"),
					    strbuf);
				}
			}

		} while (cc < 0 && !manager->bShutdown
			 && manager->event_written == 0);

		if (manager->bShutdown)
			break;

		iEvent = cc - WSA_WAIT_EVENT_0;

		/*
		 * Add or delete events as requested
		 */
		if (manager->event_written > 0)
			process_eventlist(evlist, manager);
		/*
		 * Stopped to add and delete events on the list
		 */
		if (iEvent == 0)
			continue;

		wsock = evlist->aSockList[iEvent];
		if (wsock == NULL)
			continue;

		if (WSAEnumNetworkEvents(wsock->fd, wsock->hEvent,
			&NetworkEvents) == SOCKET_ERROR) {
			err = WSAGetLastError();
			isc__strerror(err, strbuf, sizeof(strbuf));
			UNEXPECTED_ERROR(__FILE__, __LINE__,
					 "event_wait: WSAEnumNetworkEvents() %s",
					 strbuf);
			/* XXXMPA */
		}

		if(NetworkEvents.lNetworkEvents == 0 ) {
			continue;
		}

		/*
		 * Check for FD_CLOSE events first. This takes precedence over
		 * other possible events as it needs to be handled instead of
		 * any other event if it happens on the socket.
		 * The error code found, if any, is fed into the internal_*()
		 * routines.
		 */
		if(NetworkEvents.lNetworkEvents & FD_CLOSE) {
			event_errno = NetworkEvents.iErrorCode[FD_CLOSE_BIT];
		} else if (NetworkEvents.lNetworkEvents & FD_ACCEPT) {
			event_errno = NetworkEvents.iErrorCode[FD_ACCEPT_BIT];
		} else if (NetworkEvents.lNetworkEvents & FD_CONNECT) {
			event_errno = NetworkEvents.iErrorCode[FD_CONNECT_BIT];
		} else {
			UNEXPECTED_ERROR(__FILE__, __LINE__,
					 "event_wait: WSAEnumNetworkEvents() "
					 "unexpected event bit set: %0x",
					 NetworkEvents.lNetworkEvents);
		}

		if (wsock->references > 0 && wsock->pending_close == 0) {
			if (wsock->listener == 1 &&
			    wsock->pending_accept == 0) {
				wsock->pending_accept = 1;
				internal_accept(wsock, event_errno);
			}
			else {
				internal_connect(wsock, event_errno);
			}
		}
	}

	manager_log(manager, TRACE,
		    isc_msgcat_get(isc_msgcat, ISC_MSGSET_GENERAL,
				   ISC_MSG_EXITING, "event_wait exiting"));

	return ((isc_threadresult_t)0);
}

/*
 * Create a new socket manager.
 */
isc_result_t
isc_socketmgr_create(isc_mem_t *mctx, isc_socketmgr_t **managerp) {
	isc_socketmgr_t *manager;
	events_thread_t *evthread = NULL;
	isc_result_t result;

	REQUIRE(managerp != NULL && *managerp == NULL);

	manager = isc_mem_get(mctx, sizeof(*manager));
	if (manager == NULL)
		return (ISC_R_NOMEMORY);

	manager->magic = SOCKET_MANAGER_MAGIC;
	manager->mctx = NULL;
	ISC_LIST_INIT(manager->socklist);
	if (isc_mutex_init(&manager->lock) != ISC_R_SUCCESS) {
		isc_mem_put(mctx, manager, sizeof(*manager));
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "isc_mutex_init() %s",
				 isc_msgcat_get(isc_msgcat, ISC_MSGSET_GENERAL,
						ISC_MSG_FAILED, "failed"));
		return (ISC_R_UNEXPECTED);
	}
	if (isc_condition_init(&manager->shutdown_ok) != ISC_R_SUCCESS) {
		DESTROYLOCK(&manager->lock);
		isc_mem_put(mctx, manager, sizeof(*manager));
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "isc_condition_init() %s",
				 isc_msgcat_get(isc_msgcat, ISC_MSGSET_GENERAL,
						ISC_MSG_FAILED, "failed"));
		return (ISC_R_UNEXPECTED);
	}

	isc_mem_attach(mctx, &manager->mctx);

	iocompletionport_init(manager);	/* Create the Completion Ports */

	/*
	 * Event Wait Thread Initialization
	 */
	ISC_LIST_INIT(manager->ev_threads);

	/*
	 * Start up the initial event wait thread.
	 */
	result = event_thread_create(&evthread, manager);
	if (result != ISC_R_SUCCESS) {
		DESTROYLOCK(&manager->lock);
		isc_mem_put(mctx, manager, sizeof(*manager));
		return (result);
	}

	manager->prime_alert = evthread->sockev_list.aEventList[0];
	manager->event_written = 0;
	manager->bShutdown = ISC_FALSE;

	/* Initialize the event update list */
	ISC_LIST_INIT(manager->event_updates);

	*managerp = manager;

	return (ISC_R_SUCCESS);
}

void
isc_socketmgr_destroy(isc_socketmgr_t **managerp) {
	isc_socketmgr_t *manager;
	int i;
	isc_mem_t *mctx;
	events_thread_t *evthread;

	/*
	 * Destroy a socket manager.
	 */

	REQUIRE(managerp != NULL);
	manager = *managerp;
	REQUIRE(VALID_MANAGER(manager));

	LOCK(&manager->lock);

	/*
	 * Wait for all sockets to be destroyed.
	 */
	while (!ISC_LIST_EMPTY(manager->socklist)) {
		manager_log(manager, CREATION,
			    isc_msgcat_get(isc_msgcat, ISC_MSGSET_SOCKET,
					   ISC_MSG_SOCKETSREMAIN,
					   "sockets exist"));
		WAIT(&manager->shutdown_ok, &manager->lock);
	}

	UNLOCK(&manager->lock);

	/*
	 * Here, we need to had some wait code for the completion port
	 * thread.
	 */
	signal_iocompletionport_exit(manager);
	manager->bShutdown = ISC_TRUE;

	/*
	 * Wait for threads to exit.
	 */

	/*
	 * Shut down the event wait threads
	 */
	evthread = ISC_LIST_HEAD(manager->ev_threads);
	while (evthread != NULL) {
		WSASetEvent(evthread->sockev_list.aEventList[0]);
		if (isc_thread_join(evthread->thread_handle, NULL) != ISC_R_SUCCESS)
			UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "isc_thread_join() for event_wait %s",
				 isc_msgcat_get(isc_msgcat, ISC_MSGSET_GENERAL,
						ISC_MSG_FAILED, "failed"));
		ISC_LIST_DEQUEUE(manager->ev_threads, evthread, link);
		isc_mem_put(manager->mctx, evthread, sizeof(*evthread));
		evthread = ISC_LIST_HEAD(manager->ev_threads);
	}

	/*
	 * Now the I/O Completion Port Worker Threads
	 */
	for (i = 0; i < manager->maxIOCPThreads; i++) {
		if (isc_thread_join((isc_thread_t) manager->hIOCPThreads[i], NULL)
		    != ISC_R_SUCCESS)
			UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "isc_thread_join() for Completion Port %s",
				 isc_msgcat_get(isc_msgcat, ISC_MSGSET_GENERAL,
						ISC_MSG_FAILED, "failed"));
	}
	/*
	 * Clean up.
	 */

	CloseHandle(manager->hIoCompletionPort);

	(void)isc_condition_destroy(&manager->shutdown_ok);

	DESTROYLOCK(&manager->lock);
	manager->magic = 0;
	mctx= manager->mctx;
	isc_mem_put(mctx, manager, sizeof(*manager));

	isc_mem_detach(&mctx);

	*managerp = NULL;
}

static isc_result_t
socket_recv(isc_socket_t *sock, isc_socketevent_t *dev, isc_task_t *task,
	    unsigned int flags)
{
	int io_state;
	int cc = 0;
	isc_task_t *ntask = NULL;
	isc_result_t result = ISC_R_SUCCESS;
	int recv_errno = 0;

	dev->ev_sender = task;

	LOCK(&sock->lock);
	iocompletionport_update(sock);
	io_state = startio_recv(sock, dev, &cc, &recv_errno);

	switch (io_state) {
	case DOIO_PENDING:	/* I/O Started. Nothing to be done */
	case DOIO_SOFT:
		/*
		 * We couldn't read all or part of the request right now, so
		 * queue it.
		 *
		 * Attach to socket and to task
		 */
		isc_task_attach(task, &ntask);
		dev->attributes |= ISC_SOCKEVENTATTR_ATTACHED;

		/*
		 * Enqueue the request.
		 */
		ISC_LIST_ENQUEUE(sock->recv_list, dev, ev_link);

		socket_log(sock, NULL, EVENT, NULL, 0, 0,
			   "socket_recv: event %p -> task %p",
			   dev, ntask);

		if ((flags & ISC_SOCKFLAG_IMMEDIATE) != 0)
			result = ISC_R_INPROGRESS;
		break;

	case DOIO_EOF:
		dev->result = ISC_R_EOF;
		/* fallthrough */

	case DOIO_HARD:
	case DOIO_SUCCESS:
		if ((flags & ISC_SOCKFLAG_IMMEDIATE) == 0)
			send_recvdone_event(sock, &dev);
		break;
	}
	UNLOCK(&sock->lock);

	return (result);
}

isc_result_t
isc_socket_recvv(isc_socket_t *sock, isc_bufferlist_t *buflist,
		 unsigned int minimum, isc_task_t *task,
		 isc_taskaction_t action, const void *arg)
{
	isc_socketevent_t *dev;
	isc_socketmgr_t *manager;
	unsigned int iocount;
	isc_buffer_t *buffer;

	REQUIRE(VALID_SOCKET(sock));
	REQUIRE(buflist != NULL);
	REQUIRE(!ISC_LIST_EMPTY(*buflist));
	REQUIRE(task != NULL);
	REQUIRE(action != NULL);

	manager = sock->manager;
	REQUIRE(VALID_MANAGER(manager));

	iocount = isc_bufferlist_availablecount(buflist);
	REQUIRE(iocount > 0);

	INSIST(sock->bound);

	dev = allocate_socketevent(sock, ISC_SOCKEVENT_RECVDONE, action, arg);
	if (dev == NULL) {
		return (ISC_R_NOMEMORY);
	}

	/*
	 * UDP sockets are always partial read
	 */
	if (sock->type == isc_sockettype_udp)
		dev->minimum = 1;
	else {
		if (minimum == 0)
			dev->minimum = iocount;
		else
			dev->minimum = minimum;
	}

	/*
	 * Move each buffer from the passed in list to our internal one.
	 */
	buffer = ISC_LIST_HEAD(*buflist);
	while (buffer != NULL) {
		ISC_LIST_DEQUEUE(*buflist, buffer, link);
		ISC_LIST_ENQUEUE(dev->bufferlist, buffer, link);
		buffer = ISC_LIST_HEAD(*buflist);
	}

	return (socket_recv(sock, dev, task, 0));
}

isc_result_t
isc_socket_recv(isc_socket_t *sock, isc_region_t *region, unsigned int minimum,
		isc_task_t *task, isc_taskaction_t action, const void *arg)
{
	isc_socketevent_t *dev;
	isc_socketmgr_t *manager;

	REQUIRE(VALID_SOCKET(sock));
	REQUIRE(action != NULL);

	manager = sock->manager;
	REQUIRE(VALID_MANAGER(manager));

	INSIST(sock->bound);

	dev = allocate_socketevent(sock, ISC_SOCKEVENT_RECVDONE, action, arg);
	if (dev == NULL)
		return (ISC_R_NOMEMORY);

	return (isc_socket_recv2(sock, region, minimum, task, dev, 0));
}

isc_result_t
isc_socket_recv2(isc_socket_t *sock, isc_region_t *region,
		 unsigned int minimum, isc_task_t *task,
		 isc_socketevent_t *event, unsigned int flags)
{
	event->ev_sender = sock;
	event->result = ISC_R_UNEXPECTED;
	ISC_LIST_INIT(event->bufferlist);
	event->region = *region;
	event->n = 0;
	event->offset = 0;
	event->attributes = 0;

	/*
	 * UDP sockets are always partial read.
	 */
	if (sock->type == isc_sockettype_udp)
		event->minimum = 1;
	else {
		if (minimum == 0)
			event->minimum = region->length;
		else
			event->minimum = minimum;
	}

	return (socket_recv(sock, event, task, flags));
}

static isc_result_t
socket_send(isc_socket_t *sock, isc_socketevent_t *dev, isc_task_t *task,
	    isc_sockaddr_t *address, struct in6_pktinfo *pktinfo,
	    unsigned int flags)
{
	int io_state;
	int send_errno = 0;
	int cc = 0;
	isc_boolean_t have_lock = ISC_FALSE;
	isc_task_t *ntask = NULL;
	isc_result_t result = ISC_R_SUCCESS;

	dev->ev_sender = task;

	set_dev_address(address, sock, dev);
	if (pktinfo != NULL) {
		socket_log(sock, NULL, TRACE, isc_msgcat, ISC_MSGSET_SOCKET,
			   ISC_MSG_PKTINFOPROVIDED,
			   "pktinfo structure provided, ifindex %u (set to 0)",
			   pktinfo->ipi6_ifindex);

		dev->attributes |= ISC_SOCKEVENTATTR_PKTINFO;
		dev->pktinfo = *pktinfo;
		/*
		 * Set the pktinfo index to 0 here, to let the kernel decide
		 * what interface it should send on.
		 */
		dev->pktinfo.ipi6_ifindex = 0;
	}

	LOCK(&sock->lock);
	have_lock = ISC_TRUE;
	iocompletionport_update(sock);
	io_state = startio_send(sock, dev, &cc, &send_errno);

	switch (io_state) {
	case DOIO_PENDING:	/* I/O started. Nothing more to do */
	case DOIO_SOFT:
		/*
		 * We couldn't send all or part of the request right now, so
		 * queue it unless ISC_SOCKFLAG_NORETRY is set.
		 */
		if ((flags & ISC_SOCKFLAG_NORETRY) == 0) {
			isc_task_attach(task, &ntask);
			dev->attributes |= ISC_SOCKEVENTATTR_ATTACHED;
			if (!have_lock) {
				LOCK(&sock->lock);
				have_lock = ISC_TRUE;
			}

			/*
			 * Enqueue the request.
			 */
			ISC_LIST_ENQUEUE(sock->send_list, dev, ev_link);

			socket_log(sock, NULL, EVENT, NULL, 0, 0,
				   "socket_send: event %p -> task %p",
				   dev, ntask);

			if ((flags & ISC_SOCKFLAG_IMMEDIATE) != 0)
				result = ISC_R_INPROGRESS;
			break;
		}

	case DOIO_SUCCESS:
		break;
	}

	if (have_lock)
		UNLOCK(&sock->lock);

	return (result);
}

isc_result_t
isc_socket_send(isc_socket_t *sock, isc_region_t *region,
		isc_task_t *task, isc_taskaction_t action, const void *arg)
{
	/*
	 * REQUIRE() checking is performed in isc_socket_sendto().
	 */
	return (isc_socket_sendto(sock, region, task, action, arg, NULL,
				  NULL));
}

isc_result_t
isc_socket_sendto(isc_socket_t *sock, isc_region_t *region,
		  isc_task_t *task, isc_taskaction_t action, const void *arg,
		  isc_sockaddr_t *address, struct in6_pktinfo *pktinfo)
{
	isc_socketevent_t *dev;
	isc_socketmgr_t *manager;

	REQUIRE(VALID_SOCKET(sock));
	REQUIRE(region != NULL);
	REQUIRE(task != NULL);
	REQUIRE(action != NULL);

	manager = sock->manager;
	REQUIRE(VALID_MANAGER(manager));

	INSIST(sock->bound);

	dev = allocate_socketevent(sock, ISC_SOCKEVENT_SENDDONE, action, arg);
	if (dev == NULL) {
		return (ISC_R_NOMEMORY);
	}
	dev->region = *region;

	return (socket_send(sock, dev, task, address, pktinfo, 0));
}

isc_result_t
isc_socket_sendv(isc_socket_t *sock, isc_bufferlist_t *buflist,
		 isc_task_t *task, isc_taskaction_t action, const void *arg)
{
	return (isc_socket_sendtov(sock, buflist, task, action, arg, NULL,
				   NULL));
}

isc_result_t
isc_socket_sendtov(isc_socket_t *sock, isc_bufferlist_t *buflist,
		   isc_task_t *task, isc_taskaction_t action, const void *arg,
		   isc_sockaddr_t *address, struct in6_pktinfo *pktinfo)
{
	isc_socketevent_t *dev;
	isc_socketmgr_t *manager;
	unsigned int iocount;
	isc_buffer_t *buffer;

	REQUIRE(VALID_SOCKET(sock));
	REQUIRE(buflist != NULL);
	REQUIRE(!ISC_LIST_EMPTY(*buflist));
	REQUIRE(task != NULL);
	REQUIRE(action != NULL);

	manager = sock->manager;
	REQUIRE(VALID_MANAGER(manager));

	iocount = isc_bufferlist_usedcount(buflist);
	REQUIRE(iocount > 0);

	dev = allocate_socketevent(sock, ISC_SOCKEVENT_SENDDONE, action, arg);
	if (dev == NULL) {
		return (ISC_R_NOMEMORY);
	}

	/*
	 * Move each buffer from the passed in list to our internal one.
	 */
	buffer = ISC_LIST_HEAD(*buflist);
	while (buffer != NULL) {
		ISC_LIST_DEQUEUE(*buflist, buffer, link);
		ISC_LIST_ENQUEUE(dev->bufferlist, buffer, link);
		buffer = ISC_LIST_HEAD(*buflist);
	}

	return (socket_send(sock, dev, task, address, pktinfo, 0));
}

isc_result_t
isc_socket_sendto2(isc_socket_t *sock, isc_region_t *region,
		   isc_task_t *task,
		   isc_sockaddr_t *address, struct in6_pktinfo *pktinfo,
		   isc_socketevent_t *event, unsigned int flags)
{
	REQUIRE((flags & ~(ISC_SOCKFLAG_IMMEDIATE|ISC_SOCKFLAG_NORETRY)) == 0);
	if ((flags & ISC_SOCKFLAG_NORETRY) != 0)
		REQUIRE(sock->type == isc_sockettype_udp);
	event->ev_sender = sock;
	event->result = ISC_R_UNEXPECTED;
	ISC_LIST_INIT(event->bufferlist);
	event->region = *region;
	event->n = 0;
	event->offset = 0;
	event->attributes = 0;

	return (socket_send(sock, event, task, address, pktinfo, flags));
}

isc_result_t
isc_socket_bind(isc_socket_t *sock, isc_sockaddr_t *sockaddr) {
	int bind_errno;
	char strbuf[ISC_STRERRORSIZE];
	int on = 1;

	LOCK(&sock->lock);

	INSIST(!sock->bound);

	if (sock->pf != sockaddr->type.sa.sa_family) {
		UNLOCK(&sock->lock);
		return (ISC_R_FAMILYMISMATCH);
	}
	/*
	 * Only set SO_REUSEADDR when we want a specific port.
	 */
	if (isc_sockaddr_getport(sockaddr) != (in_port_t)0 &&
	    setsockopt(sock->fd, SOL_SOCKET, SO_REUSEADDR, (void *)&on,
		       sizeof(on)) < 0) {
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "setsockopt(%d) %s", sock->fd,
				 isc_msgcat_get(isc_msgcat, ISC_MSGSET_GENERAL,
						ISC_MSG_FAILED, "failed"));
		/* Press on... */
	}
	if (bind(sock->fd, &sockaddr->type.sa, sockaddr->length) < 0) {
		bind_errno = WSAGetLastError();
		UNLOCK(&sock->lock);
		switch (bind_errno) {
		case WSAEACCES:
			return (ISC_R_NOPERM);
		case WSAEADDRNOTAVAIL:
			return (ISC_R_ADDRNOTAVAIL);
		case WSAEADDRINUSE:
			return (ISC_R_ADDRINUSE);
		case WSAEINVAL:
			return (ISC_R_BOUND);
		default:
			isc__strerror(bind_errno, strbuf, sizeof(strbuf));
			UNEXPECTED_ERROR(__FILE__, __LINE__, "bind: %s",
					 strbuf);
			return (ISC_R_UNEXPECTED);
		}
	}

	socket_log(sock, sockaddr, TRACE,
		   isc_msgcat, ISC_MSGSET_SOCKET, ISC_MSG_BOUND, "bound");
	sock->bound = 1;

	UNLOCK(&sock->lock);
	return (ISC_R_SUCCESS);
}

isc_result_t
isc_socket_filter(isc_socket_t *sock, const char *filter) {
	UNUSED(sock);
	UNUSED(filter);

	REQUIRE(VALID_SOCKET(sock));
	return (ISC_R_NOTIMPLEMENTED);
}

/*
 * Set up to listen on a given socket.  We do this by creating an internal
 * event that will be dispatched when the socket has read activity.  The
 * watcher will send the internal event to the task when there is a new
 * connection.
 *
 * Unlike in read, we don't preallocate a done event here.  Every time there
 * is a new connection we'll have to allocate a new one anyway, so we might
 * as well keep things simple rather than having to track them.
 */
isc_result_t
isc_socket_listen(isc_socket_t *sock, unsigned int backlog) {
	char strbuf[ISC_STRERRORSIZE];
	isc_result_t retstat;

	REQUIRE(VALID_SOCKET(sock));

	LOCK(&sock->lock);

	REQUIRE(!sock->listener);
	REQUIRE(sock->bound);
	REQUIRE(sock->type == isc_sockettype_tcp);

	if (backlog == 0)
		backlog = SOMAXCONN;

	if (listen(sock->fd, (int)backlog) < 0) {
		UNLOCK(&sock->lock);
		isc__strerror(WSAGetLastError(), strbuf, sizeof(strbuf));

		UNEXPECTED_ERROR(__FILE__, __LINE__, "listen: %s", strbuf);

		return (ISC_R_UNEXPECTED);
	}

	sock->listener = 1;

	/* Add the socket to the list of events to accept */
	retstat = socket_event_add(sock, FD_CLOSE);
	if (retstat != ISC_R_SUCCESS) {
		UNLOCK(&sock->lock);
		if (retstat != ISC_R_NOSPACE) {
			isc__strerror(WSAGetLastError(), strbuf,
					sizeof(strbuf));
			UNEXPECTED_ERROR(__FILE__, __LINE__,
				"isc_socket_listen: socket_event_add: %s", strbuf);
		}
		return (retstat);
	}

	UNLOCK(&sock->lock);
	return (ISC_R_SUCCESS);
}

/*
 * This should try to do agressive accept() XXXMLG
 */
isc_result_t
isc_socket_accept(isc_socket_t *sock,
		  isc_task_t *task, isc_taskaction_t action, const void *arg)
{
	isc_socket_newconnev_t *dev;
	isc_socketmgr_t *manager;
	isc_task_t *ntask = NULL;
	isc_socket_t *nsock;
	isc_result_t ret;

	REQUIRE(VALID_SOCKET(sock));
	manager = sock->manager;
	REQUIRE(VALID_MANAGER(manager));

	LOCK(&sock->lock);

	REQUIRE(sock->listener);

	/*
	 * Sender field is overloaded here with the task we will be sending
	 * this event to.  Just before the actual event is delivered the
	 * actual ev_sender will be touched up to be the socket.
	 */
	dev = (isc_socket_newconnev_t *)
		isc_event_allocate(manager->mctx, task, ISC_SOCKEVENT_NEWCONN,
				   action, arg, sizeof(*dev));
	if (dev == NULL) {
		UNLOCK(&sock->lock);
		return (ISC_R_NOMEMORY);
	}
	ISC_LINK_INIT(dev, ev_link);

	ret = allocate_socket(manager, sock->type, &nsock);
	if (ret != ISC_R_SUCCESS) {
		isc_event_free((isc_event_t **)&dev);
		UNLOCK(&sock->lock);
		return (ret);
	}

	/*
	 * Attach to socket and to task.
	 */
	isc_task_attach(task, &ntask);
	nsock->references++;

	dev->ev_sender = ntask;
	dev->newsocket = nsock;

	/*
	 * Wait for connects.
	 */
	if (ISC_LIST_EMPTY(sock->accept_list) &&
	    WSAEventSelect(sock->fd, sock->hEvent, FD_ACCEPT | FD_CLOSE) != 0) {
		char strbuf[ISC_STRERRORSIZE];
		int stat;
		const char *msg;
		stat = WSAGetLastError();
		isc__strerror(stat, strbuf, sizeof(strbuf));
		msg = isc_msgcat_get(isc_msgcat, ISC_MSGSET_GENERAL,
				     ISC_MSG_FAILED, "failed");
		UNEXPECTED_ERROR(__FILE__, __LINE__, "WSAEventSelect: %s: %s",
				 msg, strbuf);
		isc_task_detach(&ntask);
		isc_socket_detach(&nsock);
		isc_event_free((isc_event_t **)&dev);
		UNLOCK(&sock->lock);
		return (ISC_R_UNEXPECTED);
	}
	/*
	 * Enqueue the event
	 */
	ISC_LIST_ENQUEUE(sock->accept_list, dev, ev_link);

	UNLOCK(&sock->lock);
	return (ISC_R_SUCCESS);
}

isc_result_t
isc_socket_connect(isc_socket_t *sock, isc_sockaddr_t *addr,
		   isc_task_t *task, isc_taskaction_t action, const void *arg)
{
	isc_socket_connev_t *dev;
	isc_task_t *ntask = NULL;
	isc_socketmgr_t *manager;
	int cc;
	int retstat;
	int errval;
	char strbuf[ISC_STRERRORSIZE];

	REQUIRE(VALID_SOCKET(sock));
	REQUIRE(addr != NULL);
	REQUIRE(task != NULL);
	REQUIRE(action != NULL);

	manager = sock->manager;
	REQUIRE(VALID_MANAGER(manager));
	REQUIRE(addr != NULL);

	if (isc_sockaddr_ismulticast(addr))
		return (ISC_R_MULTICAST);

	LOCK(&sock->lock);

	REQUIRE(!sock->connecting);

	dev = (isc_socket_connev_t *)isc_event_allocate(manager->mctx, sock,
							ISC_SOCKEVENT_CONNECT,
							action,	arg,
							sizeof(*dev));
	if (dev == NULL) {
		UNLOCK(&sock->lock);
		return (ISC_R_NOMEMORY);
	}
	ISC_LINK_INIT(dev, ev_link);

	/*
	 * Try to do the connect right away, as there can be only one
	 * outstanding, and it might happen to complete.
	 */
	sock->address = *addr;
	cc = connect(sock->fd, &addr->type.sa, addr->length);
	if (cc < 0) {
		errval = WSAGetLastError();
		if (SOFT_ERROR(errval) || errval == WSAEINPROGRESS)
			goto queue;

		switch (errval) {
#define ERROR_MATCH(a, b) case a: dev->result = b; goto err_exit;
			ERROR_MATCH(WSAEACCES, ISC_R_NOPERM);
			ERROR_MATCH(WSAEADDRNOTAVAIL, ISC_R_ADDRNOTAVAIL);
			ERROR_MATCH(WSAEAFNOSUPPORT, ISC_R_ADDRNOTAVAIL);
			ERROR_MATCH(WSAECONNREFUSED, ISC_R_CONNREFUSED);
			ERROR_MATCH(WSAEHOSTUNREACH, ISC_R_HOSTUNREACH);
			ERROR_MATCH(WSAEHOSTDOWN, ISC_R_HOSTUNREACH);
			ERROR_MATCH(WSAENETUNREACH, ISC_R_NETUNREACH);
			ERROR_MATCH(WSAENOBUFS, ISC_R_NORESOURCES);
			ERROR_MATCH(EPERM, ISC_R_HOSTUNREACH);
			ERROR_MATCH(EPIPE, ISC_R_NOTCONNECTED);
#undef ERROR_MATCH
		}

		sock->connected = 0;

		isc__strerror(errval, strbuf, sizeof(strbuf));
		UNEXPECTED_ERROR(__FILE__, __LINE__, "%d/%s", errval, strbuf);

		UNLOCK(&sock->lock);
		isc_event_free((isc_event_t **)&dev);
		return (ISC_R_UNEXPECTED);

	err_exit:
		sock->connected = 0;
		isc_task_send(task, (isc_event_t **)&dev);

		UNLOCK(&sock->lock);
		return (ISC_R_SUCCESS);
	}

	/*
	 * If connect completed, fire off the done event.
	 */
	if (cc == 0) {
		sock->connected = 1;
		sock->bound = 1;
		dev->result = ISC_R_SUCCESS;
		isc_task_send(task, (isc_event_t **)&dev);

		UNLOCK(&sock->lock);
		return (ISC_R_SUCCESS);
	}

 queue:

	/*
	 * Attach to task.
	 */
	isc_task_attach(task, &ntask);

	sock->connecting = 1;

	dev->ev_sender = ntask;

	/*
	 * Enqueue the request.
	 */
	sock->connect_ev = dev;
	/* Add the socket to the list of events to connect */
	retstat = socket_event_add(sock, FD_CONNECT | FD_CLOSE);
	if (retstat != ISC_R_SUCCESS) {
		UNLOCK(&sock->lock);
		if (retstat != ISC_R_NOSPACE) {
			isc__strerror(WSAGetLastError(), strbuf,
					sizeof(strbuf));
			UNEXPECTED_ERROR(__FILE__, __LINE__,
				"isc_socket_connect: socket_event_add: %s", strbuf);
		}
		return (retstat);
	}

	UNLOCK(&sock->lock);
	return (ISC_R_SUCCESS);
}

isc_result_t
isc_socket_getpeername(isc_socket_t *sock, isc_sockaddr_t *addressp) {
	isc_result_t ret;

	REQUIRE(VALID_SOCKET(sock));
	REQUIRE(addressp != NULL);

	LOCK(&sock->lock);

	if (sock->connected) {
		*addressp = sock->address;
		ret = ISC_R_SUCCESS;
	} else {
		ret = ISC_R_NOTCONNECTED;
	}

	UNLOCK(&sock->lock);

	return (ret);
}

isc_result_t
isc_socket_getsockname(isc_socket_t *sock, isc_sockaddr_t *addressp) {
	ISC_SOCKADDR_LEN_T len;
	isc_result_t ret;
	char strbuf[ISC_STRERRORSIZE];

	REQUIRE(VALID_SOCKET(sock));
	REQUIRE(addressp != NULL);

	LOCK(&sock->lock);

	if (!sock->bound) {
		ret = ISC_R_NOTBOUND;
		goto out;
	}

	ret = ISC_R_SUCCESS;

	len = sizeof(addressp->type);
	if (getsockname(sock->fd, &addressp->type.sa, (void *)&len) < 0) {
		isc__strerror(WSAGetLastError(), strbuf, sizeof(strbuf));
		UNEXPECTED_ERROR(__FILE__, __LINE__, "getsockname: %s",
				 strbuf);
		ret = ISC_R_UNEXPECTED;
		goto out;
	}
	addressp->length = (unsigned int)len;

 out:
	UNLOCK(&sock->lock);

	return (ret);
}

/*
 * Run through the list of events on this socket, and cancel the ones
 * queued for task "task" of type "how".  "how" is a bitmask.
 */
void
isc_socket_cancel(isc_socket_t *sock, isc_task_t *task, unsigned int how) {

	REQUIRE(VALID_SOCKET(sock));

	/*
	 * Quick exit if there is nothing to do.  Don't even bother locking
	 * in this case.
	 */
	if (how == 0)
		return;

	LOCK(&sock->lock);

	/*
	 * All of these do the same thing, more or less.
	 * Each will:
	 *	o If the internal event is marked as "posted" try to
	 *	  remove it from the task's queue.  If this fails, mark it
	 *	  as canceled instead, and let the task clean it up later.
	 *	o For each I/O request for that task of that type, post
	 *	  its done event with status of "ISC_R_CANCELED".
	 *	o Reset any state needed.
	 */
	if (((how & ISC_SOCKCANCEL_RECV) == ISC_SOCKCANCEL_RECV)
	    && !ISC_LIST_EMPTY(sock->recv_list)) {
		isc_socketevent_t      *dev;
		isc_socketevent_t      *next;
		isc_task_t	       *current_task;

		dev = ISC_LIST_HEAD(sock->recv_list);

		while (dev != NULL) {
			current_task = dev->ev_sender;
			next = ISC_LIST_NEXT(dev, ev_link);
			if ((task == NULL) || (task == current_task)) {
				dev->result = ISC_R_CANCELED;
				send_recvdone_event(sock, &dev);
			}
			dev = next;
		}
	}

	if (((how & ISC_SOCKCANCEL_SEND) == ISC_SOCKCANCEL_SEND)
	    && !ISC_LIST_EMPTY(sock->send_list)) {
		isc_socketevent_t      *dev;
		isc_socketevent_t      *next;
		isc_task_t	       *current_task;

		dev = ISC_LIST_HEAD(sock->send_list);

		while (dev != NULL) {
			current_task = dev->ev_sender;
			next = ISC_LIST_NEXT(dev, ev_link);
			if ((task == NULL) || (task == current_task)) {
				dev->result = ISC_R_CANCELED;
				send_senddone_event(sock, &dev);
			}
			dev = next;
		}
	}

	if (((how & ISC_SOCKCANCEL_ACCEPT) == ISC_SOCKCANCEL_ACCEPT)
	    && !ISC_LIST_EMPTY(sock->accept_list)) {
		isc_socket_newconnev_t *dev;
		isc_socket_newconnev_t *next;
		isc_task_t	       *current_task;

		dev = ISC_LIST_HEAD(sock->accept_list);

		while (dev != NULL) {
			current_task = dev->ev_sender;
			next = ISC_LIST_NEXT(dev, ev_link);

			if ((task == NULL) || (task == current_task)) {

				ISC_LIST_UNLINK(sock->accept_list, dev,
						ev_link);

				dev->newsocket->references--;
				free_socket(&dev->newsocket);

				dev->result = ISC_R_CANCELED;
				dev->ev_sender = sock;
				isc_task_sendanddetach(&current_task,
						       (isc_event_t **)&dev);
			}

			dev = next;
		}
		if (sock->hEvent != NULL &&
		    WSAEventSelect(sock->fd, sock->hEvent, FD_CLOSE) != 0) {
			char strbuf[ISC_STRERRORSIZE];
			int stat;
			const char *msg;
			stat = WSAGetLastError();
			isc__strerror(stat, strbuf, sizeof(strbuf));
			msg = isc_msgcat_get(isc_msgcat, ISC_MSGSET_GENERAL,
					     ISC_MSG_FAILED, "failed");
			UNEXPECTED_ERROR(__FILE__, __LINE__,
					 "WSAEventSelect: %s: %s", msg, strbuf);
		}
	}

	/*
	 * Connecting is not a list.
	 */
	if (((how & ISC_SOCKCANCEL_CONNECT) == ISC_SOCKCANCEL_CONNECT)
	    && sock->connect_ev != NULL) {
		isc_socket_connev_t    *dev;
		isc_task_t	       *current_task;

		INSIST(sock->connecting);
		sock->connecting = 0;

		dev = sock->connect_ev;
		current_task = dev->ev_sender;

		if ((task == NULL) || (task == current_task)) {
			sock->connect_ev = NULL;

			dev->result = ISC_R_CANCELED;
			dev->ev_sender = sock;
			isc_task_sendanddetach(&current_task,
					       (isc_event_t **)&dev);
		}
	}

	UNLOCK(&sock->lock);
}

isc_sockettype_t
isc_socket_gettype(isc_socket_t *sock) {
	REQUIRE(VALID_SOCKET(sock));

	return (sock->type);
}

isc_boolean_t
isc_socket_isbound(isc_socket_t *sock) {
	isc_boolean_t val;

	LOCK(&sock->lock);
	val = ((sock->bound) ? ISC_TRUE : ISC_FALSE);
	UNLOCK(&sock->lock);

	return (val);
}

void
isc_socket_ipv6only(isc_socket_t *sock, isc_boolean_t yes) {
#if defined(IPV6_V6ONLY)
	int onoff = yes ? 1 : 0;
#else
	UNUSED(yes);
	UNUSED(sock);
#endif

	REQUIRE(VALID_SOCKET(sock));

#ifdef IPV6_V6ONLY
	if (sock->pf == AF_INET6) {
		(void)setsockopt(sock->fd, IPPROTO_IPV6, IPV6_V6ONLY,
				 (void *)&onoff, sizeof(onoff));
	}
#endif
}
