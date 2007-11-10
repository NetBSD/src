#ifndef _ARM_OMAP_WDT_H_
#define _ARM_OMAP_WDT_H_

void omapwdt32k_set_timeout(unsigned int period);
int omapwdt32k_enable(int enable);
void omapwdt32k_reboot(void);

#endif /* _ARM_OMAP_WDT_H_ */
