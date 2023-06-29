
/* pseudo-devices */
void cpuctlattach(int);
void rndattach(int);
void bpfilterattach(int);
void loopattach(int);
void mdattach(int);
void ptyattach(int);
void fssattach(int);
void vlanattach(int);
void bridgeattach(int);
void agrattach(int);
void clockctlattach(int);
void drvctlattach(int);
void ksymsattach(int);
void swwdogattach(int);
void wsmuxattach(int);

/* driver structs */
extern struct cfdriver swwdog_cd;
extern struct cfdriver audio_cd;
extern struct cfdriver le_cd;
extern struct cfdriver spc_cd;
extern struct cfdriver wsdisplay_cd;
extern struct cfdriver wskbd_cd;
extern struct cfdriver wsmouse_cd;
extern struct cfdriver md_cd;
extern struct cfdriver fss_cd;
extern struct cfdriver mainbus_cd;
extern struct cfdriver clock_cd;
extern struct cfdriver lcd_cd;
extern struct cfdriver sio_cd;
extern struct cfdriver siotty_cd;
extern struct cfdriver xpbus_cd;
extern struct cfdriver xp_cd;
extern struct cfdriver psgpam_cd;
extern struct cfdriver ws_cd;
extern struct cfdriver fb_cd;
extern struct cfdriver scsibus_cd;
extern struct cfdriver cd_cd;
extern struct cfdriver sd_cd;
extern struct cfdriver st_cd;
