/*	$NetBSD: quota_proplib.c,v 1.7 2012/01/25 01:22:56 dholland Exp $	*/
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
__RCSID("$NetBSD: quota_proplib.c,v 1.7 2012/01/25 01:22:56 dholland Exp $");

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <err.h>

#include <quota.h>
#include "quotapvt.h"

#include <quota/quotaprop.h>
#include <quota/quota.h>

struct proplib_quotacursor {
	prop_array_t users;
	prop_array_t groups;

	unsigned numusers;
	unsigned numgroups;

	unsigned haveusers;
	unsigned havegroups;

	unsigned didusers;
	unsigned pos;
	unsigned didblocks;
};

int
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

unsigned
__quota_proplib_getnumidtypes(void)
{
	return QUOTA_NCLASS;
}

const char *
__quota_proplib_idtype_getname(int idtype)
{
	if (idtype < 0 || idtype >= QUOTA_NCLASS) {
		return NULL;
	}
	return ufs_quota_class_names[idtype];
}

unsigned
__quota_proplib_getnumobjtypes(void)
{
	return QUOTA_NLIMITS;
}

const char *
__quota_proplib_objtype_getname(int objtype)
{
	if (objtype < 0 || objtype >= QUOTA_NLIMITS) {
		return NULL;
	}
	return ufs_quota_limit_names[objtype];
}

int
__quota_proplib_objtype_isbytes(int objtype)
{
	switch (objtype) {
		case QUOTA_LIMIT_BLOCK: return 1;
		case QUOTA_LIMIT_FILE: return 0;
		default: break;
	}
	return 0;
}

static int
__quota_proplib_extractval(int objtype, prop_dictionary_t data,
			   struct quotaval *qv)
{
	uint64_t vals[UFS_QUOTA_NENTRIES];
	uint64_t *valptrs[1];
	int limitcode;

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

	switch (objtype) {
	    case QUOTA_OBJTYPE_BLOCKS:
		limitcode = QUOTA_LIMIT_BLOCK;
		break;
	    case QUOTA_OBJTYPE_FILES:
		limitcode = QUOTA_LIMIT_FILE;
		break;
	    default:
		errno = EINVAL;
		return -1;
	}

	valptrs[0] = vals;
	errno = proptoquota64(data, valptrs,
			      ufs_quota_entry_names, UFS_QUOTA_NENTRIES,
			      &ufs_quota_limit_names[limitcode], 1);
	if (errno) {
		return -1;
	}

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

	if (__quota_proplib_extractval(qk->qk_objtype, data, qv)) {
		serrno = errno;
		prop_object_release(dict);
		errno = serrno;
		return -1;
	}

	prop_object_release(dict);

	return 0;
}

int
__quota_proplib_put(struct quotahandle *qh, const struct quotakey *qk,
		    const struct quotaval *qv)
{
	prop_dictionary_t dict, data, cmd;
	prop_array_t cmds, datas;
	struct plistref pref;
	int8_t error8;
	uint64_t *valuesp[QUOTA_NLIMITS];
	const char *idtype;
	unsigned limitcode, otherlimitcode;
	unsigned otherobjtype;
	struct quotakey qk2;
	struct quotaval qv2;

	switch (qk->qk_idtype) {
	    case QUOTA_IDTYPE_USER:
		idtype = ufs_quota_class_names[QUOTA_CLASS_USER];
		break;
	    case QUOTA_IDTYPE_GROUP:
		idtype = ufs_quota_class_names[QUOTA_CLASS_GROUP];
		break;
	    default:
		errno = EINVAL;
		return -1;
	}

	switch (qk->qk_objtype) {
	    case QUOTA_OBJTYPE_BLOCKS:
		limitcode = QUOTA_LIMIT_BLOCK;
		otherlimitcode = QUOTA_LIMIT_FILE;
		otherobjtype = QUOTA_OBJTYPE_FILES;
		break;
	    case QUOTA_OBJTYPE_FILES:
		limitcode = QUOTA_LIMIT_FILE;
		otherlimitcode = QUOTA_LIMIT_BLOCK;
		otherobjtype = QUOTA_OBJTYPE_BLOCKS;
		break;
	    default:
		errno = EINVAL;
		return -1;
	}

	/* XXX in addition to being invalid/unsafe this also discards const */
	valuesp[limitcode] = __UNCONST(&qv->qv_hardlimit);

	/*
	 * You cannot set just the block info or just the file info.
	 * You have to set both together, or EINVAL comes back. So we
	 * have to fetch the current values for the other object type,
	 * and stuff both into the RPC packet. Blah. XXX.
	 */
	qk2.qk_idtype = qk->qk_idtype;
	qk2.qk_id = qk->qk_id;
	qk2.qk_objtype = otherobjtype;
	if (__quota_proplib_get(qh, &qk2, &qv2)) {
		if (errno == ENOENT) {
			/* Nothing there yet, use a blank value */
			quotaval_clear(&qv2);
		} else {
			return -1;
		}
	}
	valuesp[otherlimitcode] = &qv2.qv_hardlimit;

	data = quota64toprop(qk->qk_id, qk->qk_id == QUOTA_DEFAULTID ? 1 : 0,
	    valuesp, ufs_quota_entry_names, UFS_QUOTA_NENTRIES,
	    ufs_quota_limit_names, QUOTA_NLIMITS);

	if (data == NULL)
		err(1, "quota64toprop(id)");

	dict = quota_prop_create();
	cmds = prop_array_create();
	datas = prop_array_create();

	if (dict == NULL || cmds == NULL || datas == NULL) {
		errx(1, "can't allocate proplist");
	}

	if (!prop_array_add_and_rel(datas, data))
		err(1, "prop_array_add(data)");
	
	if (!quota_prop_add_command(cmds, "set", idtype, datas))
		err(1, "prop_add_command");
	if (!prop_dictionary_set(dict, "commands", cmds))
		err(1, "prop_dictionary_set(command)");
#if 0
	if (Dflag)
		printf("message to kernel:\n%s\n",
		    prop_dictionary_externalize(dict));
#endif

	if (prop_dictionary_send_syscall(dict, &pref) != 0)
		err(1, "prop_dictionary_send_syscall");
	prop_object_release(dict);

	if (quotactl(qh->qh_mountpoint, &pref) != 0)
		err(1, "quotactl");

	if (prop_dictionary_recv_syscall(&pref, &dict) != 0) {
		err(1, "prop_dictionary_recv_syscall");
	}

#if 0
	if (Dflag)
		printf("reply from kernel:\n%s\n",
		    prop_dictionary_externalize(dict));
#endif

	if ((errno = quota_get_cmds(dict, &cmds)) != 0) {
		err(1, "quota_get_cmds");
	}
	/* only one command, no need to iter */
	cmd = prop_array_get(cmds, 0);
	if (cmd == NULL)
		err(1, "prop_array_get(cmd)");

	if (!prop_dictionary_get_int8(cmd, "return", &error8))
		err(1, "prop_get(return)");

	if (error8) {
		prop_object_release(dict);
		errno = error8;
		return -1;
	}
	prop_object_release(dict);
	return 0;
}

int
__quota_proplib_delete(struct quotahandle *qh, const struct quotakey *qk)
{
	prop_array_t cmds, datas;
	prop_dictionary_t protodict, dict, data, cmd;
	struct plistref pref;
	int8_t error8;
	bool ret;
	const char *idtype;

	/*
	 * XXX for now we always clear quotas for all objtypes no
	 * matter what's passed in. This is ok (sort of) for now
	 * because the only caller is edquota, which only calls delete
	 * for both blocks and files in immediate succession. But it's
	 * wrong in the long run. I'm not fixing it at the moment
	 * because I expect all this code to be deleted in the near
	 * future.
	 */
	(void)qk->qk_objtype;

	switch (qk->qk_idtype) {
	    case QUOTA_IDTYPE_USER:
		idtype = ufs_quota_class_names[QUOTA_CLASS_USER];
		break;
	    case QUOTA_IDTYPE_GROUP:
		idtype = ufs_quota_class_names[QUOTA_CLASS_GROUP];
		break;
	    default:
		errno = EINVAL;
		return -1;
	}

	/* build a generic command */
	protodict = quota_prop_create();
	cmds = prop_array_create();
	datas = prop_array_create();
	if (protodict == NULL || cmds == NULL || datas == NULL) {
		errx(1, "can't allocate proplist");
	}

	data = prop_dictionary_create();
	if (data == NULL)
		errx(1, "can't allocate proplist");

	ret = prop_dictionary_set_uint32(data, "id", qk->qk_id);
	if (!ret)
		err(1, "prop_dictionary_set(id)");
	if (!prop_array_add_and_rel(datas, data))
		err(1, "prop_array_add(data)");

	if (!quota_prop_add_command(cmds, "clear", idtype, datas))
		err(1, "prop_add_command");

	if (!prop_dictionary_set(protodict, "commands", cmds))
		err(1, "prop_dictionary_set(command)");

#if 0
	if (Dflag) {
		fprintf(stderr, "message to kernel for %s:\n%s\n",
			qh->qh_mountpoint,
			prop_dictionary_externalize(protodict));
	}
#endif

	if (prop_dictionary_send_syscall(protodict, &pref) != 0)
		err(1, "prop_dictionary_send_syscall");
	if (quotactl(qh->qh_mountpoint, &pref) != 0)
		err(1, "quotactl");

	if (prop_dictionary_recv_syscall(&pref, &dict) != 0) {
		err(1, "prop_dictionary_recv_syscall");
	}

#if 0
	if (Dflag) {
		fprintf(stderr, "reply from kernel for %s:\n%s\n",
			qh->qh_mountpoint,
			prop_dictionary_externalize(dict));
	}
#endif

	if ((errno = quota_get_cmds(dict, &cmds)) != 0) {
		err(1, "quota_get_cmds");
	}
	/* only one command, no need to iter */
	cmd = prop_array_get(cmds, 0);
	if (cmd == NULL)
		err(1, "prop_array_get(cmd)");

	if (!prop_dictionary_get_int8(cmd, "return", &error8))
		err(1, "prop_get(return)");
	if (error8) {
		prop_object_release(dict);
		prop_object_release(protodict);
		errno = error8;
		return -1;
	}
	prop_object_release(dict);
	prop_object_release(protodict);
	return 0;
}

static int
__quota_proplib_getall(struct quotahandle *qh, int idtype, prop_array_t *ret)
{
	prop_dictionary_t dict, cmd;
	prop_array_t cmds, datas;
	struct plistref pref;
	int8_t error8;

	/*
	 * XXX this should not crash out on error. But this is what
	 * the code this came from did... probably because it can just
	 * leak memory instead of needing the proper cleanup code.
	 */

	dict = quota_prop_create();
	cmds = prop_array_create();
	datas = prop_array_create();

	if (dict == NULL || cmds == NULL || datas == NULL)
		errx(1, "can't allocate proplist");
	if (!quota_prop_add_command(cmds, "getall",
	    ufs_quota_class_names[idtype], datas))
		err(1, "prop_add_command");
	if (!prop_dictionary_set(dict, "commands", cmds))
		err(1, "prop_dictionary_set(command)");
#if 0
	if (Dflag)
		printf("message to kernel:\n%s\n",
		    prop_dictionary_externalize(dict));
#endif
	if (prop_dictionary_send_syscall(dict, &pref) != 0)
		err(1, "prop_dictionary_send_syscall");
	prop_object_release(dict);

	if (quotactl(quota_getmountpoint(qh), &pref) != 0)
		err(1, "quotactl");

	if (prop_dictionary_recv_syscall(&pref, &dict) != 0) {
		err(1, "prop_dictionary_recv_syscall");
	}
#if 0
	if (Dflag)
		printf("reply from kernel:\n%s\n",
		    prop_dictionary_externalize(dict));
#endif
	if ((errno = quota_get_cmds(dict, &cmds)) != 0) {
		err(1, "quota_get_cmds");
	}

	cmd = prop_array_get(cmds, 0);
	if (cmd == NULL) {
		err(1, "prop_array_get(cmds)");
	}

	const char *cmdstr;
	if (!prop_dictionary_get_cstring_nocopy(cmd, "command",
	    &cmdstr))
		err(1, "prop_get(command)");

	if (!prop_dictionary_get_int8(cmd, "return", &error8))
		err(1, "prop_get(return)");

	if (error8) {
		prop_object_release(dict);
		if (error8 != EOPNOTSUPP) {
			errno = error8;
			warn("get %s quotas",
			    ufs_quota_class_names[idtype]);
		}
		return -1;
	}
	datas = prop_dictionary_get(cmd, "data");
	if (datas == NULL)
		err(1, "prop_dict_get(datas)");

	prop_object_retain(datas);
	prop_object_release(dict);

	*ret = datas;
	return 0;
}

struct proplib_quotacursor *
__quota_proplib_cursor_create(void)
{
	struct proplib_quotacursor *pqc;

	pqc = malloc(sizeof(*pqc));
	if (pqc == NULL) {
		return NULL;
	}

	pqc->users = NULL;
	pqc->numusers = 0;
	pqc->haveusers = 0;

	pqc->groups = NULL;
	pqc->numgroups = 0;
	pqc->havegroups = 0;

	pqc->didusers = 0;
	pqc->pos = 0;
	pqc->didblocks = 0;

	return pqc;
}

/* ARGSUSED */
void
__quota_proplib_cursor_destroy(struct proplib_quotacursor *pqc)
{
	prop_object_release(pqc->users);
	prop_object_release(pqc->groups);
	free(pqc);
}

static int
__quota_proplib_cursor_load(struct quotahandle *qh,
			    struct proplib_quotacursor *pqc)
{
	prop_array_t users, groups;

	if (pqc->haveusers == 0) {
		if (__quota_proplib_getall(qh, QUOTA_IDTYPE_USER, &users)) {
			return -1;
		}
		pqc->users = users;
		pqc->numusers = prop_array_count(users);
		pqc->haveusers = 1;
	}

	if (pqc->havegroups == 0) {
		if (__quota_proplib_getall(qh, QUOTA_IDTYPE_GROUP, &groups)) {
			return -1;
		}
		pqc->groups = groups;
		pqc->numgroups = prop_array_count(groups);
		pqc->havegroups = 1;
	}
	return 0;
}

int
__quota_proplib_cursor_skipidtype(struct proplib_quotacursor *pqc,
				  unsigned idtype)
{
	switch (idtype) {
	    case QUOTA_IDTYPE_USER:
		/* if not yet loaded, numusers will be 0 and users NULL */
		pqc->haveusers = 1;
		break;
	    case QUOTA_IDTYPE_GROUP:
		/* if not yet loaded, numgroups will be 0 and groups NULL */
		pqc->havegroups = 1;
		break;
	    default:
		errno = EINVAL;
		return -1;
	}
	return 0;
}

static int
__quota_proplib_cursor_subget(struct proplib_quotacursor *pqc,
			      prop_dictionary_t data,
			      struct quotakey *key, struct quotaval *val)
{
	uint32_t id;
	const char *strid;

	if (prop_dictionary_get_uint32(data, "id", &id)) {
		key->qk_id = id;
	} else if (prop_dictionary_get_cstring_nocopy(data,
						      "id", &strid) &&
		   !strcmp(strid, "default")) {
		key->qk_id = QUOTA_DEFAULTID;
	} else {
		/* invalid bundle */
		errno = EINVAL;
		return -1;
	}
	if (__quota_proplib_extractval(key->qk_objtype, data, val)) {
		return -1;
	}
	return 0;
}

int
__quota_proplib_cursor_get(struct quotahandle *qh,
			   struct proplib_quotacursor *pqc,
			   struct quotakey *key, struct quotaval *val)
{
	prop_dictionary_t data;

	if (pqc->haveusers == 0 || pqc->havegroups == 0) {
		if (__quota_proplib_cursor_load(qh, pqc)) {
			return -1;
		}
	}

	if (!pqc->didblocks) {
		key->qk_objtype = QUOTA_OBJTYPE_BLOCKS;
	} else {
		key->qk_objtype = QUOTA_OBJTYPE_FILES;
	}

	if (!pqc->didusers && pqc->pos >= pqc->numusers) {
		/* in case there are 0 users */
		pqc->didusers = 1;
	}

	if (!pqc->didusers) {
		key->qk_idtype = QUOTA_IDTYPE_USER;

		data = prop_array_get(pqc->users, pqc->pos);
		if (data == NULL) {
			errno = ENOENT;
			return -1;
		}

		/* get id and value */
		if (__quota_proplib_cursor_subget(pqc, data, key, val)) {
			return -1;
		}

		/* advance */
		if (!pqc->didblocks) {
			pqc->didblocks = 1;
		} else {
			pqc->didblocks = 0;
			pqc->pos++;
			if (pqc->pos >= pqc->numusers) {
				pqc->pos = 0;
				pqc->didusers = 1;
			}
		}

		/* succeed */
		return 0;
	} else if (pqc->pos < pqc->numgroups) {
		key->qk_idtype = QUOTA_IDTYPE_GROUP;

		data = prop_array_get(pqc->groups, pqc->pos);
		if (data == NULL) {
			errno = ENOENT;
			return -1;
		}

		/* get id and value */
		if (__quota_proplib_cursor_subget(pqc, data, key, val)) {
			return -1;
		}

		/* advance */
		if (!pqc->didblocks) {
			pqc->didblocks = 1;
		} else {
			pqc->didblocks = 0;
			pqc->pos++;
		}

		/* succeed */
		return 0;
	} else {
		/* at EOF */
		/* XXX is there a better errno for this? */
		errno = ENOENT;
		return -1;
	}
}

int
__quota_proplib_cursor_getn(struct quotahandle *qh,
			    struct proplib_quotacursor *pqc,
			    struct quotakey *keys, struct quotaval *vals,
			    unsigned maxnum)
{
	unsigned i;

	if (maxnum > INT_MAX) {
		/* joker, eh? */
		errno = EINVAL;
		return -1;
	}

	for (i=0; i<maxnum; i++) {
		if (__quota_proplib_cursor_atend(qh, pqc)) {
			break;
		}
		if (__quota_proplib_cursor_get(qh, pqc, &keys[i], &vals[i])) {
			if (i > 0) {
				/*
				 * Succeed witih what we have so far;
				 * the next attempt will hit the same
				 * error again.
				 */
				break;
			}
			return -1;
		}
	}
	return i;
}

int
__quota_proplib_cursor_atend(struct quotahandle *qh,
			     struct proplib_quotacursor *pqc)
{
	if (!pqc->haveusers || !pqc->havegroups) {
		if (__quota_proplib_cursor_load(qh, pqc)) {
			/*
			 * Cannot fail here - report that we are not
			 * at EOF (lying if necessary) and let the
			 * next get call try to load again, fail and
			 * return the proper error. 
			 */
			return 0;
		}
	}

	if (!pqc->didusers && pqc->pos >= pqc->numusers) {
		pqc->didusers = 1;
	}

	if (!pqc->didusers) {
		return 0;
	}
	if (pqc->pos < pqc->numgroups) {
		return 0;
	}
	return 1;
}

int
__quota_proplib_cursor_rewind(struct proplib_quotacursor *pqc)
{
	pqc->didusers = 0;
	pqc->pos = 0;
	pqc->didblocks = 0;
	return 0;
}
