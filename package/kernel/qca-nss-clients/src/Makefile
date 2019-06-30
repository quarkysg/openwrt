# Makefile for the clients using the NSS driver

ccflags-y := -I$(obj) -I$(obj)/..

export BUILD_ID = \"Build Id: $(shell date +'%m/%d/%y, %H:%M:%S')\"
ccflags-y += -DNSS_CLIENT_BUILD_ID="$(BUILD_ID)"

obj-y+= profiler/
obj-y+= nss_qdisc/

ifeq ($(SoC), ipq807x)
obj-y+= nss_ppe_qdisc/
endif

# DTLS manager
obj-y+=dtls/

# CAPWAP Manager
ifneq ($(SoC), ipq807x)
obj-y+= capwapmgr/
endif

# Port interface Manager
ifneq ($(findstring 3.4, $(KERNELVERSION)),)
obj-y+= portifmgr/
endif

#IPv6

#Tun6RD
ifeq "$(CONFIG_IPV6_SIT_6RD)" "y"
obj-m += qca-nss-tun6rd.o
qca-nss-tun6rd-objs := nss_connmgr_tun6rd.o
ccflags-y += -DNSS_TUN6RD_DEBUG_LEVEL=0
endif

obj-m += qca-nss-tunipip6.o
qca-nss-tunipip6-objs := nss_connmgr_tunipip6.o
ccflags-y += -DNSS_TUNIPIP6_DEBUG_LEVEL=0

#NSS NETLINK
ifneq ($(findstring 3.4, $(KERNELVERSION)),)
obj-y+= netlink/
endif

# L2TPv2 manager
obj-y+=l2tp/l2tpv2/

#NSS PPTP
obj-y+= pptp/

#IPsecmgr
ifneq ($(SoC), ipq807x)
obj-y+= ipsecmgr/
endif

#LAG Manager
obj-y+= lag/

ifeq ($(SoC), ipq807x)
# Bridge manager
obj-y += bridge/
endif

# GRE manager
ifeq ($(findstring 3.4, $(KERNELVERSION)),)
obj-y+=gre/
endif

# Vlan manager
obj-y += vlan/

obj ?= .
