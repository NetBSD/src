// Check target CPUs are correctly passed.

// RUN: %clang -target aarch64 -### -c %s 2>&1 | FileCheck -check-prefix=GENERIC %s
// GENERIC: "-cc1"{{.*}} "-triple" "aarch64{{.*}}" "-target-cpu" "generic"

// RUN: %clang -target aarch64 -mcpu=cortex-a53 -### -c %s 2>&1 | FileCheck -check-prefix=CA53 %s
// CA53: "-cc1"{{.*}} "-triple" "aarch64{{.*}}" "-target-cpu" "cortex-a53"

// RUN: %clang -target aarch64 -mcpu=cortex-a57 -### -c %s 2>&1 | FileCheck -check-prefix=CA57 %s
// CA57: "-cc1"{{.*}} "-triple" "aarch64{{.*}}" "-target-cpu" "cortex-a57"
