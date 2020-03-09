#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=libusl
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

PKG_MAINTAINER:=Xiangyu Chen <xiangyu.chen@aol.com>

define Package/libusl
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=Userspace Libraries for Application
  URL:=http://github.com/chenxy1988
endef

define Package/libusl/description
  USL - User Space Library
  ========================
  USL is a collection of useful routines for building Unix
  applications. We find ourselves writing this stuff over and over again
  each time we write a new application so we collected useful stuff
  together in one place and gave it a silly name.
  USL is released under LGPL.
endef

ifdef CONFIG_TARGET_zynq_DEVICE_xlnx_zynq-bms
	TARGET_CFLAGS += -DCONFIG_XILINX_BMU_PROJECT
endif

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR)  CC='$(TARGET_CC)'
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/lib \
                       $(1)/usr/include
	$(CP) $(PKG_BUILD_DIR)/libusl.a $(1)/usr/lib/
	$(CP) $(PKG_BUILD_DIR)/usl.h $(1)/usr/include/
endef

define Package/libusl/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/libusl.a $(1)/usr/lib/
endef

$(eval $(call BuildPackage,libusl))
