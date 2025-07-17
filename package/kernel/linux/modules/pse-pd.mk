#
# Copyright (C) 2006-2011 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

PSE_MENU:=PSE-PD / PoE support

define KernelPackage/pse-pd
  SUBMENU:=$(PSE_MENU)
  TITLE:=PSE-PD Support
  KCONFIG:=CONFIG_REGULATOR=y \
  CONFIG_PSE_CONTROLLER=y
endef

define KernelPackage/pse-pd/description
 Kernel module for PSE-PD support.
endef

$(eval $(call KernelPackage,pse-pd))

define AddDepends/pse-pd
  SUBMENU:=$(PSE_MENU)
  DEPENDS+=kmod-pse-pd $(1)
endef

define KernelPackage/pse-hasivo-hs104
  SUBMENU:=$(PSE_MENU)
  TITLE:=Hasivo HS104 PSE-PD support
  KCONFIG:=CONFIG_PSE_HASIVO_HS104
  FILES:=$(LINUX_DIR)/drivers/net/pse-pd/hasivo_hs104.ko
  AUTOLOAD:=$(call AutoProbe,hasivo_hs104)
  $(call AddDepends/pse-pd)
endef

define KernelPackage/pse-hasivo-hs104/description
 Kernel module for Hasivo HS104 PSE-PD chips
endef

$(eval $(call KernelPackage,pse-hasivo-hs104))
