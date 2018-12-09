ccflags-y := -I$(obj)/../exports -I$(srctree)/net/bridge -DNSS_QDISC_DEBUG_LEVEL=2

obj-m += qca-nss-qdisc.o
qca-nss-qdisc-objs := 	nss_qdisc.o \
			nss_fifo.o \
			nss_codel.o \
			nss_tbl.o \
			nss_prio.o \
			nss_bf.o \
			nss_wrr.o \
			nss_htb.o \
			nss_blackhole.o \
			nss_wred.o
