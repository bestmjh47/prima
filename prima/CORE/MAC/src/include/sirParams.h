/*
* Copyright (c) 2012 Qualcomm Atheros, Inc.
* All Rights Reserved.
* Qualcomm Atheros Confidential and Proprietary.
*/

/*
 * Airgo Networks, Inc proprietary. All rights reserved.
 * This file sirParams.h contains the common parameter definitions, which
 * are not dependent on threadX API. These can be used by all Firmware
 * modules.
 *
 * Author:      Sandesh Goel
 * Date:        04/13/2002
 * History:-
 * Date            Modified by    Modification Information
 * --------------------------------------------------------------------
 */

#ifndef __SIRPARAMS_H
#define __SIRPARAMS_H

# include "sirTypes.h"

#define SIR_SDK_OPT_MAX_NUM_PRE_AUTH    32

// Firmware wide constants

#define SIR_MAX_PACKET_SIZE     2048
#define SIR_MAX_NUM_CHANNELS    64
#define SIR_MAX_NUM_STA_IN_IBSS 16
#define SIR_MAX_NUM_STA_IN_BSS  256

#if defined( FEATURE_WLAN_INTEGRATED_SOC )
typedef enum
{
    PHY_SINGLE_CHANNEL_CENTERED = 0,        // 20MHz IF bandwidth centered on IF carrier
    PHY_DOUBLE_CHANNEL_LOW_PRIMARY = 1,     // 40MHz IF bandwidth with lower 20MHz supporting the primary channel
    //not allowed PHY_DOUBLE_CHANNEL_CENTERED = 2,        // 40MHz IF bandwidth centered on IF carrier
    PHY_DOUBLE_CHANNEL_HIGH_PRIMARY = 3     // 40MHz IF bandwidth with higher 20MHz supporting the primary channel
}ePhyChanBondState;
#endif /* FEATURE_WLAN_INTEGRATED_SOC */

typedef enum eSriLinkState {
    eSIR_LINK_IDLE_STATE        = 0,
    eSIR_LINK_PREASSOC_STATE    = 1,
    eSIR_LINK_POSTASSOC_STATE   = 2,
    eSIR_LINK_AP_STATE          = 3,
    eSIR_LINK_IBSS_STATE        = 4,
    // BT-AMP Case
    eSIR_LINK_BTAMP_PREASSOC_STATE  = 5,
    eSIR_LINK_BTAMP_POSTASSOC_STATE  = 6,
    eSIR_LINK_BTAMP_AP_STATE  = 7,
    eSIR_LINK_BTAMP_STA_STATE  = 8,

    // Reserved for HAL internal use
    eSIR_LINK_LEARN_STATE       = 9,
    eSIR_LINK_SCAN_STATE        = 10,
    eSIR_LINK_FINISH_SCAN_STATE = 11,
    eSIR_LINK_INIT_CAL_STATE    = 12,
    eSIR_LINK_FINISH_CAL_STATE  = 13,
    eSIR_LINK_LISTEN_STATE = 14
} tSirLinkState;


/// Message queue structure used across Sirius project.
/// NOTE: this structure should be multiples of a word size (4bytes)
/// as this is used in tx_queue where it expects to be multiples of 4 bytes.
typedef struct sSirMsgQ
{
    tANI_U16 type;
    /*
     * This field can be used as sequence number/dialog token for matching
     * requests and responses.
     */
    tANI_U16 reserved;
    /**
     * Based on the type either a bodyptr pointer into
     * memory or bodyval as a 32 bit data is used.
     * bodyptr: is always a freeable pointer, one should always
     * make sure that bodyptr is always freeable.
     *
     * Messages should use either bodyptr or bodyval; not both !!!.
     */
    void *bodyptr;
    tANI_U32 bodyval;
} tSirMsgQ, *tpSirMsgQ;

/// Mailbox Message Structure Define
typedef struct sSirMbMsg
{
    tANI_U16 type;

    /**
     * This length includes 4 bytes of header, that is,
     * 2 bytes type + 2 bytes msgLen + n*4 bytes of data.
     * This field is byte length.
     */
    tANI_U16 msgLen;

    /**
     * This is the first data word in the mailbox message.
     * It is followed by n words of data.
     * NOTE: data[1] is not a place holder to store data
     * instead to dereference the message body.
     */
    tANI_U32 data[1];
} tSirMbMsg, *tpSirMbMsg;

#ifdef WLAN_FEATURE_P2P
/// Mailbox Message Structure for P2P 
typedef struct sSirMbMsgP2p
{
    tANI_U16 type;

    /**
     * This length includes 4 bytes of header, that is,
     * 2 bytes type + 2 bytes msgLen + n*4 bytes of data.
     * This field is byte length.
     */
    tANI_U16 msgLen;

    tANI_U16 sessionId;
    tANI_U16 wait;

    /**
     * This is the first data word in the mailbox message.
     * It is followed by n words of data.
     * NOTE: data[1] is not a place holder to store data
     * instead to dereference the message body.
     */
    tANI_U32 data[1];
} tSirMbMsgP2p, *tpSirMbMsgP2p;
#endif

/// Message queue definitions
//  msgtype(2bytes) reserved(2bytes) bodyptr(4bytes) bodyval(4bytes)
//  NOTE tSirMsgQ should be always multiples of WORD(4Bytes)
//  All Queue Message Size are multiples of word Size (4 bytes)
#define SYS_MSG_SIZE            (sizeof(tSirMsgQ)/4)

/// gHalMsgQ

#define SYS_HAL_MSG_SIZE        SYS_MSG_SIZE

/// gMMHhiPriorityMsgQ

#define SYS_MMH_HI_PRI_MSG_SIZE SYS_MSG_SIZE

/// gMMHprotocolMsgQ

#define SYS_MMH_PROT_MSG_SIZE   SYS_MSG_SIZE

/// gMMHdebugMsgQ

#define SYS_MMH_DEBUG_MSG_SIZE  SYS_MSG_SIZE

/// gMAINTmsgQ

#define SYS_MNT_MSG_SIZE        SYS_MSG_SIZE

/// LIM Message Queue

#define SYS_LIM_MSG_SIZE        SYS_MSG_SIZE

/// ARQ Message Queue

#define SYS_ARQ_MSG_SIZE        SYS_MSG_SIZE

/// Scheduler Message Queue

#define SYS_SCH_MSG_SIZE        SYS_MSG_SIZE

/// PMM Message Queue

#define SYS_PMM_MSG_SIZE        SYS_MSG_SIZE

/// TX Message Queue

#define SYS_TX_MSG_SIZE         (sizeof(void *)/4)  // Message pointer size

/// RX Message Queue

#define SYS_RX_MSG_SIZE         (sizeof(void *)/4)  // Message pointer size

/// PTT  Message Queue
#define SYS_NIM_PTT_MSG_SIZE    SYS_MSG_SIZE  // Message pointer size



/* *************************************** *
 *                                         *
 *        Block pool configuration         *
 *                                         *
 * *************************************** */

// The following values specify the number of blocks to be created
// for each block pool size.

#define SIR_BUF_BLK_32_NUM           64
#define SIR_BUF_BLK_64_NUM           128
#define SIR_BUF_BLK_96_NUM           16
#define SIR_BUF_BLK_128_NUM          128
#define SIR_BUF_BLK_160_NUM          8
#define SIR_BUF_BLK_192_NUM          0
#define SIR_BUF_BLK_224_NUM          0
#define SIR_BUF_BLK_256_NUM          128
#define SIR_BUF_BLK_512_NUM          0
#define SIR_BUF_BLK_768_NUM          0
#define SIR_BUF_BLK_1024_NUM         2
#define SIR_BUF_BLK_1280_NUM         0
#define SIR_BUF_BLK_1536_NUM         2
#define SIR_BUF_BLK_1792_NUM         0
#define SIR_BUF_BLK_2048_NUM         2
#define SIR_BUF_BLK_2304_NUM         0

/* ******************************************* *
 *                                             *
 *         SIRIUS MESSAGE TYPES                *
 *                                             *
 * ******************************************* */


/*
 * The following message types have bounds defined for each module for
 * inter thread/module communications.
 * Each module will get 256 message types in total.
 * Note that message type definitions for mailbox messages for
 * communication with Host are in wniApi.h file.
 *
 * Any addition/deletion to this message list should also be
 * reflected in the halUtil_getMsgString() routine.
 */

// HAL message types
#define SIR_HAL_MSG_TYPES_BEGIN            (SIR_HAL_MODULE_ID << 8)
#define SIR_HAL_ITC_MSG_TYPES_BEGIN        SIR_HAL_MSG_TYPES_BEGIN+0x20
#define SIR_HAL_RADAR_DETECTED_IND         SIR_HAL_ITC_MSG_TYPES_BEGIN
#define SIR_HAL_WDT_KAM_RSP                SIR_HAL_ITC_MSG_TYPES_BEGIN + 1
#define SIR_HAL_TIMER_TEMP_MEAS_REQ        SIR_HAL_ITC_MSG_TYPES_BEGIN + 2
#define SIR_HAL_TIMER_PERIODIC_STATS_COLLECT_REQ   SIR_HAL_ITC_MSG_TYPES_BEGIN + 3
#define SIR_HAL_CAL_REQ_NTF                SIR_HAL_ITC_MSG_TYPES_BEGIN + 4
#define SIR_HAL_MNT_OPEN_TPC_TEMP_MEAS_REQ SIR_HAL_ITC_MSG_TYPES_BEGIN + 5
#define SIR_HAL_CCA_MONITOR_INTERVAL_TO    SIR_HAL_ITC_MSG_TYPES_BEGIN + 6
#define SIR_HAL_CCA_MONITOR_DURATION_TO    SIR_HAL_ITC_MSG_TYPES_BEGIN + 7
#define SIR_HAL_CCA_MONITOR_START          SIR_HAL_ITC_MSG_TYPES_BEGIN + 8
#define SIR_HAL_CCA_MONITOR_STOP           SIR_HAL_ITC_MSG_TYPES_BEGIN + 9
#define SIR_HAL_CCA_CHANGE_MODE            SIR_HAL_ITC_MSG_TYPES_BEGIN + 10
#define SIR_HAL_TIMER_WRAP_AROUND_STATS_COLLECT_REQ   SIR_HAL_ITC_MSG_TYPES_BEGIN + 11

/*
 * New Taurus related messages
 */
#define SIR_HAL_ADD_STA_REQ                SIR_HAL_ITC_MSG_TYPES_BEGIN + 13
#define SIR_HAL_ADD_STA_RSP                SIR_HAL_ITC_MSG_TYPES_BEGIN + 14
#define SIR_HAL_DELETE_STA_REQ             SIR_HAL_ITC_MSG_TYPES_BEGIN + 15
#define SIR_HAL_DELETE_STA_RSP             SIR_HAL_ITC_MSG_TYPES_BEGIN + 16
#define SIR_HAL_ADD_BSS_REQ                SIR_HAL_ITC_MSG_TYPES_BEGIN + 17
#define SIR_HAL_ADD_BSS_RSP                SIR_HAL_ITC_MSG_TYPES_BEGIN + 18
#define SIR_HAL_DELETE_BSS_REQ             SIR_HAL_ITC_MSG_TYPES_BEGIN + 19
#define SIR_HAL_DELETE_BSS_RSP             SIR_HAL_ITC_MSG_TYPES_BEGIN + 20
#define SIR_HAL_INIT_SCAN_REQ              SIR_HAL_ITC_MSG_TYPES_BEGIN + 21
#define SIR_HAL_INIT_SCAN_RSP              SIR_HAL_ITC_MSG_TYPES_BEGIN + 22
#define SIR_HAL_START_SCAN_REQ             SIR_HAL_ITC_MSG_TYPES_BEGIN + 23
#define SIR_HAL_START_SCAN_RSP             SIR_HAL_ITC_MSG_TYPES_BEGIN + 24
#define SIR_HAL_END_SCAN_REQ               SIR_HAL_ITC_MSG_TYPES_BEGIN + 25
#define SIR_HAL_END_SCAN_RSP               SIR_HAL_ITC_MSG_TYPES_BEGIN + 26
#define SIR_HAL_FINISH_SCAN_REQ            SIR_HAL_ITC_MSG_TYPES_BEGIN + 27
#define SIR_HAL_FINISH_SCAN_RSP            SIR_HAL_ITC_MSG_TYPES_BEGIN + 28
#define SIR_HAL_SEND_BEACON_REQ            SIR_HAL_ITC_MSG_TYPES_BEGIN + 29
#define SIR_HAL_SEND_BEACON_RSP            SIR_HAL_ITC_MSG_TYPES_BEGIN + 30

#define SIR_HAL_INIT_CFG_REQ               SIR_HAL_ITC_MSG_TYPES_BEGIN + 31
#define SIR_HAL_INIT_CFG_RSP               SIR_HAL_ITC_MSG_TYPES_BEGIN + 32

#define SIR_HAL_INIT_WM_CFG_REQ            SIR_HAL_ITC_MSG_TYPES_BEGIN + 33
#define SIR_HAL_INIT_WM_CFG_RSP            SIR_HAL_ITC_MSG_TYPES_BEGIN + 34

#define SIR_HAL_SET_BSSKEY_REQ             SIR_HAL_ITC_MSG_TYPES_BEGIN + 35
#define SIR_HAL_SET_BSSKEY_RSP             SIR_HAL_ITC_MSG_TYPES_BEGIN + 36
#define SIR_HAL_SET_STAKEY_REQ             SIR_HAL_ITC_MSG_TYPES_BEGIN + 37
#define SIR_HAL_SET_STAKEY_RSP             SIR_HAL_ITC_MSG_TYPES_BEGIN + 38
#define SIR_HAL_DPU_STATS_REQ              SIR_HAL_ITC_MSG_TYPES_BEGIN + 39
#define SIR_HAL_DPU_STATS_RSP              SIR_HAL_ITC_MSG_TYPES_BEGIN + 40
#define SIR_HAL_GET_DPUINFO_REQ            SIR_HAL_ITC_MSG_TYPES_BEGIN + 41
#define SIR_HAL_GET_DPUINFO_RSP            SIR_HAL_ITC_MSG_TYPES_BEGIN + 42

#define SIR_HAL_UPDATE_EDCA_PROFILE_IND    SIR_HAL_ITC_MSG_TYPES_BEGIN + 43

#define SIR_HAL_UPDATE_STARATEINFO_REQ     SIR_HAL_ITC_MSG_TYPES_BEGIN + 45
#define SIR_HAL_UPDATE_STARATEINFO_RSP     SIR_HAL_ITC_MSG_TYPES_BEGIN + 46

#define SIR_HAL_UPDATE_BEACON_IND          SIR_HAL_ITC_MSG_TYPES_BEGIN + 47
#define SIR_HAL_UPDATE_CF_IND              SIR_HAL_ITC_MSG_TYPES_BEGIN + 48
#define SIR_HAL_CHNL_SWITCH_REQ            SIR_HAL_ITC_MSG_TYPES_BEGIN + 49
#define SIR_HAL_ADD_TS_REQ                 SIR_HAL_ITC_MSG_TYPES_BEGIN + 50
#define SIR_HAL_DEL_TS_REQ                 SIR_HAL_ITC_MSG_TYPES_BEGIN + 51
#define SIR_HAL_SOFTMAC_TXSTAT_REPORT      SIR_HAL_ITC_MSG_TYPES_BEGIN + 52

#define SIR_HAL_MBOX_SENDMSG_COMPLETE_IND  SIR_HAL_ITC_MSG_TYPES_BEGIN + 61
#define SIR_HAL_EXIT_BMPS_REQ              SIR_HAL_ITC_MSG_TYPES_BEGIN + 53
#define SIR_HAL_EXIT_BMPS_RSP              SIR_HAL_ITC_MSG_TYPES_BEGIN + 54
#define SIR_HAL_EXIT_BMPS_IND              SIR_HAL_ITC_MSG_TYPES_BEGIN + 55
#define SIR_HAL_ENTER_BMPS_REQ             SIR_HAL_ITC_MSG_TYPES_BEGIN + 56
#define SIR_HAL_ENTER_BMPS_RSP             SIR_HAL_ITC_MSG_TYPES_BEGIN + 57
#define SIR_HAL_BMPS_STATUS_IND            SIR_HAL_ITC_MSG_TYPES_BEGIN + 58
#define SIR_HAL_MISSED_BEACON_IND          SIR_HAL_ITC_MSG_TYPES_BEGIN + 59

#define SIR_HAL_SWITCH_CHANNEL_RSP         SIR_HAL_ITC_MSG_TYPES_BEGIN + 60
#define SIR_HAL_PWR_SAVE_CFG               SIR_HAL_ITC_MSG_TYPES_BEGIN + 62

#define SIR_HAL_REGISTER_PE_CALLBACK       SIR_HAL_ITC_MSG_TYPES_BEGIN + 63
#define SIR_HAL_SOFTMAC_MEM_READREQUEST    SIR_HAL_ITC_MSG_TYPES_BEGIN + 64
#define SIR_HAL_SOFTMAC_MEM_WRITEREQUEST   SIR_HAL_ITC_MSG_TYPES_BEGIN + 65

#define SIR_HAL_SOFTMAC_MEM_READRESPONSE   SIR_HAL_ITC_MSG_TYPES_BEGIN + 66
#define SIR_HAL_SOFTMAC_BULKREGWRITE_CONFIRM      SIR_HAL_ITC_MSG_TYPES_BEGIN + 67
#define SIR_HAL_SOFTMAC_BULKREGREAD_RESPONSE      SIR_HAL_ITC_MSG_TYPES_BEGIN + 68
#define SIR_HAL_SOFTMAC_HOSTMESG_MSGPROCESSRESULT SIR_HAL_ITC_MSG_TYPES_BEGIN + 69

#define SIR_HAL_ADDBA_REQ                  SIR_HAL_ITC_MSG_TYPES_BEGIN + 70
#define SIR_HAL_ADDBA_RSP                  SIR_HAL_ITC_MSG_TYPES_BEGIN + 71
#define SIR_HAL_DELBA_IND                  SIR_HAL_ITC_MSG_TYPES_BEGIN + 72
#define SIR_HAL_DEL_BA_IND                 SIR_HAL_ITC_MSG_TYPES_BEGIN + 73

//message from sme to initiate delete block ack session.
#define SIR_HAL_DELBA_REQ                  SIR_HAL_ITC_MSG_TYPES_BEGIN + 74
#define SIR_HAL_IBSS_STA_ADD               SIR_HAL_ITC_MSG_TYPES_BEGIN + 75
#define SIR_HAL_TIMER_ADJUST_ADAPTIVE_THRESHOLD_IND   SIR_HAL_ITC_MSG_TYPES_BEGIN + 76
#define SIR_HAL_SET_LINK_STATE             SIR_HAL_ITC_MSG_TYPES_BEGIN + 77
#define SIR_HAL_ENTER_IMPS_REQ             SIR_HAL_ITC_MSG_TYPES_BEGIN + 78
#define SIR_HAL_ENTER_IMPS_RSP             SIR_HAL_ITC_MSG_TYPES_BEGIN + 79
#define SIR_HAL_EXIT_IMPS_RSP              SIR_HAL_ITC_MSG_TYPES_BEGIN + 80
#define SIR_HAL_EXIT_IMPS_REQ              SIR_HAL_ITC_MSG_TYPES_BEGIN + 81
#define SIR_HAL_SOFTMAC_HOSTMESG_PS_STATUS_IND  SIR_HAL_ITC_MSG_TYPES_BEGIN + 82
#define SIR_HAL_POSTPONE_ENTER_IMPS_RSP    SIR_HAL_ITC_MSG_TYPES_BEGIN + 83
#define SIR_HAL_STA_STAT_REQ               SIR_HAL_ITC_MSG_TYPES_BEGIN + 84
#define SIR_HAL_GLOBAL_STAT_REQ            SIR_HAL_ITC_MSG_TYPES_BEGIN + 85
#define SIR_HAL_AGGR_STAT_REQ              SIR_HAL_ITC_MSG_TYPES_BEGIN + 86
#define SIR_HAL_STA_STAT_RSP               SIR_HAL_ITC_MSG_TYPES_BEGIN + 87
#define SIR_HAL_GLOBAL_STAT_RSP            SIR_HAL_ITC_MSG_TYPES_BEGIN + 88
#define SIR_HAL_AGGR_STAT_RSP              SIR_HAL_ITC_MSG_TYPES_BEGIN + 89
#define SIR_HAL_STAT_SUMM_REQ              SIR_HAL_ITC_MSG_TYPES_BEGIN + 90
#define SIR_HAL_STAT_SUMM_RSP              SIR_HAL_ITC_MSG_TYPES_BEGIN + 92
#define SIR_HAL_REMOVE_BSSKEY_REQ          SIR_HAL_ITC_MSG_TYPES_BEGIN + 93
#define SIR_HAL_REMOVE_BSSKEY_RSP          SIR_HAL_ITC_MSG_TYPES_BEGIN + 94
#define SIR_HAL_REMOVE_STAKEY_REQ          SIR_HAL_ITC_MSG_TYPES_BEGIN + 95
#define SIR_HAL_REMOVE_STAKEY_RSP          SIR_HAL_ITC_MSG_TYPES_BEGIN + 96
#define SIR_HAL_SET_STA_BCASTKEY_REQ       SIR_HAL_ITC_MSG_TYPES_BEGIN + 97
#define SIR_HAL_SET_STA_BCASTKEY_RSP       SIR_HAL_ITC_MSG_TYPES_BEGIN + 98
#define SIR_HAL_REMOVE_STA_BCASTKEY_REQ    SIR_HAL_ITC_MSG_TYPES_BEGIN + 99
#define SIR_HAL_REMOVE_STA_BCASTKEY_RSP    SIR_HAL_ITC_MSG_TYPES_BEGIN + 100
#define SIR_HAL_ADD_TS_RSP                 SIR_HAL_ITC_MSG_TYPES_BEGIN + 101
#define SIR_HAL_DPU_MIC_ERROR              SIR_HAL_ITC_MSG_TYPES_BEGIN + 102
#define SIR_HAL_TIMER_BA_ACTIVITY_REQ      SIR_HAL_ITC_MSG_TYPES_BEGIN + 103
#define SIR_HAL_TIMER_CHIP_MONITOR_TIMEOUT SIR_HAL_ITC_MSG_TYPES_BEGIN + 104
#define SIR_HAL_TIMER_TRAFFIC_ACTIVITY_REQ SIR_HAL_ITC_MSG_TYPES_BEGIN + 105
#define SIR_HAL_TIMER_ADC_RSSI_STATS       SIR_HAL_ITC_MSG_TYPES_BEGIN + 106
#define SIR_HAL_MIC_FAILURE_IND            SIR_HAL_ITC_MSG_TYPES_BEGIN + 107

#ifdef WLAN_SOFTAP_FEATURE
//107 ... Slots is Free for use.
#define SIR_HAL_UPDATE_UAPSD_IND           SIR_HAL_ITC_MSG_TYPES_BEGIN + 108
#endif

#define SIR_HAL_SET_MIMOPS_REQ                      SIR_HAL_ITC_MSG_TYPES_BEGIN + 109
#define SIR_HAL_SET_MIMOPS_RSP                      SIR_HAL_ITC_MSG_TYPES_BEGIN + 110
#define SIR_HAL_SYS_READY_IND                       SIR_HAL_ITC_MSG_TYPES_BEGIN + 111
#define SIR_HAL_SET_TX_POWER_REQ                    SIR_HAL_ITC_MSG_TYPES_BEGIN + 112
#define SIR_HAL_SET_TX_POWER_RSP                    SIR_HAL_ITC_MSG_TYPES_BEGIN + 113
#define SIR_HAL_GET_TX_POWER_REQ                    SIR_HAL_ITC_MSG_TYPES_BEGIN + 114
#define SIR_HAL_GET_TX_POWER_RSP                    SIR_HAL_ITC_MSG_TYPES_BEGIN + 115
#define SIR_HAL_GET_NOISE_REQ                       SIR_HAL_ITC_MSG_TYPES_BEGIN + 116
#define SIR_HAL_GET_NOISE_RSP                       SIR_HAL_ITC_MSG_TYPES_BEGIN + 117

/* Messages to support transmit_halt and transmit_resume */
#define SIR_HAL_TRANSMISSION_CONTROL_IND            SIR_HAL_ITC_MSG_TYPES_BEGIN + 118
/* Indication from LIM to HAL to Initialize radar interrupt */
#define SIR_HAL_INIT_RADAR_IND                      SIR_HAL_ITC_MSG_TYPES_BEGIN + 119
/* Messages to support transmit_halt and transmit_resume */


#define SIR_HAL_BEACON_PRE_IND             SIR_HAL_ITC_MSG_TYPES_BEGIN + 120
#define SIR_HAL_ENTER_UAPSD_REQ            SIR_HAL_ITC_MSG_TYPES_BEGIN + 121
#define SIR_HAL_ENTER_UAPSD_RSP            SIR_HAL_ITC_MSG_TYPES_BEGIN + 122
#define SIR_HAL_EXIT_UAPSD_REQ             SIR_HAL_ITC_MSG_TYPES_BEGIN + 123
#define SIR_HAL_EXIT_UAPSD_RSP             SIR_HAL_ITC_MSG_TYPES_BEGIN + 124
#define SIR_HAL_LOW_RSSI_IND               SIR_HAL_ITC_MSG_TYPES_BEGIN + 125
#define SIR_HAL_BEACON_FILTER_IND          SIR_HAL_ITC_MSG_TYPES_BEGIN + 126
/// PE <-> HAL WOWL messages
#define SIR_HAL_WOWL_ADD_BCAST_PTRN        SIR_HAL_ITC_MSG_TYPES_BEGIN + 127
#define SIR_HAL_WOWL_DEL_BCAST_PTRN        SIR_HAL_ITC_MSG_TYPES_BEGIN + 128
#define SIR_HAL_WOWL_ENTER_REQ             SIR_HAL_ITC_MSG_TYPES_BEGIN + 129
#define SIR_HAL_WOWL_ENTER_RSP             SIR_HAL_ITC_MSG_TYPES_BEGIN + 130
#define SIR_HAL_WOWL_EXIT_REQ              SIR_HAL_ITC_MSG_TYPES_BEGIN + 131
#define SIR_HAL_WOWL_EXIT_RSP              SIR_HAL_ITC_MSG_TYPES_BEGIN + 132
#define SIR_HAL_TX_COMPLETE_IND            SIR_HAL_ITC_MSG_TYPES_BEGIN + 133
#define SIR_HAL_TIMER_RA_COLLECT_AND_ADAPT SIR_HAL_ITC_MSG_TYPES_BEGIN + 134
/// PE <-> HAL statistics messages
#define SIR_HAL_GET_STATISTICS_REQ         SIR_HAL_ITC_MSG_TYPES_BEGIN + 135
#define SIR_HAL_GET_STATISTICS_RSP         SIR_HAL_ITC_MSG_TYPES_BEGIN + 136
#define SIR_HAL_SET_KEY_DONE               SIR_HAL_ITC_MSG_TYPES_BEGIN + 137

/// PE <-> HAL BTC messages
#define SIR_HAL_BTC_SET_CFG                SIR_HAL_ITC_MSG_TYPES_BEGIN + 138
#define SIR_HAL_SIGNAL_BT_EVENT            SIR_HAL_ITC_MSG_TYPES_BEGIN + 139
#define SIR_HAL_HANDLE_FW_MBOX_RSP            SIR_HAL_ITC_MSG_TYPES_BEGIN + 140
#define SIR_HAL_UPDATE_PROBE_RSP_TEMPLATE_IND     SIR_HAL_ITC_MSG_TYPES_BEGIN + 141

#ifdef ANI_CHIPSET_VOLANS
/* PE <-> HAL addr2 mismatch message */
#define SIR_LIM_ADDR2_MISS_IND             SIR_HAL_ITC_MSG_TYPES_BEGIN + 142
#ifdef FEATURE_INNAV_SUPPORT
/* PE <-> HAL INNAV RELATED MESSAGES */
#define SIR_HAL_START_INNAV_MEAS_REQ       SIR_HAL_ITC_MSG_TYPES_BEGIN + 143
#define SIR_HAL_START_INNAV_MEAS_RSP       SIR_HAL_ITC_MSG_TYPES_BEGIN + 144
#define SIR_HAL_FINISH_INNAV_MEAS_REQ      SIR_HAL_ITC_MSG_TYPES_BEGIN + 145
#endif
#endif

#define SIR_HAL_SET_MAX_TX_POWER_REQ       SIR_HAL_ITC_MSG_TYPES_BEGIN + 146
#define SIR_HAL_SET_MAX_TX_POWER_RSP       SIR_HAL_ITC_MSG_TYPES_BEGIN + 147

#define SIR_HAL_SEND_MSG_COMPLETE          SIR_HAL_ITC_MSG_TYPES_BEGIN + 148

/// PE <-> HAL Host Offload message
#define SIR_HAL_SET_HOST_OFFLOAD           SIR_HAL_ITC_MSG_TYPES_BEGIN + 149

#define SIR_HAL_ADD_STA_SELF_REQ           SIR_HAL_ITC_MSG_TYPES_BEGIN + 150
#define SIR_HAL_ADD_STA_SELF_RSP           SIR_HAL_ITC_MSG_TYPES_BEGIN + 151
#define SIR_HAL_DEL_STA_SELF_REQ           SIR_HAL_ITC_MSG_TYPES_BEGIN + 152
#define SIR_HAL_DEL_STA_SELF_RSP           SIR_HAL_ITC_MSG_TYPES_BEGIN + 153
#define SIR_HAL_SIGNAL_BTAMP_EVENT         SIR_HAL_ITC_MSG_TYPES_BEGIN + 154

#define SIR_HAL_CFG_RXP_FILTER_REQ         SIR_HAL_ITC_MSG_TYPES_BEGIN + 155
#define SIR_HAL_CFG_RXP_FILTER_RSP         SIR_HAL_ITC_MSG_TYPES_BEGIN + 156

#ifdef WLAN_FEATURE_VOWIFI_11R
#define SIR_HAL_AGGR_ADD_TS_REQ            SIR_HAL_ITC_MSG_TYPES_BEGIN + 157
#define SIR_HAL_AGGR_ADD_TS_RSP            SIR_HAL_ITC_MSG_TYPES_BEGIN + 158
#define SIR_HAL_AGGR_QOS_REQ               SIR_HAL_ITC_MSG_TYPES_BEGIN + 159
#define SIR_HAL_AGGR_QOS_RSP               SIR_HAL_ITC_MSG_TYPES_BEGIN + 160
#endif /* WLAN_FEATURE_VOWIFI_11R */

#ifdef WLAN_FEATURE_P2P
/* P2P <-> HAL P2P msg */
#define SIR_HAL_SET_P2P_GO_NOA_REQ         SIR_HAL_ITC_MSG_TYPES_BEGIN + 161
#define SIR_HAL_P2P_NOA_ATTR_IND           SIR_HAL_ITC_MSG_TYPES_BEGIN + 162
#endif

#define SIR_HAL_SET_LINK_STATE_RSP             SIR_HAL_ITC_MSG_TYPES_BEGIN + 165


#ifdef FEATURE_WLAN_INTEGRATED_SOC
#define SIR_HAL_WLAN_SUSPEND_IND               SIR_HAL_ITC_MSG_TYPES_BEGIN + 166
#define SIR_HAL_WLAN_RESUME_REQ                SIR_HAL_ITC_MSG_TYPES_BEGIN + 167
#endif

/// PE <-> HAL Keep Alive message
#define SIR_HAL_SET_KEEP_ALIVE             SIR_HAL_ITC_MSG_TYPES_BEGIN + 168

#ifdef WLAN_NS_OFFLOAD
#define SIR_HAL_SET_NS_OFFLOAD             SIR_HAL_ITC_MSG_TYPES_BEGIN + 169
#endif //WLAN_NS_OFFLOAD

#ifdef FEATURE_WLAN_SCAN_PNO
#define SIR_HAL_SET_PNO_REQ                SIR_HAL_ITC_MSG_TYPES_BEGIN + 170
#define SIR_HAL_SET_PNO_CHANGED_IND        SIR_HAL_ITC_MSG_TYPES_BEGIN + 171
#define SIR_HAL_UPDATE_SCAN_PARAMS         SIR_HAL_ITC_MSG_TYPES_BEGIN + 172
#define SIR_HAL_SET_RSSI_FILTER_REQ        SIR_HAL_ITC_MSG_TYPES_BEGIN + 173  
#endif // FEATURE_WLAN_SCAN_PNO


#define SIR_HAL_SET_TX_PER_TRACKING_REQ             SIR_HAL_ITC_MSG_TYPES_BEGIN + 174

#ifdef WLAN_FEATURE_PACKET_FILTERING
#define SIR_HAL_8023_MULTICAST_LIST_REQ                     SIR_HAL_ITC_MSG_TYPES_BEGIN + 175
#define SIR_HAL_RECEIVE_FILTER_SET_FILTER_REQ                 SIR_HAL_ITC_MSG_TYPES_BEGIN + 176
#define SIR_HAL_PACKET_COALESCING_FILTER_MATCH_COUNT_REQ    SIR_HAL_ITC_MSG_TYPES_BEGIN + 177
#define SIR_HAL_PACKET_COALESCING_FILTER_MATCH_COUNT_RSP    SIR_HAL_ITC_MSG_TYPES_BEGIN + 178
#define SIR_HAL_RECEIVE_FILTER_CLEAR_FILTER_REQ             SIR_HAL_ITC_MSG_TYPES_BEGIN + 179                 
#endif // WLAN_FEATURE_PACKET_FILTERING

#define SIR_HAL_SET_POWER_PARAMS_REQ SIR_HAL_ITC_MSG_TYPES_BEGIN + 180

#ifdef WLAN_FEATURE_GTK_OFFLOAD
#define SIR_HAL_GTK_OFFLOAD_REQ            SIR_HAL_ITC_MSG_TYPES_BEGIN + 181
#define SIR_HAL_GTK_OFFLOAD_GETINFO_REQ    SIR_HAL_ITC_MSG_TYPES_BEGIN + 182
#define SIR_HAL_GTK_OFFLOAD_GETINFO_RSP    SIR_HAL_ITC_MSG_TYPES_BEGIN + 183
#endif //WLAN_FEATURE_GTK_OFFLOAD

#ifdef FEATURE_WLAN_CCX
#define SIR_HAL_TSM_STATS_REQ              SIR_HAL_ITC_MSG_TYPES_BEGIN + 184
#define SIR_HAL_TSM_STATS_RSP              SIR_HAL_ITC_MSG_TYPES_BEGIN + 185
#endif


#ifdef WLAN_WAKEUP_EVENTS
#define SIR_HAL_WAKE_REASON_IND            SIR_HAL_ITC_MSG_TYPES_BEGIN + 186
#endif //WLAN_WAKEUP_EVENTS

#define SIR_HAL_SET_TM_LEVEL_REQ           SIR_HAL_ITC_MSG_TYPES_BEGIN + 187

#define SIR_HAL_MSG_TYPES_END              SIR_HAL_ITC_MSG_TYPES_BEGIN + 0xFF
// CFG message types
#define SIR_CFG_MSG_TYPES_BEGIN        (SIR_CFG_MODULE_ID << 8)
#define SIR_CFG_ITC_MSG_TYPES_BEGIN    SIR_CFG_MSG_TYPES_BEGIN+0xB0
#define SIR_CFG_PARAM_UPDATE_IND       SIR_CFG_ITC_MSG_TYPES_BEGIN
#define SIR_CFG_DOWNLOAD_COMPLETE_IND  SIR_CFG_ITC_MSG_TYPES_BEGIN + 1
#define SIR_CFG_MSG_TYPES_END          SIR_CFG_MSG_TYPES_BEGIN+0xFF

// LIM message types
#define SIR_LIM_MSG_TYPES_BEGIN        (SIR_LIM_MODULE_ID << 8)
#define SIR_LIM_ITC_MSG_TYPES_BEGIN    SIR_LIM_MSG_TYPES_BEGIN+0xB0
// Messages to/from HAL
#define SIR_LIM_RESUME_ACTIVITY_NTF        SIR_LIM_ITC_MSG_TYPES_BEGIN
#define SIR_LIM_SUSPEND_ACTIVITY_REQ       SIR_LIM_ITC_MSG_TYPES_BEGIN + 1
#define SIR_HAL_SUSPEND_ACTIVITY_RSP       SIR_LIM_ITC_MSG_TYPES_BEGIN + 2
// Message from ISR upon TFP retry interrupt
#define SIR_LIM_RETRY_INTERRUPT_MSG        SIR_LIM_ITC_MSG_TYPES_BEGIN + 3
// Message from BB Transport
#define SIR_BB_XPORT_MGMT_MSG              SIR_LIM_ITC_MSG_TYPES_BEGIN + 4
// UNUSED                                  SIR_LIM_ITC_MSG_TYPES_BEGIN + 6
// Message from ISR upon SP's Invalid session key interrupt
#define SIR_LIM_INV_KEY_INTERRUPT_MSG      SIR_LIM_ITC_MSG_TYPES_BEGIN + 7
// Message from ISR upon SP's Invalid key ID interrupt
#define SIR_LIM_KEY_ID_INTERRUPT_MSG       SIR_LIM_ITC_MSG_TYPES_BEGIN + 8
// Message from ISR upon SP's Replay threshold reached interrupt
#define SIR_LIM_REPLAY_THRES_INTERRUPT_MSG SIR_LIM_ITC_MSG_TYPES_BEGIN + 9
// Message from HDD after the TD dummy packet is cleaned up
#define SIR_LIM_TD_DUMMY_CALLBACK_MSG      SIR_LIM_ITC_MSG_TYPES_BEGIN + 0xA
// Message from SCH when the STA is ready to be deleted
#define SIR_LIM_SCH_CLEAN_MSG              SIR_LIM_ITC_MSG_TYPES_BEGIN + 0xB
// Message from ISR upon Radar Detection
#define SIR_LIM_RADAR_DETECT_IND           SIR_LIM_ITC_MSG_TYPES_BEGIN + 0xC

/////////////////////////////////////
// message id Available
////////////////////////////////////


// Message from Hal to send out a DEL-TS indication
#define SIR_LIM_DEL_TS_IND                  SIR_LIM_ITC_MSG_TYPES_BEGIN + 0xE
//Message from HAL to send BA global timer timeout
#define SIR_LIM_ADD_BA_IND                  SIR_LIM_ITC_MSG_TYPES_BEGIN + 0xF
//Indication from HAL to delete all the BA sessions when the BA activity check timer is disabled
#define SIR_LIM_DEL_BA_ALL_IND                  SIR_LIM_ITC_MSG_TYPES_BEGIN + 0x10
//Indication from HAL to delete Station context
#define SIR_LIM_DELETE_STA_CONTEXT_IND          SIR_LIM_ITC_MSG_TYPES_BEGIN + 0x11
//Indication from HAL to delete BA
#define SIR_LIM_DEL_BA_IND                      SIR_LIM_ITC_MSG_TYPES_BEGIN + 0x12
#define SIR_LIM_UPDATE_BEACON                   SIR_LIM_ITC_MSG_TYPES_BEGIN + 0x13 


// LIM Timeout messages
#define SIR_LIM_TIMEOUT_MSG_START      (SIR_LIM_MODULE_ID  << 8) + 0xD0
#define SIR_LIM_MIN_CHANNEL_TIMEOUT    SIR_LIM_TIMEOUT_MSG_START
#define SIR_LIM_MAX_CHANNEL_TIMEOUT    SIR_LIM_TIMEOUT_MSG_START + 1
#define SIR_LIM_JOIN_FAIL_TIMEOUT      SIR_LIM_TIMEOUT_MSG_START + 2
#define SIR_LIM_AUTH_FAIL_TIMEOUT      SIR_LIM_TIMEOUT_MSG_START + 3
#define SIR_LIM_AUTH_RSP_TIMEOUT       SIR_LIM_TIMEOUT_MSG_START + 4
#define SIR_LIM_ASSOC_FAIL_TIMEOUT     SIR_LIM_TIMEOUT_MSG_START + 5
#define SIR_LIM_REASSOC_FAIL_TIMEOUT   SIR_LIM_TIMEOUT_MSG_START + 6
#define SIR_LIM_HEART_BEAT_TIMEOUT     SIR_LIM_TIMEOUT_MSG_START + 7
// currently unused                    SIR_LIM_TIMEOUT_MSG_START + 0x8
#if (WNI_POLARIS_FW_PRODUCT == AP)
#define SIR_LIM_PREAUTH_CLNUP_TIMEOUT  SIR_LIM_TIMEOUT_MSG_START + 0x9
#endif
// Link Monitoring Messages
#define SIR_LIM_CHANNEL_SCAN_TIMEOUT     SIR_LIM_TIMEOUT_MSG_START + 0xA
#define SIR_LIM_PROBE_HB_FAILURE_TIMEOUT SIR_LIM_TIMEOUT_MSG_START + 0xB
#define SIR_LIM_ADDTS_RSP_TIMEOUT        SIR_LIM_TIMEOUT_MSG_START + 0xC
#if (WNI_POLARIS_FW_PRODUCT == AP) && (WNI_POLARIS_FW_PACKAGE == ADVANCED)
#define SIR_LIM_MEASUREMENT_IND_TIMEOUT  SIR_LIM_TIMEOUT_MSG_START + 0x10
#define SIR_LIM_LEARN_INTERVAL_TIMEOUT   SIR_LIM_TIMEOUT_MSG_START + 0x11
#define SIR_LIM_LEARN_DURATION_TIMEOUT   SIR_LIM_TIMEOUT_MSG_START + 0x12
#endif
#define SIR_LIM_LINK_TEST_DURATION_TIMEOUT SIR_LIM_TIMEOUT_MSG_START + 0x13
#define SIR_LIM_HASH_MISS_THRES_TIMEOUT  SIR_LIM_TIMEOUT_MSG_START + 0x16
#define SIR_LIM_CNF_WAIT_TIMEOUT         SIR_LIM_TIMEOUT_MSG_START + 0x17
#define SIR_LIM_KEEPALIVE_TIMEOUT        SIR_LIM_TIMEOUT_MSG_START + 0x18
#define SIR_LIM_UPDATE_OLBC_CACHEL_TIMEOUT SIR_LIM_TIMEOUT_MSG_START + 0x19
#define SIR_LIM_CHANNEL_SWITCH_TIMEOUT   SIR_LIM_TIMEOUT_MSG_START + 0x1A
#define SIR_LIM_QUIET_TIMEOUT            SIR_LIM_TIMEOUT_MSG_START + 0x1B
#define SIR_LIM_QUIET_BSS_TIMEOUT        SIR_LIM_TIMEOUT_MSG_START + 0x1C

#ifdef WLAN_SOFTAP_FEATURE
#define SIR_LIM_WPS_OVERLAP_TIMEOUT      SIR_LIM_TIMEOUT_MSG_START + 0x1D
#endif
#ifdef WLAN_FEATURE_VOWIFI_11R
#define SIR_LIM_FT_PREAUTH_RSP_TIMEOUT   SIR_LIM_TIMEOUT_MSG_START + 0x1E
#endif
#ifdef WLAN_FEATURE_P2P
#define SIR_LIM_REMAIN_CHN_TIMEOUT       SIR_LIM_TIMEOUT_MSG_START + 0x1F
#endif

#ifdef WMM_APSD
#define SIR_LIM_WMM_APSD_SP_START_MSG_TYPE SIR_LIM_TIMEOUT_MSG_START + 0x21
#define SIR_LIM_WMM_APSD_SP_END_MSG_TYPE SIR_LIM_TIMEOUT_MSG_START + 0x22
#endif
#define SIR_LIM_BEACON_GEN_IND          SIR_LIM_TIMEOUT_MSG_START + 0x23
#define SIR_LIM_PERIODIC_PROBE_REQ_TIMEOUT    SIR_LIM_TIMEOUT_MSG_START + 0x24
#ifdef FEATURE_WLAN_CCX
#define SIR_LIM_CCX_TSM_TIMEOUT        SIR_LIM_TIMEOUT_MSG_START + 0x25
#endif

#define SIR_LIM_MSG_TYPES_END            SIR_LIM_MSG_TYPES_BEGIN+0xFF

// SCH message types
#define SIR_SCH_MSG_TYPES_BEGIN        (SIR_SCH_MODULE_ID << 8)
#define SIR_SCH_CHANNEL_SWITCH_REQUEST SIR_SCH_MSG_TYPES_BEGIN
#define SIR_SCH_START_SCAN_REQ         SIR_SCH_MSG_TYPES_BEGIN + 1
#define SIR_SCH_START_SCAN_RSP         SIR_SCH_MSG_TYPES_BEGIN + 2
#define SIR_SCH_END_SCAN_NTF           SIR_SCH_MSG_TYPES_BEGIN + 3
#define SIR_SCH_MSG_TYPES_END          SIR_SCH_MSG_TYPES_BEGIN+0xFF

// PMM message types
#define SIR_PMM_MSG_TYPES_BEGIN        (SIR_PMM_MODULE_ID << 8)
#define SIR_PMM_CHANGE_PM_MODE         SIR_PMM_MSG_TYPES_BEGIN
#define SIR_PMM_CHANGE_IMPS_MODE       (SIR_PMM_MSG_TYPES_BEGIN + 1)        //for Idle mode power save
#define SIR_PMM_MSG_TYPES_END          (SIR_PMM_MSG_TYPES_BEGIN+0xFF)

// MNT message types
#define SIR_MNT_MSG_TYPES_BEGIN        (SIR_MNT_MODULE_ID << 8)
#define SIR_MNT_RELEASE_BD             SIR_MNT_MSG_TYPES_BEGIN + 0
#define SIR_MNT_MSG_TYPES_END          SIR_MNT_MSG_TYPES_BEGIN + 0xFF

// DVT message types
#define SIR_DVT_MSG_TYPES_BEGIN        (SIR_DVT_MODULE_ID << 8)
#define SIR_DVT_ITC_MSG_TYPES_BEGIN    SIR_DVT_MSG_TYPES_BEGIN+0x0F
#define SIR_DVT_MSG_TYPES_END          SIR_DVT_ITC_MSG_TYPES_BEGIN+0xFFF


//PTT message types
#define SIR_PTT_MSG_TYPES_BEGIN            0x3000
#define SIR_PTT_MSG_TYPES_END              0x3300


/* ****************************************** *
 *                                            *
 *         EVENT TYPE Defintions              *
 *                                            *
 * ****************************************** */

// MMH Events that are used in other modules to post events to MMH
# define SIR_HAL_MMH_TXMB_READY_EVT     0x00000002
# define SIR_HAL_MMH_RXMB_DONE_EVT      0x00000004
# define SIR_HAL_MMH_MSGQ_NE_EVT        0x00000008

# define SIR_HSTEMUL_TXMB_DONE_EVT         0x00000100
# define SIR_HSTEMUL_RXMB_READY_EVT        0x00000200
# define SIR_HSTEMUL_MSGQ_NE_EVT           0x00000400

# define SIR_TST_XMIT_MSG_QS_EMPTY_EVT     0x00000080

//added for OBSS

//Param Change Bitmap sent to HAL
#define PARAM_BCN_INTERVAL_CHANGED                      (1 << 0)
#define PARAM_SHORT_PREAMBLE_CHANGED                 (1 << 1)
#define PARAM_SHORT_SLOT_TIME_CHANGED                 (1 << 2)
#define PARAM_llACOEXIST_CHANGED                            (1 << 3)
#define PARAM_llBCOEXIST_CHANGED                            (1 << 4)
#define PARAM_llGCOEXIST_CHANGED                            (1 << 5)
#define PARAM_HT20MHZCOEXIST_CHANGED                  (1<<6)
#define PARAM_NON_GF_DEVICES_PRESENT_CHANGED (1<<7)
#define PARAM_RIFS_MODE_CHANGED                            (1<<8)
#define PARAM_LSIG_TXOP_FULL_SUPPORT_CHANGED   (1<<9)
#define PARAM_OBSS_MODE_CHANGED                               (1<<10)
#define PARAM_BEACON_UPDATE_MASK                (PARAM_BCN_INTERVAL_CHANGED|PARAM_SHORT_PREAMBLE_CHANGED|PARAM_SHORT_SLOT_TIME_CHANGED|PARAM_llACOEXIST_CHANGED |PARAM_llBCOEXIST_CHANGED|\
    PARAM_llGCOEXIST_CHANGED|PARAM_HT20MHZCOEXIST_CHANGED|PARAM_NON_GF_DEVICES_PRESENT_CHANGED|PARAM_RIFS_MODE_CHANGED|PARAM_LSIG_TXOP_FULL_SUPPORT_CHANGED| PARAM_OBSS_MODE_CHANGED)



#endif
