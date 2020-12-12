/*
 * Copyright (c) 2020 Yubico AB. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 */

#include <sys/types.h>
#include <sys/ioctl.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbhid.h>

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <usbhid.h>

#include "fido.h"

#define MAX_UHID	64

struct hid_netbsd {
	int	fd;
	size_t	report_in_len;
	size_t	report_out_len;
};

/* Hack to make this work with newer kernels even if /usr/include is old.  */
#if __NetBSD_Version__ < 901000000	/* 9.1 */
#define	USB_HID_GET_RAW	_IOR('h', 1, int)
#define	USB_HID_SET_RAW	_IOW('h', 2, int)
#endif

static bool
is_fido(int fd)
{
	report_desc_t			rdesc;
	hid_data_t			hdata;
	hid_item_t			hitem;
	bool				isfido;
	int				raw = 1;

	if ((rdesc = hid_get_report_desc(fd)) == NULL) {
		fido_log_debug("%s: failed to get report descriptor",
		    __func__);
		return (false);
	}
	if ((hdata = hid_start_parse(rdesc, 1 << hid_collection, -1))
	    == NULL) {
		fido_log_debug("%s: failed to parse report descriptor",
		    __func__);
		hid_dispose_report_desc(rdesc);
		return (false);
	}
	isfido = false;
	while ((hid_get_item(hdata, &hitem)) > 0) {
		if (HID_PAGE(hitem.usage) == 0xf1d0) {
			isfido = true;
			break;
		}
	}
	hid_end_parse(hdata);
	hid_dispose_report_desc(rdesc);
	if (!isfido)
		return (false);

        /*
	 * This step is not strictly necessary -- NetBSD puts fido
         * devices into raw mode automatically by default, but in
         * principle that might change, and this serves as a test to
         * verify that we're running on a kernel with support for raw
         * mode at all so we don't get confused issuing writes that try
         * to set the report descriptor rather than transfer data on
         * the output interrupt pipe as we need.
	 */
	if (ioctl(fd, USB_HID_SET_RAW, &raw) == -1) {
		fido_log_debug("%s: unable to set raw", __func__);
		return (false);
	}

	return (true);
}

static int
copy_info(fido_dev_info_t *di, const char *path)
{
	int			fd = -1;
	int			ok = -1;
	struct usb_device_info	udi;

	memset(di, 0, sizeof(*di));
	memset(&udi, 0, sizeof(udi));

	if ((fd = open(path, O_RDWR)) == -1) {
		if (errno != EBUSY && errno != ENOENT)
			fido_log_debug("%s: open %s: %s", __func__, path,
			    strerror(errno));
		goto fail;
	}
	if (!is_fido(fd))
		goto fail;

	if (ioctl(fd, USB_GET_DEVICEINFO, &udi) == -1)
		goto fail;

	if ((di->path = strdup(path)) == NULL ||
	    (di->manufacturer = strdup(udi.udi_vendor)) == NULL ||
	    (di->product = strdup(udi.udi_product)) == NULL)
		goto fail;

	di->vendor_id = (int16_t)udi.udi_vendorNo;
	di->product_id = (int16_t)udi.udi_productNo;

	ok = 0;
fail:
	if (fd != -1)
		close(fd);

	if (ok < 0) {
		free(di->path);
		free(di->manufacturer);
		free(di->product);
		explicit_bzero(di, sizeof(*di));
	}

	return (ok);
}

int
fido_hid_manifest(fido_dev_info_t *devlist, size_t ilen, size_t *olen)
{
	char	path[64];
	size_t	i;

	*olen = 0;

	if (ilen == 0)
		return (FIDO_OK); /* nothing to do */

	if (devlist == NULL || olen == NULL)
		return (FIDO_ERR_INVALID_ARGUMENT);

	for (i = *olen = 0; i < MAX_UHID && *olen < ilen; i++) {
		snprintf(path, sizeof(path), "/dev/uhid%zu", i);
		if (copy_info(&devlist[*olen], path) == 0) {
			devlist[*olen].io = (fido_dev_io_t) {
				fido_hid_open,
				fido_hid_close,
				fido_hid_read,
				fido_hid_write,
			};
			++(*olen);
		}
	}

	return (FIDO_OK);
}

/*
 * Workaround for NetBSD (as of 201910) bug that loses
 * sync of DATA0/DATA1 sequence bit across uhid open/close.
 * Send pings until we get a response - early pings with incorrect
 * sequence bits will be ignored as duplicate packets by the device.
 */
static int
terrible_ping_kludge(struct hid_netbsd *ctx)
{
	u_char data[256];
	int i, n;
	struct pollfd pfd;

	if (sizeof(data) < ctx->report_out_len + 1)
		return -1;
	for (i = 0; i < 4; i++) {
		memset(data, 0, sizeof(data));
		/* broadcast channel ID */
		data[1] = 0xff;
		data[2] = 0xff;
		data[3] = 0xff;
		data[4] = 0xff;
		/* Ping command */
		data[5] = 0x81;
		/* One byte ping only, Vasili */
		data[6] = 0;
		data[7] = 1;
		fido_log_debug("%s: send ping %d", __func__, i);
		if (fido_hid_write(ctx, data, ctx->report_out_len + 1) == -1)
			return -1;
		fido_log_debug("%s: wait reply", __func__);
		memset(&pfd, 0, sizeof(pfd));
		pfd.fd = ctx->fd;
		pfd.events = POLLIN;
		if ((n = poll(&pfd, 1, 100)) == -1) {
			fido_log_debug("%s: poll: %d", __func__, errno);
			return -1;
		} else if (n == 0) {
			fido_log_debug("%s: timed out", __func__);
			continue;
		}
		if (fido_hid_read(ctx, data, ctx->report_out_len, 250) == -1)
			return -1;
		/*
		 * Ping isn't always supported on the broadcast channel,
		 * so we might get an error, but we don't care - we're
		 * synched now.
		 */
		fido_log_debug("%s: got reply", __func__);
		fido_log_xxd(data, ctx->report_out_len);
		return 0;
	}
	fido_log_debug("%s: no response", __func__);
	return -1;
}

void *
fido_hid_open(const char *path)
{
	struct hid_netbsd		*ctx;
	report_desc_t			rdesc = NULL;
	hid_data_t			hdata;
	int				len, report_id = 0;

	if ((ctx = calloc(1, sizeof(*ctx))) == NULL)
		goto fail0;
	if ((ctx->fd = open(path, O_RDWR)) == -1)
		goto fail1;
	if (ioctl(ctx->fd, USB_GET_REPORT_ID, &report_id) == -1) {
		fido_log_debug("%s: failed to get report ID: %s", __func__,
		    strerror(errno));
		goto fail2;
	}
	if ((rdesc = hid_get_report_desc(ctx->fd)) == NULL) {
		fido_log_debug("%s: failed to get report descriptor",
		    __func__);
		goto fail2;
	}
	if ((hdata = hid_start_parse(rdesc, 1 << hid_collection, -1))
	    == NULL) {
		fido_log_debug("%s: failed to parse report descriptor",
		    __func__);
		goto fail3;
	}
	if ((len = hid_report_size(rdesc, hid_input, report_id)) <= 0 ||
	    (size_t)len > CTAP_MAX_REPORT_LEN) {
		fido_log_debug("%s: bad input report size %d", __func__, len);
		goto fail3;
	}
	ctx->report_in_len = (size_t)len;
	if ((len = hid_report_size(rdesc, hid_output, report_id)) <= 0 ||
	    (size_t)len > CTAP_MAX_REPORT_LEN) {
		fido_log_debug("%s: bad output report size %d", __func__, len);
		goto fail3;
	}
	ctx->report_out_len = (size_t)len;
	hid_dispose_report_desc(rdesc);

	/*
	 * NetBSD has a bug that causes it to lose
	 * track of the DATA0/DATA1 sequence toggle across uhid device
	 * open and close. This is a terrible hack to work around it.
	 */
	if (!is_fido(ctx->fd) || terrible_ping_kludge(ctx) != 0)
		goto fail2;

	return (ctx);

fail3:	hid_dispose_report_desc(rdesc);
fail2:	close(ctx->fd);
fail1:	free(ctx);
fail0:	return (NULL);
}

void
fido_hid_close(void *handle)
{
	struct hid_netbsd *ctx = handle;

	close(ctx->fd);
	free(ctx);
}

static void
xstrerror(int errnum, char *buf, size_t len)
{
	if (len < 1)
		return;

	memset(buf, 0, len);

	if (strerror_r(errnum, buf, len - 1) != 0)
		snprintf(buf, len - 1, "error %d", errnum);
}

static int
timespec_to_ms(const struct timespec *ts, int upper_bound)
{
	int64_t x;
	int64_t y;

	if (ts->tv_sec < 0 || (uint64_t)ts->tv_sec > INT64_MAX / 1000LL ||
	    ts->tv_nsec < 0 || (uint64_t)ts->tv_nsec / 1000000LL > INT64_MAX)
		return (upper_bound);

	x = ts->tv_sec * 1000LL;
	y = ts->tv_nsec / 1000000LL;

	if (INT64_MAX - x < y || x + y > upper_bound)
		return (upper_bound);

	return (int)(x + y);
}

static int
fido_hid_unix_wait(int fd, int ms)
{
	char		ebuf[128];
	struct timespec	ts_start;
	struct timespec	ts_now;
	struct timespec	ts_delta;
	struct pollfd	pfd;
	int		ms_remain;
	int		r;

	if (ms < 0)
		return (0);

	memset(&pfd, 0, sizeof(pfd));
	pfd.events = POLLIN;
	pfd.fd = fd;

	if (clock_gettime(CLOCK_MONOTONIC, &ts_start) != 0) {
		xstrerror(errno, ebuf, sizeof(ebuf));
		fido_log_debug("%s: clock_gettime: %s", __func__, ebuf);
		return (-1);
	}

	for (ms_remain = ms; ms_remain > 0;) {
		if ((r = poll(&pfd, 1, ms_remain)) > 0)
			return (0);
		else if (r == 0)
			break;
		else if (errno != EINTR) {
			xstrerror(errno, ebuf, sizeof(ebuf));
			fido_log_debug("%s: poll: %s", __func__, ebuf);
			return (-1);
		}
		/* poll interrupted - subtract time already waited */
		if (clock_gettime(CLOCK_MONOTONIC, &ts_now) != 0) {
			xstrerror(errno, ebuf, sizeof(ebuf));
			fido_log_debug("%s: clock_gettime: %s", __func__, ebuf);
			return (-1);
		}
		timespecsub(&ts_now, &ts_start, &ts_delta);
		ms_remain = ms - timespec_to_ms(&ts_delta, ms);
	}

	return (-1);
}

int
fido_hid_read(void *handle, unsigned char *buf, size_t len, int ms)
{
	struct hid_netbsd	*ctx = handle;
	ssize_t			 r;

	if (len != ctx->report_in_len) {
		fido_log_debug("%s: len %zu", __func__, len);
		return (-1);
	}

	if (fido_hid_unix_wait(ctx->fd, ms) < 0) {
		fido_log_debug("%s: fd not ready", __func__);
		return (-1);
	}

	if ((r = read(ctx->fd, buf, len)) == -1 || (size_t)r != len) {
		fido_log_debug("%s: read", __func__);
		return (-1);
	}

	return ((int)r);
}

int
fido_hid_write(void *handle, const unsigned char *buf, size_t len)
{
	struct hid_netbsd	*ctx = handle;
	ssize_t			 r;

	if (len != ctx->report_out_len + 1) {
		fido_log_debug("%s: len %zu", __func__, len);
		return (-1);
	}

	if ((r = write(ctx->fd, buf + 1, len - 1)) == -1 ||
	    (size_t)r != len - 1) {
		fido_log_debug("%s: write", __func__);
		return (-1);
	}

	return ((int)len);
}

size_t
fido_hid_report_in_len(void *handle)
{
	struct hid_netbsd *ctx = handle;

	return (ctx->report_in_len);
}

size_t
fido_hid_report_out_len(void *handle)
{
	struct hid_netbsd *ctx = handle;

	return (ctx->report_out_len);
}
