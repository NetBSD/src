#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <string.h>
#include <syslog.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include "bl.h"
#include "internal.h"

static int
bl_init(bl_t b)
{
	/* AF_UNIX address of local logger */
	static const struct sockaddr_un sun = {
		.sun_family = AF_LOCAL,
		.sun_len = sizeof(sun),
		.sun_path = _PATH_BLACKLIST,
	};

	if (b->b_fd == -1) {
		b->b_fd = socket(PF_LOCAL,
		    SOCK_DGRAM|SOCK_CLOEXEC|SOCK_NONBLOCK|SOCK_NOSIGPIPE, 0);
		if (b->b_fd == -1) {
			syslog(LOG_ERR, "%s: socket failed (%m)", __func__);
			return 0;
		}
	}

	if (b->b_connected)
		return 0;

	if (connect(b->b_fd, (const void *)&sun,
	    (socklen_t)sizeof(sun)) == -1) {
		syslog(LOG_ERR, "%s: connect failed (%m)", __func__);
		return -1;
	}
	b->b_connected = 1;
	return 0;
}

bl_t
bl_create(void)
{
	bl_t b = malloc(sizeof(*b));
	if (b == NULL) {
		syslog(LOG_ERR, "%s: malloc failed (%m)", __func__);
		return NULL;
	}
	bl_init(b);
	return b;
}

void
bl_destroy(bl_t b)
{
	close(b->b_fd);
	free(b);
}

#if 0
static int
bl_post(bl_t b, const struct sockaddr_storage *lss,
    const struct sockaddr_storage *pss, bl_event_t e, const char *ctx)
{

	struct sockaddr_storage lss, pss;
	socklen_t lsl, psl;

	lsl = sizeof(lss);
	psl = sizeof(pss);
	if (getsockname(lfd, &lss, &lsl) == -1) {
		syslog(LOG_ERR, "%s: getsockname failed (%m)", __func__);
		return -1;
	}
	if (getpeername(pfd, &pss, &psl) == -1) {
		syslog(LOG_ERR, "%s: getpeername failed (%m)", __func__);
		return -1;
	}
	return bl_post(&lss, &pss, e, ctx);
}
#endif

int
bl_add(bl_t b, bl_type_t e, int lfd, int pfd, const char *ctx)
{
	struct msghdr   msg;
	struct iovec    iov;
	union {
		char ctrl[CMSG_SPACE(2 * sizeof(int))];
		uint32_t fd[2];
	} uc;
	struct cmsghdr *cmsg;
	union {
		bl_message_t bl;
		char buf[512];
	} ub;
	int *fd;
	size_t ctxlen;

	ctxlen = strlen(ctx);
	if (ctxlen > 256)
		ctxlen = 256;

	iov.iov_base = ub.buf;
	iov.iov_len = sizeof(bl_message_t) + ctxlen;
	ub.bl.bl_len = iov.iov_len;
	ub.bl.bl_version = BL_VERSION;
	ub.bl.bl_type = (uint32_t)e;
	memcpy(ub.bl.bl_data, ctx, ctxlen);

	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	msg.msg_control = uc.ctrl;
	msg.msg_controllen = sizeof(uc.ctrl);

	cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_len = CMSG_LEN(2 * sizeof(int));
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;

	fd = (void *)CMSG_DATA(cmsg);
	fd[0] = lfd;
	fd[1] = pfd;

	if (bl_init(b) == -1)
		return -1;

	return sendmsg(b->b_fd, &msg, 0);
}
