# SPDX-FileCopyrightText: 2009 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
#
# SPDX-License-Identifier: GPL-2.0-only

sh runpaul-phase1.sh
mkdir runpaul-phase1
mv *.log runpaul-phase1/

sh runpaul-phase2.sh
mkdir runpaul-phase2
mv *.log runpaul-phase2/
