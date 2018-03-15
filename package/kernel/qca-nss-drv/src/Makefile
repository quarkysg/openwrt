# ###################################################
# # Makefile for the NSS driver
# ###################################################

obj-m += qca-nss-drv.o

#
# List the files that belong to the driver in alphabetical order.
#
qca-nss-drv-objs := \
			nss_cmn.o \
			nss_core.o \
			nss_coredump.o \
			nss_crypto.o \
			nss_dtls.o \
			nss_dynamic_interface.o \
			nss_gre_redir.o \
			nss_gre_tunnel.o \
			nss_if.o \
			nss_init.o \
			nss_ipsec.o \
			nss_ipv4.o \
			nss_ipv4_reasm.o \
			nss_ipv6.o \
			nss_ipv6_reasm.o \
			nss_lag.o \
			nss_lso_rx.o \
			nss_phys_if.o \
			nss_pm.o \
			nss_sjack.o \
			nss_stats.o \
			nss_tun6rd.o \
			nss_pptp.o \
			nss_l2tpv2.o \
			nss_map_t.o \
			nss_tunipip6.o \
			nss_virt_if.o \
			nss_shaper.o \
			nss_pppoe.o \
			nss_portid.o \
			nss_capwap.o \
			nss_eth_rx.o \
			nss_n2h.o \
			nss_oam.o \
			nss_data_plane.o \
			nss_log.o \
			nss_wifi.o \
			nss_wifi_vdev.o \
			nss_wifi_if.o

#
# TODO: Deprecated files should be removed before merge
#
qca-nss-drv-objs += \
			nss_tx_rx_virt_if.o

ccflags-y += -I$(obj)/nss_hal/include -I$(obj)/exports -DNSS_DEBUG_LEVEL=2 -DNSS_PKT_STATS_ENABLED=1
ccflags-y += -DNSS_PM_DEBUG_LEVEL=2 -DNSS_PPP_SUPPORT=1 -DNSS_SKB_RECYCLE_SUPPORT=1

ifneq ($(findstring 3.4, $(KERNELVERSION)),)
NSS_CCFLAGS = -DNSS_DT_SUPPORT=0 -DNSS_FW_DBG_SUPPORT=1 -DNSS_PM_SUPPORT=1 -DNSS_EMPTY_BUFFER_SIZE=1984
else
NSS_CCFLAGS = -DNSS_DT_SUPPORT=1 -DNSS_FW_DBG_SUPPORT=0 -DNSS_PM_SUPPORT=0 -DNSS_EMPTY_BUFFER_SIZE=1984
ccflags-y += -I$(obj)
endif

# Only the 3.14 Kernel implements fabric scaling framework and map-t
ifneq ($(findstring 3.14, $(KERNELVERSION)),)
NSS_CCFLAGS += -DNSS_FABRIC_SCALING_SUPPORT=1 -DNSS_MAP_T_SUPPORT=1
else
NSS_CCFLAGS += -DNSS_FABRIC_SCALING_SUPPORT=0 -DNSS_MAP_T_SUPPORT=0
endif

# Disable Frequency scaling
ifeq "$(NSS_FREQ_SCALE_DISABLE)" "y"
ccflags-y += -DNSS_FREQ_SCALE_SUPPORT=0
else
qca-nss-drv-objs += nss_freq.o
ccflags-y += -DNSS_FREQ_SCALE_SUPPORT=1
endif

ccflags-y += $(NSS_CCFLAGS)

export NSS_CCFLAGS

qca-nss-drv-objs += nss_profiler.o

obj ?= .

qca-nss-drv-objs += nss_hal/ipq806x/nss_hal_pvt.o
ccflags-y += -I$(obj)/nss_hal/ipq806x
