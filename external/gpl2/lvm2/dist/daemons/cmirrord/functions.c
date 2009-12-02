/*	$NetBSD: functions.c,v 1.1.1.1 2009/12/02 00:27:10 haad Exp $	*/

/*
 * Copyright (C) 2004-2009 Red Hat, Inc. All rights reserved.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License v.2.1.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64

#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <signal.h>
#include <linux/kdev_t.h>
//#define __USE_GNU /* for O_DIRECT */
#include <fcntl.h>
#include <time.h>
#include "libdevmapper.h"
#include "dm-log-userspace.h"
#include "functions.h"
#include "common.h"
#include "cluster.h"
#include "logging.h"

#define BYTE_SHIFT 3

/*
 * Magic for persistent mirrors: "MiRr"
 * Following on-disk header information is stolen from
 * drivers/md/dm-log.c
 */
#define MIRROR_MAGIC 0x4D695272
#define MIRROR_DISK_VERSION 2
#define LOG_OFFSET 2

#define RESYNC_HISTORY 50
//static char resync_history[RESYNC_HISTORY][128];
//static int idx = 0;
#define LOG_SPRINT(_lc, f, arg...) do {					\
		lc->idx++;						\
		lc->idx = lc->idx % RESYNC_HISTORY;			\
		sprintf(lc->resync_history[lc->idx], f, ## arg);	\
	} while (0)

struct log_header {
        uint32_t magic;
        uint32_t version;
        uint64_t nr_regions;
};

struct log_c {
	struct dm_list list;

	char uuid[DM_UUID_LEN];
	uint64_t luid;

	time_t delay; /* limits how fast a resume can happen after suspend */
	int touched;
	uint32_t region_size;
	uint32_t region_count;
	uint64_t sync_count;

	dm_bitset_t clean_bits;
	dm_bitset_t sync_bits;
	uint32_t recoverer;
	uint64_t recovering_region; /* -1 means not recovering */
	uint64_t skip_bit_warning; /* used to warn if region skipped */
	int sync_search;

	int resume_override;

	uint32_t block_on_error;
        enum sync {
                DEFAULTSYNC,    /* Synchronize if necessary */
                NOSYNC,         /* Devices known to be already in sync */
                FORCESYNC,      /* Force a sync to happen */
        } sync;

	uint32_t state;         /* current operational state of the log */

	struct dm_list mark_list;

	uint32_t recovery_halted;
	struct recovery_request *recovery_request_list;

	int disk_fd;            /* -1 means no disk log */
	int log_dev_failed;
	uint64_t disk_nr_regions;
	size_t disk_size;       /* size of disk_buffer in bytes */
	void *disk_buffer;      /* aligned memory for O_DIRECT */
	int idx;
	char resync_history[RESYNC_HISTORY][128];
};

struct mark_entry {
	struct dm_list list;
	uint32_t nodeid;
	uint64_t region;
};

struct recovery_request {
	uint64_t region;
	struct recovery_request *next;
};

static DM_LIST_INIT(log_list);
static DM_LIST_INIT(log_pending_list);

static int log_test_bit(dm_bitset_t bs, int bit)
{
	return dm_bit(bs, bit);
}

static void log_set_bit(struct log_c *lc, dm_bitset_t bs, int bit)
{
	dm_bit_set(bs, bit);
	lc->touched = 1;
}

static void log_clear_bit(struct log_c *lc, dm_bitset_t bs, int bit)
{
	dm_bit_clear(bs, bit);
	lc->touched = 1;
}

static int find_next_zero_bit(dm_bitset_t bs, int start)
{
	while (dm_bit(bs, start++))
		if (start >= (int)bs[0])
			return -1;

	return start - 1;
}

static uint64_t count_bits32(dm_bitset_t bs)
{
	int i, size = ((int)bs[0]/DM_BITS_PER_INT + 1);
	unsigned count = 0;

	for (i = 1; i <= size; i++)
		count += hweight32(bs[i]);

	return (uint64_t)count;
}

/*
 * get_log
 *
 * Returns: log if found, NULL otherwise
 */
static struct log_c *get_log(const char *uuid, uint64_t luid)
{
	struct log_c *lc;

	dm_list_iterate_items(lc, &log_list)
		if (!strcmp(lc->uuid, uuid) &&
		    (!luid || (luid == lc->luid)))
			return lc;

	return NULL;
}

/*
 * get_pending_log
 *
 * Pending logs are logs that have been 'clog_ctr'ed, but
 * have not joined the CPG (via clog_resume).
 *
 * Returns: log if found, NULL otherwise
 */
static struct log_c *get_pending_log(const char *uuid, uint64_t luid)
{
	struct log_c *lc;

	dm_list_iterate_items(lc, &log_pending_list)
		if (!strcmp(lc->uuid, uuid) &&
		    (!luid || (luid == lc->luid)))
			return lc;

	return NULL;
}

static void header_to_disk(struct log_header *mem, struct log_header *disk)
{
	memcpy(disk, mem, sizeof(struct log_header));
}

static void header_from_disk(struct log_header *mem, struct log_header *disk)
{
	memcpy(mem, disk, sizeof(struct log_header));
}

static int rw_log(struct log_c *lc, int do_write)
{
	int r;

	r = lseek(lc->disk_fd, 0, SEEK_SET);
	if (r < 0) {
		LOG_ERROR("[%s] rw_log:  lseek failure: %s",
			  SHORT_UUID(lc->uuid), strerror(errno));
		return -errno;
	}

	if (do_write) {
		r = write(lc->disk_fd, lc->disk_buffer, lc->disk_size);
		if (r < 0) {
			LOG_ERROR("[%s] rw_log:  write failure: %s",
				  SHORT_UUID(lc->uuid), strerror(errno));
			return -EIO; /* Failed disk write */
		}
		return 0;
	}

	/* Read */
	r = read(lc->disk_fd, lc->disk_buffer, lc->disk_size);
	if (r < 0)
		LOG_ERROR("[%s] rw_log:  read failure: %s",
			  SHORT_UUID(lc->uuid), strerror(errno));
	if (r != lc->disk_size)
		return -EIO; /* Failed disk read */
	return 0;
}

/*
 * read_log
 * @lc
 *
 * Valid return codes:
 *   -EINVAL:  Invalid header, bits not copied
 *   -EIO:     Unable to read disk log
 *    0:       Valid header, disk bit -> lc->clean_bits
 *
 * Returns: 0 on success, -EXXX on failure
 */
static int read_log(struct log_c *lc)
{
	struct log_header lh;
	size_t bitset_size;

	memset(&lh, 0, sizeof(struct log_header));

	if (rw_log(lc, 0))
		return -EIO; /* Failed disk read */

	header_from_disk(&lh, lc->disk_buffer);
	if (lh.magic != MIRROR_MAGIC)
		return -EINVAL;

	lc->disk_nr_regions = lh.nr_regions;

	/* Read disk bits into sync_bits */
	bitset_size = lc->region_count / 8;
	bitset_size += (lc->region_count % 8) ? 1 : 0;
	memcpy(lc->clean_bits, lc->disk_buffer + 1024, bitset_size);

	return 0;
}

/*
 * write_log
 * @lc
 *
 * Returns: 0 on success, -EIO on failure
 */
static int write_log(struct log_c *lc)
{
	struct log_header lh;
	size_t bitset_size;

	lh.magic = MIRROR_MAGIC;
	lh.version = MIRROR_DISK_VERSION;
	lh.nr_regions = lc->region_count;

	header_to_disk(&lh, lc->disk_buffer);

	/* Write disk bits from clean_bits */
	bitset_size = lc->region_count / 8;
	bitset_size += (lc->region_count % 8) ? 1 : 0;
	memcpy(lc->disk_buffer + 1024, lc->clean_bits, bitset_size);

	if (rw_log(lc, 1)) {
		lc->log_dev_failed = 1;
		return -EIO; /* Failed disk write */
	}
	return 0;
}

static int find_disk_path(char *major_minor_str, char *path_rtn, int *unlink_path)
{
	int r;
	DIR *dp;
	struct dirent *dep;
	struct stat statbuf;
	int major, minor;

	if (!strstr(major_minor_str, ":")) {
		r = stat(major_minor_str, &statbuf);
		if (r)
			return -errno;
		if (!S_ISBLK(statbuf.st_mode))
			return -EINVAL;
		sprintf(path_rtn, "%s", major_minor_str);
		return 0;
	}

	r = sscanf(major_minor_str, "%d:%d", &major, &minor);
	if (r != 2)
		return -EINVAL;

	LOG_DBG("Checking /dev/mapper for device %d:%d", major, minor);
	/* Check /dev/mapper dir */
	dp = opendir("/dev/mapper");
	if (!dp)
		return -ENOENT;

	while ((dep = readdir(dp)) != NULL) {
		/*
		 * FIXME: This is racy.  By the time the path is used,
		 * it may point to something else.  'fstat' will be
		 * required upon opening to ensure we got what we
		 * wanted.
		 */

		sprintf(path_rtn, "/dev/mapper/%s", dep->d_name);
		stat(path_rtn, &statbuf);
		if (S_ISBLK(statbuf.st_mode) &&
		    (major(statbuf.st_rdev) == major) &&
		    (minor(statbuf.st_rdev) == minor)) {
			LOG_DBG("  %s: YES", dep->d_name);
			closedir(dp);
			return 0;
		} else {
			LOG_DBG("  %s: NO", dep->d_name);
		}
	}

	closedir(dp);

	LOG_DBG("Path not found for %d/%d", major, minor);
	LOG_DBG("Creating /dev/mapper/%d-%d", major, minor);
	sprintf(path_rtn, "/dev/mapper/%d-%d", major, minor);
	r = mknod(path_rtn, S_IFBLK | S_IRUSR | S_IWUSR, MKDEV(major, minor));

	/*
	 * If we have to make the path, we unlink it after we open it
	 */
	*unlink_path = 1;

	return r ? -errno : 0;
}

static int _clog_ctr(char *uuid, uint64_t luid,
		     int argc, char **argv, uint64_t device_size)
{
	int i;
	int r = 0;
	char *p;
	uint64_t region_size;
	uint64_t region_count;
	struct log_c *lc = NULL;
	struct log_c *duplicate;
	enum sync sync = DEFAULTSYNC;
	uint32_t block_on_error = 0;

	int disk_log = 0;
	char disk_path[128];
	int unlink_path = 0;
	size_t page_size;
	int pages;

	/* If core log request, then argv[0] will be region_size */
	if (!strtoll(argv[0], &p, 0) || *p) {
		disk_log = 1;

		if ((argc < 2) || (argc > 4)) {
			LOG_ERROR("Too %s arguments to clustered_disk log type",
				  (argc < 3) ? "few" : "many");
			r = -EINVAL;
			goto fail;
		}

		r = find_disk_path(argv[0], disk_path, &unlink_path);
		if (r) {
			LOG_ERROR("Unable to find path to device %s", argv[0]);
			goto fail;
		}
		LOG_DBG("Clustered log disk is %s", disk_path);
	} else {
		disk_log = 0;

		if ((argc < 1) || (argc > 3)) {
			LOG_ERROR("Too %s arguments to clustered_core log type",
				  (argc < 2) ? "few" : "many");
			r = -EINVAL;
			goto fail;
		}
	}

	if (!(region_size = strtoll(argv[disk_log], &p, 0)) || *p) {
		LOG_ERROR("Invalid region_size argument to clustered_%s log type",
			  (disk_log) ? "disk" : "core");
		r = -EINVAL;
		goto fail;
	}

	region_count = device_size / region_size;
	if (device_size % region_size) {
		/*
		 * I can't remember if device_size must be a multiple
		 * of region_size, so check it anyway.
		 */
		region_count++;
	}

	for (i = 0; i < argc; i++) {
		if (!strcmp(argv[i], "sync"))
			sync = FORCESYNC;
		else if (!strcmp(argv[i], "nosync"))
			sync = NOSYNC;
		else if (!strcmp(argv[i], "block_on_error"))
			block_on_error = 1;
	}

	lc = malloc(sizeof(*lc));
	if (!lc) {
		LOG_ERROR("Unable to allocate cluster log context");
		r = -ENOMEM;
		goto fail;
	}
	memset(lc, 0, sizeof(*lc));

	lc->region_size = region_size;
	lc->region_count = region_count;
	lc->sync = sync;
	lc->block_on_error = block_on_error;
	lc->sync_search = 0;
	lc->recovering_region = (uint64_t)-1;
	lc->skip_bit_warning = region_count;
	lc->disk_fd = -1;
	lc->log_dev_failed = 0;
	strncpy(lc->uuid, uuid, DM_UUID_LEN);
	lc->luid = luid;

	if ((duplicate = get_log(lc->uuid, lc->luid)) ||
	    (duplicate = get_pending_log(lc->uuid, lc->luid))) {
		LOG_ERROR("[%s/%llu] Log already exists, unable to create.",
			  SHORT_UUID(lc->uuid), lc->luid);
		free(lc);
		return -EINVAL;
	}

	dm_list_init(&lc->mark_list);

	lc->clean_bits = dm_bitset_create(NULL, region_count);
	if (!lc->clean_bits) {
		LOG_ERROR("Unable to allocate clean bitset");
		r = -ENOMEM;
		goto fail;
	}

	lc->sync_bits = dm_bitset_create(NULL, region_count);
	if (!lc->sync_bits) {
		LOG_ERROR("Unable to allocate sync bitset");
		r = -ENOMEM;
		goto fail;
	}
	if (sync == NOSYNC)
		dm_bit_set_all(lc->sync_bits);

	lc->sync_count = (sync == NOSYNC) ? region_count : 0;
	if (disk_log) {
		page_size = sysconf(_SC_PAGESIZE);
		pages = ((int)lc->clean_bits[0])/page_size;
		pages += ((int)lc->clean_bits[0])%page_size ? 1 : 0;
		pages += 1; /* for header */

		r = open(disk_path, O_RDWR | O_DIRECT);
		if (r < 0) {
			LOG_ERROR("Unable to open log device, %s: %s",
				  disk_path, strerror(errno));
			r = errno;
			goto fail;
		}
		if (unlink_path)
			unlink(disk_path);

		lc->disk_fd = r;
		lc->disk_size = pages * page_size;

		r = posix_memalign(&(lc->disk_buffer), page_size,
				   lc->disk_size);
		if (r) {
			LOG_ERROR("Unable to allocate memory for disk_buffer");
			goto fail;
		}
		memset(lc->disk_buffer, 0, lc->disk_size);
		LOG_DBG("Disk log ready");
	}

	dm_list_add(&log_pending_list, &lc->list);

	return 0;
fail:
	if (lc) {
		if (lc->clean_bits)
			free(lc->clean_bits);
		if (lc->sync_bits)
			free(lc->sync_bits);
		if (lc->disk_buffer)
			free(lc->disk_buffer);
		if (lc->disk_fd >= 0)
			close(lc->disk_fd);
		free(lc);
	}
	return r;
}

/*
 * clog_ctr
 * @rq
 *
 * rq->data should contain constructor string as follows:
 *	<log_type> [disk] <region_size> [[no]sync] <device_len>
 * The kernel is responsible for adding the <dev_len> argument
 * to the end; otherwise, we cannot compute the region_count.
 *
 * FIXME: Currently relies on caller to fill in rq->error
 */
static int clog_dtr(struct dm_ulog_request *rq);
static int clog_ctr(struct dm_ulog_request *rq)
{
	int argc, i, r = 0;
	char *p, **argv = NULL;
	char *dev_size_str;
	uint64_t device_size;

	/* Sanity checks */
	if (!rq->data_size) {
		LOG_ERROR("Received constructor request with no data");
		return -EINVAL;
	}

	if (strlen(rq->data) > rq->data_size) {
		LOG_ERROR("Received constructor request with bad data");
		LOG_ERROR("strlen(rq->data)[%d] != rq->data_size[%llu]",
			  (int)strlen(rq->data),
			  (unsigned long long)rq->data_size);
		LOG_ERROR("rq->data = '%s' [%d]",
			  rq->data, (int)strlen(rq->data));
		return -EINVAL;
	}

	/* Split up args */
	for (argc = 0, p = rq->data; (p = strstr(p, " ")); p++, argc++)
		*p = '\0';

	argv = malloc(argc * sizeof(char *));
	if (!argv)
		return -ENOMEM;

	p = dev_size_str = rq->data;
	p += strlen(p) + 1;
	for (i = 0; i < argc; i++, p = p + strlen(p) + 1)
		argv[i] = p;

	if (strcmp(argv[0], "clustered_disk") &&
	    strcmp(argv[0], "clustered_core")) {
		LOG_ERROR("Unsupported userspace log type, \"%s\"", argv[0]);
		free(argv);
		return -EINVAL;
	}

	if (!(device_size = strtoll(dev_size_str, &p, 0)) || *p) {
		LOG_ERROR("Invalid device size argument: %s", dev_size_str);
		free(argv);
		return -EINVAL;
	}

	r = _clog_ctr(rq->uuid, rq->luid, argc - 1, argv + 1, device_size);

	/* We join the CPG when we resume */

	/* No returning data */
	rq->data_size = 0;

	if (r) {
		LOG_ERROR("Failed to create cluster log (%s)", rq->uuid);
		for (i = 0; i < argc; i++)
			LOG_ERROR("argv[%d] = %s", i, argv[i]);
	}
	else
		LOG_DBG("[%s] Cluster log created",
			SHORT_UUID(rq->uuid));

	free(argv);
	return r;
}

/*
 * clog_dtr
 * @rq
 *
 */
static int clog_dtr(struct dm_ulog_request *rq)
{
	struct log_c *lc = get_log(rq->uuid, rq->luid);

	if (lc) {
		/*
		 * The log should not be on the official list.  There
		 * should have been a suspend first.
		 */
		LOG_ERROR("[%s] DTR before SUS: leaving CPG",
			  SHORT_UUID(rq->uuid));
		destroy_cluster_cpg(rq->uuid);
	} else if (!(lc = get_pending_log(rq->uuid, rq->luid))) {
		LOG_ERROR("clog_dtr called on log that is not official or pending");
		return -EINVAL;
	}

	LOG_DBG("[%s] Cluster log removed", SHORT_UUID(lc->uuid));

	dm_list_del(&lc->list);
	if (lc->disk_fd != -1)
		close(lc->disk_fd);
	if (lc->disk_buffer)
		free(lc->disk_buffer);
	free(lc->clean_bits);
	free(lc->sync_bits);
	free(lc);

	return 0;
}

/*
 * clog_presuspend
 * @rq
 *
 */
static int clog_presuspend(struct dm_ulog_request *rq)
{
	struct log_c *lc = get_log(rq->uuid, rq->luid);

	if (!lc)
		return -EINVAL;

	if (lc->touched)
		LOG_DBG("WARNING: log still marked as 'touched' during suspend");

	lc->recovery_halted = 1;

	return 0;
}

/*
 * clog_postsuspend
 * @rq
 *
 */
static int clog_postsuspend(struct dm_ulog_request *rq)
{
	struct log_c *lc = get_log(rq->uuid, rq->luid);

	if (!lc)
		return -EINVAL;

	LOG_DBG("[%s] clog_postsuspend: leaving CPG", SHORT_UUID(lc->uuid));
	destroy_cluster_cpg(rq->uuid);

	lc->state = LOG_SUSPENDED;
	lc->recovering_region = (uint64_t)-1;
	lc->recoverer = (uint32_t)-1;
	lc->delay = time(NULL);

	return 0;
}

/*
 * cluster_postsuspend
 * @rq
 *
 */
int cluster_postsuspend(char *uuid, uint64_t luid)
{
	struct log_c *lc = get_log(uuid, luid);

	if (!lc)
		return -EINVAL;

	LOG_DBG("[%s] clog_postsuspend: finalizing", SHORT_UUID(lc->uuid));
	lc->resume_override = 0;

	/* move log to pending list */
	dm_list_del(&lc->list);
	dm_list_add(&log_pending_list, &lc->list);

	return 0;
}

/*
 * clog_resume
 * @rq
 *
 * Does the main work of resuming.
 */
static int clog_resume(struct dm_ulog_request *rq)
{
	uint32_t i;
	int commit_log = 0;
	struct log_c *lc = get_log(rq->uuid, rq->luid);

	if (!lc)
		return -EINVAL;

	switch (lc->resume_override) {
	case 1000:
		LOG_ERROR("[%s] Additional resume issued before suspend",
			  SHORT_UUID(rq->uuid));
#ifdef DEBUG
		kill(getpid(), SIGUSR1);
#endif
		return 0;
	case 0:
		lc->resume_override = 1000;
		if (lc->disk_fd == -1) {
			LOG_DBG("[%s] Master resume.",
				SHORT_UUID(lc->uuid));
			goto no_disk;
		}

		LOG_DBG("[%s] Master resume: reading disk log",
			SHORT_UUID(lc->uuid));
		commit_log = 1;
		break;
	case 1:
		LOG_ERROR("Error:: partial bit loading (just sync_bits)");
		return -EINVAL;
	case 2:
		LOG_ERROR("Error:: partial bit loading (just clean_bits)");
		return -EINVAL;
	case 3:
		LOG_DBG("[%s] Non-master resume: bits pre-loaded",
			SHORT_UUID(lc->uuid));
		lc->resume_override = 1000;
		goto out;
	default:
		LOG_ERROR("Error:: multiple loading of bits (%d)",
			  lc->resume_override);
		return -EINVAL;
	}

	if (lc->log_dev_failed) {
		LOG_ERROR("Log device has failed, unable to read bits");
		rq->error = 0;  /* We can handle this so far */
		lc->disk_nr_regions = 0;
	} else
		rq->error = read_log(lc);

	switch (rq->error) {
	case 0:
		if (lc->disk_nr_regions < lc->region_count)
			LOG_DBG("[%s] Mirror has grown, updating log bits",
				SHORT_UUID(lc->uuid));
		else if (lc->disk_nr_regions > lc->region_count)
			LOG_DBG("[%s] Mirror has shrunk, updating log bits",
				SHORT_UUID(lc->uuid));
		break;		
	case -EINVAL:
		LOG_DBG("[%s] (Re)initializing mirror log - resync issued.",
			SHORT_UUID(lc->uuid));
		lc->disk_nr_regions = 0;
		break;
	default:
		LOG_ERROR("Failed to read disk log");
		lc->disk_nr_regions = 0;
		break;
	}

no_disk:
	/* If mirror has grown, set bits appropriately */
	if (lc->sync == NOSYNC)
		for (i = lc->disk_nr_regions; i < lc->region_count; i++)
			log_set_bit(lc, lc->clean_bits, i);
	else
		for (i = lc->disk_nr_regions; i < lc->region_count; i++)
			log_clear_bit(lc, lc->clean_bits, i);

	/* Clear any old bits if device has shrunk */
	for (i = lc->region_count; i % 32; i++)
		log_clear_bit(lc, lc->clean_bits, i);

	/* copy clean across to sync */
	dm_bit_copy(lc->sync_bits, lc->clean_bits);

	if (commit_log && (lc->disk_fd >= 0)) {
		rq->error = write_log(lc);
		if (rq->error)
			LOG_ERROR("Failed initial disk log write");
		else
			LOG_DBG("Disk log initialized");
		lc->touched = 0;
	}
out:
	/*
	 * Clear any old bits if device has shrunk - necessary
	 * for non-master resume
	 */
	for (i = lc->region_count; i % 32; i++) {
		log_clear_bit(lc, lc->clean_bits, i);
		log_clear_bit(lc, lc->sync_bits, i);
	}

	lc->sync_count = count_bits32(lc->sync_bits);

	LOG_SPRINT(lc, "[%s] Initial sync_count = %llu",
		   SHORT_UUID(lc->uuid), (unsigned long long)lc->sync_count);
	lc->sync_search = 0;
	lc->state = LOG_RESUMED;
	lc->recovery_halted = 0;
	
	return rq->error;
}

/*
 * local_resume
 * @rq
 *
 * If the log is pending, we must first join the cpg and
 * put the log in the official list.
 *
 */
int local_resume(struct dm_ulog_request *rq)
{
	int r;
	time_t t;
	struct log_c *lc = get_log(rq->uuid, rq->luid);

	if (!lc) {
		/* Is the log in the pending list? */
		lc = get_pending_log(rq->uuid, rq->luid);
		if (!lc) {
			LOG_ERROR("clog_resume called on log that is not official or pending");
			return -EINVAL;
		}

		t = time(NULL);
		t -= lc->delay;
		/*
		 * This should be considered a temporary fix.  It addresses
		 * a problem that exists when nodes suspend/resume in rapid
		 * succession.  While the problem is very rare, it has been
		 * seen to happen in real-world-like testing.
		 *
		 * The problem:
		 * - Node A joins cluster
		 * - Node B joins cluster
		 * - Node A prepares checkpoint
		 * - Node A gets ready to write checkpoint
		 * - Node B leaves
		 * - Node B joins
		 * - Node A finishes write of checkpoint
		 * - Node B receives checkpoint meant for previous session
		 * -- Node B can now be non-coherent
		 *
		 * This timer will solve the problem for now, but could be
		 * replaced by a generation number sent with the resume
		 * command from the kernel.  The generation number would
		 * be included in the name of the checkpoint to prevent
		 * reading stale data.
		 */
		if ((t < 3) && (t >= 0))
			sleep(3 - t);

		/* Join the CPG */
		r = create_cluster_cpg(rq->uuid, rq->luid);
		if (r) {
			LOG_ERROR("clog_resume:  Failed to create cluster CPG");
			return r;
		}

		/* move log to official list */
		dm_list_del(&lc->list);
		dm_list_add(&log_list, &lc->list);
	}

	return 0;
}

/*
 * clog_get_region_size
 * @rq
 *
 * Since this value doesn't change, the kernel
 * should not need to talk to server to get this
 * The function is here for completness
 *
 * Returns: 0 on success, -EXXX on failure
 */
static int clog_get_region_size(struct dm_ulog_request *rq)
{
	uint64_t *rtn = (uint64_t *)rq->data;
	struct log_c *lc = get_log(rq->uuid, rq->luid);

	if (!lc && !(lc = get_pending_log(rq->uuid, rq->luid)))
		return -EINVAL;

	*rtn = lc->region_size;
	rq->data_size = sizeof(*rtn);

	return 0;
}

/*
 * clog_is_clean
 * @rq
 *
 * Returns: 1 if clean, 0 otherwise
 */
static int clog_is_clean(struct dm_ulog_request *rq)
{
	int64_t *rtn = (int64_t *)rq->data;
	uint64_t region = *((uint64_t *)(rq->data));
	struct log_c *lc = get_log(rq->uuid, rq->luid);

	if (!lc)
		return -EINVAL;

	*rtn = log_test_bit(lc->clean_bits, region);
	rq->data_size = sizeof(*rtn);

	return 0;
}

/*
 * clog_in_sync
 * @rq
 *
 * We ignore any request for non-block.  That
 * should be handled elsewhere.  (If the request
 * has come this far, it has already blocked.)
 *
 * Returns: 1 if in-sync, 0 otherwise
 */
static int clog_in_sync(struct dm_ulog_request *rq)
{
	int64_t *rtn = (int64_t *)rq->data;
	uint64_t region = *((uint64_t *)(rq->data));
	struct log_c *lc = get_log(rq->uuid, rq->luid);

	if (!lc)
		return -EINVAL;

	if (region > lc->region_count)
		return -EINVAL;

	*rtn = log_test_bit(lc->sync_bits, region);
	if (*rtn)
		LOG_DBG("[%s] Region is in-sync: %llu",
			SHORT_UUID(lc->uuid), (unsigned long long)region);
	else
		LOG_DBG("[%s] Region is not in-sync: %llu",
			SHORT_UUID(lc->uuid), (unsigned long long)region);

	rq->data_size = sizeof(*rtn);

	return 0;
}

/*
 * clog_flush
 * @rq
 *
 */
static int clog_flush(struct dm_ulog_request *rq, int server)
{
	int r = 0;
	struct log_c *lc = get_log(rq->uuid, rq->luid);
	
	if (!lc)
		return -EINVAL;

	if (!lc->touched)
		return 0;

	/*
	 * Do the actual flushing of the log only
	 * if we are the server.
	 */
	if (server && (lc->disk_fd >= 0)) {
		r = rq->error = write_log(lc);
		if (r)
			LOG_ERROR("[%s] Error writing to disk log",
				  SHORT_UUID(lc->uuid));
		else 
			LOG_DBG("[%s] Disk log written", SHORT_UUID(lc->uuid));
	}

	lc->touched = 0;

	return r;

}

/*
 * mark_region
 * @lc
 * @region
 * @who
 *
 * Put a mark region request in the tree for tracking.
 *
 * Returns: 0 on success, -EXXX on error
 */
static int mark_region(struct log_c *lc, uint64_t region, uint32_t who)
{
	int found = 0;
	struct mark_entry *m;

	dm_list_iterate_items(m, &lc->mark_list)
		if (m->region == region) {
			found = 1;
			if (m->nodeid == who)
				return 0;
		}

	if (!found)
		log_clear_bit(lc, lc->clean_bits, region);

	/*
	 * Save allocation until here - if there is a failure,
	 * at least we have cleared the bit.
	 */
	m = malloc(sizeof(*m));
	if (!m) {
		LOG_ERROR("Unable to allocate space for mark_entry: %llu/%u",
			  (unsigned long long)region, who);
		return -ENOMEM;
	}

	m->nodeid = who;
	m->region = region;
	dm_list_add(&lc->mark_list, &m->list);

	return 0;
}

/*
 * clog_mark_region
 * @rq
 *
 * rq may contain more than one mark request.  We
 * can determine the number from the 'data_size' field.
 *
 * Returns: 0 on success, -EXXX on failure
 */
static int clog_mark_region(struct dm_ulog_request *rq, uint32_t originator)
{
	int r;
	int count;
	uint64_t *region;
	struct log_c *lc = get_log(rq->uuid, rq->luid);

	if (!lc)
		return -EINVAL;

	if (rq->data_size % sizeof(uint64_t)) {
		LOG_ERROR("Bad data size given for mark_region request");
		return -EINVAL;
	}

	count = rq->data_size / sizeof(uint64_t);
	region = (uint64_t *)&rq->data;

	for (; count > 0; count--, region++) {
		r = mark_region(lc, *region, originator);
		if (r)
			return r;
	}

	rq->data_size = 0;

	return 0;
}

static int clear_region(struct log_c *lc, uint64_t region, uint32_t who)
{
	int other_matches = 0;
	struct mark_entry *m, *n;

	dm_list_iterate_items_safe(m, n, &lc->mark_list)
		if (m->region == region) {
			if (m->nodeid == who) {
				dm_list_del(&m->list);
				free(m);
			} else
				other_matches = 1;
		}

	/*
	 * Clear region if:
	 *  1) It is in-sync
	 *  2) There are no other machines that have it marked
	 */
	if (!other_matches && log_test_bit(lc->sync_bits, region))
		log_set_bit(lc, lc->clean_bits, region);

	return 0;
}

/*
 * clog_clear_region
 * @rq
 *
 * rq may contain more than one clear request.  We
 * can determine the number from the 'data_size' field.
 *
 * Returns: 0 on success, -EXXX on failure
 */
static int clog_clear_region(struct dm_ulog_request *rq, uint32_t originator)
{
	int r;
	int count;
	uint64_t *region;
	struct log_c *lc = get_log(rq->uuid, rq->luid);

	if (!lc)
		return -EINVAL;

	if (rq->data_size % sizeof(uint64_t)) {
		LOG_ERROR("Bad data size given for clear_region request");
		return -EINVAL;
	}

	count = rq->data_size / sizeof(uint64_t);
	region = (uint64_t *)&rq->data;

	for (; count > 0; count--, region++) {
		r = clear_region(lc, *region, originator);
		if (r)
			return r;
	}

	rq->data_size = 0;

	return 0;
}

/*
 * clog_get_resync_work
 * @rq
 *
 */
static int clog_get_resync_work(struct dm_ulog_request *rq, uint32_t originator)
{
	struct {
		int64_t i;
		uint64_t r;
	} *pkg = (void *)rq->data;
	struct log_c *lc = get_log(rq->uuid, rq->luid);

	if (!lc)
		return -EINVAL;

	rq->data_size = sizeof(*pkg);
	pkg->i = 0;

	if (lc->sync_search >= lc->region_count) {
		/*
		 * FIXME: handle intermittent errors during recovery
		 * by resetting sync_search... but not to many times.
		 */
		LOG_SPRINT(lc, "GET - SEQ#=%u, UUID=%s, nodeid = %u:: "
			   "Recovery finished",
			   rq->seq, SHORT_UUID(lc->uuid), originator);
		return 0;
	}

	if (lc->recovering_region != (uint64_t)-1) {
		if (lc->recoverer == originator) {
			LOG_SPRINT(lc, "GET - SEQ#=%u, UUID=%s, nodeid = %u:: "
				   "Re-requesting work (%llu)",
				   rq->seq, SHORT_UUID(lc->uuid), originator,
				   (unsigned long long)lc->recovering_region);
			pkg->r = lc->recovering_region;
			pkg->i = 1;
			LOG_COND(log_resend_requests, "***** RE-REQUEST *****");
		} else {
			LOG_SPRINT(lc, "GET - SEQ#=%u, UUID=%s, nodeid = %u:: "
				   "Someone already recovering (%llu)",
				   rq->seq, SHORT_UUID(lc->uuid), originator,
				   (unsigned long long)lc->recovering_region);
		}

		return 0;
	}

	while (lc->recovery_request_list) {
		struct recovery_request *del;

		del = lc->recovery_request_list;
		lc->recovery_request_list = del->next;

		pkg->r = del->region;
		free(del);

		if (!log_test_bit(lc->sync_bits, pkg->r)) {
			LOG_SPRINT(lc, "GET - SEQ#=%u, UUID=%s, nodeid = %u:: "
				   "Assigning priority resync work (%llu)",
				   rq->seq, SHORT_UUID(lc->uuid), originator,
				   (unsigned long long)pkg->r);
			pkg->i = 1;
			lc->recovering_region = pkg->r;
			lc->recoverer = originator;
			return 0;
		}
	}

	pkg->r = find_next_zero_bit(lc->sync_bits,
				    lc->sync_search);

	if (pkg->r >= lc->region_count) {
		LOG_SPRINT(lc, "GET - SEQ#=%u, UUID=%s, nodeid = %u:: "
			   "Resync work complete.",
			   rq->seq, SHORT_UUID(lc->uuid), originator);
		return 0;
	}

	lc->sync_search = pkg->r + 1;

	LOG_SPRINT(lc, "GET - SEQ#=%u, UUID=%s, nodeid = %u:: "
		   "Assigning resync work (%llu)",
		   rq->seq, SHORT_UUID(lc->uuid), originator,
		   (unsigned long long)pkg->r);
	pkg->i = 1;
	lc->recovering_region = pkg->r;
	lc->recoverer = originator;

	return 0;
}

/*
 * clog_set_region_sync
 * @rq
 */
static int clog_set_region_sync(struct dm_ulog_request *rq, uint32_t originator)
{
	struct {
		uint64_t region;
		int64_t in_sync;
	} *pkg = (void *)rq->data;
	struct log_c *lc = get_log(rq->uuid, rq->luid);

	if (!lc)
		return -EINVAL;

	lc->recovering_region = (uint64_t)-1;

	if (pkg->in_sync) {
		if (log_test_bit(lc->sync_bits, pkg->region)) {
			LOG_SPRINT(lc, "SET - SEQ#=%u, UUID=%s, nodeid = %u:: "
				   "Region already set (%llu)",
				   rq->seq, SHORT_UUID(lc->uuid), originator,
				   (unsigned long long)pkg->region);
		} else {
			log_set_bit(lc, lc->sync_bits, pkg->region);
			lc->sync_count++;

			/* The rest of this section is all for debugging */
			LOG_SPRINT(lc, "SET - SEQ#=%u, UUID=%s, nodeid = %u:: "
				   "Setting region (%llu)",
				   rq->seq, SHORT_UUID(lc->uuid), originator,
				   (unsigned long long)pkg->region);
			if (pkg->region == lc->skip_bit_warning)
				lc->skip_bit_warning = lc->region_count;

			if (pkg->region > (lc->skip_bit_warning + 5)) {
				LOG_ERROR("*** Region #%llu skipped during recovery ***",
					  (unsigned long long)lc->skip_bit_warning);
				lc->skip_bit_warning = lc->region_count;
#ifdef DEBUG
				kill(getpid(), SIGUSR1);
#endif
			}

			if (!log_test_bit(lc->sync_bits,
					  (pkg->region) ? pkg->region - 1 : 0)) {
				LOG_SPRINT(lc, "*** Previous bit not set ***");
				lc->skip_bit_warning = (pkg->region) ?
					pkg->region - 1 : 0;
			}
		}
	} else if (log_test_bit(lc->sync_bits, pkg->region)) {
		lc->sync_count--;
		log_clear_bit(lc, lc->sync_bits, pkg->region);
		LOG_SPRINT(lc, "SET - SEQ#=%u, UUID=%s, nodeid = %u:: "
			   "Unsetting region (%llu)",
			   rq->seq, SHORT_UUID(lc->uuid), originator,
			   (unsigned long long)pkg->region);
	}

	if (lc->sync_count != count_bits32(lc->sync_bits)) {
		unsigned long long reset = count_bits32(lc->sync_bits);

		LOG_SPRINT(lc, "SET - SEQ#=%u, UUID=%s, nodeid = %u:: "
			   "sync_count(%llu) != bitmap count(%llu)",
			   rq->seq, SHORT_UUID(lc->uuid), originator,
			   (unsigned long long)lc->sync_count, reset);
#ifdef DEBUG
		kill(getpid(), SIGUSR1);
#endif
		lc->sync_count = reset;
	}

	if (lc->sync_count > lc->region_count)
		LOG_SPRINT(lc, "SET - SEQ#=%u, UUID=%s, nodeid = %u:: "
			   "(lc->sync_count > lc->region_count) - this is bad",
			   rq->seq, SHORT_UUID(lc->uuid), originator);

	rq->data_size = 0;
	return 0;
}

/*
 * clog_get_sync_count
 * @rq
 */
static int clog_get_sync_count(struct dm_ulog_request *rq, uint32_t originator)
{
	uint64_t *sync_count = (uint64_t *)rq->data;
	struct log_c *lc = get_log(rq->uuid, rq->luid);

	/*
	 * FIXME: Mirror requires us to be able to ask for
	 * the sync count while pending... but I don't like
	 * it because other machines may not be suspended and
	 * the stored value may not be accurate.
	 */
	if (!lc)
		lc = get_pending_log(rq->uuid, rq->luid);

	if (!lc)
		return -EINVAL;

	*sync_count = lc->sync_count;

	rq->data_size = sizeof(*sync_count);

	if (lc->sync_count != count_bits32(lc->sync_bits)) {
		unsigned long long reset = count_bits32(lc->sync_bits);

		LOG_SPRINT(lc, "get_sync_count - SEQ#=%u, UUID=%s, nodeid = %u:: "
			   "sync_count(%llu) != bitmap count(%llu)",
			   rq->seq, SHORT_UUID(lc->uuid), originator,
			   (unsigned long long)lc->sync_count, reset);
#ifdef DEBUG
		kill(getpid(), SIGUSR1);
#endif
		lc->sync_count = reset;
	}

	return 0;
}

static int core_status_info(struct log_c *lc, struct dm_ulog_request *rq)
{
	char *data = (char *)rq->data;

	rq->data_size = sprintf(data, "1 clustered_core");

	return 0;
}

static int disk_status_info(struct log_c *lc, struct dm_ulog_request *rq)
{
	char *data = (char *)rq->data;
	struct stat statbuf;

	if(fstat(lc->disk_fd, &statbuf)) {
		rq->error = -errno;
		return -errno;
	}

	rq->data_size = sprintf(data, "3 clustered_disk %d:%d %c",
				major(statbuf.st_rdev), minor(statbuf.st_rdev),
				(lc->log_dev_failed) ? 'D' : 'A');

	return 0;
}

/*
 * clog_status_info
 * @rq
 *
 */
static int clog_status_info(struct dm_ulog_request *rq)
{
	int r;
	struct log_c *lc = get_log(rq->uuid, rq->luid);

	if (!lc)
		lc = get_pending_log(rq->uuid, rq->luid);

	if (!lc)
		return -EINVAL;

	if (lc->disk_fd == -1)
		r = core_status_info(lc, rq);
	else
		r = disk_status_info(lc, rq);

	return r;
}

static int core_status_table(struct log_c *lc, struct dm_ulog_request *rq)
{
	char *data = (char *)rq->data;

	rq->data_size = sprintf(data, "clustered_core %u %s%s ",
				lc->region_size,
				(lc->sync == DEFAULTSYNC) ? "" :
				(lc->sync == NOSYNC) ? "nosync " : "sync ",
				(lc->block_on_error) ? "block_on_error" : "");
	return 0;
}

static int disk_status_table(struct log_c *lc, struct dm_ulog_request *rq)
{
	char *data = (char *)rq->data;
	struct stat statbuf;

	if(fstat(lc->disk_fd, &statbuf)) {
		rq->error = -errno;
		return -errno;
	}

	rq->data_size = sprintf(data, "clustered_disk %d:%d %u %s%s ",
				major(statbuf.st_rdev), minor(statbuf.st_rdev),
				lc->region_size,
				(lc->sync == DEFAULTSYNC) ? "" :
				(lc->sync == NOSYNC) ? "nosync " : "sync ",
				(lc->block_on_error) ? "block_on_error" : "");
	return 0;
}

/*
 * clog_status_table
 * @rq
 *
 */
static int clog_status_table(struct dm_ulog_request *rq)
{
	int r;
	struct log_c *lc = get_log(rq->uuid, rq->luid);

	if (!lc)
		lc = get_pending_log(rq->uuid, rq->luid);

	if (!lc)
		return -EINVAL;

	if (lc->disk_fd == -1)
		r = core_status_table(lc, rq);
	else
		r = disk_status_table(lc, rq);

	return r;
}

/*
 * clog_is_remote_recovering
 * @rq
 *
 */
static int clog_is_remote_recovering(struct dm_ulog_request *rq)
{
	uint64_t region = *((uint64_t *)(rq->data));
	struct {
		int64_t is_recovering;
		uint64_t in_sync_hint;
	} *pkg = (void *)rq->data;
	struct log_c *lc = get_log(rq->uuid, rq->luid);

	if (!lc)
		return -EINVAL;

	if (region > lc->region_count)
		return -EINVAL;

	if (lc->recovery_halted) {
		LOG_DBG("[%s] Recovery halted... [not remote recovering]: %llu",
			SHORT_UUID(lc->uuid), (unsigned long long)region);
		pkg->is_recovering = 0;
		pkg->in_sync_hint = lc->region_count; /* none are recovering */
	} else {
		pkg->is_recovering = !log_test_bit(lc->sync_bits, region);

		/*
		 * Remember, 'lc->sync_search' is 1 plus the region
		 * currently being recovered.  So, we must take off 1
		 * to account for that; but only if 'sync_search > 1'.
		 */
		pkg->in_sync_hint = lc->sync_search ? (lc->sync_search - 1) : 0;
		LOG_DBG("[%s] Region is %s: %llu",
			SHORT_UUID(lc->uuid),
			(region == lc->recovering_region) ?
			"currently remote recovering" :
			(pkg->is_recovering) ? "pending remote recovery" :
			"not remote recovering", (unsigned long long)region);
	}

	if (pkg->is_recovering &&
	    (region != lc->recovering_region)) {
		struct recovery_request *rr;

		/* Already in the list? */
		for (rr = lc->recovery_request_list; rr; rr = rr->next)
			if (rr->region == region)
				goto out;

		/* Failure to allocated simply means we can't prioritize it */
		rr = malloc(sizeof(*rr));
		if (!rr)
			goto out;

		LOG_DBG("[%s] Adding region to priority list: %llu",
			SHORT_UUID(lc->uuid), (unsigned long long)region);
		rr->region = region;
		rr->next = lc->recovery_request_list;
		lc->recovery_request_list = rr;
	}

out:

	rq->data_size = sizeof(*pkg);

	return 0;	
}


/*
 * do_request
 * @rq: the request
 * @server: is this request performed by the server
 *
 * An inability to perform this function will return an error
 * from this function.  However, an inability to successfully
 * perform the request will fill in the 'rq->error' field.
 *
 * Returns: 0 on success, -EXXX on error
 */
int do_request(struct clog_request *rq, int server)
{
	int r;

	if (!rq)
		return 0;

	if (rq->u_rq.error)
		LOG_DBG("Programmer error: rq struct has error set");

	switch (rq->u_rq.request_type) {
	case DM_ULOG_CTR:
		r = clog_ctr(&rq->u_rq);
		break;
	case DM_ULOG_DTR:
		r = clog_dtr(&rq->u_rq);
		break;
	case DM_ULOG_PRESUSPEND:
		r = clog_presuspend(&rq->u_rq);
		break;
	case DM_ULOG_POSTSUSPEND:
		r = clog_postsuspend(&rq->u_rq);
		break;
	case DM_ULOG_RESUME:
		r = clog_resume(&rq->u_rq);
		break;
	case DM_ULOG_GET_REGION_SIZE:
		r = clog_get_region_size(&rq->u_rq);
		break;
	case DM_ULOG_IS_CLEAN:
		r = clog_is_clean(&rq->u_rq);
		break;
	case DM_ULOG_IN_SYNC:
		r = clog_in_sync(&rq->u_rq);
		break;
	case DM_ULOG_FLUSH:
		r = clog_flush(&rq->u_rq, server);
		break;
	case DM_ULOG_MARK_REGION:
		r = clog_mark_region(&rq->u_rq, rq->originator);
		break;
	case DM_ULOG_CLEAR_REGION:
		r = clog_clear_region(&rq->u_rq, rq->originator);
		break;
	case DM_ULOG_GET_RESYNC_WORK:
		r = clog_get_resync_work(&rq->u_rq, rq->originator);
		break;
	case DM_ULOG_SET_REGION_SYNC:
		r = clog_set_region_sync(&rq->u_rq, rq->originator);
		break;
	case DM_ULOG_GET_SYNC_COUNT:
		r = clog_get_sync_count(&rq->u_rq, rq->originator);
		break;
	case DM_ULOG_STATUS_INFO:
		r = clog_status_info(&rq->u_rq);
		break;
	case DM_ULOG_STATUS_TABLE:
		r = clog_status_table(&rq->u_rq);
		break;
	case DM_ULOG_IS_REMOTE_RECOVERING:
		r = clog_is_remote_recovering(&rq->u_rq);
		break;
	default:
		LOG_ERROR("Unknown request");
		r = rq->u_rq.error = -EINVAL;
		break;
	}

	if (r && !rq->u_rq.error)
		rq->u_rq.error = r;
	else if (r != rq->u_rq.error)
		LOG_DBG("Warning:  error from function != rq->u_rq.error");

	if (rq->u_rq.error && rq->u_rq.data_size) {
		/* Make sure I'm handling errors correctly above */
		LOG_DBG("Programmer error: rq->u_rq.error && rq->u_rq.data_size");
		rq->u_rq.data_size = 0;
	}

	return 0;
}

static void print_bits(char *buf, int size, int print)
{
	int i;
	char outbuf[128];

	memset(outbuf, 0, sizeof(outbuf));

	for (i = 0; i < size; i++) {
		if (!(i % 16)) {
			if (outbuf[0] != '\0') {
				if (print)
					LOG_PRINT("%s", outbuf);
				else
					LOG_DBG("%s", outbuf);
			}
			memset(outbuf, 0, sizeof(outbuf));
			sprintf(outbuf, "[%3d - %3d]", i, i+15);
		}
		sprintf(outbuf + strlen(outbuf), " %.2X", (unsigned char)buf[i]);
	}
	if (outbuf[0] != '\0') {
		if (print)
			LOG_PRINT("%s", outbuf);
		else
			LOG_DBG("%s", outbuf);
	}
}

/* int store_bits(const char *uuid, const char *which, char **buf)*/
int push_state(const char *uuid, uint64_t luid,
	       const char *which, char **buf, uint32_t debug_who)
{
	int bitset_size;
	struct log_c *lc;

	if (*buf)
		LOG_ERROR("store_bits: *buf != NULL");

	lc = get_log(uuid, luid);
	if (!lc) {
		LOG_ERROR("store_bits: No log found for %s", uuid);
		return -EINVAL;
	}

	if (!strcmp(which, "recovering_region")) {
		*buf = malloc(64); /* easily handles the 2 written numbers */
		if (!*buf)
			return -ENOMEM;
		sprintf(*buf, "%llu %u", (unsigned long long)lc->recovering_region,
			lc->recoverer);

		LOG_SPRINT(lc, "CKPT SEND - SEQ#=X, UUID=%s, nodeid = %u:: "
			   "recovering_region=%llu, recoverer=%u, sync_count=%llu",
			   SHORT_UUID(lc->uuid), debug_who,
			   (unsigned long long)lc->recovering_region,
			   lc->recoverer,
			   (unsigned long long)count_bits32(lc->sync_bits));
		return 64;
	}

	/* Size in 'int's */
	bitset_size = ((int)lc->clean_bits[0]/DM_BITS_PER_INT) + 1;

	/* Size in bytes */
	bitset_size *= 4;

	*buf = malloc(bitset_size);

	if (!*buf) {
		LOG_ERROR("store_bits: Unable to allocate memory");
		return -ENOMEM;
	}

	if (!strncmp(which, "sync_bits", 9)) {
		memcpy(*buf, lc->sync_bits + 1, bitset_size);
		LOG_DBG("[%s] storing sync_bits (sync_count = %llu):",
			SHORT_UUID(uuid), (unsigned long long)
			count_bits32(lc->sync_bits));
		print_bits(*buf, bitset_size, 0);
	} else if (!strncmp(which, "clean_bits", 9)) {
		memcpy(*buf, lc->clean_bits + 1, bitset_size);
		LOG_DBG("[%s] storing clean_bits:", SHORT_UUID(lc->uuid));
		print_bits(*buf, bitset_size, 0);
	}

	return bitset_size;
}

/*int load_bits(const char *uuid, const char *which, char *buf, int size)*/
int pull_state(const char *uuid, uint64_t luid,
	       const char *which, char *buf, int size)
{
	int bitset_size;
	struct log_c *lc;

	if (!buf)
		LOG_ERROR("pull_state: buf == NULL");

	lc = get_log(uuid, luid);
	if (!lc) {
		LOG_ERROR("pull_state: No log found for %s", uuid);
		return -EINVAL;
	}

	if (!strncmp(which, "recovering_region", 17)) {
		sscanf(buf, "%llu %u", (unsigned long long *)&lc->recovering_region,
		       &lc->recoverer);
		LOG_SPRINT(lc, "CKPT INIT - SEQ#=X, UUID=%s, nodeid = X:: "
			   "recovering_region=%llu, recoverer=%u",
			   SHORT_UUID(lc->uuid),
			   (unsigned long long)lc->recovering_region, lc->recoverer);
		return 0;
	}

	/* Size in 'int's */
	bitset_size = ((int)lc->clean_bits[0]/DM_BITS_PER_INT) + 1;

	/* Size in bytes */
	bitset_size *= 4;

	if (bitset_size != size) {
		LOG_ERROR("pull_state(%s): bad bitset_size (%d vs %d)",
			  which, size, bitset_size);
		return -EINVAL;
	}

	if (!strncmp(which, "sync_bits", 9)) {
		lc->resume_override += 1;
		memcpy(lc->sync_bits + 1, buf, bitset_size);
		LOG_DBG("[%s] loading sync_bits (sync_count = %llu):",
			SHORT_UUID(lc->uuid),(unsigned long long)
			count_bits32(lc->sync_bits));
		print_bits((char *)lc->sync_bits, bitset_size, 0);
	} else if (!strncmp(which, "clean_bits", 9)) {
		lc->resume_override += 2;
		memcpy(lc->clean_bits + 1, buf, bitset_size);
		LOG_DBG("[%s] loading clean_bits:", SHORT_UUID(lc->uuid));
		print_bits((char *)lc->clean_bits, bitset_size, 0);
	}

	return 0;
}

int log_get_state(struct dm_ulog_request *rq)
{
	struct log_c *lc;

	lc = get_log(rq->uuid, rq->luid);
	if (!lc)
		return -EINVAL;

	return lc->state;
}

/*
 * log_status
 *
 * Returns: 1 if logs are still present, 0 otherwise
 */
int log_status(void)
{
	if (!dm_list_empty(&log_list) || !dm_list_empty(&log_pending_list))
		return 1;

	return 0;
}

void log_debug(void)
{
	struct log_c *lc;
	uint64_t r;
	int i;

	LOG_ERROR("");
	LOG_ERROR("LOG COMPONENT DEBUGGING::");
	LOG_ERROR("Official log list:");
	LOG_ERROR("Pending log list:");
	dm_list_iterate_items(lc, &log_pending_list) {
		LOG_ERROR("%s", lc->uuid);
		LOG_ERROR("sync_bits:");
		print_bits((char *)lc->sync_bits, (int)lc->sync_bits[0], 1);
		LOG_ERROR("clean_bits:");
		print_bits((char *)lc->clean_bits, (int)lc->sync_bits[0], 1);
	}

	dm_list_iterate_items(lc, &log_list) {
		LOG_ERROR("%s", lc->uuid);
		LOG_ERROR("  recoverer        : %u", lc->recoverer);
		LOG_ERROR("  recovering_region: %llu",
			  (unsigned long long)lc->recovering_region);
		LOG_ERROR("  recovery_halted  : %s", (lc->recovery_halted) ?
			  "YES" : "NO");
		LOG_ERROR("sync_bits:");
		print_bits((char *)lc->sync_bits, (int)lc->sync_bits[0], 1);
		LOG_ERROR("clean_bits:");
		print_bits((char *)lc->clean_bits, (int)lc->sync_bits[0], 1);

		LOG_ERROR("Validating %s::", SHORT_UUID(lc->uuid));
		r = find_next_zero_bit(lc->sync_bits, 0);
		LOG_ERROR("  lc->region_count = %llu",
			  (unsigned long long)lc->region_count);
		LOG_ERROR("  lc->sync_count = %llu",
			  (unsigned long long)lc->sync_count);
		LOG_ERROR("  next zero bit  = %llu",
			  (unsigned long long)r);
		if ((r > lc->region_count) ||
		    ((r == lc->region_count) && (lc->sync_count > lc->region_count))) {
			LOG_ERROR("ADJUSTING SYNC_COUNT");
			lc->sync_count = lc->region_count;
		}

		LOG_ERROR("Resync request history:");
		for (i = 0; i < RESYNC_HISTORY; i++) {
			lc->idx++;
			lc->idx = lc->idx % RESYNC_HISTORY;
			if (lc->resync_history[lc->idx][0] == '\0')
				continue;
			LOG_ERROR("%d:%d) %s", i, lc->idx,
				  lc->resync_history[lc->idx]);
		}
	}
}
