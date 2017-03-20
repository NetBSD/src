/*
 * This file generated automatically from screensaver.xml by c_client.py.
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
#include "screensaver.h"

#define ALIGNOF(type) offsetof(struct { char dummy; type member; }, member)
#include "xproto.h"

xcb_extension_t xcb_screensaver_id = { "MIT-SCREEN-SAVER", 0 };

xcb_screensaver_query_version_cookie_t
xcb_screensaver_query_version (xcb_connection_t *c,
                               uint8_t           client_major_version,
                               uint8_t           client_minor_version)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_screensaver_id,
        .opcode = XCB_SCREENSAVER_QUERY_VERSION,
        .isvoid = 0
    };

    struct iovec xcb_parts[4];
    xcb_screensaver_query_version_cookie_t xcb_ret;
    xcb_screensaver_query_version_request_t xcb_out;

    xcb_out.client_major_version = client_major_version;
    xcb_out.client_minor_version = client_minor_version;
    memset(xcb_out.pad0, 0, 2);

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_screensaver_query_version_cookie_t
xcb_screensaver_query_version_unchecked (xcb_connection_t *c,
                                         uint8_t           client_major_version,
                                         uint8_t           client_minor_version)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_screensaver_id,
        .opcode = XCB_SCREENSAVER_QUERY_VERSION,
        .isvoid = 0
    };

    struct iovec xcb_parts[4];
    xcb_screensaver_query_version_cookie_t xcb_ret;
    xcb_screensaver_query_version_request_t xcb_out;

    xcb_out.client_major_version = client_major_version;
    xcb_out.client_minor_version = client_minor_version;
    memset(xcb_out.pad0, 0, 2);

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_screensaver_query_version_reply_t *
xcb_screensaver_query_version_reply (xcb_connection_t                        *c,
                                     xcb_screensaver_query_version_cookie_t   cookie  /**< */,
                                     xcb_generic_error_t                    **e)
{
    return (xcb_screensaver_query_version_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}

xcb_screensaver_query_info_cookie_t
xcb_screensaver_query_info (xcb_connection_t *c,
                            xcb_drawable_t    drawable)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_screensaver_id,
        .opcode = XCB_SCREENSAVER_QUERY_INFO,
        .isvoid = 0
    };

    struct iovec xcb_parts[4];
    xcb_screensaver_query_info_cookie_t xcb_ret;
    xcb_screensaver_query_info_request_t xcb_out;

    xcb_out.drawable = drawable;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_screensaver_query_info_cookie_t
xcb_screensaver_query_info_unchecked (xcb_connection_t *c,
                                      xcb_drawable_t    drawable)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_screensaver_id,
        .opcode = XCB_SCREENSAVER_QUERY_INFO,
        .isvoid = 0
    };

    struct iovec xcb_parts[4];
    xcb_screensaver_query_info_cookie_t xcb_ret;
    xcb_screensaver_query_info_request_t xcb_out;

    xcb_out.drawable = drawable;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_screensaver_query_info_reply_t *
xcb_screensaver_query_info_reply (xcb_connection_t                     *c,
                                  xcb_screensaver_query_info_cookie_t   cookie  /**< */,
                                  xcb_generic_error_t                 **e)
{
    return (xcb_screensaver_query_info_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}

xcb_void_cookie_t
xcb_screensaver_select_input_checked (xcb_connection_t *c,
                                      xcb_drawable_t    drawable,
                                      uint32_t          event_mask)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_screensaver_id,
        .opcode = XCB_SCREENSAVER_SELECT_INPUT,
        .isvoid = 1
    };

    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_screensaver_select_input_request_t xcb_out;

    xcb_out.drawable = drawable;
    xcb_out.event_mask = event_mask;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_void_cookie_t
xcb_screensaver_select_input (xcb_connection_t *c,
                              xcb_drawable_t    drawable,
                              uint32_t          event_mask)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_screensaver_id,
        .opcode = XCB_SCREENSAVER_SELECT_INPUT,
        .isvoid = 1
    };

    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_screensaver_select_input_request_t xcb_out;

    xcb_out.drawable = drawable;
    xcb_out.event_mask = event_mask;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

int
xcb_screensaver_set_attributes_value_list_serialize (void                                              **_buffer,
                                                     uint32_t                                            value_mask,
                                                     const xcb_screensaver_set_attributes_value_list_t  *_aux)
{
    char *xcb_out = *_buffer;
    unsigned int xcb_buffer_len = 0;
    unsigned int xcb_align_to = 0;
    unsigned int xcb_padding_offset = 0;

    unsigned int xcb_pad = 0;
    char xcb_pad0[3] = {0, 0, 0};
    struct iovec xcb_parts[16];
    unsigned int xcb_parts_idx = 0;
    unsigned int xcb_block_len = 0;
    unsigned int i;
    char *xcb_tmp;

    if(value_mask & XCB_CW_BACK_PIXMAP) {
        /* xcb_screensaver_set_attributes_value_list_t.background_pixmap */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->background_pixmap;
        xcb_block_len += sizeof(xcb_pixmap_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(xcb_pixmap_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_pixmap_t);
    }
    if(value_mask & XCB_CW_BACK_PIXEL) {
        /* xcb_screensaver_set_attributes_value_list_t.background_pixel */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->background_pixel;
        xcb_block_len += sizeof(uint32_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint32_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint32_t);
    }
    if(value_mask & XCB_CW_BORDER_PIXMAP) {
        /* xcb_screensaver_set_attributes_value_list_t.border_pixmap */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->border_pixmap;
        xcb_block_len += sizeof(xcb_pixmap_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(xcb_pixmap_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_pixmap_t);
    }
    if(value_mask & XCB_CW_BORDER_PIXEL) {
        /* xcb_screensaver_set_attributes_value_list_t.border_pixel */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->border_pixel;
        xcb_block_len += sizeof(uint32_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint32_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint32_t);
    }
    if(value_mask & XCB_CW_BIT_GRAVITY) {
        /* xcb_screensaver_set_attributes_value_list_t.bit_gravity */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->bit_gravity;
        xcb_block_len += sizeof(uint32_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint32_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint32_t);
    }
    if(value_mask & XCB_CW_WIN_GRAVITY) {
        /* xcb_screensaver_set_attributes_value_list_t.win_gravity */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->win_gravity;
        xcb_block_len += sizeof(uint32_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint32_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint32_t);
    }
    if(value_mask & XCB_CW_BACKING_STORE) {
        /* xcb_screensaver_set_attributes_value_list_t.backing_store */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->backing_store;
        xcb_block_len += sizeof(uint32_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint32_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint32_t);
    }
    if(value_mask & XCB_CW_BACKING_PLANES) {
        /* xcb_screensaver_set_attributes_value_list_t.backing_planes */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->backing_planes;
        xcb_block_len += sizeof(uint32_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint32_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint32_t);
    }
    if(value_mask & XCB_CW_BACKING_PIXEL) {
        /* xcb_screensaver_set_attributes_value_list_t.backing_pixel */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->backing_pixel;
        xcb_block_len += sizeof(uint32_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint32_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint32_t);
    }
    if(value_mask & XCB_CW_OVERRIDE_REDIRECT) {
        /* xcb_screensaver_set_attributes_value_list_t.override_redirect */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->override_redirect;
        xcb_block_len += sizeof(xcb_bool32_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(xcb_bool32_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_bool32_t);
    }
    if(value_mask & XCB_CW_SAVE_UNDER) {
        /* xcb_screensaver_set_attributes_value_list_t.save_under */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->save_under;
        xcb_block_len += sizeof(xcb_bool32_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(xcb_bool32_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_bool32_t);
    }
    if(value_mask & XCB_CW_EVENT_MASK) {
        /* xcb_screensaver_set_attributes_value_list_t.event_mask */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->event_mask;
        xcb_block_len += sizeof(uint32_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint32_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint32_t);
    }
    if(value_mask & XCB_CW_DONT_PROPAGATE) {
        /* xcb_screensaver_set_attributes_value_list_t.do_not_propogate_mask */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->do_not_propogate_mask;
        xcb_block_len += sizeof(uint32_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint32_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint32_t);
    }
    if(value_mask & XCB_CW_COLORMAP) {
        /* xcb_screensaver_set_attributes_value_list_t.colormap */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->colormap;
        xcb_block_len += sizeof(xcb_colormap_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(xcb_colormap_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_colormap_t);
    }
    if(value_mask & XCB_CW_CURSOR) {
        /* xcb_screensaver_set_attributes_value_list_t.cursor */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->cursor;
        xcb_block_len += sizeof(xcb_cursor_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(xcb_cursor_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_cursor_t);
    }
    /* insert padding */
    xcb_pad = -(xcb_block_len + xcb_padding_offset) & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
        xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
        xcb_parts_idx++;
        xcb_pad = 0;
    }
    xcb_block_len = 0;
    xcb_padding_offset = 0;

    if (NULL == xcb_out) {
        /* allocate memory */
        xcb_out = malloc(xcb_buffer_len);
        *_buffer = xcb_out;
    }

    xcb_tmp = xcb_out;
    for(i=0; i<xcb_parts_idx; i++) {
        if (0 != xcb_parts[i].iov_base && 0 != xcb_parts[i].iov_len)
            memcpy(xcb_tmp, xcb_parts[i].iov_base, xcb_parts[i].iov_len);
        if (0 != xcb_parts[i].iov_len)
            xcb_tmp += xcb_parts[i].iov_len;
    }

    return xcb_buffer_len;
}

int
xcb_screensaver_set_attributes_value_list_unpack (const void                                   *_buffer,
                                                  uint32_t                                      value_mask,
                                                  xcb_screensaver_set_attributes_value_list_t  *_aux)
{
    char *xcb_tmp = (char *)_buffer;
    unsigned int xcb_buffer_len = 0;
    unsigned int xcb_block_len = 0;
    unsigned int xcb_pad = 0;
    unsigned int xcb_align_to = 0;
    unsigned int xcb_padding_offset = 0;


    if(value_mask & XCB_CW_BACK_PIXMAP) {
        /* xcb_screensaver_set_attributes_value_list_t.background_pixmap */
        _aux->background_pixmap = *(xcb_pixmap_t *)xcb_tmp;
        xcb_block_len += sizeof(xcb_pixmap_t);
        xcb_tmp += sizeof(xcb_pixmap_t);
        xcb_align_to = ALIGNOF(xcb_pixmap_t);
    }
    if(value_mask & XCB_CW_BACK_PIXEL) {
        /* xcb_screensaver_set_attributes_value_list_t.background_pixel */
        _aux->background_pixel = *(uint32_t *)xcb_tmp;
        xcb_block_len += sizeof(uint32_t);
        xcb_tmp += sizeof(uint32_t);
        xcb_align_to = ALIGNOF(uint32_t);
    }
    if(value_mask & XCB_CW_BORDER_PIXMAP) {
        /* xcb_screensaver_set_attributes_value_list_t.border_pixmap */
        _aux->border_pixmap = *(xcb_pixmap_t *)xcb_tmp;
        xcb_block_len += sizeof(xcb_pixmap_t);
        xcb_tmp += sizeof(xcb_pixmap_t);
        xcb_align_to = ALIGNOF(xcb_pixmap_t);
    }
    if(value_mask & XCB_CW_BORDER_PIXEL) {
        /* xcb_screensaver_set_attributes_value_list_t.border_pixel */
        _aux->border_pixel = *(uint32_t *)xcb_tmp;
        xcb_block_len += sizeof(uint32_t);
        xcb_tmp += sizeof(uint32_t);
        xcb_align_to = ALIGNOF(uint32_t);
    }
    if(value_mask & XCB_CW_BIT_GRAVITY) {
        /* xcb_screensaver_set_attributes_value_list_t.bit_gravity */
        _aux->bit_gravity = *(uint32_t *)xcb_tmp;
        xcb_block_len += sizeof(uint32_t);
        xcb_tmp += sizeof(uint32_t);
        xcb_align_to = ALIGNOF(uint32_t);
    }
    if(value_mask & XCB_CW_WIN_GRAVITY) {
        /* xcb_screensaver_set_attributes_value_list_t.win_gravity */
        _aux->win_gravity = *(uint32_t *)xcb_tmp;
        xcb_block_len += sizeof(uint32_t);
        xcb_tmp += sizeof(uint32_t);
        xcb_align_to = ALIGNOF(uint32_t);
    }
    if(value_mask & XCB_CW_BACKING_STORE) {
        /* xcb_screensaver_set_attributes_value_list_t.backing_store */
        _aux->backing_store = *(uint32_t *)xcb_tmp;
        xcb_block_len += sizeof(uint32_t);
        xcb_tmp += sizeof(uint32_t);
        xcb_align_to = ALIGNOF(uint32_t);
    }
    if(value_mask & XCB_CW_BACKING_PLANES) {
        /* xcb_screensaver_set_attributes_value_list_t.backing_planes */
        _aux->backing_planes = *(uint32_t *)xcb_tmp;
        xcb_block_len += sizeof(uint32_t);
        xcb_tmp += sizeof(uint32_t);
        xcb_align_to = ALIGNOF(uint32_t);
    }
    if(value_mask & XCB_CW_BACKING_PIXEL) {
        /* xcb_screensaver_set_attributes_value_list_t.backing_pixel */
        _aux->backing_pixel = *(uint32_t *)xcb_tmp;
        xcb_block_len += sizeof(uint32_t);
        xcb_tmp += sizeof(uint32_t);
        xcb_align_to = ALIGNOF(uint32_t);
    }
    if(value_mask & XCB_CW_OVERRIDE_REDIRECT) {
        /* xcb_screensaver_set_attributes_value_list_t.override_redirect */
        _aux->override_redirect = *(xcb_bool32_t *)xcb_tmp;
        xcb_block_len += sizeof(xcb_bool32_t);
        xcb_tmp += sizeof(xcb_bool32_t);
        xcb_align_to = ALIGNOF(xcb_bool32_t);
    }
    if(value_mask & XCB_CW_SAVE_UNDER) {
        /* xcb_screensaver_set_attributes_value_list_t.save_under */
        _aux->save_under = *(xcb_bool32_t *)xcb_tmp;
        xcb_block_len += sizeof(xcb_bool32_t);
        xcb_tmp += sizeof(xcb_bool32_t);
        xcb_align_to = ALIGNOF(xcb_bool32_t);
    }
    if(value_mask & XCB_CW_EVENT_MASK) {
        /* xcb_screensaver_set_attributes_value_list_t.event_mask */
        _aux->event_mask = *(uint32_t *)xcb_tmp;
        xcb_block_len += sizeof(uint32_t);
        xcb_tmp += sizeof(uint32_t);
        xcb_align_to = ALIGNOF(uint32_t);
    }
    if(value_mask & XCB_CW_DONT_PROPAGATE) {
        /* xcb_screensaver_set_attributes_value_list_t.do_not_propogate_mask */
        _aux->do_not_propogate_mask = *(uint32_t *)xcb_tmp;
        xcb_block_len += sizeof(uint32_t);
        xcb_tmp += sizeof(uint32_t);
        xcb_align_to = ALIGNOF(uint32_t);
    }
    if(value_mask & XCB_CW_COLORMAP) {
        /* xcb_screensaver_set_attributes_value_list_t.colormap */
        _aux->colormap = *(xcb_colormap_t *)xcb_tmp;
        xcb_block_len += sizeof(xcb_colormap_t);
        xcb_tmp += sizeof(xcb_colormap_t);
        xcb_align_to = ALIGNOF(xcb_colormap_t);
    }
    if(value_mask & XCB_CW_CURSOR) {
        /* xcb_screensaver_set_attributes_value_list_t.cursor */
        _aux->cursor = *(xcb_cursor_t *)xcb_tmp;
        xcb_block_len += sizeof(xcb_cursor_t);
        xcb_tmp += sizeof(xcb_cursor_t);
        xcb_align_to = ALIGNOF(xcb_cursor_t);
    }
    /* insert padding */
    xcb_pad = -(xcb_block_len + xcb_padding_offset) & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_tmp += xcb_pad;
        xcb_pad = 0;
    }
    xcb_block_len = 0;
    xcb_padding_offset = 0;

    return xcb_buffer_len;
}

int
xcb_screensaver_set_attributes_value_list_sizeof (const void  *_buffer,
                                                  uint32_t     value_mask)
{
    xcb_screensaver_set_attributes_value_list_t _aux;
    return xcb_screensaver_set_attributes_value_list_unpack(_buffer, value_mask, &_aux);
}

int
xcb_screensaver_set_attributes_sizeof (const void  *_buffer)
{
    char *xcb_tmp = (char *)_buffer;
    const xcb_screensaver_set_attributes_request_t *_aux = (xcb_screensaver_set_attributes_request_t *)_buffer;
    unsigned int xcb_buffer_len = 0;
    unsigned int xcb_block_len = 0;
    unsigned int xcb_pad = 0;
    unsigned int xcb_align_to = 0;


    xcb_block_len += sizeof(xcb_screensaver_set_attributes_request_t);
    xcb_tmp += xcb_block_len;
    xcb_buffer_len += xcb_block_len;
    xcb_block_len = 0;
    /* value_list */
    xcb_block_len += xcb_screensaver_set_attributes_value_list_sizeof(xcb_tmp, _aux->value_mask);
    xcb_tmp += xcb_block_len;
    xcb_align_to = ALIGNOF(char);
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
xcb_screensaver_set_attributes_checked (xcb_connection_t *c,
                                        xcb_drawable_t    drawable,
                                        int16_t           x,
                                        int16_t           y,
                                        uint16_t          width,
                                        uint16_t          height,
                                        uint16_t          border_width,
                                        uint8_t           _class,
                                        uint8_t           depth,
                                        xcb_visualid_t    visual,
                                        uint32_t          value_mask,
                                        const void       *value_list)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 3,
        .ext = &xcb_screensaver_id,
        .opcode = XCB_SCREENSAVER_SET_ATTRIBUTES,
        .isvoid = 1
    };

    struct iovec xcb_parts[5];
    xcb_void_cookie_t xcb_ret;
    xcb_screensaver_set_attributes_request_t xcb_out;

    xcb_out.drawable = drawable;
    xcb_out.x = x;
    xcb_out.y = y;
    xcb_out.width = width;
    xcb_out.height = height;
    xcb_out.border_width = border_width;
    xcb_out._class = _class;
    xcb_out.depth = depth;
    xcb_out.visual = visual;
    xcb_out.value_mask = value_mask;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    /* xcb_screensaver_set_attributes_value_list_t value_list */
    xcb_parts[4].iov_base = (char *) value_list;
    xcb_parts[4].iov_len =
      xcb_screensaver_set_attributes_value_list_sizeof (value_list, value_mask);

    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_void_cookie_t
xcb_screensaver_set_attributes (xcb_connection_t *c,
                                xcb_drawable_t    drawable,
                                int16_t           x,
                                int16_t           y,
                                uint16_t          width,
                                uint16_t          height,
                                uint16_t          border_width,
                                uint8_t           _class,
                                uint8_t           depth,
                                xcb_visualid_t    visual,
                                uint32_t          value_mask,
                                const void       *value_list)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 3,
        .ext = &xcb_screensaver_id,
        .opcode = XCB_SCREENSAVER_SET_ATTRIBUTES,
        .isvoid = 1
    };

    struct iovec xcb_parts[5];
    xcb_void_cookie_t xcb_ret;
    xcb_screensaver_set_attributes_request_t xcb_out;

    xcb_out.drawable = drawable;
    xcb_out.x = x;
    xcb_out.y = y;
    xcb_out.width = width;
    xcb_out.height = height;
    xcb_out.border_width = border_width;
    xcb_out._class = _class;
    xcb_out.depth = depth;
    xcb_out.visual = visual;
    xcb_out.value_mask = value_mask;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    /* xcb_screensaver_set_attributes_value_list_t value_list */
    xcb_parts[4].iov_base = (char *) value_list;
    xcb_parts[4].iov_len =
      xcb_screensaver_set_attributes_value_list_sizeof (value_list, value_mask);

    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_void_cookie_t
xcb_screensaver_set_attributes_aux_checked (xcb_connection_t                                  *c,
                                            xcb_drawable_t                                     drawable,
                                            int16_t                                            x,
                                            int16_t                                            y,
                                            uint16_t                                           width,
                                            uint16_t                                           height,
                                            uint16_t                                           border_width,
                                            uint8_t                                            _class,
                                            uint8_t                                            depth,
                                            xcb_visualid_t                                     visual,
                                            uint32_t                                           value_mask,
                                            const xcb_screensaver_set_attributes_value_list_t *value_list)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 3,
        .ext = &xcb_screensaver_id,
        .opcode = XCB_SCREENSAVER_SET_ATTRIBUTES,
        .isvoid = 1
    };

    struct iovec xcb_parts[5];
    xcb_void_cookie_t xcb_ret;
    xcb_screensaver_set_attributes_request_t xcb_out;
    void *xcb_aux0 = 0;

    xcb_out.drawable = drawable;
    xcb_out.x = x;
    xcb_out.y = y;
    xcb_out.width = width;
    xcb_out.height = height;
    xcb_out.border_width = border_width;
    xcb_out._class = _class;
    xcb_out.depth = depth;
    xcb_out.visual = visual;
    xcb_out.value_mask = value_mask;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    /* xcb_screensaver_set_attributes_value_list_t value_list */
    xcb_parts[4].iov_len =
      xcb_screensaver_set_attributes_value_list_serialize (&xcb_aux0, value_mask, value_list);
    xcb_parts[4].iov_base = xcb_aux0;

    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    free(xcb_aux0);
    return xcb_ret;
}

xcb_void_cookie_t
xcb_screensaver_set_attributes_aux (xcb_connection_t                                  *c,
                                    xcb_drawable_t                                     drawable,
                                    int16_t                                            x,
                                    int16_t                                            y,
                                    uint16_t                                           width,
                                    uint16_t                                           height,
                                    uint16_t                                           border_width,
                                    uint8_t                                            _class,
                                    uint8_t                                            depth,
                                    xcb_visualid_t                                     visual,
                                    uint32_t                                           value_mask,
                                    const xcb_screensaver_set_attributes_value_list_t *value_list)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 3,
        .ext = &xcb_screensaver_id,
        .opcode = XCB_SCREENSAVER_SET_ATTRIBUTES,
        .isvoid = 1
    };

    struct iovec xcb_parts[5];
    xcb_void_cookie_t xcb_ret;
    xcb_screensaver_set_attributes_request_t xcb_out;
    void *xcb_aux0 = 0;

    xcb_out.drawable = drawable;
    xcb_out.x = x;
    xcb_out.y = y;
    xcb_out.width = width;
    xcb_out.height = height;
    xcb_out.border_width = border_width;
    xcb_out._class = _class;
    xcb_out.depth = depth;
    xcb_out.visual = visual;
    xcb_out.value_mask = value_mask;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    /* xcb_screensaver_set_attributes_value_list_t value_list */
    xcb_parts[4].iov_len =
      xcb_screensaver_set_attributes_value_list_serialize (&xcb_aux0, value_mask, value_list);
    xcb_parts[4].iov_base = xcb_aux0;

    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    free(xcb_aux0);
    return xcb_ret;
}

void *
xcb_screensaver_set_attributes_value_list (const xcb_screensaver_set_attributes_request_t *R)
{
    return (void *) (R + 1);
}

xcb_void_cookie_t
xcb_screensaver_unset_attributes_checked (xcb_connection_t *c,
                                          xcb_drawable_t    drawable)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_screensaver_id,
        .opcode = XCB_SCREENSAVER_UNSET_ATTRIBUTES,
        .isvoid = 1
    };

    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_screensaver_unset_attributes_request_t xcb_out;

    xcb_out.drawable = drawable;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_void_cookie_t
xcb_screensaver_unset_attributes (xcb_connection_t *c,
                                  xcb_drawable_t    drawable)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_screensaver_id,
        .opcode = XCB_SCREENSAVER_UNSET_ATTRIBUTES,
        .isvoid = 1
    };

    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_screensaver_unset_attributes_request_t xcb_out;

    xcb_out.drawable = drawable;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_void_cookie_t
xcb_screensaver_suspend_checked (xcb_connection_t *c,
                                 uint8_t           suspend)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_screensaver_id,
        .opcode = XCB_SCREENSAVER_SUSPEND,
        .isvoid = 1
    };

    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_screensaver_suspend_request_t xcb_out;

    xcb_out.suspend = suspend;
    memset(xcb_out.pad0, 0, 3);

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_void_cookie_t
xcb_screensaver_suspend (xcb_connection_t *c,
                         uint8_t           suspend)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_screensaver_id,
        .opcode = XCB_SCREENSAVER_SUSPEND,
        .isvoid = 1
    };

    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_screensaver_suspend_request_t xcb_out;

    xcb_out.suspend = suspend;
    memset(xcb_out.pad0, 0, 3);

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

