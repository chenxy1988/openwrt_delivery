#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=spi4bms
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

PKG_MAINTAINER:=Xiangyu Chen <xiangyu.chen@aol.com>

define Package/spi4bms
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=SPI seal layer BMS
  URL:=http://github.com/chenxy1988
endef

define Package/spi4bms/description
	SPI api for BMS
endef

ifdef CONFIG_TARGET_zynq_DEVICE_xlnx_zynq-bms
	TARGET_CFLAGS += -DCONFIG_XILINX_BMU_PROJECT
endif

define Build/Compile
	$(TARGET_CC) $(TARGET_CFLAGS) -Wall $(PKG_BUILD_DIR)/spi4bms.c -o $(PKG_BUILD_DIR)/$(PKG_NAME)
endef

define Package/can4bms/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/can4bms $(1)/usr/bin/
endef

$(eval $(call BuildPackage,spi4bms))
