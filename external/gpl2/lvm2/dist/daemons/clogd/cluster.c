/*	$NetBSD: cluster.c,v 1.1.1.1.2.2 2009/05/13 18:52:40 jym Exp $	*/

#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/socket.h> /* These are for OpenAIS CPGs */
#include <sys/select.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <openais/saAis.h>
#include <openais/cpg.h>
#include <openais/saCkpt.h>

#include "linux/dm-clog-tfr.h"
#include "list.h"
#include "functions.h"
#include "local.h"
#include "common.h"
#include "logging.h"
#include "link_mon.h"
#include "cluster.h"

/* Open AIS error codes */
#define str_ais_error(x) \
	((x) == SA_AIS_OK) ? "SA_AIS_OK" : \
	((x) == SA_AIS_ERR_LIBRARY) ? "SA_AIS_ERR_LIBRARY" : \
	((x) == SA_AIS_ERR_VERSION) ? "SA_AIS_ERR_VERSION" : \
	((x) == SA_AIS_ERR_INIT) ? "SA_AIS_ERR_INIT" : \
	((x) == SA_AIS_ERR_TIMEOUT) ? "SA_AIS_ERR_TIMEOUT" : \
	((x) == SA_AIS_ERR_TRY_AGAIN) ? "SA_AIS_ERR_TRY_AGAIN" : \
	((x) == SA_AIS_ERR_INVALID_PARAM) ? "SA_AIS_ERR_INVALID_PARAM" : \
	((x) == SA_AIS_ERR_NO_MEMORY) ? "SA_AIS_ERR_NO_MEMORY" : \
	((x) == SA_AIS_ERR_BAD_HANDLE) ? "SA_AIS_ERR_BAD_HANDLE" : \
	((x) == SA_AIS_ERR_BUSY) ? "SA_AIS_ERR_BUSY" : \
	((x) == SA_AIS_ERR_ACCESS) ? "SA_AIS_ERR_ACCESS" : \
	((x) == SA_AIS_ERR_NOT_EXIST) ? "SA_AIS_ERR_NOT_EXIST" : \
	((x) == SA_AIS_ERR_NAME_TOO_LONG) ? "SA_AIS_ERR_NAME_TOO_LONG" : \
	((x) == SA_AIS_ERR_EXIST) ? "SA_AIS_ERR_EXIST" : \
	((x) == SA_AIS_ERR_NO_SPACE) ? "SA_AIS_ERR_NO_SPACE" : \
	((x) == SA_AIS_ERR_INTERRUPT) ? "SA_AIS_ERR_INTERRUPT" : \
	((x) == SA_AIS_ERR_NAME_NOT_FOUND) ? "SA_AIS_ERR_NAME_NOT_FOUND" : \
	((x) == SA_AIS_ERR_NO_RESOURCES) ? "SA_AIS_ERR_NO_RESOURCES" : \
	((x) == SA_AIS_ERR_NOT_SUPPORTED) ? "SA_AIS_ERR_NOT_SUPPORTED" : \
	((x) == SA_AIS_ERR_BAD_OPERATION) ? "SA_AIS_ERR_BAD_OPERATION" : \
	((x) == SA_AIS_ERR_FAILED_OPERATION) ? "SA_AIS_ERR_FAILED_OPERATION" : \
	((x) == SA_AIS_ERR_MESSAGE_ERROR) ? "SA_AIS_ERR_MESSAGE_ERROR" : \
	((x) == SA_AIS_ERR_QUEUE_FULL) ? "SA_AIS_ERR_QUEUE_FULL" : \
	((x) == SA_AIS_ERR_QUEUE_NOT_AVAILABLE) ? "SA_AIS_ERR_QUEUE_NOT_AVAILABLE" : \
	((x) == SA_AIS_ERR_BAD_FLAGS) ? "SA_AIS_ERR_BAD_FLAGS" : \
	((x) == SA_AIS_ERR_TOO_BIG) ? "SA_AIS_ERR_TOO_BIG" : \
	((x) == SA_AIS_ERR_NO_SECTIONS) ? "SA_AIS_ERR_NO_SECTIONS" : \
	"ais_error_unknown"

#define DM_CLOG_RESPONSE 0x1000 /* in last byte of 32-bit value */
#define DM_CLOG_CHECKPOINT_READY 21
#define DM_CLOG_MEMBER_JOIN      22

#define _RQ_TYPE(x) \
	((x) == DM_CLOG_CHECKPOINT_READY) ? "DM_CLOG_CHECKPOINT_READY": \
	((x) == DM_CLOG_MEMBER_JOIN) ? "DM_CLOG_MEMBER_JOIN": \
	RQ_TYPE((x) & ~DM_CLOG_RESPONSE)

static uint32_t my_cluster_id = 0xDEAD;
static SaCkptHandleT ckpt_handle = 0;
static SaCkptCallbacksT callbacks = { 0, 0 };
static SaVersionT version = { 'B', 1, 1 };

#define DEBUGGING_HISTORY 50
static char debugging[DEBUGGING_HISTORY][128];
static int idx = 0;

static int log_resp_rec = 0;

struct checkpoint_data {
	uint32_t requester;
	char uuid[CPG_MAX_NAME_LENGTH];

	int bitmap_size; /* in bytes */
	char *sync_bits;
	char *clean_bits;
	char *recovering_region;
	struct checkpoint_data *next;
};	

#define INVALID 0
#define VALID   1
#define LEAVING 2

#define MAX_CHECKPOINT_REQUESTERS 10
struct clog_cpg {
	struct list_head list;

	uint32_t lowest_id;
	cpg_handle_t handle;
	struct cpg_name name;

	/* Are we the first, or have we received checkpoint? */
	int state;
	int cpg_state;  /* FIXME: debugging */
	int free_me;
	int delay;
	int resend_requests;
	struct list_head startup_list;
	struct list_head working_list;

	int checkpoints_needed;
	uint32_t checkpoint_requesters[MAX_CHECKPOINT_REQUESTERS];
	struct checkpoint_data *checkpoint_list;
};

/* FIXME: Need lock for this */
static struct list_head clog_cpg_list;

/*
 * cluster_send
 * @tfr
 *
 * Returns: 0 on success, -Exxx on error
 */
int cluster_send(struct clog_tfr *tfr)
{
	int r;
	int count=0;
	int found;
	struct iovec iov;
	struct clog_cpg *entry, *tmp;

	list_for_each_entry_safe(entry, tmp, &clog_cpg_list, list)
		if (!strncmp(entry->name.value, tfr->uuid, CPG_MAX_NAME_LENGTH)) {
			found = 1;
			break;
		}

	if (!found) {
		tfr->error = -ENOENT;
		return -ENOENT;
	}

	iov.iov_base = tfr;
	iov.iov_len = sizeof(struct clog_tfr) + tfr->data_size;

	if (entry->cpg_state != VALID)
		return -EINVAL;

	do {
		r = cpg_mcast_joined(entry->handle, CPG_TYPE_AGREED, &iov, 1);
		if (r != SA_AIS_ERR_TRY_AGAIN)
			break;
		count++;
		if (count < 10)
			LOG_PRINT("[%s]  Retry #%d of cpg_mcast_joined: %s",
				  SHORT_UUID(tfr->uuid), count, str_ais_error(r));
		else if ((count < 100) && !(count % 10))
			LOG_ERROR("[%s]  Retry #%d of cpg_mcast_joined: %s",
				  SHORT_UUID(tfr->uuid), count, str_ais_error(r));
		else if ((count < 1000) && !(count % 100))
			LOG_ERROR("[%s]  Retry #%d of cpg_mcast_joined: %s",
				  SHORT_UUID(tfr->uuid), count, str_ais_error(r));
		else if ((count < 10000) && !(count % 1000))
			LOG_ERROR("[%s]  Retry #%d of cpg_mcast_joined: %s - "
				  "OpenAIS not handling the load?",
				  SHORT_UUID(tfr->uuid), count, str_ais_error(r));
		usleep(1000);
	} while (1);

	if (r == CPG_OK)
		return 0;

	/* error codes found in openais/cpg.h */
	LOG_ERROR("cpg_mcast_joined error: %s", str_ais_error(r));

	tfr->error = -EBADE;
	return -EBADE;
}

static struct clog_tfr *get_matching_tfr(struct clog_tfr *t, struct list_head *l)
{
	struct clog_tfr *match;
	struct list_head *p, *n;

	list_for_each_safe(p, n, l) {
		match = (struct clog_tfr *)p;
		if (match->seq == t->seq) {
			list_del_init(p);
			return match;
		}
	}
	return NULL;
}

static char tfr_buffer[DM_CLOG_TFR_SIZE];
static int handle_cluster_request(struct clog_cpg *entry,
				  struct clog_tfr *tfr, int server)
{
	int r = 0;
	struct clog_tfr *t = (struct clog_tfr *)tfr_buffer;

	/*
	 * We need a separate clog_tfr struct, one that can carry
	 * a return payload.  Otherwise, the memory address after
	 * tfr will be altered - leading to problems
	 */
	memset(t, 0, DM_CLOG_TFR_SIZE);
	memcpy(t, tfr, sizeof(struct clog_tfr) + tfr->data_size);

	/*
	 * With resumes, we only handle our own.
	 * Resume is a special case that requires
	 * local action (to set up CPG), followed by
	 * a cluster action to co-ordinate reading
	 * the disk and checkpointing
	 */
	if ((t->request_type != DM_CLOG_RESUME) ||
	    (t->originator == my_cluster_id))
		r = do_request(t, server);

	if (server &&
	    (t->request_type != DM_CLOG_CLEAR_REGION) &&
	    (t->request_type != DM_CLOG_POSTSUSPEND)) {
		t->request_type |= DM_CLOG_RESPONSE;

		/*
		 * Errors from previous functions are in the tfr struct.
		 */
		r = cluster_send(t);
		if (r < 0)
			LOG_ERROR("cluster_send failed: %s", strerror(-r));
	}

	return r;
}

static int handle_cluster_response(struct clog_cpg *entry, struct clog_tfr *tfr)
{
	int r = 0;
	struct clog_tfr *orig_tfr;

	/*
	 * If I didn't send it, then I don't care about the response
	 */
	if (tfr->originator != my_cluster_id)
		return 0;

	tfr->request_type &= ~DM_CLOG_RESPONSE;
	orig_tfr = get_matching_tfr(tfr, &entry->working_list);

	if (!orig_tfr) {
		struct list_head *p, *n;
		struct clog_tfr *t;

		/* Unable to find match for response */

		LOG_ERROR("[%s] No match for cluster response: %s:%u",
			  SHORT_UUID(tfr->uuid),
			  _RQ_TYPE(tfr->request_type), tfr->seq);

		LOG_ERROR("Current local list:");
		if (list_empty(&entry->working_list))
			LOG_ERROR("   [none]");

		list_for_each_safe(p, n, &entry->working_list) {
			t = (struct clog_tfr *)p;
			LOG_ERROR("   [%s]  %s:%u", SHORT_UUID(t->uuid),
				  _RQ_TYPE(t->request_type), t->seq);
		}

		return -EINVAL;
	}

	if (log_resp_rec > 0) {
		LOG_COND(log_resend_requests,
			 "[%s] Response received to %s/#%u",
			 SHORT_UUID(tfr->uuid), _RQ_TYPE(tfr->request_type),
			 tfr->seq);
		log_resp_rec--;
	}

	/* FIXME: Ensure memcpy cannot explode */
	memcpy(orig_tfr, tfr, sizeof(*tfr) + tfr->data_size);

	r = kernel_send(orig_tfr);
	if (r)
		LOG_ERROR("Failed to send response to kernel");

	free(orig_tfr);
	return r;
}

static struct clog_cpg *find_clog_cpg(cpg_handle_t handle)
{
	struct clog_cpg *match, *tmp;

	list_for_each_entry_safe(match, tmp, &clog_cpg_list, list) {
		if (match->handle == handle)
			return match;
	}

	return NULL;
}

/*
 * prepare_checkpoint
 * @entry: clog_cpg describing the log
 * @cp_requester: nodeid requesting the checkpoint
 *
 * Creates and fills in a new checkpoint_data struct.
 *
 * Returns: checkpoint_data on success, NULL on error
 */
static struct checkpoint_data *prepare_checkpoint(struct clog_cpg *entry,
						  uint32_t cp_requester)
{
	int r;
	struct checkpoint_data *new;

	if (entry->state != VALID) {
		/*
		 * We can't store bitmaps yet, because the log is not
		 * valid yet.
		 */
		LOG_ERROR("Forced to refuse checkpoint for nodeid %u - log not valid yet",
			  cp_requester);
		return NULL;
	}

	new = malloc(sizeof(*new));
	if (!new) {
		LOG_ERROR("Unable to create checkpoint data for %u",
			  cp_requester);
		return NULL;
	}
	memset(new, 0, sizeof(*new));
	new->requester = cp_requester;
	strncpy(new->uuid, entry->name.value, entry->name.length);

	new->bitmap_size = push_state(entry->name.value, "clean_bits",
				      &new->clean_bits);
	if (new->bitmap_size <= 0) {
		LOG_ERROR("Failed to store clean_bits to checkpoint for node %u",
			  new->requester);
		free(new);
		return NULL;
	}

	new->bitmap_size = push_state(entry->name.value,
				      "sync_bits", &new->sync_bits);
	if (new->bitmap_size <= 0) {
		LOG_ERROR("Failed to store sync_bits to checkpoint for node %u",
			  new->requester);
		free(new->clean_bits);
		free(new);
		return NULL;
	}

	r = push_state(entry->name.value, "recovering_region", &new->recovering_region);
	if (r <= 0) {
		LOG_ERROR("Failed to store recovering_region to checkpoint for node %u",
			  new->requester);
		free(new->sync_bits);
		free(new->clean_bits);
		free(new);
		return NULL;
	}
	LOG_DBG("[%s] Checkpoint prepared for node %u:",
		SHORT_UUID(new->uuid), new->requester);
	LOG_DBG("  bitmap_size = %d", new->bitmap_size);

	return new;
}

/*
 * free_checkpoint
 * @cp: the checkpoint_data struct to free
 *
 */
static void free_checkpoint(struct checkpoint_data *cp)
{
	free(cp->recovering_region);
	free(cp->sync_bits);
	free(cp->clean_bits);
	free(cp);
}

static int export_checkpoint(struct checkpoint_data *cp)
{
	SaCkptCheckpointCreationAttributesT attr;
	SaCkptCheckpointHandleT h;
	SaCkptSectionIdT section_id;
	SaCkptSectionCreationAttributesT section_attr;
	SaCkptCheckpointOpenFlagsT flags;
	SaNameT name;
	SaAisErrorT rv;
	struct clog_tfr *tfr;
	int len, r = 0;
	char buf[32];

	LOG_DBG("Sending checkpointed data to %u", cp->requester);

	len = snprintf((char *)(name.value), SA_MAX_NAME_LENGTH,
		       "bitmaps_%s_%u", SHORT_UUID(cp->uuid), cp->requester);
	name.length = len;

	len = strlen(cp->recovering_region) + 1;

	attr.creationFlags = SA_CKPT_WR_ALL_REPLICAS;
	attr.checkpointSize = cp->bitmap_size * 2 + len;

	attr.retentionDuration = SA_TIME_MAX;
	attr.maxSections = 4;      /* don't know why we need +1 */

	attr.maxSectionSize = (cp->bitmap_size > len) ?	cp->bitmap_size : len;
	attr.maxSectionIdSize = 22;

	flags = SA_CKPT_CHECKPOINT_READ |
		SA_CKPT_CHECKPOINT_WRITE |
		SA_CKPT_CHECKPOINT_CREATE;

open_retry:
	rv = saCkptCheckpointOpen(ckpt_handle, &name, &attr, flags, 0, &h);
	if (rv == SA_AIS_ERR_TRY_AGAIN) {
		LOG_ERROR("export_checkpoint: ckpt open retry");
		usleep(1000);
		goto open_retry;
	}

	if (rv == SA_AIS_ERR_EXIST) {
		LOG_DBG("export_checkpoint: checkpoint already exists");
		return -EEXIST;
	}

	if (rv != SA_AIS_OK) {
		LOG_ERROR("[%s] Failed to open checkpoint for %u: %s",
			  SHORT_UUID(cp->uuid), cp->requester,
			  str_ais_error(rv));
		return -EIO; /* FIXME: better error */
	}

	/*
	 * Add section for sync_bits
	 */
	section_id.idLen = snprintf(buf, 32, "sync_bits");
	section_id.id = (unsigned char *)buf;
	section_attr.sectionId = &section_id;
	section_attr.expirationTime = SA_TIME_END;

sync_create_retry:
	rv = saCkptSectionCreate(h, &section_attr,
				 cp->sync_bits, cp->bitmap_size);
	if (rv == SA_AIS_ERR_TRY_AGAIN) {
		LOG_ERROR("Sync checkpoint section create retry");
		usleep(1000);
		goto sync_create_retry;
	}

	if (rv == SA_AIS_ERR_EXIST) {
		LOG_DBG("Sync checkpoint section already exists");
		saCkptCheckpointClose(h);
		return -EEXIST;
	}

	if (rv != SA_AIS_OK) {
		LOG_ERROR("Sync checkpoint section creation failed: %s",
			  str_ais_error(rv));
		saCkptCheckpointClose(h);
		return -EIO; /* FIXME: better error */
	}

	/*
	 * Add section for clean_bits
	 */
	section_id.idLen = snprintf(buf, 32, "clean_bits");
	section_id.id = (unsigned char *)buf;
	section_attr.sectionId = &section_id;
	section_attr.expirationTime = SA_TIME_END;

clean_create_retry:
	rv = saCkptSectionCreate(h, &section_attr, cp->clean_bits, cp->bitmap_size);
	if (rv == SA_AIS_ERR_TRY_AGAIN) {
		LOG_ERROR("Clean checkpoint section create retry");
		usleep(1000);
		goto clean_create_retry;
	}

	if (rv == SA_AIS_ERR_EXIST) {
		LOG_DBG("Clean checkpoint section already exists");
		saCkptCheckpointClose(h);
		return -EEXIST;
	}

	if (rv != SA_AIS_OK) {
		LOG_ERROR("Clean checkpoint section creation failed: %s",
			  str_ais_error(rv));
		saCkptCheckpointClose(h);
		return -EIO; /* FIXME: better error */
	}

	/*
	 * Add section for recovering_region
	 */
	section_id.idLen = snprintf(buf, 32, "recovering_region");
	section_id.id = (unsigned char *)buf;
	section_attr.sectionId = &section_id;
	section_attr.expirationTime = SA_TIME_END;

rr_create_retry:
	rv = saCkptSectionCreate(h, &section_attr, cp->recovering_region,
				 strlen(cp->recovering_region) + 1);
	if (rv == SA_AIS_ERR_TRY_AGAIN) {
		LOG_ERROR("RR checkpoint section create retry");
		usleep(1000);
		goto rr_create_retry;
	}

	if (rv == SA_AIS_ERR_EXIST) {
		LOG_DBG("RR checkpoint section already exists");
		saCkptCheckpointClose(h);
		return -EEXIST;
	}

	if (rv != SA_AIS_OK) {
		LOG_ERROR("RR checkpoint section creation failed: %s",
			  str_ais_error(rv));
		saCkptCheckpointClose(h);
		return -EIO; /* FIXME: better error */
	}

	LOG_DBG("export_checkpoint: closing checkpoint");
	saCkptCheckpointClose(h);

	tfr = malloc(DM_CLOG_TFR_SIZE);
	if (!tfr) {
		LOG_ERROR("export_checkpoint: Unable to allocate transfer structs");
		return -ENOMEM;
	}
	memset(tfr, 0, sizeof(*tfr));

	INIT_LIST_HEAD((struct list_head *)&tfr->private);
	tfr->request_type = DM_CLOG_CHECKPOINT_READY;
	tfr->originator = cp->requester;  /* FIXME: hack to overload meaning of originator */
	strncpy(tfr->uuid, cp->uuid, CPG_MAX_NAME_LENGTH);

	r = cluster_send(tfr);
	if (r)
		LOG_ERROR("Failed to send checkpoint ready notice: %s",
			  strerror(-r));

	free(tfr);
	return 0;
}

static int import_checkpoint(struct clog_cpg *entry, int no_read)
{
	int rtn = 0;
	SaCkptCheckpointHandleT h;
	SaCkptSectionIterationHandleT itr;
	SaCkptSectionDescriptorT desc;
	SaCkptIOVectorElementT iov;
	SaNameT name;
	SaAisErrorT rv;
	char *bitmap = NULL;
	int len;

	bitmap = malloc(1024*1024);
	if (!bitmap)
		return -ENOMEM;

	len = snprintf((char *)(name.value), SA_MAX_NAME_LENGTH, "bitmaps_%s_%u",
		       SHORT_UUID(entry->name.value), my_cluster_id);
	name.length = len;

open_retry:
	rv = saCkptCheckpointOpen(ckpt_handle, &name, NULL,
				  SA_CKPT_CHECKPOINT_READ, 0, &h);
	if (rv == SA_AIS_ERR_TRY_AGAIN) {
		LOG_ERROR("import_checkpoint: ckpt open retry");
		usleep(1000);
		goto open_retry;
	}

	if (rv != SA_AIS_OK) {
		LOG_ERROR("[%s] Failed to open checkpoint: %s",
			  SHORT_UUID(entry->name.value), str_ais_error(rv));
		return -EIO; /* FIXME: better error */
	}

unlink_retry:
	rv = saCkptCheckpointUnlink(ckpt_handle, &name);
	if (rv == SA_AIS_ERR_TRY_AGAIN) {
		LOG_ERROR("import_checkpoint: ckpt unlink retry");
		usleep(1000);
		goto unlink_retry;
	}

	if (no_read) {
		LOG_DBG("Checkpoint for this log already received");
		goto no_read;
	}

init_retry:
	rv = saCkptSectionIterationInitialize(h, SA_CKPT_SECTIONS_ANY,
					      SA_TIME_END, &itr);
	if (rv == SA_AIS_ERR_TRY_AGAIN) {
		LOG_ERROR("import_checkpoint: sync create retry");
		usleep(1000);
		goto init_retry;
	}

	if (rv != SA_AIS_OK) {
		LOG_ERROR("[%s] Sync checkpoint section creation failed: %s",
			  SHORT_UUID(entry->name.value), str_ais_error(rv));
		return -EIO; /* FIXME: better error */
	}

	len = 0;
	while (1) {
		rv = saCkptSectionIterationNext(itr, &desc);
		if (rv == SA_AIS_OK)
			len++;
		else if ((rv == SA_AIS_ERR_NO_SECTIONS) && len)
			break;
		else if (rv != SA_AIS_ERR_TRY_AGAIN) {
			LOG_ERROR("saCkptSectionIterationNext failure: %d", rv);
			break;
		}
	}
	saCkptSectionIterationFinalize(itr);
	if (len != 3) {
		LOG_ERROR("import_checkpoint: %d checkpoint sections found",
			  len);
		usleep(1000);
		goto init_retry;
	}
	saCkptSectionIterationInitialize(h, SA_CKPT_SECTIONS_ANY,
					 SA_TIME_END, &itr);

	while (1) {
		rv = saCkptSectionIterationNext(itr, &desc);
		if (rv == SA_AIS_ERR_NO_SECTIONS)
			break;

		if (rv == SA_AIS_ERR_TRY_AGAIN) {
			LOG_ERROR("import_checkpoint: ckpt iternext retry");
			usleep(1000);
			continue;
		}

		if (rv != SA_AIS_OK) {
			LOG_ERROR("import_checkpoint: clean checkpoint section "
				  "creation failed: %s", str_ais_error(rv));
			rtn = -EIO; /* FIXME: better error */
			goto fail;
		}

		if (!desc.sectionSize) {
			LOG_ERROR("Checkpoint section empty");
			continue;
		}

		memset(bitmap, 0, sizeof(*bitmap));
		iov.sectionId = desc.sectionId;
		iov.dataBuffer = bitmap;
		iov.dataSize = desc.sectionSize;
		iov.dataOffset = 0;

	read_retry:
		rv = saCkptCheckpointRead(h, &iov, 1, NULL);
		if (rv == SA_AIS_ERR_TRY_AGAIN) {
			LOG_ERROR("ckpt read retry");
			usleep(1000);
			goto read_retry;
		}

		if (rv != SA_AIS_OK) {
			LOG_ERROR("import_checkpoint: ckpt read error: %s",
				  str_ais_error(rv));
			rtn = -EIO; /* FIXME: better error */
			goto fail;
		}

		if (iov.readSize) {
			if (pull_state(entry->name.value,
				       (char *)desc.sectionId.id, bitmap,
				       iov.readSize)) {
				LOG_ERROR("Error loading state");
				rtn = -EIO;
				goto fail;
			}
		} else {
			/* Need to request new checkpoint */
			rtn = -EAGAIN;
			goto fail;
		}
	}

fail:
	saCkptSectionIterationFinalize(itr);
no_read:
	saCkptCheckpointClose(h);

	free(bitmap);
	return rtn;
}

static void do_checkpoints(struct clog_cpg *entry)
{
	struct checkpoint_data *cp;

	for (cp = entry->checkpoint_list; cp;) {
		LOG_COND(log_checkpoint,
			 "[%s] Checkpoint data available for node %u",
			 SHORT_UUID(entry->name.value), cp->requester);

		/*
		 * FIXME: Check return code.  Could send failure
		 * notice in tfr in export_checkpoint function
		 * by setting tfr->error
		 */
		switch (export_checkpoint(cp)) {
		case -EEXIST:
			LOG_COND(log_checkpoint,
				 "[%s] Checkpoint for %u already handled",
				 SHORT_UUID(entry->name.value), cp->requester);
		case 0:
			entry->checkpoint_list = cp->next;
			free_checkpoint(cp);
			cp = entry->checkpoint_list;
			break;
		default:
			/* FIXME: Skipping will cause list corruption */
			LOG_ERROR("[%s] Failed to export checkpoint for %u",
				  SHORT_UUID(entry->name.value), cp->requester);
		}
	}
}

static int resend_requests(struct clog_cpg *entry)
{
	int r = 0;
	struct list_head *p, *n;
	struct clog_tfr *tfr;

	if (!entry->resend_requests || entry->delay)
		return 0;

	if (entry->state != VALID)
		return 0;

	entry->resend_requests = 0;

	list_for_each_safe(p, n, &entry->working_list) {
		list_del_init(p);
		tfr = (struct clog_tfr *)p;

		if (strcmp(entry->name.value, tfr->uuid)) {
			LOG_ERROR("[%s]  Stray request from another log (%s)",
				  SHORT_UUID(entry->name.value),
				  SHORT_UUID(tfr->uuid));
			free(tfr);
			continue;
		}

		switch (tfr->request_type) {
		case DM_CLOG_RESUME:
			/* We are only concerned about this request locally */
		case DM_CLOG_SET_REGION_SYNC:
			/*
			 * Some requests simply do not need to be resent.
			 * If it is a request that just changes log state,
			 * then it doesn't need to be resent (everyone makes
			 * updates).
			 */
			LOG_COND(log_resend_requests,
				 "[%s] Skipping resend of %s/#%u...",
				 SHORT_UUID(entry->name.value),
				 _RQ_TYPE(tfr->request_type), tfr->seq);
			idx++;
			idx = idx % DEBUGGING_HISTORY;
			sprintf(debugging[idx], "###  No resend: [%s] %s/%u ###",
				SHORT_UUID(entry->name.value), _RQ_TYPE(tfr->request_type),
				tfr->seq);
			tfr->data_size = 0;
			kernel_send(tfr);
				
			break;
			
		default:
			/*
			 * If an action or a response is required, then
			 * the request must be resent.
			 */
			LOG_COND(log_resend_requests,
				 "[%s] Resending %s(#%u) due to new server(%u)",
				 SHORT_UUID(entry->name.value),
				 _RQ_TYPE(tfr->request_type),
				 tfr->seq, entry->lowest_id);
			idx++;
			idx = idx % DEBUGGING_HISTORY;
			sprintf(debugging[idx], "***  Resending: [%s] %s/%u ***",
				SHORT_UUID(entry->name.value), _RQ_TYPE(tfr->request_type),
				tfr->seq);
			r = cluster_send(tfr);
			if (r < 0)
				LOG_ERROR("Failed resend");
		}
		free(tfr);
	}

	return r;
}

static int do_cluster_work(void *data)
{
	int r = SA_AIS_OK;
	struct clog_cpg *entry, *tmp;

	list_for_each_entry_safe(entry, tmp, &clog_cpg_list, list) {
		r = cpg_dispatch(entry->handle, CPG_DISPATCH_ALL);
		if (r != SA_AIS_OK)
			LOG_ERROR("cpg_dispatch failed: %s", str_ais_error(r));

		if (entry->free_me) {
			free(entry);
			continue;
		}
		do_checkpoints(entry);

		resend_requests(entry);
	}

	return (r == SA_AIS_OK) ? 0 : -1;  /* FIXME: good error number? */
}

static int flush_startup_list(struct clog_cpg *entry)
{
	int r = 0;
	int i_was_server;
	struct list_head *p, *n;
	struct clog_tfr *tfr = NULL;
	struct checkpoint_data *new;

	list_for_each_safe(p, n, &entry->startup_list) {
		list_del_init(p);
		tfr = (struct clog_tfr *)p;
		if (tfr->request_type == DM_CLOG_MEMBER_JOIN) {
			new = prepare_checkpoint(entry, tfr->originator);
			if (!new) {
				/*
				 * FIXME: Need better error handling.  Other nodes
				 * will be trying to send the checkpoint too, and we
				 * must continue processing the list; so report error
				 * but continue.
				 */
				LOG_ERROR("Failed to prepare checkpoint for %u!!!",
					  tfr->originator);
				free(tfr);
				continue;
			}
			LOG_COND(log_checkpoint, "[%s] Checkpoint prepared for %u",
				 SHORT_UUID(entry->name.value), tfr->originator);
			new->next = entry->checkpoint_list;
			entry->checkpoint_list = new;
		} else {
			LOG_DBG("[%s] Processing delayed request: %s",
				SHORT_UUID(tfr->uuid), _RQ_TYPE(tfr->request_type));
			i_was_server = (tfr->error == my_cluster_id) ? 1 : 0;
			tfr->error = 0;
			r = handle_cluster_request(entry, tfr, i_was_server);

			if (r)
				/*
				 * FIXME: If we error out here, we will never get
				 * another opportunity to retry these requests
				 */
				LOG_ERROR("Error while processing delayed CPG message");
		}
		free(tfr);
	}
	return 0;
}

static void cpg_message_callback(cpg_handle_t handle, struct cpg_name *gname,
				 uint32_t nodeid, uint32_t pid,
				 void *msg, int msg_len)
{
	int i;
	int r = 0;
	int i_am_server;
	int response = 0;
	struct clog_tfr *tfr = msg;
	struct clog_tfr *tmp_tfr = NULL;
	struct clog_cpg *match;

	match = find_clog_cpg(handle);
	if (!match) {
		LOG_ERROR("Unable to find clog_cpg for cluster message");
		return;
	}

	if ((nodeid == my_cluster_id) &&
	    !(tfr->request_type & DM_CLOG_RESPONSE) &&
	    (tfr->request_type != DM_CLOG_CLEAR_REGION) &&
	    (tfr->request_type != DM_CLOG_CHECKPOINT_READY)) {
		tmp_tfr = malloc(DM_CLOG_TFR_SIZE);
		if (!tmp_tfr) {
			/*
			 * FIXME: It may be possible to continue... but we
			 * would not be able to resend any messages that might
			 * be necessary during membership changes
			 */
			LOG_ERROR("[%s] Unable to record request: -ENOMEM",
				  SHORT_UUID(tfr->uuid));
			return;
		}
		memcpy(tmp_tfr, tfr, sizeof(*tfr) + tfr->data_size);
		list_add_tail((struct list_head *)&tmp_tfr->private, &match->working_list);
	}

	if (tfr->request_type == DM_CLOG_POSTSUSPEND) {
		/*
		 * If the server (lowest_id) indicates it is leaving,
		 * then we must resend any outstanding requests.  However,
		 * we do not want to resend them if the next server in
		 * line is in the process of leaving.
		 */
		if (nodeid == my_cluster_id) {
			LOG_COND(log_resend_requests, "[%s] I am leaving.1.....",
				 SHORT_UUID(tfr->uuid));
		} else {
			if (nodeid < my_cluster_id) {
				if (nodeid == match->lowest_id) {
					struct list_head *p, *n;
					
					match->resend_requests = 1;
					LOG_COND(log_resend_requests, "[%s] %u is leaving, resend required%s",
						 SHORT_UUID(tfr->uuid), nodeid,
						 (list_empty(&match->working_list)) ? " -- working_list empty": "");
			
					list_for_each_safe(p, n, &match->working_list) {
						tmp_tfr = (struct clog_tfr *)p;
					
						LOG_COND(log_resend_requests,
							 "[%s]                %s/%u",
							 SHORT_UUID(tmp_tfr->uuid),
							 _RQ_TYPE(tmp_tfr->request_type), tmp_tfr->seq);
					}
				}

				match->delay++;
				LOG_COND(log_resend_requests, "[%s] %u is leaving, delay = %d",
					 SHORT_UUID(tfr->uuid), nodeid, match->delay);
			}
			goto out;
		}
	}

	/*
	 * We can receive messages after we do a cpg_leave but before we
	 * get our config callback.  However, since we can't respond after
	 * leaving, we simply return.
	 */
	if (match->state == LEAVING)
		return;

	i_am_server = (my_cluster_id == match->lowest_id) ? 1 : 0;

	if (tfr->request_type == DM_CLOG_CHECKPOINT_READY) {
		if (my_cluster_id == tfr->originator) {
			/* Redundant checkpoints ignored if match->valid */
			if (import_checkpoint(match, (match->state != INVALID))) {
				LOG_ERROR("[%s] Failed to import checkpoint from %u",
					  SHORT_UUID(tfr->uuid), nodeid);
				/* Could we retry? */
				goto out;
			} else if (match->state == INVALID) {
				LOG_COND(log_checkpoint,
					 "[%s] Checkpoint data received from %u.  Log is now valid",
					 SHORT_UUID(match->name.value), nodeid);
				match->state = VALID;

				flush_startup_list(match);
			}
		}
		goto out;
	}

	/*
	 * If the log is now valid, we can queue the checkpoints
	 */
	for (i = match->checkpoints_needed; i; ) {
		struct checkpoint_data *new;

		i--;
		new = prepare_checkpoint(match, match->checkpoint_requesters[i]);
		if (!new) {
			/* FIXME: Need better error handling */
			LOG_ERROR("[%s] Failed to prepare checkpoint for %u!!!",
				  SHORT_UUID(tfr->uuid), match->checkpoint_requesters[i]);
			break;
		}
		LOG_COND(log_checkpoint, "[%s] Checkpoint prepared for %u*",
			 SHORT_UUID(tfr->uuid), match->checkpoint_requesters[i]);
		match->checkpoints_needed--;

		new->next = match->checkpoint_list;
		match->checkpoint_list = new;
	}		

	if (tfr->request_type & DM_CLOG_RESPONSE) {
		response = 1;
		r = handle_cluster_response(match, tfr);
	} else {
		tfr->originator = nodeid;

		if (match->state == LEAVING) {
			LOG_ERROR("[%s]  Ignoring %s from %u.  Reason: I'm leaving",
				  SHORT_UUID(tfr->uuid), _RQ_TYPE(tfr->request_type),
				  tfr->originator);
			goto out;
		}

		if (match->state == INVALID) {
			LOG_DBG("Log not valid yet, storing request");
			tmp_tfr = malloc(DM_CLOG_TFR_SIZE);
			if (!tmp_tfr) {
				LOG_ERROR("cpg_message_callback:  Unable to"
					  " allocate transfer structs");
				r = -ENOMEM; /* FIXME: Better error #? */
				goto out;
			}

			memcpy(tmp_tfr, tfr, sizeof(*tfr) + tfr->data_size);
			tmp_tfr->error = match->lowest_id;
			list_add_tail((struct list_head *)&tmp_tfr->private,
				      &match->startup_list);
			goto out;
		}

		r = handle_cluster_request(match, tfr, i_am_server);
	}

out:
	/* nothing happens after this point.  It is just for debugging */
	if (r) {
		LOG_ERROR("[%s] Error while processing CPG message, %s: %s",
			  SHORT_UUID(tfr->uuid),
			  _RQ_TYPE(tfr->request_type & ~DM_CLOG_RESPONSE),
			  strerror(-r));
		LOG_ERROR("[%s]    Response  : %s", SHORT_UUID(tfr->uuid),
			  (response) ? "YES" : "NO");
		LOG_ERROR("[%s]    Originator: %u",
			  SHORT_UUID(tfr->uuid), tfr->originator);
		if (response)
			LOG_ERROR("[%s]    Responder : %u",
				  SHORT_UUID(tfr->uuid), nodeid);

               LOG_ERROR("HISTORY::");
               for (i = 0; i < DEBUGGING_HISTORY; i++) {
                       idx++;
                       idx = idx % DEBUGGING_HISTORY;
                       if (debugging[idx][0] == '\0')
                               continue;
                       LOG_ERROR("%d:%d) %s", i, idx, debugging[idx]);
               }
       } else if (!(tfr->request_type & DM_CLOG_RESPONSE) ||
                  (tfr->originator == my_cluster_id)) {
               int len;
               idx++;
               idx = idx % DEBUGGING_HISTORY;
               len = sprintf(debugging[idx],
                             "SEQ#=%u, UUID=%s, TYPE=%s, ORIG=%u, RESP=%s",
                             tfr->seq,
                             SHORT_UUID(tfr->uuid),
                             _RQ_TYPE(tfr->request_type),
                             tfr->originator, (response) ? "YES" : "NO");
               if (response)
                       sprintf(debugging[idx] + len, ", RSPR=%u", nodeid);
	}
}

static void cpg_join_callback(struct clog_cpg *match,
			      struct cpg_address *joined,
			      struct cpg_address *member_list,
			      int member_list_entries)
{
	int i;
	int my_pid = getpid();
	uint32_t lowest = match->lowest_id;
	struct clog_tfr *tfr;

	/* Assign my_cluster_id */
	if ((my_cluster_id == 0xDEAD) && (joined->pid == my_pid))
		my_cluster_id = joined->nodeid;

	/* Am I the very first to join? */
	if (member_list_entries == 1) {
		match->lowest_id = joined->nodeid;
		match->state = VALID;
	}

	/* If I am part of the joining list, I do not send checkpoints */
	if (joined->nodeid == my_cluster_id)
		goto out;

	LOG_COND(log_checkpoint, "[%s] Joining node, %u needs checkpoint",
		 SHORT_UUID(match->name.value), joined->nodeid);

	/*
	 * FIXME: remove checkpoint_requesters/checkpoints_needed, and use
	 * the startup_list interface exclusively
	 */
	if (list_empty(&match->startup_list) && (match->state == VALID) &&
	    (match->checkpoints_needed < MAX_CHECKPOINT_REQUESTERS)) {
		match->checkpoint_requesters[match->checkpoints_needed++] = joined->nodeid;
		goto out;
	}

	tfr = malloc(DM_CLOG_TFR_SIZE);
	if (!tfr) {
		LOG_ERROR("cpg_config_callback: "
			  "Unable to allocate transfer structs");
		LOG_ERROR("cpg_config_callback: "
			  "Unable to perform checkpoint");
		goto out;
	}
	tfr->request_type = DM_CLOG_MEMBER_JOIN;
	tfr->originator   = joined->nodeid;
	list_add_tail((struct list_head *)&tfr->private, &match->startup_list);

out:
	/* Find the lowest_id, i.e. the server */
	match->lowest_id = member_list[0].nodeid;
	for (i = 0; i < member_list_entries; i++)
		if (match->lowest_id > member_list[i].nodeid)
			match->lowest_id = member_list[i].nodeid;

	if (lowest == 0xDEAD)
		LOG_COND(log_membership_change, "[%s]  Server change <none> -> %u (%u %s)",
			 SHORT_UUID(match->name.value), match->lowest_id,
			 joined->nodeid, (member_list_entries == 1) ?
			 "is first to join" : "joined");
	else if (lowest != match->lowest_id)
		LOG_COND(log_membership_change, "[%s]  Server change %u -> %u (%u joined)",
			 SHORT_UUID(match->name.value), lowest,
			 match->lowest_id, joined->nodeid);
	else
		LOG_COND(log_membership_change, "[%s]  Server unchanged at %u (%u joined)",
			 SHORT_UUID(match->name.value),
			 lowest, joined->nodeid);
	idx++;
	idx = idx % DEBUGGING_HISTORY;
	sprintf(debugging[idx], "+++  UUID=%s  %u join  +++",
		SHORT_UUID(match->name.value), joined->nodeid);
}

static void cpg_leave_callback(struct clog_cpg *match,
			       struct cpg_address *left,
			       struct cpg_address *member_list,
			       int member_list_entries)
{
	int i, fd;
	struct list_head *p, *n;
	uint32_t lowest = match->lowest_id;
	struct clog_tfr *tfr;

	{
               idx++;
               idx = idx % DEBUGGING_HISTORY;
               sprintf(debugging[idx], "---  UUID=%s  %u left  ---",
		       SHORT_UUID(match->name.value), left->nodeid);
	}

	/* Am I leaving? */
	if (my_cluster_id == left->nodeid) {
		LOG_DBG("Finalizing leave...");
		list_del_init(&match->list);

		cpg_fd_get(match->handle, &fd);
		links_unregister(fd);

		cluster_postsuspend(match->name.value);

		list_for_each_safe(p, n, &match->working_list) {
			list_del_init(p);
			tfr = (struct clog_tfr *)p;

			if (tfr->request_type == DM_CLOG_POSTSUSPEND)
				kernel_send(tfr);
			free(tfr);
		}

		cpg_finalize(match->handle);

		match->free_me = 1;
		match->lowest_id = 0xDEAD;
		match->state = INVALID;
	}			

	if (left->nodeid < my_cluster_id) {
		match->delay = (match->delay > 0) ? match->delay - 1 : 0;
		if (!match->delay && list_empty(&match->working_list))
			match->resend_requests = 0;
		LOG_COND(log_resend_requests, "[%s] %u has left, delay = %d%s",
			 SHORT_UUID(match->name.value), left->nodeid,
			 match->delay, (list_empty(&match->working_list)) ?
			 " -- working_list empty": "");
	}

	/* Find the lowest_id, i.e. the server */
	if (!member_list_entries) {
		match->lowest_id = 0xDEAD;
		LOG_COND(log_membership_change, "[%s]  Server change %u -> <none> "
			 "(%u is last to leave)",
			 SHORT_UUID(match->name.value), left->nodeid,
			 left->nodeid);
		return;
	}
		
	match->lowest_id = member_list[0].nodeid;
	for (i = 0; i < member_list_entries; i++)
		if (match->lowest_id > member_list[i].nodeid)
			match->lowest_id = member_list[i].nodeid;

	if (lowest != match->lowest_id) {
		LOG_COND(log_membership_change, "[%s]  Server change %u -> %u (%u left)",
			 SHORT_UUID(match->name.value), lowest,
			 match->lowest_id, left->nodeid);
	} else
		LOG_COND(log_membership_change, "[%s]  Server unchanged at %u (%u left)",
			 SHORT_UUID(match->name.value), lowest, left->nodeid);

	if ((match->state == INVALID) && !match->free_me) {
		/*
		 * If all CPG members are waiting for checkpoints and they
		 * are all present in my startup_list, then I was the first to
		 * join and I must assume control.
		 *
		 * We do not normally end up here, but if there was a quick
		 * 'resume -> suspend -> resume' across the cluster, we may
		 * have initially thought we were not the first to join because
		 * of the presence of out-going (and unable to respond) members.
		 */

		i = 1; /* We do not have a DM_CLOG_MEMBER_JOIN entry */
		list_for_each_safe(p, n, &match->startup_list) {
			tfr = (struct clog_tfr *)p;
			if (tfr->request_type == DM_CLOG_MEMBER_JOIN)
				i++;
		}

		if (i == member_list_entries) {
			/* 
			 * Last node who could have given me a checkpoint just left.
			 * Setting log state to VALID and acting as 'first join'.
			 */
			match->state = VALID;
			flush_startup_list(match);
		}
	}
}

static void cpg_config_callback(cpg_handle_t handle, struct cpg_name *gname,
				struct cpg_address *member_list,
				int member_list_entries,
				struct cpg_address *left_list,
				int left_list_entries,
				struct cpg_address *joined_list,
				int joined_list_entries)
{
	struct clog_cpg *match, *tmp;
	int found = 0;

	list_for_each_entry_safe(match, tmp, &clog_cpg_list, list)
		if (match->handle == handle) {
			found = 1;
			break;
		}

	if (!found) {
		LOG_ERROR("Unable to find match for CPG config callback");
		return;
	}

	if ((joined_list_entries + left_list_entries) > 1)
		LOG_ERROR("[%s]  More than one node joining/leaving",
			  SHORT_UUID(match->name.value));

	if (joined_list_entries)
		cpg_join_callback(match, joined_list,
				  member_list, member_list_entries);
	else
		cpg_leave_callback(match, left_list,
				  member_list, member_list_entries);
}

cpg_callbacks_t cpg_callbacks = {
	.cpg_deliver_fn = cpg_message_callback,
	.cpg_confchg_fn = cpg_config_callback,
};

/*
 * remove_checkpoint
 * @entry
 *
 * Returns: 1 if checkpoint removed, 0 if no checkpoints, -EXXX on error
 */
int remove_checkpoint(struct clog_cpg *entry)
{
	int len;
	SaNameT name;
	SaAisErrorT rv;
	SaCkptCheckpointHandleT h;

	len = snprintf((char *)(name.value), SA_MAX_NAME_LENGTH, "bitmaps_%s_%u",
                       SHORT_UUID(entry->name.value), my_cluster_id);
	name.length = len;

open_retry:
	rv = saCkptCheckpointOpen(ckpt_handle, &name, NULL,
                                  SA_CKPT_CHECKPOINT_READ, 0, &h);
	if (rv == SA_AIS_ERR_TRY_AGAIN) {
		LOG_ERROR("abort_startup: ckpt open retry");
                usleep(1000);
                goto open_retry;
        }

	if (rv != SA_AIS_OK)
                return 0;

	LOG_DBG("[%s]  Removing checkpoint", SHORT_UUID(entry->name.value));
unlink_retry:
        rv = saCkptCheckpointUnlink(ckpt_handle, &name);
        if (rv == SA_AIS_ERR_TRY_AGAIN) {
                LOG_ERROR("abort_startup: ckpt unlink retry");
                usleep(1000);
                goto unlink_retry;
        }
	
	if (rv != SA_AIS_OK) {
                LOG_ERROR("[%s] Failed to unlink checkpoint: %s",
                          SHORT_UUID(entry->name.value), str_ais_error(rv));
                return -EIO;
        }

	saCkptCheckpointClose(h);

	return 1;
}

int create_cluster_cpg(char *str)
{
	int r;
	int size;
	struct clog_cpg *new = NULL;
	struct clog_cpg *tmp, *tmp2;

	list_for_each_entry_safe(tmp, tmp2, &clog_cpg_list, list)
		if (!strncmp(tmp->name.value, str, CPG_MAX_NAME_LENGTH)) {
			LOG_ERROR("Log entry already exists: %s", str);
			return -EEXIST;
		}

	new = malloc(sizeof(*new));
	if (!new) {
		LOG_ERROR("Unable to allocate memory for clog_cpg");
		return -ENOMEM;
	}
	memset(new, 0, sizeof(*new));
	INIT_LIST_HEAD(&new->list);
	new->lowest_id = 0xDEAD;
	INIT_LIST_HEAD(&new->startup_list);
	INIT_LIST_HEAD(&new->working_list);

	size = ((strlen(str) + 1) > CPG_MAX_NAME_LENGTH) ?
		CPG_MAX_NAME_LENGTH : (strlen(str) + 1);
	strncpy(new->name.value, str, size);
	new->name.length = size;

	/*
	 * Look for checkpoints before joining to see if
	 * someone wrote a checkpoint after I left a previous
	 * session.
	 */
	if (remove_checkpoint(new) == 1)
		LOG_COND(log_checkpoint,
			 "[%s]  Removing checkpoints left from previous session",
			 SHORT_UUID(new->name.value));

	r = cpg_initialize(&new->handle, &cpg_callbacks);
	if (r != SA_AIS_OK) {
		LOG_ERROR("cpg_initialize failed:  Cannot join cluster");
		free(new);
		return -EPERM;
	}

	r = cpg_join(new->handle, &new->name);
	if (r != SA_AIS_OK) {
		LOG_ERROR("cpg_join failed:  Cannot join cluster");
		free(new);
		return -EPERM;
	}

	new->cpg_state = VALID;
	list_add(&new->list, &clog_cpg_list);
	LOG_DBG("New   handle: %llu", (unsigned long long)new->handle);
	LOG_DBG("New   name: %s", new->name.value);

	/* FIXME: better variable */
	cpg_fd_get(new->handle, &r);
	links_register(r, "cluster", do_cluster_work, NULL);

	return 0;
}

static void abort_startup(struct clog_cpg *del)
{
	struct list_head *p, *n;
	struct clog_tfr *tfr = NULL;

	LOG_DBG("[%s]  CPG teardown before checkpoint received",
		SHORT_UUID(del->name.value));

	list_for_each_safe(p, n, &del->startup_list) {
		list_del_init(p);
		tfr = (struct clog_tfr *)p;
		LOG_DBG("[%s]  Ignoring request from %u: %s",
			SHORT_UUID(del->name.value), tfr->originator,
			_RQ_TYPE(tfr->request_type));
		free(tfr);
	}

	remove_checkpoint(del);
}

static int _destroy_cluster_cpg(struct clog_cpg *del)
{
	int r;
	
	LOG_COND(log_resend_requests, "[%s] I am leaving.2.....",
		 SHORT_UUID(del->name.value));

	/*
	 * We must send any left over checkpoints before
	 * leaving.  If we don't, an incoming node could
	 * be stuck with no checkpoint and stall.
	 */
	do_checkpoints(del);

	del->cpg_state = INVALID;
	del->state = LEAVING;

	if (!list_empty(&del->startup_list))
		abort_startup(del);

	r = cpg_leave(del->handle, &del->name);
	if (r != CPG_OK)
		LOG_ERROR("Error leaving CPG!");
	return 0;
}

int destroy_cluster_cpg(char *str)
{
	struct clog_cpg *del, *tmp;

	list_for_each_entry_safe(del, tmp, &clog_cpg_list, list)
		if (!strncmp(del->name.value, str, CPG_MAX_NAME_LENGTH))
			_destroy_cluster_cpg(del);

	return 0;
}

int init_cluster(void)
{
	SaAisErrorT rv;

	{
		int i;
		for (i = 0; i < DEBUGGING_HISTORY; i++)
			debugging[i][0] = '\0';
	}

	INIT_LIST_HEAD(&clog_cpg_list);
	rv = saCkptInitialize(&ckpt_handle, &callbacks, &version);

	if (rv != SA_AIS_OK)
		return EXIT_CLUSTER_CKPT_INIT;

	return 0;
}

void cleanup_cluster(void)
{
	SaAisErrorT err;

	err = saCkptFinalize(ckpt_handle);
	if (err != SA_AIS_OK)
		LOG_ERROR("Failed to finalize checkpoint handle");
}

void cluster_debug(void)
{
	struct checkpoint_data *cp;
	struct clog_cpg *entry, *tmp;
	struct list_head *p, *n;
	struct clog_tfr *t;
	int i;

	LOG_ERROR("");
	LOG_ERROR("CLUSTER COMPONENT DEBUGGING::");
	list_for_each_entry_safe(entry, tmp, &clog_cpg_list, list) {
		LOG_ERROR("%s::", SHORT_UUID(entry->name.value));
		LOG_ERROR("  lowest_id         : %u", entry->lowest_id);
		LOG_ERROR("  state             : %s", (entry->state == INVALID) ?
			  "INVALID" : (entry->state == VALID) ? "VALID" :
			  (entry->state == LEAVING) ? "LEAVING" : "UNKNOWN");
		LOG_ERROR("  cpg_state         : %d", entry->cpg_state);
		LOG_ERROR("  free_me           : %d", entry->free_me);
		LOG_ERROR("  delay             : %d", entry->delay);
		LOG_ERROR("  resend_requests   : %d", entry->resend_requests);
		LOG_ERROR("  checkpoints_needed: %d", entry->checkpoints_needed);
		for (i = 0, cp = entry->checkpoint_list;
		     i < MAX_CHECKPOINT_REQUESTERS; i++)
			if (cp)
				cp = cp->next;
			else
				break;
		LOG_ERROR("  CKPTs waiting     : %d", i);
		LOG_ERROR("  Working list:");
		list_for_each_safe(p, n, &entry->working_list) {
			t = (struct clog_tfr *)p;
			LOG_ERROR("  %s/%u", _RQ_TYPE(t->request_type), t->seq);
		}

		LOG_ERROR("  Startup list:");
		list_for_each_safe(p, n, &entry->startup_list) {
			t = (struct clog_tfr *)p;
			LOG_ERROR("  %s/%u", _RQ_TYPE(t->request_type), t->seq);
		}
	}

	LOG_ERROR("Command History:");
	for (i = 0; i < DEBUGGING_HISTORY; i++) {
		idx++;
		idx = idx % DEBUGGING_HISTORY;
		if (debugging[idx][0] == '\0')
			continue;
		LOG_ERROR("%d:%d) %s", i, idx, debugging[idx]);
	}
}
