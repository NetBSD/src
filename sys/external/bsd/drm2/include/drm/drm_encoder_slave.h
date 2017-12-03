/*	$NetBSD: drm_encoder_slave.h,v 1.1.20.2 2017/12/03 11:37:59 jdolecek Exp $	*/

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

#ifndef	_DRM_DRM_ENCODER_SLAVE_H_
#define	_DRM_DRM_ENCODER_SLAVE_H_

#include <sys/types.h>
#include <sys/rbtree.h>

#include <drm/drm_crtc.h>

struct drm_encoder_slave_funcs {
	void	(*set_config)(struct drm_encoder *, void *);
	void	(*destroy)(struct drm_encoder *);
	void	(*dpms)(struct drm_encoder *, int mode);
	void	(*save)(struct drm_encoder *);
	void	(*restore)(struct drm_encoder *);
	bool	(*mode_fixup)(struct drm_encoder *,
		    const struct drm_display_mode *,
		    struct drm_display_mode *);
	int	(*mode_valid)(struct drm_encoder *, struct drm_display_mode *);
	void	(*mode_set)(struct drm_encoder *,
		    const struct drm_display_mode *,
		    struct drm_display_mode *);
	enum drm_connector_status
		(*detect)(struct drm_encoder *, struct drm_connector *);
	int	(*get_modes)(struct drm_encoder *, struct drm_connector *);
	int	(*create_resources)(struct drm_encoder *,
		    struct drm_connector *);
	int	(*set_property)(struct drm_encoder *, struct drm_connector *,
		    struct drm_property *, uint64_t);
};

struct drm_encoder_slave {
	struct drm_encoder		base;
	struct drm_encoder_slave_funcs	*slave_funcs;
	void				*slave_priv;
	void				*bus_priv;
};

static inline struct drm_encoder_slave *
to_encoder_slave(struct drm_encoder *encoder)
{

	return container_of(encoder, struct drm_encoder_slave, base);
}

struct drm_i2c_encoder_driver {
	struct i2c_driver	i2c_driver;
	int			(*encoder_init)(struct i2c_client *,
				    struct drm_device *,
				    struct drm_encoder_slave *);
	rb_node_t		rb_node;
	volatile unsigned	refcnt;
};

struct module;

void	drm_i2c_encoders_init(void);
void	drm_i2c_encoders_fini(void);

int	drm_i2c_encoder_register(struct module *,
	    struct drm_i2c_encoder_driver *);
void	drm_i2c_encoder_unregister(struct drm_i2c_encoder_driver *);

int	drm_i2c_encoder_init(struct drm_device *, struct drm_encoder_slave *,
	    struct i2c_adapter *, const struct i2c_board_info *);
void	drm_i2c_encoder_destroy(struct drm_encoder *);
struct i2c_client *
	drm_i2c_encoder_get_client(struct drm_encoder *);

void	drm_i2c_encoder_dpms(struct drm_encoder *, int);
bool	drm_i2c_encoder_mode_fixup(struct drm_encoder *,
	    const struct drm_display_mode *, struct drm_display_mode *);
void	drm_i2c_encoder_prepare(struct drm_encoder *);
void	drm_i2c_encoder_commit(struct drm_encoder *);
void	drm_i2c_encoder_mode_set(struct drm_encoder *,
	    struct drm_display_mode *, struct drm_display_mode *);
enum drm_connector_status
	drm_i2c_encoder_detect(struct drm_encoder *, struct drm_connector *);
void	drm_i2c_encoder_save(struct drm_encoder *);
void	drm_i2c_encoder_restore(struct drm_encoder *);

#endif	/* _DRM_DRM_ENCODER_SLAVE_H_ */
