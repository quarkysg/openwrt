##################################
# Makefile for the NSS PPE QDISC.
##################################

ccflags-y := -I$(obj)/../exports -I$(obj)/../nss_qdisc -I$(srctree)/net/bridge -DNSS_PPE_QDISC_DEBUG_LEVEL=2 -DNSS_QDISC_DEBUG_LEVEL=2

obj-m += qca-nss-ppe-qdisc.o
qca-nss-ppe-qdisc-objs := 	nss_ppe_fifo.o \
				nss_ppe_htb.o \
				nss_ppe_qdisc.o \
				nss_ppe_red.o
