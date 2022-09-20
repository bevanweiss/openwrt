# SPDX-License-Identifier: GPL-2.0-only

include $(TOPDIR)/rules.mk

ARCH:=mips
BOARD:=realtek
BOARDNAME:=Realtek SoC based boards
DEVICE_TYPE:=switch
SUBTARGETS:=otto-4kec otto-24kc rtl931x

FEATURES:=ramdisk squashfs

KERNEL_PATCHVER:=5.15

define Target/Description
       Build firmware images for Realtek RTL83xx/RTL93xx based boards.
endef

include $(INCLUDE_DIR)/target.mk

DEFAULT_PACKAGES += \
	ethtool \
	firewall4 \
	ip-bridge \
	ip-full \
	kmod-gpio-button-hotplug \
	kmod-nft-offload \
	nftables \
	odhcp6c \
	tc-bpf \
	uboot-envtools \


$(eval $(call BuildTarget))
