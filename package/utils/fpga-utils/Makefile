#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=fpga-utils
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

PKG_MAINTAINER:=Xiangyu Chen <xiangyu.chen@aol.com>

define Package/fpga-utils
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=Sync data with FPGA
  URL:=http://github.com/chenxy1988
endef

define Package/fpga-utils/description
	This tool can be used to access physical memory addresses from userspace for FPGA.
	It can be useful to access hardware for which no device driver exists!
endef

ifdef CONFIG_TARGET_zynq_DEVICE_xlnx_zynq-bms
	TARGET_CFLAGS += -DCONFIG_XILINX_BMU_PROJECT
endif

define Build/Compile
	$(TARGET_CC) $(TARGET_CFLAGS) -Wall $(PKG_BUILD_DIR)/fpga.c -o $(PKG_BUILD_DIR)/$(PKG_NAME)
endef

define Package/fpga-utils/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/fpga-utils $(1)/usr/bin/
endef

$(eval $(call BuildPackage,fpga-utils))
