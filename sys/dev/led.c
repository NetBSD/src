/* $NetBSD: led.c,v 1.3 2018/01/10 15:58:40 jakllsch Exp $ */

/*-
 * Copyright (c) 2017 Jared McNeill <jmcneill@invisible.ca>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: led.c,v 1.3 2018/01/10 15:58:40 jakllsch Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/queue.h>
#include <sys/sysctl.h>
#include <sys/once.h>

#include <dev/led.h>

struct led_device {
	char 			*name;
	led_getstate_fn		getstate;
	led_setstate_fn		setstate;
	void			*priv;

	struct sysctllog	*slog;

	TAILQ_ENTRY(led_device)	devices;
};

static TAILQ_HEAD(, led_device) led_devices =
    TAILQ_HEAD_INITIALIZER(led_devices);
static kmutex_t led_lock;

static int
led_init(void)
{
	mutex_init(&led_lock, MUTEX_DEFAULT, IPL_NONE);

	return 0;
}

static struct led_device *
led_lookup(const char *name)
{
	struct led_device *led;

	KASSERT(mutex_owned(&led_lock));

	TAILQ_FOREACH(led, &led_devices, devices)
		if (strcmp(led->name, name) == 0)
			return led;

	return NULL;
}

static void
led_normalize_name(char *name)
{
	unsigned char *p;

	for (p = (unsigned char *)name; *p; p++)
		if (!isalpha(*p) && !isdigit(*p) && *p != '-' && *p != '_')
			*p = '_';
}

static void
led_free(struct led_device *led)
{
	KASSERT(mutex_owned(&led_lock));

	kmem_free(led->name, strlen(led->name) + 1);
	kmem_free(led, sizeof(*led));
}

static int
led_sysctl_handler(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct led_device *led;
	int error;
	bool state;

	mutex_enter(&led_lock);

	node = *rnode;
	led = node.sysctl_data;
	state = led->getstate(led->priv);
	node.sysctl_data = &state;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL) {
		mutex_exit(&led_lock);
		return error;
	}

	led->setstate(led->priv, state);

	mutex_exit(&led_lock);

	return 0;
}

void *
led_attach(const char *name, void *priv, led_getstate_fn getstate,
    led_setstate_fn setstate)
{
	static ONCE_DECL(control);
	struct led_device *led;
	const struct sysctlnode *node;
	int error;

	if (RUN_ONCE(&control, led_init) != 0)
		return NULL;

	led = kmem_zalloc(sizeof(*led), KM_SLEEP);
	led->name = kmem_asprintf("%s", name);
	led->getstate = getstate;
	led->setstate = setstate;
	led->priv = priv;

	/* Convert invalid sysctl node name characters to underscores */
	led_normalize_name(led->name);

	mutex_enter(&led_lock);
	if (led_lookup(name) != NULL) {
		led_free(led);
		led = NULL;
	} else {
		error = sysctl_createv(&led->slog, 0, NULL, &node,
		    CTLFLAG_PERMANENT, CTLTYPE_NODE, "led", NULL,
		    NULL, 0, NULL, 0,
		    CTL_HW, CTL_CREATE, CTL_EOL);
		if (error == 0) {
			error = sysctl_createv(&led->slog, 0, &node, NULL,
			    CTLFLAG_READWRITE, CTLTYPE_BOOL, led->name, NULL,
			    led_sysctl_handler, 0,
			    (void *)led, 0,
			    CTL_CREATE, CTL_EOL);
		}
		if (error != 0) {
			printf("led: failed to create sysctl hw.led.%s: %d\n",
			    led->name, error);
			led_free(led);
			led = NULL;
		}
	}
	if (led != NULL)
		TAILQ_INSERT_TAIL(&led_devices, led, devices);
	mutex_exit(&led_lock);

	return led;
}

void
led_detach(void *handle)
{
	struct led_device *led = handle;

	mutex_enter(&led_lock);

	TAILQ_REMOVE(&led_devices, led, devices);
	sysctl_teardown(&led->slog);
	led_free(led);

	mutex_exit(&led_lock);
}
