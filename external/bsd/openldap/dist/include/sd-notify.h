/*	$NetBSD: sd-notify.h,v 1.2 2025/09/05 21:16:19 christos Exp $	*/

/* SPDX-License-Identifier: MIT-0 */
/* Implement the systemd notify protocol without external dependencies.
 * Supports both readiness notification on startup and on reloading,
 * according to the protocol defined at:
 * https://www.freedesktop.org/software/systemd/man/latest/sd_notify.html
 * This protocol is guaranteed to be stable as per:
 * https://systemd.io/PORTABILITY_AND_STABILITY/ */
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

static int sd_notify(int ignore, const char *message) {
    union sockaddr_union {
	struct sockaddr sa;
	struct sockaddr_un sun;
    } socket_addr = {
	.sun.sun_family = AF_UNIX,
    };
    size_t path_length, message_length;
    const char *socket_path;
    int fd = -1;
    int rc = 1;

    socket_path = getenv("NOTIFY_SOCKET");
    if (!socket_path)
	return 0; /* Not running under systemd? Nothing to do */

    if (!message)
	return -EINVAL;

    message_length = strlen(message);
    if (message_length == 0)
	return -EINVAL;

    /* Only AF_UNIX is supported, with path or abstract sockets */
    if (socket_path[0] != '/' && socket_path[0] != '@')
	return -EAFNOSUPPORT;

    path_length = strlen(socket_path);
    /* Ensure there is room for NUL byte */
    if (path_length >= sizeof(socket_addr.sun.sun_path))
	return -E2BIG;

    memcpy(socket_addr.sun.sun_path, socket_path, path_length);

    /* Support for abstract socket */
    if (socket_addr.sun.sun_path[0] == '@')
	socket_addr.sun.sun_path[0] = 0;

    fd = socket(AF_UNIX, SOCK_DGRAM|SOCK_CLOEXEC, 0);
    if (fd < 0)
	return -errno;

    ssize_t written = sendto(fd, message, message_length, 0,
			     &socket_addr.sa, offsetof(struct sockaddr_un, sun_path) + path_length);
    if (written != (ssize_t) message_length)
	rc = written < 0 ? -errno : -EPROTO;

    close(fd);
    return rc;
}
