/*	$NetBSD: am7930var.h,v 1.9 2005/01/10 22:01:37 kent Exp $	*/

struct am7930_softc;

struct am7930_glue {
	u_int8_t	(*codec_iread) __P((struct am7930_softc *sc, int));
	void	(*codec_iwrite) __P((struct am7930_softc *sc, int, u_int8_t));
	u_int16_t	(*codec_iread16) __P((struct am7930_softc *sc, int));
	void	(*codec_iwrite16) __P((struct am7930_softc *sc, int, u_int16_t));
	void	(*onopen) __P((struct am7930_softc *sc));
	void	(*onclose) __P((struct am7930_softc *sc));
	int	factor;
	stream_filter_factory_t *input_conv;
	stream_filter_factory_t *output_conv;
};

struct am7930_softc {
	struct	device sc_dev;		/* base device */

	u_int8_t	sc_rlevel;	/* record level */
	u_int8_t	sc_plevel;	/* play level */
	u_int8_t	sc_mlevel;	/* monitor level */
	u_int8_t	sc_out_port;	/* output port */
	u_int8_t	sc_mic_mute;

	struct am7930_glue *sc_glue;
};

extern int     am7930debug;

void	am7930_init __P((struct am7930_softc *, int));

#define AM7930_IWRITE(x,y,z)	(*(x)->sc_glue->codec_iwrite)((x),(y),(z))
#define AM7930_IREAD(x,y)	(*(x)->sc_glue->codec_iread)((x),(y))
#define AM7930_IWRITE16(x,y,z)	(*(x)->sc_glue->codec_iwrite16)((x),(y),(z))
#define AM7930_IREAD16(x,y)	(*(x)->sc_glue->codec_iread16)((x),(y))

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

struct audio_device;
struct audio_encoding;
struct audio_params;

int	am7930_open __P((void *, int));
void	am7930_close __P((void *));
int	am7930_query_encoding __P((void *, struct audio_encoding *));
int	am7930_set_params __P((void *, int, int, audio_params_t *,
			    audio_params_t *, stream_filter_list_t *,
			    stream_filter_list_t *));
int	am7930_commit_settings __P((void *));
int	am7930_round_blocksize __P((void *, int, int, const audio_params_t *));
int	am7930_halt_output __P((void *));
int	am7930_halt_input __P((void *));
int	am7930_getdev __P((void *, struct audio_device *));
int	am7930_get_props __P((void *));
int	am7930_set_port __P((void *, mixer_ctrl_t *));
int	am7930_get_port __P((void *, mixer_ctrl_t *));
int	am7930_query_devinfo __P((void *, mixer_devinfo_t *));
