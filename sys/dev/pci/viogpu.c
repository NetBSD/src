/*	$NetBSD: viogpu.c,v 1.3 2026/01/25 01:54:55 macallan Exp $ */
/*	$OpenBSD: viogpu.c,v 1.3 2023/05/29 08:13:35 sf Exp $ */

/*
 * Copyright (c) 2024-2025 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 2021-2023 joshua stein <jcs@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/condvar.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/kernel.h>
#include <sys/mutex.h>

#include <dev/pci/virtioreg.h>
#include <dev/pci/virtiovar.h>
#include <dev/pci/viogpu.h>

#include <dev/rasops/rasops.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsdisplay_vconsvar.h>

#include <prop/proplib.h>

struct viogpu_softc;

static int	viogpu_match(device_t, cfdata_t, void *);
static void	viogpu_attach(device_t, device_t, void *);
static void	viogpu_attach_postintr(device_t);
static int	viogpu_cmd_sync(struct viogpu_softc *, void *, size_t, void *,
				size_t);
static int	viogpu_cmd_req(struct viogpu_softc *, void *, size_t, size_t);
static void	viogpu_screen_update(void *);
static int	viogpu_vq_done(struct virtqueue *vq);

static int	viogpu_get_display_info(struct viogpu_softc *);
static int	viogpu_create_2d(struct viogpu_softc *, uint32_t, uint32_t,
				 uint32_t);
static int	viogpu_set_scanout(struct viogpu_softc *, uint32_t, uint32_t,
				   uint32_t, uint32_t);
static int	viogpu_attach_backing(struct viogpu_softc *, uint32_t,
				      bus_dmamap_t);
static int	viogpu_transfer_to_host_2d(struct viogpu_softc *sc, uint32_t,
					   uint32_t, uint32_t, uint32_t,
					   uint32_t);
static int	viogpu_flush_resource(struct viogpu_softc *, uint32_t,
				      uint32_t, uint32_t, uint32_t, uint32_t);

static int	viogpu_wsioctl(void *, void *, u_long, void *, int,
			       struct lwp *);

static void 	viogpu_init_screen(void *, struct vcons_screen *, int, long *);

static void	viogpu_cursor(void *, int, int, int);
static void	viogpu_putchar(void *, int, int, u_int, long);
static void	viogpu_copycols(void *, int, int, int, int);
static void	viogpu_erasecols(void *, int, int, int, long);
static void	viogpu_copyrows(void *, int, int, int);
static void	viogpu_eraserows(void *, int, int, long);
static void	viogpu_replaceattr(void *, long, long);

struct virtio_gpu_resource_attach_backing_entries {
	struct virtio_gpu_ctrl_hdr hdr;
	__le32 resource_id;
	__le32 nr_entries;
	struct virtio_gpu_mem_entry entries[1];
} __packed;

#define VIOGPU_CMD_DMA_SIZE \
    MAX(sizeof(struct virtio_gpu_resp_display_info), \
    MAX(sizeof(struct virtio_gpu_resource_create_2d), \
    MAX(sizeof(struct virtio_gpu_set_scanout), \
    MAX(sizeof(struct virtio_gpu_resource_attach_backing_entries), \
    MAX(sizeof(struct virtio_gpu_transfer_to_host_2d), \
    sizeof(struct virtio_gpu_resource_flush)))))) + \
    sizeof(struct virtio_gpu_ctrl_hdr)

struct viogpu_softc {
	device_t		sc_dev;
	struct virtio_softc	*sc_virtio;
#define	VQCTRL	0
#define	VQCURS	1
	struct virtqueue	sc_vqs[2];

	bus_dma_segment_t	sc_dma_seg;
	bus_dmamap_t		sc_dma_map;
	void			*sc_cmd;
	int			sc_fence_id;

	int			sc_fb_height;
	int			sc_fb_width;
	bus_dma_segment_t	sc_fb_dma_seg;
	bus_dmamap_t		sc_fb_dma_map;
	size_t			sc_fb_dma_size;
	void			*sc_fb_dma_kva;

	struct wsscreen_descr		sc_wsd;
	const struct wsscreen_descr	*sc_scrlist[1];
	struct wsscreen_list		sc_wsl;
	struct vcons_data		sc_vd;
	struct vcons_screen		sc_vcs;
	bool				is_console;

	void	(*ri_cursor)(void *, int, int, int);
	void	(*ri_putchar)(void *, int, int, u_int, long);
	void	(*ri_copycols)(void *, int, int, int, int);
	void	(*ri_erasecols)(void *, int, int, int, long);
	void	(*ri_copyrows)(void *, int, int, int);
	void	(*ri_eraserows)(void *, int, int, long);
	void	(*ri_replaceattr)(void *, long, long);

	/*
	 * sc_mutex protects is_requesting, needs_update, and req_wait. It is
	 * also held while submitting and reading the return values of
	 * asynchronous commands and for the full duration of synchronous
	 * commands.
	 */
	kmutex_t		sc_mutex;
	bool			is_requesting;
	bool			needs_update;
	kcondvar_t		req_wait;
	void			*update_soft_ih;
	size_t			cur_cmd_size;
	size_t			cur_ret_size;
};

CFATTACH_DECL_NEW(viogpu, sizeof(struct viogpu_softc),
		  viogpu_match, viogpu_attach, NULL, NULL);

#if VIOGPU_DEBUG
#define VIOGPU_FEATURES		(VIRTIO_GPU_F_VIRGL | VIRTIO_GPU_F_EDID)
#else
#define VIOGPU_FEATURES		0
#endif

static struct wsdisplay_accessops viogpu_accessops = {
	.ioctl        = viogpu_wsioctl,
	.mmap         = NULL, /* This would require signalling on write to
	                       * update the screen. */
	.alloc_screen = NULL,
	.free_screen  = NULL,
	.show_screen  = NULL,
	.load_font    = NULL,
	.pollc        = NULL,
	.scroll       = NULL,
};

static int
viogpu_match(device_t parent, cfdata_t match, void *aux)
{
	struct virtio_attach_args *va = aux;

	if (va->sc_childdevid == VIRTIO_DEVICE_ID_GPU)
		return 1;

	return 0;
}

static void
viogpu_attach(device_t parent, device_t self, void *aux)
{
	struct viogpu_softc *sc = device_private(self);
	struct virtio_softc *vsc = device_private(parent);
	int error;

	if (virtio_child(vsc) != NULL) {
		aprint_error("child already attached for %s\n",
		    device_xname(parent));
		return;
	}

	sc->sc_dev = self;
	sc->sc_virtio = vsc;

	mutex_init(&sc->sc_mutex, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&sc->req_wait, "vgpu_req");
	sc->update_soft_ih = softint_establish(SOFTINT_NET,
	    viogpu_screen_update, sc);
	sc->needs_update = false;
	sc->is_requesting = false;
	sc->sc_fence_id = 0;

	virtio_child_attach_start(vsc, self, IPL_VM,
	    VIOGPU_FEATURES, VIRTIO_COMMON_FLAG_BITS);

	if (!virtio_version_1(vsc)) {
		aprint_error_dev(sc->sc_dev, "requires virtio version 1\n");
		goto err;
	}

	/* Allocate command and cursor virtqueues. */
	virtio_init_vq_vqdone(vsc, &sc->sc_vqs[VQCTRL], 0, viogpu_vq_done);
	error = virtio_alloc_vq(vsc, &sc->sc_vqs[VQCTRL], NBPG, 1, "control");
	if (error != 0) {
		aprint_error_dev(sc->sc_dev, "alloc_vq failed: %d\n", error);
		goto err;
	}

	virtio_init_vq_vqdone(vsc, &sc->sc_vqs[VQCURS], 1, viogpu_vq_done);
	error = virtio_alloc_vq(vsc, &sc->sc_vqs[VQCURS], NBPG, 1, "cursor");
	if (error != 0) {
		aprint_error_dev(sc->sc_dev, "alloc_vq failed: %d\n", error);
		goto free_vq0;
	}

	if (virtio_child_attach_finish(vsc, sc->sc_vqs,
	    __arraycount(sc->sc_vqs), NULL,
	    VIRTIO_F_INTR_MPSAFE | VIRTIO_F_INTR_SOFTINT) != 0)
		goto free_vqs;

	/* Interrupts are required for synchronous commands in attachment. */
	config_interrupts(self, viogpu_attach_postintr);

	return;

free_vqs:
	virtio_free_vq(vsc, &sc->sc_vqs[VQCURS]);
free_vq0:
	virtio_free_vq(vsc, &sc->sc_vqs[VQCTRL]);
err:
	virtio_child_attach_failed(vsc);
	cv_destroy(&sc->req_wait);
	mutex_destroy(&sc->sc_mutex);
	return;
}

static void
viogpu_attach_postintr(device_t self)
{
	struct viogpu_softc *sc = device_private(self);
	struct virtio_softc *vsc = sc->sc_virtio;
	struct wsemuldisplaydev_attach_args waa;
	struct rasops_info *ri;
	long defattr;
	int nsegs;
	int error;

	/* Set up DMA space for sending commands. */
	error = bus_dmamap_create(virtio_dmat(vsc), VIOGPU_CMD_DMA_SIZE, 1,
	    VIOGPU_CMD_DMA_SIZE, 0, BUS_DMA_NOWAIT | BUS_DMA_ALLOCNOW,
	    &sc->sc_dma_map);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev, "bus_dmamap_create failed: %d\n",
		    error);
		goto err;
	}
	error = bus_dmamem_alloc(virtio_dmat(vsc), VIOGPU_CMD_DMA_SIZE, 16, 0,
	    &sc->sc_dma_seg, 1, &nsegs, BUS_DMA_NOWAIT);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev, "bus_dmamem_alloc failed: %d\n",
		    error);
		goto destroy;
	}
	error = bus_dmamem_map(virtio_dmat(vsc), &sc->sc_dma_seg, nsegs,
	    VIOGPU_CMD_DMA_SIZE, &sc->sc_cmd, BUS_DMA_NOWAIT);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev, "bus_dmamem_map failed: %d\n",
		    error);
		goto free;
	}
	memset(sc->sc_cmd, 0, VIOGPU_CMD_DMA_SIZE);
	error = bus_dmamap_load(virtio_dmat(vsc), sc->sc_dma_map, sc->sc_cmd,
	    VIOGPU_CMD_DMA_SIZE, NULL, BUS_DMA_NOWAIT);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev, "bus_dmamap_load failed: %d\n",
		    error);
		goto unmap;
	}

	if (viogpu_get_display_info(sc) != 0)
		goto unmap;

	/* Set up DMA space for actual framebuffer. */
	sc->sc_fb_dma_size = sc->sc_fb_width * sc->sc_fb_height * 4;
	error = bus_dmamap_create(virtio_dmat(vsc), sc->sc_fb_dma_size, 1,
	    sc->sc_fb_dma_size, 0, BUS_DMA_NOWAIT | BUS_DMA_ALLOCNOW,
	    &sc->sc_fb_dma_map);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev, "bus_dmamap_create failed: %d\n",
		    error);
		goto unmap;
	}
	error = bus_dmamem_alloc(virtio_dmat(vsc), sc->sc_fb_dma_size, 1024, 0,
	    &sc->sc_fb_dma_seg, 1, &nsegs, BUS_DMA_NOWAIT);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev, "bus_dmamem_alloc failed: %d\n",
		    error);
		goto fb_destroy;
	}
	error = bus_dmamem_map(virtio_dmat(vsc), &sc->sc_fb_dma_seg, nsegs,
	    sc->sc_fb_dma_size, &sc->sc_fb_dma_kva, BUS_DMA_NOWAIT);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev, "bus_dmamem_map failed: %d\n",
		    error);
		goto fb_free;
	}
	memset(sc->sc_fb_dma_kva, 0, sc->sc_fb_dma_size);
	error = bus_dmamap_load(virtio_dmat(vsc), sc->sc_fb_dma_map,
	    sc->sc_fb_dma_kva, sc->sc_fb_dma_size, NULL, BUS_DMA_NOWAIT);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev, "bus_dmamap_load failed: %d\n",
		    error);
		goto fb_unmap;
	}

	if (viogpu_create_2d(sc, 1, sc->sc_fb_width, sc->sc_fb_height) != 0)
		goto fb_unmap;

	if (viogpu_attach_backing(sc, 1, sc->sc_fb_dma_map) != 0)
		goto fb_unmap;

	if (viogpu_set_scanout(sc, 0, 1, sc->sc_fb_width,
	    sc->sc_fb_height) != 0)
		goto fb_unmap;

#ifdef WSDISPLAY_MULTICONS
	sc->is_console = true;
#else
	sc->is_console = device_getprop_bool(self, "is_console");
#endif

	sc->sc_wsd = (struct wsscreen_descr){
		"std",
		0, 0,
		NULL,
		8, 16,
		WSSCREEN_WSCOLORS | WSSCREEN_HILIT,
		NULL
	};

	sc->sc_scrlist[0] = &sc->sc_wsd;
	sc->sc_wsl.nscreens = __arraycount(sc->sc_scrlist);
	sc->sc_wsl.screens = sc->sc_scrlist;

	vcons_init(&sc->sc_vd, sc, &sc->sc_wsd, &viogpu_accessops);
	sc->sc_vd.init_screen = viogpu_init_screen;

	vcons_init_screen(&sc->sc_vd, &sc->sc_vcs, 1, &defattr);
	sc->sc_vcs.scr_flags |= VCONS_SCREEN_IS_STATIC;
	ri = &sc->sc_vcs.scr_ri;

	sc->sc_wsd.textops = &ri->ri_ops;
	sc->sc_wsd.capabilities = ri->ri_caps;
	sc->sc_wsd.nrows = ri->ri_rows;
	sc->sc_wsd.ncols = ri->ri_cols;

	if (sc->is_console) {
		wsdisplay_cnattach(&sc->sc_wsd, ri, 0, 0, defattr);
		vcons_replay_msgbuf(&sc->sc_vcs);
	}

	device_printf(sc->sc_dev, "%dx%d, %dbpp\n", ri->ri_width,
	    ri->ri_height, ri->ri_depth);

	waa.scrdata = &sc->sc_wsl;
	waa.accessops = &viogpu_accessops;
	waa.accesscookie = &sc->sc_vd;
	waa.console = sc->is_console;

	config_found(self, &waa, wsemuldisplaydevprint, CFARGS_NONE);

	return;

fb_unmap:
	bus_dmamem_unmap(virtio_dmat(vsc), &sc->sc_fb_dma_kva,
	    sc->sc_fb_dma_size);
fb_free:
	bus_dmamem_free(virtio_dmat(vsc), &sc->sc_fb_dma_seg, 1);
fb_destroy:
	bus_dmamap_destroy(virtio_dmat(vsc), sc->sc_fb_dma_map);
unmap:
	bus_dmamem_unmap(virtio_dmat(vsc), &sc->sc_cmd, VIOGPU_CMD_DMA_SIZE);
free:
	bus_dmamem_free(virtio_dmat(vsc), &sc->sc_dma_seg, 1);
destroy:
	bus_dmamap_destroy(virtio_dmat(vsc), sc->sc_dma_map);
err:
	aprint_error_dev(sc->sc_dev, "DMA setup failed\n");
	virtio_free_vq(vsc, &sc->sc_vqs[VQCURS]);
	virtio_free_vq(vsc, &sc->sc_vqs[VQCTRL]);
	virtio_child_attach_failed(vsc);
	cv_destroy(&sc->req_wait);
	mutex_destroy(&sc->sc_mutex);
	return;
}

/*
 * This carries out a command synchronously, unlike the commands used to
 * update the screen.
 */
static int
viogpu_cmd_sync(struct viogpu_softc *sc, void *cmd, size_t cmd_size,
		void *ret, size_t ret_size)
{
	int error;

	mutex_enter(&sc->sc_mutex);

	while (sc->is_requesting == true)
		cv_wait(&sc->req_wait, &sc->sc_mutex);

	error = viogpu_cmd_req(sc, cmd, cmd_size, ret_size);
	if (error != 0)
		goto out;

	while (sc->is_requesting == true)
		cv_wait(&sc->req_wait, &sc->sc_mutex);

	if (ret != NULL)
		memcpy(ret, (char *)sc->sc_cmd + cmd_size, ret_size);

out:
	mutex_exit(&sc->sc_mutex);

	return error;
}

static void
viogpu_screen_update(void *arg)
{
	struct viogpu_softc *sc = arg;

	mutex_enter(&sc->sc_mutex);

	if (sc->is_requesting == false)
		viogpu_transfer_to_host_2d(sc, 1, 0, 0, sc->sc_fb_width,
		    sc->sc_fb_height);
	else
		sc->needs_update = true;

	mutex_exit(&sc->sc_mutex);
}

static int
viogpu_cmd_req(struct viogpu_softc *sc, void *cmd, size_t cmd_size,
	       size_t ret_size)
{
	struct virtio_softc *vsc = sc->sc_virtio;
	struct virtqueue *vq = &sc->sc_vqs[VQCTRL];
	struct virtio_gpu_ctrl_hdr *hdr =
	    (struct virtio_gpu_ctrl_hdr *)sc->sc_cmd;
	int slot, error;

	memcpy(sc->sc_cmd, cmd, cmd_size);
	memset((char *)sc->sc_cmd + cmd_size, 0, ret_size);

#if VIOGPU_DEBUG
	printf("%s: [%zu -> %zu]: ", __func__, cmd_size, ret_size);
	for (int i = 0; i < cmd_size; i++) {
		printf(" %02x", ((unsigned char *)sc->sc_cmd)[i]);
	}
	printf("\n");
#endif

	hdr->flags |= virtio_rw32(vsc, VIRTIO_GPU_FLAG_FENCE);
	hdr->fence_id = virtio_rw64(vsc, ++sc->sc_fence_id);

	error = virtio_enqueue_prep(vsc, vq, &slot);
	if (error != 0)
		panic("%s: control vq busy", device_xname(sc->sc_dev));

	error = virtio_enqueue_reserve(vsc, vq, slot,
	    sc->sc_dma_map->dm_nsegs + 1);
	if (error != 0)
		panic("%s: control vq busy", device_xname(sc->sc_dev));

	bus_dmamap_sync(virtio_dmat(vsc), sc->sc_dma_map, 0, cmd_size,
	    BUS_DMASYNC_PREWRITE);
	virtio_enqueue_p(vsc, vq, slot, sc->sc_dma_map, 0, cmd_size, true);

	bus_dmamap_sync(virtio_dmat(vsc), sc->sc_dma_map, cmd_size, ret_size,
	    BUS_DMASYNC_PREREAD);
	virtio_enqueue_p(vsc, vq, slot, sc->sc_dma_map, cmd_size, ret_size,
	    false);

	virtio_enqueue_commit(vsc, vq, slot, true);

	sc->cur_cmd_size = cmd_size;
	sc->cur_ret_size = ret_size;
	sc->is_requesting = true;

	return 0;
}

static int
viogpu_vq_done(struct virtqueue *vq)
{
	struct virtio_softc *vsc = vq->vq_owner;
	struct viogpu_softc *sc = device_private(virtio_child(vsc));
	struct virtio_gpu_ctrl_hdr *resp;
	int slot, len;
	uint32_t cmd_type, resp_type;
	uint64_t resp_fence, expect_fence;
	bool next_req_sent = false;

	mutex_enter(&sc->sc_mutex);

	while (virtio_dequeue(vsc, vq, &slot, &len) != 0)
		;

	virtio_dequeue_commit(vsc, vq, slot);

	bus_dmamap_sync(virtio_dmat(vsc), sc->sc_dma_map, 0, sc->cur_cmd_size,
	    BUS_DMASYNC_POSTWRITE);
	bus_dmamap_sync(virtio_dmat(vsc), sc->sc_dma_map, sc->cur_cmd_size,
	    sc->cur_ret_size, BUS_DMASYNC_POSTREAD);

	resp = (struct virtio_gpu_ctrl_hdr *)((char *)sc->sc_cmd +
	    sc->cur_cmd_size);

	cmd_type = virtio_rw32(vsc,
	    ((struct virtio_gpu_ctrl_hdr *)sc->sc_cmd)->type);
	resp_type = virtio_rw32(vsc, resp->type);
	resp_fence = virtio_rw64(vsc, resp->fence_id);
	expect_fence = sc->sc_fence_id;

	switch (cmd_type) {
	case VIRTIO_GPU_CMD_TRANSFER_TO_HOST_2D:
		/* The second command for screen updating must be issued. */
		if (resp_type == VIRTIO_GPU_RESP_OK_NODATA) {
			viogpu_flush_resource(sc, 1, 0, 0, sc->sc_fb_width,
			    sc->sc_fb_height);
			next_req_sent = true;
		}
		break;
	case VIRTIO_GPU_CMD_RESOURCE_FLUSH:
		if (sc->needs_update == true) {
			viogpu_transfer_to_host_2d(sc, 1, 0, 0,
			    sc->sc_fb_width, sc->sc_fb_height);
			sc->needs_update = false;
			next_req_sent = true;
		}
		break;
	default:
		/* Other command types are called synchronously. */
		break;
	}

	if (next_req_sent == false) {
		sc->is_requesting = false;
		cv_broadcast(&sc->req_wait);
	}

	mutex_exit(&sc->sc_mutex);

	if (resp_type != VIRTIO_GPU_RESP_OK_NODATA) {
		switch (cmd_type) {
		case VIRTIO_GPU_CMD_TRANSFER_TO_HOST_2D:
			device_printf(sc->sc_dev,
			    "failed TRANSFER_TO_HOST: %d\n", resp_type);
			break;
		case VIRTIO_GPU_CMD_RESOURCE_FLUSH:
			device_printf(sc->sc_dev,
			    "failed RESOURCE_FLUSH: %d\n", resp_type);
			break;
		default:
			break;
		}
	}

	if (resp_fence != expect_fence)
		printf("%s: return fence id not right (0x%" PRIx64 " != 0x%"
		    PRIx64 ")\n", __func__, resp_fence, expect_fence);

	return 0;
}

static int
viogpu_get_display_info(struct viogpu_softc *sc)
{
	struct virtio_softc *vsc = sc->sc_virtio;
	struct virtio_gpu_ctrl_hdr hdr = { 0 };
	struct virtio_gpu_resp_display_info info = { 0 };

	hdr.type = virtio_rw32(vsc, VIRTIO_GPU_CMD_GET_DISPLAY_INFO);

	viogpu_cmd_sync(sc, &hdr, sizeof(hdr), &info, sizeof(info));

	if (virtio_rw32(vsc, info.hdr.type) !=
	    VIRTIO_GPU_RESP_OK_DISPLAY_INFO) {
		device_printf(sc->sc_dev, "failed getting display info\n");
		return 1;
	}

	if (!info.pmodes[0].enabled) {
		device_printf(sc->sc_dev, "pmodes[0] is not enabled\n");
		return 1;
	}

	sc->sc_fb_width = virtio_rw32(vsc, info.pmodes[0].r.width);
	sc->sc_fb_height = virtio_rw32(vsc, info.pmodes[0].r.height);

	return 0;
}

static int
viogpu_create_2d(struct viogpu_softc *sc, uint32_t resource_id, uint32_t width,
		 uint32_t height)
{
	struct virtio_softc *vsc = sc->sc_virtio;
	struct virtio_gpu_resource_create_2d res = { 0 };
	struct virtio_gpu_ctrl_hdr resp = { 0 };

	res.hdr.type = virtio_rw32(vsc, VIRTIO_GPU_CMD_RESOURCE_CREATE_2D);
	res.resource_id = virtio_rw32(vsc, resource_id);
	res.format = virtio_rw32(vsc, VIRTIO_GPU_FORMAT_B8G8R8X8_UNORM);
	res.width = virtio_rw32(vsc, width);
	res.height = virtio_rw32(vsc, height);

	viogpu_cmd_sync(sc, &res, sizeof(res), &resp, sizeof(resp));

	if (virtio_rw32(vsc, resp.type) != VIRTIO_GPU_RESP_OK_NODATA) {
		device_printf(sc->sc_dev, "failed CREATE_2D: %d\n",
		    virtio_rw32(vsc, resp.type));
		return 1;
	}

	return 0;
}

static int
viogpu_set_scanout(struct viogpu_softc *sc, uint32_t scanout_id,
		   uint32_t resource_id, uint32_t width, uint32_t height)
{
	struct virtio_softc *vsc = sc->sc_virtio;
	struct virtio_gpu_set_scanout ss = { 0 };
	struct virtio_gpu_ctrl_hdr resp = { 0 };

	ss.hdr.type = virtio_rw32(vsc, VIRTIO_GPU_CMD_SET_SCANOUT);
	ss.scanout_id = virtio_rw32(vsc, scanout_id);
	ss.resource_id = virtio_rw32(vsc, resource_id);
	ss.r.width = virtio_rw32(vsc, width);
	ss.r.height = virtio_rw32(vsc, height);

	viogpu_cmd_sync(sc, &ss, sizeof(ss), &resp, sizeof(resp));

	if (virtio_rw32(vsc, resp.type) != VIRTIO_GPU_RESP_OK_NODATA) {
		device_printf(sc->sc_dev, "failed SET_SCANOUT: %d\n",
		    virtio_rw32(vsc, resp.type));
		return 1;
	}

	return 0;
}

static int
viogpu_attach_backing(struct viogpu_softc *sc, uint32_t resource_id,
		      bus_dmamap_t dmamap)
{
	struct virtio_softc *vsc = sc->sc_virtio;
	struct virtio_gpu_resource_attach_backing_entries backing = { 0 };
	struct virtio_gpu_ctrl_hdr resp = { 0 };

	backing.hdr.type = virtio_rw32(vsc,
	    VIRTIO_GPU_CMD_RESOURCE_ATTACH_BACKING);
	backing.resource_id = virtio_rw32(vsc, resource_id);
	backing.nr_entries = virtio_rw32(vsc, __arraycount(backing.entries));
	backing.entries[0].addr = virtio_rw64(vsc, dmamap->dm_segs[0].ds_addr);
	backing.entries[0].length = virtio_rw32(vsc,
	    dmamap->dm_segs[0].ds_len);

	if (dmamap->dm_nsegs > 1)
		printf("%s: TODO: send all %d segs\n", __func__,
		    dmamap->dm_nsegs);

#if VIOGPU_DEBUG
	printf("%s: backing addr 0x%" PRIx64 " length %d\n", __func__,
	    backing.entries[0].addr, backing.entries[0].length);
#endif

	viogpu_cmd_sync(sc, &backing, sizeof(backing), &resp, sizeof(resp));

	if (virtio_rw32(vsc, resp.type) != VIRTIO_GPU_RESP_OK_NODATA) {
		device_printf(sc->sc_dev, "failed ATTACH_BACKING: %d\n",
		    virtio_rw32(vsc, resp.type));
		return 1;
	}

	return 0;
}

static int
viogpu_transfer_to_host_2d(struct viogpu_softc *sc, uint32_t resource_id,
			   uint32_t x, uint32_t y, uint32_t width,
			   uint32_t height)
{
	struct virtio_softc *vsc = sc->sc_virtio;
	struct virtio_gpu_transfer_to_host_2d tth = { 0 };

	tth.hdr.type = virtio_rw32(vsc, VIRTIO_GPU_CMD_TRANSFER_TO_HOST_2D);
	tth.resource_id = virtio_rw32(vsc, resource_id);
	tth.r.x = virtio_rw32(vsc, x);
	tth.r.y = virtio_rw32(vsc, y);
	tth.r.width = virtio_rw32(vsc, width);
	tth.r.height = virtio_rw32(vsc, height);
	tth.offset = virtio_rw64(vsc, (y * sc->sc_fb_width + x) *
	    4 /* bpp / 8 */);

	viogpu_cmd_req(sc, &tth, sizeof(tth),
	    sizeof(struct virtio_gpu_ctrl_hdr));

	return 0;
}

static int
viogpu_flush_resource(struct viogpu_softc *sc, uint32_t resource_id,
		      uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
	struct virtio_softc *vsc = sc->sc_virtio;
	struct virtio_gpu_resource_flush flush = { 0 };

	flush.hdr.type = virtio_rw32(vsc, VIRTIO_GPU_CMD_RESOURCE_FLUSH);
	flush.resource_id = virtio_rw32(vsc, resource_id);
	flush.r.x = virtio_rw32(vsc, x);
	flush.r.y = virtio_rw32(vsc, y);
	flush.r.width = virtio_rw32(vsc, width);
	flush.r.height = virtio_rw32(vsc, height);

	viogpu_cmd_req(sc, &flush, sizeof(flush),
	    sizeof(struct virtio_gpu_ctrl_hdr));

	return 0;
}

static int
viogpu_wsioctl(void *v, void *vs, u_long cmd, void *data, int flag,
	       struct lwp *l)
{
	struct rasops_info *ri = v;
	struct wsdisplayio_fbinfo *fbi;
	struct wsdisplay_fbinfo *wdf;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_VIOGPU;
		return 0;
	case WSDISPLAYIO_GET_FBINFO:
		fbi = (struct wsdisplayio_fbinfo *)data;
		return wsdisplayio_get_fbinfo(ri, fbi);
	case WSDISPLAYIO_GINFO:
		wdf = (struct wsdisplay_fbinfo *)data;
		wdf->height = ri->ri_height;
		wdf->width = ri->ri_width;
		wdf->depth = ri->ri_depth;
		wdf->cmsize = 0;
		return 0;
	case WSDISPLAYIO_LINEBYTES:
		*(u_int *)data = ri->ri_stride;
		return 0;
	case WSDISPLAYIO_SMODE:
		return 0;
	case WSDISPLAYIO_GVIDEO:
	case WSDISPLAYIO_SVIDEO:
		return 0;
	}

	return EPASSTHROUGH;
}

static void
viogpu_init_screen(void *cookie, struct vcons_screen *scr, int existing,
		   long *defattr)
{
	struct viogpu_softc *sc = cookie;
	struct rasops_info *ri = &scr->scr_ri;

	ri->ri_bits = sc->sc_fb_dma_kva;
	ri->ri_flg = RI_CENTER | RI_CLEAR;
#if BYTE_ORDER == BIG_ENDIAN
	ri->ri_flg |= RI_BSWAP;
#endif
	ri->ri_depth = 32;
	ri->ri_width = sc->sc_fb_width;
	ri->ri_height = sc->sc_fb_height;
	ri->ri_stride = ri->ri_width * ri->ri_depth / 8;
	ri->ri_bpos = 0;	/* B8G8R8X8 */
	ri->ri_bnum = 8;
	ri->ri_gpos = 8;
	ri->ri_gnum = 8;
	ri->ri_rpos = 16;
	ri->ri_rnum = 8;
	rasops_init(ri, 0, 0);
	ri->ri_caps = WSSCREEN_WSCOLORS | WSSCREEN_HILIT | WSSCREEN_RESIZE;
	scr->scr_flags |= VCONS_LOADFONT;
	rasops_reconfig(ri, sc->sc_fb_height / ri->ri_font->fontheight,
	    sc->sc_fb_width / ri->ri_font->fontwidth);

	/*
	 * Replace select text operations with wrappers that update the screen
	 * after the operation.
	 */
	sc->ri_cursor = ri->ri_ops.cursor;
	sc->ri_putchar = ri->ri_ops.putchar;
	sc->ri_copycols = ri->ri_ops.copycols;
	sc->ri_erasecols = ri->ri_ops.erasecols;
	sc->ri_copyrows = ri->ri_ops.copyrows;
	sc->ri_eraserows = ri->ri_ops.eraserows;
	sc->ri_replaceattr = ri->ri_ops.replaceattr;
	ri->ri_ops.cursor = ri->ri_ops.cursor == NULL ? NULL : viogpu_cursor;
	ri->ri_ops.putchar = ri->ri_ops.putchar == NULL ? NULL :
	    viogpu_putchar;
	ri->ri_ops.copycols = ri->ri_ops.copycols == NULL ? NULL :
	    viogpu_copycols;
	ri->ri_ops.erasecols = ri->ri_ops.erasecols == NULL ? NULL :
	    viogpu_erasecols;
	ri->ri_ops.copyrows = ri->ri_ops.copyrows == NULL ? NULL :
	    viogpu_copyrows;
	ri->ri_ops.eraserows = ri->ri_ops.eraserows == NULL ? NULL :
	    viogpu_eraserows;
	ri->ri_ops.replaceattr = ri->ri_ops.replaceattr == NULL ? NULL :
	    viogpu_replaceattr;
}

static void
viogpu_cursor(void *c, int on, int row, int col)
{
	struct rasops_info *ri = c;
	struct vcons_screen *vscr = ri->ri_hw;
	struct viogpu_softc *sc = vscr->scr_vd->cookie;

	sc->ri_cursor(c, on, row, col);

	softint_schedule(sc->update_soft_ih);
}

static void
viogpu_putchar(void *c, int row, int col, u_int uc, long attr)
{
	struct rasops_info *ri = c;
	struct vcons_screen *vscr = ri->ri_hw;
	struct viogpu_softc *sc = vscr->scr_vd->cookie;

	sc->ri_putchar(c, row, col, uc, attr);

	softint_schedule(sc->update_soft_ih);
}

static void
viogpu_copycols(void *c, int row, int srccol, int dstcol, int ncols)
{
	struct rasops_info *ri = c;
	struct vcons_screen *vscr = ri->ri_hw;
	struct viogpu_softc *sc = vscr->scr_vd->cookie;

	sc->ri_copycols(c, row, srccol, dstcol, ncols);

	softint_schedule(sc->update_soft_ih);
}

static void
viogpu_erasecols(void *c, int row, int startcol, int ncols, long attr)
{
	struct rasops_info *ri = c;
	struct vcons_screen *vscr = ri->ri_hw;
	struct viogpu_softc *sc = vscr->scr_vd->cookie;

	sc->ri_erasecols(c, row, startcol, ncols, attr);

	softint_schedule(sc->update_soft_ih);
}

static void
viogpu_copyrows(void *c, int srcrow, int dstrow, int nrows)
{
	struct rasops_info *ri = c;
	struct vcons_screen *vscr = ri->ri_hw;
	struct viogpu_softc *sc = vscr->scr_vd->cookie;

	sc->ri_copyrows(c, srcrow, dstrow, nrows);

	softint_schedule(sc->update_soft_ih);
}

static void
viogpu_eraserows(void *c, int row, int nrows, long attr)
{
	struct rasops_info *ri = c;
	struct vcons_screen *vscr = ri->ri_hw;
	struct viogpu_softc *sc = vscr->scr_vd->cookie;

	sc->ri_eraserows(c, row, nrows, attr);

	softint_schedule(sc->update_soft_ih);
}

static void
viogpu_replaceattr(void *c, long oldattr, long newattr)
{
	struct rasops_info *ri = c;
	struct vcons_screen *vscr = ri->ri_hw;
	struct viogpu_softc *sc = vscr->scr_vd->cookie;

	sc->ri_replaceattr(c, oldattr, newattr);

	softint_schedule(sc->update_soft_ih);
}
