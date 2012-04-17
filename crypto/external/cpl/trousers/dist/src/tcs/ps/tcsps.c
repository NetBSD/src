
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004-2006
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#if defined (HAVE_BYTEORDER_H)
#include <sys/byteorder.h>
#elif defined (HTOLE_DEFINED)
#ifdef __NetBSD__
#include <sys/endian.h>
#else
#include <endian.h>
#endif
#define LE_16 htole16
#define LE_32 htole32
#define LE_64 htole64
#else
#define LE_16(x) (x)
#define LE_32(x) (x)
#define LE_64(x) (x)
#endif
#include <assert.h>
#include <fcntl.h>
#include <limits.h>

#include "trousers/tss.h"
#include "trousers_types.h"
#include "tcsps.h"
#include "tcs_tsp.h"
#include "tcs_utils.h"
#include "capabilities.h"
#include "tcslog.h"
#include "tcsd_wrap.h"
#include "tcsd.h"

int system_ps_fd = -1;
MUTEX_DECLARE(disk_cache_lock);

static struct flock fl;

int
get_file()
{
	int rc;
	/* check the global file handle first.  If it exists, lock it and return */
	if (system_ps_fd != -1) {
		int rc = 0;

		fl.l_type = F_WRLCK;
		if ((rc = fcntl(system_ps_fd, F_SETLKW, &fl))) {
			LogError("failed to get system PS lock: %s", strerror(errno));
			return -1;
		}

		return system_ps_fd;
	}

	/* open and lock the file */
	system_ps_fd = open(tcsd_options.system_ps_file, O_CREAT|O_RDWR, 0600);
	if (system_ps_fd < 0) {
		LogError("system PS: open() of %s failed: %s",
				tcsd_options.system_ps_file, strerror(errno));
		return -1;
	}

	fl.l_type = F_WRLCK;
	if ((rc = fcntl(system_ps_fd, F_SETLKW, &fl))) {
		LogError("failed to get system PS lock of file %s: %s",
			tcsd_options.system_ps_file, strerror(errno));
		return -1;
	}

	return system_ps_fd;
}

int
put_file(int fd)
{
	int rc = 0;
	/* release the file lock */
	
	fl.l_type = F_UNLCK;
	if ((rc = fcntl(fd, F_SETLKW, &fl))) {
		LogError("failed to unlock system PS file: %s",
			strerror(errno));
		return -1;
	}

	return rc;
}

void
close_file(int fd)
{
	close(fd);
	system_ps_fd = -1;
}

TSS_RESULT
psfile_get_parent_uuid_by_uuid(int fd, TSS_UUID *uuid, TSS_UUID *ret_uuid)
{
        int rc;
        UINT32 file_offset = 0;
        struct key_disk_cache *tmp;

        MUTEX_LOCK(disk_cache_lock);
        tmp = key_disk_cache_head;

        while (tmp) {
                if (memcmp(uuid, &tmp->uuid, sizeof(TSS_UUID)) || !(tmp->flags & CACHE_FLAG_VALID)) {
                        tmp = tmp->next;
                        continue;
                }

                /* jump to the location of the parent uuid */
                file_offset = TSSPS_PARENT_UUID_OFFSET(tmp);

                rc = lseek(fd, file_offset, SEEK_SET);
                if (rc == ((off_t) - 1)) {
                        LogError("lseek: %s", strerror(errno));
                        MUTEX_UNLOCK(disk_cache_lock);
                        return -1;
                }

                if ((rc = read_data(fd, ret_uuid, sizeof(TSS_UUID)))) {
			LogError("%s", __FUNCTION__);
                        MUTEX_UNLOCK(disk_cache_lock);
                        return rc;
                }

                MUTEX_UNLOCK(disk_cache_lock);
                return TSS_SUCCESS;
        }
        MUTEX_UNLOCK(disk_cache_lock);
        /* key not found */
        return -2;
}

/*
 * return a key blob from PS given a uuid
 */
TSS_RESULT
psfile_get_key_by_uuid(int fd, TSS_UUID *uuid, BYTE *ret_buffer, UINT16 *ret_buffer_size)
{
        int rc;
        UINT32 file_offset = 0;
        struct key_disk_cache *tmp;

        MUTEX_LOCK(disk_cache_lock);
        tmp = key_disk_cache_head;

        while (tmp) {
                if (memcmp(uuid, &tmp->uuid, sizeof(TSS_UUID)) || !(tmp->flags & CACHE_FLAG_VALID)) {
                        tmp = tmp->next;
                        continue;
                }

                /* jump to the location of the key blob */
                file_offset = TSSPS_BLOB_DATA_OFFSET(tmp);

                rc = lseek(fd, file_offset, SEEK_SET);
                if (rc == ((off_t) - 1)) {
                        LogError("lseek: %s", strerror(errno));
                        MUTEX_UNLOCK(disk_cache_lock);
                        return TCSERR(TSS_E_INTERNAL_ERROR);
                }

                /* we found the key; file ptr is pointing at the blob */
                if (*ret_buffer_size < tmp->blob_size) {
                        /* not enough room */
                        MUTEX_UNLOCK(disk_cache_lock);
                        return TCSERR(TSS_E_FAIL);
                }

                if ((rc = read_data(fd, ret_buffer, tmp->blob_size))) {
			LogError("%s", __FUNCTION__);
                        MUTEX_UNLOCK(disk_cache_lock);
                        return rc;
                }
		*ret_buffer_size = tmp->blob_size;
		LogDebugUnrollKey(ret_buffer);
                MUTEX_UNLOCK(disk_cache_lock);
                return TSS_SUCCESS;
        }
        MUTEX_UNLOCK(disk_cache_lock);
        /* key not found */
        return TCSERR(TSS_E_FAIL);
}

/*
 * return a key blob from PS given its cache entry. The disk cache must be
 * locked by the caller.
 */
TSS_RESULT
psfile_get_key_by_cache_entry(int fd, struct key_disk_cache *c, BYTE *ret_buffer,
			  UINT16 *ret_buffer_size)
{
        int rc;
        UINT32 file_offset = 0;

	/* jump to the location of the key blob */
	file_offset = TSSPS_BLOB_DATA_OFFSET(c);

	rc = lseek(fd, file_offset, SEEK_SET);
	if (rc == ((off_t) - 1)) {
		LogError("lseek: %s", strerror(errno));
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	/* we found the key; file ptr is pointing at the blob */
	if (*ret_buffer_size < c->blob_size) {
		/* not enough room */
		LogError("%s: Buf size too small. Needed %d bytes, passed %d", __FUNCTION__,
				c->blob_size, *ret_buffer_size);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	if ((rc = read_data(fd, ret_buffer, c->blob_size))) {
		LogError("%s: error reading %d bytes", __FUNCTION__, c->blob_size);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}
	*ret_buffer_size = c->blob_size;

	return TSS_SUCCESS;
}

/*
 * return the vendor data from PS given its cache entry. The disk cache must be
 * locked by the caller.
 */
TSS_RESULT
psfile_get_vendor_data(int fd, struct key_disk_cache *c, UINT32 *size, BYTE **data)
{
        int rc;
        UINT32 file_offset;

	/* jump to the location of the data */
	file_offset = TSSPS_VENDOR_DATA_OFFSET(c);

	rc = lseek(fd, file_offset, SEEK_SET);
	if (rc == ((off_t) - 1)) {
		LogError("lseek: %s", strerror(errno));
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	if ((*data = malloc(c->vendor_data_size)) == NULL) {
		LogError("malloc of %u bytes failed", c->vendor_data_size);
		return TCSERR(TSS_E_OUTOFMEMORY);
	}

	if ((rc = read_data(fd, *data, c->vendor_data_size))) {
		LogError("%s: error reading %u bytes", __FUNCTION__, c->vendor_data_size);
		free(*data);
		*data = NULL;
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}
	*size = c->vendor_data_size;

	return TSS_SUCCESS;
}

TSS_RESULT
psfile_get_ps_type_by_uuid(int fd, TSS_UUID *uuid, UINT32 *ret_ps_type)
{
	struct key_disk_cache *tmp;

	MUTEX_LOCK(disk_cache_lock);
	tmp = key_disk_cache_head;

        while (tmp) {
		if (memcmp(uuid, &tmp->uuid, sizeof(TSS_UUID)) ||
		    !(tmp->flags & CACHE_FLAG_VALID)) {
			tmp = tmp->next;
			continue;
                }

		if (tmp->flags & CACHE_FLAG_PARENT_PS_SYSTEM) {
			*ret_ps_type = TSS_PS_TYPE_SYSTEM;
			goto done;
		} else
			break;
        }

	*ret_ps_type = TSS_PS_TYPE_USER;
done:
	MUTEX_UNLOCK(disk_cache_lock);
	return TSS_SUCCESS;
}

TSS_RESULT
psfile_is_pub_registered(int fd, TCPA_STORE_PUBKEY *pub, TSS_BOOL *is_reg)
{
        int rc;
        UINT32 file_offset = 0;
        struct key_disk_cache *tmp;
	char tmp_buffer[2048];

        MUTEX_LOCK(disk_cache_lock);
        tmp = key_disk_cache_head;

        while (tmp) {
		/* if the key is of the wrong size or is invalid, try the next one */
                if (pub->keyLength != tmp->pub_data_size || !(tmp->flags & CACHE_FLAG_VALID)) {
                        tmp = tmp->next;
                        continue;
                }

		/* we have a valid key with the same key size as the one we're looking for.
		 * grab the pub key data off disk and compare it. */

                /* jump to the location of the public key */
                file_offset = TSSPS_PUB_DATA_OFFSET(tmp);

                rc = lseek(fd, file_offset, SEEK_SET);
                if (rc == ((off_t) - 1)) {
                        LogError("lseek: %s", strerror(errno));
                        MUTEX_UNLOCK(disk_cache_lock);
                        return TCSERR(TSS_E_INTERNAL_ERROR);
                }

		DBG_ASSERT(tmp->pub_data_size < 2048);

		/* read in the key */
                if ((rc = read_data(fd, tmp_buffer, tmp->pub_data_size))) {
			LogError("%s", __FUNCTION__);
                        MUTEX_UNLOCK(disk_cache_lock);
                        return rc;
                }

		/* do the compare */
		if (memcmp(tmp_buffer, pub->key, tmp->pub_data_size)) {
			tmp = tmp->next;
			continue;
		}

		/* the key matches, copy the uuid out */
		*is_reg = TRUE;

                MUTEX_UNLOCK(disk_cache_lock);
                return TSS_SUCCESS;
        }
        MUTEX_UNLOCK(disk_cache_lock);
        /* key not found */
	*is_reg = FALSE;
        return TSS_SUCCESS;
}


TSS_RESULT
psfile_get_uuid_by_pub(int fd, TCPA_STORE_PUBKEY *pub, TSS_UUID **ret_uuid)
{
        int rc;
        UINT32 file_offset = 0;
        struct key_disk_cache *tmp;
	char tmp_buffer[2048];

        MUTEX_LOCK(disk_cache_lock);
        tmp = key_disk_cache_head;

        while (tmp) {
		/* if the key is of the wrong size or is invalid, try the next one */
                if (pub->keyLength != tmp->pub_data_size || !(tmp->flags & CACHE_FLAG_VALID)) {
                        tmp = tmp->next;
                        continue;
                }

		/* we have a valid key with the same key size as the one we're looking for.
		 * grab the pub key data off disk and compare it. */

                /* jump to the location of the public key */
                file_offset = TSSPS_PUB_DATA_OFFSET(tmp);

                rc = lseek(fd, file_offset, SEEK_SET);
                if (rc == ((off_t) - 1)) {
                        LogError("lseek: %s", strerror(errno));
                        MUTEX_UNLOCK(disk_cache_lock);
                        return TCSERR(TSS_E_INTERNAL_ERROR);
                }

		DBG_ASSERT(tmp->pub_data_size < 2048);

		if (tmp->pub_data_size > sizeof(tmp_buffer)) {
			LogError("Source buffer size too big! Size:  %d",
				 tmp->pub_data_size);
			MUTEX_UNLOCK(disk_cache_lock);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		/* read in the key */
                if ((rc = read_data(fd, tmp_buffer, tmp->pub_data_size))) {
			LogError("%s", __FUNCTION__);
                        MUTEX_UNLOCK(disk_cache_lock);
                        return rc;
                }

		/* do the compare */
		if (memcmp(tmp_buffer, pub->key, tmp->pub_data_size)) {
			tmp = tmp->next;
			continue;
		}

		*ret_uuid = (TSS_UUID *)malloc(sizeof(TSS_UUID));
		if (*ret_uuid == NULL) {
			LogError("malloc of %zd bytes failed.", sizeof(TSS_UUID));
                        MUTEX_UNLOCK(disk_cache_lock);
			return TCSERR(TSS_E_OUTOFMEMORY);
		}

		/* the key matches, copy the uuid out */
		memcpy(*ret_uuid, &tmp->uuid, sizeof(TSS_UUID));

                MUTEX_UNLOCK(disk_cache_lock);
                return TSS_SUCCESS;
        }
        MUTEX_UNLOCK(disk_cache_lock);
        /* key not found */
        return TCSERR(TSS_E_PS_KEY_NOTFOUND);
}

TSS_RESULT
psfile_get_key_by_pub(int fd, TCPA_STORE_PUBKEY *pub, UINT32 *size, BYTE **ret_key)
{
        int rc;
        UINT32 file_offset = 0;
        struct key_disk_cache *tmp;
	BYTE tmp_buffer[4096];

        MUTEX_LOCK(disk_cache_lock);
        tmp = key_disk_cache_head;

        while (tmp) {
		/* if the key is of the wrong size or is invalid, try the next one */
                if (pub->keyLength != tmp->pub_data_size || !(tmp->flags & CACHE_FLAG_VALID)) {
                        tmp = tmp->next;
                        continue;
                }

		/* we have a valid key with the same key size as the one we're looking for.
		 * grab the pub key data off disk and compare it. */

                /* jump to the location of the public key */
                file_offset = TSSPS_PUB_DATA_OFFSET(tmp);

                rc = lseek(fd, file_offset, SEEK_SET);
                if (rc == ((off_t) - 1)) {
                        LogError("lseek: %s", strerror(errno));
                        MUTEX_UNLOCK(disk_cache_lock);
                        return TCSERR(TSS_E_INTERNAL_ERROR);
                }

		DBG_ASSERT(tmp->pub_data_size < 2048);
		if (tmp->pub_data_size > sizeof(tmp_buffer)) {
			LogError("Source buffer size too big! Size:  %d",
				 tmp->pub_data_size);
			MUTEX_UNLOCK(disk_cache_lock);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}

		/* read in the key */
                if ((rc = read_data(fd, tmp_buffer, tmp->pub_data_size))) {
			LogError("%s", __FUNCTION__);
                        MUTEX_UNLOCK(disk_cache_lock);
                        return rc;
                }

		/* do the compare */
		if (memcmp(tmp_buffer, pub->key, tmp->pub_data_size)) {
			tmp = tmp->next;
			continue;
		}

                /* jump to the location of the key blob */
                file_offset = TSSPS_BLOB_DATA_OFFSET(tmp);

                rc = lseek(fd, file_offset, SEEK_SET);
                if (rc == ((off_t) - 1)) {
                        LogError("lseek: %s", strerror(errno));
                        MUTEX_UNLOCK(disk_cache_lock);
                        return TCSERR(TSS_E_INTERNAL_ERROR);
                }

		DBG_ASSERT(tmp->blob_size < 4096);
		if (tmp->blob_size > sizeof(tmp_buffer)) {
			LogError("Blob size greater than 4096! Size:  %d",
				 tmp->blob_size);
			MUTEX_UNLOCK(disk_cache_lock);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}

		/* read in the key blob */
                if ((rc = read_data(fd, tmp_buffer, tmp->blob_size))) {
			LogError("%s", __FUNCTION__);
                        MUTEX_UNLOCK(disk_cache_lock);
                        return rc;
                }

		*ret_key = malloc(tmp->blob_size);
		if (*ret_key == NULL) {
			LogError("malloc of %d bytes failed.", tmp->blob_size);
                        MUTEX_UNLOCK(disk_cache_lock);
			return TCSERR(TSS_E_OUTOFMEMORY);
		}

		memcpy(*ret_key, tmp_buffer, tmp->blob_size);
		*size = tmp->blob_size;

                MUTEX_UNLOCK(disk_cache_lock);
                return rc;
        }
        MUTEX_UNLOCK(disk_cache_lock);
        /* key not found */
        return -2;
}

/*
 * disk store format:
 *
 * TrouSerS 0.2.0 and before:
 * Version 0:                  cached?
 * [UINT32   num_keys_on_disk]
 * [TSS_UUID uuid0           ] yes
 * [TSS_UUID uuid_parent0    ] yes
 * [UINT16   pub_data_size0  ] yes
 * [UINT16   blob_size0      ] yes
 * [UINT16   cache_flags0    ] yes
 * [BYTE[]   pub_data0       ]
 * [BYTE[]   blob0           ]
 * [...]
 *
 * TrouSerS 0.2.1+
 * Version 1:                  cached?
 * [BYTE     PS version = '\1']
 * [UINT32   num_keys_on_disk ]
 * [TSS_UUID uuid0            ] yes
 * [TSS_UUID uuid_parent0     ] yes
 * [UINT16   pub_data_size0   ] yes
 * [UINT16   blob_size0       ] yes
 * [UINT32   vendor_data_size0] yes
 * [UINT16   cache_flags0     ] yes
 * [BYTE[]   pub_data0        ]
 * [BYTE[]   blob0            ]
 * [BYTE[]   vendor_data0     ]
 * [...]
 *
 */
TSS_RESULT
psfile_write_key(int fd,
		TSS_UUID *uuid,
		TSS_UUID *parent_uuid,
		UINT32 *parent_ps,
		BYTE *vendor_data,
		UINT32 vendor_size,
		BYTE *key_blob,
		UINT16 key_blob_size)
{
	TSS_KEY key;
	UINT16 pub_key_size, cache_flags = CACHE_FLAG_VALID;
	UINT64 offset;
	int rc = 0;

	/* leaving the cache flag for parent ps type as 0 implies TSS_PS_TYPE_USER */
	if (*parent_ps == TSS_PS_TYPE_SYSTEM)
		cache_flags |= CACHE_FLAG_PARENT_PS_SYSTEM;

	/* Unload the blob to get the public key */
	offset = 0;
	if ((rc = UnloadBlob_TSS_KEY(&offset, key_blob, &key)))
		return rc;

	pub_key_size = key.pubKey.keyLength;

        if ((rc = write_key_init(fd, pub_key_size, key_blob_size, vendor_size)) < 0)
                goto done;

	/* offset now holds the number of bytes from the beginning of the file
	 * the key will be stored at
	 */
	offset = rc;

#ifdef TSS_DEBUG
	if (offset == 0)
		LogDebug("ERROR: key being written with offset 0!!");
#endif

	/* [TSS_UUID uuid0           ] yes */
        if ((rc = write_data(fd, (void *)uuid, sizeof(TSS_UUID)))) {
		LogError("%s", __FUNCTION__);
                goto done;
	}

	/* [TSS_UUID uuid_parent0    ] yes */
        if ((rc = write_data(fd, (void *)parent_uuid, sizeof(TSS_UUID)))) {
		LogError("%s", __FUNCTION__);
                goto done;
	}

	/* [UINT16   pub_data_size0  ] yes */
	pub_key_size = LE_16(pub_key_size);
        if ((rc = write_data(fd, &pub_key_size, sizeof(UINT16)))) {
		LogError("%s", __FUNCTION__);
                goto done;
	}
	/* Swap it back for later */
	pub_key_size = LE_16(pub_key_size);

	/* [UINT16   blob_size0      ] yes */
	key_blob_size = LE_16(key_blob_size);
        if ((rc = write_data(fd, &key_blob_size, sizeof(UINT16)))) {
		LogError("%s", __FUNCTION__);
                goto done;
	}
	/* Swap it back for later */
	key_blob_size = LE_16(key_blob_size);

	/* [UINT32   vendor_data_size0 ] yes */
	vendor_size = LE_32(vendor_size);
        if ((rc = write_data(fd, &vendor_size, sizeof(UINT32)))) {
		LogError("%s", __FUNCTION__);
                goto done;
	}
	/* Swap it back for later */
	vendor_size = LE_32(vendor_size);

	/* [UINT16   cache_flags0    ] yes */
	cache_flags = LE_16(cache_flags);
        if ((rc = write_data(fd, &cache_flags, sizeof(UINT16)))) {
		LogError("%s", __FUNCTION__);
                goto done;
	}
	/* Swap it back for later */
	cache_flags = LE_16(cache_flags);

	/* [BYTE[]   pub_data0       ] no */
        if ((rc = write_data(fd, (void *)key.pubKey.key, pub_key_size))) {
		LogError("%s", __FUNCTION__);
                goto done;
	}

	/* [BYTE[]   blob0           ] no */
        if ((rc = write_data(fd, (void *)key_blob, key_blob_size))) {
		LogError("%s", __FUNCTION__);
                goto done;
	}

	/* [BYTE[]   vendor_data0    ] no */
	if (vendor_size > 0) {
		if ((rc = write_data(fd, (void *)vendor_data, vendor_size))) {
			LogError("%s", __FUNCTION__);
			goto done;
		}
	}

	if ((rc = cache_key((UINT32)offset, cache_flags, uuid, parent_uuid, pub_key_size,
					key_blob_size, vendor_size)))
                goto done;
done:
	destroy_key_refs(&key);

        return rc;
}

TSS_RESULT
psfile_remove_key(int fd, struct key_disk_cache *c)
{
        TSS_RESULT result;
        UINT32 head_offset = 0, tail_offset, num_keys;
	BYTE buf[4096];
	struct stat stat_buf;
	int rc, size = 0;

	if ((rc = fstat(fd, &stat_buf)) != 0) {
		LogError("fstat: %s", strerror(errno));
		return TSS_E_INTERNAL_ERROR;
	}

	/* head_offset is the offset the beginning of the key */
	head_offset = TSSPS_UUID_OFFSET(c);

	/* tail_offset is the offset the beginning of the next key */
	tail_offset = TSSPS_VENDOR_DATA_OFFSET(c) + c->vendor_data_size;

	rc = lseek(fd, tail_offset, SEEK_SET);
	if (rc == ((off_t) - 1)) {
		LogError("lseek: %s", strerror(errno));
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	/* read in from tail, write out to head to fill the gap */
	while ((rc = read(fd, buf, sizeof(buf))) > 0) {
		size = rc;
		tail_offset += size;

		/* set the file pointer to where we want to write */
		rc = lseek(fd, head_offset, SEEK_SET);
		if (rc == ((off_t) - 1)) {
			LogError("lseek: %s", strerror(errno));
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}

		/* write the data */
		if ((result = write_data(fd, (void *)buf, size))) {
			LogError("%s", __FUNCTION__);
			return result;
		}
		head_offset += size;

		/* set the file pointer to where we want to read in the next
		 * loop */
		rc = lseek(fd, tail_offset, SEEK_SET);
		if (rc == ((off_t) - 1)) {
			LogError("lseek: %s", strerror(errno));
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
	}

	if (rc < 0) {
		LogError("read: %s", strerror(errno));
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	/* set the file pointer to where we want to write */
	rc = lseek(fd, head_offset, SEEK_SET);
	if (rc == ((off_t) - 1)) {
		LogError("lseek: %s", strerror(errno));
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	/* head_offset now contains a pointer to where we want to truncate the
	 * file. Zero out the old tail end of the file and truncate it. */

	memset(buf, 0, sizeof(buf));

	/* Zero out the old tail end of the file */
	if ((result = write_data(fd, (void *)buf, tail_offset - head_offset))) {
		LogError("%s", __FUNCTION__);
		return result;
	}

	if ((rc = ftruncate(fd, head_offset)) < 0) {
		LogError("ftruncate: %s", strerror(errno));
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	/* we succeeded in removing a key from the disk. Decrement the number
	 * of keys in the file */
	rc = lseek(fd, TSSPS_NUM_KEYS_OFFSET, SEEK_SET);
	if (rc == ((off_t) - 1)) {
		LogError("lseek: %s", strerror(errno));
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	rc = read(fd, &num_keys, sizeof(UINT32));
	num_keys = LE_32(num_keys);
	if (rc != sizeof(UINT32)) {
		LogError("read of %zd bytes: %s", sizeof(UINT32), strerror(errno));
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	rc = lseek(fd, TSSPS_NUM_KEYS_OFFSET, SEEK_SET);
	if (rc == ((off_t) - 1)) {
		LogError("lseek: %s", strerror(errno));
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	/* decrement, then write back out to disk */
	num_keys--;

	num_keys = LE_32(num_keys);
	if ((result = write_data(fd, (void *)&num_keys, sizeof(UINT32)))) {
		LogError("%s", __FUNCTION__);
		return result;
	}

	return TSS_SUCCESS;
}
