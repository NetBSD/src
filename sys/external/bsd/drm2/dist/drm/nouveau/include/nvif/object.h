/*	$NetBSD: object.h,v 1.8 2018/08/27 14:48:21 riastradh Exp $	*/

#ifndef __NVIF_OBJECT_H__
#define __NVIF_OBJECT_H__

#include <nvif/os.h>

struct nvif_sclass {
	s32 oclass;
	int minver;
	int maxver;
};

#ifdef __NetBSD__
#  define	__nvif_iomem	volatile
#  define	__iomem		__nvif_iomem
#endif

struct nvif_object {
	struct nvif_client *client;
	u32 handle;
	s32 oclass;
	void *priv; /*XXX: hack */
	struct {
#ifdef __NetBSD__
		bus_space_tag_t tag;
		bus_space_handle_t handle;
		bus_addr_t addr;
#endif
		void __iomem *ptr;
		u32 size;
	} map;
};

#ifdef __NetBSD__
#  undef	__iomem
#endif

int  nvif_object_init(struct nvif_object *, u32 handle, s32 oclass, void *, u32,
		      struct nvif_object *);
void nvif_object_fini(struct nvif_object *);
int  nvif_object_ioctl(struct nvif_object *, void *, u32, void **);
int  nvif_object_sclass_get(struct nvif_object *, struct nvif_sclass **);
void nvif_object_sclass_put(struct nvif_sclass **);
u32  nvif_object_rd(struct nvif_object *, int, u64);
void nvif_object_wr(struct nvif_object *, int, u64, u32);
int  nvif_object_mthd(struct nvif_object *, u32, void *, u32);
int  nvif_object_map(struct nvif_object *) __must_check;
void nvif_object_unmap(struct nvif_object *);

#define nvif_handle(a) (unsigned long)(void *)(a)
#define nvif_object(a) (a)->object

#ifdef __NetBSD__
static inline uint8_t
nvif_rd08(struct nvif_object *obj, uint64_t offset)
{
	if (obj->map.ptr)
		return bus_space_read_1(obj->map.tag, obj->map.handle,
		    offset);
	else
		return nvif_object_rd(obj, 1, offset);
}
static inline uint16_t
nvif_rd16(struct nvif_object *obj, uint64_t offset)
{
	if (obj->map.ptr)
		return bus_space_read_stream_2(obj->map.tag, obj->map.handle,
		    offset);
	else
		return nvif_object_rd(obj, 2, offset);
}
static inline uint32_t
nvif_rd32(struct nvif_object *obj, uint64_t offset)
{
	if (obj->map.ptr)
		return bus_space_read_stream_4(obj->map.tag, obj->map.handle,
		    offset);
	else
		return nvif_object_rd(obj, 4, offset);
}
static inline void
nvif_wr08(struct nvif_object *obj, uint64_t offset, uint8_t v)
{
	if (obj->map.ptr)
		bus_space_write_1(obj->map.tag, obj->map.handle, offset, v);
	else
		nvif_object_wr(obj, 1, offset, v);
}
static inline void
nvif_wr16(struct nvif_object *obj, uint64_t offset, uint16_t v)
{
	if (obj->map.ptr)
		bus_space_write_stream_2(obj->map.tag, obj->map.handle, offset,
		    v);
	else
		nvif_object_wr(obj, 2, offset, v);
}
static inline void
nvif_wr32(struct nvif_object *obj, uint64_t offset, uint32_t v)
{
	if (obj->map.ptr)
		bus_space_write_stream_4(obj->map.tag, obj->map.handle, offset,
		    v);
	else
		nvif_object_wr(obj, 4, offset, v);
}
#else
#define nvif_rd(a,f,b,c) ({                                                    \
	struct nvif_object *_object = (a);                                     \
	u32 _data;                                                             \
	if (likely(_object->map.ptr))                                          \
		_data = f((u8 __iomem *)_object->map.ptr + (c));               \
	else                                                                   \
		_data = nvif_object_rd(_object, (b), (c));                     \
	_data;                                                                 \
})
#define nvif_wr(a,f,b,c,d) ({                                                  \
	struct nvif_object *_object = (a);                                     \
	if (likely(_object->map.ptr))                                          \
		f((d), (u8 __iomem *)_object->map.ptr + (c));                  \
	else                                                                   \
		nvif_object_wr(_object, (b), (c), (d));                        \
})
#define nvif_rd08(a,b) ({ ((u8)nvif_rd((a), ioread8, 1, (b))); })
#define nvif_rd16(a,b) ({ ((u16)nvif_rd((a), ioread16_native, 2, (b))); })
#define nvif_rd32(a,b) ({ ((u32)nvif_rd((a), ioread32_native, 4, (b))); })
#define nvif_wr08(a,b,c) nvif_wr((a), iowrite8, 1, (b), (u8)(c))
#define nvif_wr16(a,b,c) nvif_wr((a), iowrite16_native, 2, (b), (u16)(c))
#define nvif_wr32(a,b,c) nvif_wr((a), iowrite32_native, 4, (b), (u32)(c))
#endif
#define nvif_mask(a,b,c,d) ({                                                  \
	struct nvif_object *__object = (a);                                    \
	u32 _addr = (b), _data = nvif_rd32(__object, _addr);                   \
	nvif_wr32(__object, _addr, (_data & ~(c)) | (d));                      \
	_data;                                                                 \
})

#define nvif_mthd(a,b,c,d) nvif_object_mthd((a), (b), (c), (d))

/*XXX*/
#include <core/object.h>
#define nvxx_object(a) ({                                                      \
	struct nvif_object *_object = (a);                                     \
	(struct nvkm_object *)_object->priv;                                   \
})
#endif
