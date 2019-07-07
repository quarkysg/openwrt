#
# Copyright (C) 2016-2017 Jason A. Donenfeld <Jason@zx2c4.com>
# Copyright (C) 2016 Baptiste Jonglez <openwrt@bitsofnetworks.org>
# Copyright (C) 2016-2017 Dan Luedtke <mail@danrl.com>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=wireguard

PKG_VERSION:=0.0.20190702
PKG_RELEASE:=1

PKG_SOURCE:=WireGuard-$(PKG_VERSION).tar.xz
PKG_SOURCE_URL:=https://git.zx2c4.com/WireGuard/snapshot/
PKG_HASH:=1a1311bc71abd47a72c47d918be3bacc486b3de90734661858af75cc990dbaac

PKG_LICENSE:=GPL-2.0 Apache-2.0
PKG_LICENSE_FILES:=COPYING

PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/WireGuard-$(PKG_VERSION)
PKG_BUILD_PARALLEL:=1
PKG_USE_MIPS16:=0

# WireGuard's makefile needs this to know where to build the kernel module
export KERNELDIR:=$(LINUX_DIR)

include $(INCLUDE_DIR)/package.mk

define Package/wireguard/Default
  SECTION:=net
  CATEGORY:=Network
  SUBMENU:=VPN
  URL:=https://www.wireguard.com
  MAINTAINER:=Baptiste Jonglez <openwrt@bitsofnetworks.org>, \
              Dan Luedtke <mail@danrl.com>, \
              Jason A. Donenfeld <Jason@zx2c4.com>
endef

define Package/wireguard/Default/description
  WireGuard is a novel VPN that runs inside the Linux Kernel and utilizes
  state-of-the-art cryptography. It aims to be faster, simpler, leaner, and
  more useful than IPSec, while avoiding the massive headache. It intends to
  be considerably more performant than OpenVPN.  WireGuard is designed as a
  general purpose VPN for running on embedded interfaces and super computers
  alike, fit for many different circumstances. It uses UDP.
endef

define Package/wireguard
  $(call Package/wireguard/Default)
  TITLE:=WireGuard meta-package
  DEPENDS:=+wireguard-tools +kmod-wireguard
  MENU:=1
endef

include $(INCLUDE_DIR)/kernel-defaults.mk
include $(INCLUDE_DIR)/package-defaults.mk

# Used by Build/Compile/Default
MAKE_PATH:=src/tools
MAKE_VARS += PLATFORM=linux

MAKE_OPTS:= EXTRA_CFLAGS="-I$(STAGING_DIR)/usr/include/qca-nss-drv"

define Build/Compile
	$(MAKE) $(KERNEL_MAKEOPTS) $(MAKE_OPTS) M="$(PKG_BUILD_DIR)/src" \
		$(strip WIREGUARD_NSS_SUPPORT=$(CONFIG_PACKAGE_WIREGUARD_NSS_SUPPORT)) \
		modules
	$(call Build/Compile/Default)
endef

define Package/wireguard/install
  true
endef

define Package/wireguard/description
  $(call Package/wireguard/Default/description)
endef

define Package/wireguard-tools
  $(call Package/wireguard/Default)
  TITLE:=WireGuard userspace control program (wg)
  DEPENDS:=+libmnl +ip
endef

define Package/wireguard-tools/description
  $(call Package/wireguard/Default/description)

  This package provides the userspace control program for WireGuard,
  `wg(8)`, and a netifd protocol helper.
endef

define Package/wireguard-tools/install
	$(INSTALL_DIR) $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/tools/wg $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/lib/netifd/proto/
	$(INSTALL_BIN) ./files/wireguard.sh $(1)/lib/netifd/proto/
endef

define KernelPackage/wireguard
  SECTION:=kernel
  CATEGORY:=Kernel modules
  SUBMENU:=Network Support
  TITLE:=WireGuard kernel module
  DEPENDS:=+IPV6:kmod-udptunnel6 +kmod-udptunnel4
  PKG_BUILD_DEPENDS+= PACKAGE_kmod-qca-nss-drv:kmod-qca-nss-drv
  FILES:= $(PKG_BUILD_DIR)/src/wireguard.$(LINUX_KMOD_SUFFIX)
  AUTOLOAD:=$(call AutoProbe,wireguard)
endef

define KernelPackage/wireguard/config
  if PACKAGE_kmod-wireguard
    if PACKAGE_kmod-qca-nss-drv
      config PACKAGE_WIREGUARD_NSS_SUPPORT
	bool "Enable NSS support for IPQ platform"
	default y
	help
	  This option enables support for NSS in boards
	  like AP148.
    endif
  endif
endef

define KernelPackage/wireguard/description
  $(call Package/wireguard/Default/description)

  This package provides the kernel module for WireGuard.
endef

$(eval $(call BuildPackage,wireguard))
$(eval $(call BuildPackage,wireguard-tools))
$(eval $(call KernelPackage,wireguard))
