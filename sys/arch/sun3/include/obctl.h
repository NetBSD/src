
struct obctl_cf_loc {
    int obctl_addr;
    int obctl_size;
};

#define OBCTL_DEFAULT_PARAM(cast, arg, default) \
     (cast) (arg == -1 ? default : arg)

#define OBCTL_LOC(device) (struct obctl_cf_loc *) device->dv_cfdata->cf_loc


