/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */
#ifndef __WLAN_TGT_DEF_CONFIG_H__
#define __WLAN_TGT_DEF_CONFIG_H__

/*
 * set of default target config , that can be over written by platform
 */

/*
 * default limit of 8 VAPs per device.
 */
/* Rome PRD support 5 vdevs */
#define CFG_TGT_NUM_VDEV                4

/*
 * We would need 1 AST entry per peer. Scale it by a factor of 2 to minimize hash collisions.
 * TODO: This scaling factor would be taken care inside the WAL in the future.
 */
#define CFG_TGT_NUM_PEER_AST            2

/* # of WDS entries to support.
 */
#define CFG_TGT_WDS_ENTRIES             32

/* MAC DMA burst size. 0: 128B - default, 1: 256B, 2: 64B
 */
#define CFG_TGT_DEFAULT_DMA_BURST_SIZE   0

/* Fixed delimiters to be inserted after every MPDU
 */
#define CFG_TGT_DEFAULT_MAC_AGGR_DELIM   0

/*
 * This value may need to be fine tuned, but a constant value will
 * probably always be appropriate; it is probably not necessary to
 * determine this value dynamically.
 */
#define CFG_TGT_AST_SKID_LIMIT          16 

/*
 * total number of peers per device.
 */
#define CFG_TGT_NUM_PEERS               8

/*
 * In offload mode target supports features like WOW, chatter and other
 * protocol offloads. In order to support them some functionalities like
 * reorder buffering, PN checking need to be done in target. This determines
 * maximum number of peers suported by target in offload mode
 */
#define CFG_TGT_NUM_OFFLOAD_PEERS       (CFG_TGT_NUM_PEERS + CFG_TGT_NUM_VDEV)

/*
 * Number of reorder buffers used in offload mode
 */
#define CFG_TGT_NUM_OFFLOAD_REORDER_BUFFS   8

/*
 * keys per peer node
 */
#define CFG_TGT_NUM_PEER_KEYS           2
/*
 * total number of data TX and RX TIDs 
 */
#define CFG_TGT_NUM_TIDS                (2 * CFG_TGT_NUM_PEERS)
/*
 * set this to 0x7 (Peregrine = 3 chains).
 * need to be set dynamically based on the HW capability.
 */
#define CFG_TGT_DEFAULT_TX_CHAIN_MASK   0x7
/*
 * set this to 0x7 (Peregrine = 3 chains).
 * need to be set dynamically based on the HW capability.
 */
#define CFG_TGT_DEFAULT_RX_CHAIN_MASK   0x7
/* 100 ms for video, best-effort, and background */
#define CFG_TGT_RX_TIMEOUT_LO_PRI       100
/* 40 ms for voice*/
#define CFG_TGT_RX_TIMEOUT_HI_PRI       40

/* AR9888 unified is default in ethernet mode */
#define CFG_TGT_RX_DECAP_MODE (0x2)
/* Decap to native Wifi header */
#define CFG_TGT_RX_DECAP_MODE_NWIFI (0x1)

/* maximum number of pending scan requests */
#define CFG_TGT_DEFAULT_SCAN_MAX_REQS   0x4

/* maximum number of VDEV that could use BMISS offload */
#define CFG_TGT_DEFAULT_BMISS_OFFLOAD_MAX_VDEV   0x3

/* maximum number of VDEV offload Roaming to support */
#define CFG_TGT_DEFAULT_ROAM_OFFLOAD_MAX_VDEV   0x3

/* maximum number of AP profiles pushed to offload Roaming */
#define CFG_TGT_DEFAULT_ROAM_OFFLOAD_MAX_PROFILES   0x8

/* maximum number of VDEV offload GTK to support */
#define CFG_TGT_DEFAULT_GTK_OFFLOAD_MAX_VDEV   0x3

/* default: mcast->ucast disabled if ATH_SUPPORT_MCAST2UCAST not defined */
#ifndef ATH_SUPPORT_MCAST2UCAST
#define CFG_TGT_DEFAULT_NUM_MCAST_GROUPS 0
#define CFG_TGT_DEFAULT_NUM_MCAST_TABLE_ELEMS 0
#define CFG_TGT_DEFAULT_MCAST2UCAST_MODE 0 /* disabled */
#else
/* (for testing) small multicast group membership table enabled */
#define CFG_TGT_DEFAULT_NUM_MCAST_GROUPS 4
#define CFG_TGT_DEFAULT_NUM_MCAST_TABLE_ELEMS 16
#define CFG_TGT_DEFAULT_MCAST2UCAST_MODE 2
#endif

/*
 * Specify how much memory the target should allocate for a debug log of
 * tx PPDU meta-information (how large the PPDU was, when it was sent,
 * whether it was successful, etc.)
 * The size of the log records is configurable, from a minimum of 28 bytes
 * to a maximum of about 300 bytes.  A typical configuration would result
 * in each log record being about 124 bytes.
 * Thus, 1KB of log space can hold about 30 small records, 3 large records,
 * or about 8 typical-sized records.
 */
#define CFG_TGT_DEFAULT_TX_DBG_LOG_SIZE 1024 /* bytes */

/* target based fragment timeout and MPDU duplicate detection */
#define CFG_TGT_DEFAULT_RX_SKIP_DEFRAG_TIMEOUT_DUP_DETECTION_CHECK 0

/*  Default VoW configuration
 */
#define CFG_TGT_DEFAULT_VOW_CONFIG   0

/*
 * total number of descriptors to use in the target
 */
#define CFG_TGT_NUM_MSDU_DESC    (1024 + 32)

/*
 * Maximum number of frag table entries
 */
#define CFG_TGT_MAX_FRAG_TABLE_ENTRIES 10

#endif  /*__WLAN_TGT_DEF_CONFIG_H__ */
