/*	$NetBSD: nouveau_engine_fifo_base.c,v 1.3.2.1 2016/03/19 11:30:29 skrll Exp $	*/

/*
 * Copyright 2012 Red Hat Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors: Ben Skeggs
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: nouveau_engine_fifo_base.c,v 1.3.2.1 2016/03/19 11:30:29 skrll Exp $");

#include <core/client.h>
#include <core/object.h>
#include <core/handle.h>
#include <core/event.h>
#include <core/class.h>

#include <engine/dmaobj.h>
#include <engine/fifo.h>

int
nouveau_fifo_channel_create_(struct nouveau_object *parent,
			     struct nouveau_object *engine,
			     struct nouveau_oclass *oclass,
			     int bar, u32 addr, u32 size, u32 pushbuf,
			     u64 engmask, int len, void **ptr)
{
	struct nouveau_device *device = nv_device(engine);
	struct nouveau_fifo *priv = (void *)engine;
	struct nouveau_fifo_chan *chan;
	struct nouveau_dmaeng *dmaeng;
	unsigned long flags;
	int ret;

	/* create base object class */
	ret = nouveau_namedb_create_(parent, engine, oclass, 0, NULL,
				     engmask, len, ptr);
	chan = *ptr;
	if (ret)
		return ret;

	/* validate dma object representing push buffer */
	chan->pushdma = (void *)nouveau_handle_ref(parent, pushbuf);
	if (!chan->pushdma)
		return -ENOENT;

	dmaeng = (void *)chan->pushdma->base.engine;
	switch (chan->pushdma->base.oclass->handle) {
	case NV_DMA_FROM_MEMORY_CLASS:
	case NV_DMA_IN_MEMORY_CLASS:
		break;
	default:
		return -EINVAL;
	}

	ret = dmaeng->bind(dmaeng, parent, chan->pushdma, &chan->pushgpu);
	if (ret)
		return ret;

	/* find a free fifo channel */
	spin_lock_irqsave(&priv->lock, flags);
	for (chan->chid = priv->min; chan->chid < priv->max; chan->chid++) {
		if (!priv->channel[chan->chid]) {
			priv->channel[chan->chid] = nv_object(chan);
			break;
		}
	}
	spin_unlock_irqrestore(&priv->lock, flags);

	if (chan->chid == priv->max) {
		nv_error(priv, "no free channels\n");
		return -ENOSPC;
	}

	/* map fifo control registers */
#ifdef __NetBSD__
	chan->bst = nv_device_resource_tag(device, bar);
	/* XXX errno NetBSD->Linux */
	ret = -bus_space_map(chan->bst, nv_device_resource_start(device, bar) +
	    addr + (chan->chid * size), size, 0, &chan->bsh);
	if (ret)
		return ret;
	chan->mapped = true;
#else
	chan->user = ioremap(nv_device_resource_start(device, bar) + addr +
			     (chan->chid * size), size);
	if (!chan->user)
		return -EFAULT;
#endif

	nouveau_event_trigger(priv->cevent, 0);

	chan->size = size;
	return 0;
}

void
nouveau_fifo_channel_destroy(struct nouveau_fifo_chan *chan)
{
	struct nouveau_fifo *priv = (void *)nv_object(chan)->engine;
	unsigned long flags;

#ifdef __NetBSD__
	if (chan->mapped) {
		bus_space_unmap(chan->bst, chan->bsh, chan->size);
		chan->mapped = false;
	}
#else
	iounmap(chan->user);
#endif

	spin_lock_irqsave(&priv->lock, flags);
	priv->channel[chan->chid] = NULL;
	spin_unlock_irqrestore(&priv->lock, flags);

	nouveau_gpuobj_ref(NULL, &chan->pushgpu);
	nouveau_object_ref(NULL, (struct nouveau_object **)&chan->pushdma);
	nouveau_namedb_destroy(&chan->base);
}

void
_nouveau_fifo_channel_dtor(struct nouveau_object *object)
{
	struct nouveau_fifo_chan *chan = (void *)object;
	nouveau_fifo_channel_destroy(chan);
}

u32
_nouveau_fifo_channel_rd32(struct nouveau_object *object, u64 addr)
{
	struct nouveau_fifo_chan *chan = (void *)object;
#ifdef __NetBSD__
	return bus_space_read_4(chan->bst, chan->bsh, addr);
#else
	return ioread32_native(chan->user + addr);
#endif
}

void
_nouveau_fifo_channel_wr32(struct nouveau_object *object, u64 addr, u32 data)
{
	struct nouveau_fifo_chan *chan = (void *)object;
#ifdef __NetBSD__
	bus_space_write_4(chan->bst, chan->bsh, addr, data);
#else
	iowrite32_native(data, chan->user + addr);
#endif
}

static int
nouveau_fifo_chid(struct nouveau_fifo *priv, struct nouveau_object *object)
{
	int engidx = nv_hclass(priv) & 0xff;

	while (object && object->parent) {
		if ( nv_iclass(object->parent, NV_ENGCTX_CLASS) &&
		    (nv_hclass(object->parent) & 0xff) == engidx)
			return nouveau_fifo_chan(object)->chid;
		object = object->parent;
	}

	return -1;
}

const char *
nouveau_client_name_for_fifo_chid(struct nouveau_fifo *fifo, u32 chid)
{
	struct nouveau_fifo_chan *chan = NULL;
	unsigned long flags;

	spin_lock_irqsave(&fifo->lock, flags);
	if (chid >= fifo->min && chid <= fifo->max)
		chan = (void *)fifo->channel[chid];
	spin_unlock_irqrestore(&fifo->lock, flags);

	return nouveau_client_name(chan);
}

void
nouveau_fifo_destroy(struct nouveau_fifo *priv)
{
	kfree(priv->channel);
	nouveau_event_destroy(&priv->uevent);
	nouveau_event_destroy(&priv->cevent);
	nouveau_engine_destroy(&priv->base);
}

int
nouveau_fifo_create_(struct nouveau_object *parent,
		     struct nouveau_object *engine,
		     struct nouveau_oclass *oclass,
		     int min, int max, int length, void **pobject)
{
	struct nouveau_fifo *priv;
	int ret;

	ret = nouveau_engine_create_(parent, engine, oclass, true, "PFIFO",
				     "fifo", length, pobject);
	priv = *pobject;
	if (ret)
		return ret;

	priv->min = min;
	priv->max = max;
	priv->channel = kzalloc(sizeof(*priv->channel) * (max + 1), GFP_KERNEL);
	if (!priv->channel)
		return -ENOMEM;

	ret = nouveau_event_create(1, &priv->cevent);
	if (ret)
		return ret;

	ret = nouveau_event_create(1, &priv->uevent);
	if (ret)
		return ret;

	priv->chid = nouveau_fifo_chid;
	spin_lock_init(&priv->lock);
	return 0;
}
