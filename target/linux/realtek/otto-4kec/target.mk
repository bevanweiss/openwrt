# SPDX-License-Identifier: GPL-2.0-only
BOARDNAME:=Realtek RTL838x
CPU_TYPE:=4kec

define Target/Description
	Build firmware images for Realtek RTL838x (MIPS 4kec) based boards.
endef

FEATURES := $(filter-out mips16,$(FEATURES))
