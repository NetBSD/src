/*	$NetBSD: local.c,v 1.1.1.1 2009/12/02 00:27:10 haad Exp $	*/

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
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <linux/connector.h>
#include <linux/netlink.h>

#include "dm-log-userspace.h"
#include "functions.h"
#include "cluster.h"
#include "common.h"
#include "logging.h"
#include "link_mon.h"
#include "local.h"

#ifndef CN_IDX_DM
#warning Kernel should be at least 2.6.31
#define CN_IDX_DM                       0x7     /* Device Mapper */
#define CN_VAL_DM_USERSPACE_LOG         0x1
#endif

static int cn_fd;  /* Connector (netlink) socket fd */
static char recv_buf[2048];
static char send_buf[2048];


/* FIXME: merge this function with kernel_send_helper */
static int kernel_ack(uint32_t seq, int error)
{
	int r;
	struct nlmsghdr *nlh = (struct nlmsghdr *)send_buf;
	struct cn_msg *msg = NLMSG_DATA(nlh);

	if (error < 0) {
		LOG_ERROR("Programmer error: error codes must be positive");
		return -EINVAL;
	}

	memset(send_buf, 0, sizeof(send_buf));

	nlh->nlmsg_seq = 0;
	nlh->nlmsg_pid = getpid();
	nlh->nlmsg_type = NLMSG_DONE;
	nlh->nlmsg_len = NLMSG_LENGTH(sizeof(struct cn_msg));
	nlh->nlmsg_flags = 0;

	msg->len = 0;
	msg->id.idx = CN_IDX_DM;
	msg->id.val = CN_VAL_DM_USERSPACE_LOG;
	msg->seq = seq;
	msg->ack = error;

	r = send(cn_fd, nlh, NLMSG_LENGTH(sizeof(struct cn_msg)), 0);
	/* FIXME: do better error processing */
	if (r <= 0)
		return -EBADE;

	return 0;
}


/*
 * kernel_recv
 * @rq: the newly allocated request from kernel
 *
 * Read requests from the kernel and allocate space for the new request.
 * If there is no request from the kernel, *rq is NULL.
 *
 * This function is not thread safe due to returned stack pointer.  In fact,
 * the returned pointer must not be in-use when this function is called again.
 *
 * Returns: 0 on success, -EXXX on error
 */
static int kernel_recv(struct clog_request **rq)
{
	int r = 0;
	int len;
	struct cn_msg *msg;
	struct dm_ulog_request *u_rq;

	*rq = NULL;
	memset(recv_buf, 0, sizeof(recv_buf));

	len = recv(cn_fd, recv_buf, sizeof(recv_buf), 0);
	if (len < 0) {
		LOG_ERROR("Failed to recv message from kernel");
		r = -errno;
		goto fail;
	}

	switch (((struct nlmsghdr *)recv_buf)->nlmsg_type) {
	case NLMSG_ERROR:
		LOG_ERROR("Unable to recv message from kernel: NLMSG_ERROR");
		r = -EBADE;
		goto fail;
	case NLMSG_DONE:
		msg = (struct cn_msg *)NLMSG_DATA((struct nlmsghdr *)recv_buf);
		len -= sizeof(struct nlmsghdr);

		if (len < sizeof(struct cn_msg)) {
			LOG_ERROR("Incomplete request from kernel received");
			r = -EBADE;
			goto fail;
		}

		if (msg->len > DM_ULOG_REQUEST_SIZE) {
			LOG_ERROR("Not enough space to receive kernel request (%d/%d)",
				  msg->len, DM_ULOG_REQUEST_SIZE);
			r = -EBADE;
			goto fail;
		}

		if (!msg->len)
			LOG_ERROR("Zero length message received");

		len -= sizeof(struct cn_msg);

		if (len < msg->len)
			LOG_ERROR("len = %d, msg->len = %d", len, msg->len);

		msg->data[msg->len] = '\0'; /* Cleaner way to ensure this? */
		u_rq = (struct dm_ulog_request *)msg->data;

		if (!u_rq->request_type) {
			LOG_DBG("Bad transmission, requesting resend [%u]",
				msg->seq);
			r = -EAGAIN;

			if (kernel_ack(msg->seq, EAGAIN)) {
				LOG_ERROR("Failed to NACK kernel transmission [%u]",
					  msg->seq);
				r = -EBADE;
			}
		}

		/*
		 * Now we've got sizeof(struct cn_msg) + sizeof(struct nlmsghdr)
		 * worth of space that precede the request structure from the
		 * kernel.  Since that space isn't going to be used again, we
		 * can take it for our purposes; rather than allocating a whole
		 * new structure and doing a memcpy.
		 *
		 * We should really make sure 'clog_request' doesn't grow
		 * beyond what is available to us, but we need only check it
		 * once... perhaps at compile time?
		 */
//		*rq = container_of(u_rq, struct clog_request, u_rq);
		*rq = (void *)u_rq -
			(sizeof(struct clog_request) -
			 sizeof(struct dm_ulog_request));

		/* Clear the wrapper container fields */
		memset(*rq, 0, (void *)u_rq - (void *)(*rq));
		break;
	default:
		LOG_ERROR("Unknown nlmsg_type");
		r = -EBADE;
	}

fail:
	if (r)
		*rq = NULL;

	return (r == -EAGAIN) ? 0 : r;
}

static int kernel_send_helper(void *data, int out_size)
{
	int r;
	struct nlmsghdr *nlh;
	struct cn_msg *msg;

	memset(send_buf, 0, sizeof(send_buf));

	nlh = (struct nlmsghdr *)send_buf;
	nlh->nlmsg_seq = 0;  /* FIXME: Is this used? */
	nlh->nlmsg_pid = getpid();
	nlh->nlmsg_type = NLMSG_DONE;
	nlh->nlmsg_len = NLMSG_LENGTH(out_size + sizeof(struct cn_msg));
	nlh->nlmsg_flags = 0;

	msg = NLMSG_DATA(nlh);
	memcpy(msg->data, data, out_size);
	msg->len = out_size;
	msg->id.idx = CN_IDX_DM;
	msg->id.val = CN_VAL_DM_USERSPACE_LOG;
	msg->seq = 0;

	r = send(cn_fd, nlh, NLMSG_LENGTH(out_size + sizeof(struct cn_msg)), 0);
	/* FIXME: do better error processing */
	if (r <= 0)
		return -EBADE;

	return 0;
}

/*
 * do_local_work
 *
 * Any processing errors are placed in the 'rq'
 * structure to be reported back to the kernel.
 * It may be pointless for this function to
 * return an int.
 *
 * Returns: 0 on success, -EXXX on failure
 */
static int do_local_work(void *data)
{
	int r;
	struct clog_request *rq;
	struct dm_ulog_request *u_rq = NULL;

	r = kernel_recv(&rq);
	if (r)
		return r;

	if (!rq)
		return 0;

	u_rq = &rq->u_rq;
	LOG_DBG("[%s]  Request from kernel received: [%s/%u]",
		SHORT_UUID(u_rq->uuid), RQ_TYPE(u_rq->request_type),
		u_rq->seq);
	switch (u_rq->request_type) {
	case DM_ULOG_CTR:
	case DM_ULOG_DTR:
	case DM_ULOG_GET_REGION_SIZE:
	case DM_ULOG_IN_SYNC:
	case DM_ULOG_GET_SYNC_COUNT:
	case DM_ULOG_STATUS_INFO:
	case DM_ULOG_STATUS_TABLE:
	case DM_ULOG_PRESUSPEND:
		/* We do not specify ourselves as server here */
		r = do_request(rq, 0);
		if (r)
			LOG_DBG("Returning failed request to kernel [%s]",
				RQ_TYPE(u_rq->request_type));
		r = kernel_send(u_rq);
		if (r)
			LOG_ERROR("Failed to respond to kernel [%s]",
				  RQ_TYPE(u_rq->request_type));
			
		break;
	case DM_ULOG_RESUME:
		/*
		 * Resume is a special case that requires a local
		 * component to join the CPG, and a cluster component
		 * to handle the request.
		 */
		r = local_resume(u_rq);
		if (r) {
			LOG_DBG("Returning failed request to kernel [%s]",
				RQ_TYPE(u_rq->request_type));
			r = kernel_send(u_rq);
			if (r)
				LOG_ERROR("Failed to respond to kernel [%s]",
					  RQ_TYPE(u_rq->request_type));
			break;
		}
		/* ELSE, fall through */
	case DM_ULOG_IS_CLEAN:
	case DM_ULOG_FLUSH:
	case DM_ULOG_MARK_REGION:
	case DM_ULOG_GET_RESYNC_WORK:
	case DM_ULOG_SET_REGION_SYNC:
	case DM_ULOG_IS_REMOTE_RECOVERING:
	case DM_ULOG_POSTSUSPEND:
		r = cluster_send(rq);
		if (r) {
			u_rq->data_size = 0;
			u_rq->error = r;
			kernel_send(u_rq);
		}

		break;
	case DM_ULOG_CLEAR_REGION:
		r = kernel_ack(u_rq->seq, 0);

		r = cluster_send(rq);
		if (r) {
			/*
			 * FIXME: store error for delivery on flush
			 *        This would allow us to optimize MARK_REGION
			 *        too.
			 */
		}

		break;
	default:
		LOG_ERROR("Invalid log request received (%u), ignoring.",
			  u_rq->request_type);

		return 0;
	}

	if (r && !u_rq->error)
		u_rq->error = r;

	return r;
}

/*
 * kernel_send
 * @u_rq: result to pass back to kernel
 *
 * This function returns the u_rq structure
 * (containing the results) to the kernel.
 * It then frees the structure.
 *
 * WARNING: should the structure be freed if
 * there is an error?  I vote 'yes'.  If the
 * kernel doesn't get the response, it should
 * resend the request.
 *
 * Returns: 0 on success, -EXXX on failure
 */
int kernel_send(struct dm_ulog_request *u_rq)
{
	int r;
	int size;

	if (!u_rq)
		return -EINVAL;

	size = sizeof(struct dm_ulog_request) + u_rq->data_size;

	if (!u_rq->data_size && !u_rq->error) {
		/* An ACK is all that is needed */

		/* FIXME: add ACK code */
	} else if (size > DM_ULOG_REQUEST_SIZE) {
		/*
		 * If we gotten here, we've already overrun
		 * our allotted space somewhere.
		 *
		 * We must do something, because the kernel
		 * is waiting for a response.
		 */
		LOG_ERROR("Not enough space to respond to server");
		u_rq->error = -ENOSPC;
		size = sizeof(struct dm_ulog_request);
	}

	r = kernel_send_helper(u_rq, size);
	if (r)
		LOG_ERROR("Failed to send msg to kernel.");

	return r;
}

/*
 * init_local
 *
 * Initialize kernel communication socket (netlink)
 *
 * Returns: 0 on success, values from common.h on failure
 */
int init_local(void)
{
	int r = 0;
	int opt;
	struct sockaddr_nl addr;

	cn_fd = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_CONNECTOR);
	if (cn_fd < 0)
		return EXIT_KERNEL_SOCKET;

	/* memset to fix valgrind complaint */
	memset(&addr, 0, sizeof(struct sockaddr_nl));

	addr.nl_family = AF_NETLINK;
	addr.nl_groups = CN_IDX_DM;
	addr.nl_pid = 0;

	r = bind(cn_fd, (struct sockaddr *) &addr, sizeof(addr));
	if (r < 0) {
		close(cn_fd);
		return EXIT_KERNEL_BIND;
	}

	opt = addr.nl_groups;
	r = setsockopt(cn_fd, 270, NETLINK_ADD_MEMBERSHIP, &opt, sizeof(opt));
	if (r) {
		close(cn_fd);
		return EXIT_KERNEL_SETSOCKOPT;
	}

	/*
	r = fcntl(cn_fd, F_SETFL, FNDELAY);
	*/

	links_register(cn_fd, "local", do_local_work, NULL);

	return 0;
}

/*
 * cleanup_local
 *
 * Clean up before exiting
 */
void cleanup_local(void)
{
	links_unregister(cn_fd);
	close(cn_fd);
}
