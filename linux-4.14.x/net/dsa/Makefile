# SPDX-License-Identifier: GPL-2.0
# the core
obj-$(CONFIG_NET_DSA) += dsa_core.o
dsa_core-y += dsa.o dsa2.o legacy.o port.o slave.o switch.o

# tagging formats
dsa_core-$(CONFIG_NET_DSA_TAG_BRCM) += tag_brcm.o
dsa_core-$(CONFIG_NET_DSA_TAG_DSA) += tag_dsa.o
dsa_core-$(CONFIG_NET_DSA_TAG_EDSA) += tag_edsa.o
dsa_core-$(CONFIG_NET_DSA_TAG_KSZ) += tag_ksz.o
dsa_core-$(CONFIG_NET_DSA_TAG_LAN9303) += tag_lan9303.o
dsa_core-$(CONFIG_NET_DSA_TAG_MTK) += tag_mtk.o
dsa_core-$(CONFIG_NET_DSA_TAG_QCA) += tag_qca.o
dsa_core-$(CONFIG_NET_DSA_TAG_TRAILER) += tag_trailer.o

obj-$(CONFIG_WG_PLATFORM_DSA) += wg_dsa.o
