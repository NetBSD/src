
int always_match __P((struct device *, struct cfdata *, void *));

#define DEVICE_UNIT(device) (device->dv_unit)
#define CFDATA_LOC(cfdata) (cfdata->cf_loc)
