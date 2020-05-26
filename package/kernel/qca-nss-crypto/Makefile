include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=qca-nss-crypto
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

# TODO this selection should be based on SOC version
# v1.0 is for Akronite
# v2.0 is for Hawkeye
NSS_CRYPTO_DIR:=v1.0

define KernelPackage/qca-nss-crypto/Default
  SECTION:=kernel
  CATEGORY:=Kernel modules
  SUBMENU:=Network Devices
  DEPENDS:=@TARGET_ipq806x||TARGET_ipq807x||TARGET_ipq +kmod-qca-nss-drv @!LINUX_3_18
endef

define KernelPackage/qca-nss-crypto
  $(call KernelPackage/qca-nss-crypto/Default)
  TITLE:=Kernel driver for NSS crypto driver
  FILES:=$(PKG_BUILD_DIR)/$(NSS_CRYPTO_DIR)/src/qca-nss-crypto.ko \
	$(PKG_BUILD_DIR)/$(NSS_CRYPTO_DIR)/tool/qca-nss-crypto-tool.ko
  AUTOLOAD:=$(call AutoLoad,52,qca-nss-crypto qca-nss-crypto-tool)
endef

define KernelPackage/qca-nss-crypto/Description
This package contains a NSS crypto driver for QCA chipset
endef

define Build/InstallDev/qca-nss-crypto
	$(INSTALL_DIR) $(1)/usr/include/qca-nss-crypto
	$(CP) $(PKG_BUILD_DIR)/$(NSS_CRYPTO_DIR)/include/* $(1)/usr/include/qca-nss-crypto
endef

define Build/InstallDev
	$(call Build/InstallDev/qca-nss-crypto,$(1))
endef

EXTRA_CFLAGS+= \
	-DCONFIG_NSS_DEBUG_LEVEL=4 \
	-I$(STAGING_DIR)/usr/include/qca-nss-crypto \
	-I$(STAGING_DIR)/usr/include/qca-nss-drv \
	-I$(PKG_BUILD_DIR)/$(NSS_CRYPTO_DIR)/include \
	-I$(PKG_BUILD_DIR)/$(NSS_CRYPTO_DIR)/src

define Build/Compile
	$(MAKE) $(PKG_JOBS) -C "$(LINUX_DIR)" \
		$(KERNEL_MAKE_FLAGS) \
		$(PKG_MAKE_FLAGS) \
		M="$(PKG_BUILD_DIR)" \
		EXTRA_CFLAGS="$(EXTRA_CFLAGS)" \
		NSS_CRYPTO_DIR=$(NSS_CRYPTO_DIR) \
		modules
endef

$(eval $(call KernelPackage,qca-nss-crypto))
