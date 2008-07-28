#include "drmP.h"
#include "drm.h"
#include "nouveau_drv.h"
#include "nouveau_drm.h"

int
nv10_fb_init(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	uint32_t fb_bar_size;
	int i;

	fb_bar_size = drm_get_resource_len(dev, 0) - 1;
	for (i=0; i<NV10_PFB_TILE__SIZE; i++) {
		NV_WRITE(NV10_PFB_TILE(i), 0);
		NV_WRITE(NV10_PFB_TLIMIT(i), fb_bar_size);
	}

	return 0;
}

void
nv10_fb_takedown(struct drm_device *dev)
{
}
