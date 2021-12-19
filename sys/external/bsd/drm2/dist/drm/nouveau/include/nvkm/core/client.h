/*	$NetBSD: client.h,v 1.7 2021/12/19 10:51:56 riastradh Exp $	*/

/* SPDX-License-Identifier: MIT */
#ifndef __NVKM_CLIENT_H__
#define __NVKM_CLIENT_H__
#define nvkm_client(p) container_of((p), struct nvkm_client, object)
#include <core/object.h>

struct nvkm_client {
	struct nvkm_object object;
	char name[32];
	u64 device;
	u32 debug;

	struct nvkm_client_notify *notify[32];
#ifdef __NetBSD__
	rb_tree_t objtree;
#else
	struct rb_root objroot;
#endif

	bool super;
	void *data;
	int (*ntfy)(const void *, u32, const void *, u32);

	struct list_head umem;
#ifdef __NetBSD__
	bus_space_tag_t mmiot;
	bus_space_handle_t mmioh;
	bus_addr_t mmioaddr;
	bus_size_t mmiosz;
#endif
	spinlock_t lock;
};

int  nvkm_client_new(const char *name, u64 device, const char *cfg,
		     const char *dbg,
		     int (*)(const void *, u32, const void *, u32),
		     struct nvkm_client **);
struct nvkm_client *nvkm_client_search(struct nvkm_client *, u64 handle);

int nvkm_client_notify_new(struct nvkm_object *, struct nvkm_event *,
			   void *data, u32 size);
int nvkm_client_notify_del(struct nvkm_client *, int index);
int nvkm_client_notify_get(struct nvkm_client *, int index);
int nvkm_client_notify_put(struct nvkm_client *, int index);

/* logging for client-facing objects */
#define nvif_printk(o,l,p,f,a...) do {                                         \
	const struct nvkm_object *_object = (o);                               \
	const struct nvkm_client *_client = _object->client;                   \
	if (_client->debug == NV_DBG_##l || _client->debug > NV_DBG_##l)       \
		printk(KERN_##p "nouveau: %s:%08x:%08x: "f, _client->name,     \
		       _object->handle, _object->oclass, ##a);                 \
} while(0)
#define nvif_fatal(o,f,a...) nvif_printk((o), FATAL, CRIT, f, ##a)
#define nvif_error(o,f,a...) nvif_printk((o), ERROR,  ERR, f, ##a)
#define nvif_debug(o,f,a...) nvif_printk((o), DEBUG, INFO, f, ##a)
#define nvif_trace(o,f,a...) nvif_printk((o), TRACE, INFO, f, ##a)
#define nvif_info(o,f,a...)  nvif_printk((o),  INFO, INFO, f, ##a)
#define nvif_ioctl(o,f,a...) nvif_trace((o), "ioctl: "f, ##a)
#endif
