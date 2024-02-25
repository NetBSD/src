/*	$NetBSD: controlconf.c,v 1.9.2.2 2024/02/25 15:43:05 martin Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/*! \file */

#include <inttypes.h>
#include <stdbool.h>

#include <isc/app.h>
#include <isc/base64.h>
#include <isc/buffer.h>
#include <isc/event.h>
#include <isc/file.h>
#include <isc/mem.h>
#include <isc/mutex.h>
#include <isc/net.h>
#include <isc/netaddr.h>
#include <isc/netmgr.h>
#include <isc/nonce.h>
#include <isc/random.h>
#include <isc/result.h>
#include <isc/stdtime.h>
#include <isc/string.h>
#include <isc/task.h>
#include <isc/util.h>

#include <isccc/alist.h>
#include <isccc/cc.h>
#include <isccc/ccmsg.h>
#include <isccc/events.h>
#include <isccc/sexpr.h>
#include <isccc/symtab.h>
#include <isccc/util.h>

#include <isccfg/namedconf.h>

#include <bind9/check.h>

#include <named/config.h>
#include <named/control.h>
#include <named/log.h>
#include <named/server.h>

typedef struct controlkey controlkey_t;
typedef ISC_LIST(controlkey_t) controlkeylist_t;

typedef struct controlconnection controlconnection_t;
typedef ISC_LIST(controlconnection_t) controlconnectionlist_t;

typedef struct controllistener controllistener_t;
typedef ISC_LIST(controllistener_t) controllistenerlist_t;

struct controlkey {
	char *keyname;
	uint32_t algorithm;
	isc_region_t secret;
	ISC_LINK(controlkey_t) link;
};

struct controlconnection {
	isc_nmhandle_t *readhandle;
	isc_nmhandle_t *sendhandle;
	isc_nmhandle_t *cmdhandle;
	isccc_ccmsg_t ccmsg;
	bool reading;
	bool sending;
	controllistener_t *listener;
	isccc_sexpr_t *ctrl;
	isc_buffer_t *buffer;
	isc_buffer_t *text;
	isccc_sexpr_t *request;
	isccc_sexpr_t *response;
	uint32_t alg;
	isccc_region_t secret;
	uint32_t nonce;
	isc_stdtime_t now;
	isc_result_t result;
	ISC_LINK(controlconnection_t) link;
};

struct controllistener {
	named_controls_t *controls;
	isc_mem_t *mctx;
	isc_sockaddr_t address;
	isc_nmsocket_t *sock;
	dns_acl_t *acl;
	bool exiting;
	isc_refcount_t refs;
	controlkeylist_t keys;
	isc_mutex_t connections_lock;
	controlconnectionlist_t connections;
	isc_socktype_t type;
	uint32_t perm;
	uint32_t owner;
	uint32_t group;
	bool readonly;
	ISC_LINK(controllistener_t) link;
};

struct named_controls {
	named_server_t *server;
	controllistenerlist_t listeners;
	atomic_bool shuttingdown;
	isc_mutex_t symtab_lock;
	isccc_symtab_t *symtab;
};

static isc_result_t
control_newconn(isc_nmhandle_t *handle, isc_result_t result, void *arg);
static void
control_recvmessage(isc_nmhandle_t *handle, isc_result_t result, void *arg);

#define CLOCKSKEW 300

static void
free_controlkey(controlkey_t *key, isc_mem_t *mctx) {
	if (key->keyname != NULL) {
		isc_mem_free(mctx, key->keyname);
	}
	if (key->secret.base != NULL) {
		isc_mem_put(mctx, key->secret.base, key->secret.length);
	}
	isc_mem_put(mctx, key, sizeof(*key));
}

static void
free_controlkeylist(controlkeylist_t *keylist, isc_mem_t *mctx) {
	while (!ISC_LIST_EMPTY(*keylist)) {
		controlkey_t *key = ISC_LIST_HEAD(*keylist);
		ISC_LIST_UNLINK(*keylist, key, link);
		free_controlkey(key, mctx);
	}
}

static void
free_listener(controllistener_t *listener) {
	INSIST(listener->exiting);
	INSIST(ISC_LIST_EMPTY(listener->connections));

	isc_refcount_destroy(&listener->refs);

	if (listener->sock != NULL) {
		isc_nmsocket_close(&listener->sock);
	}

	free_controlkeylist(&listener->keys, listener->mctx);

	if (listener->acl != NULL) {
		dns_acl_detach(&listener->acl);
	}
	isc_mutex_destroy(&listener->connections_lock);

	isc_mem_putanddetach(&listener->mctx, listener, sizeof(*listener));
}

static void
maybe_free_listener(controllistener_t *listener) {
	if (isc_refcount_decrement(&listener->refs) == 1) {
		free_listener(listener);
	}
}

static void
shutdown_listener(controllistener_t *listener) {
	if (!listener->exiting) {
		char socktext[ISC_SOCKADDR_FORMATSIZE];

		ISC_LIST_UNLINK(listener->controls->listeners, listener, link);

		isc_sockaddr_format(&listener->address, socktext,
				    sizeof(socktext));
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_CONTROL, ISC_LOG_NOTICE,
			      "stopping command channel on %s", socktext);
#if 0
		/* XXX: no unix domain socket support */
		if (listener->type == isc_socktype_unix) {
			isc_socket_cleanunix(&listener->address, true);
		}
#endif
		listener->exiting = true;
	}

	isc_nm_stoplistening(listener->sock);
	maybe_free_listener(listener);
}

static bool
address_ok(isc_sockaddr_t *sockaddr, controllistener_t *listener) {
	dns_aclenv_t *env =
		ns_interfacemgr_getaclenv(named_g_server->interfacemgr);
	isc_netaddr_t netaddr;
	isc_result_t result;
	int match;

	/* ACL doesn't apply to unix domain sockets */
	if (listener->type != isc_socktype_tcp) {
		return (true);
	}

	isc_netaddr_fromsockaddr(&netaddr, sockaddr);

	result = dns_acl_match(&netaddr, NULL, listener->acl, env, &match,
			       NULL);
	return (result == ISC_R_SUCCESS && match > 0);
}

static void
control_senddone(isc_nmhandle_t *handle, isc_result_t result, void *arg) {
	controlconnection_t *conn = (controlconnection_t *)arg;
	controllistener_t *listener = conn->listener;
	isc_sockaddr_t peeraddr = isc_nmhandle_peeraddr(handle);

	REQUIRE(conn->sending);

	conn->sending = false;

	if (conn->result == ISC_R_SHUTTINGDOWN) {
		isc_app_shutdown();
		goto cleanup_sendhandle;
	}

	if (atomic_load_acquire(&listener->controls->shuttingdown) ||
	    result == ISC_R_SHUTTINGDOWN)
	{
		goto cleanup_sendhandle;
	} else if (result != ISC_R_SUCCESS) {
		char socktext[ISC_SOCKADDR_FORMATSIZE];

		isc_sockaddr_format(&peeraddr, socktext, sizeof(socktext));
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_CONTROL, ISC_LOG_WARNING,
			      "error sending command response to %s: %s",
			      socktext, isc_result_totext(result));
		goto cleanup_sendhandle;
	}

	isc_nmhandle_attach(handle, &conn->readhandle);
	conn->reading = true;

	isc_nmhandle_detach(&conn->sendhandle);

	isccc_ccmsg_readmessage(&conn->ccmsg, control_recvmessage, conn);
	return;

cleanup_sendhandle:
	isc_nmhandle_detach(&conn->sendhandle);
}

static void
log_invalid(isccc_ccmsg_t *ccmsg, isc_result_t result) {
	char socktext[ISC_SOCKADDR_FORMATSIZE];
	isc_sockaddr_t peeraddr = isc_nmhandle_peeraddr(ccmsg->handle);

	isc_sockaddr_format(&peeraddr, socktext, sizeof(socktext));
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_CONTROL, ISC_LOG_ERROR,
		      "invalid command from %s: %s", socktext,
		      isc_result_totext(result));
}

static void
conn_cleanup(controlconnection_t *conn) {
	controllistener_t *listener = conn->listener;

	if (conn->response != NULL) {
		isccc_sexpr_free(&conn->response);
	}
	if (conn->request != NULL) {
		isccc_sexpr_free(&conn->request);
	}
	if (conn->secret.rstart != NULL) {
		isc_mem_put(listener->mctx, conn->secret.rstart,
			    REGION_SIZE(conn->secret));
	}
	if (conn->text != NULL) {
		isc_buffer_free(&conn->text);
	}
}

static void
control_respond(isc_nmhandle_t *handle, controlconnection_t *conn) {
	controllistener_t *listener = conn->listener;
	isccc_sexpr_t *data = NULL;
	isc_buffer_t b;
	isc_region_t r;
	isc_result_t result;

	result = isccc_cc_createresponse(conn->request, conn->now,
					 conn->now + 60, &conn->response);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	if (conn->result == ISC_R_SHUTTINGDOWN) {
		result = ISC_R_SUCCESS;
	} else {
		result = conn->result;
	}

	data = isccc_alist_lookup(conn->response, "_data");
	if (data != NULL) {
		if (isccc_cc_defineuint32(data, "result", result) == NULL) {
			goto cleanup;
		}
	}

	if (result != ISC_R_SUCCESS) {
		if (data != NULL) {
			const char *estr = isc_result_totext(result);
			if (isccc_cc_definestring(data, "err", estr) == NULL) {
				goto cleanup;
			}
		}
	}

	if (isc_buffer_usedlength(conn->text) > 0) {
		if (data != NULL) {
			char *str = (char *)isc_buffer_base(conn->text);
			if (isccc_cc_definestring(data, "text", str) == NULL) {
				goto cleanup;
			}
		}
	}

	conn->ctrl = isccc_alist_lookup(conn->response, "_ctrl");
	if (conn->ctrl == NULL ||
	    isccc_cc_defineuint32(conn->ctrl, "_nonce", conn->nonce) == NULL)
	{
		goto cleanup;
	}

	if (conn->buffer == NULL) {
		isc_buffer_allocate(listener->mctx, &conn->buffer, 2 * 2048);
	}

	isc_buffer_clear(conn->buffer);
	/* Skip the length field (4 bytes) */
	isc_buffer_add(conn->buffer, 4);

	result = isccc_cc_towire(conn->response, &conn->buffer, conn->alg,
				 &conn->secret);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	isc_buffer_init(&b, conn->buffer->base, 4);
	isc_buffer_putuint32(&b, conn->buffer->used - 4);

	r.base = conn->buffer->base;
	r.length = conn->buffer->used;

	isc_nmhandle_attach(handle, &conn->sendhandle);
	conn->sending = true;
	conn_cleanup(conn);

	isc_nmhandle_detach(&conn->cmdhandle);

	isc_nm_send(conn->sendhandle, &r, control_senddone, conn);

	return;

cleanup:
	conn_cleanup(conn);
	isc_nmhandle_detach(&conn->cmdhandle);
}

static void
control_command(isc_task_t *task, isc_event_t *event) {
	controlconnection_t *conn = event->ev_arg;
	controllistener_t *listener = conn->listener;

	UNUSED(task);

	if (atomic_load_acquire(&listener->controls->shuttingdown)) {
		conn_cleanup(conn);
		isc_nmhandle_detach(&conn->cmdhandle);
		goto done;
	}

	conn->result = named_control_docommand(conn->request,
					       listener->readonly, &conn->text);
	control_respond(conn->cmdhandle, conn);

done:
	isc_event_free(&event);
}

static void
control_recvmessage(isc_nmhandle_t *handle, isc_result_t result, void *arg) {
	controlconnection_t *conn = (controlconnection_t *)arg;
	controllistener_t *listener = conn->listener;
	controlkey_t *key = NULL;
	isc_event_t *event = NULL;
	isccc_time_t sent;
	isccc_time_t exp;
	uint32_t nonce;

	conn->reading = false;

	/* Is the server shutting down? */
	if (atomic_load_acquire(&listener->controls->shuttingdown)) {
		goto cleanup_readhandle;
	}

	if (result != ISC_R_SUCCESS) {
		if (result == ISC_R_SHUTTINGDOWN) {
			atomic_store_release(&listener->controls->shuttingdown,
					     true);
		} else if (result != ISC_R_EOF) {
			log_invalid(&conn->ccmsg, result);
		}

		goto cleanup_readhandle;
	}

	for (key = ISC_LIST_HEAD(listener->keys); key != NULL;
	     key = ISC_LIST_NEXT(key, link))
	{
		isccc_region_t ccregion;

		ccregion.rstart = isc_buffer_base(conn->ccmsg.buffer);
		ccregion.rend = isc_buffer_used(conn->ccmsg.buffer);
		conn->secret.rstart = isc_mem_get(listener->mctx,
						  key->secret.length);
		memmove(conn->secret.rstart, key->secret.base,
			key->secret.length);
		conn->secret.rend = conn->secret.rstart + key->secret.length;
		conn->alg = key->algorithm;
		result = isccc_cc_fromwire(&ccregion, &conn->request, conn->alg,
					   &conn->secret);
		if (result == ISC_R_SUCCESS) {
			break;
		}
		isc_mem_put(listener->mctx, conn->secret.rstart,
			    REGION_SIZE(conn->secret));
	}

	if (key == NULL) {
		log_invalid(&conn->ccmsg, ISCCC_R_BADAUTH);
		goto cleanup;
	}

	/* We shouldn't be getting a reply. */
	if (isccc_cc_isreply(conn->request)) {
		log_invalid(&conn->ccmsg, ISC_R_FAILURE);
		goto cleanup;
	}

	isc_stdtime_get(&conn->now);

	/*
	 * Limit exposure to replay attacks.
	 */
	conn->ctrl = isccc_alist_lookup(conn->request, "_ctrl");
	if (!isccc_alist_alistp(conn->ctrl)) {
		log_invalid(&conn->ccmsg, ISC_R_FAILURE);
		goto cleanup;
	}

	if (isccc_cc_lookupuint32(conn->ctrl, "_tim", &sent) == ISC_R_SUCCESS) {
		if ((sent + CLOCKSKEW) < conn->now ||
		    (sent - CLOCKSKEW) > conn->now)
		{
			log_invalid(&conn->ccmsg, ISCCC_R_CLOCKSKEW);
			goto cleanup;
		}
	} else {
		log_invalid(&conn->ccmsg, ISC_R_FAILURE);
		goto cleanup;
	}

	/*
	 * Expire messages that are too old.
	 */
	if (isccc_cc_lookupuint32(conn->ctrl, "_exp", &exp) == ISC_R_SUCCESS &&
	    conn->now > exp)
	{
		log_invalid(&conn->ccmsg, ISCCC_R_EXPIRED);
		goto cleanup;
	}

	/*
	 * Duplicate suppression (required for UDP).
	 */
	LOCK(&listener->controls->symtab_lock);
	isccc_cc_cleansymtab(listener->controls->symtab, conn->now);
	result = isccc_cc_checkdup(listener->controls->symtab, conn->request,
				   conn->now);
	UNLOCK(&listener->controls->symtab_lock);
	if (result != ISC_R_SUCCESS) {
		if (result == ISC_R_EXISTS) {
			result = ISCCC_R_DUPLICATE;
		}
		log_invalid(&conn->ccmsg, result);
		goto cleanup;
	}

	if (conn->nonce != 0 &&
	    (isccc_cc_lookupuint32(conn->ctrl, "_nonce", &nonce) !=
		     ISC_R_SUCCESS ||
	     conn->nonce != nonce))
	{
		log_invalid(&conn->ccmsg, ISCCC_R_BADAUTH);
		goto cleanup;
	}

	isc_buffer_allocate(listener->mctx, &conn->text, 2 * 2048);

	isc_nmhandle_attach(handle, &conn->cmdhandle);
	isc_nmhandle_detach(&conn->readhandle);

	if (conn->nonce == 0) {
		/*
		 * Establish nonce.
		 */
		while (conn->nonce == 0) {
			isc_nonce_buf(&conn->nonce, sizeof(conn->nonce));
		}
		conn->result = ISC_R_SUCCESS;
		control_respond(handle, conn);
		return;
	}

	/*
	 * Trigger the command.
	 */

	event = isc_event_allocate(listener->mctx, conn, NAMED_EVENT_COMMAND,
				   control_command, conn, sizeof(isc_event_t));
	isc_task_send(named_g_server->task, &event);

	return;

cleanup:
	conn_cleanup(conn);

cleanup_readhandle:
	/*
	 * readhandle could be NULL if we're shutting down,
	 * but if not we need to detach it.
	 */
	if (conn->readhandle != NULL) {
		isc_nmhandle_detach(&conn->readhandle);
	}
}

static void
conn_reset(void *arg) {
	controlconnection_t *conn = (controlconnection_t *)arg;
	controllistener_t *listener = conn->listener;

	if (conn->buffer != NULL) {
		isc_buffer_free(&conn->buffer);
	}

	if (conn->reading) {
		isccc_ccmsg_cancelread(&conn->ccmsg);
		return;
	}

	LOCK(&listener->connections_lock);
	ISC_LIST_UNLINK(listener->connections, conn, link);
	UNLOCK(&listener->connections_lock);
#ifdef ENABLE_AFL
	if (named_g_fuzz_type == isc_fuzz_rndc) {
		named_fuzz_notify();
	}
#endif /* ifdef ENABLE_AFL */

	isccc_ccmsg_invalidate(&conn->ccmsg);
}

static void
conn_put(void *arg) {
	controlconnection_t *conn = (controlconnection_t *)arg;
	controllistener_t *listener = conn->listener;

	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_CONTROL, ISC_LOG_DEBUG(3),
		      "freeing control connection");
	maybe_free_listener(listener);
}

static void
newconnection(controllistener_t *listener, isc_nmhandle_t *handle) {
	controlconnection_t *conn = NULL;

	conn = isc_nmhandle_getdata(handle);
	if (conn == NULL) {
		conn = isc_nmhandle_getextra(handle);
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_CONTROL, ISC_LOG_DEBUG(3),
			      "allocate new control connection");
		isc_nmhandle_setdata(handle, conn, conn_reset, conn_put);
		isc_refcount_increment(&listener->refs);
	}

	*conn = (controlconnection_t){ .listener = listener,
				       .reading = false,
				       .alg = DST_ALG_UNKNOWN };

	isccc_ccmsg_init(listener->mctx, handle, &conn->ccmsg);

	/* Set a 32 KiB upper limit on incoming message. */
	isccc_ccmsg_setmaxsize(&conn->ccmsg, 32768);

	LOCK(&listener->connections_lock);
	ISC_LIST_INITANDAPPEND(listener->connections, conn, link);
	UNLOCK(&listener->connections_lock);

	isc_nmhandle_attach(handle, &conn->readhandle);
	conn->reading = true;

	isccc_ccmsg_readmessage(&conn->ccmsg, control_recvmessage, conn);
}

static isc_result_t
control_newconn(isc_nmhandle_t *handle, isc_result_t result, void *arg) {
	controllistener_t *listener = arg;
	isc_sockaddr_t peeraddr;

	if (result != ISC_R_SUCCESS) {
		if (result == ISC_R_SHUTTINGDOWN) {
			shutdown_listener(listener);
		}
		return (result);
	}

	peeraddr = isc_nmhandle_peeraddr(handle);
	if (!address_ok(&peeraddr, listener)) {
		char socktext[ISC_SOCKADDR_FORMATSIZE];
		isc_sockaddr_format(&peeraddr, socktext, sizeof(socktext));
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_CONTROL, ISC_LOG_WARNING,
			      "rejected command channel message from %s",
			      socktext);
		return (ISC_R_FAILURE);
	}

	newconnection(listener, handle);
	return (ISC_R_SUCCESS);
}

static void
controls_shutdown(named_controls_t *controls) {
	controllistener_t *listener = NULL;
	controllistener_t *next = NULL;

	for (listener = ISC_LIST_HEAD(controls->listeners); listener != NULL;
	     listener = next)
	{
		/*
		 * This is asynchronous.  As listeners shut down, they will
		 * call their callbacks.
		 */
		next = ISC_LIST_NEXT(listener, link);
		shutdown_listener(listener);
	}
}

void
named_controls_shutdown(named_controls_t *controls) {
	controls_shutdown(controls);
	atomic_store_release(&controls->shuttingdown, true);
}

static isc_result_t
cfgkeylist_find(const cfg_obj_t *keylist, const char *keyname,
		const cfg_obj_t **objp) {
	const cfg_listelt_t *element = NULL;
	const char *str = NULL;
	const cfg_obj_t *obj = NULL;

	for (element = cfg_list_first(keylist); element != NULL;
	     element = cfg_list_next(element))
	{
		obj = cfg_listelt_value(element);
		str = cfg_obj_asstring(cfg_map_getname(obj));
		if (strcasecmp(str, keyname) == 0) {
			break;
		}
	}
	if (element == NULL) {
		return (ISC_R_NOTFOUND);
	}
	obj = cfg_listelt_value(element);
	*objp = obj;
	return (ISC_R_SUCCESS);
}

static void
controlkeylist_fromcfg(const cfg_obj_t *keylist, isc_mem_t *mctx,
		       controlkeylist_t *keyids) {
	const cfg_listelt_t *element = NULL;
	char *newstr = NULL;
	const char *str = NULL;
	const cfg_obj_t *obj = NULL;
	controlkey_t *key = NULL;

	for (element = cfg_list_first(keylist); element != NULL;
	     element = cfg_list_next(element))
	{
		obj = cfg_listelt_value(element);
		str = cfg_obj_asstring(obj);
		newstr = isc_mem_strdup(mctx, str);
		key = isc_mem_get(mctx, sizeof(*key));
		key->keyname = newstr;
		key->algorithm = DST_ALG_UNKNOWN;
		key->secret.base = NULL;
		key->secret.length = 0;
		ISC_LINK_INIT(key, link);
		ISC_LIST_APPEND(*keyids, key, link);
		newstr = NULL;
	}
}

static void
register_keys(const cfg_obj_t *control, const cfg_obj_t *keylist,
	      controlkeylist_t *keyids, isc_mem_t *mctx, const char *socktext) {
	controlkey_t *keyid = NULL, *next = NULL;
	const cfg_obj_t *keydef = NULL;
	char secret[1024];
	isc_buffer_t b;
	isc_result_t result;

	/*
	 * Find the keys corresponding to the keyids used by this listener.
	 */
	for (keyid = ISC_LIST_HEAD(*keyids); keyid != NULL; keyid = next) {
		next = ISC_LIST_NEXT(keyid, link);

		result = cfgkeylist_find(keylist, keyid->keyname, &keydef);
		if (result != ISC_R_SUCCESS) {
			cfg_obj_log(control, named_g_lctx, ISC_LOG_WARNING,
				    "couldn't find key '%s' for use with "
				    "command channel %s",
				    keyid->keyname, socktext);
			ISC_LIST_UNLINK(*keyids, keyid, link);
			free_controlkey(keyid, mctx);
		} else {
			const cfg_obj_t *algobj = NULL;
			const cfg_obj_t *secretobj = NULL;
			const char *algstr = NULL;
			const char *secretstr = NULL;
			unsigned int algtype;

			(void)cfg_map_get(keydef, "algorithm", &algobj);
			(void)cfg_map_get(keydef, "secret", &secretobj);
			INSIST(algobj != NULL && secretobj != NULL);

			algstr = cfg_obj_asstring(algobj);
			secretstr = cfg_obj_asstring(secretobj);

			result = named_config_getkeyalgorithm2(algstr, NULL,
							       &algtype, NULL);
			if (result != ISC_R_SUCCESS) {
				cfg_obj_log(control, named_g_lctx,
					    ISC_LOG_WARNING,
					    "unsupported algorithm '%s' in "
					    "key '%s' for use with command "
					    "channel %s",
					    algstr, keyid->keyname, socktext);
				ISC_LIST_UNLINK(*keyids, keyid, link);
				free_controlkey(keyid, mctx);
				continue;
			}

			keyid->algorithm = algtype;
			isc_buffer_init(&b, secret, sizeof(secret));
			result = isc_base64_decodestring(secretstr, &b);

			if (result != ISC_R_SUCCESS) {
				cfg_obj_log(keydef, named_g_lctx,
					    ISC_LOG_WARNING,
					    "secret for key '%s' on "
					    "command channel %s: %s",
					    keyid->keyname, socktext,
					    isc_result_totext(result));
				ISC_LIST_UNLINK(*keyids, keyid, link);
				free_controlkey(keyid, mctx);
				continue;
			}

			keyid->secret.length = isc_buffer_usedlength(&b);
			keyid->secret.base = isc_mem_get(mctx,
							 keyid->secret.length);
			memmove(keyid->secret.base, isc_buffer_base(&b),
				keyid->secret.length);
		}
	}
}

#define CHECK(x)                               \
	do {                                   \
		result = (x);                  \
		if (result != ISC_R_SUCCESS) { \
			goto cleanup;          \
		}                              \
	} while (0)

static isc_result_t
get_rndckey(isc_mem_t *mctx, controlkeylist_t *keyids) {
	isc_result_t result;
	cfg_parser_t *pctx = NULL;
	cfg_obj_t *config = NULL;
	const cfg_obj_t *key = NULL;
	const cfg_obj_t *algobj = NULL;
	const cfg_obj_t *secretobj = NULL;
	const char *algstr = NULL;
	const char *secretstr = NULL;
	controlkey_t *keyid = NULL;
	char secret[1024];
	unsigned int algtype;
	isc_buffer_t b;

	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_CONTROL, ISC_LOG_INFO,
		      "configuring command channel from '%s'", named_g_keyfile);
	if (!isc_file_exists(named_g_keyfile)) {
		return (ISC_R_FILENOTFOUND);
	}

	CHECK(cfg_parser_create(mctx, named_g_lctx, &pctx));
	CHECK(cfg_parse_file(pctx, named_g_keyfile, &cfg_type_rndckey,
			     &config));
	CHECK(cfg_map_get(config, "key", &key));

	keyid = isc_mem_get(mctx, sizeof(*keyid));
	keyid->keyname = isc_mem_strdup(mctx,
					cfg_obj_asstring(cfg_map_getname(key)));
	keyid->secret.base = NULL;
	keyid->secret.length = 0;
	keyid->algorithm = DST_ALG_UNKNOWN;
	ISC_LINK_INIT(keyid, link);
	if (keyid->keyname == NULL) {
		CHECK(ISC_R_NOMEMORY);
	}

	CHECK(bind9_check_key(key, named_g_lctx));

	(void)cfg_map_get(key, "algorithm", &algobj);
	(void)cfg_map_get(key, "secret", &secretobj);
	INSIST(algobj != NULL && secretobj != NULL);

	algstr = cfg_obj_asstring(algobj);
	secretstr = cfg_obj_asstring(secretobj);

	result = named_config_getkeyalgorithm2(algstr, NULL, &algtype, NULL);
	if (result != ISC_R_SUCCESS) {
		cfg_obj_log(key, named_g_lctx, ISC_LOG_WARNING,
			    "unsupported algorithm '%s' in "
			    "key '%s' for use with command "
			    "channel",
			    algstr, keyid->keyname);
		goto cleanup;
	}

	keyid->algorithm = algtype;
	isc_buffer_init(&b, secret, sizeof(secret));
	result = isc_base64_decodestring(secretstr, &b);

	if (result != ISC_R_SUCCESS) {
		cfg_obj_log(key, named_g_lctx, ISC_LOG_WARNING,
			    "secret for key '%s' on command channel: %s",
			    keyid->keyname, isc_result_totext(result));
		goto cleanup;
	}

	keyid->secret.length = isc_buffer_usedlength(&b);
	keyid->secret.base = isc_mem_get(mctx, keyid->secret.length);
	memmove(keyid->secret.base, isc_buffer_base(&b), keyid->secret.length);
	ISC_LIST_APPEND(*keyids, keyid, link);
	keyid = NULL;
	result = ISC_R_SUCCESS;

cleanup:
	if (keyid != NULL) {
		free_controlkey(keyid, mctx);
	}
	if (config != NULL) {
		cfg_obj_destroy(pctx, &config);
	}
	if (pctx != NULL) {
		cfg_parser_destroy(&pctx);
	}
	return (result);
}

/*
 * Ensures that both '*global_keylistp' and '*control_keylistp' are
 * valid or both are NULL.
 */
static void
get_key_info(const cfg_obj_t *config, const cfg_obj_t *control,
	     const cfg_obj_t **global_keylistp,
	     const cfg_obj_t **control_keylistp) {
	isc_result_t result;
	const cfg_obj_t *control_keylist = NULL;
	const cfg_obj_t *global_keylist = NULL;

	REQUIRE(global_keylistp != NULL && *global_keylistp == NULL);
	REQUIRE(control_keylistp != NULL && *control_keylistp == NULL);

	control_keylist = cfg_tuple_get(control, "keys");

	if (!cfg_obj_isvoid(control_keylist) &&
	    cfg_list_first(control_keylist) != NULL)
	{
		result = cfg_map_get(config, "key", &global_keylist);

		if (result == ISC_R_SUCCESS) {
			*global_keylistp = global_keylist;
			*control_keylistp = control_keylist;
		}
	}
}

static void
update_listener(named_controls_t *cp, controllistener_t **listenerp,
		const cfg_obj_t *control, const cfg_obj_t *config,
		isc_sockaddr_t *addr, cfg_aclconfctx_t *aclconfctx,
		const char *socktext, isc_socktype_t type) {
	controllistener_t *listener = NULL;
	const cfg_obj_t *allow = NULL;
	const cfg_obj_t *global_keylist = NULL;
	const cfg_obj_t *control_keylist = NULL;
	dns_acl_t *new_acl = NULL;
	controlkeylist_t keys;
	isc_result_t result = ISC_R_SUCCESS;

	for (listener = ISC_LIST_HEAD(cp->listeners); listener != NULL;
	     listener = ISC_LIST_NEXT(listener, link))
	{
		if (isc_sockaddr_equal(addr, &listener->address)) {
			break;
		}
	}

	if (listener == NULL) {
		*listenerp = NULL;
		return;
	}

	/*
	 * There is already a listener for this sockaddr.
	 * Update the access list and key information.
	 *
	 * First try to deal with the key situation.  There are a few
	 * possibilities:
	 *  (a)	It had an explicit keylist and still has an explicit keylist.
	 *  (b)	It had an automagic key and now has an explicit keylist.
	 *  (c)	It had an explicit keylist and now needs an automagic key.
	 *  (d) It has an automagic key and still needs the automagic key.
	 *
	 * (c) and (d) are the annoying ones.  The caller needs to know
	 * that it should use the automagic configuration for key information
	 * in place of the named.conf configuration.
	 *
	 * XXXDCL There is one other hazard that has not been dealt with,
	 * the problem that if a key change is being caused by a control
	 * channel reload, then the response will be with the new key
	 * and not able to be decrypted by the client.
	 */
	if (control != NULL) {
		get_key_info(config, control, &global_keylist,
			     &control_keylist);
	}

	if (control_keylist != NULL) {
		INSIST(global_keylist != NULL);

		ISC_LIST_INIT(keys);
		controlkeylist_fromcfg(control_keylist, listener->mctx, &keys);
		free_controlkeylist(&listener->keys, listener->mctx);
		listener->keys = keys;
		register_keys(control, global_keylist, &listener->keys,
			      listener->mctx, socktext);
	} else {
		free_controlkeylist(&listener->keys, listener->mctx);
		result = get_rndckey(listener->mctx, &listener->keys);
	}

	if (result != ISC_R_SUCCESS && global_keylist != NULL) {
		/*
		 * This message might be a little misleading since the
		 * "new keys" might in fact be identical to the old ones,
		 * but tracking whether they are identical just for the
		 * sake of avoiding this message would be too much trouble.
		 */
		if (control != NULL) {
			cfg_obj_log(control, named_g_lctx, ISC_LOG_WARNING,
				    "couldn't install new keys for "
				    "command channel %s: %s",
				    socktext, isc_result_totext(result));
		} else {
			isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
				      NAMED_LOGMODULE_CONTROL, ISC_LOG_WARNING,
				      "couldn't install new keys for "
				      "command channel %s: %s",
				      socktext, isc_result_totext(result));
		}
	}

	/*
	 * Now, keep the old access list unless a new one can be made.
	 */
	if (control != NULL && type == isc_socktype_tcp) {
		allow = cfg_tuple_get(control, "allow");
		result = cfg_acl_fromconfig(allow, config, named_g_lctx,
					    aclconfctx, listener->mctx, 0,
					    &new_acl);
	} else {
		result = dns_acl_any(listener->mctx, &new_acl);
	}

	if (control != NULL) {
		const cfg_obj_t *readonly = NULL;

		readonly = cfg_tuple_get(control, "read-only");
		if (!cfg_obj_isvoid(readonly)) {
			listener->readonly = cfg_obj_asboolean(readonly);
		}
	}

	if (result == ISC_R_SUCCESS) {
		dns_acl_detach(&listener->acl);
		dns_acl_attach(new_acl, &listener->acl);
		dns_acl_detach(&new_acl);
		/* XXXDCL say the old acl is still used? */
	} else if (control != NULL) {
		cfg_obj_log(control, named_g_lctx, ISC_LOG_WARNING,
			    "couldn't install new acl for "
			    "command channel %s: %s",
			    socktext, isc_result_totext(result));
	} else {
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_CONTROL, ISC_LOG_WARNING,
			      "couldn't install new acl for "
			      "command channel %s: %s",
			      socktext, isc_result_totext(result));
	}

#if 0
	/* XXX: no unix socket support yet */
	if (result == ISC_R_SUCCESS && type == isc_socktype_unix) {
		uint32_t perm, owner, group;
		perm = cfg_obj_asuint32(cfg_tuple_get(control, "perm"));
		owner = cfg_obj_asuint32(cfg_tuple_get(control, "owner"));
		group = cfg_obj_asuint32(cfg_tuple_get(control, "group"));
		result = ISC_R_SUCCESS;
		if (listener->perm != perm || listener->owner != owner ||
		    listener->group != group)
		{
			result = isc_socket_permunix(&listener->address, perm,
						     owner, group);
		}
		if (result == ISC_R_SUCCESS) {
			listener->perm = perm;
			listener->owner = owner;
			listener->group = group;
		} else if (control != NULL) {
			cfg_obj_log(control, named_g_lctx, ISC_LOG_WARNING,
				    "couldn't update ownership/permission for "
				    "command channel %s",
				    socktext);
		}
	}
#endif

	*listenerp = listener;
}

static void
add_listener(named_controls_t *cp, controllistener_t **listenerp,
	     const cfg_obj_t *control, const cfg_obj_t *config,
	     isc_sockaddr_t *addr, cfg_aclconfctx_t *aclconfctx,
	     const char *socktext, isc_socktype_t type) {
	isc_mem_t *mctx = cp->server->mctx;
	controllistener_t *listener = NULL;
	const cfg_obj_t *allow = NULL;
	const cfg_obj_t *global_keylist = NULL;
	const cfg_obj_t *control_keylist = NULL;
	dns_acl_t *new_acl = NULL;
	isc_result_t result = ISC_R_SUCCESS;
	int pf;

	listener = isc_mem_get(mctx, sizeof(*listener));
	*listener = (controllistener_t){ .controls = cp,
					 .address = *addr,
					 .type = type };
	isc_mem_attach(mctx, &listener->mctx);
	isc_mutex_init(&listener->connections_lock);
	ISC_LINK_INIT(listener, link);
	ISC_LIST_INIT(listener->keys);
	ISC_LIST_INIT(listener->connections);
	isc_refcount_init(&listener->refs, 1);

	/*
	 * Make the ACL.
	 */
	if (control != NULL && type == isc_socktype_tcp) {
		const cfg_obj_t *readonly = NULL;

		allow = cfg_tuple_get(control, "allow");
		CHECK(cfg_acl_fromconfig(allow, config, named_g_lctx,
					 aclconfctx, mctx, 0, &new_acl));

		readonly = cfg_tuple_get(control, "read-only");
		if (!cfg_obj_isvoid(readonly)) {
			listener->readonly = cfg_obj_asboolean(readonly);
		}
	} else {
		CHECK(dns_acl_any(mctx, &new_acl));
	}

	dns_acl_attach(new_acl, &listener->acl);
	dns_acl_detach(&new_acl);

	if (config != NULL) {
		get_key_info(config, control, &global_keylist,
			     &control_keylist);
	}

	if (control_keylist != NULL) {
		controlkeylist_fromcfg(control_keylist, listener->mctx,
				       &listener->keys);
		register_keys(control, global_keylist, &listener->keys,
			      listener->mctx, socktext);
	} else {
		result = get_rndckey(mctx, &listener->keys);
		if (result != ISC_R_SUCCESS && control != NULL) {
			cfg_obj_log(control, named_g_lctx, ISC_LOG_WARNING,
				    "couldn't install keys for "
				    "command channel %s: %s",
				    socktext, isc_result_totext(result));
		}
	}

	pf = isc_sockaddr_pf(&listener->address);
	if ((pf == AF_INET && isc_net_probeipv4() != ISC_R_SUCCESS) ||
	    (pf == AF_UNIX && isc_net_probeunix() != ISC_R_SUCCESS) ||
	    (pf == AF_INET6 && isc_net_probeipv6() != ISC_R_SUCCESS))
	{
		CHECK(ISC_R_FAMILYNOSUPPORT);
	}

#if 0
	/* XXX: no unix socket support yet */
	if (type == isc_socktype_unix) {
		isc_socket_cleanunix(&listener->address, false);
	}
#endif

	CHECK(isc_nm_listentcp(
		named_g_netmgr, &listener->address, control_newconn, listener,
		sizeof(controlconnection_t), 5, NULL, &listener->sock));
#if 0
	/* XXX: no unix socket support yet */
	if (type == isc_socktype_unix) {
		listener->perm =
			cfg_obj_asuint32(cfg_tuple_get(control, "perm"));
		listener->owner =
			cfg_obj_asuint32(cfg_tuple_get(control, "owner"));
		listener->group =
			cfg_obj_asuint32(cfg_tuple_get(control, "group"));
		result = isc_socket_permunix(&listener->address, listener->perm,
					     listener->owner, listener->group);
	}
#endif

	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_CONTROL, ISC_LOG_NOTICE,
		      "command channel listening on %s", socktext);
	*listenerp = listener;
	return;

cleanup:
	isc_refcount_decrement(&listener->refs);
	listener->exiting = true;
	free_listener(listener);

	if (control != NULL) {
		cfg_obj_log(control, named_g_lctx, ISC_LOG_WARNING,
			    "couldn't add command channel %s: %s", socktext,
			    isc_result_totext(result));
	} else {
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_CONTROL, ISC_LOG_NOTICE,
			      "couldn't add command channel %s: %s", socktext,
			      isc_result_totext(result));
	}

	*listenerp = NULL;

	/* XXXDCL return error results? fail hard? */
}

isc_result_t
named_controls_configure(named_controls_t *cp, const cfg_obj_t *config,
			 cfg_aclconfctx_t *aclconfctx) {
	controllistener_t *listener = NULL;
	controllistenerlist_t new_listeners;
	const cfg_obj_t *controlslist = NULL;
	const cfg_listelt_t *element, *element2;
	char socktext[ISC_SOCKADDR_FORMATSIZE];

	ISC_LIST_INIT(new_listeners);

	/*
	 * Get the list of named.conf 'controls' statements.
	 */
	(void)cfg_map_get(config, "controls", &controlslist);

	/*
	 * Run through the new control channel list, noting sockets that
	 * are already being listened on and moving them to the new list.
	 *
	 * Identifying duplicate addr/port combinations is left to either
	 * the underlying config code, or to the bind attempt getting an
	 * address-in-use error.
	 */
	if (controlslist != NULL) {
		for (element = cfg_list_first(controlslist); element != NULL;
		     element = cfg_list_next(element))
		{
			const cfg_obj_t *controls = NULL;
			const cfg_obj_t *inetcontrols = NULL;

			controls = cfg_listelt_value(element);
			(void)cfg_map_get(controls, "inet", &inetcontrols);
			if (inetcontrols == NULL) {
				continue;
			}

			for (element2 = cfg_list_first(inetcontrols);
			     element2 != NULL;
			     element2 = cfg_list_next(element2))
			{
				const cfg_obj_t *control = NULL;
				const cfg_obj_t *obj = NULL;
				isc_sockaddr_t addr;

				/*
				 * The parser handles BIND 8 configuration file
				 * syntax, so it allows unix phrases as well
				 * inet phrases with no keys{} clause.
				 */
				control = cfg_listelt_value(element2);

				obj = cfg_tuple_get(control, "address");
				addr = *cfg_obj_assockaddr(obj);
				if (isc_sockaddr_getport(&addr) == 0) {
					isc_sockaddr_setport(
						&addr, NAMED_CONTROL_PORT);
				}

				isc_sockaddr_format(&addr, socktext,
						    sizeof(socktext));

				isc_log_write(named_g_lctx,
					      NAMED_LOGCATEGORY_GENERAL,
					      NAMED_LOGMODULE_CONTROL,
					      ISC_LOG_DEBUG(9),
					      "processing control channel %s",
					      socktext);

				update_listener(cp, &listener, control, config,
						&addr, aclconfctx, socktext,
						isc_socktype_tcp);

				if (listener != NULL) {
					/*
					 * Remove the listener from the old
					 * list, so it won't be shut down.
					 */
					ISC_LIST_UNLINK(cp->listeners, listener,
							link);
				} else {
					/*
					 * This is a new listener.
					 */
					add_listener(cp, &listener, control,
						     config, &addr, aclconfctx,
						     socktext,
						     isc_socktype_tcp);
				}

				if (listener != NULL) {
					ISC_LIST_APPEND(new_listeners, listener,
							link);
				}
			}
		}
		for (element = cfg_list_first(controlslist); element != NULL;
		     element = cfg_list_next(element))
		{
			const cfg_obj_t *controls = NULL;
			const cfg_obj_t *unixcontrols = NULL;

			controls = cfg_listelt_value(element);
			(void)cfg_map_get(controls, "unix", &unixcontrols);
			if (unixcontrols == NULL) {
				continue;
			}

			cfg_obj_log(controls, named_g_lctx, ISC_LOG_ERROR,
				    "UNIX domain sockets not yet supported");
			return (ISC_R_FAILURE);

#if 0
			/* XXX: no unix domain socket support in netmgr */
			for (element2 = cfg_list_first(unixcontrols);
			     element2 != NULL;
			     element2 = cfg_list_next(element2))
			{
				const cfg_obj_t *control = NULL;
				const cfg_obj_t *path = NULL;
				isc_sockaddr_t addr;
				isc_result_t result;

				/*
				 * The parser handles BIND 8 configuration file
				 * syntax, so it allows unix phrases as well
				 * inet phrases with no keys{} clause.
				 */
				control = cfg_listelt_value(element2);

				path = cfg_tuple_get(control, "path");
				result = isc_sockaddr_frompath(
					&addr, cfg_obj_asstring(path));
				if (result != ISC_R_SUCCESS) {
					isc_log_write(
						named_g_lctx,
						NAMED_LOGCATEGORY_GENERAL,
						NAMED_LOGMODULE_CONTROL,
						ISC_LOG_DEBUG(9),
						"control channel '%s': %s",
						cfg_obj_asstring(path),
						isc_result_totext(result));
					continue;
				}

				isc_log_write(named_g_lctx,
					      NAMED_LOGCATEGORY_GENERAL,
					      NAMED_LOGMODULE_CONTROL,
					      ISC_LOG_DEBUG(9),
					      "processing control channel '%s'",
					      cfg_obj_asstring(path));

				update_listener(cp, &listener, control, config,
						&addr, aclconfctx,
						cfg_obj_asstring(path),
						isc_socktype_unix);

				if (listener != NULL) {
					/*
					 * Remove the listener from the old
					 * list, so it won't be shut down.
					 */
					ISC_LIST_UNLINK(cp->listeners, listener,
							link);
				} else {
					/*
					 * This is a new listener.
					 */
					add_listener(cp, &listener, control,
						     config, &addr, aclconfctx,
						     cfg_obj_asstring(path),
						     isc_socktype_unix);
				}

				if (listener != NULL) {
					ISC_LIST_APPEND(new_listeners, listener,
							link);
				}
			}
#endif
		}
	} else {
		int i;

		for (i = 0; i < 2; i++) {
			isc_sockaddr_t addr;

			if (i == 0) {
				struct in_addr localhost;

				if (isc_net_probeipv4() != ISC_R_SUCCESS) {
					continue;
				}
				localhost.s_addr = htonl(INADDR_LOOPBACK);
				isc_sockaddr_fromin(&addr, &localhost, 0);
			} else {
				if (isc_net_probeipv6() != ISC_R_SUCCESS) {
					continue;
				}
				isc_sockaddr_fromin6(&addr, &in6addr_loopback,
						     0);
			}
			isc_sockaddr_setport(&addr, NAMED_CONTROL_PORT);

			isc_sockaddr_format(&addr, socktext, sizeof(socktext));

			update_listener(cp, &listener, NULL, NULL, &addr, NULL,
					socktext, isc_socktype_tcp);

			if (listener != NULL) {
				/*
				 * Remove the listener from the old
				 * list, so it won't be shut down.
				 */
				ISC_LIST_UNLINK(cp->listeners, listener, link);
			} else {
				/*
				 * This is a new listener.
				 */
				add_listener(cp, &listener, NULL, NULL, &addr,
					     NULL, socktext, isc_socktype_tcp);
			}

			if (listener != NULL) {
				ISC_LIST_APPEND(new_listeners, listener, link);
			}
		}
	}

	/*
	 * named_control_shutdown() will stop whatever is on the global
	 * listeners list, which currently only has whatever sockaddrs
	 * were in the previous configuration (if any) that do not
	 * remain in the current configuration.
	 */
	controls_shutdown(cp);

	/*
	 * Put all of the valid listeners on the listeners list.
	 * Anything already on listeners in the process of shutting
	 * down will be taken care of by listen_done().
	 */
	ISC_LIST_APPENDLIST(cp->listeners, new_listeners, link);
	return (ISC_R_SUCCESS);
}

isc_result_t
named_controls_create(named_server_t *server, named_controls_t **ctrlsp) {
	isc_mem_t *mctx = server->mctx;
	isc_result_t result;
	named_controls_t *controls = isc_mem_get(mctx, sizeof(*controls));

	*controls = (named_controls_t){
		.server = server,
	};

	ISC_LIST_INIT(controls->listeners);

	atomic_init(&controls->shuttingdown, false);
	isc_mutex_init(&controls->symtab_lock);
	LOCK(&controls->symtab_lock);
	result = isccc_cc_createsymtab(&controls->symtab);
	UNLOCK(&controls->symtab_lock);

	if (result != ISC_R_SUCCESS) {
		isc_mutex_destroy(&controls->symtab_lock);
		isc_mem_put(server->mctx, controls, sizeof(*controls));
		return (result);
	}
	*ctrlsp = controls;
	return (ISC_R_SUCCESS);
}

void
named_controls_destroy(named_controls_t **ctrlsp) {
	named_controls_t *controls = *ctrlsp;
	*ctrlsp = NULL;

	REQUIRE(ISC_LIST_EMPTY(controls->listeners));

	LOCK(&controls->symtab_lock);
	isccc_symtab_destroy(&controls->symtab);
	UNLOCK(&controls->symtab_lock);
	isc_mutex_destroy(&controls->symtab_lock);
	isc_mem_put(controls->server->mctx, controls, sizeof(*controls));
}
