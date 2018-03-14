/*
 * This file generated automatically from dri3.xml by c_client.py.
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
#include "dri3.h"

#define ALIGNOF(type) offsetof(struct { char dummy; type member; }, member)
#include "xproto.h"

xcb_extension_t xcb_dri3_id = { "DRI3", 0 };

xcb_dri3_query_version_cookie_t
xcb_dri3_query_version (xcb_connection_t *c,
                        uint32_t          major_version,
                        uint32_t          minor_version)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_dri3_id,
        .opcode = XCB_DRI3_QUERY_VERSION,
        .isvoid = 0
    };

    struct iovec xcb_parts[4];
    xcb_dri3_query_version_cookie_t xcb_ret;
    xcb_dri3_query_version_request_t xcb_out;

    xcb_out.major_version = major_version;
    xcb_out.minor_version = minor_version;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_dri3_query_version_cookie_t
xcb_dri3_query_version_unchecked (xcb_connection_t *c,
                                  uint32_t          major_version,
                                  uint32_t          minor_version)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_dri3_id,
        .opcode = XCB_DRI3_QUERY_VERSION,
        .isvoid = 0
    };

    struct iovec xcb_parts[4];
    xcb_dri3_query_version_cookie_t xcb_ret;
    xcb_dri3_query_version_request_t xcb_out;

    xcb_out.major_version = major_version;
    xcb_out.minor_version = minor_version;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_dri3_query_version_reply_t *
xcb_dri3_query_version_reply (xcb_connection_t                 *c,
                              xcb_dri3_query_version_cookie_t   cookie  /**< */,
                              xcb_generic_error_t             **e)
{
    return (xcb_dri3_query_version_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}

xcb_dri3_open_cookie_t
xcb_dri3_open (xcb_connection_t *c,
               xcb_drawable_t    drawable,
               uint32_t          provider)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_dri3_id,
        .opcode = XCB_DRI3_OPEN,
        .isvoid = 0
    };

    struct iovec xcb_parts[4];
    xcb_dri3_open_cookie_t xcb_ret;
    xcb_dri3_open_request_t xcb_out;

    xcb_out.drawable = drawable;
    xcb_out.provider = provider;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED|XCB_REQUEST_REPLY_FDS, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_dri3_open_cookie_t
xcb_dri3_open_unchecked (xcb_connection_t *c,
                         xcb_drawable_t    drawable,
                         uint32_t          provider)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_dri3_id,
        .opcode = XCB_DRI3_OPEN,
        .isvoid = 0
    };

    struct iovec xcb_parts[4];
    xcb_dri3_open_cookie_t xcb_ret;
    xcb_dri3_open_request_t xcb_out;

    xcb_out.drawable = drawable;
    xcb_out.provider = provider;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_REPLY_FDS, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_dri3_open_reply_t *
xcb_dri3_open_reply (xcb_connection_t        *c,
                     xcb_dri3_open_cookie_t   cookie  /**< */,
                     xcb_generic_error_t    **e)
{
    return (xcb_dri3_open_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}

int *
xcb_dri3_open_reply_fds (xcb_connection_t       *c  /**< */,
                         xcb_dri3_open_reply_t  *reply)
{
    return xcb_get_reply_fds(c, reply, sizeof(xcb_dri3_open_reply_t) + 4 * reply->length);
}

xcb_void_cookie_t
xcb_dri3_pixmap_from_buffer_checked (xcb_connection_t *c,
                                     xcb_pixmap_t      pixmap,
                                     xcb_drawable_t    drawable,
                                     uint32_t          size,
                                     uint16_t          width,
                                     uint16_t          height,
                                     uint16_t          stride,
                                     uint8_t           depth,
                                     uint8_t           bpp,
                                     int32_t           pixmap_fd)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_dri3_id,
        .opcode = XCB_DRI3_PIXMAP_FROM_BUFFER,
        .isvoid = 1
    };

    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_dri3_pixmap_from_buffer_request_t xcb_out;
    int fds[1];
    int fd_index = 0;

    xcb_out.pixmap = pixmap;
    xcb_out.drawable = drawable;
    xcb_out.size = size;
    xcb_out.width = width;
    xcb_out.height = height;
    xcb_out.stride = stride;
    xcb_out.depth = depth;
    xcb_out.bpp = bpp;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    fds[fd_index++] = pixmap_fd;
    xcb_ret.sequence = xcb_send_request_with_fds(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req, 1, fds);
    return xcb_ret;
}

xcb_void_cookie_t
xcb_dri3_pixmap_from_buffer (xcb_connection_t *c,
                             xcb_pixmap_t      pixmap,
                             xcb_drawable_t    drawable,
                             uint32_t          size,
                             uint16_t          width,
                             uint16_t          height,
                             uint16_t          stride,
                             uint8_t           depth,
                             uint8_t           bpp,
                             int32_t           pixmap_fd)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_dri3_id,
        .opcode = XCB_DRI3_PIXMAP_FROM_BUFFER,
        .isvoid = 1
    };

    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_dri3_pixmap_from_buffer_request_t xcb_out;
    int fds[1];
    int fd_index = 0;

    xcb_out.pixmap = pixmap;
    xcb_out.drawable = drawable;
    xcb_out.size = size;
    xcb_out.width = width;
    xcb_out.height = height;
    xcb_out.stride = stride;
    xcb_out.depth = depth;
    xcb_out.bpp = bpp;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    fds[fd_index++] = pixmap_fd;
    xcb_ret.sequence = xcb_send_request_with_fds(c, 0, xcb_parts + 2, &xcb_req, 1, fds);
    return xcb_ret;
}

xcb_dri3_buffer_from_pixmap_cookie_t
xcb_dri3_buffer_from_pixmap (xcb_connection_t *c,
                             xcb_pixmap_t      pixmap)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_dri3_id,
        .opcode = XCB_DRI3_BUFFER_FROM_PIXMAP,
        .isvoid = 0
    };

    struct iovec xcb_parts[4];
    xcb_dri3_buffer_from_pixmap_cookie_t xcb_ret;
    xcb_dri3_buffer_from_pixmap_request_t xcb_out;

    xcb_out.pixmap = pixmap;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED|XCB_REQUEST_REPLY_FDS, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_dri3_buffer_from_pixmap_cookie_t
xcb_dri3_buffer_from_pixmap_unchecked (xcb_connection_t *c,
                                       xcb_pixmap_t      pixmap)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_dri3_id,
        .opcode = XCB_DRI3_BUFFER_FROM_PIXMAP,
        .isvoid = 0
    };

    struct iovec xcb_parts[4];
    xcb_dri3_buffer_from_pixmap_cookie_t xcb_ret;
    xcb_dri3_buffer_from_pixmap_request_t xcb_out;

    xcb_out.pixmap = pixmap;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_REPLY_FDS, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_dri3_buffer_from_pixmap_reply_t *
xcb_dri3_buffer_from_pixmap_reply (xcb_connection_t                      *c,
                                   xcb_dri3_buffer_from_pixmap_cookie_t   cookie  /**< */,
                                   xcb_generic_error_t                  **e)
{
    return (xcb_dri3_buffer_from_pixmap_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}

int *
xcb_dri3_buffer_from_pixmap_reply_fds (xcb_connection_t                     *c  /**< */,
                                       xcb_dri3_buffer_from_pixmap_reply_t  *reply)
{
    return xcb_get_reply_fds(c, reply, sizeof(xcb_dri3_buffer_from_pixmap_reply_t) + 4 * reply->length);
}

xcb_void_cookie_t
xcb_dri3_fence_from_fd_checked (xcb_connection_t *c,
                                xcb_drawable_t    drawable,
                                uint32_t          fence,
                                uint8_t           initially_triggered,
                                int32_t           fence_fd)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_dri3_id,
        .opcode = XCB_DRI3_FENCE_FROM_FD,
        .isvoid = 1
    };

    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_dri3_fence_from_fd_request_t xcb_out;
    int fds[1];
    int fd_index = 0;

    xcb_out.drawable = drawable;
    xcb_out.fence = fence;
    xcb_out.initially_triggered = initially_triggered;
    memset(xcb_out.pad0, 0, 3);

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    fds[fd_index++] = fence_fd;
    xcb_ret.sequence = xcb_send_request_with_fds(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req, 1, fds);
    return xcb_ret;
}

xcb_void_cookie_t
xcb_dri3_fence_from_fd (xcb_connection_t *c,
                        xcb_drawable_t    drawable,
                        uint32_t          fence,
                        uint8_t           initially_triggered,
                        int32_t           fence_fd)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_dri3_id,
        .opcode = XCB_DRI3_FENCE_FROM_FD,
        .isvoid = 1
    };

    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_dri3_fence_from_fd_request_t xcb_out;
    int fds[1];
    int fd_index = 0;

    xcb_out.drawable = drawable;
    xcb_out.fence = fence;
    xcb_out.initially_triggered = initially_triggered;
    memset(xcb_out.pad0, 0, 3);

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    fds[fd_index++] = fence_fd;
    xcb_ret.sequence = xcb_send_request_with_fds(c, 0, xcb_parts + 2, &xcb_req, 1, fds);
    return xcb_ret;
}

xcb_dri3_fd_from_fence_cookie_t
xcb_dri3_fd_from_fence (xcb_connection_t *c,
                        xcb_drawable_t    drawable,
                        uint32_t          fence)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_dri3_id,
        .opcode = XCB_DRI3_FD_FROM_FENCE,
        .isvoid = 0
    };

    struct iovec xcb_parts[4];
    xcb_dri3_fd_from_fence_cookie_t xcb_ret;
    xcb_dri3_fd_from_fence_request_t xcb_out;

    xcb_out.drawable = drawable;
    xcb_out.fence = fence;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED|XCB_REQUEST_REPLY_FDS, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_dri3_fd_from_fence_cookie_t
xcb_dri3_fd_from_fence_unchecked (xcb_connection_t *c,
                                  xcb_drawable_t    drawable,
                                  uint32_t          fence)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_dri3_id,
        .opcode = XCB_DRI3_FD_FROM_FENCE,
        .isvoid = 0
    };

    struct iovec xcb_parts[4];
    xcb_dri3_fd_from_fence_cookie_t xcb_ret;
    xcb_dri3_fd_from_fence_request_t xcb_out;

    xcb_out.drawable = drawable;
    xcb_out.fence = fence;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_REPLY_FDS, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_dri3_fd_from_fence_reply_t *
xcb_dri3_fd_from_fence_reply (xcb_connection_t                 *c,
                              xcb_dri3_fd_from_fence_cookie_t   cookie  /**< */,
                              xcb_generic_error_t             **e)
{
    return (xcb_dri3_fd_from_fence_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}

int *
xcb_dri3_fd_from_fence_reply_fds (xcb_connection_t                *c  /**< */,
                                  xcb_dri3_fd_from_fence_reply_t  *reply)
{
    return xcb_get_reply_fds(c, reply, sizeof(xcb_dri3_fd_from_fence_reply_t) + 4 * reply->length);
}

int
xcb_dri3_get_supported_modifiers_sizeof (const void  *_buffer)
{
    char *xcb_tmp = (char *)_buffer;
    const xcb_dri3_get_supported_modifiers_reply_t *_aux = (xcb_dri3_get_supported_modifiers_reply_t *)_buffer;
    unsigned int xcb_buffer_len = 0;
    unsigned int xcb_block_len = 0;
    unsigned int xcb_pad = 0;
    unsigned int xcb_align_to = 0;


    xcb_block_len += sizeof(xcb_dri3_get_supported_modifiers_reply_t);
    xcb_tmp += xcb_block_len;
    xcb_buffer_len += xcb_block_len;
    xcb_block_len = 0;
    /* window_modifiers */
    xcb_block_len += _aux->num_window_modifiers * sizeof(uint64_t);
    xcb_tmp += xcb_block_len;
    xcb_align_to = ALIGNOF(uint64_t);
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_tmp += xcb_pad;
        xcb_pad = 0;
    }
    xcb_block_len = 0;
    /* screen_modifiers */
    xcb_block_len += _aux->num_screen_modifiers * sizeof(uint64_t);
    xcb_tmp += xcb_block_len;
    xcb_align_to = ALIGNOF(uint64_t);
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

xcb_dri3_get_supported_modifiers_cookie_t
xcb_dri3_get_supported_modifiers (xcb_connection_t *c,
                                  uint32_t          window,
                                  uint8_t           depth,
                                  uint8_t           bpp)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_dri3_id,
        .opcode = XCB_DRI3_GET_SUPPORTED_MODIFIERS,
        .isvoid = 0
    };

    struct iovec xcb_parts[4];
    xcb_dri3_get_supported_modifiers_cookie_t xcb_ret;
    xcb_dri3_get_supported_modifiers_request_t xcb_out;

    xcb_out.window = window;
    xcb_out.depth = depth;
    xcb_out.bpp = bpp;
    memset(xcb_out.pad0, 0, 2);

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_dri3_get_supported_modifiers_cookie_t
xcb_dri3_get_supported_modifiers_unchecked (xcb_connection_t *c,
                                            uint32_t          window,
                                            uint8_t           depth,
                                            uint8_t           bpp)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_dri3_id,
        .opcode = XCB_DRI3_GET_SUPPORTED_MODIFIERS,
        .isvoid = 0
    };

    struct iovec xcb_parts[4];
    xcb_dri3_get_supported_modifiers_cookie_t xcb_ret;
    xcb_dri3_get_supported_modifiers_request_t xcb_out;

    xcb_out.window = window;
    xcb_out.depth = depth;
    xcb_out.bpp = bpp;
    memset(xcb_out.pad0, 0, 2);

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

uint64_t *
xcb_dri3_get_supported_modifiers_window_modifiers (const xcb_dri3_get_supported_modifiers_reply_t *R)
{
    return (uint64_t *) (R + 1);
}

int
xcb_dri3_get_supported_modifiers_window_modifiers_length (const xcb_dri3_get_supported_modifiers_reply_t *R)
{
    return R->num_window_modifiers;
}

xcb_generic_iterator_t
xcb_dri3_get_supported_modifiers_window_modifiers_end (const xcb_dri3_get_supported_modifiers_reply_t *R)
{
    xcb_generic_iterator_t i;
    i.data = ((uint64_t *) (R + 1)) + (R->num_window_modifiers);
    i.rem = 0;
    i.index = (char *) i.data - (char *) R;
    return i;
}

uint64_t *
xcb_dri3_get_supported_modifiers_screen_modifiers (const xcb_dri3_get_supported_modifiers_reply_t *R)
{
    xcb_generic_iterator_t prev = xcb_dri3_get_supported_modifiers_window_modifiers_end(R);
    return (uint64_t *) ((char *) prev.data + XCB_TYPE_PAD(uint64_t, prev.index) + 0);
}

int
xcb_dri3_get_supported_modifiers_screen_modifiers_length (const xcb_dri3_get_supported_modifiers_reply_t *R)
{
    return R->num_screen_modifiers;
}

xcb_generic_iterator_t
xcb_dri3_get_supported_modifiers_screen_modifiers_end (const xcb_dri3_get_supported_modifiers_reply_t *R)
{
    xcb_generic_iterator_t i;
    xcb_generic_iterator_t prev = xcb_dri3_get_supported_modifiers_window_modifiers_end(R);
    i.data = ((uint64_t *) ((char*) prev.data + XCB_TYPE_PAD(uint64_t, prev.index))) + (R->num_screen_modifiers);
    i.rem = 0;
    i.index = (char *) i.data - (char *) R;
    return i;
}

xcb_dri3_get_supported_modifiers_reply_t *
xcb_dri3_get_supported_modifiers_reply (xcb_connection_t                           *c,
                                        xcb_dri3_get_supported_modifiers_cookie_t   cookie  /**< */,
                                        xcb_generic_error_t                       **e)
{
    return (xcb_dri3_get_supported_modifiers_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}

xcb_void_cookie_t
xcb_dri3_pixmap_from_buffers_checked (xcb_connection_t *c,
                                      xcb_pixmap_t      pixmap,
                                      xcb_window_t      window,
                                      uint8_t           num_buffers,
                                      uint16_t          width,
                                      uint16_t          height,
                                      uint32_t          stride0,
                                      uint32_t          offset0,
                                      uint32_t          stride1,
                                      uint32_t          offset1,
                                      uint32_t          stride2,
                                      uint32_t          offset2,
                                      uint32_t          stride3,
                                      uint32_t          offset3,
                                      uint8_t           depth,
                                      uint8_t           bpp,
                                      uint64_t          modifier,
                                      const int32_t    *buffers)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_dri3_id,
        .opcode = XCB_DRI3_PIXMAP_FROM_BUFFERS,
        .isvoid = 1
    };

    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_dri3_pixmap_from_buffers_request_t xcb_out;
    unsigned int i;
    int fds[num_buffers];
    int fd_index = 0;

    xcb_out.pixmap = pixmap;
    xcb_out.window = window;
    xcb_out.num_buffers = num_buffers;
    memset(xcb_out.pad0, 0, 3);
    xcb_out.width = width;
    xcb_out.height = height;
    xcb_out.stride0 = stride0;
    xcb_out.offset0 = offset0;
    xcb_out.stride1 = stride1;
    xcb_out.offset1 = offset1;
    xcb_out.stride2 = stride2;
    xcb_out.offset2 = offset2;
    xcb_out.stride3 = stride3;
    xcb_out.offset3 = offset3;
    xcb_out.depth = depth;
    xcb_out.bpp = bpp;
    memset(xcb_out.pad1, 0, 2);
    xcb_out.modifier = modifier;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    for (i = 0; i < num_buffers; i++)
        fds[fd_index++] = buffers[i];
    xcb_ret.sequence = xcb_send_request_with_fds(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req, num_buffers, fds);
    return xcb_ret;
}

xcb_void_cookie_t
xcb_dri3_pixmap_from_buffers (xcb_connection_t *c,
                              xcb_pixmap_t      pixmap,
                              xcb_window_t      window,
                              uint8_t           num_buffers,
                              uint16_t          width,
                              uint16_t          height,
                              uint32_t          stride0,
                              uint32_t          offset0,
                              uint32_t          stride1,
                              uint32_t          offset1,
                              uint32_t          stride2,
                              uint32_t          offset2,
                              uint32_t          stride3,
                              uint32_t          offset3,
                              uint8_t           depth,
                              uint8_t           bpp,
                              uint64_t          modifier,
                              const int32_t    *buffers)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_dri3_id,
        .opcode = XCB_DRI3_PIXMAP_FROM_BUFFERS,
        .isvoid = 1
    };

    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_dri3_pixmap_from_buffers_request_t xcb_out;
    unsigned int i;
    int fds[num_buffers];
    int fd_index = 0;

    xcb_out.pixmap = pixmap;
    xcb_out.window = window;
    xcb_out.num_buffers = num_buffers;
    memset(xcb_out.pad0, 0, 3);
    xcb_out.width = width;
    xcb_out.height = height;
    xcb_out.stride0 = stride0;
    xcb_out.offset0 = offset0;
    xcb_out.stride1 = stride1;
    xcb_out.offset1 = offset1;
    xcb_out.stride2 = stride2;
    xcb_out.offset2 = offset2;
    xcb_out.stride3 = stride3;
    xcb_out.offset3 = offset3;
    xcb_out.depth = depth;
    xcb_out.bpp = bpp;
    memset(xcb_out.pad1, 0, 2);
    xcb_out.modifier = modifier;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    for (i = 0; i < num_buffers; i++)
        fds[fd_index++] = buffers[i];
    xcb_ret.sequence = xcb_send_request_with_fds(c, 0, xcb_parts + 2, &xcb_req, num_buffers, fds);
    return xcb_ret;
}

int
xcb_dri3_buffers_from_pixmap_sizeof (const void  *_buffer,
                                     int32_t      buffers)
{
    char *xcb_tmp = (char *)_buffer;
    const xcb_dri3_buffers_from_pixmap_reply_t *_aux = (xcb_dri3_buffers_from_pixmap_reply_t *)_buffer;
    unsigned int xcb_buffer_len = 0;
    unsigned int xcb_block_len = 0;
    unsigned int xcb_pad = 0;
    unsigned int xcb_align_to = 0;


    xcb_block_len += sizeof(xcb_dri3_buffers_from_pixmap_reply_t);
    xcb_tmp += xcb_block_len;
    xcb_buffer_len += xcb_block_len;
    xcb_block_len = 0;
    /* strides */
    xcb_block_len += _aux->nfd * sizeof(uint32_t);
    xcb_tmp += xcb_block_len;
    xcb_align_to = ALIGNOF(uint32_t);
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_tmp += xcb_pad;
        xcb_pad = 0;
    }
    xcb_block_len = 0;
    /* offsets */
    xcb_block_len += _aux->nfd * sizeof(uint32_t);
    xcb_tmp += xcb_block_len;
    xcb_align_to = ALIGNOF(uint32_t);
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

xcb_dri3_buffers_from_pixmap_cookie_t
xcb_dri3_buffers_from_pixmap (xcb_connection_t *c,
                              xcb_pixmap_t      pixmap)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_dri3_id,
        .opcode = XCB_DRI3_BUFFERS_FROM_PIXMAP,
        .isvoid = 0
    };

    struct iovec xcb_parts[4];
    xcb_dri3_buffers_from_pixmap_cookie_t xcb_ret;
    xcb_dri3_buffers_from_pixmap_request_t xcb_out;

    xcb_out.pixmap = pixmap;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED|XCB_REQUEST_REPLY_FDS, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_dri3_buffers_from_pixmap_cookie_t
xcb_dri3_buffers_from_pixmap_unchecked (xcb_connection_t *c,
                                        xcb_pixmap_t      pixmap)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_dri3_id,
        .opcode = XCB_DRI3_BUFFERS_FROM_PIXMAP,
        .isvoid = 0
    };

    struct iovec xcb_parts[4];
    xcb_dri3_buffers_from_pixmap_cookie_t xcb_ret;
    xcb_dri3_buffers_from_pixmap_request_t xcb_out;

    xcb_out.pixmap = pixmap;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_REPLY_FDS, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

uint32_t *
xcb_dri3_buffers_from_pixmap_strides (const xcb_dri3_buffers_from_pixmap_reply_t *R)
{
    return (uint32_t *) (R + 1);
}

int
xcb_dri3_buffers_from_pixmap_strides_length (const xcb_dri3_buffers_from_pixmap_reply_t *R)
{
    return R->nfd;
}

xcb_generic_iterator_t
xcb_dri3_buffers_from_pixmap_strides_end (const xcb_dri3_buffers_from_pixmap_reply_t *R)
{
    xcb_generic_iterator_t i;
    i.data = ((uint32_t *) (R + 1)) + (R->nfd);
    i.rem = 0;
    i.index = (char *) i.data - (char *) R;
    return i;
}

uint32_t *
xcb_dri3_buffers_from_pixmap_offsets (const xcb_dri3_buffers_from_pixmap_reply_t *R)
{
    xcb_generic_iterator_t prev = xcb_dri3_buffers_from_pixmap_strides_end(R);
    return (uint32_t *) ((char *) prev.data + XCB_TYPE_PAD(uint32_t, prev.index) + 0);
}

int
xcb_dri3_buffers_from_pixmap_offsets_length (const xcb_dri3_buffers_from_pixmap_reply_t *R)
{
    return R->nfd;
}

xcb_generic_iterator_t
xcb_dri3_buffers_from_pixmap_offsets_end (const xcb_dri3_buffers_from_pixmap_reply_t *R)
{
    xcb_generic_iterator_t i;
    xcb_generic_iterator_t prev = xcb_dri3_buffers_from_pixmap_strides_end(R);
    i.data = ((uint32_t *) ((char*) prev.data + XCB_TYPE_PAD(uint32_t, prev.index))) + (R->nfd);
    i.rem = 0;
    i.index = (char *) i.data - (char *) R;
    return i;
}

int32_t *
xcb_dri3_buffers_from_pixmap_buffers (const xcb_dri3_buffers_from_pixmap_reply_t *R)
{
    xcb_generic_iterator_t prev = xcb_dri3_buffers_from_pixmap_offsets_end(R);
    return (int32_t *) ((char *) prev.data + XCB_TYPE_PAD(int32_t, prev.index) + 0);
}

int
xcb_dri3_buffers_from_pixmap_buffers_length (const xcb_dri3_buffers_from_pixmap_reply_t *R)
{
    return R->nfd;
}

xcb_generic_iterator_t
xcb_dri3_buffers_from_pixmap_buffers_end (const xcb_dri3_buffers_from_pixmap_reply_t *R)
{
    xcb_generic_iterator_t i;
    xcb_generic_iterator_t prev = xcb_dri3_buffers_from_pixmap_offsets_end(R);
    i.data = ((int32_t *) ((char*) prev.data + XCB_TYPE_PAD(int32_t, prev.index))) + (R->nfd);
    i.rem = 0;
    i.index = (char *) i.data - (char *) R;
    return i;
}

xcb_dri3_buffers_from_pixmap_reply_t *
xcb_dri3_buffers_from_pixmap_reply (xcb_connection_t                       *c,
                                    xcb_dri3_buffers_from_pixmap_cookie_t   cookie  /**< */,
                                    xcb_generic_error_t                   **e)
{
    return (xcb_dri3_buffers_from_pixmap_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}

int *
xcb_dri3_buffers_from_pixmap_reply_fds (xcb_connection_t                      *c  /**< */,
                                        xcb_dri3_buffers_from_pixmap_reply_t  *reply)
{
    return xcb_get_reply_fds(c, reply, sizeof(xcb_dri3_buffers_from_pixmap_reply_t) + 4 * reply->length);
}

