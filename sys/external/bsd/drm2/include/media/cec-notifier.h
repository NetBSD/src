/* Public domain. */

#ifndef _MEDIA_CEC_NOTIFIER_H
#define _MEDIA_CEC_NOTIFIER_H

struct cec_connector_info {
};

#define cec_notifier_set_phys_addr_from_edid(x, y)	do { __USE(x); __USE(y); } while(0)
#define cec_notifier_phys_addr_invalidate(x)	do { __USE(x); } while(0)
#define cec_notifier_put(x)			do { __USE(x); } while(0)
#define cec_notifier_get_conn(x, y)		NULL
#define cec_fill_conn_info_from_drm(x, y)	do { __USE(x); __USE(y); } while(0)
#define cec_notifier_conn_register(x, y, z)	(void *)1
#define cec_notifier_conn_unregister(x)		do { __USE(x); } while(0)

#endif
