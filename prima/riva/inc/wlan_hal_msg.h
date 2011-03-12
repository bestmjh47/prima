/*==========================================================================
 *
 *  @file:     wlan_hal_msg.h
 *
 *  @brief:    Exports and types for messages sent to HAL from WDI
 *
 *  @author:   Kumar Anand
 *
 *             Copyright (C) 2010, Qualcomm, Inc. 
 *             All rights reserved.
 *
 *=========================================================================*/

#ifndef _WLAN_HAL_MSG_H_
#define _WLAN_HAL_MSG_H_

#include "halLegacyPalTypes.h"
#include "halCompiler.h"
#include "wlan_qct_dev_defs.h"

/*---------------------------------------------------------------------------
  Commom Type definitons
 ---------------------------------------------------------------------------*/

//This is to force compiler to use the maximum of an int ( 4 bytes )
#define WLAN_HAL_MAX_ENUM_SIZE    0x7FFFFFFF

//Max no. of transmit categories
#define STACFG_MAX_TC    8

//The maximum value of access category
#define WLAN_HAL_MAX_AC			  4

typedef tANI_U8 tSirMacAddr[6];

#define WALN_HAL_STA_INVALID_IDX 0xFF
#define WLAN_HAL_BSS_INVALID_IDX 0xFF

//Default Beacon template size
#define BEACON_TEMPLATE_SIZE 0x180 

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

/* Message types for messages exchanged between WDI and HAL */
typedef enum 
{
   //Init/De-Init
   WLAN_HAL_START_REQ,
   WLAN_HAL_START_RSP,
   WLAN_HAL_STOP_REQ,
   WLAN_HAL_STOP_RSP,

   //Scan
   WLAN_HAL_INIT_SCAN_REQ,
   WLAN_HAL_INIT_SCAN_RSP,
   WLAN_HAL_START_SCAN_REQ,
   WLAN_HAL_START_SCAN_RSP,
   WLAN_HAL_END_SCAN_REQ,
   WLAN_HAL_END_SCAN_RSP,
   WLAN_HAL_FINISH_SCAN_REQ,
   WLAN_HAL_FINISH_SCAN_RSP,

   // HW STA configuration/deconfiguration
   WLAN_HAL_CONFIG_STA_REQ,
   WLAN_HAL_CONFIG_STA_RSP,
   WLAN_HAL_DELETE_STA_REQ,
   WLAN_HAL_DELETE_STA_RSP,
   WLAN_HAL_CONFIG_BSS_REQ,
   WLAN_HAL_CONFIG_BSS_RSP,
   WLAN_HAL_DELETE_BSS_REQ,
   WLAN_HAL_DELETE_BSS_RSP,

   //Infra STA asscoiation
   WLAN_HAL_JOIN_REQ,
   WLAN_HAL_JOIN_RSP,
   WLAN_HAL_POST_ASSOC_REQ,
   WLAN_HAL_POST_ASSOC_RSP,

   //Security
   WLAN_HAL_SET_BSSKEY_REQ,
   WLAN_HAL_SET_BSSKEY_RSP,
   WLAN_HAL_SET_STAKEY_REQ,
   WLAN_HAL_SET_STAKEY_RSP,
   WLAN_HAL_RMV_BSSKEY_REQ,
   WLAN_HAL_RMV_BSSKEY_RSP,
   WLAN_HAL_RMV_STAKEY_REQ,
   WLAN_HAL_RMV_STAKEY_RSP,

   //Qos Related
   WLAN_HAL_ADD_TS_REQ,
   WLAN_HAL_ADD_TS_RSP,
   WLAN_HAL_DEL_TS_REQ,
   WLAN_HAL_DEL_TS_RSP,
   WLAN_HAL_UPD_EDCA_PARAMS_REQ,
   WLAN_HAL_UPD_EDCA_PARAMS_RSP,
   WLAN_HAL_ADD_BA_REQ,
   WLAN_HAL_ADD_BA_RSP,
   WLAN_HAL_DEL_BA_REQ,
   WLAN_HAL_DEL_BA_RSP,

   WLAN_HAL_CH_SWITCH_REQ, 
   WLAN_HAL_CH_SWITCH_RSP, 
   WLAN_HAL_SET_LINK_ST_REQ,
   WLAN_HAL_SET_LINK_ST_RSP,
   WLAN_HAL_GET_STATS_REQ, 
   WLAN_HAL_GET_STATS_RSP,
   WLAN_HAL_UPDATE_CFG_REQ,
   WLAN_HAL_UPDATE_CFG_RSP,

   WLAN_HAL_LOW_RSSI_IND,
   WLAN_HAL_MISSED_BEACON_IND,
   WLAN_HAL_UNKNOWN_ADDR2_FRAME_RX_IND,
   WLAN_HAL_MIC_FAILURE_IND,
   WLAN_HAL_FATAL_ERROR_IND,
   WLAN_HAL_SET_KEYDONE_MSG,
   //NV Interface
   WLAN_HAL_DOWNLOAD_NV_REQ,
   WLAN_HAL_DOWNLOAD_NV_RSP,

   WLAN_HAL_ADD_BA_SESSION_REQ,
   WLAN_HAL_ADD_BA_SESSION_RSP,
   WLAN_HAL_TRIGGER_BA_REQ,
   WLAN_HAL_TRIGGER_BA_RSP,
   WLAN_HAL_UPDATE_BEACON_REQ,
   WLAN_HAL_UPDATE_BEACON_RSP,
   WLAN_HAL_SEND_BEACON_REQ,
   WLAN_HAL_SEND_BEACON_RSP,

#ifdef FEATURE_5GHZ_BAND
   WLAN_HAL_ENABLE_RADAR_DETECT_REQ ,
   WLAN_HAL_ENABLE_RADAR_DETECT_RSP ,
   WLAN_HAL_GET_TPC_REPORT_REQ ,
   WLAN_HAL_GET_TPC_REPORT_RSP ,
   WLAN_HAL_RADAR_DETECT_IND   ,
   WLAN_HAL_RADAR_DETECT_INTR_IND,
#endif
   

   WLAN_HAL_MSG_MAX = WLAN_HAL_MAX_ENUM_SIZE
}tHalHostMsgType;

typedef enum
{
   eDRIVER_TYPE_PRODUCTION  = 0,
   eDRIVER_TYPE_MFG         = 1,
   eDRIVER_TYPE_DVT         = 2,
   eDRIVER_TYPE_MAX         = WLAN_HAL_MAX_ENUM_SIZE
} tDriverType;

typedef enum
{
   HAL_STOP_TYPE_SYS_RESET,
   HAL_STOP_TYPE_SYS_DEEP_SLEEP,
   HAL_STOP_TYPE_RF_KILL,
   HAL_STOP_TYPE_MAX = WLAN_HAL_MAX_ENUM_SIZE
}tHalStopType;

typedef enum
{
   eHAL_SYS_MODE_NORMAL,
   eHAL_SYS_MODE_LEARN,
   eHAL_SYS_MODE_SCAN,
   eHAL_SYS_MODE_PROMISC,
   eHAL_SYS_MODE_MAX = WLAN_HAL_MAX_ENUM_SIZE
} eHalSysMode;

typedef enum
{
    PHY_SINGLE_CHANNEL_CENTERED = 0,     // 20MHz IF bandwidth centered on IF carrier
    PHY_DOUBLE_CHANNEL_LOW_PRIMARY = 1,  // 40MHz IF bandwidth with lower 20MHz supporting the primary channel
    PHY_DOUBLE_CHANNEL_CENTERED = 2,     // 40MHz IF bandwidth centered on IF carrier
    PHY_DOUBLE_CHANNEL_HIGH_PRIMARY = 3, // 40MHz IF bandwidth with higher 20MHz supporting the primary channel
    PHY_CHANNEL_BONDING_STATE_MAX = WLAN_HAL_MAX_ENUM_SIZE
}ePhyChanBondState;

// Spatial Multiplexing(SM) Power Save mode
typedef enum eSirMacHTMIMOPowerSaveState
{
  eSIR_HT_MIMO_PS_STATIC = 0,    // Static SM Power Save mode
  eSIR_HT_MIMO_PS_DYNAMIC = 1,   // Dynamic SM Power Save mode
  eSIR_HT_MIMO_PS_NA = 2,        // reserved
  eSIR_HT_MIMO_PS_NO_LIMIT = 3,  // SM Power Save disabled
  eSIR_HT_MIMO_PS_MAX = WLAN_HAL_MAX_ENUM_SIZE
} tSirMacHTMIMOPowerSaveState;

/* each station added has a rate mode which specifies the sta attributes */
typedef enum eStaRateMode {
    eSTA_TAURUS = 0,
    eSTA_TITAN,
    eSTA_POLARIS,
    eSTA_11b,
    eSTA_11bg,
    eSTA_11a,
    eSTA_11n,
    eSTA_INVALID_RATE_MODE = WLAN_HAL_MAX_ENUM_SIZE
} tStaRateMode, *tpStaRateMode;

#define SIR_NUM_11B_RATES           4  //1,2,5.5,11
#define SIR_NUM_11A_RATES           8  //6,9,12,18,24,36,48,54
#define SIR_NUM_POLARIS_RATES       3  //72,96,108

#define SIR_MAC_MAX_SUPPORTED_MCS_SET    16


typedef enum eSirBssType
{
    eSIR_INFRASTRUCTURE_MODE,
    eSIR_INFRA_AP_MODE,                    //Added for softAP support
    eSIR_IBSS_MODE,
    eSIR_BTAMP_STA_MODE,                   //Added for BT-AMP support
    eSIR_BTAMP_AP_MODE,                    //Added for BT-AMP support
    eSIR_AUTO_MODE,
    eSIR_DONOT_USE_BSS_TYPE = WLAN_HAL_MAX_ENUM_SIZE
} tSirBssType;

typedef enum eSirNwType
{
    eSIR_11A_NW_TYPE,
    eSIR_11B_NW_TYPE,
    eSIR_11G_NW_TYPE,
    eSIR_11N_NW_TYPE,
    eSIR_DONOT_USE_NW_TYPE = WLAN_HAL_MAX_ENUM_SIZE
} tSirNwType;

typedef tANI_U16 tSirMacBeaconInterval;

#define SIR_MAC_RATESET_EID_MAX            12

typedef enum eSirMacHTOperatingMode
{
  eSIR_HT_OP_MODE_PURE,                // No Protection
  eSIR_HT_OP_MODE_OVERLAP_LEGACY,      // Overlap Legacy device present, protection is optional
  eSIR_HT_OP_MODE_NO_LEGACY_20MHZ_HT,  // No legacy device, but 20 MHz HT present
  eSIR_HT_OP_MODE_MIXED,               // Protection is required
  eSIR_HT_OP_MODE_MAX = WLAN_HAL_MAX_ENUM_SIZE
} tSirMacHTOperatingMode;

typedef enum eSirMacHTSecondaryChannelOffset
{
    eHT_SECONDARY_CHANNEL_OFFSET_NONE = 0,
    eHT_SECONDARY_CHANNEL_OFFSET_UP = 1,
    eHT_SECONDARY_CHANNEL_OFFSET_DOWN = 3,
    eHT_SECONDARY_CHANNEL_OFFSET_MAX = WLAN_HAL_MAX_ENUM_SIZE
} tSirMacHTSecondaryChannelOffset;

/// Encryption type enum used with peer
typedef enum eAniEdType
{
    eSIR_ED_NONE,
    eSIR_ED_WEP40,
    eSIR_ED_WEP104,
    eSIR_ED_TKIP,
    eSIR_ED_CCMP,
    eSIR_ED_AES_128_CMAC,
#if defined(FEATURE_WLAN_WAPI)
    eSIR_ED_WPI,
#endif
    eSIR_ED_NOT_IMPLEMENTED = WLAN_HAL_MAX_ENUM_SIZE
} tAniEdType;

#if defined(FEATURE_WLAN_WAPI)
#define WLAN_MAX_KEY_RSC_LEN                16
#define WLAN_WAPI_KEY_RSC_LEN               16
#else
#define WLAN_MAX_KEY_RSC_LEN                8
#endif
#define WLAN_WPA2_KEY_RSC_LEN               6

/// MAX key length when ULA is used
#define SIR_MAC_MAX_KEY_LENGTH              32
#define SIR_MAC_MAX_NUM_OF_DEFAULT_KEYS     4

/// Enum to specify whether key is used
/// for TX only, RX only or both
typedef enum eAniKeyDirection
{
    eSIR_TX_ONLY,
    eSIR_RX_ONLY,
    eSIR_TX_RX,
#ifdef FEATURE_SAP
    eSIR_TX_DEFAULT,
#endif
    eSIR_DONOT_USE_KEY_DIRECTION = WLAN_HAL_MAX_ENUM_SIZE
} tAniKeyDirection;

typedef enum eAniWepType
{
    eSIR_WEP_STATIC,
    eSIR_WEP_DYNAMIC,
    eSIR_WEP_MAX = WLAN_HAL_MAX_ENUM_SIZE
} tAniWepType;

typedef enum eSriLinkState {

    eSIR_LINK_IDLE_STATE        = 0,
    eSIR_LINK_PREASSOC_STATE    = 1,
    eSIR_LINK_POSTASSOC_STATE   = 2,
    eSIR_LINK_AP_STATE          = 3,
    eSIR_LINK_IBSS_STATE        = 4,

    /* BT-AMP Case */
    eSIR_LINK_BTAMP_PREASSOC_STATE  = 5,
    eSIR_LINK_BTAMP_POSTASSOC_STATE  = 6,
    eSIR_LINK_BTAMP_AP_STATE  = 7,
    eSIR_LINK_BTAMP_STA_STATE  = 8,
    
    /* Reserved for HAL Internal Use */
    eSIR_LINK_LEARN_STATE       = 9,
    eSIR_LINK_SCAN_STATE        = 10,
    eSIR_LINK_FINISH_SCAN_STATE = 11,
    eSIR_LINK_INIT_CAL_STATE    = 12,
    eSIR_LINK_FINISH_CAL_STATE  = 13,
    eSIR_LINK_MAX = WLAN_HAL_MAX_ENUM_SIZE
} tSirLinkState;

typedef enum
{
    HAL_SUMMARY_STATS_INFO           = 0x00000001,
    HAL_GLOBAL_CLASS_A_STATS_INFO    = 0x00000002,
    HAL_GLOBAL_CLASS_B_STATS_INFO    = 0x00000004,
    HAL_GLOBAL_CLASS_C_STATS_INFO    = 0x00000008,
    HAL_GLOBAL_CLASS_D_STATS_INFO    = 0x00000010,
    HAL_PER_STA_STATS_INFO           = 0x00000020
}eHalStatsMask;

typedef enum
{
    NV_FIELDS_IMAGE                 = 0,

    NV_TABLE_RATE_POWER_SETTINGS    = 2,
    NV_TABLE_REGULATORY_DOMAINS     = 3,
    NV_TABLE_DEFAULT_COUNTRY        = 4,
    NV_TABLE_TPC_POWER_TABLE        = 5,
    NV_TABLE_TPC_PDADC_OFFSETS      = 6,
    NV_TABLE_RF_CAL_VALUES          = 7,
    NV_TABLE_RSSI_CHANNEL_OFFSETS   = 9,
    NV_TABLE_CAL_MEMORY             = 10,
    NV_TABLE_CAL_STATUS             = 11,

    NUM_NV_TABLE_IDS,
    NV_ALL_TABLES                   = 0xFFF,
    NV_BINARY_IMAGE                 = 0x1000,
    NV_MAX_TABLE                    = WLAN_HAL_MAX_ENUM_SIZE
}eNvTable;

/*---------------------------------------------------------------------------
  Message definitons - All the messages below need to be packed
 ---------------------------------------------------------------------------*/

#if defined(__ANI_COMPILER_PRAGMA_PACK_STACK)
#pragma pack(push, 1)
#elif defined(__ANI_COMPILER_PRAGMA_PACK)
#pragma pack(1)
#else
#endif

/* 4-byte control message header used by HAL*/
typedef PACKED_PRE struct PACKED_POST
{
   tHalHostMsgType  msgType;
   tANI_U32         msgLen;
} tHalMsgHeader, *tpHalMsgHeader;

/* Config format required by HAL for each CFG item*/
typedef PACKED_PRE struct PACKED_POST
{
   /* Cfg Id. The Id required by HAL is exported by HAL
    * in shared header file between UMAC and HAL.*/
   tANI_U16   uCfgId;

   /* Length of the Cfg. This parameter is used to go to next cfg 
    * in the TLV format.*/
   tANI_U16   uCfgLen;

   /* Padding bytes for unaligned address's */
   tANI_U16   uCfgPadBytes;

   /* Reserve bytes for making cfgVal to align address */
   tANI_U16   uCfgReserve;

   /* Following the uCfgLen field there should be a 'uCfgLen' bytes
    * containing the uCfgValue ; tANI_U8 uCfgValue[uCfgLen] */
} tHalCfg, *tpHalCfg;

/*---------------------------------------------------------------------------
  WLAN_HAL_START_REQ
---------------------------------------------------------------------------*/

typedef PACKED_PRE struct PACKED_POST sHalMacStartParameters
{
    /* Drive Type - Production or FTM etc */
    tDriverType  driverType;

    /*Length of the config buffer*/
    tANI_U32  uConfigBufferLen;

    /* Following this there is a TLV formatted buffer of length 
     * "uConfigBufferLen" bytes containing all config values. 
     * The TLV is expected to be formatted like this:
     * 0           15            31           31+CFG_LEN-1        length-1
     * |   CFG_ID   |   CFG_LEN   |   CFG_BODY    |  CFG_ID  |......|
     */
} tHalMacStartParameters, *tpHalMacStartParameters;

typedef PACKED_PRE struct PACKED_POST
{
   /* Note: The length specified in tHalMacStartReqMsg messages should be
    * header.msgLen = sizeof(tHalMacStartReqMsg) + uConfigBufferLen */
   tHalMsgHeader header;
   tHalMacStartParameters startReqParams;
}  tHalMacStartReqMsg, *tpHalMacStartReqMsg;

/*---------------------------------------------------------------------------
  WLAN_HAL_START_RSP
---------------------------------------------------------------------------*/

typedef PACKED_PRE struct PACKED_POST sHalMacStartRspParameters
{
   /*success or failure */
   tANI_U16  status;

   /*Max number of STA supported by the device*/
   tANI_U8     ucMaxStations;	

   /*Max number of BSS supported by the device*/
   tANI_U8     ucMaxBssids;

   /*Self STA Index */
   tANI_U16    selfStaIdx;

   /*Self STA Mac*/
   tSirMacAddr selfStaMac;

   /* Self STA DPU Index */
   tANI_U16 selfStaDpuId;

} tHalMacStartRspParams, *tpHalMacStartRspParams;

typedef PACKED_PRE struct PACKED_POST
{
   tHalMsgHeader header;
   tHalMacStartRspParams startRspParams;
}  tHalMacStartRspMsg, *tpHalMacStartRspMsg;

/*---------------------------------------------------------------------------
  WLAN_HAL_STOP_REQ
---------------------------------------------------------------------------*/

typedef PACKED_PRE struct PACKED_POST
{
  /*The reason for which the device is being stopped*/
  tHalStopType   reason;

}tHalMacStopReqParams, *tpHalMacStopReqParams;

typedef PACKED_PRE struct PACKED_POST
{
   tHalMsgHeader header;
   tHalMacStopReqParams stopReqParams;
}  tHalMacStopReqMsg, *tpHalMacStopReqMsg;

/*---------------------------------------------------------------------------
  WLAN_HAL_STOP_RSP
---------------------------------------------------------------------------*/

typedef PACKED_PRE struct PACKED_POST
{
  /*success or failure */
  tANI_U32   status;

}tHalMacStopRspParams, *tpHalMacStopRspParams;

typedef PACKED_PRE struct PACKED_POST
{
   tHalMsgHeader header;
   tHalMacStopRspParams stopRspParams;
}  tHalMacStopRspMsg, *tpHalMacStopRspMsg;

/*---------------------------------------------------------------------------
  WLAN_HAL_UPDATE_CFG_REQ
---------------------------------------------------------------------------*/

typedef PACKED_PRE struct PACKED_POST
{
    /* Length of the config buffer. Allows UMAC to update multiple CFGs */
    tANI_U32  uConfigBufferLen;

    /* Following this there is a TLV formatted buffer of length 
     * "uConfigBufferLen" bytes containing all config values. 
     * The TLV is expected to be formatted like this:
     * 0           15            31           31+CFG_LEN-1        length-1
     * |   CFG_ID   |   CFG_LEN   |   CFG_BODY    |  CFG_ID  |......|
     */
} tHalUpdateCfgReqParams, *tpHalUpdateCfgReqParams;

typedef PACKED_PRE struct PACKED_POST
{
   /* Note: The length specified in tHalUpdateCfgReqMsg messages should be
    * header.msgLen = sizeof(tHalUpdateCfgReqMsg) + uConfigBufferLen */
   tHalMsgHeader header;
   tHalUpdateCfgReqParams updateCfgReqParams;
}  tHalUpdateCfgReqMsg, *tpHalUpdateCfgReqMsg;

/*---------------------------------------------------------------------------
  WLAN_HAL_UPDATE_CFG_RSP
---------------------------------------------------------------------------*/

typedef PACKED_PRE struct PACKED_POST
{
  /* success or failure */
  tANI_U32   status;

}tHalUpdateCfgRspParams, *tpHalUpdateCfgRspParams;

typedef PACKED_PRE struct PACKED_POST
{
   tHalMsgHeader header;
   tHalUpdateCfgRspParams updateCfgRspParams;
}  tHalUpdateCfgRspMsg, *tpHalUpdateCfgRspMsg;

/*---------------------------------------------------------------------------
  WLAN_HAL_INIT_SCAN_REQ
---------------------------------------------------------------------------*/

/// Frame control field format (2 bytes)
typedef  __ani_attr_pre_packed struct sSirMacFrameCtl
{

#ifndef ANI_LITTLE_BIT_ENDIAN

    tANI_U8 subType :4;
    tANI_U8 type :2;
    tANI_U8 protVer :2;

    tANI_U8 order :1;
    tANI_U8 wep :1;
    tANI_U8 moreData :1;
    tANI_U8 powerMgmt :1;
    tANI_U8 retry :1;
    tANI_U8 moreFrag :1;
    tANI_U8 fromDS :1;
    tANI_U8 toDS :1;

#else

    tANI_U8 protVer :2;
    tANI_U8 type :2;
    tANI_U8 subType :4;

    tANI_U8 toDS :1;
    tANI_U8 fromDS :1;
    tANI_U8 moreFrag :1;
    tANI_U8 retry :1;
    tANI_U8 powerMgmt :1;
    tANI_U8 moreData :1;
    tANI_U8 wep :1;
    tANI_U8 order :1;

#endif

} __ani_attr_packed  tSirMacFrameCtl, *tpSirMacFrameCtl;

/// Sequence control field
typedef __ani_attr_pre_packed struct sSirMacSeqCtl
{

#ifndef ANI_LITTLE_BIT_ENDIAN

    tANI_U8 seqNumLo : 4;
    tANI_U8 fragNum : 4;

    tANI_U8 seqNumHi : 8;

#else

    tANI_U8 fragNum : 4;
    tANI_U8 seqNumLo : 4;
    tANI_U8 seqNumHi : 8;

#endif
} __ani_attr_packed tSirMacSeqCtl, *tpSirMacSeqCtl;

/// Management header format
typedef __ani_attr_pre_packed struct sSirMacMgmtHdr
{
    tSirMacFrameCtl fc;
    tANI_U8           durationLo;
    tANI_U8           durationHi;
    tANI_U8              da[6];
    tANI_U8              sa[6];
    tANI_U8              bssId[6];
    tSirMacSeqCtl   seqControl;
} __ani_attr_packed tSirMacMgmtHdr, *tpSirMacMgmtHdr;

typedef PACKED_PRE struct PACKED_POST {

   /*LEARN - AP Role
    SCAN - STA Role*/
    eHalSysMode scanMode;
    
    /*BSSID of the BSS*/
    tSirMacAddr bssid;

    /*Whether BSS needs to be notified*/
    tANI_U8 notifyBss;

    /*Kind of frame to be used for notifying the BSS (Data Null, QoS Null, or
    CTS to Self). Must always be a valid frame type.*/
    tANI_U8 frameType;

    /*UMAC has the option of passing the MAC frame to be used for notifying
     the BSS. If non-zero, HAL will use the MAC frame buffer pointed to by
     macMgmtHdr. If zero, HAL will generate the appropriate MAC frame based on
     frameType.*/
    tANI_U8 frameLength;
  
    /* Following the framelength there is a MAC frame buffer if frameLength 
       is non-zero. */
    tSirMacMgmtHdr macMgmtHdr;

} tInitScanParams, * tpInitScanParams;

typedef PACKED_PRE struct PACKED_POST
{
   tHalMsgHeader header;
   tInitScanParams initScanParams;
}  tHalInitScanReqMsg, *tpHalInitScanReqMsg;

/*---------------------------------------------------------------------------
  WLAN_HAL_INIT_SCAN_RSP
---------------------------------------------------------------------------*/

typedef PACKED_PRE struct PACKED_POST
{
  /*success or failure */
  tANI_U32   status;

}tHalInitScanRspParams, *tpHalInitScanRspParams;

typedef PACKED_PRE struct PACKED_POST
{
   tHalMsgHeader header;
   tHalInitScanRspParams initScanRspParams;
}  tHalInitScanRspMsg, *tpHalInitScanRspMsg;

/*---------------------------------------------------------------------------
  WLAN_HAL_START_SCAN_REQ
---------------------------------------------------------------------------*/

typedef PACKED_PRE struct PACKED_POST 
{
   /*Indicates the channel to scan*/
   tANI_U8 scanChannel;

 } tStartScanParams, * tpStartScanParams;

typedef PACKED_PRE struct PACKED_POST
{
   tHalMsgHeader header;
   tStartScanParams startScanParams;
}  tHalStartScanReqMsg, *tpHalStartScanReqMsg;

/*---------------------------------------------------------------------------
  WLAN_HAL_START_SCAN_RSP
---------------------------------------------------------------------------*/

typedef PACKED_PRE struct PACKED_POST
{
  /*success or failure */
  tANI_U32   status;

#if defined WLAN_FEATURE_VOWIFI
    tANI_U32 startTSF[2];
    tANI_S8 txMgmtPower;
#endif

}tHalStartScanRspParams, *tpHalStartScanRspParams;

typedef PACKED_PRE struct PACKED_POST
{
   tHalMsgHeader header;
   tHalStartScanRspParams startScanRspParams;
}  tHalStartScanRspMsg, *tpHalStartScanRspMsg;

/*---------------------------------------------------------------------------
  WLAN_HAL_END_SCAN_REQ
---------------------------------------------------------------------------*/

typedef PACKED_PRE struct PACKED_POST
{
   /*Indicates the channel to stop scanning.  Not used really. But retained
    for symmetry with "start Scan" message. It can also help in error
    check if needed.*/
    tANI_U8 scanChannel;

} tEndScanParams, *tpEndScanParams;

typedef PACKED_PRE struct PACKED_POST
{
   tHalMsgHeader header;
   tEndScanParams endScanParams;
}  tHalEndScanReqMsg, *tpHalEndScanReqMsg;

/*---------------------------------------------------------------------------
  WLAN_HAL_END_SCAN_RSP
---------------------------------------------------------------------------*/

typedef PACKED_PRE struct PACKED_POST
{
  /*success or failure */
  tANI_U32   status;

}tHalEndScanRspParams, *tpHalEndScanRspParams;

typedef PACKED_PRE struct PACKED_POST
{
   tHalMsgHeader header;
   tHalEndScanRspParams endScanRspParams;
}  tHalEndScanRspMsg, *tpHalEndScanRspMsg;

/*---------------------------------------------------------------------------
  WLAN_HAL_FINISH_SCAN_REQ
---------------------------------------------------------------------------*/

typedef PACKED_PRE struct PACKED_POST
{
    /* Identifies the operational state of the AP/STA
     * LEARN - AP Role SCAN - STA Role */
    eHalSysMode scanMode;

    /*Operating channel to tune to.*/
    tANI_U8 currentOperChannel;

    /*Channel Bonding state If 20/40 MHz is operational, this will indicate the
      40 MHz extension channel in combination with the control channel*/
    ePhyChanBondState cbState;

    /*BSSID of the BSS*/
    tSirMacAddr bssid;

    /*Whether BSS needs to be notified*/
    tANI_U8 notifyBss;

    /*Kind of frame to be used for notifying the BSS (Data Null, QoS Null, or
     CTS to Self). Must always be a valid frame type.*/
    tANI_U8 frameType;

    /*UMAC has the option of passing the MAC frame to be used for notifying
      the BSS. If non-zero, HAL will use the MAC frame buffer pointed to by
      macMgmtHdr. If zero, HAL will generate the appropriate MAC frame based on
      frameType.*/
    tANI_U8 frameLength;
    
    /*Following the framelength there is a MAC frame buffer if frameLength 
      is non-zero.*/    
    tSirMacMgmtHdr macMgmtHdr;

} tFinishScanParams, *tpFinishScanParams;

typedef PACKED_PRE struct PACKED_POST
{
   tHalMsgHeader header;
   tFinishScanParams finishScanParams;
}  tHalFinishScanReqMsg, *tpHalFinishScanReqMsg;

/*---------------------------------------------------------------------------
  WLAN_HAL_FINISH_SCAN_RSP
---------------------------------------------------------------------------*/

typedef PACKED_PRE struct PACKED_POST
{
  /*success or failure */
  tANI_U32   status;

}tHalFinishScanRspParams, *tpHalFinishScanRspParams;

typedef PACKED_PRE struct PACKED_POST
{
   tHalMsgHeader header;
   tHalFinishScanRspParams finishScanRspParams;
}  tHalFinishScanRspMsg, *tpHalFinishScanRspMsg;

/*---------------------------------------------------------------------------
  WLAN_HAL_CONFIG_STA_REQ
---------------------------------------------------------------------------*/

typedef PACKED_PRE struct PACKED_POST {
    /*
    * For Self STA Entry: this represents Self Mode.
    * For Peer Stations, this represents the mode of the peer.
    * On Station:
    * --this mode is updated when PE adds the Self Entry.
    * -- OR when PE sends 'ADD_BSS' message and station context in BSS is used to indicate the mode of the AP.
    * ON AP:
    * -- this mode is updated when PE sends 'ADD_BSS' and Sta entry for that BSS is used
    *     to indicate the self mode of the AP.
    * -- OR when a station is associated, PE sends 'ADD_STA' message with this mode updated.
    */

    tStaRateMode        opRateMode;
    // 11b, 11a and aniLegacyRates are IE rates which gives rate in unit of 500Kbps
    tANI_U16             llbRates[SIR_NUM_11B_RATES];
    tANI_U16             llaRates[SIR_NUM_11A_RATES];
    tANI_U16             aniLegacyRates[SIR_NUM_POLARIS_RATES];

    //Taurus only supports 26 Titan Rates(no ESF/concat Rates will be supported)
    //First 26 bits are reserved for those Titan rates and
    //the last 4 bits(bit28-31) for Taurus, 2(bit26-27) bits are reserved.
    tANI_U32             aniEnhancedRateBitmap; //Titan and Taurus Rates

    /*
    * 0-76 bits used, remaining reserved
    * bits 0-15 and 32 should be set.
    */
    tANI_U8 supportedMCSSet[SIR_MAC_MAX_SUPPORTED_MCS_SET];

    /*
     * RX Highest Supported Data Rate defines the highest data
     * rate that the STA is able to receive, in unites of 1Mbps.
     * This value is derived from "Supported MCS Set field" inside
     * the HT capability element.
     */
    tANI_U16 rxHighestDataRate;

} tSirSupportedRates, *tpSirSupportedRates;

typedef PACKED_PRE struct PACKED_POST
{
    /*BSSID of STA*/
    tSirMacAddr bssId;

    /*ASSOC ID, as assigned by UMAC*/
    tANI_U16 assocId;

    /* STA entry Type: 0 - Self, 1 - Other/Peer, 2 - BSSID, 3 - BCAST */
    tANI_U8 staType;

    /*Short Preamble Supported.*/
    tANI_U8 shortPreambleSupported;

    /*MAC Address of STA*/
    tSirMacAddr staMac;

    /*Listen interval of the STA*/
    tANI_U16 listenInterval;

    /*Support for 11e/WMM*/
    tANI_U8 wmmEnabled;

    /*11n HT capable STA*/
    tANI_U8 htCapable;

    /*TX Width Set: 0 - 20 MHz only, 1 - 20/40 MHz*/
    tANI_U8 txChannelWidthSet;

    /*RIFS mode 0 - NA, 1 - Allowed */
    tANI_U8 rifsMode;

    /*L-SIG TXOP Protection mechanism 
      0 - No Support, 1 - Supported
      SG - there is global field */
    tANI_U8 lsigTxopProtection;

    /*Max Ampdu Size supported by STA. TPE programming.
      0 : 8k , 1 : 16k, 2 : 32k, 3 : 64k */
    tANI_U8 maxAmpduSize;

    /*Max Ampdu density. Used by RA.  3 : 0~7 : 2^(11nAMPDUdensity -4)*/
    tANI_U8 maxAmpduDensity;

    /*Max AMSDU size 1 : 3839 bytes, 0 : 7935 bytes*/
    tANI_U8 maxAmsduSize;

    /*Short GI support for 40Mhz packets*/
    tANI_U8 fShortGI40Mhz;

    /*Short GI support for 20Mhz packets*/
    tANI_U8 fShortGI20Mhz;

    /*These rates are the intersection of peer and self capabilities.*/
    tSirSupportedRates supportedRates;

    /*Robust Management Frame (RMF) enabled/disabled*/
    tANI_U8 rmfEnabled;

    /*HAL should update the existing STA entry, if this flag is set. UMAC 
      will set this flag in case of RE-ASSOC, where we want to reuse the old
      STA ID. 0 = Add, 1 = Update*/
    tANI_U8 action;

    /*U-APSD Flags: 1b per AC.  Encoded as follows:
       b7 b6 b5 b4 b3 b2 b1 b0 =
       X  X  X  X  BE BK VI VO */
    tANI_U8 uAPSD;

    /*Max SP Length*/
    tANI_U8 maxSPLen;

    /*11n Green Field preamble support
      0 - Not supported, 1 - Supported */
    tANI_U8 greenFieldCapable;

    /*MIMO Power Save mode*/
    tSirMacHTMIMOPowerSaveState mimoPS;

    /*Delayed BA Support*/
    tANI_U8 delayedBASupport;
    
    /*Max AMPDU duration in 32us*/
    tANI_U8 us32MaxAmpduDuration;
    
    /*HT STA should set it to 1 if it is enabled in BSS. HT STA should set
      it to 0 if AP does not support it. This indication is sent to HAL and
      HAL uses this flag to pickup up appropriate 40Mhz rates.*/
    tANI_U8 fDsssCckMode40Mhz;

    /* Valid STA Idx when action=Update. Set to 0xFF when invalid!
       Retained for backward compalibity with existing HAL code*/
    tANI_U16 staIdx;

    /* BSSID of BSS to which station is associated. Set to 0xFF when invalid.
       Retained for backward compalibity with existing HAL code*/
    tANI_U16 bssIdx;

} tConfigStaParams, *tpConfigStaParams;

typedef PACKED_PRE struct PACKED_POST
{
   tHalMsgHeader header;
   tConfigStaParams configStaParams;
}  tConfigStaReqMsg, *tpConfigStaReqMsg;

/*---------------------------------------------------------------------------
  WLAN_HAL_CONFIG_STA_RSP
---------------------------------------------------------------------------*/

typedef PACKED_PRE struct PACKED_POST
{
  /*success or failure */
  tANI_U32   status;

  /* Station index; valid only when 'status' field value SUCCESS */
  tANI_U16 staIdx;

  /* BSSID Index of BSS to which the station is associated */
  tANI_U16 bssIdx;

  /* DPU Index for PTK */
  tANI_U8 dpuIndex;

  /* DPU Index for GTK */  
  tANI_U8 bcastDpuIndex;

  /*DPU Index for IGTK  */
  tANI_U8 bcastMgmtDpuIdx;

  /*PTK DPU signature*/
  tANI_U8 ucUcastSig;

  /*GTK DPU isignature*/
  tANI_U8 ucBcastSig;

  /* IGTK DPU signature*/
  tANI_U8 ucMgmtSig;

}tConfigStaRspParams, *tpConfigStaRspParams;

typedef PACKED_PRE struct PACKED_POST
{
   tHalMsgHeader header;
   tConfigStaRspParams configStaRspParams;
}tConfigStaRspMsg, *tpConfigStaRspMsg;

/*---------------------------------------------------------------------------
  WLAN_HAL_DELETE_STA_REQ
---------------------------------------------------------------------------*/

/* Delete STA Request params */
typedef PACKED_PRE struct PACKED_POST 
{
   /* Index of STA to delete */
   tANI_U16    staIdx;
} tDeleteStaParams, *tpDeleteStaParams;

/* Delete STA Request message*/
typedef PACKED_PRE struct PACKED_POST
{
   tHalMsgHeader header;
   tDeleteStaParams delStaParams;
}  tDeleteStaReqMsg, *tpDeleteStaReqMsg;

/*---------------------------------------------------------------------------
  WLAN_HAL_DELETE_STA_RSP
---------------------------------------------------------------------------*/

/* Delete STA Response Params */
typedef PACKED_PRE struct PACKED_POST 
{
   /*success or failure */
   tANI_U32   status;

   /* Index of STA deleted */
   tANI_U16    staId;
} tDeleteStaRspParams, *tpDeleteStaRspParams;

/* Delete STA Response message*/
typedef PACKED_PRE struct PACKED_POST
{
   tHalMsgHeader header;
   tDeleteStaRspParams delStaRspParams;
}  tDeleteStaRspMsg, *tpDeleteStaRspMsg;

/*---------------------------------------------------------------------------
  WLAN_HAL_CONFIG_BSS_REQ
---------------------------------------------------------------------------*/

//12 Bytes long because this structure can be used to represent rate
//and extended rate set IEs. The parser assume this to be at least 12
typedef __ani_attr_pre_packed struct sSirMacRateSet
{
    tANI_U8  numRates;
    tANI_U8  rate[SIR_MAC_RATESET_EID_MAX];
} __ani_attr_packed tSirMacRateSet;

// access category record
typedef __ani_attr_pre_packed struct sSirMacAciAifsn
{
#ifndef ANI_LITTLE_BIT_ENDIAN
    tANI_U8  rsvd  : 1;
    tANI_U8  aci   : 2;
    tANI_U8  acm   : 1;
    tANI_U8  aifsn : 4;
#else
    tANI_U8  aifsn : 4;
    tANI_U8  acm   : 1;
    tANI_U8  aci   : 2;
    tANI_U8  rsvd  : 1;
#endif
} __ani_attr_packed tSirMacAciAifsn;

// contention window size
typedef __ani_attr_pre_packed struct sSirMacCW
{
#ifndef ANI_LITTLE_BIT_ENDIAN
    tANI_U8  max : 4;
    tANI_U8  min : 4;
#else
    tANI_U8  min : 4;
    tANI_U8  max : 4;
#endif
} __ani_attr_packed tSirMacCW;

typedef __ani_attr_pre_packed struct sSirMacEdcaParamRecord
{
    tSirMacAciAifsn  aci;
    tSirMacCW        cw;
    tANI_U16         txoplimit;
} __ani_attr_packed tSirMacEdcaParamRecord;

typedef __ani_attr_pre_packed struct sSirMacSSid
{
    tANI_U8        length;
    tANI_U8        ssId[32];
} __ani_attr_packed tSirMacSSid;

typedef PACKED_PRE struct PACKED_POST
{
    /* BSSID */
    tSirMacAddr bssId;

#ifdef HAL_SELF_STA_PER_BSS
    /* Self Mac Address */
    tSirMacAddr  selfMacAddr;
#endif

    /* BSS type */
    tSirBssType bssType;

    /*Operational Mode: AP =0, STA = 1*/
    tANI_U8 operMode;

    /*Network Type*/
    tSirNwType nwType;

    /*Used to classify PURE_11G/11G_MIXED to program MTU*/
    tANI_U8 shortSlotTimeSupported;

    /*Co-exist with 11a STA*/
    tANI_U8 llaCoexist;

    /*Co-exist with 11b STA*/
    tANI_U8 llbCoexist;

    /*Co-exist with 11g STA*/
    tANI_U8 llgCoexist;

    /*Coexistence with 11n STA*/
    tANI_U8 ht20Coexist;

    /*Non GF coexist flag*/
    tANI_U8 llnNonGFCoexist;

    /*TXOP protection support*/
    tANI_U8 fLsigTXOPProtectionFullSupport;

    /*RIFS mode*/
    tANI_U8 fRIFSMode;

    /*Beacon Interval in TU*/
    tSirMacBeaconInterval beaconInterval;

    /*DTIM period*/
    tANI_U8 dtimPeriod;

     /*TX Width Set: 0 - 20 MHz only, 1 - 20/40 MHz*/	
    tANI_U8 txChannelWidthSet;

    /*Operating channel*/	
    tANI_U8 currentOperChannel;

    /*Extension channel for channel bonding*/
    tANI_U8 currentExtChannel;

    /*Context of the station being added in HW
      Add a STA entry for "itself" -
      On AP  - Add the AP itself in an "STA context"
      On STA - Add the AP to which this STA is joining in an "STA context" */
    tConfigStaParams staContext;

    /*SSID of the BSS*/
    tSirMacSSid ssId;

    /*HAL should update the existing BSS entry, if this flag is set.
      UMAC will set this flag in case of reassoc, where we want to resue the
      the old BSSID and still return success 0 = Add, 1 = Update*/
    tANI_U8 action;

    /* MAC Rate Set */
    tSirMacRateSet rateSet;

    /*Enable/Disable HT capabilities of the BSS*/
    tANI_U8 htCapable;

    /*RMF enabled/disabled*/
    tANI_U8 rmfEnabled;

    /*HT Operating Mode operating mode of the 802.11n STA*/
    tSirMacHTOperatingMode htOperMode;

    /*Dual CTS Protection: 0 - Unused, 1 - Used*/
    tANI_U8 dualCTSProtection;

    /* Probe Response Max retries */
    tANI_U8   ucMaxProbeRespRetryLimit;

    /* To Enable Hidden ssid */
    tANI_U8   bHiddenSSIDEn;

    /* To Enable Disable FW Proxy Probe Resp */
    tANI_U8   bProxyProbeRespEn;

    /* Boolean to indicate if EDCA params are valid. UMAC might not have valid 
       EDCA params or might not desire to apply EDCA params during config BSS. 
       0 implies Not Valid ; Non-Zero implies valid*/
    tANI_U8   edcaParamsValid;

    /*EDCA Parameters for Best Effort Access Category*/
    tSirMacEdcaParamRecord acbe;
    
    /*EDCA Parameters forBackground Access Category*/
    tSirMacEdcaParamRecord acbk;

    /*EDCA Parameters for Video Access Category*/
    tSirMacEdcaParamRecord acvi;

    /*EDCA Parameters for Voice Access Category*/
    tSirMacEdcaParamRecord acvo;
    
     
   
} tConfigBssParams, * tpConfigBssParams;

typedef PACKED_PRE struct PACKED_POST
{
   tHalMsgHeader header;
   tConfigBssParams configBssParams;
}  tConfigBssReqMsg, *tpConfigBssReqMsg;

/*---------------------------------------------------------------------------
  WLAN_HAL_CONFIG_BSS_RSP
---------------------------------------------------------------------------*/

typedef PACKED_PRE struct PACKED_POST
{
    /* Success or Failure */
    tANI_U32   status;

    /* BSS index allocated by HAL */
    tANI_U16 bssIdx;

    /* DPU descriptor index for PTK */
    tANI_U8    dpuDescIndx;

    /* PTK DPU signature */
    tANI_U8    ucastDpuSignature;

    /* DPU descriptor index for GTK*/
    tANI_U8    bcastDpuDescIndx;

    /* GTK DPU signature */
    tANI_U8    bcastDpuSignature;

    /*DPU descriptor for IGTK*/
    tANI_U8    mgmtDpuDescIndx;

    /* IGTK DPU signature */
    tANI_U8    mgmtDpuSignature;

    /* Station Index for BSS entry*/
    tANI_U8     bssStaIdx;

    /* Self station index for this BSS */
    tANI_U8     bssSelfStaIdx;

    /* Bcast station for buffering bcast frames in AP role */
    tANI_U8     bssBcastStaIdx;

    /*MAC Address of STA(PEER/SELF) in staContext of configBSSReq*/
    tSirMacAddr staMac;

} tConfigBssRspParams, * tpConfigBssRspParams;

typedef PACKED_PRE struct PACKED_POST
{
   tHalMsgHeader header;
   tConfigBssRspParams configBssRspParams;
}  tConfigBssRspMsg, *tpConfigBssRspMsg;

/*---------------------------------------------------------------------------
  WLAN_HAL_DELETE_BSS_REQ
---------------------------------------------------------------------------*/

typedef PACKED_PRE struct PACKED_POST
{
    /* BSS index to be deleted */
    tANI_U16 bssIdx;

} tDeleteBssParams, *tpDeleteBssParams;

typedef PACKED_PRE struct PACKED_POST
{
   tHalMsgHeader header;
   tDeleteBssParams deleteBssParams;
}  tDeleteBssReqMsg, *tpDeleteBssReqMsg;

/*---------------------------------------------------------------------------
  WLAN_HAL_DELETE_BSS_RSP
---------------------------------------------------------------------------*/

typedef PACKED_PRE struct PACKED_POST
{
    /* Success or Failure */
    tANI_U32   status;

    /* BSS index that has been deleted */
    tANI_U16 bssIdx;

} tDeleteBssRspParams, *tpDeleteBssRspParams;

typedef PACKED_PRE struct PACKED_POST
{
   tHalMsgHeader header;
   tDeleteBssRspParams deleteBssRspParams;
}  tDeleteBssRspMsg, *tpDeleteBssRspMsg;

/*---------------------------------------------------------------------------
  WLAN_HAL_JOIN_REQ
---------------------------------------------------------------------------*/

typedef PACKED_PRE struct PACKED_POST
{
  /*Indicates the BSSID to which STA is going to associate*/
  tSirMacAddr     bssId; 

  /*Indicates the channel to switch to.*/
  tANI_U8         ucChannel;
#ifndef WLAN_FEATURE_VOWIFI   
  /*Local power constraint*/
  tANI_U8         ucLocalPowerConstraint;
#endif

  /*Secondary channel offset */
  tSirMacHTSecondaryChannelOffset  secondaryChannelOffset;

#ifdef WLAN_FEATURE_VOWIFI
    /* Max TX power */
    tANI_S8 maxTxPower;
    
    /* Self STA MAC */
    tSirMacAddr selfStaMacAddr;
#endif
} tHalJoinReqParams, *tpHalJoinReqParams;

typedef PACKED_PRE struct PACKED_POST
{
   tHalMsgHeader header;
   tHalJoinReqParams joinReqParams;
}  tHalJoinReqMsg, *tpHalJoinReqMsg;

/*---------------------------------------------------------------------------
  WLAN_HAL_JOIN_RSP
---------------------------------------------------------------------------*/

typedef PACKED_PRE struct PACKED_POST
{
  /*success or failure */
  tANI_U32   status;

#ifdef WLAN_FEATURE_VOWIFI
  /* HAL fills in the tx power used for mgmt frames in this field */
  tANI_S8 txMgmtPower;
#endif

}tHalJoinRspParams, *tpHalJoinRspParams;

typedef PACKED_PRE struct PACKED_POST
{
   tHalMsgHeader header;
   tHalJoinRspParams joinRspParams;
}tHalJoinRspMsg, *tpHalJoinRspMsg;

/*---------------------------------------------------------------------------
  WLAN_HAL_POST_ASSOC_REQ
---------------------------------------------------------------------------*/

typedef PACKED_PRE struct PACKED_POST
{
   tConfigStaParams configStaParams;
   tConfigBssParams configBssParams;
} tPostAssocReqParams, *tpPostAssocReqParams;

typedef PACKED_PRE struct PACKED_POST
{
   tHalMsgHeader header;
   tPostAssocReqParams postAssocReqParams;
}  tPostAssocReqMsg, *tpPostAssocReqMsg;

/*---------------------------------------------------------------------------
  WLAN_HAL_POST_ASSOC_RSP
---------------------------------------------------------------------------*/

typedef PACKED_PRE struct PACKED_POST
{
   tConfigStaRspParams configStaRspParams;
   tConfigBssRspParams configBssRspParams;
} tPostAssocRspParams, *tpPostAssocRspParams;

typedef PACKED_PRE struct PACKED_POST
{
   tHalMsgHeader header;
   tPostAssocRspParams postAssocRspParams;
}  tPostAssocRspMsg, *tpPostAssocRspMsg;

/*---------------------------------------------------------------------------
  WLAN_HAL_SET_BSSKEY_REQ
---------------------------------------------------------------------------*/
/// Definition for Encryption Keys
typedef PACKED_PRE struct PACKED_POST
{
    tANI_U8                  keyId;
    tANI_U8                  unicast;  // 0 for multicast
    tAniKeyDirection         keyDirection;
    tANI_U8                  keyRsc[WLAN_MAX_KEY_RSC_LEN];  // Usage is unknown
    tANI_U8                  paeRole;  // =1 for authenticator,=0 for supplicant
    tANI_U16                 keyLength;
    tANI_U8                  key[SIR_MAC_MAX_KEY_LENGTH];
} tSirKeys, *tpSirKeys;
/*
 * This is used by PE to create a set of WEP keys for a given BSS.
 */
typedef PACKED_PRE struct PACKED_POST
{
    /*BSS Index of the BSS*/
    tANI_U8         bssIdx;

    /*Encryption Type used with peer*/
    tAniEdType      encType;

    /*Number of keys*/
    tANI_U8         numKeys;

    /*Array of keys.*/
    tSirKeys        key[SIR_MAC_MAX_NUM_OF_DEFAULT_KEYS];
  
    /*Control for Replay Count, 1= Single TID based replay count on Tx
    0 = Per TID based replay count on TX */
    tANI_U8         singleTidRc;
} tSetBssKeyParams, *tpSetBssKeyParams;

typedef PACKED_PRE struct PACKED_POST
{
   tHalMsgHeader header;
   tSetBssKeyParams setBssKeyParams;
} tSetBssKeyReqMsg, *tpSetBssKeyReqMsg;

/*---------------------------------------------------------------------------
  WLAN_HAL_SET_BSSKEY_RSP
---------------------------------------------------------------------------*/
typedef PACKED_PRE struct PACKED_POST
{
  /*success or failure */
  tANI_U32   status;

} tSetBssKeyRspParams, *tpSetBssKeyRspParams;

typedef PACKED_PRE struct PACKED_POST
{
   tHalMsgHeader header;
   tSetBssKeyRspParams setBssKeyRspParams;
}  tSetBssKeyRspMsg, *tpSetBssKeyRspMsg;

/*---------------------------------------------------------------------------
   WLAN_HAL_SET_STAKEY_REQ,
---------------------------------------------------------------------------*/

/*
 * This is used by PE to configure the key information on a given station.
 * When the secType is WEP40 or WEP104, the defWEPIdx is used to locate
 * a preconfigured key from a BSS the station assoicated with; otherwise
 * a new key descriptor is created based on the key field.
 */
typedef PACKED_PRE struct PACKED_POST
{
    /*STA Index*/
    tANI_U16        staIdx;

    /*Encryption Type used with peer*/
    tAniEdType      encType;

    /*STATIC/DYNAMIC - valid only for WEP*/
    tAniWepType     wepType; 

    /*Default WEP key, valid only for static WEP, must between 0 and 3.*/
    tANI_U8         defWEPIdx;

#ifdef FEATURE_SAP
    /* valid only for non-static WEP encyrptions */
    tSirKeys        key[SIR_MAC_MAX_NUM_OF_DEFAULT_KEYS];            
#else
    tSirKeys        key;
#endif
  
    /*Control for Replay Count, 1= Single TID based replay count on Tx
      0 = Per TID based replay count on TX */
    tANI_U8         singleTidRc;

} tSetStaKeyParams, *tpSetStaKeyParams;

typedef PACKED_PRE struct PACKED_POST
{
   tHalMsgHeader header;
   tSetStaKeyParams setStaKeyParams;
} tSetStaKeyReqMsg, *tpSetStaKeyReqMsg;

/*---------------------------------------------------------------------------
   WLAN_HAL_SET_STAKEY_RSP,
---------------------------------------------------------------------------*/
typedef PACKED_PRE struct PACKED_POST
{
  /*success or failure */
  tANI_U32   status;

} tSetStaKeyRspParams, *tpSetStaKeyRspParams;

typedef PACKED_PRE struct PACKED_POST
{
   tHalMsgHeader header;
   tSetStaKeyRspParams setStaKeyRspParams;
} tSetStaKeyRspMsg, *tpSetStaKeyRspMsg;

/*---------------------------------------------------------------------------
   WLAN_HAL_RMV_BSSKEY_REQ,
---------------------------------------------------------------------------*/
/*
 * This is used by PE to remove keys for a given BSS.
 */
typedef PACKED_PRE struct PACKED_POST

{
    /*BSS Index of the BSS*/
    tANI_U8         bssIdx;
    
    /*Encryption Type used with peer*/
    tAniEdType      encType;

    /*Key Id*/
    tANI_U8         keyId;

    /*STATIC/DYNAMIC. Used in Nullifying in Key Descriptors for Static/Dynamic keys*/
    tAniWepType    wepType;

} tRemoveBssKeyParams, *tpRemoveBssKeyParams;

typedef PACKED_PRE struct PACKED_POST
{
   tHalMsgHeader header;
   tRemoveBssKeyParams removeBssKeyParams;
}  tRemoveBssKeyReqMsg, *tpRemoveBssKeyReqMsg;

/*---------------------------------------------------------------------------
   WLAN_HAL_RMV_BSSKEY_RSP,
---------------------------------------------------------------------------*/
typedef PACKED_PRE struct PACKED_POST
{
  /*success or failure */
  tANI_U32   status;

} tRemoveBssKeyRspParams, *tpRemoveBssKeyRspParams;

typedef PACKED_PRE struct PACKED_POST
{
   tHalMsgHeader header;
   tRemoveBssKeyRspParams removeBssKeyRspParams;
}  tRemoveBssKeyRspMsg, *tpRemoveBssKeyRspMsg;

/*---------------------------------------------------------------------------
   WLAN_HAL_RMV_STAKEY_REQ,
---------------------------------------------------------------------------*/
/*
 * This is used by PE to Remove the key information on a given station.
 */
typedef PACKED_PRE struct PACKED_POST
{
    /*STA Index*/
    tANI_U16         staIdx;

    /*Encryption Type used with peer*/
    tAniEdType      encType;

    /*Key Id*/
    tANI_U8           keyId;

    /*Whether to invalidate the Broadcast key or Unicast key. In case of WEP,
      the same key is used for both broadcast and unicast.*/
    tANI_BOOLEAN    unicast;

} tRemoveStaKeyParams, *tpRemoveStaKeyParams;

typedef PACKED_PRE struct PACKED_POST
{
   tHalMsgHeader header;
   tRemoveStaKeyParams removeStaKeyParams;
}  tRemoveStaKeyReqMsg, *tpRemoveStaKeyReqMsg;

/*---------------------------------------------------------------------------
   WLAN_HAL_RMV_STAKEY_RSP,
---------------------------------------------------------------------------*/
typedef PACKED_PRE struct PACKED_POST
{
  /*success or failure */
  tANI_U32   status;
} tRemoveStaKeyRspParams, *tpRemoveStaKeyRspParams;

typedef PACKED_PRE struct PACKED_POST
{
   tHalMsgHeader header;
   tRemoveStaKeyRspParams removeStaKeyRspParams;
}  tRemoveStaKeyRspMsg, *tpRemoveStaKeyRspMsg;

/*---------------------------------------------------------------------------
WLAN_HAL_CH_SWITCH_REQ
---------------------------------------------------------------------------*/

typedef PACKED_PRE struct PACKED_POST
{
    /* Channel number */
    tANI_U8 channelNumber;

#ifndef WLAN_FEATURE_VOWIFI
    /* Local power constraint */
    tANI_U8 localPowerConstraint;
#endif

    /*Secondary channel offset */
    tSirMacHTSecondaryChannelOffset secondaryChannelOffset;

#ifdef WLAN_FEATURE_VOWIFI
    /* Max TX power */
    tANI_S8 maxTxPower;
    
    /* Self STA MAC */
    tSirMacAddr selfStaMacAddr;

    /*BSSID needed to identify session. As the request has power constraints,
      this should be applied only to that session*/
    tSirMacAddr bssId;
#endif
    
}tSwitchChannelParams, *tpSwitchChannelParams;

typedef PACKED_PRE struct PACKED_POST
{
   tHalMsgHeader header;
   tSwitchChannelParams switchChannelParams;
}  tSwitchChannelReqMsg, *tpSwitchChannelReqMsg;

/*---------------------------------------------------------------------------
WLAN_HAL_CH_SWITCH_RSP
---------------------------------------------------------------------------*/

typedef PACKED_PRE struct PACKED_POST
{
    /* Status */
    tANI_U32 status;

    /* Channel number - same as in request*/
    tANI_U8 channelNumber;

#ifdef WLAN_FEATURE_VOWIFI
    /* HAL fills in the tx power used for mgmt frames in this field */
    tANI_S8 txMgmtPower;

    /* BSSID needed to identify session - same as in request*/
    tSirMacAddr bssId;
#endif
    
}tSwitchChannelRspParams, *tpSwitchChannelRspParams;

typedef PACKED_PRE struct PACKED_POST
{
   tHalMsgHeader header;
   tSwitchChannelRspParams switchChannelRspParams;
}  tSwitchChannelRspMsg, *tpSwitchChannelRspMsg;

/*---------------------------------------------------------------------------
WLAN_HAL_UPD_EDCA_PARAMS_REQ
---------------------------------------------------------------------------*/

typedef PACKED_PRE struct PACKED_POST
{
   /*BSS Index*/
   tANI_U16 bssIdx;

   /* Best Effort */
   tSirMacEdcaParamRecord acbe; 

   /* Background */
   tSirMacEdcaParamRecord acbk;
   
   /* Video */
   tSirMacEdcaParamRecord acvi;

   /* Voice */
   tSirMacEdcaParamRecord acvo;

} tEdcaParams, *tpEdcaParams;

typedef PACKED_PRE struct PACKED_POST
{
   tHalMsgHeader header;
   tEdcaParams edcaParams;
}  tUpdateEdcaParamsReqMsg, *tpUpdateEdcaParamsReqMsg;

/*---------------------------------------------------------------------------
WLAN_HAL_UPD_EDCA_PARAMS_RSP
---------------------------------------------------------------------------*/
typedef PACKED_PRE struct PACKED_POST
{
  /*success or failure */
  tANI_U32   status;
} tEdcaRspParams, *tpEdcaRspParams;

typedef PACKED_PRE struct PACKED_POST
{
   tHalMsgHeader header;
   tEdcaRspParams edcaRspParams;
}  tUpdateEdcaParamsRspMsg, *tpUpdateEdcaParamsRspMsg;



/*---------------------------------------------------------------------------
 * WLAN_HAL_GET_STATS_REQ
 *--------------------------------------------------------------------------*/
typedef PACKED_PRE struct PACKED_POST

{
    /* Index of STA to which the statistics */
    tANI_U16 staIdx;

    /* Encryption mode */
    tANI_U8 encMode;
    
    /* status */
    tANI_U32  status;
    
    /* Statistics */
    tANI_U32  sendBlocks;
    tANI_U32  recvBlocks;
    tANI_U32  replays;
    tANI_U8   micErrorCnt;
    tANI_U32  protExclCnt;
    tANI_U16  formatErrCnt;
    tANI_U16  unDecryptableCnt;
    tANI_U32  decryptErrCnt;
    tANI_U32  decryptOkCnt;
} tDpuStatsParams, * tpDpuStatsParams;

typedef PACKED_PRE struct PACKED_POST
{
   /* Valid STA Idx for per STA stats request */
   tANI_U32    staId;

   /* Categories of stats requested as specified in eHalStatsMask*/
   tANI_U32    statsMask;
}tHalStatsReqParams, *tpHalStatsReqParams;

typedef PACKED_PRE struct PACKED_POST
{
   tHalMsgHeader        header;
   tHalStatsReqParams   statsReqParams;
} tHalStatsReqMsg, *tpHalStatsReqMsg;

/*---------------------------------------------------------------------------
 * WLAN_HAL_GET_STATS_RSP
 *--------------------------------------------------------------------------*/

typedef PACKED_PRE struct PACKED_POST
{
    tANI_U32 retry_cnt[4];          //Total number of packets(per AC) that were successfully transmitted with retries
    tANI_U32 multiple_retry_cnt[4]; //The number of MSDU packets and MMPDU frames per AC that the 802.11
                                    // station successfully transmitted after more than one retransmission attempt

    tANI_U32 tx_frm_cnt[4];         //Total number of packets(per AC) that were successfully transmitted 
                                    //(with and without retries, including multi-cast, broadcast)     
    tANI_U32 rx_frm_cnt;            //Total number of packets that were successfully received 
                                    //(after appropriate filter rules including multi-cast, broadcast)    
    tANI_U32 frm_dup_cnt;           //Total number of duplicate frames received successfully
    tANI_U32 fail_cnt[4];           //Total number packets(per AC) failed to transmit
    tANI_U32 rts_fail_cnt;          //Total number of RTS/CTS sequence failures for transmission of a packet
    tANI_U32 ack_fail_cnt;          //Total number packets failed transmit because of no ACK from the remote entity
    tANI_U32 rts_succ_cnt;          //Total number of RTS/CTS sequence success for transmission of a packet 
    tANI_U32 rx_discard_cnt;        //The sum of the receive error count and dropped-receive-buffer error count. 
                                    //HAL will provide this as a sum of (FCS error) + (Fail get BD/PDU in HW)
    tANI_U32 rx_error_cnt;          //The receive error count. HAL will provide the RxP FCS error global counter.
    tANI_U32 tx_byte_cnt;           //The sum of the transmit-directed byte count, transmit-multicast byte count 
                                    //and transmit-broadcast byte count. HAL will sum TPE UC/MC/BCAST global counters 
                                    //to provide this.
}tAniSummaryStatsInfo, *tpAniSummaryStatsInfo;

typedef PACKED_PRE struct PACKED_POST
{
    tANI_U32 rx_frag_cnt;              //The number of MPDU frames received by the 802.11 station for MSDU packets 
                                       //or MMPDU frames
    tANI_U32 promiscuous_rx_frag_cnt;  //The number of MPDU frames received by the 802.11 station for MSDU packets 
                                       //or MMPDU frames when a promiscuous packet filter was enabled
    tANI_U32 rx_input_sensitivity;     //The receiver input sensitivity referenced to a FER of 8% at an MPDU length 
                                       //of 1024 bytes at the antenna connector. Each element of the array shall correspond 
                                       //to a supported rate and the order shall be the same as the supporteRates parameter.
    tANI_U32 max_pwr;                  //The maximum transmit power in dBm upto one decimal. 
                                       //for eg: if it is 10.5dBm, the value would be 105 
    tANI_U32 sync_fail_cnt;            //Number of times the receiver failed to synchronize with the incoming signal 
                                       //after detecting the sync in the preamble of the transmitted PLCP protocol data unit. 
    tANI_U32 tx_rate;                  //Transmit rate, in units of 500 kbit/sec, for the most recently transmitted frame 

}tAniGlobalClassAStatsInfo, *tpAniGlobalClassAStatsInfo;

typedef PACKED_PRE struct PACKED_POST
{
    tANI_U32 rx_wep_unencrypted_frm_cnt;  //The number of unencrypted received MPDU frames that the MAC layer discarded when 
                                          //the IEEE 802.11 dot11ExcludeUnencrypted management information base (MIB) object 
                                          //is enabled
    tANI_U32 rx_mic_fail_cnt;             //The number of received MSDU packets that that the 802.11 station discarded 
                                          //because of MIC failures
    tANI_U32 tkip_icv_err;                //The number of encrypted MPDU frames that the 802.11 station failed to decrypt 
                                          //because of a TKIP ICV error
    tANI_U32 aes_ccmp_format_err;         //The number of received MPDU frames that the 802.11 discarded because of an 
                                          //invalid AES-CCMP format
    tANI_U32 aes_ccmp_replay_cnt;         //The number of received MPDU frames that the 802.11 station discarded because of 
                                          //the AES-CCMP replay protection procedure
    tANI_U32 aes_ccmp_decrpt_err;         //The number of received MPDU frames that the 802.11 station discarded because of 
                                          //errors detected by the AES-CCMP decryption algorithm
    tANI_U32 wep_undecryptable_cnt;       //The number of encrypted MPDU frames received for which a WEP decryption key was 
                                          //not available on the 802.11 station
    tANI_U32 wep_icv_err;                 //The number of encrypted MPDU frames that the 802.11 station failed to decrypt 
                                          //because of a WEP ICV error
    tANI_U32 rx_decrypt_succ_cnt;         //The number of received encrypted packets that the 802.11 station successfully 
                                          //decrypted
    tANI_U32 rx_decrypt_fail_cnt;         //The number of encrypted packets that the 802.11 station failed to decrypt

}tAniGlobalSecurityStats, *tpAniGlobalSecurityStats;
   
typedef PACKED_PRE struct PACKED_POST
{
    tAniGlobalSecurityStats ucStats;
    tAniGlobalSecurityStats mcbcStats;
}tAniGlobalClassBStatsInfo, *tpAniGlobalClassBStatsInfo;

typedef PACKED_PRE struct PACKED_POST
{
    tANI_U32 rx_amsdu_cnt;             //This counter shall be incremented for a received A-MSDU frame with the stations 
                                       //MAC address in the address 1 field or an A-MSDU frame with a group address in the 
                                       //address 1 field
    tANI_U32 rx_ampdu_cnt;             //This counter shall be incremented when the MAC receives an AMPDU from the PHY
    tANI_U32 tx_20_frm_cnt;            //This counter shall be incremented when a Frame is transmitted only on the 
                                       //primary channel
    tANI_U32 rx_20_frm_cnt;            //This counter shall be incremented when a Frame is received only on the primary channel
    tANI_U32 rx_mpdu_in_ampdu_cnt;     //This counter shall be incremented by the number of MPDUs received in the A-MPDU 
                                       //when an A-MPDU is received
    tANI_U32 ampdu_delimiter_crc_err;  //This counter shall be incremented when an MPDU delimiter has a CRC error when this 
                                       //is the first CRC error in the received AMPDU or when the previous delimiter has been 
                                       //decoded correctly
}tAniGlobalClassCStatsInfo, *tpAniGlobalClassCStatsInfo;

typedef PACKED_PRE struct PACKED_POST
{
    tANI_U32 tx_frag_cnt[4];        //The number of MPDU frames that the 802.11 station transmitted and acknowledged 
                                    //through a received 802.11 ACK frame
    tANI_U32 tx_ampdu_cnt;          //This counter shall be incremented when an A-MPDU is transmitted 
    tANI_U32 tx_mpdu_in_ampdu_cnt;  //This counter shall increment by the number of MPDUs in the AMPDU when an A-MPDU 
                                    //is transmitted
}tAniPerStaStatsInfo, *tpAniPerStaStatsInfo;

typedef PACKED_PRE struct PACKED_POST
{
   /* Success or Failure */
   tANI_U32 status;

   /* STA Idx */
   tANI_U32 staId;

   /* Categories of STATS being returned as per eHalStatsMask*/
   tANI_U32 statsMask;

   /* Summary Stats */
   tAniSummaryStatsInfo summaryStats;

   /* Global Class A Stats */
   tAniGlobalClassAStatsInfo globalClassAStats;

   /* Global Class B Stats */
   tAniGlobalClassBStatsInfo globalClassBStats;

   /* Global Class C Stats */
   tAniGlobalClassCStatsInfo globalClassCStats;

   /* Per STA Stats */
   tAniPerStaStatsInfo perStaStats;

} tHalStatsRspParams, *tpHalStatsRspParams;

typedef PACKED_PRE struct PACKED_POST
{
   tHalMsgHeader  header;
   tHalStatsRspParams statsRspParams;
} tHalStatsRspMsg, *tpHalStatsRspMsg;

/*---------------------------------------------------------------------------
 * WLAN_HAL_SET_LINK_ST_REQ
 *--------------------------------------------------------------------------*/
typedef PACKED_PRE struct PACKED_POST
{
    tSirMacAddr bssid;
    tSirLinkState state;
} tLinkStateParams, *tpLinkStateParams;

typedef PACKED_PRE struct PACKED_POST
{
   tHalMsgHeader header;
   tLinkStateParams linkStateParams;
}  tSetLinkStateReqMsg, *tpSetLinkStateReqMsg;

/*---------------------------------------------------------------------------
 * WLAN_HAL_SET_LINK_ST_RSP
 *--------------------------------------------------------------------------*/

typedef PACKED_PRE struct PACKED_POST
{
  /*success or failure */
  tANI_U32   status;
} tLinkStateRspParams, *tpLinkStateRspParams;

typedef PACKED_PRE struct PACKED_POST
{
   tHalMsgHeader header;
   tLinkStateRspParams linkStateRspParams;
}  tSetLinkStateRspMsg, *tpSetLinkStateRspMsg;

/*---------------------------------------------------------------------------
 * WLAN_HAL_ADD_TS_REQ
 *--------------------------------------------------------------------------*/

/* TSPEC Params */
typedef __ani_attr_pre_packed struct sSirMacTSInfoTfc
{
#ifndef ANI_LITTLE_BIT_ENDIAN
    tANI_U16       ackPolicy : 2;
    tANI_U16       userPrio : 3;
    tANI_U16       psb : 1;
    tANI_U16       aggregation : 1;
    tANI_U16       accessPolicy : 2;
    tANI_U16       direction : 2;
    tANI_U16       tsid : 4;
    tANI_U16       trafficType : 1;
#else
    tANI_U16       trafficType : 1;
    tANI_U16       tsid : 4;
    tANI_U16       direction : 2;
    tANI_U16       accessPolicy : 2;
    tANI_U16       aggregation : 1;
    tANI_U16       psb : 1;
    tANI_U16       userPrio : 3;
    tANI_U16       ackPolicy : 2;
#endif
} __ani_attr_packed tSirMacTSInfoTfc;

/* Flag to schedule the traffic type */
typedef __ani_attr_pre_packed struct sSirMacTSInfoSch
{
#ifndef ANI_LITTLE_BIT_ENDIAN
    tANI_U8        rsvd : 7;
    tANI_U8        schedule : 1;
#else
    tANI_U8        schedule : 1;
    tANI_U8        rsvd : 7;
#endif
} __ani_attr_packed tSirMacTSInfoSch;

/* Traffic and scheduling info */
typedef __ani_attr_pre_packed struct sSirMacTSInfo
{
    tSirMacTSInfoTfc traffic;
    tSirMacTSInfoSch schedule;
} __ani_attr_packed tSirMacTSInfo;

/* Information elements */
typedef __ani_attr_pre_packed struct sSirMacTspecIE
{
    tANI_U8             type;
    tANI_U8             length;
    tSirMacTSInfo       tsinfo;
    tANI_U16            nomMsduSz;
    tANI_U16            maxMsduSz;
    tANI_U32            minSvcInterval;
    tANI_U32            maxSvcInterval;
    tANI_U32            inactInterval;
    tANI_U32            suspendInterval;
    tANI_U32            svcStartTime;
    tANI_U32            minDataRate;
    tANI_U32            meanDataRate;
    tANI_U32            peakDataRate;
    tANI_U32            maxBurstSz;
    tANI_U32            delayBound;
    tANI_U32            minPhyRate;
    tANI_U16            surplusBw;
    tANI_U16            mediumTime;
}__ani_attr_packed tSirMacTspecIE;

typedef PACKED_PRE struct PACKED_POST
{
    /* Station Index */
    tANI_U16 staIdx;

    /* TSPEC handler uniquely identifying a TSPEC for a STA in a BSS */
    tANI_U16 tspecIdx;

    /* To program TPE with required parameters */
    tSirMacTspecIE   tspec;

    /* U-APSD Flags: 1b per AC.  Encoded as follows:
     b7 b6 b5 b4 b3 b2 b1 b0 =
     X  X  X  X  BE BK VI VO */
    tANI_U8 uAPSD;

    /* These parameters are for all the access categories */
    tANI_U32 srvInterval[WLAN_HAL_MAX_AC];   // Service Interval
    tANI_U32 susInterval[WLAN_HAL_MAX_AC];   // Suspend Interval
    tANI_U32 delayInterval[WLAN_HAL_MAX_AC]; // Delay Interval
           
} tAddTsParams, *tpAddTsParams;

typedef PACKED_PRE struct PACKED_POST
{
    tHalMsgHeader header;
    tAddTsParams  addTsParams;	
}  tAddTsReqMsg, *tpAddTsReqMsg;

/*---------------------------------------------------------------------------
 * WLAN_HAL_ADD_TS_RSP
 *--------------------------------------------------------------------------*/

typedef PACKED_PRE struct PACKED_POST
{
    /*success or failure */
    tANI_U32   status;
} tAddTsRspParams, *tpAddTsRspParams;

typedef PACKED_PRE struct PACKED_POST
{
    tHalMsgHeader header;
    tAddTsRspParams addTsRspParams;
}  tAddTsRspMsg, *tpAddTsRspMsg;


/*---------------------------------------------------------------------------
 * WLAN_HAL_DEL_TS_REQ
 *--------------------------------------------------------------------------*/

typedef PACKED_PRE struct PACKED_POST
{
    /* Station Index */
    tANI_U16 staIdx;

    /* TSPEC identifier uniquely identifying a TSPEC for a STA in a BSS */
    tANI_U16 tspecIdx;

    /* To lookup station id using the mac address */
    tSirMacAddr bssId; 

} tDelTsParams, *tpDelTsParams;

typedef PACKED_PRE struct PACKED_POST
{
    tHalMsgHeader header;
    tDelTsParams  delTsParams; 
}  tDelTsReqMsg, *tpDelTsReqMsg;

/*---------------------------------------------------------------------------
 * WLAN_HAL_DEL_TS_RSP
 *--------------------------------------------------------------------------*/

typedef PACKED_PRE struct PACKED_POST
{
    /*success or failure */
    tANI_U32   status;
} tDelTsRspParams, *tpDelTsRspParams;

typedef PACKED_PRE struct PACKED_POST
{
    tHalMsgHeader header;
    tDelTsRspParams delTsRspParams;
}  tDelTsRspMsg, *tpDelTsRspMsg;

/* End of TSpec Parameters */

/* Start of BLOCK ACK related Parameters */

/*---------------------------------------------------------------------------
 * WLAN_HAL_ADD_BA_SESSION_REQ
 *--------------------------------------------------------------------------*/

typedef PACKED_PRE struct PACKED_POST
{
    /* Station Index */
    tANI_U16 staIdx;

    /* Peer MAC Address */
    tSirMacAddr peerMacAddr;

    /* ADDBA Action Frame dialog token
       HAL will not interpret this object */
    tANI_U8 baDialogToken;

    /* TID for which the BA is being setup
       This identifies the TC or TS of interest */
    tANI_U8 baTID;

    /* 0 - Delayed BA (Not supported)
       1 - Immediate BA */
    tANI_U8 baPolicy;

    /* Indicates the number of buffers for this TID (baTID)
       NOTE - This is the requested buffer size. When this
       is processed by HAL and subsequently by HDD, it is
       possible that HDD may change this buffer size. Any
       change in the buffer size should be noted by PE and
       advertized appropriately in the ADDBA response */
    tANI_U16 baBufferSize;

    /* BA timeout in TU's 0 means no timeout will occur */
    tANI_U16 baTimeout;

    /* b0..b3 - Fragment Number - Always set to 0
       b4..b15 - Starting Sequence Number of first MSDU
       for which this BA is setup */
    tANI_U16 baSSN;

    /* ADDBA direction
       1 - Originator
       0 - Recipient */
    tANI_U8 baDirection;
} tAddBASessionParams, *tpAddBASessionParams;

typedef PACKED_PRE struct PACKED_POST
{
    tHalMsgHeader header;
    tAddBASessionParams  addBASessionParams; 
}tAddBASessionReqMsg, *tpAddBASessionReqMsg;

/*---------------------------------------------------------------------------
 * WLAN_HAL_ADD_BA_SESSION_RSP
 *--------------------------------------------------------------------------*/

typedef PACKED_PRE struct PACKED_POST
{
    /*success or failure */
    tANI_U32   status;

    /* Dialog token */
    tANI_U8 baDialogToken;

    /* TID for which the BA session has been setup */
    tANI_U8 baTID;

    /* BA Buffer Size allocated for the current BA session */
    tANI_U8 baBufferSize;

    tANI_U8 baSessionID;

    /* Reordering Window buffer */
    tANI_U8 winSize;
    
    /*Station Index to id the sta */
    tANI_U8 STAID;
    
    /* Starting Sequence Number */
    tANI_U16 SSN;
} tAddBASessionRspParams, *tpAddBASessionRspParams;

typedef PACKED_PRE struct PACKED_POST
{
    tHalMsgHeader header;
    tAddBASessionRspParams addBASessionRspParams;
}  tAddBASessionRspMsg, *tpAddBASessionRspMsg;

/*---------------------------------------------------------------------------
 * WLAN_HAL_ADD_BA_REQ
 *--------------------------------------------------------------------------*/

typedef PACKED_PRE struct PACKED_POST
{
    /* Session Id */
    tANI_U8 baSessionID;

    /* Reorder Window Size */
    tANI_U8 winSize;

#ifdef FEATURE_ON_CHIP_REORDERING
    tANI_BOOLEAN isReorderingDoneOnChip;
#endif
} tAddBAParams, *tpAddBAParams;

typedef PACKED_PRE struct PACKED_POST
{
    tHalMsgHeader header;
    tAddBAParams  addBAParams; 
}  tAddBAReqMsg, *tpAddBAReqMsg;


/*---------------------------------------------------------------------------
 * WLAN_HAL_ADD_BA_RSP
 *--------------------------------------------------------------------------*/

typedef PACKED_PRE struct PACKED_POST
{
    /*success or failure */
    tANI_U32   status;

    /* Dialog token */
    tANI_U8 baDialogToken;
 
} tAddBARspParams, *tpAddBARspParams;

typedef PACKED_PRE struct PACKED_POST
{
    tHalMsgHeader header;
    tAddBARspParams addBARspParams;
}  tAddBARspMsg, *tpAddBARspMsg;


/*---------------------------------------------------------------------------
 * WLAN_HAL_TRIGGER_BA_REQ
 *--------------------------------------------------------------------------*/


typedef struct sAddBaInfo
{
    tANI_U16 fBaEnable : 1;
    tANI_U16 startingSeqNum: 12;
    tANI_U16 reserved : 3;
}tAddBaInfo, *tpAddBaInfo;

typedef struct sTriggerBaRspCandidate
{
    tSirMacAddr staAddr;
    tAddBaInfo baInfo[STACFG_MAX_TC];
}tTriggerBaRspCandidate, *tpTriggerBaRspCandidate;

typedef struct sTriggerBaCandidate
{
    tANI_U8  staIdx;
    tANI_U8 tidBitmap;
}tTriggerBaReqCandidate, *tptTriggerBaReqCandidate;

typedef PACKED_PRE struct PACKED_POST
{
    /* Session Id */
    tANI_U8 baSessionID;

    /* baCandidateCnt is followed by trigger BA 
     * Candidate List(tTriggerBaCandidate)
     */
    tANI_U16 baCandidateCnt;
    
} tTriggerBAParams, *tpTriggerBAParams;

typedef PACKED_PRE struct PACKED_POST
{
   tHalMsgHeader header;
   tTriggerBAParams  triggerBAParams; 
}  tTriggerBAReqMsg, *tpTriggerBAReqMsg;


/*---------------------------------------------------------------------------
 * WLAN_HAL_TRIGGER_BA_RSP
 *--------------------------------------------------------------------------*/

typedef PACKED_PRE struct PACKED_POST
{
   
    /* TO SUPPORT BT-AMP */	
    tSirMacAddr  bssId; 

    /* success or failure */
    tANI_U32   status;

    /* baCandidateCnt is followed by trigger BA 
     * Rsp Candidate List(tTriggerRspBaCandidate)
     */
    tANI_U16 baCandidateCnt;
    

} tTriggerBARspParams, *tpTriggerBARspParams;

typedef PACKED_PRE struct PACKED_POST
{
    tHalMsgHeader header;
    tTriggerBARspParams triggerBARspParams;
}  tTriggerBARspMsg, *tpTriggerBARspMsg;

/*---------------------------------------------------------------------------
 * WLAN_HAL_DEL_BA_REQ
 *--------------------------------------------------------------------------*/

typedef PACKED_PRE struct PACKED_POST
{
    /* Station Index */
    tANI_U16 staIdx;

    /* TID for which the BA session is being deleted */
    tANI_U8 baTID;

    /* DELBA direction
       1 - Originator
       0 - Recipient */
    tANI_U8 baDirection;  
} tDelBAParams, *tpDelBAParams;

typedef PACKED_PRE struct PACKED_POST
{
   tHalMsgHeader header;
   tDelBAParams  delBAParams; 
}  tDelBAReqMsg, *tpDelBAReqMsg;

/*---------------------------------------------------------------------------
 * WLAN_HAL_DEL_BA_RSP
 *--------------------------------------------------------------------------*/

typedef PACKED_PRE struct PACKED_POST
{
    /* success or failure */
    tANI_U32   status;
} tDelBARspParams, *tpDelBARspParams;

typedef PACKED_PRE struct PACKED_POST
{
    tHalMsgHeader header;
    tDelBARspParams delBARspParams;
}  tDelBARspMsg, *tpDelBARspMsg;


/*---------------------------------------------------------------------------
 * WLAN_HAL_SET_KEYDONE_MSG
 *--------------------------------------------------------------------------*/

typedef PACKED_PRE struct PACKED_POST
{
  /*bssid of the keys */
  tANI_U8   bssidx;
} tSetKeyDoneParams, *tpSetKeyDoneParams;

typedef PACKED_PRE struct PACKED_POST
{
   tHalMsgHeader header;
   tSetKeyDoneParams setKeyDoneParams;
}  tSetKeyDoneMsg, *tpSetKeyDoneMsg;

/*---------------------------------------------------------------------------
 * WLAN_HAL_DOWNLOAD_NV_REQ
 *--------------------------------------------------------------------------*/
typedef PACKED_PRE struct PACKED_POST
{
    /* Fragment sequence number of the NV Image. Note that NV Image might not
     * fit into one message due to size limitation of the SMD channel FIFO. UMAC
     * can hence choose to chop the NV blob into multiple fragments starting with 
     * seqeunce number 0, 1, 2 etc. The last fragment MUST be indicated by 
     * marking the isLastFragment field to 1. Note that all the NV blobs would be
     * concatenated together by HAL without any padding bytes in between.*/
    tANI_U16 fragNumber;

    /* Is this the last fragment? When set to 1 it indicates that no more fragments
     * will be sent by UMAC and HAL can concatenate all the NV blobs rcvd & proceed 
     * with the parsing. HAL would generate a WLAN_HAL_DOWNLOAD_NV_RSP to the
     * WLAN_HAL_DOWNLOAD_NV_REQ only after it receives the last fragment */
    tANI_U16 isLastFragment;

    /* NV Validity BitMap. Bitmap indicates what NV fields in the NV blob
     * are valid. Note this is the complete bitmap and hence would be the same
     * in all fragments. E.g. bit[NV_TABLE_REGULATORY_DOMAINS] =1 in the 
     * nvValidityBitmap(b31, b32,...b0) indicates if NV_TABLE_REGULATORY_DOMAINS is
     * is valid in the NV Image */
    tANI_U32  nvValidityBitmap;

    /* NV Image size (number of bytes) */
    tANI_U32 nvImgBufferSize;

    /* Following the 'nvImageBufferSize', there should be nvImageBufferSize
     * bytes of NV Image i.e. uint8[nvImageBufferSize] */
} tHalNvImgDownloadReqParams, *tpHalNvImgDownloadReqParams;

typedef PACKED_PRE struct PACKED_POST
{
    /* Note: The length specified in tHalNvImgDownloadReqMsg messages should be
     * header.msgLen = sizeof(tHalNvImgDownloadReqMsg) + nvImgBufferSize */
    tHalMsgHeader header;
    tHalNvImgDownloadReqParams nvImageReqParams;
} tHalNvImgDownloadReqMsg, *tpHalNvImgDownloadReqMsg;

/*---------------------------------------------------------------------------
 * WLAN_HAL_DOWNLOAD_NV_RSP
 *--------------------------------------------------------------------------*/
typedef PACKED_PRE struct PACKED_POST
{
    /* Success or Failure. HAL would generate a WLAN_HAL_DOWNLOAD_NV_RSP
     * only after it receives the last fragment i.e. WLAN_HAL_DOWNLOAD_NV_REQ 
     * is rcvd with isLastFragment field set to 1 */
    tANI_U32   status;
} tHalNvImgDownloadRspParams, *tpHalNvImgDownloadRspParams;

typedef PACKED_PRE struct PACKED_POST
{
    tHalMsgHeader header;
    tHalNvImgDownloadRspParams nvImageRspParams;
}  tHalNvImgDownloadRspMsg, *tpHalNvImgDownloadRspMsg;

/*---------------------------------------------------------------------------
 * WLAN_HAL_DEL_NV_IND
 *--------------------------------------------------------------------------*/
typedef PACKED_PRE struct PACKED_POST
{
    /* NV BitMap. Bitmap indicates what NV fields are to be 
     * deleted/invalidated. E.g. bit[NV_TABLE_REGULATORY_DOMAINS] in the 
     * nvDeleteBitmap(b31, b32,...b0) corresponds to NV_TABLE_REGULATORY_DOMAINS */
    tANI_U32  nvDeleteBitmap;
} tHalNvDeleteParams, *tpHalNvDeleteParams;

typedef PACKED_PRE struct PACKED_POST
{
    tHalMsgHeader header;
    tHalNvDeleteParams nNvDeleteParams;
} tHalNvDeleteInd, *tpHalNvDeleteInd;

/*---------------------------------------------------------------------------
 * WLAN_HAL_STORE_NV_IND
 *--------------------------------------------------------------------------*/
typedef PACKED_PRE struct PACKED_POST
{
    /* NV Item */
    eNvTable tableID;

    /* Size of NV Blob */
    tANI_U32 nvBlobSize;

    /* Following the 'nvBlobSize', there should be nvBlobSize bytes of 
     * NV blob i.e. uint8[nvBlobSize] */
} tHalNvStoreParams, *tpHalNvStoreParams;

typedef PACKED_PRE struct PACKED_POST
{
    /* Note: The length specified in tHalNvStoreInd messages should be
     * header.msgLen = sizeof(tHalNvStoreInd) + nvBlobSize */
    tHalMsgHeader header;
    tHalNvStoreParams nvStoreParams;
}  tHalNvStoreInd, *tpHalNvStoreInd;

/* End of Block Ack Related Parameters */

/*---------------------------------------------------------------------------
 * WLAN_HAL_MIC_FAILURE_IND
 *--------------------------------------------------------------------------*/

#define SIR_CIPHER_SEQ_CTR_SIZE 6

typedef PACKED_PRE struct PACKED_POST
{
	tSirMacAddr  srcMacAddr;     //address used to compute MIC 
    tSirMacAddr  taMacAddr;      //transmitter address
    tSirMacAddr  dstMacAddr;
	tANI_U8      multicast;             
	tANI_U8      IV1;            // first byte of IV
	tANI_U8      keyId;          // second byte of IV
	tANI_U8      TSC[SIR_CIPHER_SEQ_CTR_SIZE]; // sequence number
    tSirMacAddr  rxMacAddr;      // receive address
} tSirMicFailureInfo, *tpSirMicFailureInfo;

/* Definition for MIC failure indication
   MAC reports this each time a MIC failure occures on Rx TKIP packet
 */
typedef PACKED_PRE struct PACKED_POST
{
    tSirMacAddr         bssId;   // BSSID
    tSirMicFailureInfo  info;
} tSirMicFailureInd, *tpSirMicFailureInd;

typedef PACKED_PRE struct PACKED_POST
{
   tHalMsgHeader header;
   tSirMicFailureInd micFailureInd;
}  tMicFailureIndMsg, *tpMicFailureIndMsg;

/*---------------------------------------------------------------------------
 * WLAN_HAL_UPDATE_BEACON_REQ
 *--------------------------------------------------------------------------*/
typedef PACKED_PRE struct PACKED_POST
{

    tANI_U8  bssIdx;

    //shortPreamble mode. HAL should update all the STA rates when it
    //receives this message
    tANI_U8 fShortPreamble;
    //short Slot time.
    tANI_U8 fShortSlotTime;
    //Beacon Interval
    tANI_U16 beaconInterval;
    //Protection related
    tANI_U8 llaCoexist;
    tANI_U8 llbCoexist;
    tANI_U8 llgCoexist;
    tANI_U8 ht20MhzCoexist;
    tANI_U8 llnNonGFCoexist;
    tANI_U8 fLsigTXOPProtectionFullSupport;
    tANI_U8 fRIFSMode;

    tANI_U16 paramChangeBitmap;
}tUpdateBeaconParams, *tpUpdateBeaconParams;


typedef PACKED_PRE struct PACKED_POST
{
    tHalMsgHeader header;
    tUpdateBeaconParams updateBeaconParam;
}  tUpdateBeaconReqMsg, *tpUpdateBeaconReqMsg;

/*---------------------------------------------------------------------------
 * WLAN_HAL_UPDATE_BEACON_RSP
 *--------------------------------------------------------------------------*/
typedef PACKED_PRE struct PACKED_POST
{
    tANI_U32   status;
} tUpdateBeaconRspParams, *tpUpdateBeaconRspParams;

typedef PACKED_PRE struct PACKED_POST
{
    tHalMsgHeader header;
    tUpdateBeaconRspParams updateBeaconRspParam;
}  tUpdateBeaconRspMsg, *tpUpdateBeaconRspMsg;

/*---------------------------------------------------------------------------
 * WLAN_HAL_SEND_BEACON_REQ
 *--------------------------------------------------------------------------*/
typedef PACKED_PRE struct PACKED_POST
{
    tANI_U32 beaconLength; //length of the template.
    tANI_U8 beacon[BEACON_TEMPLATE_SIZE];     // Beacon data.
    tSirMacAddr bssId;
#ifdef WLAN_SOFTAP_FEATURE
    tANI_U32 timIeOffset; //TIM IE offset from the beginning of the template.
#endif
    
}tSendBeaconParams, *tpSendBeaconParams;


typedef PACKED_PRE struct PACKED_POST
{
  tHalMsgHeader header;
  tSendBeaconParams sendBeaconParam;
}tSendBeaconReqMsg, *tpSendBeaconReqMsg;

/*---------------------------------------------------------------------------
 * WLAN_HAL_SEND_BEACON_RSP
 *--------------------------------------------------------------------------*/
typedef PACKED_PRE struct PACKED_POST
{
    tANI_U32   status;
} tSendBeaconRspParams, *tpSendBeaconRspParams;

typedef PACKED_PRE struct PACKED_POST
{
    tHalMsgHeader header;
    tSendBeaconRspParams sendBeaconRspParam;
}  tSendBeaconRspMsg, *tpSendBeaconRspMsg;

#ifdef FEATURE_5GHZ_BAND

/*---------------------------------------------------------------------------
 * WLAN_HAL_ENABLE_RADAR_DETECT_REQ
 *--------------------------------------------------------------------------*/
typedef PACKED_PRE struct PACKED_POST
{
    tSirMacAddr BSSID;
    tANI_U8   channel; 
}tSirEnableRadarInfoType, *tptSirEnableRadarInfoType;


typedef PACKED_PRE struct PACKED_POST
{
    /* Link Parameters */
    tSirEnableRadarInfoType EnableRadarInfo;
}tEnableRadarReqParams, *tpEnableRadarReqParams;

typedef PACKED_PRE struct PACKED_POST
{
    tHalMsgHeader header;
    tEnableRadarReqParams  enableRadarReqParams; 
}tEnableRadarReqMsg, *tpEnableRadarReqMsg;

/*---------------------------------------------------------------------------
 * WLAN_HAL_ENABLE_RADAR_DETECT_RSP
 *--------------------------------------------------------------------------*/

typedef PACKED_PRE struct PACKED_POST
{
    /* Link Parameters */
    tSirMacAddr BSSID;
    /* success or failure */	
    tANI_U32   status;
}tEnableRadarRspParams, *tpEnableRadarRspParams;

typedef PACKED_PRE struct PACKED_POST
{
    tHalMsgHeader header;
    tEnableRadarRspParams  enableRadarRspParams; 
}tEnableRadarRspMsg, *tpEnableRadarRspMsg;

/*---------------------------------------------------------------------------
 *WLAN_HAL_RADAR_DETECT_INTR_IND
 *--------------------------------------------------------------------------*/

typedef PACKED_PRE struct PACKED_POST
{
    tANI_U8 radarDetChannel;
}tRadarDetectIntrIndParams, *tpRadarDetectIntrIndParams;

typedef PACKED_PRE struct PACKED_POST
{
    tHalMsgHeader header;
    tRadarDetectIntrIndParams  radarDetectIntrIndParams; 
}tRadarDetectIntrIndMsg, *tptRadarDetectIntrIndMsg;

/*---------------------------------------------------------------------------
 *WLAN_HAL_RADAR_DETECT_IND
 *-------------------------------------------------------------------------*/
typedef PACKED_PRE struct PACKED_POST
{
    /*channel number in which the RADAR detected*/
    tANI_U8          channelNumber;

    /*RADAR pulse width*/
    tANI_U16         radarPulseWidth; // in usecond

    /*Number of RADAR pulses */
    tANI_U16         numRadarPulse;
}tRadarDetectIndParams,*tpRadarDetectIndParams;

typedef PACKED_PRE struct PACKED_POST
{
   tHalMsgHeader header;
   tRadarDetectIndParams  radarDetectIndParams; 
}tRadarDetectIndMsg, *tptRadarDetectIndMsg;


/*---------------------------------------------------------------------------
 *WLAN_HAL_GET_TPC_REPORT_REQ
 *-------------------------------------------------------------------------*/
typedef PACKED_PRE struct PACKED_POST
{
   tSirMacAddr sta;
   tANI_U8     dialogToken;
   tANI_U8     txpower;
}tSirGetTpcReportReqParams, *tpSirGetTpcReportReqParams;


typedef PACKED_PRE struct PACKED_POST
{
   tHalMsgHeader header;
   tSirGetTpcReportReqParams  getTpcReportReqParams; 
}tSirGetTpcReportReqMsg, *tpSirGetTpcReportReqMsg;

/*---------------------------------------------------------------------------
 * WLAN_HAL_GET_TPC_REPORT_RSP
 *--------------------------------------------------------------------------*/

typedef PACKED_PRE struct PACKED_POST
{
    /* success or failure */	
    tANI_U32   status;
}tSirGetTpcReportRspParams, *tpSirGetTpcReportRspParams;

typedef PACKED_PRE struct PACKED_POST
{
    tHalMsgHeader header;
    tSirGetTpcReportRspParams  getTpcReportRspParams; 
}tSirGetTpcReportRspMsg, *tpSirGetTpcReportRspMsg;

#endif

#if defined(__ANI_COMPILER_PRAGMA_PACK_STACK)
#pragma pack(pop)
#elif defined(__ANI_COMPILER_PRAGMA_PACK)
#else
#endif

#endif /* _WLAN_HAL_MSG_H_ */
