/*
 * Driver support for the intersil7170 used in sun[34]s to provide
 * real time clock and time-of-day support.
 * 
 * Derived from: datasheet "ICM7170 a uP-Compatible Real-Time Clock"
 *                          document #301680-005, Dec 85
 */

struct intersil_map {		       /* from p. 7 of 10 */
    unsigned char csecs;
    unsigned char hours;
    unsigned char minutes;
    unsigned char seconds;
    unsigned char month;
    unsigned char date;
    unsigned char year;
    unsigned char day;
};

struct intersil7170 {
    struct intersil_map counters;
    struct intersil_map ram;	/* should be ok as both are word aligned */
    unsigned char interrupt_reg;
    unsigned char command_reg;
};

/*  bit assignments for command register, p. 6 of 10, write-only */
#define INTERSIL_CMD_FREQ_32K    0x0 
#define INTERSIL_CMD_FREQ_1M     0x1
#define INTERSIL_CMD_FREQ_2M     0x2
#define INTERSIL_CMD_FREQ_4M     0x3

#define INTERSIL_CMD_12HR_MODE   0x0
#define INTERSIL_CMD_24HR_MODE   0x4

#define INTERSIL_CMD_STOP        0x0
#define INTERSIL_CMD_RUN         0x8

#define INTERSIL_CMD_IDISABLE   0x0
#define INTERSIL_CMD_IENABLE   0x10

#define INTERSIL_CMD_TEST_MODE       0x0
#define INTERSIL_CMD_NORMAL_MODE    0x20

/* bit assignments for interrupt register r/w, p 7 of 10*/

#define INTERSIL_INTER_ALARM       0x1 /* r/w */
#define INTERSIL_INTER_CSECONDS    0x2 /* r/w */
#define INTERSIL_INTER_DSECONDS    0x4 /* r/w */
#define INTERSIL_INTER_SECONDS     0x8 /* r/w */
#define INTERSIL_INTER_MINUTES    0x10 /* r/w */
#define INTERSIL_INTER_HOURS      0x20 /* r/w */
#define INTERSIL_INTER_DAYS       0x40 /* r/w */
#define INTERSIL_INTER_PENDING    0x80 /* read-only */

/* useful info */

#define INTERSIL_YEAR_BASE 68
#define INTERSIL_UNIX_BASE 70

#define INTERSIL_LEAP_YEAR(x) !((x) % 4)
