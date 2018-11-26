include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=qca-nss-ecm
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define KernelPackage/qca-nss-ecm/Default
  SECTION:=kernel
  CATEGORY:=Kernel modules
  SUBMENU:=Network Support
  DEPENDS:=@TARGET_ipq806x +kmod-qca-nss-drv +iptables-mod-extra +kmod-ipt-conntrack +kmod-shortcut-fe +kmod-l2tp +kmod-pppol2tp +kmod-bonding +kmod-pptp @!LINUX_3_18
  TITLE:=QCA NSS Enhanced Connection Manager (ECM)
  FILES:=$(PKG_BUILD_DIR)/ecm.ko
  KCONFIG:=CONFIG_BRIDGE_NETFILTER=y
endef

define KernelPackage/qca-nss-ecm/Description/Default
This package contains the QCA NSS Enhanced Connection Manager
endef

define KernelPackage/qca-nss-ecm/Default/install
	$(INSTALL_DIR) $(1)/etc/firewall.d $(1)/etc/init.d $(1)/usr/bin $(1)/lib/netifd/offload
	$(INSTALL_DATA) ./files/qca-nss-ecm.firewall $(1)/etc/firewall.d/qca-nss-ecm
	$(INSTALL_BIN) ./files/qca-nss-ecm.init $(1)/etc/init.d/qca-nss-ecm
	$(INSTALL_BIN) ./files/ecm_dump.sh $(1)/usr/bin/
	$(INSTALL_BIN) ./files/on-demand-down $(1)/lib/netifd/offload/on-demand-down
endef

define KernelPackage/qca-nss-ecm
  $(call KernelPackage/qca-nss-ecm/Default)
  AUTOLOAD:=$(call AutoLoad,52,ecm)
endef

define KernelPackage/qca-nss-ecm/Description
  $(call KernelPackage/qca-nss-ecm/Description/Default)
endef

define KernelPackage/qca-nss-ecm/install
$(call KernelPackage/qca-nss-ecm/Default/install, $(1))
	$(INSTALL_DIR) $(1)/etc/uci-defaults $(1)/etc/sysctl.d
	$(INSTALL_DATA) ./files/qca-nss-ecm.defaults $(1)/etc/uci-defaults/99-qca-nss-ecm
	$(INSTALL_BIN) ./files/qca-nss-ecm.sysctl $(1)/etc/sysctl.d/qca-nss-ecm.conf

	# install service by default
	$(INSTALL_DIR) $(1)/etc/rc.d
	$(LN) ../init.d/qca-nss-ecm $(1)/etc/rc.d/S70qca-nss-ecm
endef

# Variant with additional features enabled for premium profile
define KernelPackage/qca-nss-ecm-premium/Default
$(call KernelPackage/qca-nss-ecm/Default)
  TITLE+= (with premium features)
  VARIANT:=premium
  DEPENDS+=+kmod-qca-hyfi-bridge +kmod-qca-mcs +kmod-nat46
endef

define KernelPackage/qca-nss-ecm-premium/Description/Default
$(call KernelPackage/qca-nss-ecm/Description/Default)
with the premium features enabled
endef

define KernelPackage/qca-nss-ecm-premium/Default/install
$(call KernelPackage/qca-nss-ecm/install)
endef

define KernelPackage/qca-nss-ecm-premium
$(call KernelPackage/qca-nss-ecm-premium/Default)
AUTOLOAD:=$(call AutoLoad,52,ecm)
endef

define KernelPackage/qca-nss-ecm-premium/Description
$(call KernelPackage/qca-nss-ecm-premium/Description/Default)
endef

define KernelPackage/qca-nss-ecm-premium/install
$(call KernelPackage/qca-nss-ecm/install, $(1))
endef

define KernelPackage/qca-nss-ecm-noload
  $(call KernelPackage/qca-nss-ecm/Default)
  PROVIDES:=kmod-qca-nss-ecm
endef

define KernelPackage/qca-nss-ecm-noload/Description
  $(call KernelPackage/qca-nss-ecm/Description/Default)
  When selected, this package installs the driver but does not load it at init.
endef

define KernelPackage/qca-nss-ecm-noload/install
$(call KernelPackage/qca-nss-ecm/Default/install, $(1))
endef

define KernelPackage/qca-nss-ecm-premium-noload
  $(call KernelPackage/qca-nss-ecm-premium/Default)
  PROVIDES:=kmod-qca-nss-ecm-premium
endef

define KernelPackage/qca-nss-ecm-premium-noload/Description
  $(call KernelPackage/qca-nss-ecm-premium/Description/Default)
  When selected, this package installs the driver but does not load it at init.
endef

define KernelPackage/qca-nss-ecm-premium-noload/install
$(call KernelPackage/qca-nss-ecm-premium/Default/install, $(1))
endef

EXTRA_CFLAGS+= \
	-I$(STAGING_DIR)/usr/include/hyfibr \
	-I$(STAGING_DIR)/usr/include/qca-mcs \
	-I$(STAGING_DIR)/usr/include/qca-nss-drv \
	-I$(STAGING_DIR)/usr/include/shortcut-fe \
	-I$(STAGING_DIR)/usr/include/nat46

ECM_MAKE_OPTS:=
ifeq ($(LOCAL_VARIANT),premium)
ECM_MAKE_OPTS+=ECM_CLASSIFIER_HYFI_ENABLE=y \
		ECM_MULTICAST_ENABLE=y
endif

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" $(strip $(ECM_MAKE_OPTS)) \
		CROSS_COMPILE="$(TARGET_CROSS)" \
		ARCH="$(LINUX_KARCH)" \
		SUBDIRS="$(PKG_BUILD_DIR)" \
		EXTRA_CFLAGS="$(EXTRA_CFLAGS)" \
		modules
endef

define KernelPackage/qca-nss-ecm/config/Default
$(call Package/$(PKG_NAME)/override_source_path,kmod-$(PKG_NAME))
$(call Package/$(PKG_NAME)/override_version,kmod-$(PKG_NAME),$(PKG_SUPPORTED_VERSION))
endef

$(eval $(call KernelPackage,qca-nss-ecm-noload))
$(eval $(call KernelPackage,qca-nss-ecm))
$(eval $(call KernelPackage,qca-nss-ecm-premium-noload))
$(eval $(call KernelPackage,qca-nss-ecm-premium))
