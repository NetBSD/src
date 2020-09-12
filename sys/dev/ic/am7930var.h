/*	$NetBSD: am7930var.h,v 1.15 2020/09/12 05:19:16 isaki Exp $	*/

struct am7930_softc;

struct am7930_glue {
	uint8_t	(*codec_dread)(struct am7930_softc *sc, int);
	void	(*codec_dwrite)(struct am7930_softc *sc, int, uint8_t);
};

struct am7930_buf {
	uint8_t *start;
	uint8_t *end;
	uint8_t *data;
	uint8_t *blkend;
	uint blksize;

	void (*intr)(void *);
	void *arg;
	uint intr_pending;
};

struct am7930_softc {
	device_t sc_dev;	/* base device */

	uint8_t	sc_rlevel;	/* record level */
	uint8_t	sc_plevel;	/* play level */
	uint8_t	sc_mlevel;	/* monitor level */
	uint8_t	sc_out_port;	/* output port */
	uint8_t	sc_mic_mute;

	struct am7930_glue *sc_glue;
	struct am7930_buf sc_p;	/* for play */
	struct am7930_buf sc_r;	/* for rec */

	kmutex_t sc_lock;
	kmutex_t sc_intr_lock;
	void *sc_sicookie;		/* softint(9) cookie */
	struct evcnt sc_intrcnt;	/* statistics */
};

extern int     am7930debug;

void	am7930_init(struct am7930_softc *, int);
int	am7930_hwintr(void *);
void	am7930_swintr(void *);

/* direct access functions */
#define AM7930_DWRITE(x,y,z)	(*(x)->sc_glue->codec_dwrite)((x),(y),(z))
#define AM7930_DREAD(x,y)	(*(x)->sc_glue->codec_dread)((x),(y))


#define AUDIOAMD_POLL_MODE	0
#define AUDIOAMD_DMA_MODE	1

/*
 * audio channel definitions.
 */

#define AUDIOAMD_SPEAKER_VOL	0	/* speaker volume */
#define AUDIOAMD_HEADPHONES_VOL	1	/* headphones volume */
#define AUDIOAMD_OUTPUT_CLASS	2

#define AUDIOAMD_MONITOR_VOL	3	/* monitor input volume */
#define AUDIOAMD_MONITOR_OUTPUT	4	/* output selector */
#define AUDIOAMD_MONITOR_CLASS	5

#define AUDIOAMD_MIC_VOL	6	/* microphone volume */
#define AUDIOAMD_MIC_MUTE	7
#define AUDIOAMD_INPUT_CLASS	8

#define AUDIOAMD_RECORD_SOURCE	9	/* source selector */
#define AUDIOAMD_RECORD_CLASS	10

/*
 * audio(9) MI callbacks from upper-level audio layer.
 */

int	am7930_query_format(void *, audio_format_query_t *);
int	am7930_set_format(void *, int,
	    const audio_params_t *, const audio_params_t *,
	    audio_filter_reg_t *, audio_filter_reg_t *);
int	am7930_commit_settings(void *);
int	am7930_trigger_output(void *, void *, void *, int, void (*)(void *),
	    void *, const audio_params_t *);
int	am7930_trigger_input(void *, void *, void *, int, void (*)(void *),
	    void *, const audio_params_t *);
int	am7930_halt_output(void *);
int	am7930_halt_input(void *);
int	am7930_getdev(void *, struct audio_device *);
int	am7930_get_props(void *);
int	am7930_set_port(void *, mixer_ctrl_t *);
int	am7930_get_port(void *, mixer_ctrl_t *);
int	am7930_query_devinfo(void *, mixer_devinfo_t *);
void	am7930_get_locks(void *, kmutex_t **, kmutex_t **);
