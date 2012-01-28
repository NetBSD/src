
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004-2006
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "trousers/tss.h"
#include "trousers_types.h"
#include "trousers_types.h"
#include "tcs_tsp.h"
#include "tcs_utils.h"
#include "tcs_int_literals.h"
#include "capabilities.h"
#include "tcslog.h"
#include "tcsps.h"
#include "req_mgr.h"


TSS_RESULT
ps_init_disk_cache(void)
{
	int fd;
	TSS_RESULT rc;

	MUTEX_INIT(disk_cache_lock);

	if ((fd = get_file()) < 0)
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if ((rc = init_disk_cache(fd)))
		return rc;

	/* this is temporary, to clear out a PS file from trousers
	 * versions before 0.2.1 */
	if ((rc = clean_disk_cache(fd)))
		return rc;

	put_file(fd);
	return TSS_SUCCESS;
}

void
ps_close_disk_cache(void)
{
	int fd;

	if ((fd = get_file()) < 0) {
		LogError("get_file() failed while trying to close disk cache.");
		return;
	}

	close_disk_cache(fd);

	put_file(fd);
}

TSS_BOOL
ps_is_key_registered(TCPA_STORE_PUBKEY *pub)
{
	TSS_UUID *uuid;
	int fd;
	TSS_RESULT rc;
	TSS_BOOL is_reg = FALSE;

	if ((fd = get_file()) < 0)
		return FALSE;

	if ((rc = psfile_get_uuid_by_pub(fd, pub, &uuid))) {
		put_file(fd);
		return FALSE;
	}

	put_file(fd);

	if ((isUUIDRegistered(uuid, &is_reg)))
		is_reg = FALSE;

	free(uuid);
	return is_reg;
}

TSS_RESULT
getParentUUIDByUUID(TSS_UUID *uuid, TSS_UUID *ret_uuid)
{
	struct key_disk_cache *disk_tmp;

	/* check the registered key disk cache */
	MUTEX_LOCK(disk_cache_lock);

	for (disk_tmp = key_disk_cache_head; disk_tmp; disk_tmp = disk_tmp->next) {
		if ((disk_tmp->flags & CACHE_FLAG_VALID) &&
		    !memcmp(&disk_tmp->uuid, uuid, sizeof(TSS_UUID))) {
			memcpy(ret_uuid, &disk_tmp->parent_uuid, sizeof(TSS_UUID));
			MUTEX_UNLOCK(disk_cache_lock);
			return TSS_SUCCESS;
		}
	}
	MUTEX_UNLOCK(disk_cache_lock);

	return TCSERR(TSS_E_FAIL);
}

TSS_RESULT
isUUIDRegistered(TSS_UUID *uuid, TSS_BOOL *is_reg)
{
	struct key_disk_cache *disk_tmp;

	/* check the registered key disk cache */
	MUTEX_LOCK(disk_cache_lock);

	for (disk_tmp = key_disk_cache_head; disk_tmp; disk_tmp = disk_tmp->next) {
		if ((disk_tmp->flags & CACHE_FLAG_VALID) &&
		    !memcmp(&disk_tmp->uuid, uuid, sizeof(TSS_UUID))) {
			*is_reg = TRUE;
			MUTEX_UNLOCK(disk_cache_lock);
			return TSS_SUCCESS;
		}
	}
	MUTEX_UNLOCK(disk_cache_lock);
	*is_reg = FALSE;

	return TSS_SUCCESS;
}

void
disk_cache_shift(struct key_disk_cache *c)
{
	UINT32 key_size, offset;
	struct key_disk_cache *tmp = key_disk_cache_head;

	/* offset is the end of the key location in the file */
	offset = TSSPS_VENDOR_DATA_OFFSET(c) + c->vendor_data_size;
	/* key_size is the size of the key entry on disk */
	key_size = offset - TSSPS_UUID_OFFSET(c);

	/* for each disk cache entry, if the data for that entry is at an
	 * offset greater than the key beign removed, then the entry needs to
	 * be decremented by the size of key's disk footprint (the key_size
	 * variable) */
	while (tmp) {
		if (tmp->offset >= offset) {
			tmp->offset -= key_size;
		}

		tmp = tmp->next;
	}
}

TSS_RESULT
ps_remove_key(TSS_UUID *uuid)
{
	struct key_disk_cache *tmp, *prev = NULL;
	TSS_RESULT rc;
        int fd = -1;

	MUTEX_LOCK(disk_cache_lock);
	tmp = key_disk_cache_head;

	for (; tmp; prev = tmp, tmp = tmp->next) {
		if ((tmp->flags & CACHE_FLAG_VALID) &&
		    !memcmp(uuid, &tmp->uuid, sizeof(TSS_UUID))) {
			if ((fd = get_file()) < 0) {
				rc = TCSERR(TSS_E_INTERNAL_ERROR);
				break;
			}

			rc = psfile_remove_key(fd, tmp);

			put_file(fd);

			/* if moving the file contents around succeeded, then
			 * change the offsets of the keys in the cache in
			 * mem_cache_shift() and remove the key from the
			 * cache. */
			if (!rc) {
				disk_cache_shift(tmp);
				if (prev) {
					prev->next = tmp->next;
				} else {
					key_disk_cache_head = tmp->next;
				}
				free(tmp);
			} else {
				LogError("Error removing registered key.");
			}

			MUTEX_UNLOCK(disk_cache_lock);
			return rc;
		}
	}

	MUTEX_UNLOCK(disk_cache_lock);

	return TCSERR(TCSERR(TSS_E_PS_KEY_NOTFOUND));
}

/*
 * temporary function to clean out blanked keys from a PS file from
 * trousers 0.2.0 and before
 */
TSS_RESULT
clean_disk_cache(int fd)
{
	struct key_disk_cache *tmp, *prev = NULL;
	TSS_RESULT rc;

	MUTEX_LOCK(disk_cache_lock);
	tmp = key_disk_cache_head;

	for (; tmp; prev = tmp, tmp = tmp->next) {
		if (!(tmp->flags & CACHE_FLAG_VALID)) {
			rc = psfile_remove_key(fd, tmp);

			/* if moving the file contents around succeeded, then
			 * change the offsets of the keys in the cache in
			 * mem_cache_shift() and remove the key from the
			 * cache. */
			if (!rc) {
				disk_cache_shift(tmp);
				if (prev) {
					prev->next = tmp->next;
				}
				free(tmp);
			} else {
				LogError("Error removing blank key.");
			}

			MUTEX_UNLOCK(disk_cache_lock);
			return rc;
		}
	}

	MUTEX_UNLOCK(disk_cache_lock);
	return TSS_SUCCESS;
}

TSS_RESULT
ps_get_key_by_uuid(TSS_UUID *uuid, BYTE *blob, UINT16 *blob_size)
{
        int fd = -1;
        TSS_RESULT rc = TSS_SUCCESS;

        if ((fd = get_file()) < 0)
                return TCSERR(TSS_E_INTERNAL_ERROR);

        rc = psfile_get_key_by_uuid(fd, uuid, blob, blob_size);

        put_file(fd);
        return rc;
}

TSS_RESULT
ps_get_key_by_cache_entry(struct key_disk_cache *c, BYTE *blob, UINT16 *blob_size)
{
        int fd = -1;
        TSS_RESULT rc = TSS_SUCCESS;

        if ((fd = get_file()) < 0)
                return TCSERR(TSS_E_INTERNAL_ERROR);

        rc = psfile_get_key_by_cache_entry(fd, c, blob, blob_size);

        put_file(fd);
        return rc;
}

TSS_RESULT
ps_get_vendor_data(struct key_disk_cache *c, UINT32 *size, BYTE **data)
{
        int fd = -1;
        TSS_RESULT rc;

        if ((fd = get_file()) < 0)
                return TCSERR(TSS_E_INTERNAL_ERROR);

        rc = psfile_get_vendor_data(fd, c, size, data);

        put_file(fd);
        return rc;
}

TSS_RESULT
ps_is_pub_registered(TCPA_STORE_PUBKEY *key)
{
        int fd = -1;
        TSS_BOOL answer;

        if ((fd = get_file()) < 0)
                return FALSE;

        if (psfile_is_pub_registered(fd, key, &answer)) {
                put_file(fd);
                return FALSE;
        }

        put_file(fd);
        return answer;
}

TSS_RESULT
ps_get_uuid_by_pub(TCPA_STORE_PUBKEY *pub, TSS_UUID **uuid)
{
        int fd = -1;
	TSS_RESULT ret;

        if ((fd = get_file()) < 0)
                return TCSERR(TSS_E_INTERNAL_ERROR);

        ret = psfile_get_uuid_by_pub(fd, pub, uuid);

        put_file(fd);
        return ret;
}

TSS_RESULT
ps_get_key_by_pub(TCPA_STORE_PUBKEY *pub, UINT32 *size, BYTE **key)
{
        int fd = -1;
	TSS_RESULT ret;

        if ((fd = get_file()) < 0)
                return TCSERR(TSS_E_INTERNAL_ERROR);

        ret = psfile_get_key_by_pub(fd, pub, size, key);

        put_file(fd);
        return ret;
}

TSS_RESULT
ps_write_key(TSS_UUID *uuid, TSS_UUID *parent_uuid, BYTE *vendor_data,
	     UINT32 vendor_size, BYTE *blob, UINT32 blob_size)
{
        int fd = -1;
        TSS_RESULT rc;
	UINT32 parent_ps;
	UINT16 short_blob_size = (UINT16)blob_size;

        if ((fd = get_file()) < 0)
                return TCSERR(TSS_E_INTERNAL_ERROR);

	/* this case needed for PS file init. if the key file doesn't yet exist, the
	 * psfile_get_parent_ps_type_by_uuid() call would fail. */
	if (!memcmp(parent_uuid, &NULL_UUID, sizeof(TSS_UUID))) {
		parent_ps = TSS_PS_TYPE_SYSTEM;
	} else {
		if ((rc = psfile_get_ps_type_by_uuid(fd, parent_uuid, &parent_ps)))
			return rc;
	}

        rc = psfile_write_key(fd, uuid, parent_uuid, &parent_ps, vendor_data,
			      vendor_size, blob, short_blob_size);

        put_file(fd);
        return TSS_SUCCESS;
}
