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

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/mman.h>

#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

#ifdef HAVE_OPENSSL_CAST_H
#include <openssl/cast.h>
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

enum {
	MAX_ID_LENGTH		= 128,
	MAX_PASSPHRASE_LENGTH	= 256
};

/* read any gpg config file */
static int
conffile(char *homedir, char *userid, size_t length, int verbose)
{
	regmatch_t	 matchv[10];
	regex_t		 keyre;
	char		 buf[BUFSIZ];
	FILE		*fp;

	(void) snprintf(buf, sizeof(buf), "%s/.gnupg/gpg.conf", homedir);
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
			if (verbose) {
				printf("setting default key to \"%.*s\"\n",
				(int)(matchv[1].rm_eo - matchv[1].rm_so),
				&buf[(int)matchv[1].rm_so]);
			}
		}
	}
	(void) fclose(fp);
	return 1;
}

/* wrapper to get a pass phrase from the user */
static void
get_pass_phrase(char *phrase, size_t size)
{
	char           *p;

	while ((p = getpass("netpgp passphrase: ")) == NULL) {
	}
	(void) snprintf(phrase, size, "%s", p);
}

/* small function to pretty print an 8-character raw userid */
static char    *
userid_to_id(const unsigned char *userid, char *id)
{
	static const char *hexes = "0123456789ABCDEF";
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
psuccess(FILE *fp, char *f, __ops_validation_t *res, __ops_keyring_t *pubring)
{
	const __ops_keydata_t	*pubkey;
	unsigned		 i;
	char			 id[MAX_ID_LENGTH + 1];

	for (i = 0; i < res->validc; i++) {
		(void) fprintf(fp,
			"Good signature for %s made %susing %s key %s\n",
			f,
			ctime(&res->valid_sigs[i].birthtime),
			__ops_show_pka(res->valid_sigs[i].key_alg),
			userid_to_id(res->valid_sigs[i].signer_id, id));
		pubkey = __ops_keyring_find_key_by_id(pubring,
			(const unsigned char *) res->valid_sigs[i].signer_id);
		__ops_print_pubkeydata(pubkey);
	}
}

/* sign a file, and put the signature in a separate file */
static int
sign_detached(char *f, char *sigfile, __ops_seckey_t *seckey,
		const char *hashstr)
{
	__ops_create_sig_t	*sig;
	__ops_hash_alg_t	 alg;
	__ops_createinfo_t	*info;
	unsigned char	 	 keyid[OPS_KEY_ID_SIZE];
	unsigned char		*mmapped;
	struct stat		 st;
	time_t			 t;
	char			 fname[MAXPATHLEN];
	int			 fd;

	/* find out which hash algorithm to use */
	alg = __ops_str_to_hash_alg(hashstr);
	if (alg == OPS_HASH_UNKNOWN) {
		(void) fprintf(stderr,"Unknown hash algorithm: %s\n", hashstr);
		return 0;
	}

	/* create a new signature */
	sig = __ops_create_sig_new();
	__ops_start_sig(sig, seckey, alg, OPS_SIG_BINARY);

	/* read the contents of 'f' */
	fd = open(f, O_RDONLY);
	if (fd < 0) {
		(void) fprintf(stderr, "can't open file \"%s\" to sign\n",
			f);
		return 0;
	}
	/* attempt to mmap(2) the file - if that fails, fall back to
	 * standard read(2) */
	mmapped = MAP_FAILED;
	if (fstat(fd, &st) == 0) {
		mmapped = mmap(NULL, (size_t)st.st_size, PROT_READ,
					MAP_FILE | MAP_PRIVATE, fd, 0);
	}
	if (mmapped == MAP_FAILED) {
		for (;;) {
			unsigned char	buf[8192];
			int 		n;

			if ((n = read(fd, buf, sizeof(buf))) == 0) {
				break;
			}
			if (n < 0) {
				(void) fprintf(stderr, "short read \"%s\"\n",
						f);
				(void) close(fd);
				return 0;
			}
			__ops_sig_add_data(sig, buf, (unsigned)n);
		}
	} else {
		__ops_sig_add_data(sig, mmapped, (unsigned)st.st_size);
		(void) munmap(mmapped, (unsigned)st.st_size);
	}
	(void) close(fd);

	/* calculate the signature */
	t = time(NULL);
	__ops_sig_add_birthtime(sig, t);
	__ops_keyid(keyid, OPS_KEY_ID_SIZE, OPS_KEY_ID_SIZE,
		&seckey->pubkey);
	__ops_sig_add_issuer_key_id(sig, keyid);
	__ops_sig_hashed_subpackets_end(sig);

	/* write the signature to the detached file */
	if (sigfile == NULL) {
		(void) snprintf(fname, sizeof(fname), "%s.sig", f);
		sigfile = fname;
	}
	fd = open(sigfile, O_CREAT|O_TRUNC|O_WRONLY, 0666);
	if (fd < 0) {
		(void) fprintf(stderr, "can't write signature to \"%s\"\n",
				sigfile);
		return 0;
	}

	info = __ops_createinfo_new();
	__ops_writer_set_fd(info, fd);
	__ops_write_sig(sig, &seckey->pubkey, seckey, info);
	__ops_seckey_free(seckey);
	(void) close(fd);

	return 1;
}

/***************************************************************************/
/* exported functions start here */
/***************************************************************************/

/* initialise a netpgp_t structure */
int
netpgp_init(netpgp_t *netpgp, char *userid, char *pubring, char *secring)
{
	__ops_keyring_t	*keyring;
	char		*homedir;
	char		 ringname[MAXPATHLEN];
	char		 id[MAX_ID_LENGTH];

#ifdef HAVE_SYS_RESOURCE_H
	struct rlimit	limit;

	(void) memset(&limit, 0x0, sizeof(limit));
	if (setrlimit(RLIMIT_CORE, &limit) != 0) {
		(void) fprintf(stderr,
			"netpgp_init: warning - can't turn off core dumps\n");
	}
#else
	(void) fprintf(stderr,
		"netpgp_init: warning - no way of switching off core dumps\n");
#endif
	homedir = getenv("HOME");
	if (userid == NULL) {
		(void) memset(id, 0x0, sizeof(id));
		conffile(homedir, id, sizeof(id), 1);
		if (id[0] != 0x0) {
			userid = id;
		}
	}
	if (userid == NULL) {
		(void) fprintf(stderr, "Cannot find user id\n");
		return 0;
	}
	(void) netpgp_setvar(netpgp, "userid", id);
	if (pubring == NULL) {
		(void) snprintf(ringname, sizeof(ringname),
			"%s/.gnupg/pubring.gpg", homedir);
		pubring = ringname;
	}
	keyring = calloc(1, sizeof(*keyring));
	if (!__ops_keyring_fileread(keyring, 0, pubring)) {
		(void) fprintf(stderr, "Cannot read pub keyring %s\n", pubring);
		return 0;
	}
	netpgp->pubring = keyring;
	netpgp->pubringfile = strdup(pubring);
	if (secring == NULL) {
		(void) snprintf(ringname, sizeof(ringname),
				"%s/.gnupg/secring.gpg", homedir);
		secring = ringname;
	}
	keyring = calloc(1, sizeof(*keyring));
	if (!__ops_keyring_fileread(keyring, 0, secring)) {
		(void) fprintf(stderr, "Cannot read sec keyring %s\n", secring);
		return 0;
	}
	netpgp->secring = keyring;
	netpgp->secringfile = strdup(secring);
	netpgp->userid = strdup(userid);
	return 1;
}

/* finish off with the netpgp_t struct */
int
netpgp_end(netpgp_t *netpgp)
{
	if (netpgp->pubring != NULL) {
		__ops_keyring_free(netpgp->pubring);
	}
	if (netpgp->pubringfile != NULL) {
		(void) free(netpgp->pubringfile);
	}
	if (netpgp->secring != NULL) {
		__ops_keyring_free(netpgp->secring);
	}
	if (netpgp->secringfile != NULL) {
		(void) free(netpgp->secringfile);
	}
	(void) free(netpgp->userid);
	return 1;
}

/* list the keys in a keyring */
int
netpgp_list_keys(netpgp_t *netpgp)
{
	__ops_keyring_list(netpgp->pubring);
	return 1;
}

/* find a key in a keyring */
int
netpgp_find_key(netpgp_t *netpgp, char *id)
{
	if (id == NULL) {
		(void) fprintf(stderr, "NULL id to search for\n");
		return 0;
	}
	return __ops_keyring_find_key_by_userid(netpgp->pubring, id) != NULL;
}

/* export a given key */
int
netpgp_export_key(netpgp_t *netpgp, char *userid)
{
	const __ops_keydata_t	*keypair;

	if (userid == NULL) {
		userid = netpgp->userid;
	}
	keypair = __ops_keyring_find_key_by_userid(netpgp->pubring, userid);
	if (keypair == NULL) {
		(void) fprintf(stderr,
			"Cannot find own key \"%s\" in keyring\n", userid);
		return 0;
	}
	__ops_export_key(keypair, NULL);
	return 1;
}

/* import a key into our keyring */
int
netpgp_import_key(netpgp_t *netpgp, char *f)
{
	int	done;

	if ((done = __ops_keyring_fileread(netpgp->pubring, 0, f)) == 0) {
		done = __ops_keyring_fileread(netpgp->pubring, 1, f);
	}
	if (!done) {
		(void) fprintf(stderr, "Cannot import key from file %s\n", f);
		return 0;
	}
	__ops_keyring_list(netpgp->pubring);
	return 1;
}

/* generate a new key */
int
netpgp_generate_key(netpgp_t *netpgp, char *id, int numbits)
{
	__ops_createinfo_t	*create;
	__ops_keydata_t		*keypair;
	__ops_user_id_t		 uid;
	int             	 fd;

	(void) memset(&uid, 0x0, sizeof(uid));
	uid.user_id = (unsigned char *) id;
	keypair = __ops_rsa_create_selfsigned_keypair(numbits,
				(const unsigned long)65537, &uid);
	if (keypair == NULL) {
		(void) fprintf(stderr, "Cannot generate key\n");
		return 0;
	}
	/* write public key */
	fd = __ops_setup_file_append(&create, netpgp->pubringfile);
	__ops_write_transferable_pubkey(keypair, 0, create);
	__ops_teardown_file_write(create, fd);
	__ops_keyring_free(netpgp->pubring);
	if (!__ops_keyring_fileread(netpgp->pubring, 0,
				netpgp->pubringfile)) {
		(void) fprintf(stderr, "Cannot re-read keyring %s\n",
				netpgp->pubringfile);
		return 0;
	}
	/* write secret key */
	fd = __ops_setup_file_append(&create, netpgp->secringfile);
	__ops_write_transferable_seckey(keypair, NULL, 0, 0, create);
	__ops_teardown_file_write(create, fd);
	__ops_keyring_free(netpgp->secring);
	if (!__ops_keyring_fileread(netpgp->secring, 0,
				netpgp->secringfile)) {
		fprintf(stderr, "Cannot re-read keyring %s\n",
				netpgp->secringfile);
		return 0;
	}
	__ops_keydata_free(keypair);
	return 1;
}

/* encrypt a file */
int
netpgp_encrypt_file(netpgp_t *netpgp, char *userid, char *f, char *out, int armored)
{
	const __ops_keydata_t	*keypair;
	const char		*suffix;
	char			 outname[MAXPATHLEN];

	if (userid == NULL) {
		userid = netpgp->userid;
	}
	suffix = (armored) ? ".asc" : ".gpg";
	keypair = __ops_keyring_find_key_by_userid(netpgp->pubring, userid);
	if (keypair == NULL) {
		(void) fprintf(stderr, "Userid '%s' not found in keyring\n",
					userid);
		return 0;
	}
	if (out == NULL) {
		(void) snprintf(outname, sizeof(outname), "%s%s", f, suffix);
		out = outname;
	}
	return (int)__ops_encrypt_file(f, out, keypair, (unsigned)armored, 1U);
}

/* decrypt a file */
int
netpgp_decrypt_file(netpgp_t *netpgp, char *f, char *out, int armored)
{
	return __ops_decrypt_file(f, out, netpgp->secring, (unsigned)armored,
		1U, get_passphrase_cb);
}

/* sign a file */
int
netpgp_sign_file(netpgp_t *netpgp, char *userid, char *f, char *out,
		int armored, int cleartext, int detached)
{
	const __ops_keydata_t	*keypair;
	__ops_seckey_t		*seckey;
	char			*hashalg;
	char			 passphrase[MAX_PASSPHRASE_LENGTH];

	if (userid == NULL) {
		userid = netpgp->userid;
	}
	/* get key with which to sign */
	keypair = __ops_keyring_find_key_by_userid(netpgp->secring, userid);
	if (keypair == NULL) {
		(void) fprintf(stderr, "Userid '%s' not found in keyring\n",
				userid);
		return 0;
	}
	do {
		/* print out the user id */
		__ops_print_pubkeydata(keypair);
		/* get the passphrase */
		get_pass_phrase(passphrase, sizeof(passphrase));
		/* now decrypt key */
		seckey = __ops_decrypt_seckey(keypair, passphrase);
		if (seckey == NULL) {
			(void) fprintf(stderr, "Bad passphrase\n");
		}
	} while (seckey == NULL);
	/* sign file */
	hashalg = netpgp_getvar(netpgp, "hash");
	if (cleartext) {
		__ops_sign_file_as_cleartext(f, out, seckey, hashalg, 1U);
	} else if (detached) {
		sign_detached(f, out, seckey, hashalg);
	} else {
		__ops_sign_file(f, out, seckey, hashalg, (unsigned)armored, 1);
	}
	(void) memset(passphrase, 0x0, sizeof(passphrase));
	return 1;
}

/* verify a file */
int
netpgp_verify_file(netpgp_t *netpgp, char *infile, const char *outfile,
			int armored)
{
	__ops_validation_t	result;

	(void) memset(&result, 0x0, sizeof(result));
	if (__ops_validate_file(&result, infile, outfile, armored,
				netpgp->pubring)) {
		psuccess(stderr, infile, &result, netpgp->pubring);
		return 1;
	}
	if (result.validc + result.invalidc + result.unknownc == 0) {
		(void) fprintf(stderr,
		"\"%s\": No signatures found - is this a signed file?\n",
			infile);
		return 0;
	}
	(void) fprintf(stderr,
"\"%s\": verification failure: %d invalid signatures, %d unknown signatures\n",
		infile, result.invalidc, result.unknownc);
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

int
netpgp_list_packets(netpgp_t *netpgp, char *f, int armour, char *pubringname)
{
	__ops_keyring_t	*keyring;
	char		 ringname[MAXPATHLEN];
	char		*homedir;

	homedir = getenv("HOME");
	if (pubringname == NULL) {
		(void) snprintf(ringname, sizeof(ringname),
			"%s/.gnupg/pubring.gpg", homedir);
		pubringname = ringname;
	}
	keyring = calloc(1, sizeof(*keyring));
	if (!__ops_keyring_fileread(keyring, 0, pubringname)) {
		(void) fprintf(stderr, "Cannot read pub keyring %s\n",
			pubringname);
		return 0;
	}
	netpgp->pubring = keyring;
	netpgp->pubringfile = strdup(pubringname);
	__ops_list_packets(f, (unsigned)armour, keyring, get_passphrase_cb);
	return 1;
}

/* set a variable */
int
netpgp_setvar(netpgp_t *netpgp, const char *name, const char *value)
{
	if (strcmp(name, "hash") == 0 || strcmp(name, "algorithm") == 0) {
		if (netpgp->hashalg) {
			(void) free(netpgp->hashalg);
			netpgp->hashalg = NULL;
		}
		if (__ops_str_to_hash_alg(value) == OPS_HASH_UNKNOWN) {
			return 0;
		}
		netpgp->hashalg = strdup(value);
		return 1;
	}
	if (strcmp(name, "userid") == 0 || strcmp(name, "user") == 0) {
		if (netpgp->userid) {
			(void) free(netpgp->userid);
			netpgp->userid = NULL;
		}
		netpgp->userid = strdup(value);
		return 1;
	}
	return 0;
}

/* get a variable's value (NULL if not set) */
char *
netpgp_getvar(netpgp_t *netpgp, const char *name)
{
	if (strcmp(name, "hash") == 0 || strcmp(name, "algorithm") == 0) {
		return netpgp->hashalg;
	}
	if (strcmp(name, "userid") == 0 || strcmp(name, "user") == 0) {
		return netpgp->userid;
	}
	return NULL;
}
