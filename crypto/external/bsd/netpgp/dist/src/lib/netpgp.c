/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Alistair Crooks (agc@NetBSD.org)
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
#include "config.h"

#ifdef HAVE_SYS_CDEFS_H
#include <sys/cdefs.h>
#endif

#if defined(__NetBSD__)
__COPYRIGHT("@(#) Copyright (c) 2009 The NetBSD Foundation, Inc. All rights reserved.");
__RCSID("$NetBSD: netpgp.c,v 1.30 2009/11/19 21:56:00 agc Exp $");
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/mman.h>

#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include <regex.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <errno.h>

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#include <netpgp.h>

#include "packet.h"
#include "packet-parse.h"
#include "keyring.h"
#include "errors.h"
#include "packet-show.h"
#include "create.h"
#include "netpgpsdk.h"
#include "memory.h"
#include "validate.h"
#include "readerwriter.h"
#include "netpgpdefs.h"
#include "crypto.h"

/* read any gpg config file */
static int
conffile(netpgp_t *netpgp, char *homedir, char *userid, size_t length)
{
	regmatch_t	 matchv[10];
	regex_t		 keyre;
	char		 buf[BUFSIZ];
	FILE		*fp;

	__OPS_USED(netpgp);
	(void) snprintf(buf, sizeof(buf), "%s/gpg.conf", homedir);
	if ((fp = fopen(buf, "r")) == NULL) {
		return 0;
	}
	(void) memset(&keyre, 0x0, sizeof(keyre));
	(void) regcomp(&keyre, "^[ \t]*default-key[ \t]+([0-9a-zA-F]+)",
		REG_EXTENDED);
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		if (regexec(&keyre, buf, 10, matchv, 0) == 0) {
			(void) memcpy(userid, &buf[(int)matchv[1].rm_so],
				MIN((unsigned)(matchv[1].rm_eo -
						matchv[1].rm_so), length));
			(void) fprintf(stderr,
				"netpgp: default key set to \"%.*s\"\n",
				(int)(matchv[1].rm_eo - matchv[1].rm_so),
				&buf[(int)matchv[1].rm_so]);
		}
	}
	(void) fclose(fp);
	return 1;
}

/* small function to pretty print an 8-character raw userid */
static char    *
userid_to_id(const unsigned char *userid, char *id)
{
	static const char *hexes = "0123456789abcdef";
	int		   i;

	for (i = 0; i < 8 ; i++) {
		id[i * 2] = hexes[(unsigned)(userid[i] & 0xf0) >> 4];
		id[(i * 2) + 1] = hexes[userid[i] & 0xf];
	}
	id[8 * 2] = 0x0;
	return id;
}

/* print out the successful signature information */
static void
resultp(__ops_io_t *io,
	const char *f,
	__ops_validation_t *res,
	__ops_keyring_t *ring)
{
	const __ops_key_t	*pubkey;
	unsigned		 i;
	char			 id[MAX_ID_LENGTH + 1];

	for (i = 0; i < res->validc; i++) {
		(void) fprintf(io->res,
			"Good signature for %s made %susing %s key %s\n",
			f,
			ctime(&res->valid_sigs[i].birthtime),
			__ops_show_pka(res->valid_sigs[i].key_alg),
			userid_to_id(res->valid_sigs[i].signer_id, id));
		pubkey = __ops_getkeybyid(io, ring,
			(const unsigned char *) res->valid_sigs[i].signer_id);
		__ops_print_pubkeydata(io, pubkey);
	}
}

/* check there's enough space in the arrays */
static int
size_arrays(netpgp_t *netpgp, unsigned needed)
{
	char	**temp;

	if (netpgp->size == 0) {
		/* only get here first time around */
		netpgp->size = needed;
		if ((netpgp->name = calloc(sizeof(char *), needed)) == NULL) {
			(void) fprintf(stderr, "size_arrays: bad alloc\n");
			return 0;
		}
		if ((netpgp->value = calloc(sizeof(char *), needed)) == NULL) {
			free(netpgp->name);
			(void) fprintf(stderr, "size_arrays: bad alloc\n");
			return 0;
		}
	} else if (netpgp->c == netpgp->size) {
		/* only uses 'needed' when filled array */
		netpgp->size += needed;
		temp = realloc(netpgp->name, sizeof(char *) * needed);
		if (temp == NULL) {
			(void) fprintf(stderr, "size_arrays: bad alloc\n");
			return 0;
		}
		netpgp->name = temp;
		temp = realloc(netpgp->value, sizeof(char *) * needed);
		if (temp == NULL) {
			(void) fprintf(stderr, "size_arrays: bad alloc\n");
			return 0;
		}
		netpgp->value = temp;
	}
	return 1;
}

/* find the name in the array */
static int
findvar(netpgp_t *netpgp, const char *name)
{
	unsigned	i;

	for (i = 0 ; i < netpgp->c && strcmp(netpgp->name[i], name) != 0; i++) {
	}
	return (i == netpgp->c) ? -1 : (int)i;
}

/* read a keyring and return it */
static void *
readkeyring(netpgp_t *netpgp, const char *name)
{
	__ops_keyring_t	*keyring;
	const unsigned	 noarmor = 0;
	char		 f[MAXPATHLEN];
	char		*filename;
	char		*homedir;

	homedir = netpgp_getvar(netpgp, "homedir");
	if ((filename = netpgp_getvar(netpgp, name)) == NULL) {
		(void) snprintf(f, sizeof(f), "%s/%s.gpg", homedir, name);
		filename = f;
	}
	if ((keyring = calloc(1, sizeof(*keyring))) == NULL) {
		(void) fprintf(stderr, "readkeyring: bad alloc\n");
		return NULL;
	}
	if (!__ops_keyring_fileread(keyring, noarmor, filename)) {
		free(keyring);
		(void) fprintf(stderr, "Can't read %s %s\n", name, filename);
		return NULL;
	}
	netpgp_setvar(netpgp, name, filename);
	return keyring;
}

/***************************************************************************/
/* exported functions start here */
/***************************************************************************/

/* initialise a netpgp_t structure */
int
netpgp_init(netpgp_t *netpgp)
{
	__ops_io_t	*io;
	char		 id[MAX_ID_LENGTH];
	char		*homedir;
	char		*userid;
	char		*stream;
	char		*passfd;
	char		*results;
	int		 coredumps;

#ifdef HAVE_SYS_RESOURCE_H
	struct rlimit	limit;

	coredumps = netpgp_getvar(netpgp, "coredumps") != NULL;
	if (!coredumps) {
		(void) memset(&limit, 0x0, sizeof(limit));
		if (setrlimit(RLIMIT_CORE, &limit) != 0) {
			(void) fprintf(stderr,
			"netpgp_init: warning - can't turn off core dumps\n");
			coredumps = 1;
		}
	}
#else
	coredumps = 1;
#endif
	if ((io = calloc(1, sizeof(*io))) == NULL) {
		(void) fprintf(stderr, "netpgp_init: bad alloc\n");
		return 0;
	}
	io->outs = stdout;
	if ((stream = netpgp_getvar(netpgp, "stdout")) != NULL &&
	    strcmp(stream, "stderr") == 0) {
		io->outs = stderr;
	}
	io->errs = stderr;
	if ((stream = netpgp_getvar(netpgp, "stderr")) != NULL &&
	    strcmp(stream, "stdout") == 0) {
		io->errs = stdout;
	}
	io->errs = stderr;
	netpgp->io = io;
	if (coredumps) {
		(void) fprintf(io->errs,
			"netpgp: warning: core dumps enabled\n");
	}
	if ((homedir = netpgp_getvar(netpgp, "homedir")) == NULL) {
		(void) fprintf(io->errs, "netpgp: bad homedir\n");
		return 0;
	}
	if ((userid = netpgp_getvar(netpgp, "userid")) == NULL) {
		(void) memset(id, 0x0, sizeof(id));
		(void) conffile(netpgp, homedir, id, sizeof(id));
		if (id[0] != 0x0) {
			netpgp_setvar(netpgp, "userid", userid = id);
		}
	}
	if (userid == NULL) {
		if (netpgp_getvar(netpgp, "need userid") != NULL) {
			(void) fprintf(io->errs, "Cannot find user id\n");
			return 0;
		}
	} else {
		(void) netpgp_setvar(netpgp, "userid", userid);
	}
	if ((netpgp->pubring = readkeyring(netpgp, "pubring")) == NULL) {
		(void) fprintf(io->errs, "Can't read pub keyring\n");
		return 0;
	}
	if ((netpgp->secring = readkeyring(netpgp, "secring")) == NULL) {
		(void) fprintf(io->errs, "Can't read sec keyring\n");
		return 0;
	}
	if ((passfd = netpgp_getvar(netpgp, "pass-fd")) != NULL &&
	    (netpgp->passfp = fdopen(atoi(passfd), "r")) == NULL) {
		(void) fprintf(io->errs, "Can't open fd %s for reading\n",
			passfd);
		return 0;
	}
	if ((results = netpgp_getvar(netpgp, "results")) == NULL) {
		io->res = io->errs;
	} else if ((io->res = fopen(results, "w")) == NULL) {
		(void) fprintf(io->errs, "Can't open results %s for writing\n",
			results);
		return 0;
	}
	return 1;
}

/* finish off with the netpgp_t struct */
int
netpgp_end(netpgp_t *netpgp)
{
	unsigned	i;

	for (i = 0 ; i < netpgp->c ; i++) {
		if (netpgp->name[i] != NULL) {
			free(netpgp->name[i]);
		}
		if (netpgp->value[i] != NULL) {
			free(netpgp->value[i]);
		}
	}
	if (netpgp->name != NULL) {
		free(netpgp->name);
	}
	if (netpgp->value != NULL) {
		free(netpgp->value);
	}
	if (netpgp->pubring != NULL) {
		__ops_keyring_free(netpgp->pubring);
	}
	if (netpgp->secring != NULL) {
		__ops_keyring_free(netpgp->secring);
	}
	free(netpgp->io);
	return 1;
}

/* list the keys in a keyring */
int
netpgp_list_keys(netpgp_t *netpgp)
{
	return __ops_keyring_list(netpgp->io, netpgp->pubring);
}

/* find a key in a keyring */
int
netpgp_find_key(netpgp_t *netpgp, char *id)
{
	__ops_io_t	*io;

	io = netpgp->io;
	if (id == NULL) {
		(void) fprintf(io->errs, "NULL id to search for\n");
		return 0;
	}
	return __ops_getkeybyname(netpgp->io, netpgp->pubring, id) != NULL;
}

/* get a key in a keyring */
char *
netpgp_get_key(netpgp_t *netpgp, const char *id)
{
	const __ops_key_t	*key;
	__ops_io_t		*io;
	char			*newkey;

	io = netpgp->io;
	if (id == NULL) {
		(void) fprintf(io->errs, "NULL id to search for\n");
		return NULL;
	}
	if ((key = __ops_getkeybyname(netpgp->io, netpgp->pubring, id)) == NULL) {
		(void) fprintf(io->errs, "Can't find key '%s'\n", id);
		return NULL;
	}
	return (__ops_sprint_pubkeydata(key, &newkey) > 0) ? newkey : NULL;
}

/* export a given key */
int
netpgp_export_key(netpgp_t *netpgp, char *userid)
{
	const __ops_key_t	*keypair;
	__ops_io_t		*io;

	io = netpgp->io;
	if (userid == NULL) {
		userid = netpgp_getvar(netpgp, "userid");
	}
	keypair = __ops_getkeybyname(io, netpgp->pubring, userid);
	if (keypair == NULL) {
		(void) fprintf(io->errs,
			"Cannot find own key \"%s\" in keyring\n", userid);
		return 0;
	}
	return __ops_export_key(keypair, NULL);
}

/* import a key into our keyring */
int
netpgp_import_key(netpgp_t *netpgp, char *f)
{
	const unsigned	noarmor = 0;
	const unsigned	armor = 1;
	__ops_io_t	*io;
	int		done;

	io = netpgp->io;
	if ((done = __ops_keyring_fileread(netpgp->pubring, noarmor, f)) == 0) {
		done = __ops_keyring_fileread(netpgp->pubring, armor, f);
	}
	if (!done) {
		(void) fprintf(io->errs, "Cannot import key from file %s\n",
				f);
		return 0;
	}
	return __ops_keyring_list(io, netpgp->pubring);
}

/* generate a new key */
int
netpgp_generate_key(netpgp_t *netpgp, char *id, int numbits)
{
	__ops_key_t		*keypair;
	__ops_userid_t		 uid;
	__ops_output_t		*create;
	const unsigned		 noarmor = 0;
	__ops_io_t		*io;
	char			*ringfile;
	int             	 fd;

	(void) memset(&uid, 0x0, sizeof(uid));
	io = netpgp->io;
	/* generate a new key for 'id' */
	uid.userid = (unsigned char *) id;
	keypair = __ops_rsa_new_selfsign_key(numbits, 65537UL, &uid);
	if (keypair == NULL) {
		(void) fprintf(io->errs, "Cannot generate key\n");
		return 0;
	}
	/* write public key, and try to re-read it */
	ringfile = netpgp_getvar(netpgp, "pubring"); 
	fd = __ops_setup_file_append(&create, ringfile);
	if (!__ops_write_xfer_pubkey(create, keypair, noarmor)) {
		(void) fprintf(io->errs, "Cannot write pubkey\n");
		return 0;
	}
	__ops_teardown_file_write(create, fd);
	__ops_keyring_free(netpgp->pubring);
	if (!__ops_keyring_fileread(netpgp->pubring, noarmor, ringfile)) {
		(void) fprintf(io->errs, "Cannot read pubring %s\n", ringfile);
		return 0;
	}
	/* write secret key, and try to re-read it */
	ringfile = netpgp_getvar(netpgp, "sec ring file"); 
	fd = __ops_setup_file_append(&create, ringfile);
	if (!__ops_write_xfer_seckey(create, keypair, NULL, 0, noarmor)) {
		(void) fprintf(io->errs, "Cannot write seckey\n");
		return 0;
	}
	__ops_teardown_file_write(create, fd);
	__ops_keyring_free(netpgp->secring);
	if (!__ops_keyring_fileread(netpgp->secring, noarmor, ringfile)) {
		(void) fprintf(io->errs, "Can't read secring %s\n", ringfile);
		return 0;
	}
	__ops_keydata_free(keypair);
	return 1;
}

/* encrypt a file */
int
netpgp_encrypt_file(netpgp_t *netpgp,
			const char *userid,
			const char *f,
			char *out,
			int armored)
{
	const __ops_key_t	*keypair;
	const unsigned		 overwrite = 1;
	const char		*suffix;
	__ops_io_t		*io;
	char			 outname[MAXPATHLEN];

	io = netpgp->io;
	if (f == NULL) {
		(void) fprintf(io->errs,
			"netpgp_encrypt_file: no filename specified\n");
		return 0;
	}
	if (userid == NULL) {
		userid = netpgp_getvar(netpgp, "userid");
	}
	suffix = (armored) ? ".asc" : ".gpg";
	keypair = __ops_getkeybyname(io, netpgp->pubring, userid);
	if (keypair == NULL) {
		(void) fprintf(io->errs, "Userid '%s' not found in keyring\n",
					userid);
		return 0;
	}
	if (out == NULL) {
		(void) snprintf(outname, sizeof(outname), "%s%s", f, suffix);
		out = outname;
	}
	return (int)__ops_encrypt_file(io, f, out, keypair, (unsigned)armored,
					overwrite);
}

/* decrypt a file */
int
netpgp_decrypt_file(netpgp_t *netpgp, const char *f, char *out, int armored)
{
	const unsigned	overwrite = 1;
	__ops_io_t		*io;

	io = netpgp->io;
	if (f == NULL) {
		(void) fprintf(io->errs,
			"netpgp_decrypt_file: no filename specified\n");
		return 0;
	}
	return __ops_decrypt_file(netpgp->io, f, out, netpgp->secring,
				(unsigned)armored, overwrite, netpgp->passfp,
				get_passphrase_cb);
}

/* sign a file */
int
netpgp_sign_file(netpgp_t *netpgp,
		const char *userid,
		const char *f,
		char *out,
		int armored,
		int cleartext,
		int detached)
{
	const __ops_key_t	*keypair;
	__ops_seckey_t		*seckey;
	const unsigned		 overwrite = 1;
	__ops_io_t		*io;
	char			*hashalg;
	int			 ret;

	io = netpgp->io;
	if (f == NULL) {
		(void) fprintf(io->errs,
			"netpgp_sign_file: no filename specified\n");
		return 0;
	}
	if (userid == NULL) {
		userid = netpgp_getvar(netpgp, "userid");
	}
	/* get key with which to sign */
	keypair = __ops_getkeybyname(io, netpgp->secring, userid);
	if (keypair == NULL) {
		(void) fprintf(io->errs, "Userid '%s' not found in keyring\n",
				userid);
		return 0;
	}
	ret = 1;
	do {
		/* print out the user id */
		__ops_print_pubkeydata(io, keypair);
		/* now decrypt key */
		seckey = __ops_decrypt_seckey(keypair);
		if (seckey == NULL) {
			(void) fprintf(io->errs, "Bad passphrase\n");
		}
	} while (seckey == NULL);
	/* sign file */
	hashalg = netpgp_getvar(netpgp, "hash");
	if (detached) {
		ret = __ops_sign_detached(io, f, out, seckey, hashalg);
	} else {
		ret = __ops_sign_file(io, f, out, seckey, hashalg,
				(unsigned)armored, (unsigned)cleartext, overwrite);
	}
	__ops_forget(seckey, sizeof(*seckey));
	return ret;
}

/* verify a file */
int
netpgp_verify_file(netpgp_t *netpgp, const char *in, const char *out, int armored)
{
	__ops_validation_t	 result;
	__ops_io_t		*io;

	(void) memset(&result, 0x0, sizeof(result));
	io = netpgp->io;
	if (in == NULL) {
		(void) fprintf(io->errs,
			"netpgp_verify_file: no filename specified\n");
		return 0;
	}
	if (__ops_validate_file(io, &result, in, out, armored,
						netpgp->pubring)) {
		resultp(io, in, &result, netpgp->pubring);
		return 1;
	}
	if (result.validc + result.invalidc + result.unknownc == 0) {
		(void) fprintf(io->errs,
		"\"%s\": No signatures found - is this a signed file?\n",
			in);
	} else {
		(void) fprintf(io->errs,
"\"%s\": verification failure: %u invalid signatures, %u unknown signatures\n",
			in, result.invalidc, result.unknownc);
	}
	return 0;
}

/* sign some memory */
int
netpgp_sign_memory(netpgp_t *netpgp,
		const char *userid,
		char *mem,
		size_t size,
		char *out,
		size_t outsize,
		const unsigned armored,
		const unsigned cleartext)
{
	const __ops_key_t	*keypair;
	__ops_seckey_t		*seckey;
	__ops_memory_t		*signedmem;
	__ops_io_t		*io;
	char			*hashalg;
	int			 ret;

	io = netpgp->io;
	if (mem == NULL) {
		(void) fprintf(io->errs,
			"netpgp_sign_memory: no memory to sign\n");
		return 0;
	}
	if (userid == NULL) {
		userid = netpgp_getvar(netpgp, "userid");
	}
	/* get key with which to sign */
	keypair = __ops_getkeybyname(io, netpgp->secring, userid);
	if (keypair == NULL) {
		(void) fprintf(io->errs, "Userid '%s' not found in keyring\n",
				userid);
		return 0;
	}
	ret = 1;
	do {
		/* print out the user id */
		__ops_print_pubkeydata(io, keypair);
		/* now decrypt key */
		seckey = __ops_decrypt_seckey(keypair);
		if (seckey == NULL) {
			(void) fprintf(io->errs, "Bad passphrase\n");
		}
	} while (seckey == NULL);
	/* sign file */
	hashalg = netpgp_getvar(netpgp, "hash");
	signedmem = __ops_sign_buf(io, mem, size, seckey, hashalg,
						armored, cleartext);
	if (signedmem) {
		size_t	m;

		m = MIN(__ops_mem_len(signedmem), outsize);
		(void) memcpy(out, __ops_mem_data(signedmem), m);
		__ops_memory_free(signedmem);
	}
	__ops_forget(seckey, sizeof(*seckey));
	return ret;
}

/* verify memory */
int
netpgp_verify_memory(netpgp_t *netpgp, const void *in, const size_t size, const int armored)
{
	__ops_validation_t	 result;
	__ops_memory_t		*signedmem;
	__ops_io_t		*io;
	int			 ret;

	(void) memset(&result, 0x0, sizeof(result));
	io = netpgp->io;
	if (in == NULL) {
		(void) fprintf(io->errs,
			"netpgp_verify_memory: no memory to verify\n");
		return 0;
	}
	signedmem = __ops_memory_new();
	__ops_memory_add(signedmem, in, size);
	ret = __ops_validate_mem(io, &result, signedmem, armored,
						netpgp->pubring);
	__ops_memory_free(signedmem);
	if (ret) {
		resultp(io, in, &result, netpgp->pubring);
		return 1;
	}
	if (result.validc + result.invalidc + result.unknownc == 0) {
		(void) fprintf(io->errs,
		"No signatures found - is this memory signed?\n");
	} else {
		(void) fprintf(io->errs,
"memory verification failure: %u invalid signatures, %u unknown signatures\n",
			result.invalidc, result.unknownc);
	}
	return 0;
}

/* wrappers for the ops_debug_level functions we added to openpgpsdk */

/* set the debugging level per filename */
int
netpgp_set_debug(const char *f)
{
	return __ops_set_debug_level(f);
}

/* get the debugging level per filename */
int
netpgp_get_debug(const char *f)
{
	return __ops_get_debug_level(f);
}

/* return the version for the library */
const char *
netpgp_get_info(const char *type)
{
	return __ops_get_info(type);
}

/* list all the packets in a file */
int
netpgp_list_packets(netpgp_t *netpgp, char *f, int armour, char *pubringname)
{
	__ops_keyring_t	*keyring;
	const unsigned	 noarmor = 0;
	__ops_io_t	*io;
	char		 ringname[MAXPATHLEN];
	char		*homedir;
	int		 ret;

	io = netpgp->io;
	if (f == NULL) {
		(void) fprintf(io->errs, "No file containing packets\n");
		return 0;
	}
	homedir = netpgp_getvar(netpgp, "homedir");
	if (pubringname == NULL) {
		(void) snprintf(ringname, sizeof(ringname),
				"%s/pubring.gpg", homedir);
		pubringname = ringname;
	}
	if ((keyring = calloc(1, sizeof(*keyring))) == NULL) {
		(void) fprintf(io->errs, "netpgp_list_packets: bad alloc\n");
		return 0;
	}
	if (!__ops_keyring_fileread(keyring, noarmor, pubringname)) {
		free(keyring);
		(void) fprintf(io->errs, "Cannot read pub keyring %s\n",
			pubringname);
		return 0;
	}
	netpgp->pubring = keyring;
	netpgp_setvar(netpgp, "pubring", pubringname);
	ret = __ops_list_packets(io, f, (unsigned)armour, keyring,
					netpgp->passfp,
					get_passphrase_cb);
	free(keyring);
	return ret;
}

/* set a variable */
int
netpgp_setvar(netpgp_t *netpgp, const char *name, const char *value)
{
	int	i;

	if ((i = findvar(netpgp, name)) < 0) {
		/* add the element to the array */
		if (size_arrays(netpgp, netpgp->size + 15)) {
			netpgp->name[i = netpgp->c++] = strdup(name);
		}
	} else {
		/* replace the element in the array */
		if (netpgp->value[i]) {
			free(netpgp->value[i]);
			netpgp->value[i] = NULL;
		}
	}
	/* sanity checks for range of values */
	if (strcmp(name, "hash") == 0 || strcmp(name, "algorithm") == 0) {
		if (__ops_str_to_hash_alg(value) == OPS_HASH_UNKNOWN) {
			return 0;
		}
	}
	netpgp->value[i] = strdup(value);
	return 1;
}

/* get a variable's value (NULL if not set) */
char *
netpgp_getvar(netpgp_t *netpgp, const char *name)
{
	int	i;

	return ((i = findvar(netpgp, name)) < 0) ? NULL : netpgp->value[i];
}
