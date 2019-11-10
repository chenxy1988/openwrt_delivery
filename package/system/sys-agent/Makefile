#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=sysagent
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

PKG_MAINTAINER:=Xiangyu Chen <xiangyu.chen@aol.com>

define Package/sysagent
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=Simple agent for embedded system
  URL:=http://github.com/chenxy1988
endef

define Package/sysagent/description
	Simple agent for embedded system
endef

ifdef CONFIG_TARGET_zynq_DEVICE_xlnx_zynq-bms
	TARGET_CFLAGS += -DCONFIG_XILINX_BMU_PROJECT
endif

define Build/Compile
	$(TARGET_CC) $(PKG_BUILD_DIR)/sysagent.c -o $(PKG_BUILD_DIR)/$(PKG_NAME)
endef

define Package/sysagent/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/sysagent $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/sys.init $(1)/etc/init.d/$(PKG_NAME)
endef

$(eval $(call BuildPackage,sysagent))
