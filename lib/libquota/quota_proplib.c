/*	$NetBSD: quota_proplib.c,v 1.3 2012/01/09 15:34:34 dholland Exp $	*/
/*-
  * Copyright (c) 2011 Manuel Bouyer
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

#include <sys/cdefs.h>
__RCSID("$NetBSD: quota_proplib.c,v 1.3 2012/01/09 15:34:34 dholland Exp $");

#include <string.h>
#include <errno.h>
#include <err.h>

#include <quota.h>
#include "quotapvt.h"

#include <quota/quotaprop.h>
#include <quota/quota.h>

static int
__quota_proplib_getversion(struct quotahandle *qh, int8_t *version_ret)
{
	const char *idtype;
	prop_dictionary_t dict, data, cmd;
	prop_array_t cmds, blank, datas;
	const char *cmdstr;
	struct plistref pref;
	int8_t error8;

	/* XXX does this matter? */
	idtype = ufs_quota_class_names[QUOTA_CLASS_USER];

	/*
	 * XXX this should not crash out on error. But this is what
	 * the code this came from did... probably because it can just
	 * leak memory instead of needing the proper cleanup code.
	 */

	dict = quota_prop_create();
	if (dict == NULL) {
		err(1, "quota_getimplname: quota_prop_create");
	}

	cmds = prop_array_create();
	if (cmds == NULL) {
		err(1, "quota_getimplname: prop_array_create");
	}

	blank = prop_array_create();
	if (blank == NULL) {
		err(1, "quota_getimplname: prop_array_create");
	}

	if (!quota_prop_add_command(cmds, "get version", idtype, blank)) {
		err(1, "quota_getimplname: quota_prop_add_command");
	}

	if (!prop_dictionary_set(dict, "commands", cmds)) {
		err(1, "quota_getimplname: prop_dictionary_set");
	}

	if (prop_dictionary_send_syscall(dict, &pref) != 0) {
		err(1, "quota_getimplname: prop_dictionary_send_syscall");
	}

	/* XXX don't we need prop_object_release(cmds) here too? */
	prop_object_release(dict);

	if (quotactl(qh->qh_mountpoint, &pref) != 0)
		err(1, "quota_getimplname: quotactl");
	
	if (prop_dictionary_recv_syscall(&pref, &dict) != 0) {
		err(1, "quota_getimplname: prop_dictionary_recv_syscall");
	}
				    
	if ((errno = quota_get_cmds(dict, &cmds)) != 0) {
		err(1, "quota_getimplname: bad response (%s)",
		    "quota_get_cmds");
	}

	cmd = prop_array_get(cmds, 0);
	if (cmd == NULL) {
		err(1, "quota_getimplname: bad response (%s)",
		    "prop_array_get");
	}

	if (!prop_dictionary_get_cstring_nocopy(cmd, "command", &cmdstr)) {
		err(1, "quota_getimplname: bad response (%s)",
		    "prop_dictionary_get_cstring_nocopy");
	}

	if (strcmp("get version", cmdstr) != 0) {
		errx(1, "quota_getimplname: bad response (%s)",
		     "command name did not match");
	}
			
	if (!prop_dictionary_get_int8(cmd, "return", &error8)) {
		err(1, "quota_getimplname: bad response (%s)",
		    "prop_dictionary_get_int8");
	}

	if (error8) {
		/* this means the RPC action failed */
		prop_object_release(dict);
		errno = error8;
		return -1;
	}

	datas = prop_dictionary_get(cmd, "data");
	if (datas == NULL) {
		err(1, "quota_getimplname: bad response (%s)",
		    "prop_dictionary_get");
	}

	data = prop_array_get(datas, 0);
	if (data == NULL) {
		err(1, "quota_getimplname: bad response (%s)",
		    "prop_array_get");
	}

	if (!prop_dictionary_get_int8(data, "version", version_ret)) {
		err(1, "quota_getimplname: bad response (%s)",
		    "prop_array_get_int8");
	}

	return 0;
}

const char *
__quota_proplib_getimplname(struct quotahandle *qh)
{
	int8_t version;

	if (__quota_proplib_getversion(qh, &version) < 0) {
		return NULL;
	}
	switch (version) {
		case 1: return "ffs quota1";
		case 2: return "ffs quota2";
		default: break;
	}
	return "unknown";
}

int
__quota_proplib_get(struct quotahandle *qh, const struct quotakey *qk,
		    struct quotaval *qv)
{
	prop_dictionary_t dict, data, cmd;
	prop_array_t cmds, datas;
	struct plistref pref;
	int8_t error8;
	const char *idstr;
	const char *cmdstr;
	uint64_t vals[UFS_QUOTA_NENTRIES];
	uint64_t *valptrs[1];
	int limitcode;
	int serrno;

	switch (qk->qk_idtype) {
	    case QUOTA_IDTYPE_USER:
		idstr = QUOTADICT_CLASS_USER;
		break;
	    case QUOTA_IDTYPE_GROUP:
		idstr = QUOTADICT_CLASS_GROUP;
		break;
	    default:
		errno = EINVAL;
		return -1;
	}

	/*
	 * Cons up the RPC packet.
	 */

	data = prop_dictionary_create();
	if (data == NULL) {
		errno = ENOMEM;
		return -1;
	}
	if (qk->qk_id == QUOTA_DEFAULTID) {
		if (!prop_dictionary_set_cstring(data, "id", "default")) {
			prop_object_release(data);
			errno = ENOMEM;
			return -1;
		}
	} else {
		if (!prop_dictionary_set_uint32(data, "id", qk->qk_id)) {
			prop_object_release(data);
			errno = ENOMEM;
			return -1;
		}
	}

	datas = prop_array_create();
	if (datas == NULL) {
		prop_object_release(data);
		errno = ENOMEM;
		return -1;
	}
	if (!prop_array_add_and_rel(datas, data)) {
		prop_object_release(datas);
		/* DATA is consumed if this fails! */
		errno = ENOMEM;
		return -1;
	}

	cmds = prop_array_create();
	if (cmds == NULL) {
		prop_object_release(datas);
		errno = ENOMEM;
		return -1;
	}
	if (!quota_prop_add_command(cmds, "get", idstr, datas)) {
		prop_object_release(cmds);
		/* AFAICT, CMDS is consumed if this fails, too. */
		errno = ENOMEM;
		return -1;
	}

	dict = quota_prop_create();
	if (dict == NULL) {
		prop_object_release(cmds);
		errno = ENOMEM;
		return -1;
	}
	if (!prop_dictionary_set(dict, "commands", cmds)) {
		prop_object_release(dict);
		/* here CMDS is *not* released on failure. yay consistency! */
		prop_object_release(cmds);
		errno = ENOMEM;
		return -1;
	}
	/* as far as I can tell this is required here - dholland */
	prop_object_release(cmds);

	/*
	 * Convert it to an XML turd for transfer.
	 */
	
	if (prop_dictionary_send_syscall(dict, &pref) != 0) {
		serrno = errno;
		prop_object_release(dict);
		errno = serrno;
		return -1;
	}
	prop_object_release(dict);

	/*
	 * Send it off.
	 *
	 * Note:
	 *
	 * prop_dictionary_send_syscall allocates memory in PREF,
	 * which we ought to free if quotactl fails, but there's no
	 * way (or no documented way) to do this without breaking the
	 * abstraction.
	 *
	 * Furthermore, quotactl replaces the send buffer in PREF
	 * with a receive buffer. (AFAIK at least...) This overwrites
	 * the send buffer and makes it impossible to free it. The
	 * receive buffer is consumed by prop_dictionary_recv_syscall
	 * with munmap(); however, I'm not sure what happens if the
	 * prop_dictionary_recv_syscall operation fails.
	 *
	 * So it at least looks as if the send bundle is leaked on
	 * every quotactl call.
	 *
	 * XXX.
	 *
	 * - dholland 20111125
	 */

	if (quotactl(qh->qh_mountpoint, &pref) != 0) {
		/* XXX free PREF buffer here */
		return -1;
	}
	/* XXX free now-overwritten PREF buffer here */

	/*
	 * Convert the XML response turd.
	 */
	
	if (prop_dictionary_recv_syscall(&pref, &dict) != 0) {
		/* XXX do we have to free the buffer in PREF here? */
		return -1;
	}

	/*
	 * Now unpack the response.
	 */

	/* more consistency, returning an errno instead of setting it */
	errno = quota_get_cmds(dict, &cmds);
	if (errno != 0) {
		prop_object_release(dict);
		return -1;
	}

	cmd = prop_array_get(cmds, 0);
	if (cmd == NULL) {
		prop_object_release(dict);
		errno = EINVAL;
		return -1;
	}

	if (!prop_dictionary_get_cstring_nocopy(cmd, "command", &cmdstr)) {
		/* malformed response from the kernel */
		prop_object_release(dict);
		errno = EINVAL;
		return -1;
	}

	if (strcmp("get", cmdstr) != 0) {
		/* malformed response from the kernel */
		prop_object_release(dict);
		errno = EINVAL;
		return -1;
	}
			
	if (!prop_dictionary_get_int8(cmd, "return", &error8)) {
		/* malformed response from the kernel */
		prop_object_release(dict);
		errno = EINVAL;
		return -1;
	}

	if (error8 == ENODEV) {
		/* XXX this currently means quotas are not enabled */
		/* XXX but there's currently no way to fail in quota_open */
		/* XXX in that case */
		quotaval_clear(qv);
		prop_object_release(dict);
		return 0;
	}
		
	if (error8) {
		/* this means the RPC action failed */
		prop_object_release(dict);
		errno = error8;
		return -1;
	}

	datas = prop_dictionary_get(cmd, "data");
	if (datas == NULL) {
		/* malformed response from the kernel */
		prop_object_release(dict);
		errno = EINVAL;
		return -1;
	}

	if (prop_array_count(datas) == 0) {
		/* No quotas for this id */
		prop_object_release(dict);
		errno = ENOENT;
		return -1;
	}

	data = prop_array_get(datas, 0);
	if (data == NULL) {
		/* malformed response from the kernel */
		prop_object_release(dict);
		errno = EINVAL;
		return -1;
	}

	/*
	 * So, the way proptoquota64 works is that you pass it an
	 * array of pointers to uint64. Each of these pointers is
	 * supposed to point to 5 (UFS_QUOTA_NENTRIES) uint64s. This
	 * array of pointers is the second argument. The third and
	 * forth argument are the names of the five values to extract,
	 * and UFS_QUOTA_NENTRIES. The last two arguments are the
	 * names assocated with the pointers (QUOTATYPE_LDICT_BLOCK,
	 * QUOTADICT_LTYPE_FILE) and the number of pointers. Most of
	 * the existing code was unsafely casting struct quotaval
	 * (formerly struct ufs_quota_entry) to (uint64_t *) and using
	 * that as the block of 5 uint64s. I refuse to countenance
	 * that. Also, most of that code extracts both block and file
	 * limits at once (last arguments are ufs_quota_limit_names
	 * and UFS_QUOTA_NLIMITS) but I only need one.
	 */

	switch (qk->qk_objtype) {
	    case QUOTA_OBJTYPE_BLOCKS:
		limitcode = QUOTA_LIMIT_BLOCK;
		break;
	    case QUOTA_OBJTYPE_FILES:
		limitcode = QUOTA_LIMIT_FILE;
		break;
	    default:
		prop_object_release(dict);
		errno = EINVAL;
		return -1;
	}

	valptrs[0] = vals;
	errno = proptoquota64(data, valptrs,
			      ufs_quota_entry_names, UFS_QUOTA_NENTRIES,
			      &ufs_quota_limit_names[limitcode], 1);
	if (errno) {
		serrno = errno;
		prop_object_release(dict);
		errno = serrno;
		return -1;
	}
	prop_object_release(dict);

	/*
	 * there are no symbolic constants for these indexes! XXX
	 */
	qv->qv_hardlimit = vals[0];
	qv->qv_softlimit = vals[1];
	qv->qv_usage = vals[2];
	qv->qv_expiretime = vals[3];
	qv->qv_grace = vals[4];

	return 0;
}
