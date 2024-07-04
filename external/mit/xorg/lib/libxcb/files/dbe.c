/*
 * This file generated automatically from dbe.xml by c_client.py.
 * Edit at your peril.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stddef.h>  /* for offsetof() */
#include "xcbext.h"
#include "dbe.h"

#define ALIGNOF(type) offsetof(struct { char dummy; type member; }, member)
#include "xproto.h"

xcb_extension_t xcb_dbe_id = { "DOUBLE-BUFFER", 0 };

void
xcb_dbe_back_buffer_next (xcb_dbe_back_buffer_iterator_t *i)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_dbe_back_buffer_t);
}

xcb_generic_iterator_t
xcb_dbe_back_buffer_end (xcb_dbe_back_buffer_iterator_t i)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}

void
xcb_dbe_swap_info_next (xcb_dbe_swap_info_iterator_t *i)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_dbe_swap_info_t);
}

xcb_generic_iterator_t
xcb_dbe_swap_info_end (xcb_dbe_swap_info_iterator_t i)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}

void
xcb_dbe_buffer_attributes_next (xcb_dbe_buffer_attributes_iterator_t *i)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_dbe_buffer_attributes_t);
}

xcb_generic_iterator_t
xcb_dbe_buffer_attributes_end (xcb_dbe_buffer_attributes_iterator_t i)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}

void
xcb_dbe_visual_info_next (xcb_dbe_visual_info_iterator_t *i)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_dbe_visual_info_t);
}

xcb_generic_iterator_t
xcb_dbe_visual_info_end (xcb_dbe_visual_info_iterator_t i)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}

int
xcb_dbe_visual_infos_sizeof (const void  *_buffer)
{
    char *xcb_tmp = (char *)_buffer;
    const xcb_dbe_visual_infos_t *_aux = (xcb_dbe_visual_infos_t *)_buffer;
    unsigned int xcb_buffer_len = 0;
    unsigned int xcb_block_len = 0;
    unsigned int xcb_pad = 0;
    unsigned int xcb_align_to = 0;


    xcb_block_len += sizeof(xcb_dbe_visual_infos_t);
    xcb_tmp += xcb_block_len;
    xcb_buffer_len += xcb_block_len;
    xcb_block_len = 0;
    /* infos */
    xcb_block_len += _aux->n_infos * sizeof(xcb_dbe_visual_info_t);
    xcb_tmp += xcb_block_len;
    xcb_align_to = ALIGNOF(xcb_dbe_visual_info_t);
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_tmp += xcb_pad;
        xcb_pad = 0;
    }
    xcb_block_len = 0;

    return xcb_buffer_len;
}

xcb_dbe_visual_info_t *
xcb_dbe_visual_infos_infos (const xcb_dbe_visual_infos_t *R)
{
    return (xcb_dbe_visual_info_t *) (R + 1);
}

int
xcb_dbe_visual_infos_infos_length (const xcb_dbe_visual_infos_t *R)
{
    return R->n_infos;
}

xcb_dbe_visual_info_iterator_t
xcb_dbe_visual_infos_infos_iterator (const xcb_dbe_visual_infos_t *R)
{
    xcb_dbe_visual_info_iterator_t i;
    i.data = (xcb_dbe_visual_info_t *) (R + 1);
    i.rem = R->n_infos;
    i.index = (char *) i.data - (char *) R;
    return i;
}

void
xcb_dbe_visual_infos_next (xcb_dbe_visual_infos_iterator_t *i)
{
    xcb_dbe_visual_infos_t *R = i->data;
    xcb_generic_iterator_t child;
    child.data = (xcb_dbe_visual_infos_t *)(((char *)R) + xcb_dbe_visual_infos_sizeof(R));
    i->index = (char *) child.data - (char *) i->data;
    --i->rem;
    i->data = (xcb_dbe_visual_infos_t *) child.data;
}

xcb_generic_iterator_t
xcb_dbe_visual_infos_end (xcb_dbe_visual_infos_iterator_t i)
{
    xcb_generic_iterator_t ret;
    while(i.rem > 0)
        xcb_dbe_visual_infos_next(&i);
    ret.data = i.data;
    ret.rem = i.rem;
    ret.index = i.index;
    return ret;
}

xcb_dbe_query_version_cookie_t
xcb_dbe_query_version (xcb_connection_t *c,
                       uint8_t           major_version,
                       uint8_t           minor_version)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_dbe_id,
        .opcode = XCB_DBE_QUERY_VERSION,
        .isvoid = 0
    };

    struct iovec xcb_parts[4];
    xcb_dbe_query_version_cookie_t xcb_ret;
    xcb_dbe_query_version_request_t xcb_out;

    xcb_out.major_version = major_version;
    xcb_out.minor_version = minor_version;
    memset(xcb_out.pad0, 0, 2);

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_dbe_query_version_cookie_t
xcb_dbe_query_version_unchecked (xcb_connection_t *c,
                                 uint8_t           major_version,
                                 uint8_t           minor_version)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_dbe_id,
        .opcode = XCB_DBE_QUERY_VERSION,
        .isvoid = 0
    };

    struct iovec xcb_parts[4];
    xcb_dbe_query_version_cookie_t xcb_ret;
    xcb_dbe_query_version_request_t xcb_out;

    xcb_out.major_version = major_version;
    xcb_out.minor_version = minor_version;
    memset(xcb_out.pad0, 0, 2);

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_dbe_query_version_reply_t *
xcb_dbe_query_version_reply (xcb_connection_t                *c,
                             xcb_dbe_query_version_cookie_t   cookie  /**< */,
                             xcb_generic_error_t            **e)
{
    return (xcb_dbe_query_version_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}

xcb_void_cookie_t
xcb_dbe_allocate_back_buffer_checked (xcb_connection_t      *c,
                                      xcb_window_t           window,
                                      xcb_dbe_back_buffer_t  buffer,
                                      uint8_t                swap_action)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_dbe_id,
        .opcode = XCB_DBE_ALLOCATE_BACK_BUFFER,
        .isvoid = 1
    };

    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_dbe_allocate_back_buffer_request_t xcb_out;

    xcb_out.window = window;
    xcb_out.buffer = buffer;
    xcb_out.swap_action = swap_action;
    memset(xcb_out.pad0, 0, 3);

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_void_cookie_t
xcb_dbe_allocate_back_buffer (xcb_connection_t      *c,
                              xcb_window_t           window,
                              xcb_dbe_back_buffer_t  buffer,
                              uint8_t                swap_action)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_dbe_id,
        .opcode = XCB_DBE_ALLOCATE_BACK_BUFFER,
        .isvoid = 1
    };

    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_dbe_allocate_back_buffer_request_t xcb_out;

    xcb_out.window = window;
    xcb_out.buffer = buffer;
    xcb_out.swap_action = swap_action;
    memset(xcb_out.pad0, 0, 3);

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_void_cookie_t
xcb_dbe_deallocate_back_buffer_checked (xcb_connection_t      *c,
                                        xcb_dbe_back_buffer_t  buffer)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_dbe_id,
        .opcode = XCB_DBE_DEALLOCATE_BACK_BUFFER,
        .isvoid = 1
    };

    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_dbe_deallocate_back_buffer_request_t xcb_out;

    xcb_out.buffer = buffer;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_void_cookie_t
xcb_dbe_deallocate_back_buffer (xcb_connection_t      *c,
                                xcb_dbe_back_buffer_t  buffer)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_dbe_id,
        .opcode = XCB_DBE_DEALLOCATE_BACK_BUFFER,
        .isvoid = 1
    };

    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_dbe_deallocate_back_buffer_request_t xcb_out;

    xcb_out.buffer = buffer;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

int
xcb_dbe_swap_buffers_sizeof (const void  *_buffer)
{
    char *xcb_tmp = (char *)_buffer;
    const xcb_dbe_swap_buffers_request_t *_aux = (xcb_dbe_swap_buffers_request_t *)_buffer;
    unsigned int xcb_buffer_len = 0;
    unsigned int xcb_block_len = 0;
    unsigned int xcb_pad = 0;
    unsigned int xcb_align_to = 0;


    xcb_block_len += sizeof(xcb_dbe_swap_buffers_request_t);
    xcb_tmp += xcb_block_len;
    xcb_buffer_len += xcb_block_len;
    xcb_block_len = 0;
    /* actions */
    xcb_block_len += _aux->n_actions * sizeof(xcb_dbe_swap_info_t);
    xcb_tmp += xcb_block_len;
    xcb_align_to = ALIGNOF(xcb_dbe_swap_info_t);
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_tmp += xcb_pad;
        xcb_pad = 0;
    }
    xcb_block_len = 0;

    return xcb_buffer_len;
}

xcb_void_cookie_t
xcb_dbe_swap_buffers_checked (xcb_connection_t          *c,
                              uint32_t                   n_actions,
                              const xcb_dbe_swap_info_t *actions)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 4,
        .ext = &xcb_dbe_id,
        .opcode = XCB_DBE_SWAP_BUFFERS,
        .isvoid = 1
    };

    struct iovec xcb_parts[6];
    xcb_void_cookie_t xcb_ret;
    xcb_dbe_swap_buffers_request_t xcb_out;

    xcb_out.n_actions = n_actions;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    /* xcb_dbe_swap_info_t actions */
    xcb_parts[4].iov_base = (char *) actions;
    xcb_parts[4].iov_len = n_actions * sizeof(xcb_dbe_swap_info_t);
    xcb_parts[5].iov_base = 0;
    xcb_parts[5].iov_len = -xcb_parts[4].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_void_cookie_t
xcb_dbe_swap_buffers (xcb_connection_t          *c,
                      uint32_t                   n_actions,
                      const xcb_dbe_swap_info_t *actions)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 4,
        .ext = &xcb_dbe_id,
        .opcode = XCB_DBE_SWAP_BUFFERS,
        .isvoid = 1
    };

    struct iovec xcb_parts[6];
    xcb_void_cookie_t xcb_ret;
    xcb_dbe_swap_buffers_request_t xcb_out;

    xcb_out.n_actions = n_actions;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    /* xcb_dbe_swap_info_t actions */
    xcb_parts[4].iov_base = (char *) actions;
    xcb_parts[4].iov_len = n_actions * sizeof(xcb_dbe_swap_info_t);
    xcb_parts[5].iov_base = 0;
    xcb_parts[5].iov_len = -xcb_parts[4].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_dbe_swap_info_t *
xcb_dbe_swap_buffers_actions (const xcb_dbe_swap_buffers_request_t *R)
{
    return (xcb_dbe_swap_info_t *) (R + 1);
}

int
xcb_dbe_swap_buffers_actions_length (const xcb_dbe_swap_buffers_request_t *R)
{
    return R->n_actions;
}

xcb_dbe_swap_info_iterator_t
xcb_dbe_swap_buffers_actions_iterator (const xcb_dbe_swap_buffers_request_t *R)
{
    xcb_dbe_swap_info_iterator_t i;
    i.data = (xcb_dbe_swap_info_t *) (R + 1);
    i.rem = R->n_actions;
    i.index = (char *) i.data - (char *) R;
    return i;
}

xcb_void_cookie_t
xcb_dbe_begin_idiom_checked (xcb_connection_t *c)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_dbe_id,
        .opcode = XCB_DBE_BEGIN_IDIOM,
        .isvoid = 1
    };

    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_dbe_begin_idiom_request_t xcb_out;


    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_void_cookie_t
xcb_dbe_begin_idiom (xcb_connection_t *c)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_dbe_id,
        .opcode = XCB_DBE_BEGIN_IDIOM,
        .isvoid = 1
    };

    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_dbe_begin_idiom_request_t xcb_out;


    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_void_cookie_t
xcb_dbe_end_idiom_checked (xcb_connection_t *c)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_dbe_id,
        .opcode = XCB_DBE_END_IDIOM,
        .isvoid = 1
    };

    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_dbe_end_idiom_request_t xcb_out;


    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_void_cookie_t
xcb_dbe_end_idiom (xcb_connection_t *c)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_dbe_id,
        .opcode = XCB_DBE_END_IDIOM,
        .isvoid = 1
    };

    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_dbe_end_idiom_request_t xcb_out;


    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

int
xcb_dbe_get_visual_info_sizeof (const void  *_buffer)
{
    char *xcb_tmp = (char *)_buffer;
    const xcb_dbe_get_visual_info_request_t *_aux = (xcb_dbe_get_visual_info_request_t *)_buffer;
    unsigned int xcb_buffer_len = 0;
    unsigned int xcb_block_len = 0;
    unsigned int xcb_pad = 0;
    unsigned int xcb_align_to = 0;


    xcb_block_len += sizeof(xcb_dbe_get_visual_info_request_t);
    xcb_tmp += xcb_block_len;
    xcb_buffer_len += xcb_block_len;
    xcb_block_len = 0;
    /* drawables */
    xcb_block_len += _aux->n_drawables * sizeof(xcb_drawable_t);
    xcb_tmp += xcb_block_len;
    xcb_align_to = ALIGNOF(xcb_drawable_t);
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_tmp += xcb_pad;
        xcb_pad = 0;
    }
    xcb_block_len = 0;

    return xcb_buffer_len;
}

xcb_dbe_get_visual_info_cookie_t
xcb_dbe_get_visual_info (xcb_connection_t     *c,
                         uint32_t              n_drawables,
                         const xcb_drawable_t *drawables)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 4,
        .ext = &xcb_dbe_id,
        .opcode = XCB_DBE_GET_VISUAL_INFO,
        .isvoid = 0
    };

    struct iovec xcb_parts[6];
    xcb_dbe_get_visual_info_cookie_t xcb_ret;
    xcb_dbe_get_visual_info_request_t xcb_out;

    xcb_out.n_drawables = n_drawables;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    /* xcb_drawable_t drawables */
    xcb_parts[4].iov_base = (char *) drawables;
    xcb_parts[4].iov_len = n_drawables * sizeof(xcb_visualid_t);
    xcb_parts[5].iov_base = 0;
    xcb_parts[5].iov_len = -xcb_parts[4].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_dbe_get_visual_info_cookie_t
xcb_dbe_get_visual_info_unchecked (xcb_connection_t     *c,
                                   uint32_t              n_drawables,
                                   const xcb_drawable_t *drawables)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 4,
        .ext = &xcb_dbe_id,
        .opcode = XCB_DBE_GET_VISUAL_INFO,
        .isvoid = 0
    };

    struct iovec xcb_parts[6];
    xcb_dbe_get_visual_info_cookie_t xcb_ret;
    xcb_dbe_get_visual_info_request_t xcb_out;

    xcb_out.n_drawables = n_drawables;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    /* xcb_drawable_t drawables */
    xcb_parts[4].iov_base = (char *) drawables;
    xcb_parts[4].iov_len = n_drawables * sizeof(xcb_visualid_t);
    xcb_parts[5].iov_base = 0;
    xcb_parts[5].iov_len = -xcb_parts[4].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

int
xcb_dbe_get_visual_info_supported_visuals_length (const xcb_dbe_get_visual_info_reply_t *R)
{
    return R->n_supported_visuals;
}

xcb_dbe_visual_infos_iterator_t
xcb_dbe_get_visual_info_supported_visuals_iterator (const xcb_dbe_get_visual_info_reply_t *R)
{
    xcb_dbe_visual_infos_iterator_t i;
    i.data = (xcb_dbe_visual_infos_t *) (R + 1);
    i.rem = R->n_supported_visuals;
    i.index = (char *) i.data - (char *) R;
    return i;
}

xcb_dbe_get_visual_info_reply_t *
xcb_dbe_get_visual_info_reply (xcb_connection_t                  *c,
                               xcb_dbe_get_visual_info_cookie_t   cookie  /**< */,
                               xcb_generic_error_t              **e)
{
    return (xcb_dbe_get_visual_info_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}

xcb_dbe_get_back_buffer_attributes_cookie_t
xcb_dbe_get_back_buffer_attributes (xcb_connection_t      *c,
                                    xcb_dbe_back_buffer_t  buffer)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_dbe_id,
        .opcode = XCB_DBE_GET_BACK_BUFFER_ATTRIBUTES,
        .isvoid = 0
    };

    struct iovec xcb_parts[4];
    xcb_dbe_get_back_buffer_attributes_cookie_t xcb_ret;
    xcb_dbe_get_back_buffer_attributes_request_t xcb_out;

    xcb_out.buffer = buffer;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_dbe_get_back_buffer_attributes_cookie_t
xcb_dbe_get_back_buffer_attributes_unchecked (xcb_connection_t      *c,
                                              xcb_dbe_back_buffer_t  buffer)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_dbe_id,
        .opcode = XCB_DBE_GET_BACK_BUFFER_ATTRIBUTES,
        .isvoid = 0
    };

    struct iovec xcb_parts[4];
    xcb_dbe_get_back_buffer_attributes_cookie_t xcb_ret;
    xcb_dbe_get_back_buffer_attributes_request_t xcb_out;

    xcb_out.buffer = buffer;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_dbe_get_back_buffer_attributes_reply_t *
xcb_dbe_get_back_buffer_attributes_reply (xcb_connection_t                             *c,
                                          xcb_dbe_get_back_buffer_attributes_cookie_t   cookie  /**< */,
                                          xcb_generic_error_t                         **e)
{
    return (xcb_dbe_get_back_buffer_attributes_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}

