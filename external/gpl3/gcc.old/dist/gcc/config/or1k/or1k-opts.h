#ifndef OR1K_OPTS_H
#define OR1K_OPTS_H

enum or1k_delay {
  OR1K_DELAY_OFF = 0,
  OR1K_DELAY_ON = 1,
  OR1K_DELAY_COMPAT = 2
};

#define TARGET_DELAY_ON     (or1k_delay_selected == OR1K_DELAY_ON)
#define TARGET_DELAY_OFF    (or1k_delay_selected == OR1K_DELAY_OFF)
#define TARGET_DELAY_COMPAT (or1k_delay_selected == OR1K_DELAY_COMPAT)

#endif
