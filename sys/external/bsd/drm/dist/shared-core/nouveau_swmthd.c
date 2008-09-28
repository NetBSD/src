/*
 * Copyright (C) 2007 Arthur Huillet.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/*
 * Authors:
 *   Arthur Huillet <arthur.huillet AT free DOT fr>
 */

#include "drmP.h"
#include "drm.h"
#include "nouveau_drm.h"
#include "nouveau_drv.h"
#include "nouveau_reg.h"

/*TODO: add a "card_type" attribute*/
typedef struct{
	uint32_t oclass; /* object class for this software method */
	uint32_t mthd; /* method number */
	void (*method_code)(struct drm_device *dev, uint32_t oclass, uint32_t mthd); /* pointer to the function that does the work */
 } nouveau_software_method_t;


 /* This function handles the NV04 setcontext software methods.
One function for all because they are very similar.*/
static void nouveau_NV04_setcontext_sw_method(struct drm_device *dev, uint32_t oclass, uint32_t mthd) {
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	uint32_t inst_loc = NV_READ(NV04_PGRAPH_CTX_SWITCH4) & 0xFFFF;
	uint32_t value_to_set = 0, bit_to_set = 0;

	switch ( oclass ) {
		case 0x4a:
			switch ( mthd ) {
				case 0x188 :
				case 0x18c :
					bit_to_set = 0;
					break;
				case 0x198 :
					bit_to_set = 1 << 24; /*PATCH_STATUS_VALID*/
					break;
				case 0x2fc :
					bit_to_set = NV_READ(NV04_PGRAPH_TRAPPED_DATA) << 15; /*PATCH_CONFIG = NV04_PGRAPH_TRAPPED_DATA*/
					break;
				default : ;
				};
			break;
		case 0x5c:
			switch ( mthd ) {
				case 0x184:
					bit_to_set = 1 << 13; /*USER_CLIP_ENABLE*/
					break;
				case 0x188:
				case 0x18c:
					bit_to_set = 0;
					break;
				case 0x198:
					bit_to_set = 1 << 24; /*PATCH_STATUS_VALID*/
					break;
				case 0x2fc :
					bit_to_set = NV_READ(NV04_PGRAPH_TRAPPED_DATA) << 15; /*PATCH_CONFIG = NV04_PGRAPH_TRAPPED_DATA*/
					break;
			};
			break;
		case 0x5f:
			switch ( mthd ) {
				case 0x184 :
					bit_to_set = 1 << 12; /*CHROMA_KEY_ENABLE*/
					break;
				case 0x188 :
					bit_to_set = 1 << 13; /*USER_CLIP_ENABLE*/
					break;
				case 0x18c :
				case 0x190 :
					bit_to_set = 0;
					break;
				case 0x19c :
					bit_to_set = 1 << 24; /*PATCH_STATUS_VALID*/
					break;
				case 0x2fc :
					bit_to_set = NV_READ(NV04_PGRAPH_TRAPPED_DATA) << 15; /*PATCH_CONFIG = NV04_PGRAPH_TRAPPED_DATA*/
					break;
			};
			break;
		case 0x61:
			switch ( mthd ) {
				case 0x188 :
					bit_to_set = 1 << 13; /*USER_CLIP_ENABLE*/
					break;
				case 0x18c :
				case 0x190 :
					bit_to_set = 0;
					break;
				case 0x19c :
					bit_to_set = 1 << 24; /*PATCH_STATUS_VALID*/
					break;
				case 0x2fc :
					bit_to_set = NV_READ(NV04_PGRAPH_TRAPPED_DATA) << 15; /*PATCH_CONFIG = NV04_PGRAPH_TRAPPED_DATA*/
					break;
			};
			break;
		case 0x77:
			switch ( mthd ) {
				case 0x198 :
					bit_to_set = 1 << 24; /*PATCH_STATUS_VALID*/
					break;
				case 0x304 :
					bit_to_set = NV_READ(NV04_PGRAPH_TRAPPED_DATA) << 15; //PATCH_CONFIG
					break;
			};
			break;
		default :;
		};

	value_to_set = (NV_READ(0x00700000 | inst_loc << 4))| bit_to_set;

	/*RAMIN*/
	nouveau_wait_for_idle(dev);
	NV_WRITE(0x00700000 | inst_loc << 4, value_to_set);

	/*DRM_DEBUG("CTX_SWITCH1 value is %#x\n", NV_READ(NV04_PGRAPH_CTX_SWITCH1));*/
	NV_WRITE(NV04_PGRAPH_CTX_SWITCH1, value_to_set);

	/*DRM_DEBUG("CTX_CACHE1 + xxx value is %#x\n", NV_READ(NV04_PGRAPH_CTX_CACHE1 + (((NV_READ(NV04_PGRAPH_TRAPPED_ADDR) >> 13) & 0x7) << 2)));*/
	NV_WRITE(NV04_PGRAPH_CTX_CACHE1 + (((NV_READ(NV04_PGRAPH_TRAPPED_ADDR) >> 13) & 0x7) << 2), value_to_set);
}

 nouveau_software_method_t nouveau_sw_methods[] = {
	/*NV04 context software methods*/
	{ 0x4a, 0x188, nouveau_NV04_setcontext_sw_method },
	{ 0x4a, 0x18c, nouveau_NV04_setcontext_sw_method },
	{ 0x4a, 0x198, nouveau_NV04_setcontext_sw_method },
	{ 0x4a, 0x2fc, nouveau_NV04_setcontext_sw_method },
	{ 0x5c, 0x184, nouveau_NV04_setcontext_sw_method },
	{ 0x5c, 0x188, nouveau_NV04_setcontext_sw_method },
	{ 0x5c, 0x18c, nouveau_NV04_setcontext_sw_method },
	{ 0x5c, 0x198, nouveau_NV04_setcontext_sw_method },
	{ 0x5c, 0x2fc, nouveau_NV04_setcontext_sw_method },
	{ 0x5f, 0x184, nouveau_NV04_setcontext_sw_method },
	{ 0x5f, 0x188, nouveau_NV04_setcontext_sw_method },
	{ 0x5f, 0x18c, nouveau_NV04_setcontext_sw_method },
	{ 0x5f, 0x190, nouveau_NV04_setcontext_sw_method },
	{ 0x5f, 0x19c, nouveau_NV04_setcontext_sw_method },
	{ 0x5f, 0x2fc, nouveau_NV04_setcontext_sw_method },
	{ 0x61, 0x188, nouveau_NV04_setcontext_sw_method },
	{ 0x61, 0x18c, nouveau_NV04_setcontext_sw_method },
	{ 0x61, 0x190, nouveau_NV04_setcontext_sw_method },
	{ 0x61, 0x19c, nouveau_NV04_setcontext_sw_method },
	{ 0x61, 0x2fc, nouveau_NV04_setcontext_sw_method },
	{ 0x77, 0x198, nouveau_NV04_setcontext_sw_method },
	{ 0x77, 0x304, nouveau_NV04_setcontext_sw_method },
	/*terminator*/
	{ 0x0, 0x0, NULL, },
 };

 int nouveau_sw_method_execute(struct drm_device *dev, uint32_t oclass, uint32_t method) {
	int i = 0;
	while ( nouveau_sw_methods[ i ] . method_code != NULL )
		{
		if ( nouveau_sw_methods[ i ] . oclass == oclass && nouveau_sw_methods[ i ] . mthd == method )
			{
			nouveau_sw_methods[ i ] . method_code(dev, oclass, method);
			return 0;
			}
		i ++;
		}

	 return 1;
 }
