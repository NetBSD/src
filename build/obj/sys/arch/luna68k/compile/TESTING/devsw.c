/*
 * MACHINE GENERATED: DO NOT EDIT
 *
 * devsw.c, from "/Users/sidqian/Downloads/summer/L2S/netbsd-src/sys/arch/luna68k/conf/TESTING"
 */

#include <sys/param.h>
#include <sys/conf.h>

/* device switch table for block device */
extern const struct bdevsw swap_bdevsw;
extern const struct bdevsw sd_bdevsw;
extern const struct bdevsw st_bdevsw;
extern const struct bdevsw cd_bdevsw;
extern const struct bdevsw md_bdevsw;
extern const struct bdevsw fss_bdevsw;
extern const struct bdevsw dk_bdevsw;

const struct bdevsw *bdevsw0[] = {
	NULL,	//   0
	&swap_bdevsw,	//   1
	&sd_bdevsw,	//   2
	&st_bdevsw,	//   3
	&cd_bdevsw,	//   4
	NULL,	//   5
	NULL,	//   6
	&md_bdevsw,	//   7
	NULL,	//   8
	NULL,	//   9
	NULL,	//  10
	NULL,	//  11
	NULL,	//  12
	NULL,	//  13
	NULL,	//  14
	NULL,	//  15
	NULL,	//  16
	NULL,	//  17
	NULL,	//  18
	NULL,	//  19
	NULL,	//  20
	NULL,	//  21
	NULL,	//  22
	NULL,	//  23
	NULL,	//  24
	NULL,	//  25
	NULL,	//  26
	NULL,	//  27
	NULL,	//  28
	NULL,	//  29
	NULL,	//  30
	NULL,	//  31
	NULL,	//  32
	NULL,	//  33
	NULL,	//  34
	NULL,	//  35
	NULL,	//  36
	NULL,	//  37
	NULL,	//  38
	NULL,	//  39
	NULL,	//  40
	NULL,	//  41
	NULL,	//  42
	NULL,	//  43
	NULL,	//  44
	NULL,	//  45
	NULL,	//  46
	NULL,	//  47
	NULL,	//  48
	NULL,	//  49
	NULL,	//  50
	NULL,	//  51
	NULL,	//  52
	NULL,	//  53
	NULL,	//  54
	NULL,	//  55
	NULL,	//  56
	NULL,	//  57
	NULL,	//  58
	NULL,	//  59
	NULL,	//  60
	NULL,	//  61
	NULL,	//  62
	NULL,	//  63
	NULL,	//  64
	NULL,	//  65
	NULL,	//  66
	NULL,	//  67
	NULL,	//  68
	NULL,	//  69
	NULL,	//  70
	NULL,	//  71
	NULL,	//  72
	NULL,	//  73
	NULL,	//  74
	NULL,	//  75
	NULL,	//  76
	NULL,	//  77
	NULL,	//  78
	NULL,	//  79
	NULL,	//  80
	NULL,	//  81
	NULL,	//  82
	NULL,	//  83
	NULL,	//  84
	NULL,	//  85
	NULL,	//  86
	NULL,	//  87
	NULL,	//  88
	NULL,	//  89
	NULL,	//  90
	NULL,	//  91
	NULL,	//  92
	NULL,	//  93
	NULL,	//  94
	NULL,	//  95
	NULL,	//  96
	NULL,	//  97
	NULL,	//  98
	NULL,	//  99
	NULL,	// 100
	NULL,	// 101
	NULL,	// 102
	NULL,	// 103
	NULL,	// 104
	NULL,	// 105
	NULL,	// 106
	NULL,	// 107
	NULL,	// 108
	NULL,	// 109
	NULL,	// 110
	NULL,	// 111
	NULL,	// 112
	NULL,	// 113
	NULL,	// 114
	NULL,	// 115
	NULL,	// 116
	NULL,	// 117
	NULL,	// 118
	NULL,	// 119
	NULL,	// 120
	NULL,	// 121
	NULL,	// 122
	NULL,	// 123
	NULL,	// 124
	NULL,	// 125
	NULL,	// 126
	NULL,	// 127
	NULL,	// 128
	NULL,	// 129
	NULL,	// 130
	NULL,	// 131
	NULL,	// 132
	NULL,	// 133
	NULL,	// 134
	NULL,	// 135
	NULL,	// 136
	NULL,	// 137
	NULL,	// 138
	NULL,	// 139
	NULL,	// 140
	NULL,	// 141
	NULL,	// 142
	NULL,	// 143
	NULL,	// 144
	NULL,	// 145
	NULL,	// 146
	NULL,	// 147
	NULL,	// 148
	NULL,	// 149
	NULL,	// 150
	NULL,	// 151
	NULL,	// 152
	NULL,	// 153
	NULL,	// 154
	NULL,	// 155
	NULL,	// 156
	NULL,	// 157
	NULL,	// 158
	NULL,	// 159
	NULL,	// 160
	NULL,	// 161
	NULL,	// 162
	&fss_bdevsw,	// 163
	NULL,	// 164
	NULL,	// 165
	NULL,	// 166
	NULL,	// 167
	&dk_bdevsw,	// 168
	NULL,	// 169
	NULL,	// 170
	NULL,	// 171
	NULL,	// 172
	NULL,	// 173
	NULL,	// 174
	NULL,	// 175
	NULL,	// 176
	NULL,	// 177
	NULL,	// 178
	NULL,	// 179
	NULL,	// 180
	NULL,	// 181
	NULL,	// 182
	NULL,	// 183
	NULL,	// 184
	NULL,	// 185
	NULL,	// 186
	NULL,	// 187
	NULL,	// 188
	NULL,	// 189
	NULL,	// 190
	NULL,	// 191
	NULL,	// 192
	NULL,	// 193
	NULL,	// 194
	NULL,	// 195
	NULL,	// 196
	NULL,	// 197
	NULL,	// 198
	NULL,	// 199
	NULL,	// 200
	NULL,	// 201
	NULL,	// 202
	NULL,	// 203
	NULL,	// 204
	NULL,	// 205
	NULL,	// 206
	NULL,	// 207
	NULL,	// 208
};

const struct bdevsw **bdevsw = bdevsw0;
const int sys_bdevsws = __arraycount(bdevsw0);
int max_bdevsws = __arraycount(bdevsw0);

/* device switch table for character device */
extern const struct cdevsw cons_cdevsw;
extern const struct cdevsw ctty_cdevsw;
extern const struct cdevsw mem_cdevsw;
extern const struct cdevsw swap_cdevsw;
extern const struct cdevsw pts_cdevsw;
extern const struct cdevsw ptc_cdevsw;
extern const struct cdevsw log_cdevsw;
extern const struct cdevsw siotty_cdevsw;
extern const struct cdevsw sd_cdevsw;
extern const struct cdevsw st_cdevsw;
extern const struct cdevsw cd_cdevsw;
extern const struct cdevsw md_cdevsw;
extern const struct cdevsw wskbd_cdevsw;
extern const struct cdevsw wsmouse_cdevsw;
extern const struct cdevsw wsdisplay_cdevsw;
extern const struct cdevsw filedesc_cdevsw;
extern const struct cdevsw bpf_cdevsw;
extern const struct cdevsw scsibus_cdevsw;
extern const struct cdevsw wsmux_cdevsw;
extern const struct cdevsw rnd_cdevsw;
extern const struct cdevsw clockctl_cdevsw;
extern const struct cdevsw ksyms_cdevsw;
extern const struct cdevsw xp_cdevsw;
extern const struct cdevsw lcd_cdevsw;
extern const struct cdevsw audio_cdevsw;
extern const struct cdevsw fss_cdevsw;
extern const struct cdevsw ptm_cdevsw;
extern const struct cdevsw drvctl_cdevsw;
extern const struct cdevsw dk_cdevsw;
extern const struct cdevsw cpuctl_cdevsw;

const struct cdevsw *cdevsw0[] = {
	&cons_cdevsw,	//   0
	&ctty_cdevsw,	//   1
	&mem_cdevsw,	//   2
	&swap_cdevsw,	//   3
	&pts_cdevsw,	//   4
	&ptc_cdevsw,	//   5
	&log_cdevsw,	//   6
	&siotty_cdevsw,	//   7
	&sd_cdevsw,	//   8
	&st_cdevsw,	//   9
	&cd_cdevsw,	//  10
	NULL,	//  11
	NULL,	//  12
	NULL,	//  13
	NULL,	//  14
	NULL,	//  15
	&md_cdevsw,	//  16
	&wskbd_cdevsw,	//  17
	&wsmouse_cdevsw,	//  18
	&wsdisplay_cdevsw,	//  19
	&filedesc_cdevsw,	//  20
	&bpf_cdevsw,	//  21
	NULL,	//  22
	NULL,	//  23
	NULL,	//  24
	NULL,	//  25
	NULL,	//  26
	NULL,	//  27
	NULL,	//  28
	NULL,	//  29
	NULL,	//  30
	&scsibus_cdevsw,	//  31
	NULL,	//  32
	&wsmux_cdevsw,	//  33
	&rnd_cdevsw,	//  34
	&clockctl_cdevsw,	//  35
	NULL,	//  36
	NULL,	//  37
	&ksyms_cdevsw,	//  38
	NULL,	//  39
	&xp_cdevsw,	//  40
	&lcd_cdevsw,	//  41
	&audio_cdevsw,	//  42
	NULL,	//  43
	NULL,	//  44
	NULL,	//  45
	NULL,	//  46
	NULL,	//  47
	NULL,	//  48
	NULL,	//  49
	NULL,	//  50
	NULL,	//  51
	NULL,	//  52
	NULL,	//  53
	NULL,	//  54
	NULL,	//  55
	NULL,	//  56
	NULL,	//  57
	NULL,	//  58
	NULL,	//  59
	NULL,	//  60
	NULL,	//  61
	NULL,	//  62
	NULL,	//  63
	NULL,	//  64
	NULL,	//  65
	NULL,	//  66
	NULL,	//  67
	NULL,	//  68
	NULL,	//  69
	NULL,	//  70
	NULL,	//  71
	NULL,	//  72
	NULL,	//  73
	NULL,	//  74
	NULL,	//  75
	NULL,	//  76
	NULL,	//  77
	NULL,	//  78
	NULL,	//  79
	NULL,	//  80
	NULL,	//  81
	NULL,	//  82
	NULL,	//  83
	NULL,	//  84
	NULL,	//  85
	NULL,	//  86
	NULL,	//  87
	NULL,	//  88
	NULL,	//  89
	NULL,	//  90
	NULL,	//  91
	NULL,	//  92
	NULL,	//  93
	NULL,	//  94
	NULL,	//  95
	NULL,	//  96
	NULL,	//  97
	NULL,	//  98
	NULL,	//  99
	NULL,	// 100
	NULL,	// 101
	NULL,	// 102
	NULL,	// 103
	NULL,	// 104
	NULL,	// 105
	NULL,	// 106
	NULL,	// 107
	NULL,	// 108
	NULL,	// 109
	NULL,	// 110
	NULL,	// 111
	NULL,	// 112
	NULL,	// 113
	NULL,	// 114
	NULL,	// 115
	NULL,	// 116
	NULL,	// 117
	NULL,	// 118
	NULL,	// 119
	NULL,	// 120
	NULL,	// 121
	NULL,	// 122
	NULL,	// 123
	NULL,	// 124
	NULL,	// 125
	NULL,	// 126
	NULL,	// 127
	NULL,	// 128
	NULL,	// 129
	NULL,	// 130
	NULL,	// 131
	NULL,	// 132
	NULL,	// 133
	NULL,	// 134
	NULL,	// 135
	NULL,	// 136
	NULL,	// 137
	NULL,	// 138
	NULL,	// 139
	NULL,	// 140
	NULL,	// 141
	NULL,	// 142
	NULL,	// 143
	NULL,	// 144
	NULL,	// 145
	NULL,	// 146
	NULL,	// 147
	NULL,	// 148
	NULL,	// 149
	NULL,	// 150
	NULL,	// 151
	NULL,	// 152
	NULL,	// 153
	NULL,	// 154
	NULL,	// 155
	NULL,	// 156
	NULL,	// 157
	NULL,	// 158
	NULL,	// 159
	NULL,	// 160
	NULL,	// 161
	NULL,	// 162
	&fss_cdevsw,	// 163
	NULL,	// 164
	&ptm_cdevsw,	// 165
	NULL,	// 166
	&drvctl_cdevsw,	// 167
	&dk_cdevsw,	// 168
	NULL,	// 169
	NULL,	// 170
	NULL,	// 171
	NULL,	// 172
	NULL,	// 173
	NULL,	// 174
	NULL,	// 175
	NULL,	// 176
	NULL,	// 177
	NULL,	// 178
	NULL,	// 179
	NULL,	// 180
	NULL,	// 181
	NULL,	// 182
	NULL,	// 183
	NULL,	// 184
	NULL,	// 185
	NULL,	// 186
	NULL,	// 187
	&cpuctl_cdevsw,	// 188
	NULL,	// 189
	NULL,	// 190
	NULL,	// 191
	NULL,	// 192
	NULL,	// 193
	NULL,	// 194
	NULL,	// 195
	NULL,	// 196
	NULL,	// 197
	NULL,	// 198
	NULL,	// 199
	NULL,	// 200
	NULL,	// 201
	NULL,	// 202
	NULL,	// 203
	NULL,	// 204
	NULL,	// 205
	NULL,	// 206
	NULL,	// 207
	NULL,	// 208
	NULL,	// 209
	NULL,	// 210
	NULL,	// 211
	NULL,	// 212
	NULL,	// 213
	NULL,	// 214
	NULL,	// 215
	NULL,	// 216
	NULL,	// 217
	NULL,	// 218
	NULL,	// 219
	NULL,	// 220
	NULL,	// 221
	NULL,	// 222
	NULL,	// 223
	NULL,	// 224
	NULL,	// 225
	NULL,	// 226
	NULL,	// 227
	NULL,	// 228
	NULL,	// 229
	NULL,	// 230
	NULL,	// 231
	NULL,	// 232
	NULL,	// 233
	NULL,	// 234
	NULL,	// 235
	NULL,	// 236
	NULL,	// 237
	NULL,	// 238
	NULL,	// 239
	NULL,	// 240
	NULL,	// 241
	NULL,	// 242
	NULL,	// 243
	NULL,	// 244
	NULL,	// 245
	NULL,	// 246
	NULL,	// 247
	NULL,	// 248
	NULL,	// 249
	NULL,	// 250
	NULL,	// 251
	NULL,	// 252
	NULL,	// 253
	NULL,	// 254
	NULL,	// 255
	NULL,	// 256
	NULL,	// 257
	NULL,	// 258
	NULL,	// 259
	NULL,	// 260
	NULL,	// 261
	NULL,	// 262
	NULL,	// 263
	NULL,	// 264
	NULL,	// 265
	NULL,	// 266
	NULL,	// 267
	NULL,	// 268
	NULL,	// 269
	NULL,	// 270
	NULL,	// 271
	NULL,	// 272
	NULL,	// 273
	NULL,	// 274
	NULL,	// 275
	NULL,	// 276
	NULL,	// 277
	NULL,	// 278
	NULL,	// 279
	NULL,	// 280
	NULL,	// 281
	NULL,	// 282
	NULL,	// 283
	NULL,	// 284
	NULL,	// 285
	NULL,	// 286
	NULL,	// 287
	NULL,	// 288
	NULL,	// 289
	NULL,	// 290
	NULL,	// 291
	NULL,	// 292
	NULL,	// 293
	NULL,	// 294
	NULL,	// 295
	NULL,	// 296
	NULL,	// 297
	NULL,	// 298
	NULL,	// 299
	NULL,	// 300
	NULL,	// 301
	NULL,	// 302
	NULL,	// 303
	NULL,	// 304
	NULL,	// 305
	NULL,	// 306
	NULL,	// 307
	NULL,	// 308
	NULL,	// 309
	NULL,	// 310
	NULL,	// 311
	NULL,	// 312
	NULL,	// 313
	NULL,	// 314
	NULL,	// 315
	NULL,	// 316
	NULL,	// 317
	NULL,	// 318
	NULL,	// 319
	NULL,	// 320
	NULL,	// 321
	NULL,	// 322
	NULL,	// 323
	NULL,	// 324
	NULL,	// 325
	NULL,	// 326
	NULL,	// 327
	NULL,	// 328
	NULL,	// 329
	NULL,	// 330
	NULL,	// 331
	NULL,	// 332
	NULL,	// 333
	NULL,	// 334
	NULL,	// 335
	NULL,	// 336
	NULL,	// 337
	NULL,	// 338
	NULL,	// 339
	NULL,	// 340
	NULL,	// 341
	NULL,	// 342
	NULL,	// 343
	NULL,	// 344
	NULL,	// 345
	NULL,	// 346
	NULL,	// 347
	NULL,	// 348
	NULL,	// 349
	NULL,	// 350
	NULL,	// 351
	NULL,	// 352
	NULL,	// 353
	NULL,	// 354
	NULL,	// 355
	NULL,	// 356
	NULL,	// 357
	NULL,	// 358
	NULL,	// 359
	NULL,	// 360
	NULL,	// 361
	NULL,	// 362
	NULL,	// 363
	NULL,	// 364
};

const struct cdevsw **cdevsw = cdevsw0;
const int sys_cdevsws = __arraycount(cdevsw0);
int max_cdevsws = __arraycount(cdevsw0);

/* device conversion table */
struct devsw_conv devsw_conv0[] = {
	{ "crypto", -1, 160, DEVNODE_SINGLE, 0, { 0, 0 }},
	{ "pf", -1, 161, DEVNODE_SINGLE, 0, { 0, 0 }},
	{ "fss", 163, 163, DEVNODE_VECTOR, 0, { 4, 0 }},
	{ "pps", -1, 164, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "ptm", -1, 165, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "atabus", -1, 166, DEVNODE_VECTOR, 0, { 4, 0 }},
	{ "drvctl", -1, 167, DEVNODE_SINGLE, 0, { 0, 0 }},
	{ "dk", 168, 168, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "tap", -1, 169, DEVNODE_VECTOR, 0, { 4, 0 }},
	{ "veriexec", -1, 170, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "fw", -1, 171, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "ucycom", -1, 172, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "gpio", -1, 173, DEVNODE_VECTOR, DEVNODE_FLAG_LINKZERO, { 8, 0 }},
	{ "utoppy", -1, 174, DEVNODE_VECTOR, 0, { 2, 0 }},
	{ "bthub", -1, 175, DEVNODE_SINGLE, 0, { 0, 0 }},
	{ "amr", -1, 176, DEVNODE_VECTOR, 0, { 1, 0 }},
	{ "lockstat", -1, 177, DEVNODE_SINGLE, 0, { 0, 0 }},
	{ "putter", -1, 178, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "srt", -1, 179, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "drm", -1, 180, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "bio", -1, 181, DEVNODE_SINGLE, 0, { 0, 0 }},
	{ "altmem", 182, 182, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "twa", -1, 187, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "cpuctl", -1, 188, DEVNODE_SINGLE, 0, { 0, 0 }},
	{ "pad", -1, 189, DEVNODE_VECTOR, DEVNODE_FLAG_LINKZERO, { 4, 0 }},
	{ "zfs", 190, 190, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "tprof", -1, 191, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "isv", -1, 192, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "video", -1, 193, DEVNODE_VECTOR, 0, { 4, 0 }},
	{ "dm", 169, 194, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "hdaudio", -1, 195, DEVNODE_VECTOR, 0, { 4, 0 }},
	{ "uhso", -1, 196, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "rumpblk", 197, 197, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "npf", -1, 198, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "flash", 199, 199, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "dtv", -1, 200, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "iic", -1, 201, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "iscsi", -1, 203, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "tpm", -1, 204, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "mfi", -1, 205, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "seeprom", -1, 206, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "dtrace", -1, 207, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "spiflash", 208, 208, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "lua", -1, 209, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "spkr", -1, 240, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "hdmicec", -1, 340, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "nvme", -1, 341, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "qemufwcfg", -1, 342, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "autofs", -1, 343, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "gpiopps", -1, 344, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "nvmm", -1, 345, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "kcov", -1, 346, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "spi", -1, 347, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "ipmi", -1, 354, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "vhci", -1, 355, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "vio9p", -1, 356, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "fault", -1, 357, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "wwanc", -1, 358, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "acpi", -1, 359, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "smbios", -1, 360, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "efi", -1, 361, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "sht3xtemp", -1, 362, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "scmd", -1, 363, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "viocon", -1, 364, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "cons", -1, 0, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "ctty", -1, 1, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "mem", -1, 2, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "swap", 1, 3, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "pts", -1, 4, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "ptc", -1, 5, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "log", -1, 6, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "siotty", -1, 7, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "sd", 2, 8, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "st", 3, 9, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "cd", 4, 10, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "ch", -1, 11, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "ss", -1, 12, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "uk", -1, 13, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "ccd", 5, 14, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "vnd", 6, 15, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "md", 7, 16, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "wskbd", -1, 17, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "wsmouse", -1, 18, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "wsdisplay", -1, 19, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "filedesc", -1, 20, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "bpf", -1, 21, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "tun", -1, 22, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "ipl", -1, 23, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "scsibus", -1, 31, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "raid", 14, 32, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "wsmux", -1, 33, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "rnd", -1, 34, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "clockctl", -1, 35, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "cgd", 15, 37, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "ksyms", -1, 38, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "wsfont", -1, 39, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "xp", -1, 40, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "lcd", -1, 41, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
	{ "audio", -1, 42, DEVNODE_DONTBOTHER, 0, { 0, 0 }},
};

struct devsw_conv *devsw_conv = devsw_conv0;
int max_devsw_convs = __arraycount(devsw_conv0);

const dev_t swapdev = makedev(1, 0);
const dev_t zerodev = makedev(2, DEV_ZERO);

/* mem_no is only used in iskmemdev() */
const int mem_no = 2;
