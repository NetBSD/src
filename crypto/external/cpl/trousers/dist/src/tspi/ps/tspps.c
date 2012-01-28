
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
#include <pwd.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <assert.h>
#include <fcntl.h>
#include <limits.h>
#include <netdb.h>
#if defined (HAVE_BYTEORDER_H)
#include <sys/byteorder.h>
#elif defined(HTOLE_DEFINED)
#include <endian.h>
#define LE_16 htole16
#define LE_32 htole32
#define LE_64 htole64
#else
#define LE_16(x) (x)
#define LE_32(x) (x)
#define LE_64(x) (x)
#endif

#include "trousers/tss.h"
#include "trousers/trousers.h"
#include "trousers_types.h"
#include "tcs_tsp.h"
#include "spi_utils.h"
#include "tspps.h"
#include "tsplog.h"

static int user_ps_fd = -1;
static MUTEX_DECLARE_INIT(user_ps_lock);
#if (defined (__FreeBSD__) || defined (__OpenBSD__))
static MUTEX_DECLARE_INIT(user_ps_path);
#endif
static struct flock fl;


/*
 * Determine the default path to the persistent storage file and create it if it doesn't exist.
 */
TSS_RESULT
get_user_ps_path(char **file)
{
	TSS_RESULT result;
	char *file_name = NULL, *home_dir = NULL;
	struct passwd *pwp;
#if (defined (__linux) || defined (linux) || defined(__GLIBC__))
	struct passwd pw;
#endif
	struct stat stat_buf;
	char buf[PASSWD_BUFSIZE];
	uid_t euid;
	int rc;

	if ((file_name = getenv("TSS_USER_PS_FILE"))) {
		*file = strdup(file_name);
		return (*file) ? TSS_SUCCESS : TSPERR(TSS_E_OUTOFMEMORY);
	}
#if (defined (__FreeBSD__) || defined (__OpenBSD__))
	MUTEX_LOCK(user_ps_path);
#endif

	euid = geteuid();

#if defined (SOLARIS)
	/*
         * Solaris keeps user PS in a local directory instead of
         * in the user's home directory, which may be shared
         * by multiple systems.
         *
         * The directory path on Solaris is /var/tpm/userps/[EUID]/
         */
        rc = snprintf(buf, sizeof (buf), "%s/%d", TSS_USER_PS_DIR, euid);
#else
	setpwent();
	while (1) {
#if (defined (__linux) || defined (linux) || defined(__GLIBC__))
		rc = getpwent_r(&pw, buf, PASSWD_BUFSIZE, &pwp);
		if (rc) {
			LogDebugFn("USER PS: Error getting path to home directory: getpwent_r: %s",
				   strerror(rc));
			endpwent();
			return TSPERR(TSS_E_INTERNAL_ERROR);
		}

#elif (defined (__FreeBSD__) || defined (__OpenBSD__))
		if ((pwp = getpwent()) == NULL) {
			LogDebugFn("USER PS: Error getting path to home directory: getpwent: %s",
                                   strerror(rc));
			endpwent();
			MUTEX_UNLOCK(user_ps_path);
			return TSPERR(TSS_E_INTERNAL_ERROR);
		}
#endif
		if (euid == pwp->pw_uid) {
                        home_dir = strdup(pwp->pw_dir);
                        break;
                }
        }
        endpwent();

	if (!home_dir)
		return TSPERR(TSS_E_OUTOFMEMORY);

	/* Tack on TSS_USER_PS_DIR and see if it exists */
	rc = snprintf(buf, sizeof (buf), "%s/%s", home_dir, TSS_USER_PS_DIR);
#endif /* SOLARIS */
	if (rc == sizeof (buf)) {
		LogDebugFn("USER PS: Path to file too long! (> %d bytes)", PASSWD_BUFSIZE);
		result = TSPERR(TSS_E_INTERNAL_ERROR);
		goto done;
	}

	errno = 0;
	if ((rc = stat(buf, &stat_buf)) == -1) {
		if (errno == ENOENT) {
			errno = 0;
			/* Create the user's ps directory if it is not there. */
			if ((rc = mkdir(buf, 0700)) == -1) {
				LogDebugFn("USER PS: Error creating dir: %s: %s", buf,
					   strerror(errno));
				result = TSPERR(TSS_E_INTERNAL_ERROR);
				goto done;
			}
		} else {
			LogDebugFn("USER PS: Error stating dir: %s: %s", buf, strerror(errno));
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}
	}

	/* Directory exists or has been created, return the path to the file */
#if defined (SOLARIS)
	rc = snprintf(buf, sizeof (buf), "%s/%d/%s", TSS_USER_PS_DIR, euid,
		      TSS_USER_PS_FILE);
#else
	rc = snprintf(buf, sizeof (buf), "%s/%s/%s", home_dir, TSS_USER_PS_DIR,
		      TSS_USER_PS_FILE);
#endif
	if (rc == sizeof (buf)) {
		LogDebugFn("USER PS: Path to file too long! (> %zd bytes)", sizeof (buf));
	} else
		*file = strdup(buf);

	result = (*file) ? TSS_SUCCESS : TSPERR(TSS_E_OUTOFMEMORY);
done:
	free(home_dir);
	return result;
}

TSS_RESULT
get_file(int *fd)
{
	TSS_RESULT result;
	int rc = 0;
	char *file_name = NULL;

	MUTEX_LOCK(user_ps_lock);

	/* check the global file handle first.  If it exists, lock it and return */
	if (user_ps_fd != -1) {
		fl.l_type = F_WRLCK;
		if ((rc = fcntl(user_ps_fd, F_SETLKW, &fl))) {
			LogDebug("USER PS: failed to lock file: %s", strerror(errno));
			MUTEX_UNLOCK(user_ps_lock);
			return TSPERR(TSS_E_INTERNAL_ERROR);
		}
		*fd = user_ps_fd;
		return TSS_SUCCESS;
	}

	/* open and lock the file */
	if ((result = get_user_ps_path(&file_name))) {
		LogDebugFn("USER PS: error getting file path");
		MUTEX_UNLOCK(user_ps_lock);
		return result;
	}

	user_ps_fd = open(file_name, O_CREAT|O_RDWR, 0600);
	if (user_ps_fd < 0) {
		LogDebug("USER PS: open of %s failed: %s", file_name, strerror(errno));
		free(file_name);
		MUTEX_UNLOCK(user_ps_lock);
		return TSPERR(TSS_E_INTERNAL_ERROR);
	}
	fl.l_type = F_WRLCK;
	if ((rc = fcntl(user_ps_fd, F_SETLKW, &fl))) {
		LogDebug("USER PS: failed to get lock of %s: %s", file_name, strerror(errno));
		free(file_name);
		close(user_ps_fd);
		user_ps_fd = -1;
		MUTEX_UNLOCK(user_ps_lock);
		return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	*fd = user_ps_fd;
	free(file_name);
	return TSS_SUCCESS;
}

int
put_file(int fd)
{
	int rc = 0;

	fsync(fd);

	/* release the file lock */
	fl.l_type = F_UNLCK;
	if ((rc = fcntl(fd, F_SETLKW, &fl))) {
		LogDebug("USER PS: failed to unlock file: %s", strerror(errno));
		rc = -1;
	}

	MUTEX_UNLOCK(user_ps_lock);
	return rc;
}

void
psfile_close(int fd)
{
	close(fd);
	user_ps_fd = -1;
	MUTEX_UNLOCK(user_ps_lock);
}

TSS_RESULT
psfile_is_key_registered(int fd, TSS_UUID *uuid, TSS_BOOL *answer)
{
        TSS_RESULT result;
        struct key_disk_cache tmp;

	if ((result = psfile_get_cache_entry_by_uuid(fd, uuid, &tmp)) == TSS_SUCCESS)
		*answer = TRUE;
	else if (result == (TSS_E_PS_KEY_NOTFOUND | TSS_LAYER_TSP))
		*answer = FALSE;
        else
                return result;

        return TSS_SUCCESS;
}

TSS_RESULT
psfile_get_parent_uuid_by_uuid(int fd, TSS_UUID *uuid, TSS_UUID *ret_uuid)
{
	TSS_RESULT result;
        struct key_disk_cache tmp;

	if ((result = psfile_get_cache_entry_by_uuid(fd, uuid, &tmp)))
		return result;

	memcpy(ret_uuid, &tmp.parent_uuid, sizeof(TSS_UUID));

        return TSS_SUCCESS;
}

TSS_RESULT
psfile_get_parent_ps_type(int fd, TSS_UUID *uuid, UINT32 *type)
{
	TSS_RESULT result;
        struct key_disk_cache tmp;

	if ((result = psfile_get_cache_entry_by_uuid(fd, uuid, &tmp)))
		return result;

	if (tmp.flags & CACHE_FLAG_PARENT_PS_SYSTEM)
		*type = TSS_PS_TYPE_SYSTEM;
	else
		*type = TSS_PS_TYPE_USER;

        return TSS_SUCCESS;
}

/*
 * return a key struct from PS given a uuid
 */
TSS_RESULT
psfile_get_key_by_uuid(int fd, TSS_UUID *uuid, BYTE *key)
{
        int rc;
	TSS_RESULT result;
        off_t file_offset;
        struct key_disk_cache tmp;
	BYTE buf[4096];

	if ((result = psfile_get_cache_entry_by_uuid(fd, uuid, &tmp)))
		return result;

	/* jump to the location of the key blob */
	file_offset = TSSPS_BLOB_DATA_OFFSET(&tmp);

	rc = lseek(fd, file_offset, SEEK_SET);
	if (rc == ((off_t)-1)) {
		LogDebugFn("lseek: %s", strerror(errno));
		return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	if (tmp.blob_size > 4096) {
		LogError("Blob size greater than 4096! Size:  %d",
			  tmp.blob_size);
		return TSPERR(TSS_E_INTERNAL_ERROR);
	}
	if ((rc = read_data(fd, buf, tmp.blob_size))) {
		LogDebugFn("Blob read from disk failed.");
		return rc;
	}

	memcpy(key, buf, tmp.blob_size);
	return TSS_SUCCESS;
}

/*
 * return a key struct from PS given a public key
 */
TSS_RESULT
psfile_get_key_by_pub(int fd, TSS_UUID *uuid, UINT32 pub_size, BYTE *pub, BYTE *key)
{
        int rc;
	TSS_RESULT result;
        off_t file_offset;
        struct key_disk_cache tmp;
	BYTE buf[4096];

	if ((result = psfile_get_cache_entry_by_pub(fd, pub_size, pub, &tmp)))
		return result;

	/* jump to the location of the key blob */
	file_offset = TSSPS_BLOB_DATA_OFFSET(&tmp);

	rc = lseek(fd, file_offset, SEEK_SET);
	if (rc == ((off_t)-1)) {
		LogDebugFn("lseek: %s", strerror(errno));
		return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	if (tmp.blob_size > 4096) {
		LogError("Blob size greater than 4096! Size:  %d",
			  tmp.blob_size);
		return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	if ((result = read_data(fd, buf, tmp.blob_size))) {
		LogDebugFn("Blob read from disk failed.");
		return result;
	}

	memcpy(key, buf, tmp.blob_size);
	memcpy(uuid, &tmp.uuid, sizeof(TSS_UUID));

	return TSS_SUCCESS;
}

TSS_RESULT
psfile_get_uuid_by_pub(int fd, UINT32 pub_size, BYTE *pub, TSS_UUID *uuid)
{
	TSS_RESULT result;
        struct key_disk_cache tmp;

	if ((result = psfile_get_cache_entry_by_pub(fd, pub_size, pub, &tmp)))
		return result;

	memcpy(uuid, &tmp.uuid, sizeof(TSS_UUID));

        return TSS_SUCCESS;
}

TSS_RESULT
psfile_change_num_keys(int fd, BYTE increment)
{
	int rc;
	TSS_RESULT result;
	UINT32 num_keys;

	rc = lseek(fd, TSSPS_NUM_KEYS_OFFSET, SEEK_SET);
	if (rc == ((off_t)-1)) {
		LogDebug("lseek: %s", strerror(errno));
		return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	rc = read(fd, &num_keys, sizeof(UINT32));
	if (rc != sizeof(UINT32)) {
		LogDebug("read of %zd bytes: %s", sizeof(UINT32), strerror(errno));
		return TSPERR(TSS_E_INTERNAL_ERROR);
	}
	num_keys = LE_32(num_keys);

	if (increment)
		num_keys++;
	else
		num_keys--;

	rc = lseek(fd, TSSPS_NUM_KEYS_OFFSET, SEEK_SET);
	if (rc == ((off_t)-1)) {
		LogDebug("lseek: %s", strerror(errno));
		return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	num_keys = LE_32(num_keys);
	if ((result = write_data(fd, (void *)&num_keys, sizeof(UINT32)))) {
		LogDebug("%s", __FUNCTION__);
		return result;
	}

	return TSS_SUCCESS;
}

/* Write the initial header (number of keys and PS version) to initialize a new file */
TSS_RESULT
psfile_write_key_header(int fd)
{
	int rc;
	TSS_RESULT result;
	UINT32 i;

	rc = lseek(fd, TSSPS_VERSION_OFFSET, SEEK_SET);
	if (rc == ((off_t)-1)) {
		LogDebug("lseek: %s", strerror(errno));
		return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	i = TSSPS_VERSION;
        if ((result = write_data(fd, &i, sizeof(BYTE)))) {
		LogDebug("%s", __FUNCTION__);
		return result;
	}

	rc = lseek(fd, TSSPS_NUM_KEYS_OFFSET, SEEK_SET);
	if (rc == ((off_t)-1)) {
		LogDebug("lseek: %s", strerror(errno));
		return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	i = 0;
        if ((result = write_data(fd, &i, sizeof(UINT32)))) {
		LogDebug("%s", __FUNCTION__);
		return result;
	}

	return TSS_SUCCESS;
}

/*
 * disk store format:
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
		 UINT32 parent_ps,
		 BYTE *key_blob,
		 UINT16 key_blob_size)
{
	TSS_RESULT result;
	TSS_KEY key;
	UINT32 zero = 0;
	UINT64 offset;
	UINT16 pub_key_size, cache_flags = 0;
	struct stat stat_buf;
	int rc, file_offset;

	/* leaving the cache flag for parent ps type as 0 implies TSS_PS_TYPE_USER */
	if (parent_ps == TSS_PS_TYPE_SYSTEM)
		cache_flags |= CACHE_FLAG_PARENT_PS_SYSTEM;

	if ((rc = fstat(fd, &stat_buf)) == -1) {
		LogDebugFn("stat failed: %s", strerror(errno));
		return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	file_offset = stat_buf.st_size;

	if (file_offset < (int)TSSPS_KEYS_OFFSET) {
		if ((result = psfile_write_key_header(fd)))
			return result;
		file_offset = TSSPS_KEYS_OFFSET;
	}

	rc = lseek(fd, file_offset, SEEK_SET);
	if (rc == ((off_t)-1)) {
		LogDebug("lseek: %s", strerror(errno));
		return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	/* Unload the blob to get the public key */
	offset = 0;
	if ((result = UnloadBlob_TSS_KEY(&offset, key_blob, &key)))
		return result;

	pub_key_size = key.pubKey.keyLength;

	/* [TSS_UUID uuid0           ] yes */
        if ((result = write_data(fd, (void *)uuid, sizeof(TSS_UUID)))) {
		LogDebug("%s", __FUNCTION__);
		goto done;
	}

	/* [TSS_UUID uuid_parent0    ] yes */
        if ((result = write_data(fd, (void *)parent_uuid, sizeof(TSS_UUID)))) {
		LogDebug("%s", __FUNCTION__);
		goto done;
	}

	/* [UINT16   pub_data_size0  ] yes */
	pub_key_size = LE_16(pub_key_size);
        if ((result = write_data(fd, &pub_key_size, sizeof(UINT16)))) {
		LogDebug("%s", __FUNCTION__);
		goto done;
	}
	pub_key_size = LE_16(pub_key_size);

	/* [UINT16   blob_size0      ] yes */
	key_blob_size = LE_16(key_blob_size);
        if ((result = write_data(fd, &key_blob_size, sizeof(UINT16)))) {
		LogDebug("%s", __FUNCTION__);
		goto done;
	}
	key_blob_size = LE_16(key_blob_size);

	/* [UINT32   vendor_data_size0 ] yes */
        if ((result = write_data(fd, &zero, sizeof(UINT32)))) {
		LogDebug("%s", __FUNCTION__);
		goto done;
	}

	/* [UINT16   cache_flags0    ] yes */
	cache_flags = LE_16(cache_flags);
        if ((result = write_data(fd, &cache_flags, sizeof(UINT16)))) {
		LogDebug("%s", __FUNCTION__);
		goto done;
	}
	cache_flags = LE_16(cache_flags);

	/* [BYTE[]   pub_data0       ] no */
        if ((result = write_data(fd, (void *)key.pubKey.key, pub_key_size))) {
		LogDebug("%s", __FUNCTION__);
		goto done;
	}

	/* [BYTE[]   blob0           ] no */
        if ((result = write_data(fd, (void *)key_blob, key_blob_size))) {
		LogDebug("%s", __FUNCTION__);
		goto done;
	}

	if ((result = psfile_change_num_keys(fd, TSS_PSFILE_INCREMENT_NUM_KEYS))) {
		LogDebug("%s", __FUNCTION__);
		goto done;
	}

done:
	free_key_refs(&key);
        return result;
}

TSS_RESULT
psfile_remove_key(int fd, TSS_UUID *uuid)
{
        TSS_RESULT result;
        UINT32 head_offset = 0, tail_offset;
	int rc, size = 0;
	struct key_disk_cache c;
	BYTE buf[4096];

	if ((result = psfile_get_cache_entry_by_uuid(fd, uuid, &c)))
		return result;

	/* head_offset is the offset the beginning of the key */
	head_offset = TSSPS_UUID_OFFSET(&c);

	/* tail_offset is the offset the beginning of the next key */
	tail_offset = TSSPS_VENDOR_DATA_OFFSET(&c) + c.vendor_data_size;

	rc = lseek(fd, tail_offset, SEEK_SET);
	if (rc == ((off_t)-1)) {
		LogDebug("lseek: %s", strerror(errno));
		return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	/* read in from tail, write out to head to fill the gap */
	while ((rc = read(fd, buf, sizeof(buf))) > 0) {
		size = rc;
		tail_offset += size;

		/* set the file pointer to where we want to write */
		rc = lseek(fd, head_offset, SEEK_SET);
		if (rc == ((off_t)-1)) {
			LogDebug("lseek: %s", strerror(errno));
			return TSPERR(TSS_E_INTERNAL_ERROR);
		}

		/* write the data */
		if ((result = write_data(fd, (void *)buf, size))) {
			LogDebug("%s", __FUNCTION__);
			return result;
		}
		head_offset += size;

		/* set the file pointer to where we want to read in the next
		 * loop */
		rc = lseek(fd, tail_offset, SEEK_SET);
		if (rc == ((off_t)-1)) {
			LogDebug("lseek: %s", strerror(errno));
			return TSPERR(TSS_E_INTERNAL_ERROR);
		}
	}

	if (rc < 0) {
		LogDebug("read: %s", strerror(errno));
		return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	/* set the file pointer to where we want to write */
	rc = lseek(fd, head_offset, SEEK_SET);
	if (rc == ((off_t)-1)) {
		LogDebug("lseek: %s", strerror(errno));
		return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	/* head_offset now contains a pointer to where we want to truncate the
	 * file. Zero out the old tail end of the file and truncate it. */

	memset(buf, 0, sizeof(buf));

	/* Zero out the old tail end of the file */
	if ((result = write_data(fd, (void *)buf, tail_offset - head_offset))) {
		LogDebug("%s", __FUNCTION__);
		return result;
	}

	if ((rc = ftruncate(fd, head_offset)) < 0) {
		LogDebug("ftruncate: %s", strerror(errno));
		return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	/* we succeeded in removing a key from the disk. Decrement the number
	 * of keys in the file */
	if ((result = psfile_change_num_keys(fd, TSS_PSFILE_DECREMENT_NUM_KEYS)))
		return result;

	return TSS_SUCCESS;
}

TSS_RESULT
psfile_get_all_cache_entries(int fd, UINT32 *size, struct key_disk_cache **c)
{
	UINT32 i, num_keys = psfile_get_num_keys(fd);
	int offset;
	TSS_RESULT result;
	struct key_disk_cache *tmp = NULL;

	if (num_keys == 0) {
		*size = 0;
		*c = NULL;
		return TSS_SUCCESS;
	}

	/* make sure the file pointer is where we expect, just after the number
	 * of keys on disk at the head of the file
	 */
	offset = lseek(fd, TSSPS_KEYS_OFFSET, SEEK_SET);
	if (offset == ((off_t)-1)) {
		LogDebug("lseek: %s", strerror(errno));
		return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	if ((tmp = malloc(num_keys * sizeof(struct key_disk_cache))) == NULL) {
		LogDebug("malloc of %zu bytes failed.", num_keys * sizeof(struct key_disk_cache));
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

	for (i = 0; i < num_keys; i++) {
		offset = lseek(fd, 0, SEEK_CUR);
		if (offset == ((off_t)-1)) {
			LogDebug("lseek: %s", strerror(errno));
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto err_exit;
		}
		tmp[i].offset = offset;

		/* read UUID */
		if ((result = read_data(fd, &tmp[i].uuid, sizeof(TSS_UUID)))) {
			LogDebug("%s", __FUNCTION__);
			goto err_exit;
		}

		/* read parent UUID */
		if ((result = read_data(fd, &tmp[i].parent_uuid, sizeof(TSS_UUID)))) {
			LogDebug("%s", __FUNCTION__);
			goto err_exit;
		}

		/* pub data size */
		if ((result = read_data(fd, &tmp[i].pub_data_size, sizeof(UINT16)))) {
			LogDebug("%s", __FUNCTION__);
			goto err_exit;
		}
		tmp[i].pub_data_size = LE_16(tmp[i].pub_data_size);

		DBG_ASSERT(tmp[i].pub_data_size <= 2048);

		/* blob size */
		if ((result = read_data(fd, &tmp[i].blob_size, sizeof(UINT16)))) {
			LogDebug("%s", __FUNCTION__);
			goto err_exit;
		}
		tmp[i].blob_size = LE_16(tmp[i].blob_size);

		DBG_ASSERT(tmp[i].blob_size <= 4096);

		/* vendor data size */
		if ((result = read_data(fd, &tmp[i].vendor_data_size, sizeof(UINT32)))) {
			LogDebug("%s", __FUNCTION__);
			goto err_exit;
		}
		tmp[i].vendor_data_size = LE_32(tmp[i].vendor_data_size);

		/* cache flags */
		if ((result = read_data(fd, &tmp[i].flags, sizeof(UINT16)))) {
			LogDebug("%s", __FUNCTION__);
			goto err_exit;
		}
		tmp[i].flags = LE_16(tmp[i].flags);

		/* fast forward over the pub key */
		offset = lseek(fd, tmp[i].pub_data_size, SEEK_CUR);
		if (offset == ((off_t)-1)) {
			LogDebug("lseek: %s", strerror(errno));
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto err_exit;
		}

		/* fast forward over the blob */
		offset = lseek(fd, tmp[i].blob_size, SEEK_CUR);
		if (offset == ((off_t)-1)) {
			LogDebug("lseek: %s", strerror(errno));
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto err_exit;
		}

		/* ignore vendor data for user ps */
	}

	*size = num_keys;
	*c = tmp;

	return TSS_SUCCESS;

err_exit:
	free(tmp);
	return result;
}

TSS_RESULT
copy_key_info(int fd, TSS_KM_KEYINFO *ki, struct key_disk_cache *c)
{
	TSS_KEY key;
	BYTE blob[4096];
	UINT64 offset;
	TSS_RESULT result;
	off_t off;

	/* Set the file pointer to the offset that the key blob is at */
	off = lseek(fd, TSSPS_BLOB_DATA_OFFSET(c), SEEK_SET);
	if (off == ((off_t)-1)) {
		LogDebug("lseek: %s", strerror(errno));
		return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	/* Read in the key blob */
	if ((result = read_data(fd, (void *)blob, c->blob_size))) {
		LogDebug("%s", __FUNCTION__);
		return result;
	}

	/* Expand the blob into a useable form */
	offset = 0;
	if ((result = UnloadBlob_TSS_KEY(&offset, blob, &key)))
		return result;

	if (key.hdr.key12.tag == TPM_TAG_KEY12) {
		ki->versionInfo.bMajor = TSS_SPEC_MAJOR;
		ki->versionInfo.bMinor = TSS_SPEC_MINOR;
		ki->versionInfo.bRevMajor = 0;
		ki->versionInfo.bRevMinor = 0;
	} else
		memcpy(&ki->versionInfo, &key.hdr.key11.ver, sizeof(TSS_VERSION));
	memcpy(&ki->keyUUID, &c->uuid, sizeof(TSS_UUID));
	memcpy(&ki->parentKeyUUID, &c->parent_uuid, sizeof(TSS_UUID));
	ki->bAuthDataUsage = key.authDataUsage;

	free_key_refs(&key);

	return TSS_SUCCESS;
}

TSS_RESULT
copy_key_info2(int fd, TSS_KM_KEYINFO2 *ki, struct key_disk_cache *c)
{
	TSS_KEY key;
	BYTE blob[4096];
	UINT64 offset;
	TSS_RESULT result;
	off_t off;

	/* Set the file pointer to the offset that the key blob is at */
	off = lseek(fd, TSSPS_BLOB_DATA_OFFSET(c), SEEK_SET);
	if (off == ((off_t)-1)) {
		LogDebug("lseek: %s", strerror(errno));
		return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	/* Read in the key blob */
	if ((result = read_data(fd, (void *)blob, c->blob_size))) {
		LogDebug("%s", __FUNCTION__);
		return result;
	}

	/* Expand the blob into a useable form */
	offset = 0;
	if ((result = UnloadBlob_TSS_KEY(&offset, blob, &key)))
		return result;

	if (key.hdr.key12.tag == TPM_TAG_KEY12) {
		ki->versionInfo.bMajor = TSS_SPEC_MAJOR;
		ki->versionInfo.bMinor = TSS_SPEC_MINOR;
		ki->versionInfo.bRevMajor = 0;
		ki->versionInfo.bRevMinor = 0;
	} else
		memcpy(&ki->versionInfo, &key.hdr.key11.ver, sizeof(TSS_VERSION));
	memcpy(&ki->keyUUID, &c->uuid, sizeof(TSS_UUID));
	memcpy(&ki->parentKeyUUID, &c->parent_uuid, sizeof(TSS_UUID));

	/* CHECK: fill the two new fields of TSS_KM_KEYINFO2 */
	ki->persistentStorageType = TSS_PS_TYPE_USER;
	ki->persistentStorageTypeParent = c->flags & CACHE_FLAG_PARENT_PS_SYSTEM ?
					  TSS_PS_TYPE_SYSTEM : TSS_PS_TYPE_USER;

	ki->bAuthDataUsage = key.authDataUsage;

	free_key_refs(&key);

	return TSS_SUCCESS;
}


TSS_RESULT
psfile_get_registered_keys(int fd,
			   TSS_UUID *uuid,
			   TSS_UUID *tcs_uuid,
			   UINT32 *size,
			   TSS_KM_KEYINFO **keys)
{
	TSS_RESULT result;
	struct key_disk_cache *cache_entries;
	UINT32 cache_size, i, j;
	TSS_KM_KEYINFO *keyinfos = NULL;
	TSS_UUID *find_uuid;

        if ((result = psfile_get_all_cache_entries(fd, &cache_size, &cache_entries)))
                return result;

	if (cache_size == 0) {
		if (uuid)
			return TSPERR(TSS_E_PS_KEY_NOTFOUND);
		else {
			*size = 0;
			*keys = NULL;
			return TSS_SUCCESS;
		}
	}

        if (uuid) {
		find_uuid = uuid;
		j = 0;

restart_search:
		/* Search for the requested UUID.  When found, allocate new space for it, copy
		 * it in, then change the uuid to be searched for it its parent and start over. */
		for (i = 0; i < cache_size; i++) {
			if (!memcmp(&cache_entries[i].uuid, find_uuid, sizeof(TSS_UUID))) {
				if (!(keyinfos = realloc(keyinfos,
							 (j+1) * sizeof(TSS_KM_KEYINFO)))) {
					free(cache_entries);
					free(keyinfos);
					return TSPERR(TSS_E_OUTOFMEMORY);
				}
				memset(&keyinfos[j], 0, sizeof(TSS_KM_KEYINFO));

				if ((result = copy_key_info(fd, &keyinfos[j], &cache_entries[i]))) {
					free(cache_entries);
					free(keyinfos);
					return result;
				}

				find_uuid = &keyinfos[j].parentKeyUUID;
				j++;
				goto restart_search;
			}
		}

		/* Searching for keys in the user PS will always lead us up to some key in the
		 * system PS. Return that key's uuid so that the upper layers can call down to TCS
		 * to search for it. */
		memcpy(tcs_uuid, find_uuid, sizeof(TSS_UUID));

		*size = j;
        } else {
		if ((keyinfos = calloc(cache_size, sizeof(TSS_KM_KEYINFO))) == NULL) {
			LogDebug("malloc of %zu bytes failed.",
				 cache_size * sizeof(TSS_KM_KEYINFO));
			free(cache_entries);
			return TSPERR(TSS_E_OUTOFMEMORY);
		}

                for (i = 0; i < cache_size; i++) {
			if ((result = copy_key_info(fd, &keyinfos[i], &cache_entries[i]))) {
				free(cache_entries);
				free(keyinfos);
				return result;
			}
                }

		*size = cache_size;
        }

	free(cache_entries);

	*keys = keyinfos;

	return TSS_SUCCESS;
}

TSS_RESULT
psfile_get_registered_keys2(int fd,
			   TSS_UUID *uuid,
			   TSS_UUID *tcs_uuid,
			   UINT32 *size,
			   TSS_KM_KEYINFO2 **keys)
{
	TSS_RESULT result;
	struct key_disk_cache *cache_entries;
	UINT32 cache_size, i, j;
	TSS_KM_KEYINFO2 *keyinfos = NULL;
	TSS_UUID *find_uuid;

        if ((result = psfile_get_all_cache_entries(fd, &cache_size, &cache_entries)))
                return result;

	if (cache_size == 0) {
		if (uuid)
			return TSPERR(TSS_E_PS_KEY_NOTFOUND);
		else {
			*size = 0;
			*keys = NULL;
			return TSS_SUCCESS;
		}
	}

	if (uuid) {
		find_uuid = uuid;
		j = 0;

		restart_search:
			/* Search for the requested UUID.  When found, allocate new space for it, copy
			 * it in, then change the uuid to be searched for it its parent and start over. */
			for (i = 0; i < cache_size; i++) {
				/*Return 0 if normal finish*/
				if (!memcmp(&cache_entries[i].uuid, find_uuid, sizeof(TSS_UUID))) {
					if (!(keyinfos = realloc(keyinfos,
							(j+1) * sizeof(TSS_KM_KEYINFO2)))) {
						free(cache_entries);
						free(keyinfos);
						return TSPERR(TSS_E_OUTOFMEMORY);
					}
					/* Here the key UUID is found and needs to be copied for the array*/
					/* Initializes the keyinfos with 0's*/
					memset(&keyinfos[j], 0, sizeof(TSS_KM_KEYINFO2));

					if ((result = copy_key_info2(fd, &keyinfos[j], &cache_entries[i]))) {
						free(cache_entries);
						free(keyinfos);
						return result;
					}

					find_uuid = &keyinfos[j].parentKeyUUID;
					j++;
					goto restart_search;
				}
			}

		/* Searching for keys in the user PS will always lead us up to some key in the
		 * system PS. Return that key's uuid so that the upper layers can call down to TCS
		 * to search for it. */
		memcpy(tcs_uuid, find_uuid, sizeof(TSS_UUID));

		*size = j;
	} else {
		if ((keyinfos = calloc(cache_size, sizeof(TSS_KM_KEYINFO2))) == NULL) {
			LogDebug("malloc of %zu bytes failed.",
					cache_size * sizeof(TSS_KM_KEYINFO2));
			free(cache_entries);
			return TSPERR(TSS_E_OUTOFMEMORY);
		}

		for (i = 0; i < cache_size; i++) {
			if ((result = copy_key_info2(fd, &keyinfos[i], &cache_entries[i]))) {
				free(cache_entries);
				free(keyinfos);
				return result;
			}
		}

		*size = cache_size;
	}

	free(cache_entries);

	*keys = keyinfos;

	return TSS_SUCCESS;
}

/*
 * read into the PS file and return the number of keys
 */
UINT32
psfile_get_num_keys(int fd)
{
	UINT32 num_keys;
	int rc;

	/* go to the number of keys */
	rc = lseek(fd, TSSPS_NUM_KEYS_OFFSET, SEEK_SET);
	if (rc == ((off_t)-1)) {
		LogDebug("lseek: %s", strerror(errno));
		return 0;
	}

	rc = read(fd, &num_keys, sizeof(UINT32));
	if (rc < 0) {
		LogDebug("read of %zd bytes: %s", sizeof(UINT32), strerror(errno));
		return 0;
	} else if ((unsigned)rc < sizeof(UINT32)) {
		num_keys = 0;
	}

	/* The system PS file is written in little-endian */
	num_keys = LE_32(num_keys);
	return num_keys;
}

/*
 * disk store format:
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
psfile_get_cache_entry_by_uuid(int fd, TSS_UUID *uuid, struct key_disk_cache *c)
{
	UINT32 i, num_keys = psfile_get_num_keys(fd);
	int offset;
	TSS_RESULT result;
	BYTE found = 0;

	if (num_keys == 0)
		return TSPERR(TSS_E_PS_KEY_NOTFOUND);

	/* make sure the file pointer is where we expect, just after the number
	 * of keys on disk at the head of the file
	 */
	offset = lseek(fd, TSSPS_KEYS_OFFSET, SEEK_SET);
	if (offset == ((off_t)-1)) {
		LogDebug("lseek: %s", strerror(errno));
		return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	for (i = 0; i < num_keys && !found; i++) {
		offset = lseek(fd, 0, SEEK_CUR);
		if (offset == ((off_t)-1)) {
			LogDebug("lseek: %s", strerror(errno));
			return TSPERR(TSS_E_INTERNAL_ERROR);
		}
		c->offset = offset;

		/* read UUID */
		if ((result = read_data(fd, (void *)&c->uuid, sizeof(TSS_UUID)))) {
			LogDebug("%s", __FUNCTION__);
			return result;
		}

		if (!memcmp(&c->uuid, uuid, sizeof(TSS_UUID))) {
			found = 1;

			/* read parent UUID */
			if ((result = read_data(fd, (void *)&c->parent_uuid, sizeof(TSS_UUID)))) {
				LogDebug("%s", __FUNCTION__);
				return result;
			}
		} else {
			/* fast forward over the parent UUID */
			offset = lseek(fd, sizeof(TSS_UUID), SEEK_CUR);
			if (offset == ((off_t)-1)) {
				LogDebug("lseek: %s", strerror(errno));
				return TSPERR(TSS_E_INTERNAL_ERROR);
			}
		}

		/* pub data size */
		if ((result = read_data(fd, &c->pub_data_size, sizeof(UINT16)))) {
			LogDebug("%s", __FUNCTION__);
			return result;
		}
		c->pub_data_size = LE_16(c->pub_data_size);
		DBG_ASSERT(c->pub_data_size <= 2048 && c->pub_data_size > 0);

		/* blob size */
		if ((result = read_data(fd, &c->blob_size, sizeof(UINT16)))) {
			LogDebug("%s", __FUNCTION__);
			return result;
		}
		c->blob_size = LE_16(c->blob_size); 
		DBG_ASSERT(c->blob_size <= 4096 && c->blob_size > 0);

		/* vendor data size */
		if ((result = read_data(fd, &c->vendor_data_size, sizeof(UINT32)))) {
			LogDebug("%s", __FUNCTION__);
			return result;
		}
		c->vendor_data_size = LE_32(c->vendor_data_size); 

		/* cache flags */
		if ((result = read_data(fd, &c->flags, sizeof(UINT16)))) {
			LogDebug("%s", __FUNCTION__);
			return result;
		}
		c->flags = LE_16(c->flags); 

		/* fast forward over the pub key */
		offset = lseek(fd, c->pub_data_size, SEEK_CUR);
		if (offset == ((off_t)-1)) {
			LogDebug("lseek: %s", strerror(errno));
			return TSPERR(TSS_E_INTERNAL_ERROR);
		}

		/* fast forward over the blob */
		offset = lseek(fd, c->blob_size, SEEK_CUR);
		if (offset == ((off_t)-1)) {
			LogDebug("lseek: %s", strerror(errno));
			return TSPERR(TSS_E_INTERNAL_ERROR);
		}

		/* ignore vendor data in user ps */
	}

	return found ? TSS_SUCCESS : TSPERR(TSS_E_PS_KEY_NOTFOUND);
}

TSS_RESULT
psfile_get_cache_entry_by_pub(int fd, UINT32 pub_size, BYTE *pub, struct key_disk_cache *c)
{
	BYTE blob[2048];
	UINT32 i, num_keys = psfile_get_num_keys(fd);
	int offset;
	TSS_RESULT result;

	if (num_keys == 0)
		return TSPERR(TSS_E_PS_KEY_NOTFOUND);

	/* make sure the file pointer is where we expect, just after the number
	 * of keys on disk at the head of the file
	 */
	offset = lseek(fd, TSSPS_KEYS_OFFSET, SEEK_SET);
	if (offset == ((off_t)-1)) {
		LogDebug("lseek: %s", strerror(errno));
		return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	for (i = 0; i < num_keys; i++) {
		offset = lseek(fd, 0, SEEK_CUR);
		if (offset == ((off_t)-1)) {
			LogDebug("lseek: %s", strerror(errno));
			return TSPERR(TSS_E_INTERNAL_ERROR);
		}
		c->offset = offset;

		/* read UUID */
		if ((result = read_data(fd, (void *)&c->uuid, sizeof(TSS_UUID)))) {
			LogDebug("%s", __FUNCTION__);
			return result;
		}

		/* read parent UUID */
		if ((result = read_data(fd, (void *)&c->parent_uuid, sizeof(TSS_UUID)))) {
			LogDebug("%s", __FUNCTION__);
			return result;
		}

		/* pub data size */
		if ((result = read_data(fd, &c->pub_data_size, sizeof(UINT16)))) {
			LogDebug("%s", __FUNCTION__);
			return result;
		}

		c->pub_data_size = LE_16(c->pub_data_size);
		DBG_ASSERT(c->pub_data_size <= 2048 && c->pub_data_size > 0);

		/* blob size */
		if ((result = read_data(fd, &c->blob_size, sizeof(UINT16)))) {
			LogDebug("%s", __FUNCTION__);
			return result;
		}

		c->blob_size = LE_16(c->blob_size);
		DBG_ASSERT(c->blob_size <= 4096 && c->blob_size > 0);

		/* vendor data size */
		if ((result = read_data(fd, &c->vendor_data_size, sizeof(UINT32)))) {
			LogDebug("%s", __FUNCTION__);
			return result;
		}
		c->vendor_data_size = LE_32(c->vendor_data_size);

		/* cache flags */
		if ((result = read_data(fd, &c->flags, sizeof(UINT16)))) {
			LogDebug("%s", __FUNCTION__);
			return result;
		}
		c->flags = LE_16(c->flags);

		if (c->pub_data_size == pub_size) {
			/* read in the pub key */
			if ((result = read_data(fd, blob, c->pub_data_size))) {
				LogDebug("%s", __FUNCTION__);
				return result;
			}

			if (!memcmp(blob, pub, pub_size))
				break;
		}

		/* fast forward over the blob */
		offset = lseek(fd, c->blob_size, SEEK_CUR);
		if (offset == ((off_t)-1)) {
			LogDebug("lseek: %s", strerror(errno));
			return TSPERR(TSS_E_INTERNAL_ERROR);
		}

		/* ignore vendor data */
	}

	return TSS_SUCCESS;
}
