#ifndef _SH3_BUS_UTIL_H_
#define _SH3_BUS_UTIL_H_
/*
 * Utility macros; INTERNAL USE ONLY.
 */

#define	__TYPENAME(BITS)	u_int##BITS##_t

#define _BUS_SPACE_READ(PREFIX, BYTES, BITS)				\
__TYPENAME(BITS)							\
PREFIX##_read_##BYTES(void *, bus_space_handle_t,  bus_size_t);		\
__TYPENAME(BITS)							\
PREFIX##_read_##BYTES(void *tag, bus_space_handle_t bsh,		\
		      bus_size_t offset)				\
{									\
	_BUS_SPACE_ACCESS_HOOK();					\
	return *(volatile __TYPENAME(BITS) *)(bsh + offset);		\
}

#define _BUS_SPACE_READ_MULTI(PREFIX, BYTES, BITS)			\
void								\
PREFIX##_read_multi_##BYTES(void *, bus_space_handle_t,	bus_size_t,	\
			    __TYPENAME(BITS) *,	bus_size_t);		\
void								\
PREFIX##_read_multi_##BYTES(void *tag, bus_space_handle_t bsh,		\
			    bus_size_t offset, __TYPENAME(BITS) *addr,	\
			    bus_size_t count)				\
{									\
	volatile __TYPENAME(BITS) *p = (void *)(bsh + offset);		\
	_BUS_SPACE_ACCESS_HOOK();					\
	while (count--)							\
		*addr++ = *p;						\
}

#define _BUS_SPACE_READ_REGION(PREFIX, BYTES, BITS)			\
void								\
PREFIX##_read_region_##BYTES(void *, bus_space_handle_t, bus_size_t,	\
			     __TYPENAME(BITS) *, bus_size_t);		\
void								\
PREFIX##_read_region_##BYTES(void *tag, bus_space_handle_t bsh,		\
			     bus_size_t offset, __TYPENAME(BITS) *addr,	\
			     bus_size_t count)				\
{									\
	volatile __TYPENAME(BITS) *p = (void *)(bsh + offset);		\
	_BUS_SPACE_ACCESS_HOOK();					\
	while (count--)							\
		*addr++ = *p++;						\
}

#define _BUS_SPACE_WRITE(PREFIX, BYTES, BITS)				\
void								\
PREFIX##_write_##BYTES(void *, bus_space_handle_t, bus_size_t,		\
		       __TYPENAME(BITS));				\
void								\
PREFIX##_write_##BYTES(void *tag, bus_space_handle_t bsh,		\
		       bus_size_t offset, __TYPENAME(BITS) value)	\
{									\
	_BUS_SPACE_ACCESS_HOOK();					\
	*(volatile __TYPENAME(BITS) *)(bsh + offset) = value;		\
}

#define _BUS_SPACE_WRITE_MULTI(PREFIX, BYTES, BITS)			\
void								\
PREFIX##_write_multi_##BYTES(void *, bus_space_handle_t, bus_size_t,	\
			     const __TYPENAME(BITS) *, bus_size_t);	\
void								\
PREFIX##_write_multi_##BYTES(void *tag, bus_space_handle_t bsh,		\
			     bus_size_t offset,				\
			     const __TYPENAME(BITS) *addr,		\
			     bus_size_t count)				\
{									\
	volatile __TYPENAME(BITS) *p = (void *)(bsh + offset);		\
	_BUS_SPACE_ACCESS_HOOK();					\
	while (count--)							\
		*p = *addr++;						\
}

#define _BUS_SPACE_WRITE_REGION(PREFIX, BYTES, BITS)			\
void								\
PREFIX##_write_region_##BYTES(void *, bus_space_handle_t, bus_size_t,	\
			      const __TYPENAME(BITS) *, bus_size_t);	\
void								\
PREFIX##_write_region_##BYTES(void *tag, bus_space_handle_t bsh,	\
			      bus_size_t offset,			\
			      const __TYPENAME(BITS) *addr,		\
			      bus_size_t count)				\
{									\
	volatile __TYPENAME(BITS) *p = (void *)(bsh + offset);		\
	_BUS_SPACE_ACCESS_HOOK();					\
	while (count--)							\
		*p++ = *addr++;						\
}

#define _BUS_SPACE_SET_MULTI(PREFIX, BYTES, BITS)			\
void								\
PREFIX##_set_multi_##BYTES(void *, bus_space_handle_t, bus_size_t,	\
			   __TYPENAME(BITS), bus_size_t);		\
void								\
PREFIX##_set_multi_##BYTES(void *tag, bus_space_handle_t bsh,		\
			   bus_size_t offset, __TYPENAME(BITS) value,	\
			   bus_size_t count)				\
{									\
	volatile __TYPENAME(BITS) *p = (void *)(bsh + offset);		\
	_BUS_SPACE_ACCESS_HOOK();					\
	while (count--)							\
		*p = value;						\
}

#define _BUS_SPACE_COPY_REGION(PREFIX, BYTES, BITS)			\
void								\
PREFIX##_copy_region_##BYTES(void *, bus_space_handle_t, bus_size_t,	\
			     bus_space_handle_t, bus_size_t,		\
			     bus_size_t);				\
void								\
PREFIX##_copy_region_##BYTES(void *t, bus_space_handle_t h1,		\
			     bus_size_t o1, bus_space_handle_t h2,	\
			     bus_size_t o2, bus_size_t c)		\
{									\
	volatile __TYPENAME(BITS) *addr1 = (void *)(h1 + o1);		\
	volatile __TYPENAME(BITS) *addr2 = (void *)(h2 + o2);		\
	_BUS_SPACE_ACCESS_HOOK();					\
									\
	if (addr1 >= addr2) {	/* src after dest: copy forward */	\
		while (c--)						\
			*addr2++ = *addr1++;				\
	} else {		/* dest after src: copy backwards */	\
		addr1 += c - 1;						\
		addr2 += c - 1;						\
		while (c--)						\
			*addr2-- = *addr1--;				\
	}								\
}
#endif /* _SH3_BUS_UTIL_H_ */
