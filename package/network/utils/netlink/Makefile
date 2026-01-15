include $(TOPDIR)/rules.mk

PKG_NAME:=netlink
PKG_VERSION:=1.0
PKG_RELEASE:=1

PKG_MAINTAINER:=Bevan Weiss <bevan.weiss@gmail.com>
PKG_LICENSE:=GPL-2.0-only

# Dependencies: libubox, libubus, and kernel headers for ethtool netlink
PKG_BUILD_DEPENDS:=@LINUX_HEADERS
DEPENDS:=@LINUX_HEADERS

# Source is local folder
PKG_SOURCE:= 
PKG_SOURCE_URL:= 
PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

ETHTOOL_KERNEL_VERSION ?= 612

define Package/netlink
  SECTION:=net
  CATEGORY:=Network
  TITLE:=UBUS helper for netlink
  DEPENDS:=+libubus +libubox +libmnl
endef

define Package/netlink/description
Expose netlink topics via ubus (currently PSE-PD).
Future-proof: can add additional ethtool or netlink topics.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
	$(CP) ./src/ethtool/* $(PKG_BUILD_DIR)/ethtool/
endef

define Build/Compile
	$(TARGET_CC) $(TARGET_CFLAGS) \
		-I$(STAGING_DIR)/usr/include \
		-I$(STAGING_DIR)/usr/include/libubox \
		-I$(KERNEL_DIR)/include/uapi \
		-L$(STAGING_DIR)/usr/lib \
		-o $(PKG_BUILD_DIR)/netlink-ubus \
		$(PKG_BUILD_DIR)/main.c \
		$(PKG_BUILD_DIR)/nl.c \
		$(PKG_BUILD_DIR)/ubus.c \
		$(PKG_BUILD_DIR)/ethtool/*.c \
		-lubus -lubox -lmnl
endef

define Package/netlink/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/netlink-ubus $(1)/usr/sbin/
endef

$(eval $(call BuildPackage,netlink))
