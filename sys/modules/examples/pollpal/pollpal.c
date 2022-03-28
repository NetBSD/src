/*	$NetBSD: pollpal.c,v 1.4 2022/03/28 12:33:22 riastradh Exp $	*/ 

/*-
* Copyright (c) 2020 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: pollpal.c,v 1.4 2022/03/28 12:33:22 riastradh Exp $");

#include <sys/module.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/lwp.h>

#include <sys/condvar.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/mutex.h>
#include <sys/kmem.h>
#include <sys/poll.h>
#include <sys/select.h>

/* 
 * Create a device /dev/pollpal
 *
 * To use this device you need to do:
 * 	mknod /dev/pollpal c 351 0
 *
 */ 

dev_type_open(pollpal_open);

static struct cdevsw pollpal_cdevsw = {
	.d_open = pollpal_open,
	.d_close = noclose,
	.d_read = noread,
	.d_write = nowrite,
	.d_ioctl = noioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER
};

static int	pollpal_nopen = 0;

static int	pollpal_close(file_t *);
static int	pollpal_write(file_t *, off_t *, struct uio *, kauth_cred_t,
    int);
static int	pollpal_read(file_t *, off_t *, struct uio *, kauth_cred_t,
    int);
static int	pollpal_poll(file_t *, int);

const struct fileops pollpal_fileops = {
	.fo_read = pollpal_read,
	.fo_write = pollpal_write,
	.fo_ioctl = fbadop_ioctl,
	.fo_fcntl = fnullop_fcntl,
	.fo_poll = pollpal_poll,
	.fo_stat = fbadop_stat,
	.fo_close = pollpal_close,
	.fo_kqfilter = fnullop_kqfilter,
	.fo_restart = fnullop_restart,
};

typedef struct pollpal_softc {
	kmutex_t lock;
	kcondvar_t sc_cv;
	struct selinfo psel;
	char *buf;
	size_t buf_len;
	/* Device can have two states 1.READ_WAITING, 2.WRITE_WAITING. */ 
	enum states {
		READ_WAITING = 0,
		WRITE_WAITING = 1
	} sc_state;
} pal_t;

static int 
check_pal(const char *str, size_t len)
{
	size_t n;
	
	n = 0;
	
	while (n <= len / 2) {
		if (str[n] != str[len - n - 1])
			return 0;

		n++;
	} 
	
	return 1;
} 

int 
pollpal_open(dev_t dev, int flag, int mode, struct lwp *l __unused)
{
	struct file *fp;
	int error, fd;
	pal_t *pl;
	
	error = fd_allocfile(&fp, &fd);
	if (error)
		return error;

	++pollpal_nopen;

	pl = kmem_zalloc(sizeof(*pl), KM_SLEEP);

	pl->sc_state = READ_WAITING;
	mutex_init(&pl->lock, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&pl->sc_cv, "sc_cv");
	selinit(&pl->psel);

	return fd_clone(fp, fd, flag, &pollpal_fileops, pl);
} 

int 
pollpal_close(file_t * fp)
{
	pal_t * pl = fp->f_data;
	KASSERT(pl != NULL);
	
	if (pl->buf != NULL)
		kmem_free(pl->buf, pl->buf_len);
	
	seldestroy(&pl->psel);
	cv_destroy(&pl->sc_cv);
	mutex_destroy(&pl->lock);
	kmem_free(pl, sizeof(*pl));

	--pollpal_nopen;	

	return 0;
} 

/*
 * Device would write only in READ_WAITING state and then update the state to 
 * WRITE_WAITING.
 */ 
int
pollpal_write(file_t *fp, off_t *offset, struct uio *uio, kauth_cred_t cred,
    int flag)
{
	pal_t *pl = fp->f_data;
	int error;
	
	error = 0;
	
	/* To ensure that only single process can write in device at a time. */ 
	mutex_enter(&pl->lock);
	cv_broadcast(&pl->sc_cv);
	switch (pl->sc_state) {
	case READ_WAITING:
		pl->sc_state = WRITE_WAITING;
		selnotify(&pl->psel, POLLOUT | POLLWRNORM, 0);
		while (pl->sc_state == WRITE_WAITING) {
			if (pl->buf) {
				error = cv_wait_sig(&pl->sc_cv, &pl->lock);
				if (error)
					goto ret;

			}

			pl->buf_len = uio->uio_iov->iov_len;
			pl->buf = kmem_alloc(pl->buf_len, KM_SLEEP);
			uiomove(pl->buf, pl->buf_len, uio);
			printf("Use cat to know the result.\n");
			break;
		}
		break;
	case WRITE_WAITING:
		printf("State: WRITE_WAITING\n");
		break;
	}

ret:
	mutex_exit(&pl->lock);
	return error;
} 

/*
 * Device would read only in WRITE_WAITING state and then update the state to 
 * READ_WAITING.
 */ 
int
pollpal_read(file_t *fp, off_t *offset, struct uio *uio, kauth_cred_t cred,
    int flags)
{
	pal_t *pl = fp->f_data;
	int error;
	
	error = 0;
	
	/* To ensure that only single process can read device at a time. */ 
	mutex_enter(&pl->lock);
	cv_broadcast(&pl->sc_cv);
	switch (pl->sc_state) {
	case READ_WAITING:
		printf("State: READ_WAITING\n");
		printf("You need to write something to the module first!\n");
		goto ret;
	case WRITE_WAITING:
		pl->sc_state = READ_WAITING;
		selnotify(&pl->psel, POLLIN | POLLRDNORM, 0);
		while (pl->sc_state == READ_WAITING) {
			if (!pl->buf) {
				error = cv_wait_sig(&pl->sc_cv, &pl->lock);
				if (error)
					goto ret;
				
			}
			printf("The string you entered was: ");
			uiomove(pl->buf, pl->buf_len, uio);
			printf("\n");
			if (check_pal(pl->buf, pl->buf_len))
				printf("String '%s' was a palindrome.\n", 
				    pl->buf);
			else
				printf("String '%s' was not a palindrome.\n", 
				    pl->buf);

			break;
		}
		break;
	} 
	kmem_free(pl->buf, pl->buf_len);
	pl->buf = NULL;

ret:
	mutex_exit(&pl->lock);
	return error;
} 

int 
pollpal_poll(struct file *fp, int events)
{
	pal_t *pl = fp->f_data;
	int revents;
	
	revents = 0;
	
	mutex_enter(&pl->lock);
	switch (pl->sc_state) {
	case READ_WAITING:
		if (events & (POLLOUT | POLLWRNORM)) {
			/* When device is in READ_WAITING state it can write */
			revents |= POLLOUT | POLLWRNORM;
		} else {
			/* Record the request if it wasn't satisfied. */
			selrecord(curlwp, &pl->psel);
		}
		break;
	case WRITE_WAITING:
		if (events & (POLLIN | POLLRDNORM)) {
			/* When device is in WRITE_WAITING state it can read. */
			revents |= POLLIN | POLLRDNORM;
		} else {
			/* Record the request if it wasn't satisfied. */
			selrecord(curlwp, &pl->psel);
		}
		break;
	}

	mutex_exit(&pl->lock);
	return revents;
} 

MODULE(MODULE_CLASS_MISC, pollpal, NULL);

static int
pollpal_modcmd(modcmd_t cmd, void *arg __unused)
{
	int cmajor = 351, bmajor = -1;

	switch (cmd) {
	case MODULE_CMD_INIT:
		if (devsw_attach("pollpal", NULL, &bmajor, &pollpal_cdevsw, 
		    &cmajor))
			return ENXIO;
		return 0;
	case MODULE_CMD_FINI:
		if (pollpal_nopen != 0)
			return EBUSY;
		devsw_detach(NULL, &pollpal_cdevsw);
		return 0;
	default:
		return ENOTTY;
	} 
}
