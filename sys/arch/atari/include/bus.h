/*	$NetBSD: bus.h,v 1.7 1997/06/25 12:06:10 leo Exp $	*/

/*
 * Copyright (c) 1996 Leo Weppelman.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Leo Weppelman for the
 *	NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ATARI_BUS_H_
#define _ATARI_BUS_H_

/*
 * I/O addresses (in bus space)
 */
typedef u_long bus_io_addr_t;
typedef u_long bus_io_size_t;

/*
 * Memory addresses (in bus space)
 */
typedef u_long	bus_addr_t;
typedef u_long	bus_size_t;

/*
 * DMA methods (currently unused)
 */
typedef void	*bus_dma_tag_t;
typedef void	*bus_dmamap_t;

/*
 * Access methods for bus resources and address space.
 */
typedef u_long	bus_space_tag_t;
typedef caddr_t	bus_space_handle_t;

int	bus_space_map __P((bus_space_tag_t, bus_addr_t, bus_size_t,
				int, bus_space_handle_t *));
void	bus_space_unmap __P((bus_space_tag_t, bus_space_handle_t,
				bus_size_t));
void	bus_space_read_multi_1 __P((bus_space_tag_t, bus_space_handle_t,
				int, caddr_t, int));
void	bus_space_read_multi_2 __P((bus_space_tag_t, bus_space_handle_t,
				int, u_int16_t *, int));
void	bus_space_read_multi_4 __P((bus_space_tag_t, bus_space_handle_t,
				int, u_int32_t *, int));
void	bus_space_read_multi_8 __P((bus_space_tag_t, bus_space_handle_t,
				int, u_int64_t *, int));
void	bus_space_write_multi_1 __P((bus_space_tag_t, bus_space_handle_t,
				int, caddr_t, int));
void	bus_space_write_multi_2 __P((bus_space_tag_t, bus_space_handle_t,
				int, u_int16_t *, int));
void	bus_space_write_multi_4 __P((bus_space_tag_t, bus_space_handle_t,
				int, u_int32_t *, int));
void	bus_space_write_multi_8 __P((bus_space_tag_t, bus_space_handle_t,
				int, u_int64_t *, int));

/*
 * Check accesibility of the location for various sized bus accesses
 */
#define bus_space_peek_1(t, h, o)					\
	(!badbaddr((caddr_t)((h) + (o)), 1))
#define bus_space_peek_2(t, h, o)					\
	(!badbaddr((caddr_t)((h) + (o)), 2))
#define bus_space_peek_4(t, h, o)					\
	(!badbaddr((caddr_t)((h) + (o)), 4))
#define bus_space_peek_8(t, h, o)					\
	(!badbaddr((caddr_t)((h) + (o)), 8))

#define	bus_space_read_1(t, h, o)					\
    ((void) t, (*(volatile u_int8_t *)((h) + (o))))
#define	bus_space_read_2(t, h, o)					\
    ((void) t, (*(volatile u_int16_t *)((h) + (o))))
#define	bus_space_read_4(t, h, o)					\
    ((void) t, (*(volatile u_int32_t *)((h) + (o))))
#define	bus_space_read_8(t, h, o)					\
    ((void) t, (*(volatile u_int64_t *)((h) + (o))))
#define	bus_space_write_1(t, h, o, v)					\
    ((void) t, ((void)(*(volatile u_int8_t *)((h) + (o)) = (v))))
#define	bus_space_write_2(t, h, o, v)					\
    ((void) t, ((void)(*(volatile u_int16_t *)((h) + (o)) = (v))))
#define	bus_space_write_4(t, h, o, v)					\
    ((void) t, ((void)(*(volatile u_int32_t *)((h) + (o)) = (v))))
#define	bus_space_write_8(t, h, o, v)					\
    ((void) t, ((void)(*(volatile u_int64_t *)((h) + (o)) = (v))))

extern __inline__ void
bus_space_read_multi_1(t, h, o, a, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	int			o, c;
	caddr_t			a;
{
	for (; c; a++, c--)
		*(u_int8_t *)a = bus_space_read_1(t, h, o);
}

extern __inline__ void
bus_space_read_multi_2(t, h, o, a, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	int			o, c;
	u_int16_t		*a;
{
	for (; c; a++, c--)
		*a = bus_space_read_2(t, h, o);
}

extern __inline__ void
bus_space_read_multi_4(t, h, o, a, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	int			o, c;
	u_int32_t		*a;
{
	for (; c; a++, c--)
		*a = bus_space_read_4(t, h, o);
}

extern __inline__ void
bus_space_read_multi_8(t, h, o, a, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	int			o, c;
	u_int64_t		*a;
{
	for (; c; a++, c--)
		*a = bus_space_read_8(t, h, o);
}

extern __inline__ void
bus_space_write_multi_1(t, h, o, a, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	int			o, c;
	caddr_t			a;
{
	for (; c; a++, c--)
		bus_space_write_1(t, h, o, *(u_int8_t *)a);
}

extern __inline__ void
bus_space_write_multi_2(t, h, o, a, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	int			o, c;
	u_int16_t		*a;
{
	for (; c; a++, c--)
		bus_space_write_2(t, h, o, *a);
}

extern __inline__ void
bus_space_write_multi_4(t, h, o, a, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	int			o, c;
	u_int32_t		*a;
{
	for (; c; a++, c--)
		bus_space_write_4(t, h, o, *a);
}

extern __inline__ void
bus_space_write_multi_8(t, h, o, a, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	int			o, c;
	u_int64_t		*a;
{
	for (; c; a++, c--)
		bus_space_write_8(t, h, o, *a);
}
#endif /* _ATARI_BUS_H_ */
