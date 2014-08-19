#ifndef __NV50_DISP_H__
#define __NV50_DISP_H__

#include <core/parent.h>
#include <core/namedb.h>
#include <core/engctx.h>
#include <core/ramht.h>
#include <core/event.h>

#include <engine/dmaobj.h>

#include "dport.h"
#include "priv.h"

struct nv50_disp_impl {
	struct nouveau_disp_impl base;
	struct {
		const struct nv50_disp_mthd_chan *core;
		const struct nv50_disp_mthd_chan *base;
		const struct nv50_disp_mthd_chan *ovly;
		int prev;
	} mthd;
};

struct nv50_disp_priv {
	struct nouveau_disp base;
	struct nouveau_oclass *sclass;

	struct work_struct supervisor;
	u32 super;

	struct {
		int nr;
	} head;
	struct {
		int nr;
		int (*power)(struct nv50_disp_priv *, int dac, u32 data);
		int (*sense)(struct nv50_disp_priv *, int dac, u32 load);
	} dac;
	struct {
		int nr;
		int (*power)(struct nv50_disp_priv *, int sor, u32 data);
		int (*hda_eld)(struct nv50_disp_priv *, int sor, u8 *, u32);
		int (*hdmi)(struct nv50_disp_priv *, int head, int sor, u32);
		u32 lvdsconf;
		const struct nouveau_dp_func *dp;
	} sor;
	struct {
		int nr;
		int (*power)(struct nv50_disp_priv *, int ext, u32 data);
		u8 type[3];
		const struct nouveau_dp_func *dp;
	} pior;
};

#define HEAD_MTHD(n) (n), (n) + 0x03

int nv50_disp_base_scanoutpos(struct nouveau_object *, u32, void *, u32);

#define DAC_MTHD(n) (n), (n) + 0x03

int nv50_dac_mthd(struct nouveau_object *, u32, void *, u32);
int nv50_dac_power(struct nv50_disp_priv *, int, u32);
int nv50_dac_sense(struct nv50_disp_priv *, int, u32);

#define SOR_MTHD(n) (n), (n) + 0x3f

int nva3_hda_eld(struct nv50_disp_priv *, int, u8 *, u32);
int nvd0_hda_eld(struct nv50_disp_priv *, int, u8 *, u32);

int nv84_hdmi_ctrl(struct nv50_disp_priv *, int, int, u32);
int nva3_hdmi_ctrl(struct nv50_disp_priv *, int, int, u32);
int nvd0_hdmi_ctrl(struct nv50_disp_priv *, int, int, u32);

int nv50_sor_mthd(struct nouveau_object *, u32, void *, u32);
int nv50_sor_power(struct nv50_disp_priv *, int, u32);

int nv94_sor_dp_train_init(struct nv50_disp_priv *, int, int, int, u16, u16,
		           u32, struct dcb_output *);
int nv94_sor_dp_train_fini(struct nv50_disp_priv *, int, int, int, u16, u16,
		           u32, struct dcb_output *);
int nv94_sor_dp_train(struct nv50_disp_priv *, int, int, u16, u16, u32,
		      struct dcb_output *);
int nv94_sor_dp_lnkctl(struct nv50_disp_priv *, int, int, int, u16, u16, u32,
		       struct dcb_output *);
int nv94_sor_dp_drvctl(struct nv50_disp_priv *, int, int, int, u16, u16, u32,
		       struct dcb_output *);

int nvd0_sor_dp_train(struct nv50_disp_priv *, int, int, u16, u16, u32,
		      struct dcb_output *);
int nvd0_sor_dp_lnkctl(struct nv50_disp_priv *, int, int, int, u16, u16, u32,
		       struct dcb_output *);
int nvd0_sor_dp_drvctl(struct nv50_disp_priv *, int, int, int, u16, u16, u32,
		       struct dcb_output *);

#define PIOR_MTHD(n) (n), (n) + 0x03

int nv50_pior_mthd(struct nouveau_object *, u32, void *, u32);
int nv50_pior_power(struct nv50_disp_priv *, int, u32);

struct nv50_disp_base {
	struct nouveau_parent base;
	struct nouveau_ramht *ramht;
	u32 chan;
};

struct nv50_disp_chan {
	struct nouveau_namedb base;
	int chid;
};

int  nv50_disp_chan_create_(struct nouveau_object *, struct nouveau_object *,
			    struct nouveau_oclass *, int, int, void **);
void nv50_disp_chan_destroy(struct nv50_disp_chan *);
u32  nv50_disp_chan_rd32(struct nouveau_object *, u64);
void nv50_disp_chan_wr32(struct nouveau_object *, u64, u32);

#define nv50_disp_chan_init(a)                                                 \
	nouveau_namedb_init(&(a)->base)
#define nv50_disp_chan_fini(a,b)                                               \
	nouveau_namedb_fini(&(a)->base, (b))

int  nv50_disp_dmac_create_(struct nouveau_object *, struct nouveau_object *,
			    struct nouveau_oclass *, u32, int, int, void **);
void nv50_disp_dmac_dtor(struct nouveau_object *);

struct nv50_disp_dmac {
	struct nv50_disp_chan base;
	struct nouveau_dmaobj *pushdma;
	u32 push;
};

struct nv50_disp_pioc {
	struct nv50_disp_chan base;
};

struct nv50_disp_mthd_list {
	u32 mthd;
	u32 addr;
	struct {
		u32 mthd;
		u32 addr;
		const char *name;
	} data[];
};

struct nv50_disp_mthd_chan {
	const char *name;
	u32 addr;
	struct {
		const char *name;
		int nr;
		const struct nv50_disp_mthd_list *mthd;
	} data[];
};

extern struct nouveau_ofuncs nv50_disp_mast_ofuncs;
extern const struct nv50_disp_mthd_list nv50_disp_mast_mthd_base;
extern const struct nv50_disp_mthd_list nv50_disp_mast_mthd_sor;
extern const struct nv50_disp_mthd_list nv50_disp_mast_mthd_pior;
extern struct nouveau_ofuncs nv50_disp_sync_ofuncs;
extern const struct nv50_disp_mthd_list nv50_disp_sync_mthd_image;
extern struct nouveau_ofuncs nv50_disp_ovly_ofuncs;
extern const struct nv50_disp_mthd_list nv50_disp_ovly_mthd_base;
extern struct nouveau_ofuncs nv50_disp_oimm_ofuncs;
extern struct nouveau_ofuncs nv50_disp_curs_ofuncs;
extern struct nouveau_ofuncs nv50_disp_base_ofuncs;
extern struct nouveau_oclass nv50_disp_cclass;
void nv50_disp_mthd_chan(struct nv50_disp_priv *, int debug, int head,
			 const struct nv50_disp_mthd_chan *);
void nv50_disp_intr_supervisor(struct work_struct *);
void nv50_disp_intr(struct nouveau_subdev *);

extern const struct nv50_disp_mthd_chan nv84_disp_mast_mthd_chan;
extern const struct nv50_disp_mthd_list nv84_disp_mast_mthd_dac;
extern const struct nv50_disp_mthd_list nv84_disp_mast_mthd_head;
extern const struct nv50_disp_mthd_chan nv84_disp_sync_mthd_chan;
extern const struct nv50_disp_mthd_chan nv84_disp_ovly_mthd_chan;
extern struct nouveau_omthds nv84_disp_base_omthds[];

extern const struct nv50_disp_mthd_chan nv94_disp_mast_mthd_chan;

extern struct nouveau_ofuncs nvd0_disp_mast_ofuncs;
extern const struct nv50_disp_mthd_list nvd0_disp_mast_mthd_base;
extern const struct nv50_disp_mthd_list nvd0_disp_mast_mthd_dac;
extern const struct nv50_disp_mthd_list nvd0_disp_mast_mthd_sor;
extern const struct nv50_disp_mthd_list nvd0_disp_mast_mthd_pior;
extern struct nouveau_ofuncs nvd0_disp_sync_ofuncs;
extern struct nouveau_ofuncs nvd0_disp_ovly_ofuncs;
extern const struct nv50_disp_mthd_chan nvd0_disp_sync_mthd_chan;
extern struct nouveau_ofuncs nvd0_disp_oimm_ofuncs;
extern struct nouveau_ofuncs nvd0_disp_curs_ofuncs;
extern struct nouveau_omthds nvd0_disp_base_omthds[];
extern struct nouveau_ofuncs nvd0_disp_base_ofuncs;
extern struct nouveau_oclass nvd0_disp_cclass;
void nvd0_disp_intr_supervisor(struct work_struct *);
void nvd0_disp_intr(struct nouveau_subdev *);

extern const struct nv50_disp_mthd_chan nve0_disp_mast_mthd_chan;
extern const struct nv50_disp_mthd_chan nve0_disp_ovly_mthd_chan;

#endif
