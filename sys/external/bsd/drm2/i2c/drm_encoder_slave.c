/*	$NetBSD: drm_encoder_slave.c,v 1.1.20.2 2017/12/03 11:37:58 jdolecek Exp $	*/

/*-
 * Copyright (c) 2015 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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
__KERNEL_RCSID(0, "$NetBSD: drm_encoder_slave.c,v 1.1.20.2 2017/12/03 11:37:58 jdolecek Exp $");

#include <sys/types.h>
#include <sys/atomic.h>
#include <sys/errno.h>
#include <sys/kmem.h>
#include <sys/mutex.h>
#include <sys/rbtree.h>
#include <sys/systm.h>

#include <linux/i2c.h>

#include <drm/drm_encoder_slave.h>

static struct {
	kmutex_t	lock;
	rb_tree_t	tree;	/* struct drm_i2c_encoder_driver */
} drm_i2c_encoder_drivers;

static int
compare_i2c_encoders(void *cookie __unused, const void *va, const void *vb)
{
	const struct drm_i2c_encoder_driver *const da = va;
	const struct drm_i2c_encoder_driver *const db = vb;

	return strncmp(da->i2c_driver.driver.name, db->i2c_driver.driver.name,
	    I2C_NAME_SIZE);
}

static int
compare_i2c_encoder_key(void *cookie __unused, const void *n, const void *k)
{
	const struct drm_i2c_encoder_driver *const driver = n;
	const char *const name = k;

	return strncmp(driver->i2c_driver.driver.name, name, I2C_NAME_SIZE);
}

static rb_tree_ops_t drm_i2c_encoder_rb_ops = {
	.rbto_compare_nodes = &compare_i2c_encoders,
	.rbto_compare_key = &compare_i2c_encoder_key,
	.rbto_node_offset = offsetof(struct drm_i2c_encoder_driver, rb_node),
	.rbto_context = NULL,
};

void
drm_i2c_encoders_init(void)
{

	mutex_init(&drm_i2c_encoder_drivers.lock, MUTEX_DEFAULT, IPL_NONE);
	rb_tree_init(&drm_i2c_encoder_drivers.tree, &drm_i2c_encoder_rb_ops);
}

void
drm_i2c_encoders_fini(void)
{

	KASSERT(RB_TREE_MIN(&drm_i2c_encoder_drivers.tree) == NULL);
#if 0				/* XXX no rb_tree_destroy */
	rb_tree_destroy(&drm_i2c_encoder_drivers.tree);
#endif
	mutex_destroy(&drm_i2c_encoder_drivers.lock);
}

int
drm_i2c_encoder_register(struct module *owner __unused,
    struct drm_i2c_encoder_driver *driver)
{
	struct drm_i2c_encoder_driver *collision;
	int ret = 0;

	mutex_enter(&drm_i2c_encoder_drivers.lock);
	collision = rb_tree_insert_node(&drm_i2c_encoder_drivers.tree, driver);
	if (collision != driver)
		ret = -EEXIST;
	mutex_exit(&drm_i2c_encoder_drivers.lock);

	return ret;
}

void
drm_i2c_encoder_unregister(struct drm_i2c_encoder_driver *driver)
{

	/* XXX How to guarantee?  */
	KASSERT(driver->refcnt == 0);

	mutex_enter(&drm_i2c_encoder_drivers.lock);
	rb_tree_remove_node(&drm_i2c_encoder_drivers.tree, driver);
	mutex_exit(&drm_i2c_encoder_drivers.lock);
}

struct drm_i2c_encoder_bus_priv {
	struct i2c_client		*i2c_client;
	struct drm_i2c_encoder_driver	*i2c_driver;
};

int
drm_i2c_encoder_init(struct drm_device *dev, struct drm_encoder_slave *slave,
    struct i2c_adapter *adapter, const struct i2c_board_info *info)
{
	struct drm_i2c_encoder_driver *driver;
	struct drm_i2c_encoder_bus_priv *bus_priv;
	struct i2c_client *client;
	unsigned refcnt;
	int ret;

	bus_priv = kmem_alloc(sizeof(*bus_priv), KM_SLEEP);
	slave->bus_priv = bus_priv;

	mutex_enter(&drm_i2c_encoder_drivers.lock);
	driver = rb_tree_find_node(&drm_i2c_encoder_drivers.tree, info->type);
	mutex_exit(&drm_i2c_encoder_drivers.lock);
	if (driver == NULL) {
		ret = -ENODEV;
		goto fail0;
	}
	do {
		refcnt = driver->refcnt;
		if (refcnt == UINT_MAX) {
			ret = -ENFILE;
			goto fail0;
		}
	} while (atomic_cas_uint(&driver->refcnt, refcnt, refcnt + 1)
	    != refcnt);
	bus_priv->i2c_driver = driver;

	client = i2c_new_device(adapter, info);
	KASSERT(client != NULL);
	bus_priv->i2c_client = client;

	ret = (*driver->encoder_init)(client, dev, slave);
	if (ret)
		goto fail1;

	if (info->platform_data)
		(*slave->slave_funcs->set_config)(&slave->base,
		    info->platform_data);

	/* Success!  */
	return 0;

fail1:	bus_priv->i2c_client = NULL;
	i2c_unregister_device(client);
	bus_priv->i2c_driver = NULL;
	atomic_dec_uint(&driver->refcnt);
fail0:	KASSERT(ret < 0);
	slave->bus_priv = NULL;
	kmem_free(bus_priv, sizeof(*bus_priv));
	return ret;
}

void
drm_i2c_encoder_destroy(struct drm_encoder *encoder)
{
	struct drm_encoder_slave *const slave = to_encoder_slave(encoder);
	struct drm_i2c_encoder_bus_priv *const bus_priv = slave->bus_priv;
	struct i2c_client *const client = bus_priv->i2c_client;
	struct drm_i2c_encoder_driver *const driver = bus_priv->i2c_driver;

	bus_priv->i2c_client = NULL;
	i2c_unregister_device(client);
	bus_priv->i2c_driver = NULL;
	atomic_dec_uint(&driver->refcnt);
}

struct i2c_client *
drm_i2c_encoder_get_client(struct drm_encoder *encoder)
{
	struct drm_encoder_slave *const slave = to_encoder_slave(encoder);
	struct drm_i2c_encoder_bus_priv *const bus_priv = slave->bus_priv;

	return bus_priv->i2c_client;
}

static inline struct drm_encoder_slave_funcs *
slave_funcs(struct drm_encoder *encoder)
{

	return to_encoder_slave(encoder)->slave_funcs;
}

void
drm_i2c_encoder_dpms(struct drm_encoder *encoder, int mode)
{

	(*slave_funcs(encoder)->dpms)(encoder, mode);
}

void
drm_i2c_encoder_prepare(struct drm_encoder *encoder)
{

	drm_i2c_encoder_dpms(encoder, DRM_MODE_DPMS_OFF);
}

void
drm_i2c_encoder_commit(struct drm_encoder *encoder)
{

	drm_i2c_encoder_dpms(encoder, DRM_MODE_DPMS_ON);
}

bool
drm_i2c_encoder_mode_fixup(struct drm_encoder *encoder,
    const struct drm_display_mode *mode,
    struct drm_display_mode *adjusted_mode)
{

	return (*slave_funcs(encoder)->mode_fixup)(encoder, mode,
	    adjusted_mode);
}

void
drm_i2c_encoder_mode_set(struct drm_encoder *encoder,
    struct drm_display_mode *mode, struct drm_display_mode *adjusted_mode)
{

	return (*slave_funcs(encoder)->mode_set)(encoder, mode, adjusted_mode);
}

enum drm_connector_status
drm_i2c_encoder_detect(struct drm_encoder *encoder,
    struct drm_connector *connector)
{

	return (*slave_funcs(encoder)->detect)(encoder, connector);
}

void
drm_i2c_encoder_save(struct drm_encoder *encoder)
{

	(*slave_funcs(encoder)->save)(encoder);
}

void
drm_i2c_encoder_restore(struct drm_encoder *encoder)
{

	(*slave_funcs(encoder)->restore)(encoder);
}
