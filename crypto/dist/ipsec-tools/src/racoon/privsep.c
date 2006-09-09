/*	$NetBSD: privsep.c,v 1.6 2006/09/09 16:22:10 manu Exp $	*/

/* Id: privsep.c,v 1.15 2005/08/08 11:23:44 vanhu Exp */

/*
 * Copyright (C) 2004 Emmanuel Dreyfus
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "config.h"

#include <unistd.h>
#include <string.h>
#ifdef __NetBSD__
#include <stdlib.h>	/* for setproctitle */
#endif
#include <errno.h>
#include <signal.h>
#include <pwd.h>

#include <sys/socket.h>
#include <sys/param.h>

#include "gcmalloc.h"
#include "vmbuf.h"
#include "misc.h"
#include "plog.h"
#include "var.h"
#include "libpfkey.h"

#include "crypto_openssl.h"
#include "isakmp_var.h"
#include "isakmp.h"
#ifdef ENABLE_HYBRID
#include "resolv.h"
#include "isakmp_xauth.h"
#include "isakmp_cfg.h"
#endif
#include "localconf.h"
#include "remoteconf.h"
#include "admin.h"
#include "sockmisc.h"
#include "privsep.h"

static int privsep_sock[2] = { -1, -1 };

static int privsep_recv(int, struct privsep_com_msg **, size_t *);
static int privsep_send(int, struct privsep_com_msg *, size_t);
static int safety_check(struct privsep_com_msg *, int i);
static int port_check(int);
static int unsafe_env(char *const *);
static int unknown_name(int);
static int unsafe_path(char *, int);

static int
privsep_send(sock, buf, len)
	int sock;
	struct privsep_com_msg *buf;
	size_t len;
{
	if (buf == NULL)
		return 0;

	if (sendto(sock, (char *)buf, len, 0, NULL, 0) == -1) {
		plog(LLV_ERROR, LOCATION, NULL,
		    "privsep_send failed: %s\n", 
		    strerror(errno));
		return -1;
	}

	racoon_free((char *)buf);

	return 0;
}


static int
privsep_recv(sock, bufp, lenp)
	int sock;
	struct privsep_com_msg **bufp;
	size_t *lenp;
{
	struct admin_com com;
	struct admin_com *combuf;
	size_t len;

	*bufp = NULL;
	*lenp = 0;

	/* Get the header */
	while ((len = recvfrom(sock, (char *)&com, 
	    sizeof(com), MSG_PEEK, NULL, NULL)) == -1) {
		if (errno == EINTR)
			continue;

		plog(LLV_ERROR, LOCATION, NULL,
		    "privsep_recv failed: %s\n",
		    strerror(errno));
		return -1;
	}
	
	/* Check for short packets */
	if (len < sizeof(com)) {
		plog(LLV_ERROR, LOCATION, NULL,
		    "corrupted privsep message (short header)\n");
		return -1;
	}

	/* Allocate buffer for the whole message */
	if ((combuf = (struct admin_com *)racoon_malloc(com.ac_len)) == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
		    "failed to allocate memory: %s\n", strerror(errno));
		return -1;
	}

	/* Get the whole buffer */
	while ((len = recvfrom(sock, (char *)combuf, 
	    com.ac_len, 0, NULL, NULL)) == -1) {
		if (errno == EINTR)
			continue;
		plog(LLV_ERROR, LOCATION, NULL,
		    "failed to recv privsep command: %s\n", 
		    strerror(errno));
		return -1;
	}

	/* We expect len to match */
	if (len != com.ac_len) {
		plog(LLV_ERROR, LOCATION, NULL,
		    "corrupted privsep message (short packet)\n");
		return -1;
	}

	*bufp = (struct privsep_com_msg *)combuf;
	*lenp = len;

	return 0;
}

int
privsep_init(void)
{
	int i;
	pid_t child_pid;

	/* If running as root, we don't use the privsep code path */
	if (lcconf->uid == 0)
		return 0;

	/* 
	 * When running privsep, certificate and script paths 
	 * are mandatory, as they enable us to check path safety
	 * in the privilegied instance
	 */
	if ((lcconf->pathinfo[LC_PATHTYPE_CERT] == NULL) ||
	    (lcconf->pathinfo[LC_PATHTYPE_SCRIPT] == NULL)) {
		plog(LLV_ERROR, LOCATION, NULL, "privilege separation "
		   "require path cert and path script in the config file\n");
		return -1;
	}

	if (socketpair(PF_LOCAL, SOCK_DGRAM, 0, privsep_sock) != 0) {
		plog(LLV_ERROR, LOCATION, NULL, 
		    "Cannot allocate privsep_sock: %s\n", strerror(errno));
		return -1;
	}

	switch (child_pid = fork()) {
	case -1:
		plog(LLV_ERROR, LOCATION, NULL, "Cannot fork privsep: %s\n", 
		    strerror(errno));
		return -1;
		break;

	case 0: /* Child: drop privileges */
		if (lcconf->chroot != NULL) {
			if (chdir(lcconf->chroot) != 0) {
				plog(LLV_ERROR, LOCATION, NULL, 
				    "Cannot chdir(%s): %s\n", lcconf->chroot, 
				    strerror(errno));
				return -1;
			}
			if (chroot(lcconf->chroot) != 0) {
				plog(LLV_ERROR, LOCATION, NULL, 
				    "Cannot chroot(%s): %s\n", lcconf->chroot, 
				    strerror(errno));
				return -1;
			}
		}

		if (setgid(lcconf->gid) != 0) {
			plog(LLV_ERROR, LOCATION, NULL, 
			    "Cannot setgid(%d): %s\n", lcconf->gid,
			    strerror(errno));
			return -1;
		}

		if (setegid(lcconf->gid) != 0) {
			plog(LLV_ERROR, LOCATION, NULL, 
			    "Cannot setegid(%d): %s\n", lcconf->gid,
			    strerror(errno));
			return -1;
		}

		if (setuid(lcconf->uid) != 0) {
			plog(LLV_ERROR, LOCATION, NULL, 
			    "Cannot setuid(%d): %s\n", lcconf->uid,
			    strerror(errno));
			return -1;
		}

		if (seteuid(lcconf->uid) != 0) {
			plog(LLV_ERROR, LOCATION, NULL, 
			    "Cannot seteuid(%d): %s\n", lcconf->uid,
			    strerror(errno));
			return -1;
		}

		return 0;
		break;

	default: /* Parent: privilegied process */
		break;
	}

	/* 
	 * Close everything except the socketpair, 
	 * and stdout if running in the forground.
	 */
	for (i = sysconf(_SC_OPEN_MAX); i > 0; i--) {
		if (i == privsep_sock[0])
			continue;
		if (i == privsep_sock[1])
			continue;
		if ((f_foreground) && (i == 1))
			continue;
		(void)close(i);
	}

	/* Above trickery closed the log file, reopen it */
	ploginit();

	plog(LLV_INFO, LOCATION, NULL, 
	    "racoon privilegied process running with PID %d\n", getpid());

#ifdef __NetBSD__
	setproctitle("[priv]");
#endif
	
	/* 
	 * Don't catch any signal 
	 * This duplicate session:signals[], which is static...
	 */
	signal(SIGHUP, SIG_DFL);
	signal(SIGINT, SIG_DFL);
	signal(SIGTERM, SIG_DFL);
	signal(SIGUSR1, SIG_DFL);
	signal(SIGUSR2, SIG_DFL);
	signal(SIGCHLD, SIG_DFL);

	while (1) {
		size_t len;
		struct privsep_com_msg *combuf;
		struct privsep_com_msg *reply;
		char *data;
		size_t *buflen;
		size_t totallen;
		char *bufs[PRIVSEP_NBUF_MAX];
		int i;

		if (privsep_recv(privsep_sock[0], &combuf, &len) != 0)
			goto out;

		/* Safety checks and gather the data */
		if (len < sizeof(*combuf)) {
			plog(LLV_ERROR, LOCATION, NULL,
			    "corrupted privsep message (short buflen)\n");
			goto out;
		}

		data = (char *)(combuf + 1);
		totallen = sizeof(*combuf);
		for (i = 0; i < PRIVSEP_NBUF_MAX; i++) {
			bufs[i] = (char *)data;
			data += combuf->bufs.buflen[i];
			totallen += combuf->bufs.buflen[i];
		}

		if (totallen > len) {
			plog(LLV_ERROR, LOCATION, NULL,
			    "corrupted privsep message (bufs too big)\n");
			goto out;
		}
	
		/* Prepare the reply buffer */
		if ((reply = racoon_malloc(sizeof(*reply))) == NULL) {
			plog(LLV_ERROR, LOCATION, NULL,
			    "Cannot allocate reply buffer: %s\n", 
			    strerror(errno));
			goto out;
		}
		bzero(reply, sizeof(*reply));
		reply->hdr.ac_cmd = combuf->hdr.ac_cmd;
		reply->hdr.ac_len = sizeof(*reply);

		switch(combuf->hdr.ac_cmd) {
		/* 
		 * XXX Improvement: instead of returning the key, 
		 * stuff eay_get_pkcs1privkey and eay_get_x509sign
		 * together and sign the hash in the privilegied 
		 * instance? 
		 * pro: the key remains inaccessible to unpriv
		 * con: a compromised unpriv racoon can still sign anything
		 */
		case PRIVSEP_EAY_GET_PKCS1PRIVKEY: {
			vchar_t *privkey;

			/* Make sure the string is NULL terminated */
			if (safety_check(combuf, 0) != 0)
				break;
			bufs[0][combuf->bufs.buflen[0] - 1] = '\0';

			if (unsafe_path(bufs[0], LC_PATHTYPE_CERT) != 0) {
				plog(LLV_ERROR, LOCATION, NULL, 
				    "privsep_eay_get_pkcs1privkey: "
				    "unsafe cert \"%s\"\n", bufs[0]);
			}

			plog(LLV_DEBUG, LOCATION, NULL, 
			    "eay_get_pkcs1privkey(\"%s\")\n", bufs[0]);

			if ((privkey = eay_get_pkcs1privkey(bufs[0])) == NULL){
				reply->hdr.ac_errno = errno;
				break;
			}

			reply->bufs.buflen[0] = privkey->l;
			reply->hdr.ac_len = sizeof(*reply) + privkey->l;
			reply = racoon_realloc(reply, reply->hdr.ac_len);
			if (reply == NULL) {
				plog(LLV_ERROR, LOCATION, NULL,
				    "Cannot allocate reply buffer: %s\n", 
				    strerror(errno));
				goto out;
			}

			memcpy(reply + 1, privkey->v, privkey->l);
			vfree(privkey);
			break;
		}
		
		case PRIVSEP_SCRIPT_EXEC: {
			char *script;
			int name;
			char **envp = NULL;
			int envc = 0;
			int count = 0;
			int i;

			/*
			 * First count the bufs, and make sure strings
			 * are NULL terminated. 
			 *
			 * We expect: script, name, envp[], void
			 */ 
			if (safety_check(combuf, 0) != 0)
				break;
			bufs[0][combuf->bufs.buflen[0] - 1] = '\0';
			count++;	/* script */

			count++;	/* name */

			for (; count < PRIVSEP_NBUF_MAX; count++) {
				if (combuf->bufs.buflen[count] == 0)
					break;
				bufs[count]
				    [combuf->bufs.buflen[count] - 1] = '\0';
				envc++;
			}

			/* count a void buf and perform safety check */
			count++;
			if (count >= PRIVSEP_NBUF_MAX) {
				plog(LLV_ERROR, LOCATION, NULL, 
				    "privsep_script_exec: too many args\n");
				goto out;
			}


			/* 
			 * Allocate the arrays for envp 
			 */
			envp = racoon_malloc((envc + 1) * sizeof(char *));
			if (envp == NULL) {
				plog(LLV_ERROR, LOCATION, NULL, 
				    "cannot allocate memory: %s\n",
				    strerror(errno));
				goto out;
			}
			bzero(envp, (envc + 1) * sizeof(char *));

	
			/*
			 * Populate script, name and envp 
			 */
			count = 0;
			script = bufs[count++];

			if (combuf->bufs.buflen[count] != sizeof(name)) {
				plog(LLV_ERROR, LOCATION, NULL, 
				    "privsep_script_exec: corrupted message\n");
				goto out;
			}
			memcpy((char *)&name, bufs[count++], sizeof(name));

			for (i = 0; combuf->bufs.buflen[count]; count++)
				envp[i++] = bufs[count];

			count++;		/* void */

			plog(LLV_DEBUG, LOCATION, NULL, 
			    "script_exec(\"%s\", %d, %p)\n", 
			    script, name, envp);

			/* 
			 * Check env for dangerous variables
			 * Check script path and name
			 * Perform fork and execve
			 */
			if ((unsafe_env(envp) == 0) &&
			    (unknown_name(name) == 0) &&
			    (unsafe_path(script, LC_PATHTYPE_SCRIPT) == 0))
				(void)script_exec(script, name, envp);
			else
				plog(LLV_ERROR, LOCATION, NULL, 
				    "privsep_script_exec: "
				    "unsafe script \"%s\"\n", script);

			racoon_free(envp);
			break;
		}

		case PRIVSEP_GETPSK: {
			vchar_t *psk;
			int keylen;

			/* Make sure the string is NULL terminated */
			if (safety_check(combuf, 0) != 0)
				break;
			bufs[0][combuf->bufs.buflen[0] - 1] = '\0';

			if (combuf->bufs.buflen[1] != sizeof(keylen)) {
				plog(LLV_ERROR, LOCATION, NULL, 
				    "privsep_getpsk: corrupted message\n");
				goto out;
			}
			memcpy(&keylen, bufs[1], sizeof(keylen));

			plog(LLV_DEBUG, LOCATION, NULL, 
			    "getpsk(\"%s\", %d)\n", bufs[0], keylen);

			if ((psk = getpsk(bufs[0], keylen)) == NULL) {
				reply->hdr.ac_errno = errno;
				break;
			}

			reply->bufs.buflen[0] = psk->l;
			reply->hdr.ac_len = sizeof(*reply) + psk->l;
			reply = racoon_realloc(reply, reply->hdr.ac_len); 
			if (reply == NULL) {
				plog(LLV_ERROR, LOCATION, NULL,
				    "Cannot allocate reply buffer: %s\n", 
				    strerror(errno));
				goto out;
			}

			memcpy(reply + 1, psk->v, psk->l);
			vfree(psk);
			break;
		}

#ifdef ENABLE_HYBRID
		case PRIVSEP_ACCOUNTING_SYSTEM: {
			int pool_size;
			int port;
			int inout;
			struct sockaddr *raddr;

			if (safety_check(combuf, 0) != 0)
				break;
			if (safety_check(combuf, 1) != 0)
				break;
			if (safety_check(combuf, 2) != 0)
				break;
			if (safety_check(combuf, 3) != 0)
				break;

			memcpy(&port, bufs[0], sizeof(port));
			raddr = (struct sockaddr *)bufs[1];

			bufs[2][combuf->bufs.buflen[2] - 1] = '\0';
			memcpy(&inout, bufs[3], sizeof(port));

			if (port_check(port) != 0)
				break;

			plog(LLV_DEBUG, LOCATION, NULL, 
			    "accounting_system(%d, %s, %s)\n", 
			    port, saddr2str(raddr), bufs[2]); 

			errno = 0;
			if (isakmp_cfg_accounting_system(port, 
			    raddr, bufs[2], inout) != 0) {
				if (errno == 0)
					reply->hdr.ac_errno = EINVAL;
				else
					reply->hdr.ac_errno = errno;
			}
			break;
		}
		case PRIVSEP_XAUTH_LOGIN_SYSTEM: {
			if (safety_check(combuf, 0) != 0)
				break;
			bufs[0][combuf->bufs.buflen[0] - 1] = '\0';

			if (safety_check(combuf, 1) != 0)
				break;
			bufs[1][combuf->bufs.buflen[1] - 1] = '\0';

			plog(LLV_DEBUG, LOCATION, NULL, 
			    "xauth_login_system(\"%s\", <password>)\n", 
			    bufs[0]);

			errno = 0;
			if (xauth_login_system(bufs[0], bufs[1]) != 0) {
				if (errno == 0)
					reply->hdr.ac_errno = EINVAL;
				else
					reply->hdr.ac_errno = errno;
			}
			break;
		}
#ifdef HAVE_LIBPAM
		case PRIVSEP_ACCOUNTING_PAM: {
			int port;
			int inout;
			int pool_size;

			if (safety_check(combuf, 0) != 0)
				break;
			if (safety_check(combuf, 1) != 0)
				break;
			if (safety_check(combuf, 2) != 0)
				break;

			memcpy(&port, bufs[0], sizeof(port));
			memcpy(&inout, bufs[1], sizeof(inout));
			memcpy(&pool_size, bufs[2], sizeof(pool_size));

			if (pool_size != isakmp_cfg_config.pool_size)
				if (isakmp_cfg_resize_pool(pool_size) != 0)
					break;

			if (port_check(port) != 0)
				break;

			plog(LLV_DEBUG, LOCATION, NULL, 
			    "isakmp_cfg_accounting_pam(%d, %d)\n", 
			    port, inout); 

			errno = 0;
			if (isakmp_cfg_accounting_pam(port, inout) != 0) {
				if (errno == 0)
					reply->hdr.ac_errno = EINVAL;
				else
					reply->hdr.ac_errno = errno;
			}
			break;
		}

		case PRIVSEP_XAUTH_LOGIN_PAM: {
			int port;
			int pool_size;
			struct sockaddr *raddr;

			if (safety_check(combuf, 0) != 0)
				break;
			if (safety_check(combuf, 1) != 0)
				break;
			if (safety_check(combuf, 2) != 0)
				break;
			if (safety_check(combuf, 3) != 0)
				break;
			if (safety_check(combuf, 4) != 0)
				break;

			memcpy(&port, bufs[0], sizeof(port));
			memcpy(&pool_size, bufs[1], sizeof(pool_size));
			raddr = (struct sockaddr *)bufs[2];
			
			bufs[3][combuf->bufs.buflen[3] - 1] = '\0';
			bufs[4][combuf->bufs.buflen[4] - 1] = '\0';

			if (pool_size != isakmp_cfg_config.pool_size)
				if (isakmp_cfg_resize_pool(pool_size) != 0)
					break;

			if (port_check(port) != 0)
				break;

			plog(LLV_DEBUG, LOCATION, NULL, 
			    "xauth_login_pam(%d, %s, \"%s\", <password>)\n", 
			    port, saddr2str(raddr), bufs[3]); 

			errno = 0;
			if (xauth_login_pam(port, 
			    raddr, bufs[3], bufs[4]) != 0) {
				if (errno == 0)
					reply->hdr.ac_errno = EINVAL;
				else
					reply->hdr.ac_errno = errno;
			}
			break;
		}

		case PRIVSEP_CLEANUP_PAM: {
			int port;
			int pool_size;

			if (safety_check(combuf, 0) != 0)
				break;
			if (safety_check(combuf, 1) != 0)
				break;

			memcpy(&port, bufs[0], sizeof(port));
			memcpy(&pool_size, bufs[1], sizeof(pool_size));

			if (pool_size != isakmp_cfg_config.pool_size)
				if (isakmp_cfg_resize_pool(pool_size) != 0)
					break;

			if (port_check(port) != 0)
				break;

			plog(LLV_DEBUG, LOCATION, NULL, 
			    "cleanup_pam(%d)\n", port);

			cleanup_pam(port);
			reply->hdr.ac_errno = 0;

			break;
		}
#endif /* HAVE_LIBPAM */
#endif /* ENABLE_HYBRID */

		default:
			plog(LLV_ERROR, LOCATION, NULL,
			    "unexpected privsep command %d\n", 
			    combuf->hdr.ac_cmd);
			goto out;
			break;
		}

		/* This frees reply */
		if (privsep_send(privsep_sock[0], 
		    reply, reply->hdr.ac_len) != 0)
			goto out;

		racoon_free(combuf);
	}

out:
	plog(LLV_INFO, LOCATION, NULL, "privsep exit\n");
	_exit(0);
}


vchar_t *
privsep_eay_get_pkcs1privkey(path) 
	char *path;
{
	vchar_t *privkey;
	struct privsep_com_msg *msg;
	size_t len;

	if (geteuid() == 0)
		return eay_get_pkcs1privkey(path);

	len = sizeof(*msg) + strlen(path) + 1;
	if ((msg = racoon_malloc(len)) == NULL) {
		plog(LLV_ERROR, LOCATION, NULL, 
		    "Cannot allocate memory: %s\n", strerror(errno));
		return NULL;
	}
	bzero(msg, len);
	msg->hdr.ac_cmd = PRIVSEP_EAY_GET_PKCS1PRIVKEY;
	msg->hdr.ac_len = len;
	msg->bufs.buflen[0] = len - sizeof(*msg);
	memcpy(msg + 1, path, msg->bufs.buflen[0]);

	if (privsep_send(privsep_sock[1], msg, len) != 0)
		return NULL;

	if (privsep_recv(privsep_sock[1], &msg, &len) != 0)
		return NULL;

	if (msg->hdr.ac_errno != 0) {
		errno = msg->hdr.ac_errno;
		goto out;
	}

	if ((privkey = vmalloc(len - sizeof(*msg))) == NULL)
		goto out;

	memcpy(privkey->v, msg + 1, privkey->l);
	racoon_free(msg);
	return privkey;

out:
	racoon_free(msg);
	return NULL;
}

/*
 * No prigilege separation trick here, we just open PFKEY before
 * dropping root privs and we remember it later.
 */
static int  pfkey_socket = -1;
int
privsep_pfkey_open(void)
{
	int ps;

	if (pfkey_socket != -1)
		return pfkey_socket;

	ps = pfkey_open();
	if (ps != -1)
		pfkey_socket = ps;

	return ps;
}

/*
 * Consequence of the above trickery: don't 
 * really close PFKEY as we never re-open it.
 */
void
privsep_pfkey_close(ps)
	int ps;
{
	return;
}

int
privsep_script_exec(script, name, envp)
	char *script;
	int name;
	char *const envp[];
{
	int count = 0;
	char *const *c;
	char *data;
	size_t len;
	struct privsep_com_msg *msg;

	if (geteuid() == 0)
		return script_exec(script, name, envp);

	if ((msg = racoon_malloc(sizeof(*msg))) == NULL) {
		plog(LLV_ERROR, LOCATION, NULL, 
		    "Cannot allocate memory: %s\n", strerror(errno));
		return -1;
	}

	bzero(msg, sizeof(*msg));
	msg->hdr.ac_cmd = PRIVSEP_SCRIPT_EXEC;
	msg->hdr.ac_len = sizeof(*msg);

	/*
	 * We send: 
	 * script, name, envp[0], ... envp[N], void
	 */

	/*
	 * Safety check on the counts: PRIVSEP_NBUF_MAX max
	 */
	count = 0;
	count++;					/* script */
	count++;					/* name */
	for (c = envp; *c; c++)				/* envp */
		count++;
	count++;					/* void */

	if (count > PRIVSEP_NBUF_MAX) {
		plog(LLV_ERROR, LOCATION, NULL, "Unexpected error: "
		    "privsep_script_exec count > PRIVSEP_NBUF_MAX\n");
		racoon_free(msg);
		return -1;
	}


	/*
	 * Compute the length
	 */
	count = 0;
	msg->bufs.buflen[count] = strlen(script) + 1;	/* script */
	msg->hdr.ac_len += msg->bufs.buflen[count++];

	msg->bufs.buflen[count] = sizeof(name);		/* name */
	msg->hdr.ac_len += msg->bufs.buflen[count++];

	for (c = envp; *c; c++) {			/* envp */
		msg->bufs.buflen[count] = strlen(*c) + 1;
		msg->hdr.ac_len += msg->bufs.buflen[count++];
	}

	msg->bufs.buflen[count] = 0; 			/* void */
	msg->hdr.ac_len += msg->bufs.buflen[count++];

	if ((msg = racoon_realloc(msg, msg->hdr.ac_len)) == NULL) {
		plog(LLV_ERROR, LOCATION, NULL, 
		    "Cannot allocate memory: %s\n", strerror(errno));
		return -1;
	}
	
	/*
	 * Now copy the data
	 */
	data = (char *)(msg + 1);
	count = 0;

	memcpy(data, (char *)script, msg->bufs.buflen[count]);	/* script */
	data += msg->bufs.buflen[count++];

	memcpy(data, (char *)&name, msg->bufs.buflen[count]);	/* name */
	data += msg->bufs.buflen[count++];

	for (c = envp; *c; c++) {				/* envp */
		memcpy(data, *c, msg->bufs.buflen[count]); 
		data += msg->bufs.buflen[count++];
	}

	count++;						/* void */

	/*
	 * And send it!
	 */
	if (privsep_send(privsep_sock[1], msg, msg->hdr.ac_len) != 0)
		return -1;

	if (privsep_recv(privsep_sock[1], &msg, &len) != 0)
		return -1;

	if (msg->hdr.ac_errno != 0) {
		errno = msg->hdr.ac_errno;
		racoon_free(msg);
		return -1;
	}

	racoon_free(msg);
	return 0;
}

vchar_t *
privsep_getpsk(str, keylen)
	const char *str;
	int keylen;
{
	vchar_t *psk;
	struct privsep_com_msg *msg;
	size_t len;
	int *keylenp;
	char *data;

	if (geteuid() == 0)
		return getpsk(str, keylen);

	len = sizeof(*msg) + strlen(str) + 1 + sizeof(keylen);
	if ((msg = racoon_malloc(len)) == NULL) {
		plog(LLV_ERROR, LOCATION, NULL, 
		    "Cannot allocate memory: %s\n", strerror(errno));
		return NULL;
	}
	bzero(msg, len);
	msg->hdr.ac_cmd = PRIVSEP_GETPSK;
	msg->hdr.ac_len = len;

	data = (char *)(msg + 1);
	msg->bufs.buflen[0] = strlen(str) + 1;
	memcpy(data, str, msg->bufs.buflen[0]);

	data += msg->bufs.buflen[0];
	msg->bufs.buflen[1] = sizeof(keylen);
	memcpy(data, &keylen, sizeof(keylen));

	if (privsep_send(privsep_sock[1], msg, len) != 0)
		return NULL;

	if (privsep_recv(privsep_sock[1], &msg, &len) != 0)
		return NULL;

	if (msg->hdr.ac_errno != 0) {
		errno = msg->hdr.ac_errno;
		goto out;
	}

	if ((psk = vmalloc(len - sizeof(*msg))) == NULL)
		goto out;

	memcpy(psk->v, msg + 1, psk->l);
	racoon_free(msg);
	return psk;

out:
	racoon_free(msg);
	return NULL;
}

#ifdef ENABLE_HYBRID
int
privsep_xauth_login_system(usr, pwd)
	char *usr;
	char *pwd;
{
	struct privsep_com_msg *msg;
	size_t len;
	char *data;

	if (geteuid() == 0)
		return xauth_login_system(usr, pwd);

	len = sizeof(*msg) + strlen(usr) + 1 + strlen(pwd) + 1;
	if ((msg = racoon_malloc(len)) == NULL) {
		plog(LLV_ERROR, LOCATION, NULL, 
		    "Cannot allocate memory: %s\n", strerror(errno));
		return -1;
	}
	bzero(msg, len);
	msg->hdr.ac_cmd = PRIVSEP_XAUTH_LOGIN_SYSTEM;
	msg->hdr.ac_len = len;

	data = (char *)(msg + 1);
	msg->bufs.buflen[0] = strlen(usr) + 1;
	memcpy(data, usr, msg->bufs.buflen[0]);
	data += msg->bufs.buflen[0];

	msg->bufs.buflen[1] = strlen(pwd) + 1;
	memcpy(data, pwd, msg->bufs.buflen[1]);
	
	if (privsep_send(privsep_sock[1], msg, len) != 0)
		return -1;

	if (privsep_recv(privsep_sock[1], &msg, &len) != 0)
		return -1;

	if (msg->hdr.ac_errno != 0) {
		racoon_free(msg);
		return -1;
	}

	racoon_free(msg);
	return 0;
}

int 
privsep_accounting_system(port, raddr, usr, inout)
	int port;
	struct sockaddr *raddr;
	char *usr;
	int inout;
{
	struct privsep_com_msg *msg;
	size_t len;
	char *data;
	int result;

	if (geteuid() == 0)
		return isakmp_cfg_accounting_system(port, raddr,
						    usr, inout);

	len = sizeof(*msg) 
	    + sizeof(port)
	    + sysdep_sa_len(raddr) 
	    + strlen(usr) + 1
	    + sizeof(inout);

	if ((msg = racoon_malloc(len)) == NULL) {
		plog(LLV_ERROR, LOCATION, NULL, 
		    "Cannot allocate memory: %s\n", strerror(errno));
		return -1;
	}
	bzero(msg, len);
	msg->hdr.ac_cmd = PRIVSEP_ACCOUNTING_SYSTEM;
	msg->hdr.ac_len = len;
	msg->bufs.buflen[0] = sizeof(port);
	msg->bufs.buflen[1] = sysdep_sa_len(raddr);
	msg->bufs.buflen[2] = strlen(usr) + 1;
	msg->bufs.buflen[3] = sizeof(inout);

	data = (char *)(msg + 1);
	memcpy(data, &port, msg->bufs.buflen[0]);

	data += msg->bufs.buflen[0];
	memcpy(data, raddr, msg->bufs.buflen[1]);

	data += msg->bufs.buflen[1];
	memcpy(data, usr, msg->bufs.buflen[2]);

	data += msg->bufs.buflen[2];
	memcpy(data, &inout, msg->bufs.buflen[3]);

	if (privsep_send(privsep_sock[1], msg, len) != 0)
		return -1;

	if (privsep_recv(privsep_sock[1], &msg, &len) != 0)
		return -1;

	if (msg->hdr.ac_errno != 0) {
		errno = msg->hdr.ac_errno;
		goto out;
	}

	racoon_free(msg);
	return 0;

out:
	racoon_free(msg);
	return -1;
}

static int
port_check(port)
	int port;
{
	if ((port < 0) || (port >= isakmp_cfg_config.pool_size)) {
		plog(LLV_ERROR, LOCATION, NULL, 
		    "privsep: port %d outside of allowed range [0,%zu]\n",
		    port, isakmp_cfg_config.pool_size - 1);
		return -1;
	}

	return 0;
}
#endif

static int 
safety_check(msg, index)
	struct privsep_com_msg *msg;
	int index;
{
	if (index >= PRIVSEP_NBUF_MAX) {
		plog(LLV_ERROR, LOCATION, NULL, 
		    "privsep: Corrupted message, too many buffers\n");
		return -1;
	}
		
	if (msg->bufs.buflen[index] == 0) {
		plog(LLV_ERROR, LOCATION, NULL, 
		    "privsep: Corrupted message, unexpected void buffer\n");
		return -1;
	}

	return 0;
}

/*
 * Filter unsafe environement variables
 */
static int
unsafe_env(envp)
	char *const *envp;
{
	char *const *e;
	char *const *be;
	char *const bad_env[] = { "PATH=", "LD_LIBRARY_PATH=", "IFS=", NULL };

	for (e = envp; *e; e++) {
		for (be = bad_env; *be; be++) {
			if (strncmp(*e, *be, strlen(*be)) == 0) {
				goto found;
			}
		}
	}

	return 0;
found:
	plog(LLV_ERROR, LOCATION, NULL, 
	    "privsep_script_exec: unsafe environement variable\n");
	return -1;
}

/*
 * Check path safety
 */
static int 
unsafe_path(script, pathtype)
	char *script;
	int pathtype;
{
	char *path;
	char rpath[MAXPATHLEN + 1];
	size_t len;

	if (script == NULL) 
		return -1;

	path = lcconf->pathinfo[pathtype];

	/* No path was given for scripts: skip the check */
	if (path == NULL)
		return 0;

	if (realpath(script, rpath) == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
		    "script path \"%s\" is invalid\n", script);
		return -1;
	}

	len = strlen(path);
	if (strncmp(path, rpath, len) != 0)
		return -1;

	return 0;
}

static int 
unknown_name(name)
	int name;
{
	if ((name < 0) || (name > SCRIPT_MAX)) {
		plog(LLV_ERROR, LOCATION, NULL, 
		    "privsep_script_exec: unsafe name index\n");
		return -1;
	}

	return 0;
}

#ifdef HAVE_LIBPAM
int 
privsep_accounting_pam(port, inout)
	int port;
	int inout;
{
	struct privsep_com_msg *msg;
	size_t len;
	int *port_data;
	int *inout_data;
	int *pool_size_data;
	int result;

	if (geteuid() == 0)
		return isakmp_cfg_accounting_pam(port, inout);

	len = sizeof(*msg) 
	    + sizeof(port) 
	    + sizeof(inout)
	    + sizeof(isakmp_cfg_config.pool_size);

	if ((msg = racoon_malloc(len)) == NULL) {
		plog(LLV_ERROR, LOCATION, NULL, 
		    "Cannot allocate memory: %s\n", strerror(errno));
		return -1;
	}
	bzero(msg, len);
	msg->hdr.ac_cmd = PRIVSEP_ACCOUNTING_PAM;
	msg->hdr.ac_len = len;
	msg->bufs.buflen[0] = sizeof(port);
	msg->bufs.buflen[1] = sizeof(inout);
	msg->bufs.buflen[2] = sizeof(isakmp_cfg_config.pool_size);

	port_data = (int *)(msg + 1);
	inout_data = (int *)(port_data + 1);
	pool_size_data = (int *)(inout_data + 1);

	*port_data = port;
	*inout_data = inout;
	*pool_size_data = isakmp_cfg_config.pool_size;

	if (privsep_send(privsep_sock[1], msg, len) != 0)
		return -1;

	if (privsep_recv(privsep_sock[1], &msg, &len) != 0)
		return -1;

	if (msg->hdr.ac_errno != 0) {
		errno = msg->hdr.ac_errno;
		goto out;
	}

	racoon_free(msg);
	return 0;

out:
	racoon_free(msg);
	return -1;
}

int 
privsep_xauth_login_pam(port, raddr, usr, pwd)
	int port;
	struct sockaddr *raddr;
	char *usr;
	char *pwd;
{
	struct privsep_com_msg *msg;
	size_t len;
	char *data;
	int result;

	if (geteuid() == 0)
		return xauth_login_pam(port, raddr, usr, pwd);

	len = sizeof(*msg) 
	    + sizeof(port)
	    + sizeof(isakmp_cfg_config.pool_size)
	    + sysdep_sa_len(raddr) 
	    + strlen(usr) + 1
	    + strlen(pwd) + 1;

	if ((msg = racoon_malloc(len)) == NULL) {
		plog(LLV_ERROR, LOCATION, NULL, 
		    "Cannot allocate memory: %s\n", strerror(errno));
		return -1;
	}
	bzero(msg, len);
	msg->hdr.ac_cmd = PRIVSEP_XAUTH_LOGIN_PAM;
	msg->hdr.ac_len = len;
	msg->bufs.buflen[0] = sizeof(port);
	msg->bufs.buflen[1] = sizeof(isakmp_cfg_config.pool_size);
	msg->bufs.buflen[2] = sysdep_sa_len(raddr);
	msg->bufs.buflen[3] = strlen(usr) + 1;
	msg->bufs.buflen[4] = strlen(pwd) + 1;

	data = (char *)(msg + 1);
	memcpy(data, &port, msg->bufs.buflen[0]);

	data += msg->bufs.buflen[0];
	memcpy(data, &isakmp_cfg_config.pool_size, msg->bufs.buflen[1]);

	data += msg->bufs.buflen[1];
	memcpy(data, raddr, msg->bufs.buflen[2]);

	data += msg->bufs.buflen[2];
	memcpy(data, usr, msg->bufs.buflen[3]);

	data += msg->bufs.buflen[3];
	memcpy(data, pwd, msg->bufs.buflen[4]);

	if (privsep_send(privsep_sock[1], msg, len) != 0)
		return -1;

	if (privsep_recv(privsep_sock[1], &msg, &len) != 0)
		return -1;

	if (msg->hdr.ac_errno != 0) {
		errno = msg->hdr.ac_errno;
		goto out;
	}

	racoon_free(msg);
	return 0;

out:
	racoon_free(msg);
	return -1;
}

void
privsep_cleanup_pam(port)
	int port;
{
	struct privsep_com_msg *msg;
	size_t len;
	char *data;
	int result;

	if (geteuid() == 0)
		return cleanup_pam(port);

	len = sizeof(*msg) 
	    + sizeof(port)
	    + sizeof(isakmp_cfg_config.pool_size);

	if ((msg = racoon_malloc(len)) == NULL) {
		plog(LLV_ERROR, LOCATION, NULL, 
		    "Cannot allocate memory: %s\n", strerror(errno));
		return;
	}
	bzero(msg, len);
	msg->hdr.ac_cmd = PRIVSEP_CLEANUP_PAM;
	msg->hdr.ac_len = len;
	msg->bufs.buflen[0] = sizeof(port);
	msg->bufs.buflen[1] = sizeof(isakmp_cfg_config.pool_size);

	data = (char *)(msg + 1);
	memcpy(data, &port, msg->bufs.buflen[0]);

	data += msg->bufs.buflen[0];
	memcpy(data, &isakmp_cfg_config.pool_size, msg->bufs.buflen[1]);

	if (privsep_send(privsep_sock[1], msg, len) != 0)
		return;

	if (privsep_recv(privsep_sock[1], &msg, &len) != 0)
		return;

	if (msg->hdr.ac_errno != 0)
		errno = msg->hdr.ac_errno;

	racoon_free(msg);
	return;
}
#endif
