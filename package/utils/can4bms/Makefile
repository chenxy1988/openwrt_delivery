#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=can4bms
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

PKG_MAINTAINER:=Xiangyu Chen <xiangyu.chen@aol.com>

define Package/can4bms
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=CAN bus server for BMS
  URL:=http://github.com/chenxy1988
endef

define Package/can4bms/description
	CAN bus server for listen whole data over CAN bus on BMS.
endef

ifdef CONFIG_TARGET_zynq_DEVICE_xlnx_zynq-bms
	TARGET_CFLAGS += -DCONFIG_XILINX_BMU_PROJECT
endif

define Build/Compile
	$(TARGET_CC) $(TARGET_CFLAGS) $(PKG_BUILD_DIR)/can4bms.c $(PKG_BUILD_DIR)/network.h $(PKG_BUILD_DIR)/network.c  $(PKG_BUILD_DIR)/protocol.c -o $(PKG_BUILD_DIR)/$(PKG_NAME) -lpthread
endef

define Package/can4bms/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/can4bms $(1)/usr/bin/
endef

$(eval $(call BuildPackage,can4bms))
