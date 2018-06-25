/*
 * This file generated automatically from xf86vidmode.xml by c_client.py.
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
#include "xf86vidmode.h"

#define ALIGNOF(type) offsetof(struct { char dummy; type member; }, member)

xcb_extension_t xcb_xf86vidmode_id = { "XFree86-VidModeExtension", 0 };

void
xcb_xf86vidmode_syncrange_next (xcb_xf86vidmode_syncrange_iterator_t *i)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_xf86vidmode_syncrange_t);
}

xcb_generic_iterator_t
xcb_xf86vidmode_syncrange_end (xcb_xf86vidmode_syncrange_iterator_t i)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}

void
xcb_xf86vidmode_dotclock_next (xcb_xf86vidmode_dotclock_iterator_t *i)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_xf86vidmode_dotclock_t);
}

xcb_generic_iterator_t
xcb_xf86vidmode_dotclock_end (xcb_xf86vidmode_dotclock_iterator_t i)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}

void
xcb_xf86vidmode_mode_info_next (xcb_xf86vidmode_mode_info_iterator_t *i)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_xf86vidmode_mode_info_t);
}

xcb_generic_iterator_t
xcb_xf86vidmode_mode_info_end (xcb_xf86vidmode_mode_info_iterator_t i)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}

xcb_xf86vidmode_query_version_cookie_t
xcb_xf86vidmode_query_version (xcb_connection_t *c)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_xf86vidmode_id,
        .opcode = XCB_XF86VIDMODE_QUERY_VERSION,
        .isvoid = 0
    };

    struct iovec xcb_parts[4];
    xcb_xf86vidmode_query_version_cookie_t xcb_ret;
    xcb_xf86vidmode_query_version_request_t xcb_out;


    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_xf86vidmode_query_version_cookie_t
xcb_xf86vidmode_query_version_unchecked (xcb_connection_t *c)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_xf86vidmode_id,
        .opcode = XCB_XF86VIDMODE_QUERY_VERSION,
        .isvoid = 0
    };

    struct iovec xcb_parts[4];
    xcb_xf86vidmode_query_version_cookie_t xcb_ret;
    xcb_xf86vidmode_query_version_request_t xcb_out;


    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_xf86vidmode_query_version_reply_t *
xcb_xf86vidmode_query_version_reply (xcb_connection_t                        *c,
                                     xcb_xf86vidmode_query_version_cookie_t   cookie  /**< */,
                                     xcb_generic_error_t                    **e)
{
    return (xcb_xf86vidmode_query_version_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}

int
xcb_xf86vidmode_get_mode_line_sizeof (const void  *_buffer)
{
    char *xcb_tmp = (char *)_buffer;
    const xcb_xf86vidmode_get_mode_line_reply_t *_aux = (xcb_xf86vidmode_get_mode_line_reply_t *)_buffer;
    unsigned int xcb_buffer_len = 0;
    unsigned int xcb_block_len = 0;
    unsigned int xcb_pad = 0;
    unsigned int xcb_align_to = 0;


    xcb_block_len += sizeof(xcb_xf86vidmode_get_mode_line_reply_t);
    xcb_tmp += xcb_block_len;
    xcb_buffer_len += xcb_block_len;
    xcb_block_len = 0;
    /* private */
    xcb_block_len += _aux->privsize * sizeof(uint8_t);
    xcb_tmp += xcb_block_len;
    xcb_align_to = ALIGNOF(uint8_t);
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

xcb_xf86vidmode_get_mode_line_cookie_t
xcb_xf86vidmode_get_mode_line (xcb_connection_t *c,
                               uint16_t          screen)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_xf86vidmode_id,
        .opcode = XCB_XF86VIDMODE_GET_MODE_LINE,
        .isvoid = 0
    };

    struct iovec xcb_parts[4];
    xcb_xf86vidmode_get_mode_line_cookie_t xcb_ret;
    xcb_xf86vidmode_get_mode_line_request_t xcb_out;

    xcb_out.screen = screen;
    memset(xcb_out.pad0, 0, 2);

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_xf86vidmode_get_mode_line_cookie_t
xcb_xf86vidmode_get_mode_line_unchecked (xcb_connection_t *c,
                                         uint16_t          screen)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_xf86vidmode_id,
        .opcode = XCB_XF86VIDMODE_GET_MODE_LINE,
        .isvoid = 0
    };

    struct iovec xcb_parts[4];
    xcb_xf86vidmode_get_mode_line_cookie_t xcb_ret;
    xcb_xf86vidmode_get_mode_line_request_t xcb_out;

    xcb_out.screen = screen;
    memset(xcb_out.pad0, 0, 2);

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

uint8_t *
xcb_xf86vidmode_get_mode_line_private (const xcb_xf86vidmode_get_mode_line_reply_t *R)
{
    return (uint8_t *) (R + 1);
}

int
xcb_xf86vidmode_get_mode_line_private_length (const xcb_xf86vidmode_get_mode_line_reply_t *R)
{
    return R->privsize;
}

xcb_generic_iterator_t
xcb_xf86vidmode_get_mode_line_private_end (const xcb_xf86vidmode_get_mode_line_reply_t *R)
{
    xcb_generic_iterator_t i;
    i.data = ((uint8_t *) (R + 1)) + (R->privsize);
    i.rem = 0;
    i.index = (char *) i.data - (char *) R;
    return i;
}

xcb_xf86vidmode_get_mode_line_reply_t *
xcb_xf86vidmode_get_mode_line_reply (xcb_connection_t                        *c,
                                     xcb_xf86vidmode_get_mode_line_cookie_t   cookie  /**< */,
                                     xcb_generic_error_t                    **e)
{
    return (xcb_xf86vidmode_get_mode_line_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}

int
xcb_xf86vidmode_mod_mode_line_sizeof (const void  *_buffer)
{
    char *xcb_tmp = (char *)_buffer;
    const xcb_xf86vidmode_mod_mode_line_request_t *_aux = (xcb_xf86vidmode_mod_mode_line_request_t *)_buffer;
    unsigned int xcb_buffer_len = 0;
    unsigned int xcb_block_len = 0;
    unsigned int xcb_pad = 0;
    unsigned int xcb_align_to = 0;


    xcb_block_len += sizeof(xcb_xf86vidmode_mod_mode_line_request_t);
    xcb_tmp += xcb_block_len;
    xcb_buffer_len += xcb_block_len;
    xcb_block_len = 0;
    /* private */
    xcb_block_len += _aux->privsize * sizeof(uint8_t);
    xcb_tmp += xcb_block_len;
    xcb_align_to = ALIGNOF(uint8_t);
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
xcb_xf86vidmode_mod_mode_line_checked (xcb_connection_t *c,
                                       uint32_t          screen,
                                       uint16_t          hdisplay,
                                       uint16_t          hsyncstart,
                                       uint16_t          hsyncend,
                                       uint16_t          htotal,
                                       uint16_t          hskew,
                                       uint16_t          vdisplay,
                                       uint16_t          vsyncstart,
                                       uint16_t          vsyncend,
                                       uint16_t          vtotal,
                                       uint32_t          flags,
                                       uint32_t          privsize,
                                       const uint8_t    *private)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 4,
        .ext = &xcb_xf86vidmode_id,
        .opcode = XCB_XF86VIDMODE_MOD_MODE_LINE,
        .isvoid = 1
    };

    struct iovec xcb_parts[6];
    xcb_void_cookie_t xcb_ret;
    xcb_xf86vidmode_mod_mode_line_request_t xcb_out;

    xcb_out.screen = screen;
    xcb_out.hdisplay = hdisplay;
    xcb_out.hsyncstart = hsyncstart;
    xcb_out.hsyncend = hsyncend;
    xcb_out.htotal = htotal;
    xcb_out.hskew = hskew;
    xcb_out.vdisplay = vdisplay;
    xcb_out.vsyncstart = vsyncstart;
    xcb_out.vsyncend = vsyncend;
    xcb_out.vtotal = vtotal;
    memset(xcb_out.pad0, 0, 2);
    xcb_out.flags = flags;
    memset(xcb_out.pad1, 0, 12);
    xcb_out.privsize = privsize;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    /* uint8_t private */
    xcb_parts[4].iov_base = (char *) private;
    xcb_parts[4].iov_len = privsize * sizeof(uint8_t);
    xcb_parts[5].iov_base = 0;
    xcb_parts[5].iov_len = -xcb_parts[4].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_void_cookie_t
xcb_xf86vidmode_mod_mode_line (xcb_connection_t *c,
                               uint32_t          screen,
                               uint16_t          hdisplay,
                               uint16_t          hsyncstart,
                               uint16_t          hsyncend,
                               uint16_t          htotal,
                               uint16_t          hskew,
                               uint16_t          vdisplay,
                               uint16_t          vsyncstart,
                               uint16_t          vsyncend,
                               uint16_t          vtotal,
                               uint32_t          flags,
                               uint32_t          privsize,
                               const uint8_t    *private)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 4,
        .ext = &xcb_xf86vidmode_id,
        .opcode = XCB_XF86VIDMODE_MOD_MODE_LINE,
        .isvoid = 1
    };

    struct iovec xcb_parts[6];
    xcb_void_cookie_t xcb_ret;
    xcb_xf86vidmode_mod_mode_line_request_t xcb_out;

    xcb_out.screen = screen;
    xcb_out.hdisplay = hdisplay;
    xcb_out.hsyncstart = hsyncstart;
    xcb_out.hsyncend = hsyncend;
    xcb_out.htotal = htotal;
    xcb_out.hskew = hskew;
    xcb_out.vdisplay = vdisplay;
    xcb_out.vsyncstart = vsyncstart;
    xcb_out.vsyncend = vsyncend;
    xcb_out.vtotal = vtotal;
    memset(xcb_out.pad0, 0, 2);
    xcb_out.flags = flags;
    memset(xcb_out.pad1, 0, 12);
    xcb_out.privsize = privsize;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    /* uint8_t private */
    xcb_parts[4].iov_base = (char *) private;
    xcb_parts[4].iov_len = privsize * sizeof(uint8_t);
    xcb_parts[5].iov_base = 0;
    xcb_parts[5].iov_len = -xcb_parts[4].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

uint8_t *
xcb_xf86vidmode_mod_mode_line_private (const xcb_xf86vidmode_mod_mode_line_request_t *R)
{
    return (uint8_t *) (R + 1);
}

int
xcb_xf86vidmode_mod_mode_line_private_length (const xcb_xf86vidmode_mod_mode_line_request_t *R)
{
    return R->privsize;
}

xcb_generic_iterator_t
xcb_xf86vidmode_mod_mode_line_private_end (const xcb_xf86vidmode_mod_mode_line_request_t *R)
{
    xcb_generic_iterator_t i;
    i.data = ((uint8_t *) (R + 1)) + (R->privsize);
    i.rem = 0;
    i.index = (char *) i.data - (char *) R;
    return i;
}

xcb_void_cookie_t
xcb_xf86vidmode_switch_mode_checked (xcb_connection_t *c,
                                     uint16_t          screen,
                                     uint16_t          zoom)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_xf86vidmode_id,
        .opcode = XCB_XF86VIDMODE_SWITCH_MODE,
        .isvoid = 1
    };

    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xf86vidmode_switch_mode_request_t xcb_out;

    xcb_out.screen = screen;
    xcb_out.zoom = zoom;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_void_cookie_t
xcb_xf86vidmode_switch_mode (xcb_connection_t *c,
                             uint16_t          screen,
                             uint16_t          zoom)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_xf86vidmode_id,
        .opcode = XCB_XF86VIDMODE_SWITCH_MODE,
        .isvoid = 1
    };

    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xf86vidmode_switch_mode_request_t xcb_out;

    xcb_out.screen = screen;
    xcb_out.zoom = zoom;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

int
xcb_xf86vidmode_get_monitor_sizeof (const void  *_buffer)
{
    char *xcb_tmp = (char *)_buffer;
    const xcb_xf86vidmode_get_monitor_reply_t *_aux = (xcb_xf86vidmode_get_monitor_reply_t *)_buffer;
    unsigned int xcb_buffer_len = 0;
    unsigned int xcb_block_len = 0;
    unsigned int xcb_pad = 0;
    unsigned int xcb_align_to = 0;


    xcb_block_len += sizeof(xcb_xf86vidmode_get_monitor_reply_t);
    xcb_tmp += xcb_block_len;
    xcb_buffer_len += xcb_block_len;
    xcb_block_len = 0;
    /* hsync */
    xcb_block_len += _aux->num_hsync * sizeof(xcb_xf86vidmode_syncrange_t);
    xcb_tmp += xcb_block_len;
    xcb_align_to = ALIGNOF(xcb_xf86vidmode_syncrange_t);
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_tmp += xcb_pad;
        xcb_pad = 0;
    }
    xcb_block_len = 0;
    /* vsync */
    xcb_block_len += _aux->num_vsync * sizeof(xcb_xf86vidmode_syncrange_t);
    xcb_tmp += xcb_block_len;
    xcb_align_to = ALIGNOF(xcb_xf86vidmode_syncrange_t);
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_tmp += xcb_pad;
        xcb_pad = 0;
    }
    xcb_block_len = 0;
    /* vendor */
    xcb_block_len += _aux->vendor_length * sizeof(char);
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
    /* alignment_pad */
    xcb_block_len += (((_aux->vendor_length + 3) & (~3)) - _aux->vendor_length) * sizeof(char);
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
    /* model */
    xcb_block_len += _aux->model_length * sizeof(char);
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

xcb_xf86vidmode_get_monitor_cookie_t
xcb_xf86vidmode_get_monitor (xcb_connection_t *c,
                             uint16_t          screen)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_xf86vidmode_id,
        .opcode = XCB_XF86VIDMODE_GET_MONITOR,
        .isvoid = 0
    };

    struct iovec xcb_parts[4];
    xcb_xf86vidmode_get_monitor_cookie_t xcb_ret;
    xcb_xf86vidmode_get_monitor_request_t xcb_out;

    xcb_out.screen = screen;
    memset(xcb_out.pad0, 0, 2);

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_xf86vidmode_get_monitor_cookie_t
xcb_xf86vidmode_get_monitor_unchecked (xcb_connection_t *c,
                                       uint16_t          screen)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_xf86vidmode_id,
        .opcode = XCB_XF86VIDMODE_GET_MONITOR,
        .isvoid = 0
    };

    struct iovec xcb_parts[4];
    xcb_xf86vidmode_get_monitor_cookie_t xcb_ret;
    xcb_xf86vidmode_get_monitor_request_t xcb_out;

    xcb_out.screen = screen;
    memset(xcb_out.pad0, 0, 2);

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_xf86vidmode_syncrange_t *
xcb_xf86vidmode_get_monitor_hsync (const xcb_xf86vidmode_get_monitor_reply_t *R)
{
    return (xcb_xf86vidmode_syncrange_t *) (R + 1);
}

int
xcb_xf86vidmode_get_monitor_hsync_length (const xcb_xf86vidmode_get_monitor_reply_t *R)
{
    return R->num_hsync;
}

xcb_generic_iterator_t
xcb_xf86vidmode_get_monitor_hsync_end (const xcb_xf86vidmode_get_monitor_reply_t *R)
{
    xcb_generic_iterator_t i;
    i.data = ((xcb_xf86vidmode_syncrange_t *) (R + 1)) + (R->num_hsync);
    i.rem = 0;
    i.index = (char *) i.data - (char *) R;
    return i;
}

xcb_xf86vidmode_syncrange_t *
xcb_xf86vidmode_get_monitor_vsync (const xcb_xf86vidmode_get_monitor_reply_t *R)
{
    xcb_generic_iterator_t prev = xcb_xf86vidmode_get_monitor_hsync_end(R);
    return (xcb_xf86vidmode_syncrange_t *) ((char *) prev.data + XCB_TYPE_PAD(xcb_xf86vidmode_syncrange_t, prev.index) + 0);
}

int
xcb_xf86vidmode_get_monitor_vsync_length (const xcb_xf86vidmode_get_monitor_reply_t *R)
{
    return R->num_vsync;
}

xcb_generic_iterator_t
xcb_xf86vidmode_get_monitor_vsync_end (const xcb_xf86vidmode_get_monitor_reply_t *R)
{
    xcb_generic_iterator_t i;
    xcb_generic_iterator_t prev = xcb_xf86vidmode_get_monitor_hsync_end(R);
    i.data = ((xcb_xf86vidmode_syncrange_t *) ((char*) prev.data + XCB_TYPE_PAD(xcb_xf86vidmode_syncrange_t, prev.index))) + (R->num_vsync);
    i.rem = 0;
    i.index = (char *) i.data - (char *) R;
    return i;
}

char *
xcb_xf86vidmode_get_monitor_vendor (const xcb_xf86vidmode_get_monitor_reply_t *R)
{
    xcb_generic_iterator_t prev = xcb_xf86vidmode_get_monitor_vsync_end(R);
    return (char *) ((char *) prev.data + XCB_TYPE_PAD(char, prev.index) + 0);
}

int
xcb_xf86vidmode_get_monitor_vendor_length (const xcb_xf86vidmode_get_monitor_reply_t *R)
{
    return R->vendor_length;
}

xcb_generic_iterator_t
xcb_xf86vidmode_get_monitor_vendor_end (const xcb_xf86vidmode_get_monitor_reply_t *R)
{
    xcb_generic_iterator_t i;
    xcb_generic_iterator_t prev = xcb_xf86vidmode_get_monitor_vsync_end(R);
    i.data = ((char *) ((char*) prev.data + XCB_TYPE_PAD(char, prev.index))) + (R->vendor_length);
    i.rem = 0;
    i.index = (char *) i.data - (char *) R;
    return i;
}

void *
xcb_xf86vidmode_get_monitor_alignment_pad (const xcb_xf86vidmode_get_monitor_reply_t *R)
{
    xcb_generic_iterator_t prev = xcb_xf86vidmode_get_monitor_vendor_end(R);
    return (void *) ((char *) prev.data + XCB_TYPE_PAD(char, prev.index) + 0);
}

int
xcb_xf86vidmode_get_monitor_alignment_pad_length (const xcb_xf86vidmode_get_monitor_reply_t *R)
{
    return (((R->vendor_length + 3) & (~3)) - R->vendor_length);
}

xcb_generic_iterator_t
xcb_xf86vidmode_get_monitor_alignment_pad_end (const xcb_xf86vidmode_get_monitor_reply_t *R)
{
    xcb_generic_iterator_t i;
    xcb_generic_iterator_t prev = xcb_xf86vidmode_get_monitor_vendor_end(R);
    i.data = ((char *) ((char*) prev.data + XCB_TYPE_PAD(char, prev.index))) + ((((R->vendor_length + 3) & (~3)) - R->vendor_length));
    i.rem = 0;
    i.index = (char *) i.data - (char *) R;
    return i;
}

char *
xcb_xf86vidmode_get_monitor_model (const xcb_xf86vidmode_get_monitor_reply_t *R)
{
    xcb_generic_iterator_t prev = xcb_xf86vidmode_get_monitor_alignment_pad_end(R);
    return (char *) ((char *) prev.data + XCB_TYPE_PAD(char, prev.index) + 0);
}

int
xcb_xf86vidmode_get_monitor_model_length (const xcb_xf86vidmode_get_monitor_reply_t *R)
{
    return R->model_length;
}

xcb_generic_iterator_t
xcb_xf86vidmode_get_monitor_model_end (const xcb_xf86vidmode_get_monitor_reply_t *R)
{
    xcb_generic_iterator_t i;
    xcb_generic_iterator_t prev = xcb_xf86vidmode_get_monitor_alignment_pad_end(R);
    i.data = ((char *) ((char*) prev.data + XCB_TYPE_PAD(char, prev.index))) + (R->model_length);
    i.rem = 0;
    i.index = (char *) i.data - (char *) R;
    return i;
}

xcb_xf86vidmode_get_monitor_reply_t *
xcb_xf86vidmode_get_monitor_reply (xcb_connection_t                      *c,
                                   xcb_xf86vidmode_get_monitor_cookie_t   cookie  /**< */,
                                   xcb_generic_error_t                  **e)
{
    return (xcb_xf86vidmode_get_monitor_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}

xcb_void_cookie_t
xcb_xf86vidmode_lock_mode_switch_checked (xcb_connection_t *c,
                                          uint16_t          screen,
                                          uint16_t          lock)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_xf86vidmode_id,
        .opcode = XCB_XF86VIDMODE_LOCK_MODE_SWITCH,
        .isvoid = 1
    };

    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xf86vidmode_lock_mode_switch_request_t xcb_out;

    xcb_out.screen = screen;
    xcb_out.lock = lock;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_void_cookie_t
xcb_xf86vidmode_lock_mode_switch (xcb_connection_t *c,
                                  uint16_t          screen,
                                  uint16_t          lock)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_xf86vidmode_id,
        .opcode = XCB_XF86VIDMODE_LOCK_MODE_SWITCH,
        .isvoid = 1
    };

    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xf86vidmode_lock_mode_switch_request_t xcb_out;

    xcb_out.screen = screen;
    xcb_out.lock = lock;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

int
xcb_xf86vidmode_get_all_mode_lines_sizeof (const void  *_buffer)
{
    char *xcb_tmp = (char *)_buffer;
    const xcb_xf86vidmode_get_all_mode_lines_reply_t *_aux = (xcb_xf86vidmode_get_all_mode_lines_reply_t *)_buffer;
    unsigned int xcb_buffer_len = 0;
    unsigned int xcb_block_len = 0;
    unsigned int xcb_pad = 0;
    unsigned int xcb_align_to = 0;


    xcb_block_len += sizeof(xcb_xf86vidmode_get_all_mode_lines_reply_t);
    xcb_tmp += xcb_block_len;
    xcb_buffer_len += xcb_block_len;
    xcb_block_len = 0;
    /* modeinfo */
    xcb_block_len += _aux->modecount * sizeof(xcb_xf86vidmode_mode_info_t);
    xcb_tmp += xcb_block_len;
    xcb_align_to = ALIGNOF(xcb_xf86vidmode_mode_info_t);
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

xcb_xf86vidmode_get_all_mode_lines_cookie_t
xcb_xf86vidmode_get_all_mode_lines (xcb_connection_t *c,
                                    uint16_t          screen)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_xf86vidmode_id,
        .opcode = XCB_XF86VIDMODE_GET_ALL_MODE_LINES,
        .isvoid = 0
    };

    struct iovec xcb_parts[4];
    xcb_xf86vidmode_get_all_mode_lines_cookie_t xcb_ret;
    xcb_xf86vidmode_get_all_mode_lines_request_t xcb_out;

    xcb_out.screen = screen;
    memset(xcb_out.pad0, 0, 2);

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_xf86vidmode_get_all_mode_lines_cookie_t
xcb_xf86vidmode_get_all_mode_lines_unchecked (xcb_connection_t *c,
                                              uint16_t          screen)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_xf86vidmode_id,
        .opcode = XCB_XF86VIDMODE_GET_ALL_MODE_LINES,
        .isvoid = 0
    };

    struct iovec xcb_parts[4];
    xcb_xf86vidmode_get_all_mode_lines_cookie_t xcb_ret;
    xcb_xf86vidmode_get_all_mode_lines_request_t xcb_out;

    xcb_out.screen = screen;
    memset(xcb_out.pad0, 0, 2);

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_xf86vidmode_mode_info_t *
xcb_xf86vidmode_get_all_mode_lines_modeinfo (const xcb_xf86vidmode_get_all_mode_lines_reply_t *R)
{
    return (xcb_xf86vidmode_mode_info_t *) (R + 1);
}

int
xcb_xf86vidmode_get_all_mode_lines_modeinfo_length (const xcb_xf86vidmode_get_all_mode_lines_reply_t *R)
{
    return R->modecount;
}

xcb_xf86vidmode_mode_info_iterator_t
xcb_xf86vidmode_get_all_mode_lines_modeinfo_iterator (const xcb_xf86vidmode_get_all_mode_lines_reply_t *R)
{
    xcb_xf86vidmode_mode_info_iterator_t i;
    i.data = (xcb_xf86vidmode_mode_info_t *) (R + 1);
    i.rem = R->modecount;
    i.index = (char *) i.data - (char *) R;
    return i;
}

xcb_xf86vidmode_get_all_mode_lines_reply_t *
xcb_xf86vidmode_get_all_mode_lines_reply (xcb_connection_t                             *c,
                                          xcb_xf86vidmode_get_all_mode_lines_cookie_t   cookie  /**< */,
                                          xcb_generic_error_t                         **e)
{
    return (xcb_xf86vidmode_get_all_mode_lines_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}

int
xcb_xf86vidmode_add_mode_line_sizeof (const void  *_buffer)
{
    char *xcb_tmp = (char *)_buffer;
    const xcb_xf86vidmode_add_mode_line_request_t *_aux = (xcb_xf86vidmode_add_mode_line_request_t *)_buffer;
    unsigned int xcb_buffer_len = 0;
    unsigned int xcb_block_len = 0;
    unsigned int xcb_pad = 0;
    unsigned int xcb_align_to = 0;


    xcb_block_len += sizeof(xcb_xf86vidmode_add_mode_line_request_t);
    xcb_tmp += xcb_block_len;
    xcb_buffer_len += xcb_block_len;
    xcb_block_len = 0;
    /* private */
    xcb_block_len += _aux->privsize * sizeof(uint8_t);
    xcb_tmp += xcb_block_len;
    xcb_align_to = ALIGNOF(uint8_t);
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
xcb_xf86vidmode_add_mode_line_checked (xcb_connection_t           *c,
                                       uint32_t                    screen,
                                       xcb_xf86vidmode_dotclock_t  dotclock,
                                       uint16_t                    hdisplay,
                                       uint16_t                    hsyncstart,
                                       uint16_t                    hsyncend,
                                       uint16_t                    htotal,
                                       uint16_t                    hskew,
                                       uint16_t                    vdisplay,
                                       uint16_t                    vsyncstart,
                                       uint16_t                    vsyncend,
                                       uint16_t                    vtotal,
                                       uint32_t                    flags,
                                       uint32_t                    privsize,
                                       xcb_xf86vidmode_dotclock_t  after_dotclock,
                                       uint16_t                    after_hdisplay,
                                       uint16_t                    after_hsyncstart,
                                       uint16_t                    after_hsyncend,
                                       uint16_t                    after_htotal,
                                       uint16_t                    after_hskew,
                                       uint16_t                    after_vdisplay,
                                       uint16_t                    after_vsyncstart,
                                       uint16_t                    after_vsyncend,
                                       uint16_t                    after_vtotal,
                                       uint32_t                    after_flags,
                                       const uint8_t              *private)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 4,
        .ext = &xcb_xf86vidmode_id,
        .opcode = XCB_XF86VIDMODE_ADD_MODE_LINE,
        .isvoid = 1
    };

    struct iovec xcb_parts[6];
    xcb_void_cookie_t xcb_ret;
    xcb_xf86vidmode_add_mode_line_request_t xcb_out;

    xcb_out.screen = screen;
    xcb_out.dotclock = dotclock;
    xcb_out.hdisplay = hdisplay;
    xcb_out.hsyncstart = hsyncstart;
    xcb_out.hsyncend = hsyncend;
    xcb_out.htotal = htotal;
    xcb_out.hskew = hskew;
    xcb_out.vdisplay = vdisplay;
    xcb_out.vsyncstart = vsyncstart;
    xcb_out.vsyncend = vsyncend;
    xcb_out.vtotal = vtotal;
    memset(xcb_out.pad0, 0, 2);
    xcb_out.flags = flags;
    memset(xcb_out.pad1, 0, 12);
    xcb_out.privsize = privsize;
    xcb_out.after_dotclock = after_dotclock;
    xcb_out.after_hdisplay = after_hdisplay;
    xcb_out.after_hsyncstart = after_hsyncstart;
    xcb_out.after_hsyncend = after_hsyncend;
    xcb_out.after_htotal = after_htotal;
    xcb_out.after_hskew = after_hskew;
    xcb_out.after_vdisplay = after_vdisplay;
    xcb_out.after_vsyncstart = after_vsyncstart;
    xcb_out.after_vsyncend = after_vsyncend;
    xcb_out.after_vtotal = after_vtotal;
    memset(xcb_out.pad2, 0, 2);
    xcb_out.after_flags = after_flags;
    memset(xcb_out.pad3, 0, 12);

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    /* uint8_t private */
    xcb_parts[4].iov_base = (char *) private;
    xcb_parts[4].iov_len = privsize * sizeof(uint8_t);
    xcb_parts[5].iov_base = 0;
    xcb_parts[5].iov_len = -xcb_parts[4].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_void_cookie_t
xcb_xf86vidmode_add_mode_line (xcb_connection_t           *c,
                               uint32_t                    screen,
                               xcb_xf86vidmode_dotclock_t  dotclock,
                               uint16_t                    hdisplay,
                               uint16_t                    hsyncstart,
                               uint16_t                    hsyncend,
                               uint16_t                    htotal,
                               uint16_t                    hskew,
                               uint16_t                    vdisplay,
                               uint16_t                    vsyncstart,
                               uint16_t                    vsyncend,
                               uint16_t                    vtotal,
                               uint32_t                    flags,
                               uint32_t                    privsize,
                               xcb_xf86vidmode_dotclock_t  after_dotclock,
                               uint16_t                    after_hdisplay,
                               uint16_t                    after_hsyncstart,
                               uint16_t                    after_hsyncend,
                               uint16_t                    after_htotal,
                               uint16_t                    after_hskew,
                               uint16_t                    after_vdisplay,
                               uint16_t                    after_vsyncstart,
                               uint16_t                    after_vsyncend,
                               uint16_t                    after_vtotal,
                               uint32_t                    after_flags,
                               const uint8_t              *private)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 4,
        .ext = &xcb_xf86vidmode_id,
        .opcode = XCB_XF86VIDMODE_ADD_MODE_LINE,
        .isvoid = 1
    };

    struct iovec xcb_parts[6];
    xcb_void_cookie_t xcb_ret;
    xcb_xf86vidmode_add_mode_line_request_t xcb_out;

    xcb_out.screen = screen;
    xcb_out.dotclock = dotclock;
    xcb_out.hdisplay = hdisplay;
    xcb_out.hsyncstart = hsyncstart;
    xcb_out.hsyncend = hsyncend;
    xcb_out.htotal = htotal;
    xcb_out.hskew = hskew;
    xcb_out.vdisplay = vdisplay;
    xcb_out.vsyncstart = vsyncstart;
    xcb_out.vsyncend = vsyncend;
    xcb_out.vtotal = vtotal;
    memset(xcb_out.pad0, 0, 2);
    xcb_out.flags = flags;
    memset(xcb_out.pad1, 0, 12);
    xcb_out.privsize = privsize;
    xcb_out.after_dotclock = after_dotclock;
    xcb_out.after_hdisplay = after_hdisplay;
    xcb_out.after_hsyncstart = after_hsyncstart;
    xcb_out.after_hsyncend = after_hsyncend;
    xcb_out.after_htotal = after_htotal;
    xcb_out.after_hskew = after_hskew;
    xcb_out.after_vdisplay = after_vdisplay;
    xcb_out.after_vsyncstart = after_vsyncstart;
    xcb_out.after_vsyncend = after_vsyncend;
    xcb_out.after_vtotal = after_vtotal;
    memset(xcb_out.pad2, 0, 2);
    xcb_out.after_flags = after_flags;
    memset(xcb_out.pad3, 0, 12);

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    /* uint8_t private */
    xcb_parts[4].iov_base = (char *) private;
    xcb_parts[4].iov_len = privsize * sizeof(uint8_t);
    xcb_parts[5].iov_base = 0;
    xcb_parts[5].iov_len = -xcb_parts[4].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

uint8_t *
xcb_xf86vidmode_add_mode_line_private (const xcb_xf86vidmode_add_mode_line_request_t *R)
{
    return (uint8_t *) (R + 1);
}

int
xcb_xf86vidmode_add_mode_line_private_length (const xcb_xf86vidmode_add_mode_line_request_t *R)
{
    return R->privsize;
}

xcb_generic_iterator_t
xcb_xf86vidmode_add_mode_line_private_end (const xcb_xf86vidmode_add_mode_line_request_t *R)
{
    xcb_generic_iterator_t i;
    i.data = ((uint8_t *) (R + 1)) + (R->privsize);
    i.rem = 0;
    i.index = (char *) i.data - (char *) R;
    return i;
}

int
xcb_xf86vidmode_delete_mode_line_sizeof (const void  *_buffer)
{
    char *xcb_tmp = (char *)_buffer;
    const xcb_xf86vidmode_delete_mode_line_request_t *_aux = (xcb_xf86vidmode_delete_mode_line_request_t *)_buffer;
    unsigned int xcb_buffer_len = 0;
    unsigned int xcb_block_len = 0;
    unsigned int xcb_pad = 0;
    unsigned int xcb_align_to = 0;


    xcb_block_len += sizeof(xcb_xf86vidmode_delete_mode_line_request_t);
    xcb_tmp += xcb_block_len;
    xcb_buffer_len += xcb_block_len;
    xcb_block_len = 0;
    /* private */
    xcb_block_len += _aux->privsize * sizeof(uint8_t);
    xcb_tmp += xcb_block_len;
    xcb_align_to = ALIGNOF(uint8_t);
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
xcb_xf86vidmode_delete_mode_line_checked (xcb_connection_t           *c,
                                          uint32_t                    screen,
                                          xcb_xf86vidmode_dotclock_t  dotclock,
                                          uint16_t                    hdisplay,
                                          uint16_t                    hsyncstart,
                                          uint16_t                    hsyncend,
                                          uint16_t                    htotal,
                                          uint16_t                    hskew,
                                          uint16_t                    vdisplay,
                                          uint16_t                    vsyncstart,
                                          uint16_t                    vsyncend,
                                          uint16_t                    vtotal,
                                          uint32_t                    flags,
                                          uint32_t                    privsize,
                                          const uint8_t              *private)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 4,
        .ext = &xcb_xf86vidmode_id,
        .opcode = XCB_XF86VIDMODE_DELETE_MODE_LINE,
        .isvoid = 1
    };

    struct iovec xcb_parts[6];
    xcb_void_cookie_t xcb_ret;
    xcb_xf86vidmode_delete_mode_line_request_t xcb_out;

    xcb_out.screen = screen;
    xcb_out.dotclock = dotclock;
    xcb_out.hdisplay = hdisplay;
    xcb_out.hsyncstart = hsyncstart;
    xcb_out.hsyncend = hsyncend;
    xcb_out.htotal = htotal;
    xcb_out.hskew = hskew;
    xcb_out.vdisplay = vdisplay;
    xcb_out.vsyncstart = vsyncstart;
    xcb_out.vsyncend = vsyncend;
    xcb_out.vtotal = vtotal;
    memset(xcb_out.pad0, 0, 2);
    xcb_out.flags = flags;
    memset(xcb_out.pad1, 0, 12);
    xcb_out.privsize = privsize;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    /* uint8_t private */
    xcb_parts[4].iov_base = (char *) private;
    xcb_parts[4].iov_len = privsize * sizeof(uint8_t);
    xcb_parts[5].iov_base = 0;
    xcb_parts[5].iov_len = -xcb_parts[4].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_void_cookie_t
xcb_xf86vidmode_delete_mode_line (xcb_connection_t           *c,
                                  uint32_t                    screen,
                                  xcb_xf86vidmode_dotclock_t  dotclock,
                                  uint16_t                    hdisplay,
                                  uint16_t                    hsyncstart,
                                  uint16_t                    hsyncend,
                                  uint16_t                    htotal,
                                  uint16_t                    hskew,
                                  uint16_t                    vdisplay,
                                  uint16_t                    vsyncstart,
                                  uint16_t                    vsyncend,
                                  uint16_t                    vtotal,
                                  uint32_t                    flags,
                                  uint32_t                    privsize,
                                  const uint8_t              *private)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 4,
        .ext = &xcb_xf86vidmode_id,
        .opcode = XCB_XF86VIDMODE_DELETE_MODE_LINE,
        .isvoid = 1
    };

    struct iovec xcb_parts[6];
    xcb_void_cookie_t xcb_ret;
    xcb_xf86vidmode_delete_mode_line_request_t xcb_out;

    xcb_out.screen = screen;
    xcb_out.dotclock = dotclock;
    xcb_out.hdisplay = hdisplay;
    xcb_out.hsyncstart = hsyncstart;
    xcb_out.hsyncend = hsyncend;
    xcb_out.htotal = htotal;
    xcb_out.hskew = hskew;
    xcb_out.vdisplay = vdisplay;
    xcb_out.vsyncstart = vsyncstart;
    xcb_out.vsyncend = vsyncend;
    xcb_out.vtotal = vtotal;
    memset(xcb_out.pad0, 0, 2);
    xcb_out.flags = flags;
    memset(xcb_out.pad1, 0, 12);
    xcb_out.privsize = privsize;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    /* uint8_t private */
    xcb_parts[4].iov_base = (char *) private;
    xcb_parts[4].iov_len = privsize * sizeof(uint8_t);
    xcb_parts[5].iov_base = 0;
    xcb_parts[5].iov_len = -xcb_parts[4].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

uint8_t *
xcb_xf86vidmode_delete_mode_line_private (const xcb_xf86vidmode_delete_mode_line_request_t *R)
{
    return (uint8_t *) (R + 1);
}

int
xcb_xf86vidmode_delete_mode_line_private_length (const xcb_xf86vidmode_delete_mode_line_request_t *R)
{
    return R->privsize;
}

xcb_generic_iterator_t
xcb_xf86vidmode_delete_mode_line_private_end (const xcb_xf86vidmode_delete_mode_line_request_t *R)
{
    xcb_generic_iterator_t i;
    i.data = ((uint8_t *) (R + 1)) + (R->privsize);
    i.rem = 0;
    i.index = (char *) i.data - (char *) R;
    return i;
}

int
xcb_xf86vidmode_validate_mode_line_sizeof (const void  *_buffer)
{
    char *xcb_tmp = (char *)_buffer;
    const xcb_xf86vidmode_validate_mode_line_request_t *_aux = (xcb_xf86vidmode_validate_mode_line_request_t *)_buffer;
    unsigned int xcb_buffer_len = 0;
    unsigned int xcb_block_len = 0;
    unsigned int xcb_pad = 0;
    unsigned int xcb_align_to = 0;


    xcb_block_len += sizeof(xcb_xf86vidmode_validate_mode_line_request_t);
    xcb_tmp += xcb_block_len;
    xcb_buffer_len += xcb_block_len;
    xcb_block_len = 0;
    /* private */
    xcb_block_len += _aux->privsize * sizeof(uint8_t);
    xcb_tmp += xcb_block_len;
    xcb_align_to = ALIGNOF(uint8_t);
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

xcb_xf86vidmode_validate_mode_line_cookie_t
xcb_xf86vidmode_validate_mode_line (xcb_connection_t           *c,
                                    uint32_t                    screen,
                                    xcb_xf86vidmode_dotclock_t  dotclock,
                                    uint16_t                    hdisplay,
                                    uint16_t                    hsyncstart,
                                    uint16_t                    hsyncend,
                                    uint16_t                    htotal,
                                    uint16_t                    hskew,
                                    uint16_t                    vdisplay,
                                    uint16_t                    vsyncstart,
                                    uint16_t                    vsyncend,
                                    uint16_t                    vtotal,
                                    uint32_t                    flags,
                                    uint32_t                    privsize,
                                    const uint8_t              *private)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 4,
        .ext = &xcb_xf86vidmode_id,
        .opcode = XCB_XF86VIDMODE_VALIDATE_MODE_LINE,
        .isvoid = 0
    };

    struct iovec xcb_parts[6];
    xcb_xf86vidmode_validate_mode_line_cookie_t xcb_ret;
    xcb_xf86vidmode_validate_mode_line_request_t xcb_out;

    xcb_out.screen = screen;
    xcb_out.dotclock = dotclock;
    xcb_out.hdisplay = hdisplay;
    xcb_out.hsyncstart = hsyncstart;
    xcb_out.hsyncend = hsyncend;
    xcb_out.htotal = htotal;
    xcb_out.hskew = hskew;
    xcb_out.vdisplay = vdisplay;
    xcb_out.vsyncstart = vsyncstart;
    xcb_out.vsyncend = vsyncend;
    xcb_out.vtotal = vtotal;
    memset(xcb_out.pad0, 0, 2);
    xcb_out.flags = flags;
    memset(xcb_out.pad1, 0, 12);
    xcb_out.privsize = privsize;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    /* uint8_t private */
    xcb_parts[4].iov_base = (char *) private;
    xcb_parts[4].iov_len = privsize * sizeof(uint8_t);
    xcb_parts[5].iov_base = 0;
    xcb_parts[5].iov_len = -xcb_parts[4].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_xf86vidmode_validate_mode_line_cookie_t
xcb_xf86vidmode_validate_mode_line_unchecked (xcb_connection_t           *c,
                                              uint32_t                    screen,
                                              xcb_xf86vidmode_dotclock_t  dotclock,
                                              uint16_t                    hdisplay,
                                              uint16_t                    hsyncstart,
                                              uint16_t                    hsyncend,
                                              uint16_t                    htotal,
                                              uint16_t                    hskew,
                                              uint16_t                    vdisplay,
                                              uint16_t                    vsyncstart,
                                              uint16_t                    vsyncend,
                                              uint16_t                    vtotal,
                                              uint32_t                    flags,
                                              uint32_t                    privsize,
                                              const uint8_t              *private)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 4,
        .ext = &xcb_xf86vidmode_id,
        .opcode = XCB_XF86VIDMODE_VALIDATE_MODE_LINE,
        .isvoid = 0
    };

    struct iovec xcb_parts[6];
    xcb_xf86vidmode_validate_mode_line_cookie_t xcb_ret;
    xcb_xf86vidmode_validate_mode_line_request_t xcb_out;

    xcb_out.screen = screen;
    xcb_out.dotclock = dotclock;
    xcb_out.hdisplay = hdisplay;
    xcb_out.hsyncstart = hsyncstart;
    xcb_out.hsyncend = hsyncend;
    xcb_out.htotal = htotal;
    xcb_out.hskew = hskew;
    xcb_out.vdisplay = vdisplay;
    xcb_out.vsyncstart = vsyncstart;
    xcb_out.vsyncend = vsyncend;
    xcb_out.vtotal = vtotal;
    memset(xcb_out.pad0, 0, 2);
    xcb_out.flags = flags;
    memset(xcb_out.pad1, 0, 12);
    xcb_out.privsize = privsize;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    /* uint8_t private */
    xcb_parts[4].iov_base = (char *) private;
    xcb_parts[4].iov_len = privsize * sizeof(uint8_t);
    xcb_parts[5].iov_base = 0;
    xcb_parts[5].iov_len = -xcb_parts[4].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_xf86vidmode_validate_mode_line_reply_t *
xcb_xf86vidmode_validate_mode_line_reply (xcb_connection_t                             *c,
                                          xcb_xf86vidmode_validate_mode_line_cookie_t   cookie  /**< */,
                                          xcb_generic_error_t                         **e)
{
    return (xcb_xf86vidmode_validate_mode_line_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}

int
xcb_xf86vidmode_switch_to_mode_sizeof (const void  *_buffer)
{
    char *xcb_tmp = (char *)_buffer;
    const xcb_xf86vidmode_switch_to_mode_request_t *_aux = (xcb_xf86vidmode_switch_to_mode_request_t *)_buffer;
    unsigned int xcb_buffer_len = 0;
    unsigned int xcb_block_len = 0;
    unsigned int xcb_pad = 0;
    unsigned int xcb_align_to = 0;


    xcb_block_len += sizeof(xcb_xf86vidmode_switch_to_mode_request_t);
    xcb_tmp += xcb_block_len;
    xcb_buffer_len += xcb_block_len;
    xcb_block_len = 0;
    /* private */
    xcb_block_len += _aux->privsize * sizeof(uint8_t);
    xcb_tmp += xcb_block_len;
    xcb_align_to = ALIGNOF(uint8_t);
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
xcb_xf86vidmode_switch_to_mode_checked (xcb_connection_t           *c,
                                        uint32_t                    screen,
                                        xcb_xf86vidmode_dotclock_t  dotclock,
                                        uint16_t                    hdisplay,
                                        uint16_t                    hsyncstart,
                                        uint16_t                    hsyncend,
                                        uint16_t                    htotal,
                                        uint16_t                    hskew,
                                        uint16_t                    vdisplay,
                                        uint16_t                    vsyncstart,
                                        uint16_t                    vsyncend,
                                        uint16_t                    vtotal,
                                        uint32_t                    flags,
                                        uint32_t                    privsize,
                                        const uint8_t              *private)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 4,
        .ext = &xcb_xf86vidmode_id,
        .opcode = XCB_XF86VIDMODE_SWITCH_TO_MODE,
        .isvoid = 1
    };

    struct iovec xcb_parts[6];
    xcb_void_cookie_t xcb_ret;
    xcb_xf86vidmode_switch_to_mode_request_t xcb_out;

    xcb_out.screen = screen;
    xcb_out.dotclock = dotclock;
    xcb_out.hdisplay = hdisplay;
    xcb_out.hsyncstart = hsyncstart;
    xcb_out.hsyncend = hsyncend;
    xcb_out.htotal = htotal;
    xcb_out.hskew = hskew;
    xcb_out.vdisplay = vdisplay;
    xcb_out.vsyncstart = vsyncstart;
    xcb_out.vsyncend = vsyncend;
    xcb_out.vtotal = vtotal;
    memset(xcb_out.pad0, 0, 2);
    xcb_out.flags = flags;
    memset(xcb_out.pad1, 0, 12);
    xcb_out.privsize = privsize;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    /* uint8_t private */
    xcb_parts[4].iov_base = (char *) private;
    xcb_parts[4].iov_len = privsize * sizeof(uint8_t);
    xcb_parts[5].iov_base = 0;
    xcb_parts[5].iov_len = -xcb_parts[4].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_void_cookie_t
xcb_xf86vidmode_switch_to_mode (xcb_connection_t           *c,
                                uint32_t                    screen,
                                xcb_xf86vidmode_dotclock_t  dotclock,
                                uint16_t                    hdisplay,
                                uint16_t                    hsyncstart,
                                uint16_t                    hsyncend,
                                uint16_t                    htotal,
                                uint16_t                    hskew,
                                uint16_t                    vdisplay,
                                uint16_t                    vsyncstart,
                                uint16_t                    vsyncend,
                                uint16_t                    vtotal,
                                uint32_t                    flags,
                                uint32_t                    privsize,
                                const uint8_t              *private)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 4,
        .ext = &xcb_xf86vidmode_id,
        .opcode = XCB_XF86VIDMODE_SWITCH_TO_MODE,
        .isvoid = 1
    };

    struct iovec xcb_parts[6];
    xcb_void_cookie_t xcb_ret;
    xcb_xf86vidmode_switch_to_mode_request_t xcb_out;

    xcb_out.screen = screen;
    xcb_out.dotclock = dotclock;
    xcb_out.hdisplay = hdisplay;
    xcb_out.hsyncstart = hsyncstart;
    xcb_out.hsyncend = hsyncend;
    xcb_out.htotal = htotal;
    xcb_out.hskew = hskew;
    xcb_out.vdisplay = vdisplay;
    xcb_out.vsyncstart = vsyncstart;
    xcb_out.vsyncend = vsyncend;
    xcb_out.vtotal = vtotal;
    memset(xcb_out.pad0, 0, 2);
    xcb_out.flags = flags;
    memset(xcb_out.pad1, 0, 12);
    xcb_out.privsize = privsize;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    /* uint8_t private */
    xcb_parts[4].iov_base = (char *) private;
    xcb_parts[4].iov_len = privsize * sizeof(uint8_t);
    xcb_parts[5].iov_base = 0;
    xcb_parts[5].iov_len = -xcb_parts[4].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

uint8_t *
xcb_xf86vidmode_switch_to_mode_private (const xcb_xf86vidmode_switch_to_mode_request_t *R)
{
    return (uint8_t *) (R + 1);
}

int
xcb_xf86vidmode_switch_to_mode_private_length (const xcb_xf86vidmode_switch_to_mode_request_t *R)
{
    return R->privsize;
}

xcb_generic_iterator_t
xcb_xf86vidmode_switch_to_mode_private_end (const xcb_xf86vidmode_switch_to_mode_request_t *R)
{
    xcb_generic_iterator_t i;
    i.data = ((uint8_t *) (R + 1)) + (R->privsize);
    i.rem = 0;
    i.index = (char *) i.data - (char *) R;
    return i;
}

xcb_xf86vidmode_get_view_port_cookie_t
xcb_xf86vidmode_get_view_port (xcb_connection_t *c,
                               uint16_t          screen)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_xf86vidmode_id,
        .opcode = XCB_XF86VIDMODE_GET_VIEW_PORT,
        .isvoid = 0
    };

    struct iovec xcb_parts[4];
    xcb_xf86vidmode_get_view_port_cookie_t xcb_ret;
    xcb_xf86vidmode_get_view_port_request_t xcb_out;

    xcb_out.screen = screen;
    memset(xcb_out.pad0, 0, 2);

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_xf86vidmode_get_view_port_cookie_t
xcb_xf86vidmode_get_view_port_unchecked (xcb_connection_t *c,
                                         uint16_t          screen)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_xf86vidmode_id,
        .opcode = XCB_XF86VIDMODE_GET_VIEW_PORT,
        .isvoid = 0
    };

    struct iovec xcb_parts[4];
    xcb_xf86vidmode_get_view_port_cookie_t xcb_ret;
    xcb_xf86vidmode_get_view_port_request_t xcb_out;

    xcb_out.screen = screen;
    memset(xcb_out.pad0, 0, 2);

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_xf86vidmode_get_view_port_reply_t *
xcb_xf86vidmode_get_view_port_reply (xcb_connection_t                        *c,
                                     xcb_xf86vidmode_get_view_port_cookie_t   cookie  /**< */,
                                     xcb_generic_error_t                    **e)
{
    return (xcb_xf86vidmode_get_view_port_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}

xcb_void_cookie_t
xcb_xf86vidmode_set_view_port_checked (xcb_connection_t *c,
                                       uint16_t          screen,
                                       uint32_t          x,
                                       uint32_t          y)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_xf86vidmode_id,
        .opcode = XCB_XF86VIDMODE_SET_VIEW_PORT,
        .isvoid = 1
    };

    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xf86vidmode_set_view_port_request_t xcb_out;

    xcb_out.screen = screen;
    memset(xcb_out.pad0, 0, 2);
    xcb_out.x = x;
    xcb_out.y = y;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_void_cookie_t
xcb_xf86vidmode_set_view_port (xcb_connection_t *c,
                               uint16_t          screen,
                               uint32_t          x,
                               uint32_t          y)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_xf86vidmode_id,
        .opcode = XCB_XF86VIDMODE_SET_VIEW_PORT,
        .isvoid = 1
    };

    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xf86vidmode_set_view_port_request_t xcb_out;

    xcb_out.screen = screen;
    memset(xcb_out.pad0, 0, 2);
    xcb_out.x = x;
    xcb_out.y = y;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

int
xcb_xf86vidmode_get_dot_clocks_sizeof (const void  *_buffer)
{
    char *xcb_tmp = (char *)_buffer;
    const xcb_xf86vidmode_get_dot_clocks_reply_t *_aux = (xcb_xf86vidmode_get_dot_clocks_reply_t *)_buffer;
    unsigned int xcb_buffer_len = 0;
    unsigned int xcb_block_len = 0;
    unsigned int xcb_pad = 0;
    unsigned int xcb_align_to = 0;


    xcb_block_len += sizeof(xcb_xf86vidmode_get_dot_clocks_reply_t);
    xcb_tmp += xcb_block_len;
    xcb_buffer_len += xcb_block_len;
    xcb_block_len = 0;
    /* clock */
    xcb_block_len += ((1 - (_aux->flags & 1)) * _aux->clocks) * sizeof(uint32_t);
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

xcb_xf86vidmode_get_dot_clocks_cookie_t
xcb_xf86vidmode_get_dot_clocks (xcb_connection_t *c,
                                uint16_t          screen)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_xf86vidmode_id,
        .opcode = XCB_XF86VIDMODE_GET_DOT_CLOCKS,
        .isvoid = 0
    };

    struct iovec xcb_parts[4];
    xcb_xf86vidmode_get_dot_clocks_cookie_t xcb_ret;
    xcb_xf86vidmode_get_dot_clocks_request_t xcb_out;

    xcb_out.screen = screen;
    memset(xcb_out.pad0, 0, 2);

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_xf86vidmode_get_dot_clocks_cookie_t
xcb_xf86vidmode_get_dot_clocks_unchecked (xcb_connection_t *c,
                                          uint16_t          screen)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_xf86vidmode_id,
        .opcode = XCB_XF86VIDMODE_GET_DOT_CLOCKS,
        .isvoid = 0
    };

    struct iovec xcb_parts[4];
    xcb_xf86vidmode_get_dot_clocks_cookie_t xcb_ret;
    xcb_xf86vidmode_get_dot_clocks_request_t xcb_out;

    xcb_out.screen = screen;
    memset(xcb_out.pad0, 0, 2);

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

uint32_t *
xcb_xf86vidmode_get_dot_clocks_clock (const xcb_xf86vidmode_get_dot_clocks_reply_t *R)
{
    return (uint32_t *) (R + 1);
}

int
xcb_xf86vidmode_get_dot_clocks_clock_length (const xcb_xf86vidmode_get_dot_clocks_reply_t *R)
{
    return ((1 - (R->flags & 1)) * R->clocks);
}

xcb_generic_iterator_t
xcb_xf86vidmode_get_dot_clocks_clock_end (const xcb_xf86vidmode_get_dot_clocks_reply_t *R)
{
    xcb_generic_iterator_t i;
    i.data = ((uint32_t *) (R + 1)) + (((1 - (R->flags & 1)) * R->clocks));
    i.rem = 0;
    i.index = (char *) i.data - (char *) R;
    return i;
}

xcb_xf86vidmode_get_dot_clocks_reply_t *
xcb_xf86vidmode_get_dot_clocks_reply (xcb_connection_t                         *c,
                                      xcb_xf86vidmode_get_dot_clocks_cookie_t   cookie  /**< */,
                                      xcb_generic_error_t                     **e)
{
    return (xcb_xf86vidmode_get_dot_clocks_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}

xcb_void_cookie_t
xcb_xf86vidmode_set_client_version_checked (xcb_connection_t *c,
                                            uint16_t          major,
                                            uint16_t          minor)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_xf86vidmode_id,
        .opcode = XCB_XF86VIDMODE_SET_CLIENT_VERSION,
        .isvoid = 1
    };

    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xf86vidmode_set_client_version_request_t xcb_out;

    xcb_out.major = major;
    xcb_out.minor = minor;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_void_cookie_t
xcb_xf86vidmode_set_client_version (xcb_connection_t *c,
                                    uint16_t          major,
                                    uint16_t          minor)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_xf86vidmode_id,
        .opcode = XCB_XF86VIDMODE_SET_CLIENT_VERSION,
        .isvoid = 1
    };

    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xf86vidmode_set_client_version_request_t xcb_out;

    xcb_out.major = major;
    xcb_out.minor = minor;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_void_cookie_t
xcb_xf86vidmode_set_gamma_checked (xcb_connection_t *c,
                                   uint16_t          screen,
                                   uint32_t          red,
                                   uint32_t          green,
                                   uint32_t          blue)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_xf86vidmode_id,
        .opcode = XCB_XF86VIDMODE_SET_GAMMA,
        .isvoid = 1
    };

    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xf86vidmode_set_gamma_request_t xcb_out;

    xcb_out.screen = screen;
    memset(xcb_out.pad0, 0, 2);
    xcb_out.red = red;
    xcb_out.green = green;
    xcb_out.blue = blue;
    memset(xcb_out.pad1, 0, 12);

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_void_cookie_t
xcb_xf86vidmode_set_gamma (xcb_connection_t *c,
                           uint16_t          screen,
                           uint32_t          red,
                           uint32_t          green,
                           uint32_t          blue)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_xf86vidmode_id,
        .opcode = XCB_XF86VIDMODE_SET_GAMMA,
        .isvoid = 1
    };

    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xf86vidmode_set_gamma_request_t xcb_out;

    xcb_out.screen = screen;
    memset(xcb_out.pad0, 0, 2);
    xcb_out.red = red;
    xcb_out.green = green;
    xcb_out.blue = blue;
    memset(xcb_out.pad1, 0, 12);

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_xf86vidmode_get_gamma_cookie_t
xcb_xf86vidmode_get_gamma (xcb_connection_t *c,
                           uint16_t          screen)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_xf86vidmode_id,
        .opcode = XCB_XF86VIDMODE_GET_GAMMA,
        .isvoid = 0
    };

    struct iovec xcb_parts[4];
    xcb_xf86vidmode_get_gamma_cookie_t xcb_ret;
    xcb_xf86vidmode_get_gamma_request_t xcb_out;

    xcb_out.screen = screen;
    memset(xcb_out.pad0, 0, 26);

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_xf86vidmode_get_gamma_cookie_t
xcb_xf86vidmode_get_gamma_unchecked (xcb_connection_t *c,
                                     uint16_t          screen)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_xf86vidmode_id,
        .opcode = XCB_XF86VIDMODE_GET_GAMMA,
        .isvoid = 0
    };

    struct iovec xcb_parts[4];
    xcb_xf86vidmode_get_gamma_cookie_t xcb_ret;
    xcb_xf86vidmode_get_gamma_request_t xcb_out;

    xcb_out.screen = screen;
    memset(xcb_out.pad0, 0, 26);

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_xf86vidmode_get_gamma_reply_t *
xcb_xf86vidmode_get_gamma_reply (xcb_connection_t                    *c,
                                 xcb_xf86vidmode_get_gamma_cookie_t   cookie  /**< */,
                                 xcb_generic_error_t                **e)
{
    return (xcb_xf86vidmode_get_gamma_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}

int
xcb_xf86vidmode_get_gamma_ramp_sizeof (const void  *_buffer)
{
    char *xcb_tmp = (char *)_buffer;
    const xcb_xf86vidmode_get_gamma_ramp_reply_t *_aux = (xcb_xf86vidmode_get_gamma_ramp_reply_t *)_buffer;
    unsigned int xcb_buffer_len = 0;
    unsigned int xcb_block_len = 0;
    unsigned int xcb_pad = 0;
    unsigned int xcb_align_to = 0;


    xcb_block_len += sizeof(xcb_xf86vidmode_get_gamma_ramp_reply_t);
    xcb_tmp += xcb_block_len;
    xcb_buffer_len += xcb_block_len;
    xcb_block_len = 0;
    /* red */
    xcb_block_len += ((_aux->size + 1) & (~1)) * sizeof(uint16_t);
    xcb_tmp += xcb_block_len;
    xcb_align_to = ALIGNOF(uint16_t);
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_tmp += xcb_pad;
        xcb_pad = 0;
    }
    xcb_block_len = 0;
    /* green */
    xcb_block_len += ((_aux->size + 1) & (~1)) * sizeof(uint16_t);
    xcb_tmp += xcb_block_len;
    xcb_align_to = ALIGNOF(uint16_t);
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_tmp += xcb_pad;
        xcb_pad = 0;
    }
    xcb_block_len = 0;
    /* blue */
    xcb_block_len += ((_aux->size + 1) & (~1)) * sizeof(uint16_t);
    xcb_tmp += xcb_block_len;
    xcb_align_to = ALIGNOF(uint16_t);
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

xcb_xf86vidmode_get_gamma_ramp_cookie_t
xcb_xf86vidmode_get_gamma_ramp (xcb_connection_t *c,
                                uint16_t          screen,
                                uint16_t          size)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_xf86vidmode_id,
        .opcode = XCB_XF86VIDMODE_GET_GAMMA_RAMP,
        .isvoid = 0
    };

    struct iovec xcb_parts[4];
    xcb_xf86vidmode_get_gamma_ramp_cookie_t xcb_ret;
    xcb_xf86vidmode_get_gamma_ramp_request_t xcb_out;

    xcb_out.screen = screen;
    xcb_out.size = size;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_xf86vidmode_get_gamma_ramp_cookie_t
xcb_xf86vidmode_get_gamma_ramp_unchecked (xcb_connection_t *c,
                                          uint16_t          screen,
                                          uint16_t          size)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_xf86vidmode_id,
        .opcode = XCB_XF86VIDMODE_GET_GAMMA_RAMP,
        .isvoid = 0
    };

    struct iovec xcb_parts[4];
    xcb_xf86vidmode_get_gamma_ramp_cookie_t xcb_ret;
    xcb_xf86vidmode_get_gamma_ramp_request_t xcb_out;

    xcb_out.screen = screen;
    xcb_out.size = size;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

uint16_t *
xcb_xf86vidmode_get_gamma_ramp_red (const xcb_xf86vidmode_get_gamma_ramp_reply_t *R)
{
    return (uint16_t *) (R + 1);
}

int
xcb_xf86vidmode_get_gamma_ramp_red_length (const xcb_xf86vidmode_get_gamma_ramp_reply_t *R)
{
    return ((R->size + 1) & (~1));
}

xcb_generic_iterator_t
xcb_xf86vidmode_get_gamma_ramp_red_end (const xcb_xf86vidmode_get_gamma_ramp_reply_t *R)
{
    xcb_generic_iterator_t i;
    i.data = ((uint16_t *) (R + 1)) + (((R->size + 1) & (~1)));
    i.rem = 0;
    i.index = (char *) i.data - (char *) R;
    return i;
}

uint16_t *
xcb_xf86vidmode_get_gamma_ramp_green (const xcb_xf86vidmode_get_gamma_ramp_reply_t *R)
{
    xcb_generic_iterator_t prev = xcb_xf86vidmode_get_gamma_ramp_red_end(R);
    return (uint16_t *) ((char *) prev.data + XCB_TYPE_PAD(uint16_t, prev.index) + 0);
}

int
xcb_xf86vidmode_get_gamma_ramp_green_length (const xcb_xf86vidmode_get_gamma_ramp_reply_t *R)
{
    return ((R->size + 1) & (~1));
}

xcb_generic_iterator_t
xcb_xf86vidmode_get_gamma_ramp_green_end (const xcb_xf86vidmode_get_gamma_ramp_reply_t *R)
{
    xcb_generic_iterator_t i;
    xcb_generic_iterator_t prev = xcb_xf86vidmode_get_gamma_ramp_red_end(R);
    i.data = ((uint16_t *) ((char*) prev.data + XCB_TYPE_PAD(uint16_t, prev.index))) + (((R->size + 1) & (~1)));
    i.rem = 0;
    i.index = (char *) i.data - (char *) R;
    return i;
}

uint16_t *
xcb_xf86vidmode_get_gamma_ramp_blue (const xcb_xf86vidmode_get_gamma_ramp_reply_t *R)
{
    xcb_generic_iterator_t prev = xcb_xf86vidmode_get_gamma_ramp_green_end(R);
    return (uint16_t *) ((char *) prev.data + XCB_TYPE_PAD(uint16_t, prev.index) + 0);
}

int
xcb_xf86vidmode_get_gamma_ramp_blue_length (const xcb_xf86vidmode_get_gamma_ramp_reply_t *R)
{
    return ((R->size + 1) & (~1));
}

xcb_generic_iterator_t
xcb_xf86vidmode_get_gamma_ramp_blue_end (const xcb_xf86vidmode_get_gamma_ramp_reply_t *R)
{
    xcb_generic_iterator_t i;
    xcb_generic_iterator_t prev = xcb_xf86vidmode_get_gamma_ramp_green_end(R);
    i.data = ((uint16_t *) ((char*) prev.data + XCB_TYPE_PAD(uint16_t, prev.index))) + (((R->size + 1) & (~1)));
    i.rem = 0;
    i.index = (char *) i.data - (char *) R;
    return i;
}

xcb_xf86vidmode_get_gamma_ramp_reply_t *
xcb_xf86vidmode_get_gamma_ramp_reply (xcb_connection_t                         *c,
                                      xcb_xf86vidmode_get_gamma_ramp_cookie_t   cookie  /**< */,
                                      xcb_generic_error_t                     **e)
{
    return (xcb_xf86vidmode_get_gamma_ramp_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}

int
xcb_xf86vidmode_set_gamma_ramp_sizeof (const void  *_buffer)
{
    char *xcb_tmp = (char *)_buffer;
    const xcb_xf86vidmode_set_gamma_ramp_request_t *_aux = (xcb_xf86vidmode_set_gamma_ramp_request_t *)_buffer;
    unsigned int xcb_buffer_len = 0;
    unsigned int xcb_block_len = 0;
    unsigned int xcb_pad = 0;
    unsigned int xcb_align_to = 0;


    xcb_block_len += sizeof(xcb_xf86vidmode_set_gamma_ramp_request_t);
    xcb_tmp += xcb_block_len;
    xcb_buffer_len += xcb_block_len;
    xcb_block_len = 0;
    /* red */
    xcb_block_len += ((_aux->size + 1) & (~1)) * sizeof(uint16_t);
    xcb_tmp += xcb_block_len;
    xcb_align_to = ALIGNOF(uint16_t);
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_tmp += xcb_pad;
        xcb_pad = 0;
    }
    xcb_block_len = 0;
    /* green */
    xcb_block_len += ((_aux->size + 1) & (~1)) * sizeof(uint16_t);
    xcb_tmp += xcb_block_len;
    xcb_align_to = ALIGNOF(uint16_t);
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_tmp += xcb_pad;
        xcb_pad = 0;
    }
    xcb_block_len = 0;
    /* blue */
    xcb_block_len += ((_aux->size + 1) & (~1)) * sizeof(uint16_t);
    xcb_tmp += xcb_block_len;
    xcb_align_to = ALIGNOF(uint16_t);
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
xcb_xf86vidmode_set_gamma_ramp_checked (xcb_connection_t *c,
                                        uint16_t          screen,
                                        uint16_t          size,
                                        const uint16_t   *red,
                                        const uint16_t   *green,
                                        const uint16_t   *blue)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 8,
        .ext = &xcb_xf86vidmode_id,
        .opcode = XCB_XF86VIDMODE_SET_GAMMA_RAMP,
        .isvoid = 1
    };

    struct iovec xcb_parts[10];
    xcb_void_cookie_t xcb_ret;
    xcb_xf86vidmode_set_gamma_ramp_request_t xcb_out;

    xcb_out.screen = screen;
    xcb_out.size = size;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    /* uint16_t red */
    xcb_parts[4].iov_base = (char *) red;
    xcb_parts[4].iov_len = ((size + 1) & (~1)) * sizeof(uint16_t);
    xcb_parts[5].iov_base = 0;
    xcb_parts[5].iov_len = -xcb_parts[4].iov_len & 3;
    /* uint16_t green */
    xcb_parts[6].iov_base = (char *) green;
    xcb_parts[6].iov_len = ((size + 1) & (~1)) * sizeof(uint16_t);
    xcb_parts[7].iov_base = 0;
    xcb_parts[7].iov_len = -xcb_parts[6].iov_len & 3;
    /* uint16_t blue */
    xcb_parts[8].iov_base = (char *) blue;
    xcb_parts[8].iov_len = ((size + 1) & (~1)) * sizeof(uint16_t);
    xcb_parts[9].iov_base = 0;
    xcb_parts[9].iov_len = -xcb_parts[8].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_void_cookie_t
xcb_xf86vidmode_set_gamma_ramp (xcb_connection_t *c,
                                uint16_t          screen,
                                uint16_t          size,
                                const uint16_t   *red,
                                const uint16_t   *green,
                                const uint16_t   *blue)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 8,
        .ext = &xcb_xf86vidmode_id,
        .opcode = XCB_XF86VIDMODE_SET_GAMMA_RAMP,
        .isvoid = 1
    };

    struct iovec xcb_parts[10];
    xcb_void_cookie_t xcb_ret;
    xcb_xf86vidmode_set_gamma_ramp_request_t xcb_out;

    xcb_out.screen = screen;
    xcb_out.size = size;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    /* uint16_t red */
    xcb_parts[4].iov_base = (char *) red;
    xcb_parts[4].iov_len = ((size + 1) & (~1)) * sizeof(uint16_t);
    xcb_parts[5].iov_base = 0;
    xcb_parts[5].iov_len = -xcb_parts[4].iov_len & 3;
    /* uint16_t green */
    xcb_parts[6].iov_base = (char *) green;
    xcb_parts[6].iov_len = ((size + 1) & (~1)) * sizeof(uint16_t);
    xcb_parts[7].iov_base = 0;
    xcb_parts[7].iov_len = -xcb_parts[6].iov_len & 3;
    /* uint16_t blue */
    xcb_parts[8].iov_base = (char *) blue;
    xcb_parts[8].iov_len = ((size + 1) & (~1)) * sizeof(uint16_t);
    xcb_parts[9].iov_base = 0;
    xcb_parts[9].iov_len = -xcb_parts[8].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

uint16_t *
xcb_xf86vidmode_set_gamma_ramp_red (const xcb_xf86vidmode_set_gamma_ramp_request_t *R)
{
    return (uint16_t *) (R + 1);
}

int
xcb_xf86vidmode_set_gamma_ramp_red_length (const xcb_xf86vidmode_set_gamma_ramp_request_t *R)
{
    return ((R->size + 1) & (~1));
}

xcb_generic_iterator_t
xcb_xf86vidmode_set_gamma_ramp_red_end (const xcb_xf86vidmode_set_gamma_ramp_request_t *R)
{
    xcb_generic_iterator_t i;
    i.data = ((uint16_t *) (R + 1)) + (((R->size + 1) & (~1)));
    i.rem = 0;
    i.index = (char *) i.data - (char *) R;
    return i;
}

uint16_t *
xcb_xf86vidmode_set_gamma_ramp_green (const xcb_xf86vidmode_set_gamma_ramp_request_t *R)
{
    xcb_generic_iterator_t prev = xcb_xf86vidmode_set_gamma_ramp_red_end(R);
    return (uint16_t *) ((char *) prev.data + XCB_TYPE_PAD(uint16_t, prev.index) + 0);
}

int
xcb_xf86vidmode_set_gamma_ramp_green_length (const xcb_xf86vidmode_set_gamma_ramp_request_t *R)
{
    return ((R->size + 1) & (~1));
}

xcb_generic_iterator_t
xcb_xf86vidmode_set_gamma_ramp_green_end (const xcb_xf86vidmode_set_gamma_ramp_request_t *R)
{
    xcb_generic_iterator_t i;
    xcb_generic_iterator_t prev = xcb_xf86vidmode_set_gamma_ramp_red_end(R);
    i.data = ((uint16_t *) ((char*) prev.data + XCB_TYPE_PAD(uint16_t, prev.index))) + (((R->size + 1) & (~1)));
    i.rem = 0;
    i.index = (char *) i.data - (char *) R;
    return i;
}

uint16_t *
xcb_xf86vidmode_set_gamma_ramp_blue (const xcb_xf86vidmode_set_gamma_ramp_request_t *R)
{
    xcb_generic_iterator_t prev = xcb_xf86vidmode_set_gamma_ramp_green_end(R);
    return (uint16_t *) ((char *) prev.data + XCB_TYPE_PAD(uint16_t, prev.index) + 0);
}

int
xcb_xf86vidmode_set_gamma_ramp_blue_length (const xcb_xf86vidmode_set_gamma_ramp_request_t *R)
{
    return ((R->size + 1) & (~1));
}

xcb_generic_iterator_t
xcb_xf86vidmode_set_gamma_ramp_blue_end (const xcb_xf86vidmode_set_gamma_ramp_request_t *R)
{
    xcb_generic_iterator_t i;
    xcb_generic_iterator_t prev = xcb_xf86vidmode_set_gamma_ramp_green_end(R);
    i.data = ((uint16_t *) ((char*) prev.data + XCB_TYPE_PAD(uint16_t, prev.index))) + (((R->size + 1) & (~1)));
    i.rem = 0;
    i.index = (char *) i.data - (char *) R;
    return i;
}

xcb_xf86vidmode_get_gamma_ramp_size_cookie_t
xcb_xf86vidmode_get_gamma_ramp_size (xcb_connection_t *c,
                                     uint16_t          screen)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_xf86vidmode_id,
        .opcode = XCB_XF86VIDMODE_GET_GAMMA_RAMP_SIZE,
        .isvoid = 0
    };

    struct iovec xcb_parts[4];
    xcb_xf86vidmode_get_gamma_ramp_size_cookie_t xcb_ret;
    xcb_xf86vidmode_get_gamma_ramp_size_request_t xcb_out;

    xcb_out.screen = screen;
    memset(xcb_out.pad0, 0, 2);

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_xf86vidmode_get_gamma_ramp_size_cookie_t
xcb_xf86vidmode_get_gamma_ramp_size_unchecked (xcb_connection_t *c,
                                               uint16_t          screen)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_xf86vidmode_id,
        .opcode = XCB_XF86VIDMODE_GET_GAMMA_RAMP_SIZE,
        .isvoid = 0
    };

    struct iovec xcb_parts[4];
    xcb_xf86vidmode_get_gamma_ramp_size_cookie_t xcb_ret;
    xcb_xf86vidmode_get_gamma_ramp_size_request_t xcb_out;

    xcb_out.screen = screen;
    memset(xcb_out.pad0, 0, 2);

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_xf86vidmode_get_gamma_ramp_size_reply_t *
xcb_xf86vidmode_get_gamma_ramp_size_reply (xcb_connection_t                              *c,
                                           xcb_xf86vidmode_get_gamma_ramp_size_cookie_t   cookie  /**< */,
                                           xcb_generic_error_t                          **e)
{
    return (xcb_xf86vidmode_get_gamma_ramp_size_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}

xcb_xf86vidmode_get_permissions_cookie_t
xcb_xf86vidmode_get_permissions (xcb_connection_t *c,
                                 uint16_t          screen)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_xf86vidmode_id,
        .opcode = XCB_XF86VIDMODE_GET_PERMISSIONS,
        .isvoid = 0
    };

    struct iovec xcb_parts[4];
    xcb_xf86vidmode_get_permissions_cookie_t xcb_ret;
    xcb_xf86vidmode_get_permissions_request_t xcb_out;

    xcb_out.screen = screen;
    memset(xcb_out.pad0, 0, 2);

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_xf86vidmode_get_permissions_cookie_t
xcb_xf86vidmode_get_permissions_unchecked (xcb_connection_t *c,
                                           uint16_t          screen)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_xf86vidmode_id,
        .opcode = XCB_XF86VIDMODE_GET_PERMISSIONS,
        .isvoid = 0
    };

    struct iovec xcb_parts[4];
    xcb_xf86vidmode_get_permissions_cookie_t xcb_ret;
    xcb_xf86vidmode_get_permissions_request_t xcb_out;

    xcb_out.screen = screen;
    memset(xcb_out.pad0, 0, 2);

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_xf86vidmode_get_permissions_reply_t *
xcb_xf86vidmode_get_permissions_reply (xcb_connection_t                          *c,
                                       xcb_xf86vidmode_get_permissions_cookie_t   cookie  /**< */,
                                       xcb_generic_error_t                      **e)
{
    return (xcb_xf86vidmode_get_permissions_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}

