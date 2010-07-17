/*
 * This file generated automatically from dri2.xml by c_client.py.
 * Edit at your peril.
 */

#include <string.h>
#include <assert.h>
#include "xcbext.h"
#include "dri2.h"
#include "xproto.h"

xcb_extension_t xcb_dri2_id = { "DRI2", 0 };


/*****************************************************************************
 **
 ** void xcb_dri2_dri2_buffer_next
 ** 
 ** @param xcb_dri2_dri2_buffer_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_dri2_dri2_buffer_next (xcb_dri2_dri2_buffer_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_dri2_dri2_buffer_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_dri2_dri2_buffer_end
 ** 
 ** @param xcb_dri2_dri2_buffer_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_dri2_dri2_buffer_end (xcb_dri2_dri2_buffer_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}


/*****************************************************************************
 **
 ** void xcb_dri2_attach_format_next
 ** 
 ** @param xcb_dri2_attach_format_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_dri2_attach_format_next (xcb_dri2_attach_format_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_dri2_attach_format_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_dri2_attach_format_end
 ** 
 ** @param xcb_dri2_attach_format_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_dri2_attach_format_end (xcb_dri2_attach_format_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}


/*****************************************************************************
 **
 ** xcb_dri2_query_version_cookie_t xcb_dri2_query_version
 ** 
 ** @param xcb_connection_t *c
 ** @param uint32_t          major_version
 ** @param uint32_t          minor_version
 ** @returns xcb_dri2_query_version_cookie_t
 **
 *****************************************************************************/
 
xcb_dri2_query_version_cookie_t
xcb_dri2_query_version (xcb_connection_t *c  /**< */,
                        uint32_t          major_version  /**< */,
                        uint32_t          minor_version  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_dri2_id,
        /* opcode */ XCB_DRI2_QUERY_VERSION,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_dri2_query_version_cookie_t xcb_ret;
    xcb_dri2_query_version_request_t xcb_out;
    
    xcb_out.major_version = major_version;
    xcb_out.minor_version = minor_version;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_dri2_query_version_cookie_t xcb_dri2_query_version_unchecked
 ** 
 ** @param xcb_connection_t *c
 ** @param uint32_t          major_version
 ** @param uint32_t          minor_version
 ** @returns xcb_dri2_query_version_cookie_t
 **
 *****************************************************************************/
 
xcb_dri2_query_version_cookie_t
xcb_dri2_query_version_unchecked (xcb_connection_t *c  /**< */,
                                  uint32_t          major_version  /**< */,
                                  uint32_t          minor_version  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_dri2_id,
        /* opcode */ XCB_DRI2_QUERY_VERSION,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_dri2_query_version_cookie_t xcb_ret;
    xcb_dri2_query_version_request_t xcb_out;
    
    xcb_out.major_version = major_version;
    xcb_out.minor_version = minor_version;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_dri2_query_version_reply_t * xcb_dri2_query_version_reply
 ** 
 ** @param xcb_connection_t                 *c
 ** @param xcb_dri2_query_version_cookie_t   cookie
 ** @param xcb_generic_error_t             **e
 ** @returns xcb_dri2_query_version_reply_t *
 **
 *****************************************************************************/
 
xcb_dri2_query_version_reply_t *
xcb_dri2_query_version_reply (xcb_connection_t                 *c  /**< */,
                              xcb_dri2_query_version_cookie_t   cookie  /**< */,
                              xcb_generic_error_t             **e  /**< */)
{
    return (xcb_dri2_query_version_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}


/*****************************************************************************
 **
 ** xcb_dri2_connect_cookie_t xcb_dri2_connect
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_window_t      window
 ** @param uint32_t          driver_type
 ** @returns xcb_dri2_connect_cookie_t
 **
 *****************************************************************************/
 
xcb_dri2_connect_cookie_t
xcb_dri2_connect (xcb_connection_t *c  /**< */,
                  xcb_window_t      window  /**< */,
                  uint32_t          driver_type  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_dri2_id,
        /* opcode */ XCB_DRI2_CONNECT,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_dri2_connect_cookie_t xcb_ret;
    xcb_dri2_connect_request_t xcb_out;
    
    xcb_out.window = window;
    xcb_out.driver_type = driver_type;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_dri2_connect_cookie_t xcb_dri2_connect_unchecked
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_window_t      window
 ** @param uint32_t          driver_type
 ** @returns xcb_dri2_connect_cookie_t
 **
 *****************************************************************************/
 
xcb_dri2_connect_cookie_t
xcb_dri2_connect_unchecked (xcb_connection_t *c  /**< */,
                            xcb_window_t      window  /**< */,
                            uint32_t          driver_type  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_dri2_id,
        /* opcode */ XCB_DRI2_CONNECT,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_dri2_connect_cookie_t xcb_ret;
    xcb_dri2_connect_request_t xcb_out;
    
    xcb_out.window = window;
    xcb_out.driver_type = driver_type;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** char * xcb_dri2_connect_driver_name
 ** 
 ** @param const xcb_dri2_connect_reply_t *R
 ** @returns char *
 **
 *****************************************************************************/
 
char *
xcb_dri2_connect_driver_name (const xcb_dri2_connect_reply_t *R  /**< */)
{
    return (char *) (R + 1);
}


/*****************************************************************************
 **
 ** int xcb_dri2_connect_driver_name_length
 ** 
 ** @param const xcb_dri2_connect_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_dri2_connect_driver_name_length (const xcb_dri2_connect_reply_t *R  /**< */)
{
    return R->driver_name_length;
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_dri2_connect_driver_name_end
 ** 
 ** @param const xcb_dri2_connect_reply_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_dri2_connect_driver_name_end (const xcb_dri2_connect_reply_t *R  /**< */)
{
    xcb_generic_iterator_t i;
    i.data = ((char *) (R + 1)) + (R->driver_name_length);
    i.rem = 0;
    i.index = (char *) i.data - (char *) R;
    return i;
}


/*****************************************************************************
 **
 ** char * xcb_dri2_connect_device_name
 ** 
 ** @param const xcb_dri2_connect_reply_t *R
 ** @returns char *
 **
 *****************************************************************************/
 
char *
xcb_dri2_connect_device_name (const xcb_dri2_connect_reply_t *R  /**< */)
{
    xcb_generic_iterator_t prev = xcb_dri2_connect_driver_name_end(R);
    return (char *) ((char *) prev.data + XCB_TYPE_PAD(char, prev.index) + 0);
}


/*****************************************************************************
 **
 ** int xcb_dri2_connect_device_name_length
 ** 
 ** @param const xcb_dri2_connect_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_dri2_connect_device_name_length (const xcb_dri2_connect_reply_t *R  /**< */)
{
    return R->device_name_length;
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_dri2_connect_device_name_end
 ** 
 ** @param const xcb_dri2_connect_reply_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_dri2_connect_device_name_end (const xcb_dri2_connect_reply_t *R  /**< */)
{
    xcb_generic_iterator_t i;
    xcb_generic_iterator_t child = xcb_dri2_connect_driver_name_end(R);
    i.data = ((char *) child.data) + (R->device_name_length);
    i.rem = 0;
    i.index = (char *) i.data - (char *) R;
    return i;
}


/*****************************************************************************
 **
 ** xcb_dri2_connect_reply_t * xcb_dri2_connect_reply
 ** 
 ** @param xcb_connection_t           *c
 ** @param xcb_dri2_connect_cookie_t   cookie
 ** @param xcb_generic_error_t       **e
 ** @returns xcb_dri2_connect_reply_t *
 **
 *****************************************************************************/
 
xcb_dri2_connect_reply_t *
xcb_dri2_connect_reply (xcb_connection_t           *c  /**< */,
                        xcb_dri2_connect_cookie_t   cookie  /**< */,
                        xcb_generic_error_t       **e  /**< */)
{
    return (xcb_dri2_connect_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}


/*****************************************************************************
 **
 ** xcb_dri2_authenticate_cookie_t xcb_dri2_authenticate
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_window_t      window
 ** @param uint32_t          magic
 ** @returns xcb_dri2_authenticate_cookie_t
 **
 *****************************************************************************/
 
xcb_dri2_authenticate_cookie_t
xcb_dri2_authenticate (xcb_connection_t *c  /**< */,
                       xcb_window_t      window  /**< */,
                       uint32_t          magic  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_dri2_id,
        /* opcode */ XCB_DRI2_AUTHENTICATE,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_dri2_authenticate_cookie_t xcb_ret;
    xcb_dri2_authenticate_request_t xcb_out;
    
    xcb_out.window = window;
    xcb_out.magic = magic;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_dri2_authenticate_cookie_t xcb_dri2_authenticate_unchecked
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_window_t      window
 ** @param uint32_t          magic
 ** @returns xcb_dri2_authenticate_cookie_t
 **
 *****************************************************************************/
 
xcb_dri2_authenticate_cookie_t
xcb_dri2_authenticate_unchecked (xcb_connection_t *c  /**< */,
                                 xcb_window_t      window  /**< */,
                                 uint32_t          magic  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_dri2_id,
        /* opcode */ XCB_DRI2_AUTHENTICATE,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_dri2_authenticate_cookie_t xcb_ret;
    xcb_dri2_authenticate_request_t xcb_out;
    
    xcb_out.window = window;
    xcb_out.magic = magic;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_dri2_authenticate_reply_t * xcb_dri2_authenticate_reply
 ** 
 ** @param xcb_connection_t                *c
 ** @param xcb_dri2_authenticate_cookie_t   cookie
 ** @param xcb_generic_error_t            **e
 ** @returns xcb_dri2_authenticate_reply_t *
 **
 *****************************************************************************/
 
xcb_dri2_authenticate_reply_t *
xcb_dri2_authenticate_reply (xcb_connection_t                *c  /**< */,
                             xcb_dri2_authenticate_cookie_t   cookie  /**< */,
                             xcb_generic_error_t            **e  /**< */)
{
    return (xcb_dri2_authenticate_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_dri2_create_drawable_checked
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_drawable_t    drawable
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_dri2_create_drawable_checked (xcb_connection_t *c  /**< */,
                                  xcb_drawable_t    drawable  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_dri2_id,
        /* opcode */ XCB_DRI2_CREATE_DRAWABLE,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_dri2_create_drawable_request_t xcb_out;
    
    xcb_out.drawable = drawable;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_dri2_create_drawable
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_drawable_t    drawable
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_dri2_create_drawable (xcb_connection_t *c  /**< */,
                          xcb_drawable_t    drawable  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_dri2_id,
        /* opcode */ XCB_DRI2_CREATE_DRAWABLE,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_dri2_create_drawable_request_t xcb_out;
    
    xcb_out.drawable = drawable;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_dri2_destroy_drawable_checked
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_drawable_t    drawable
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_dri2_destroy_drawable_checked (xcb_connection_t *c  /**< */,
                                   xcb_drawable_t    drawable  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_dri2_id,
        /* opcode */ XCB_DRI2_DESTROY_DRAWABLE,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_dri2_destroy_drawable_request_t xcb_out;
    
    xcb_out.drawable = drawable;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_dri2_destroy_drawable
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_drawable_t    drawable
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_dri2_destroy_drawable (xcb_connection_t *c  /**< */,
                           xcb_drawable_t    drawable  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_dri2_id,
        /* opcode */ XCB_DRI2_DESTROY_DRAWABLE,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_dri2_destroy_drawable_request_t xcb_out;
    
    xcb_out.drawable = drawable;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_dri2_get_buffers_cookie_t xcb_dri2_get_buffers
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_drawable_t    drawable
 ** @param uint32_t          count
 ** @param uint32_t          attachments_len
 ** @param const uint32_t   *attachments
 ** @returns xcb_dri2_get_buffers_cookie_t
 **
 *****************************************************************************/
 
xcb_dri2_get_buffers_cookie_t
xcb_dri2_get_buffers (xcb_connection_t *c  /**< */,
                      xcb_drawable_t    drawable  /**< */,
                      uint32_t          count  /**< */,
                      uint32_t          attachments_len  /**< */,
                      const uint32_t   *attachments  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 4,
        /* ext */ &xcb_dri2_id,
        /* opcode */ XCB_DRI2_GET_BUFFERS,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[6];
    xcb_dri2_get_buffers_cookie_t xcb_ret;
    xcb_dri2_get_buffers_request_t xcb_out;
    
    xcb_out.drawable = drawable;
    xcb_out.count = count;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_parts[4].iov_base = (char *) attachments;
    xcb_parts[4].iov_len = attachments_len * sizeof(uint32_t);
    xcb_parts[5].iov_base = 0;
    xcb_parts[5].iov_len = -xcb_parts[4].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_dri2_get_buffers_cookie_t xcb_dri2_get_buffers_unchecked
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_drawable_t    drawable
 ** @param uint32_t          count
 ** @param uint32_t          attachments_len
 ** @param const uint32_t   *attachments
 ** @returns xcb_dri2_get_buffers_cookie_t
 **
 *****************************************************************************/
 
xcb_dri2_get_buffers_cookie_t
xcb_dri2_get_buffers_unchecked (xcb_connection_t *c  /**< */,
                                xcb_drawable_t    drawable  /**< */,
                                uint32_t          count  /**< */,
                                uint32_t          attachments_len  /**< */,
                                const uint32_t   *attachments  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 4,
        /* ext */ &xcb_dri2_id,
        /* opcode */ XCB_DRI2_GET_BUFFERS,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[6];
    xcb_dri2_get_buffers_cookie_t xcb_ret;
    xcb_dri2_get_buffers_request_t xcb_out;
    
    xcb_out.drawable = drawable;
    xcb_out.count = count;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_parts[4].iov_base = (char *) attachments;
    xcb_parts[4].iov_len = attachments_len * sizeof(uint32_t);
    xcb_parts[5].iov_base = 0;
    xcb_parts[5].iov_len = -xcb_parts[4].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_dri2_dri2_buffer_t * xcb_dri2_get_buffers_buffers
 ** 
 ** @param const xcb_dri2_get_buffers_reply_t *R
 ** @returns xcb_dri2_dri2_buffer_t *
 **
 *****************************************************************************/
 
xcb_dri2_dri2_buffer_t *
xcb_dri2_get_buffers_buffers (const xcb_dri2_get_buffers_reply_t *R  /**< */)
{
    return (xcb_dri2_dri2_buffer_t *) (R + 1);
}


/*****************************************************************************
 **
 ** int xcb_dri2_get_buffers_buffers_length
 ** 
 ** @param const xcb_dri2_get_buffers_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_dri2_get_buffers_buffers_length (const xcb_dri2_get_buffers_reply_t *R  /**< */)
{
    return R->count;
}


/*****************************************************************************
 **
 ** xcb_dri2_dri2_buffer_iterator_t xcb_dri2_get_buffers_buffers_iterator
 ** 
 ** @param const xcb_dri2_get_buffers_reply_t *R
 ** @returns xcb_dri2_dri2_buffer_iterator_t
 **
 *****************************************************************************/
 
xcb_dri2_dri2_buffer_iterator_t
xcb_dri2_get_buffers_buffers_iterator (const xcb_dri2_get_buffers_reply_t *R  /**< */)
{
    xcb_dri2_dri2_buffer_iterator_t i;
    i.data = (xcb_dri2_dri2_buffer_t *) (R + 1);
    i.rem = R->count;
    i.index = (char *) i.data - (char *) R;
    return i;
}


/*****************************************************************************
 **
 ** xcb_dri2_get_buffers_reply_t * xcb_dri2_get_buffers_reply
 ** 
 ** @param xcb_connection_t               *c
 ** @param xcb_dri2_get_buffers_cookie_t   cookie
 ** @param xcb_generic_error_t           **e
 ** @returns xcb_dri2_get_buffers_reply_t *
 **
 *****************************************************************************/
 
xcb_dri2_get_buffers_reply_t *
xcb_dri2_get_buffers_reply (xcb_connection_t               *c  /**< */,
                            xcb_dri2_get_buffers_cookie_t   cookie  /**< */,
                            xcb_generic_error_t           **e  /**< */)
{
    return (xcb_dri2_get_buffers_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}


/*****************************************************************************
 **
 ** xcb_dri2_copy_region_cookie_t xcb_dri2_copy_region
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_drawable_t    drawable
 ** @param uint32_t          region
 ** @param uint32_t          dest
 ** @param uint32_t          src
 ** @returns xcb_dri2_copy_region_cookie_t
 **
 *****************************************************************************/
 
xcb_dri2_copy_region_cookie_t
xcb_dri2_copy_region (xcb_connection_t *c  /**< */,
                      xcb_drawable_t    drawable  /**< */,
                      uint32_t          region  /**< */,
                      uint32_t          dest  /**< */,
                      uint32_t          src  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_dri2_id,
        /* opcode */ XCB_DRI2_COPY_REGION,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_dri2_copy_region_cookie_t xcb_ret;
    xcb_dri2_copy_region_request_t xcb_out;
    
    xcb_out.drawable = drawable;
    xcb_out.region = region;
    xcb_out.dest = dest;
    xcb_out.src = src;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_dri2_copy_region_cookie_t xcb_dri2_copy_region_unchecked
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_drawable_t    drawable
 ** @param uint32_t          region
 ** @param uint32_t          dest
 ** @param uint32_t          src
 ** @returns xcb_dri2_copy_region_cookie_t
 **
 *****************************************************************************/
 
xcb_dri2_copy_region_cookie_t
xcb_dri2_copy_region_unchecked (xcb_connection_t *c  /**< */,
                                xcb_drawable_t    drawable  /**< */,
                                uint32_t          region  /**< */,
                                uint32_t          dest  /**< */,
                                uint32_t          src  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_dri2_id,
        /* opcode */ XCB_DRI2_COPY_REGION,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_dri2_copy_region_cookie_t xcb_ret;
    xcb_dri2_copy_region_request_t xcb_out;
    
    xcb_out.drawable = drawable;
    xcb_out.region = region;
    xcb_out.dest = dest;
    xcb_out.src = src;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_dri2_copy_region_reply_t * xcb_dri2_copy_region_reply
 ** 
 ** @param xcb_connection_t               *c
 ** @param xcb_dri2_copy_region_cookie_t   cookie
 ** @param xcb_generic_error_t           **e
 ** @returns xcb_dri2_copy_region_reply_t *
 **
 *****************************************************************************/
 
xcb_dri2_copy_region_reply_t *
xcb_dri2_copy_region_reply (xcb_connection_t               *c  /**< */,
                            xcb_dri2_copy_region_cookie_t   cookie  /**< */,
                            xcb_generic_error_t           **e  /**< */)
{
    return (xcb_dri2_copy_region_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}


/*****************************************************************************
 **
 ** xcb_dri2_get_buffers_with_format_cookie_t xcb_dri2_get_buffers_with_format
 ** 
 ** @param xcb_connection_t               *c
 ** @param xcb_drawable_t                  drawable
 ** @param uint32_t                        count
 ** @param uint32_t                        attachments_len
 ** @param const xcb_dri2_attach_format_t *attachments
 ** @returns xcb_dri2_get_buffers_with_format_cookie_t
 **
 *****************************************************************************/
 
xcb_dri2_get_buffers_with_format_cookie_t
xcb_dri2_get_buffers_with_format (xcb_connection_t               *c  /**< */,
                                  xcb_drawable_t                  drawable  /**< */,
                                  uint32_t                        count  /**< */,
                                  uint32_t                        attachments_len  /**< */,
                                  const xcb_dri2_attach_format_t *attachments  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 4,
        /* ext */ &xcb_dri2_id,
        /* opcode */ XCB_DRI2_GET_BUFFERS_WITH_FORMAT,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[6];
    xcb_dri2_get_buffers_with_format_cookie_t xcb_ret;
    xcb_dri2_get_buffers_with_format_request_t xcb_out;
    
    xcb_out.drawable = drawable;
    xcb_out.count = count;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_parts[4].iov_base = (char *) attachments;
    xcb_parts[4].iov_len = attachments_len * sizeof(xcb_dri2_attach_format_t);
    xcb_parts[5].iov_base = 0;
    xcb_parts[5].iov_len = -xcb_parts[4].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_dri2_get_buffers_with_format_cookie_t xcb_dri2_get_buffers_with_format_unchecked
 ** 
 ** @param xcb_connection_t               *c
 ** @param xcb_drawable_t                  drawable
 ** @param uint32_t                        count
 ** @param uint32_t                        attachments_len
 ** @param const xcb_dri2_attach_format_t *attachments
 ** @returns xcb_dri2_get_buffers_with_format_cookie_t
 **
 *****************************************************************************/
 
xcb_dri2_get_buffers_with_format_cookie_t
xcb_dri2_get_buffers_with_format_unchecked (xcb_connection_t               *c  /**< */,
                                            xcb_drawable_t                  drawable  /**< */,
                                            uint32_t                        count  /**< */,
                                            uint32_t                        attachments_len  /**< */,
                                            const xcb_dri2_attach_format_t *attachments  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 4,
        /* ext */ &xcb_dri2_id,
        /* opcode */ XCB_DRI2_GET_BUFFERS_WITH_FORMAT,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[6];
    xcb_dri2_get_buffers_with_format_cookie_t xcb_ret;
    xcb_dri2_get_buffers_with_format_request_t xcb_out;
    
    xcb_out.drawable = drawable;
    xcb_out.count = count;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_parts[4].iov_base = (char *) attachments;
    xcb_parts[4].iov_len = attachments_len * sizeof(xcb_dri2_attach_format_t);
    xcb_parts[5].iov_base = 0;
    xcb_parts[5].iov_len = -xcb_parts[4].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_dri2_dri2_buffer_t * xcb_dri2_get_buffers_with_format_buffers
 ** 
 ** @param const xcb_dri2_get_buffers_with_format_reply_t *R
 ** @returns xcb_dri2_dri2_buffer_t *
 **
 *****************************************************************************/
 
xcb_dri2_dri2_buffer_t *
xcb_dri2_get_buffers_with_format_buffers (const xcb_dri2_get_buffers_with_format_reply_t *R  /**< */)
{
    return (xcb_dri2_dri2_buffer_t *) (R + 1);
}


/*****************************************************************************
 **
 ** int xcb_dri2_get_buffers_with_format_buffers_length
 ** 
 ** @param const xcb_dri2_get_buffers_with_format_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_dri2_get_buffers_with_format_buffers_length (const xcb_dri2_get_buffers_with_format_reply_t *R  /**< */)
{
    return R->count;
}


/*****************************************************************************
 **
 ** xcb_dri2_dri2_buffer_iterator_t xcb_dri2_get_buffers_with_format_buffers_iterator
 ** 
 ** @param const xcb_dri2_get_buffers_with_format_reply_t *R
 ** @returns xcb_dri2_dri2_buffer_iterator_t
 **
 *****************************************************************************/
 
xcb_dri2_dri2_buffer_iterator_t
xcb_dri2_get_buffers_with_format_buffers_iterator (const xcb_dri2_get_buffers_with_format_reply_t *R  /**< */)
{
    xcb_dri2_dri2_buffer_iterator_t i;
    i.data = (xcb_dri2_dri2_buffer_t *) (R + 1);
    i.rem = R->count;
    i.index = (char *) i.data - (char *) R;
    return i;
}


/*****************************************************************************
 **
 ** xcb_dri2_get_buffers_with_format_reply_t * xcb_dri2_get_buffers_with_format_reply
 ** 
 ** @param xcb_connection_t                           *c
 ** @param xcb_dri2_get_buffers_with_format_cookie_t   cookie
 ** @param xcb_generic_error_t                       **e
 ** @returns xcb_dri2_get_buffers_with_format_reply_t *
 **
 *****************************************************************************/
 
xcb_dri2_get_buffers_with_format_reply_t *
xcb_dri2_get_buffers_with_format_reply (xcb_connection_t                           *c  /**< */,
                                        xcb_dri2_get_buffers_with_format_cookie_t   cookie  /**< */,
                                        xcb_generic_error_t                       **e  /**< */)
{
    return (xcb_dri2_get_buffers_with_format_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}

