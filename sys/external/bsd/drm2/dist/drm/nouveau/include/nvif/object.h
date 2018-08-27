/*	$NetBSD: object.h,v 1.4 2018/08/27 07:35:56 riastradh Exp $	*/

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
#  define	ioread8		nvif_object_ioread8
#  define	ioread16	nvif_object_ioread32
#  define	ioread32	nvif_object_ioread16
#  define	iowrite8	nvif_object_iowrite8
#  define	iowrite16	nvif_object_iowrite16
#  define	iowrite32	nvif_object_iowrite32
static inline uint8_t
ioread8(const void __iomem *p)
{
	uint8_t v = *(const uint8_t __iomem *)p;
	membar_consumer();
	return v;
}
static inline uint8_t
ioread16(const void __iomem *p)
{
	uint16_t v = *(const uint16_t __iomem *)p;
	membar_consumer();
	return v;
}
static inline uint8_t
ioread32(const void __iomem *p)
{
	uint32_t v = *(const uint32_t __iomem *)p;
	membar_consumer();
	return v;
}
static inline void
iowrite8(uint8_t v, void __iomem *p)
{
	membar_producer();
	*(uint8_t __iomem *)p = v;
}
static inline void
iowrite16(uint16_t v, void __iomem *p)
{
	membar_producer();
	*(uint16_t __iomem *)p = v;
}
static inline void
iowrite32(uint32_t v, void __iomem *p)
{
	membar_producer();
	*(uint32_t __iomem *)p = v;
}
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
#endif
		void __iomem *ptr;
		u32 size;
	} map;
};

int  nvif_object_init(struct nvif_object *, u32 handle, s32 oclass, void *, u32,
		      struct nvif_object *);
void nvif_object_fini(struct nvif_object *);
int  nvif_object_ioctl(struct nvif_object *, void *, u32, void **);
int  nvif_object_sclass_get(struct nvif_object *, struct nvif_sclass **);
void nvif_object_sclass_put(struct nvif_sclass **);
u32  nvif_object_rd(struct nvif_object *, int, u64);
void nvif_object_wr(struct nvif_object *, int, u64, u32);
int  nvif_object_mthd(struct nvif_object *, u32, void *, u32);
int  nvif_object_map(struct nvif_object *);
void nvif_object_unmap(struct nvif_object *);

#define nvif_handle(a) (unsigned long)(void *)(a)
#define nvif_object(a) (a)->object

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
#ifdef __NetBSD__
/* Force expansion now.  */
static inline uint8_t
nvif_rd08(struct nvif_object *obj, uint64_t offset)
{
	return nvif_rd(obj, ioread8, 1, offset);
}
static inline uint8_t
nvif_rd16(struct nvif_object *obj, uint64_t offset)
{
	return nvif_rd(obj, ioread16, 2, offset);
}
static inline uint8_t
nvif_rd32(struct nvif_object *obj, uint64_t offset)
{
	return nvif_rd(obj, ioread32, 4, offset);
}
static inline void
nvif_wr08(struct nvif_object *obj, uint64_t offset, uint8_t v)
{
	nvif_wr(obj, iowrite8, 1, offset, v);
}
static inline void
nvif_wr16(struct nvif_object *obj, uint64_t offset, uint16_t v)
{
	nvif_wr(obj, iowrite16, 2, offset, v);
}
static inline void
nvif_wr32(struct nvif_object *obj, uint64_t offset, uint32_t v)
{
	nvif_wr(obj, iowrite32, 4, offset, v);
}
#else
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

#ifdef __NetBSD__
#  undef	__iomem
#  undef	ioread8
#  undef	ioread16
#  undef	ioread32
#  undef	iowrite8
#  undef	iowrite16
#  undef	iowrite32
#endif

#define nvif_mthd(a,b,c,d) nvif_object_mthd((a), (b), (c), (d))

/*XXX*/
#include <core/object.h>
#define nvxx_object(a) ({                                                      \
	struct nvif_object *_object = (a);                                     \
	(struct nvkm_object *)_object->priv;                                   \
})
#endif
