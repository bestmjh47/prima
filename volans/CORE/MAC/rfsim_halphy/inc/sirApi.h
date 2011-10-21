/*
 * Airgo Networks, Inc proprietary. All rights reserved.
 * This file sirApi.h contains definitions exported by
 * Sirius software.
 * Author:        Chandra Modumudi
 * Date:          04/16/2002
 * History:-
 * Date           Modified by    Modification Information
 * --------------------------------------------------------------------
 */

#ifndef __SIR_API_H
#define __SIR_API_H

#include "sirTypes.h"
#include "sirMacProtDef.h"
#include "aniSystemDefs.h"


/// Maximum number of STAs allowed in the BSS
#define SIR_MAX_NUM_STA                256

/// Maximum number of Neighbors reported by STA for LB feature
#define SIR_MAX_NUM_NEIGHBOR_BSS       3

/// Maximum number of Neighbors reported by STA for LB feature
#define SIR_MAX_NUM_ALTERNATE_RADIOS   5

/// Maximum size of SCAN_RSP message
#define SIR_MAX_SCAN_RSP_MSG_LENGTH    2600

/// Start of Sirius software/Host driver message types
#define SIR_HAL_HOST_MSG_START         0x1000

/// Power save level definitions
#define SIR_MAX_POWER_SAVE          3
#define SIR_INTERMEDIATE_POWER_SAVE 4
#define SIR_NO_POWER_SAVE           5

/// Max supported channel list
#define SIR_MAX_SUPPORTED_CHANNEL_LIST      96

/// Maximum DTIM Factor
#define SIR_MAX_DTIM_FACTOR         32



#define SIR_NUM_11B_RATES 4   //1,2,5.5,11
#define SIR_NUM_11A_RATES 8  //6,9,12,18,24,36,48,54
#define SIR_NUM_POLARIS_RATES 3 //72,96,108
#define SIR_NUM_TITAN_RATES 26
#define SIR_NUM_TAURUS_RATES 4 //136.5, 151.7,283.5,315
#define SIR_NUM_PROP_RATES  (SIR_NUM_TITAN_RATES + SIR_NUM_TAURUS_RATES)

#define SIR_11N_PROP_RATE_136_5 (1<<28)
#define SIR_11N_PROP_RATE_151_7 (1<<29)
#define SIR_11N_PROP_RATE_283_5 (1<<30)
#define SIR_11N_PROP_RATE_315     (1<<31)
#define SIR_11N_PROP_RATE_BITMAP 0x80000000 //only 315MBPS rate is supported today
//Taurus is going to support 26 Titan Rates(no ESF/concat Rates will be supported)
//First 26 bits are reserved for Titan and last 4 bits for Taurus, 2(27 and 28) bits are reserved.
//#define SIR_TITAN_PROP_RATE_BITMAP 0x03FFFFFF
//Disable all Titan rates
#define SIR_TITAN_PROP_RATE_BITMAP 0
#define SIR_CONVERT_2_U32_BITMAP(nRates) ((nRates + 31)/32)

/* #tANI_U32's needed for a bitmap representation for all prop rates */
#define SIR_NUM_U32_MAP_RATES    SIR_CONVERT_2_U32_BITMAP(SIR_NUM_PROP_RATES)


#define SIR_PM_SLEEP_MODE   0
#define SIR_PM_ACTIVE_MODE        1

// Used by various modules to load ALL CFG's
#define ANI_IGNORE_CFG_ID 0xFFFF

enum eSirHostMsgTypes
{
    SIR_HAL_APP_SETUP_NTF = SIR_HAL_HOST_MSG_START,
    SIR_HAL_INITIAL_CAL_FAILED_NTF,
    SIR_HAL_NIC_OPER_NTF,
    SIR_HAL_INIT_START_REQ,
    SIR_HAL_SHUTDOWN_REQ,
    SIR_HAL_SHUTDOWN_CNF,
    SIR_HAL_RESET_REQ,
    SIR_HAL_RADIO_ON_OFF_IND,    
    SIR_HAL_RESET_CNF,
    SIR_WRITE_TO_TD,
    SIR_HAL_HDD_ADDBA_REQ, // MAC -> HDD
    SIR_HAL_HDD_ADDBA_RSP, // HDD -> HAL        
    SIR_HAL_DELETEBA_IND, // MAC -> HDD
    SIR_HAL_BA_FAIL_IND // HDD -> MAC
};



/**
 * Module ID definitions.
 */
enum {
    SIR_BOOT_MODULE_ID = 1,
    SIR_HAL_MODULE_ID  = 0x10,
    SIR_CFG_MODULE_ID,
    SIR_LIM_MODULE_ID,
    SIR_ARQ_MODULE_ID,
    SIR_SCH_MODULE_ID,
    SIR_PMM_MODULE_ID,
    SIR_MNT_MODULE_ID,
    SIR_DBG_MODULE_ID,
    SIR_DPH_MODULE_ID,
    SIR_SYS_MODULE_ID,
    SIR_SMS_MODULE_ID,

    SIR_PHY_MODULE_ID = 0x20,


    // Add any modules above this line
    SIR_DVT_MODULE_ID
};


/**
 * First and last module definition for logging utility
 *
 * NOTE:  The following definitions need to be updated if
 *        the above list is changed.
 */
#define SIR_FIRST_MODULE_ID     SIR_HAL_MODULE_ID
#define SIR_LAST_MODULE_ID      SIR_DVT_MODULE_ID


// Type declarations used by Firmware and Host software

// Scan type enum used in scan request
typedef enum eSirScanType
{
    eSIR_PASSIVE_SCAN,
    eSIR_ACTIVE_SCAN,
} tSirScanType;

/// Result codes Firmware return to Host SW
typedef enum eSirResultCodes
{
    eSIR_SME_SUCCESS,

    eSIR_EOF_SOF_EXCEPTION,
    eSIR_BMU_EXCEPTION,
    eSIR_LOW_PDU_EXCEPTION,
    eSIR_USER_TRIG_RESET,
    eSIR_LOGP_EXCEPTION,
    eSIR_CP_EXCEPTION,
    eSIR_STOP_BSS,
    eSIR_AHB_HANG_EXCEPTION,
    eSIR_DPU_EXCEPTION,
    eSIR_RPE_EXCEPTION,
    eSIR_TPE_EXCEPTION,
    eSIR_DXE_EXCEPTION,
    eSIR_RXP_EXCEPTION,
    eSIR_MCPU_EXCEPTION,
    eSIR_MCU_EXCEPTION,
    eSIR_MTU_EXCEPTION,
    eSIR_MIF_EXCEPTION,
    eSIR_FW_EXCEPTION,
    eSIR_PHY_HANG_EXCEPTION,
    eSIR_MAILBOX_SANITY_CHK_FAILED,
    eSIR_RADIO_HW_SWITCH_STATUS_IS_OFF, // Only where this switch is present
    eSIR_CFB_FLAG_STUCK_EXCEPTION,

    eSIR_SME_BASIC_RATES_NOT_SUPPORTED_STATUS=30,

    eSIR_SME_INVALID_PARAMETERS=500,
    eSIR_SME_UNEXPECTED_REQ_RESULT_CODE,
    eSIR_SME_RESOURCES_UNAVAILABLE,
    eSIR_SME_SCAN_FAILED,   // Unable to find a BssDescription
                            // matching requested scan criteria
    eSIR_SME_BSS_ALREADY_STARTED_OR_JOINED,
    eSIR_SME_LOST_LINK_WITH_PEER_RESULT_CODE,
    eSIR_SME_REFUSED,
    eSIR_SME_JOIN_TIMEOUT_RESULT_CODE,
    eSIR_SME_AUTH_TIMEOUT_RESULT_CODE,
    eSIR_SME_ASSOC_TIMEOUT_RESULT_CODE,
    eSIR_SME_REASSOC_TIMEOUT_RESULT_CODE,
    eSIR_SME_MAX_NUM_OF_PRE_AUTH_REACHED,
    eSIR_SME_AUTH_REFUSED,
    eSIR_SME_INVALID_WEP_DEFAULT_KEY,
    eSIR_SME_NO_KEY_MAPPING_KEY_FOR_PEER,
    eSIR_SME_ASSOC_REFUSED,
    eSIR_SME_REASSOC_REFUSED,
    eSIR_SME_DEAUTH_WHILE_JOIN, //Recieved Deauth while joining or pre-auhtentication.
    eSIR_SME_DISASSOC_WHILE_JOIN, //Recieved Disassociation while joining.
    eSIR_SME_DEAUTH_WHILE_REASSOC, //Recieved Deauth while ReAssociate.
    eSIR_SME_DISASSOC_WHILE_REASSOC, //Recieved Disassociation while ReAssociate
    eSIR_SME_STA_NOT_AUTHENTICATED,
    eSIR_SME_STA_NOT_ASSOCIATED,
    eSIR_SME_STA_DISASSOCIATED,
    eSIR_SME_ALREADY_JOINED_A_BSS,
    eSIR_ULA_COMPLETED,
    eSIR_ULA_FAILURE,
    eSIR_SME_LINK_ESTABLISHED,
    eSIR_SME_UNABLE_TO_PERFORM_MEASUREMENTS,
    eSIR_SME_UNABLE_TO_PERFORM_DFS,
    eSIR_SME_DFS_FAILED,
    eSIR_SME_TRANSFER_STA, // To be used when STA need to be LB'ed
    eSIR_SME_INVALID_LINK_TEST_PARAMETERS,// Given in LINK_TEST_START_RSP
    eSIR_SME_LINK_TEST_MAX_EXCEEDED,    // Given in LINK_TEST_START_RSP
    eSIR_SME_UNSUPPORTED_RATE,          // Given in LINK_TEST_RSP if peer does
                                        // support requested rate in
                                        // LINK_TEST_REQ
    eSIR_SME_LINK_TEST_TIMEOUT,         // Given in LINK_TEST_IND if peer does
                                        // not respond before next test packet
                                        // is sent
    eSIR_SME_LINK_TEST_COMPLETE,        // Given in LINK_TEST_IND at the end
                                        // of link test
    eSIR_SME_LINK_TEST_INVALID_STATE,   // Given in LINK_TEST_START_RSP
    eSIR_SME_LINK_TEST_TERMINATE,       // Given in LINK_TEST_START_RSP
    eSIR_SME_LINK_TEST_INVALID_ADDRESS, // Given in LINK_TEST_STOP_RSP
    eSIR_SME_POLARIS_RESET,             // Given in SME_STOP_BSS_REQ
    eSIR_SME_SETCONTEXT_FAILED,         // Given in SME_SETCONTEXT_REQ when
                                        // unable to plumb down keys
    eSIR_SME_BSS_RESTART,               // Given in SME_STOP_BSS_REQ

    eSIR_SME_MORE_SCAN_RESULTS_FOLLOW,  // Given in SME_SCAN_RSP message
                                        // that more SME_SCAN_RSP
                                        // messages are following.
                                        // SME_SCAN_RSP message with
                                        // eSIR_SME_SUCCESS status
                                        // code is the last one.
    eSIR_SME_INVALID_ASSOC_RSP_RXED,    // Sent in SME_JOIN/REASSOC_RSP
                                        // messages upon receiving
                                        // invalid Re/Assoc Rsp frame.
    eSIR_SME_MIC_COUNTER_MEASURES,      // STOP BSS triggered by MIC failures: MAC software to disassoc all stations
                                        // with MIC_FAILURE reason code and perform the stop bss operation
    eSIR_SME_ADDTS_RSP_TIMEOUT,         // didn't get response from peer within
                                        // timeout interval
    eSIR_SME_ADDTS_RSP_FAILED,          // didn't get success response from HAL
    eSIR_SME_RECEIVED,
    // TBA - TSPEC related Result Codes

    eSIR_SME_CHANNEL_SWITCH_FAIL,        // failed to send out Channel Switch Action Frame
    eSIR_SME_INVALID_STA_ROLE,
#ifdef GEN4_SCAN
    eSIR_SME_CHANNEL_SWITCH_DISABLED,    // either 11h is disabled or channelSwitch is currently active
    eSIR_SME_HAL_SCAN_INIT_FAILED,       // SIR_HAL_INIT_SCAN_RSP returned failed status
    eSIR_SME_HAL_SCAN_START_FAILED,      // SIR_HAL_START_SCAN_RSP returned failed status
    eSIR_SME_HAL_SCAN_END_FAILED,        // SIR_HAL_END_SCAN_RSP returned failed status
    eSIR_SME_HAL_SCAN_FINISH_FAILED,     // SIR_HAL_FINISH_SCAN_RSP returned failed status
    eSIR_SME_HAL_SEND_MESSAGE_FAIL,      // Failed to send a message to HAL
#else // GEN4_SCAN
    eSIR_SME_CHANNEL_SWITCH_DISABLED,    // either 11h is disabled or channelSwitch is currently active
    eSIR_SME_HAL_SEND_MESSAGE_FAIL,      // Failed to send a message to HAL
#endif // GEN4_SCAN
    eSIR_SME_STOP_BSS_FAILURE,           // Failed to stop the bss
    eSIR_SME_STA_ASSOCIATED,
    eSIR_SME_INVALID_PMM_STATE,
    eSIR_SME_CANNOT_ENTER_IMPS,
    eSIR_SME_IMPS_REQ_FAILED,
    eSIR_SME_BMPS_REQ_FAILED,
    eSIR_SME_UAPSD_REQ_FAILED,
    eSIR_SME_WOWL_ENTER_REQ_FAILED,
    eSIR_SME_WOWL_EXIT_REQ_FAILED,
    eSIR_DONOT_USE_RESULT_CODE = SIR_MAX_ENUM_SIZE
    
} tSirResultCodes;

//
// Enumerated constants to identify
// 1) the operating state of Channel Bonding
// 2) the secondary CB channel to be used
//
typedef enum eAniCBSecondaryMode
{
    eANI_CB_SECONDARY_NONE,
    eANI_CB_SECONDARY_DOWN,
    eANI_CB_SECONDARY_UP,
    eANI_DONOT_USE_SECONDARY_MODE = SIR_MAX_ENUM_SIZE
} tAniCBSecondaryMode;




/* each station added has a rate mode which specifies the sta attributes */
typedef enum eStaRateMode {
    eSTA_TAURUS = 0,
    eSTA_TITAN,
    eSTA_POLARIS,
    eSTA_11b,
    eSTA_11bg,
    eSTA_11a,
    eSTA_11n,
    eSTA_INVALID_RATE_MODE
} tStaRateMode, *tpStaRateMode;

//although in tSirSupportedRates each IE is 16bit but PE only passes IEs in 8 bits with MSB=1 for basic rates.
//change the mask for bit0-7 only so HAL gets correct basic rates for setting response rates.
#define IERATE_BASICRATE_MASK     0x80
#define IERATE_RATE_MASK          0x7f
#define IERATE_IS_BASICRATE(x)   ((x) & IERATE_BASICRATE_MASK)
#define ANIENHANCED_TAURUS_RATEMAP_BITOFFSET_START  28

typedef struct sSirSupportedRates {
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


typedef enum eSirRFBand
{
    SIR_BAND_UNKNOWN,
    SIR_BAND_2_4_GHZ,
    SIR_BAND_5_GHZ,
} tSirRFBand;


/*
* Specifies which beacons are to be indicated upto the host driver when
* Station is in power save mode.
*/
typedef enum eBeaconForwarding
{
    ePM_BEACON_FWD_NTH,
    ePM_BEACON_FWD_TIM,
    ePM_BEACON_FWD_DTIM,
    ePM_BEACON_FWD_NONE
} tBeaconForwarding;

//
// A bit-encoding, identifying the new TITAN capabilities
// and state information. The capabilities being exposed
// are -
// Concatenation
// Compression
//FIXME_CBMODE: need to seperate out HT and TITAN CB mode fields.
// Channel Bonding - Only this filed is used for HT also. 
// Reverse FCS
//
// The bitfield encoding is as follows -
//
//  b7  b6   b5   b4   b3   b2   b1   b0
// --------------------------------------
// | X | X |CB/O|CB/O|CB/A|RFCS| CP | CC |
// --------------------------------------
// where,
// CC   - Concatenation: 1 - ON, 0 - OFF
// CP   - Compression: 1 - ON, 0 - OFF
// RFCS - Reverse FCS Support: 1 - ON, 0 - OFF
// CB/A - Channel Bonding "Admin" state: 1 - ON, 0 - OFF
// CB/O - Channel Bonding "Oper" state:
//        00 - CB Oper state OFF
//        01 - CB Secondary channel DOWN
//        10 - CB Secondary channel UP
//        11 - Reserved
// X    - Don't care
//
// This enumerated data type is used for IPC between the
// LIM and SME (WSM/HDD) for the following northbound
// interfaces -
// LIM -> WSM
// tSirNeighborBssInfo,
// tSirSmeAssocInd,
// tSirSmeReassocInd
//
// LIM -> HDD/Roaming
// tSirBssDescription
//
typedef tANI_U8 tAniTitanHtCapabilityInfo;

//
// Identifies the neighbor BSS' that was(were) detected
// by an STA and reported to the AP
//
typedef struct sAniTitanCBNeighborInfo
{
  // A BSS was found on the Primary
  tANI_U8 cbBssFoundPri;

  // A BSS was found on the adjacent Upper Secondary
  tANI_U8 cbBssFoundSecUp;

  // A BSS was found on the adjacent Lower Secondary
  tANI_U8 cbBssFoundSecDown;

} tAniTitanCBNeighborInfo, *tpAniTitanCBNeighborInfo;

//
// MACRO's to extract info from tAniTitanHtCapabilityInfo
//
#define SME_GET_CONCAT_STATE(titanHtCaps) \
        (titanHtCaps & 0x01)
#define SME_SET_CONCAT_STATE(titanHtCaps,state) \
        (((state) == eHAL_CLEAR)? \
          ((titanHtCaps) = (titanHtCaps) & (0x3E)): \
          ((titanHtCaps) = (titanHtCaps) | (0x01)))

#define SME_GET_COMPRESSION_STATE(titanHtCaps) \
        ((titanHtCaps & 0x02) >> 1)
#define SME_SET_COMPRESSION_STATE(titanHtCaps,state) \
        (((state) == eHAL_CLEAR)? \
          ((titanHtCaps) = (titanHtCaps) & (0x3D)): \
          ((titanHtCaps) = (titanHtCaps) | (0x02)))

#define SME_GET_RFCS_STATE(titanHtCaps) \
        ((titanHtCaps & 0x04) >> 2)
#define SME_SET_RFCS_STATE(titanHtCaps,state) \
        (((state) == eHAL_CLEAR)? \
          ((titanHtCaps) = (titanHtCaps) & (0x3B)): \
          ((titanHtCaps) = (titanHtCaps) | (0x04)))

#define SME_GET_CB_ADMIN_STATE(titanHtCaps) \
        ((titanHtCaps & 0x08) >> 3)
#define SME_SET_CB_ADMIN_STATE(titanHtCaps,state) \
        (((state) == eHAL_CLEAR)? \
          ((titanHtCaps) = (titanHtCaps) & (0x37)): \
          ((titanHtCaps) = (titanHtCaps) | (0x08)))

// NOTE - The value returned by this MACRO, SME_GET_CB_OPER_STATE,
// can be used along with the enumerated type,
// tAniCBSecondaryMode, to identify the Admin/Oper state of CB
#define SME_GET_CB_OPER_STATE(titanHtCaps) \
        ((titanHtCaps & 0x30) >> 4)
#define SME_SET_CB_OPER_STATE(titanHtCaps,state) \
        ((titanHtCaps) = (tANI_U8)(((titanHtCaps) & (0x0F)) | ((state) << 4)))

/// Generic type for sending a response message
/// with result code to host software
typedef struct sSirSmeRsp
{
    tANI_U16             messageType; // eWNI_SME_*_RSP
    tANI_U16             length;
    tSirResultCodes statusCode;
} tSirSmeRsp, *tpSirSmeRsp;

/// Definition for kick starting Firmware on STA
typedef struct sSirSmeStartReq
{
    tANI_U16   messageType; // eWNI_SME_START_REQ
    tANI_U16   length;
    tANI_U32   roamingAtPolaris;
#if (WNI_POLARIS_FW_PRODUCT == WLAN_STA) || defined(ANI_AP_CLIENT_SDK)
    tANI_U32   sendNewBssInd;
#endif
} tSirSmeStartReq, *tpSirSmeStartReq;

/// Definition for indicating all modules ready on STA
typedef struct sSirSmeReadyReq
{
    tANI_U16   messageType; // eWNI_SME_SYS_READY_IND
    tANI_U16   length;
} tSirSmeReadyReq, *tpSirSmeReadyReq;

/// Definition for response message to previously issued start request
typedef struct sSirSmeStartRsp
{
    tANI_U16             messageType; // eWNI_SME_START_RSP
    tANI_U16             length;
    tSirResultCodes statusCode;
} tSirSmeStartRsp, *tpSirSmeStartRsp;

#if (WNI_POLARIS_FW_PACKAGE == ADVANCED) && (WNI_POLARIS_FW_PRODUCT == AP)
/**
 * Trigger Type
 */
typedef enum {
    // During Initialization and NM triggers only
    eSIR_TRIGGER_INIT_NM,

    // During Initialization, NM triggers and Periodic evaluation
    eSIR_TRIGGER_INIT_NM_PERIODIC
}tSirDfsTrigType;
#endif

/// Definition for Load structure
typedef struct sSirLoad
{
    tANI_U16             numStas;
    tANI_U16             channelUtilization;
} tSirLoad, *tpSirLoad;

/// BSS type enum used in while scanning/joining etc
typedef enum eSirBssType
{
    eSIR_INFRASTRUCTURE_MODE,
    eSIR_IBSS_MODE,
    eSIR_AUTO_MODE,
    eSIR_DONOT_USE_BSS_TYPE = SIR_MAX_ENUM_SIZE
} tSirBssType;

/// Definition for WDS Information
typedef struct sSirWdsInfo
{
    tANI_U16                wdsLength;
    tANI_U8                 wdsBytes[ANI_WDS_INFO_MAX_LENGTH];
} tSirWdsInfo, *tpSirWdsInfo;

/// Power Capability info used in 11H
typedef struct sSirMacPowerCapInfo
{
    tANI_U8              minTxPower;
    tANI_U8              maxTxPower;
} tSirMacPowerCapInfo, *tpSirMacPowerCapInfo;

/// Supported Channel info used in 11H
typedef struct sSirSupChnl
{
    tANI_U8              numChnl;
    tANI_U8              channelList[SIR_MAX_SUPPORTED_CHANNEL_LIST];
} tSirSupChnl, *tpSirSupChnl;

typedef enum eSirNwType
{
    eSIR_11A_NW_TYPE,
    eSIR_11B_NW_TYPE,
    eSIR_11G_NW_TYPE,
    eSIR_11N_NW_TYPE,
    eSIR_DONOT_USE_NW_TYPE = SIR_MAX_ENUM_SIZE
} tSirNwType;

/// Definition for new iBss peer info
typedef struct sSirNewIbssPeerInfo
{
    tSirMacAddr    peerAddr;
    tANI_U16            aid;
} tSirNewIbssPeerInfo, *tpSirNewIbssPeerInfo;

/// Definition for Alternate BSS info
typedef struct sSirAlternateRadioInfo
{
    tSirMacAddr    bssId;
    tANI_U8             channelId;
} tSirAlternateRadioInfo, *tpSirAlternateRadioInfo;

/// Definition for Alternate BSS list
typedef struct sSirAlternateRadioList
{
    tANI_U8                       numBss;
    tSirAlternateRadioInfo   alternateRadio[1];
} tSirAlternateRadioList, *tpSirAlternateRadioList;

/// Definition for kick starting BSS
/// ---> MAC
/**
 * Usage of ssId, numSSID & ssIdList:
 * ---------------------------------
 * 1. ssId.length of zero indicates that Broadcast/Suppress SSID
 *    feature is enabled.
 * 2. If ssId.length is zero, MAC SW will advertise NULL SSID
 *    and interpret the SSID list from numSSID & ssIdList.
 * 3. If ssId.length is non-zero, MAC SW will advertise the SSID
 *    specified in the ssId field and it is expected that
 *    application will set numSSID to one (only one SSID present
 *    in the list) and SSID in the list is same as ssId field.
 * 4. Application will always set numSSID >= 1.
 */
//*****NOTE: Please make sure all codes are updated if inserting field into this structure..**********
typedef struct sSirSmeStartBssReq
{
    tANI_U16                     messageType;       // eWNI_SME_START_BSS_REQ
    tANI_U16                     length;
    tSirBssType             bssType;
    tSirMacSSid             ssId;
    tANI_U8                      channelId;
    tAniCBSecondaryMode     cbMode;
#if (WNI_POLARIS_FW_PACKAGE == ADVANCED) && (WNI_POLARIS_FW_PRODUCT == AP)
    tSirAlternateRadioList  alternateRadioList;
    tANI_S8                      powerLevel;
    tSirWdsInfo             wdsInfo;
#endif
    tSirRSNie               rsnIE;             // RSN IE to be sent in
                                               // Beacon and Probe
                                               // Response frames
    tSirNwType              nwType;            // Indicates 11a/b/g
    tSirMacRateSet          operationalRateSet;// Has 11a or 11b rates
#if (WNI_POLARIS_FW_PACKAGE == ADVANCED) && (WNI_POLARIS_FW_PRODUCT == AP)
    tANI_U8                      numSSID;
    tSirMacSSid             ssIdList[ANI_MAX_NUM_OF_SSIDS];
#endif
} tSirSmeStartBssReq, *tpSirSmeStartBssReq;

typedef struct sSirBssDescription
{
    //offset of the ieFields from bssId.
    tANI_U16             length;
    tSirMacAddr     bssId;
    tANI_U32             timeStamp[2];
    tANI_U16             beaconInterval;
    tANI_U16             capabilityInfo;
    tSirNwType      nwType; // Indicates 11a/b/g
    tANI_U8              aniIndicator;
    tANI_S8              rssi;
    tANI_S8              sinr;
    //channelId what peer sent in beacon/probersp.
    tANI_U8              channelId;
    //channelId on which we are parked at.
    //used only in scan case.
    tANI_U8              channelIdSelf;
    tANI_U8              sSirBssDescriptionRsvd[3];
    //
    // FIXME - This structure is not packed!
    // Thus, the fields should be aligned at DWORD boundaries
    // Elsewhere, titanHtCaps is of type tAniTitanHtCapabilityInfo
    //
    tANI_U32             titanHtCaps;
    tANI_TIMESTAMP nReceivedTime;     //base on a tick count. It is a time stamp, not a relative time.
    tANI_U32             ieFields[1];
} tSirBssDescription, *tpSirBssDescription;

/// Definition for response message to previously
/// issued start BSS request
/// MAC --->
typedef struct sSirSmeStartBssRsp
{
    tANI_U16             messageType; // eWNI_SME_START_BSS_RSP
    tANI_U16             length;
    tSirResultCodes statusCode;
    tSirBssDescription bssDescription;
} tSirSmeStartBssRsp, *tpSirSmeStartBssRsp;

#if (WNI_POLARIS_FW_PACKAGE == ADVANCED) && (WNI_POLARIS_FW_PRODUCT == AP)
typedef struct sSirMeasControl
{
    // Periodic Measurements enabled flag
    tAniBool             periodicMeasEnabled;
    // Indicates whether to involve STAs in measurements or not
    tAniBool             involveSTAs;
    // Basic or enhanced measurement metrics
    // 0 - for basic, 1 - for enhanced
    tANI_U8                   metricsType;
    // Indicates active or passive scanning
    tSirScanType         scanType;
    // Following indicates how often measurements
    // on each channel are made for long-scan-duration
    tANI_U8                   longChannelScanPeriodicity;
    //
    // Channel Bonding plus 11H related scan control
    // 0 - CB and/or 11H is disabled
    // 1 - CB and 11H is enabled
    //
    tANI_BOOLEAN    cb11hEnabled;
} tSirMeasControl, *tpSirMeasControl;

typedef struct sSirMeasDuration
{
    // Following indicates time duration over which
    // all channels in the channelList to be measured once
    tANI_U32                  shortTermPeriod;
    // Following indicates time duration over which
    // cached DFS measurements are averaged
    tANI_U32                  averagingPeriod;
    // Following indicates time duration for making
    // DFS measurements on each channel
    tANI_U32                  shortChannelScanDuration;
    // Following indicates time duration for making
    // DFS measurements on each channel for long term measurements
    tANI_U32                  longChannelScanDuration;
} tSirMeasDuration, *tpSirMeasDuration;
#endif

typedef struct sSirChannelList
{
    tANI_U8          numChannels;
    tANI_U8          channelNumber[1];
} tSirChannelList, *tpSirChannelList;


#if (WNI_POLARIS_FW_PACKAGE == ADVANCED)
/// Definition for Neighbor BSS info
typedef struct sSirNeighborBssInfo
{
    tSirMacAddr             bssId;
    tANI_U8                 channelId;
    tAniTitanHtCapabilityInfo titanHtCaps;
    tAniBool                wniIndicator;
    tSirBssType             bssType;
    tANI_U8                 sinr;
    tANI_S8                 rssi;
    tSirLoad                load;
    tAniSSID                ssId;
    tAniApName              apName;
    tSirRSNie               rsnIE;
#if 0    
    tDot11fIEHTCaps         HTCaps;
    tDot11fIEHTInfo         HTInfo;
#endif
    tSirNwType              nwType;             // Indicates 11a/b/g
    tANI_U16                capabilityInfo;
    tSirMacRateSet          operationalRateSet; // Has 11a or 11b rates
    tSirMacRateSet          extendedRateSet;    // Has 11g rates
    tANI_U16                beaconInterval;
    tANI_U8                 dtimPeriod;
    tANI_U8                 HTCapsPresent;
    tANI_U8                 HTInfoPresent;
    tANI_U8                 wmeInfoPresent;
    tANI_U8                 wmeEdcaPresent;
    tANI_U8                 wsmCapablePresent;
    tANI_U8                 hcfEnabled;
    tANI_U16                propIECapability;
    tANI_U32                localPowerConstraints;
    tANI_S32                aggrRssi;
    tANI_U32                dataCount;
    tANI_U32                totalPackets;
} tSirNeighborBssInfo, *tpSirNeighborBssInfo;


/// Definition for Neighbor BSS with WDS info
typedef struct sSirNeighborBssWdsInfo
{
    tSirNeighborBssInfo    neighborBssInfo;
    tSirWdsInfo            wdsInfo;
} tSirNeighborBssWdsInfo, *tpSirNeighborBssWdsInfo;

/// Definition for Neighbor BSS list
typedef struct sSirNeighborBssList
{
    tANI_U32                  numBss;
    tSirNeighborBssInfo  bssList[1];
} tSirNeighborBssList, *tpSirNeighborBssList;

/// Definition for Neighbor BSS list with WDS info
typedef struct sSirNeighborBssWdsList
{
    tANI_U32                     numBssWds;
    tSirNeighborBssWdsInfo  bssWdsList[1];
} tSirNeighborBssWdsList, *tpSirNeighborBssWdsList;

#if (WNI_POLARIS_FW_PRODUCT == AP)
/// Definition for kick starting measurements
/// ---> MAC
typedef struct sSirSmeMeasurementReq
{
    tANI_U16                  messageType; // eWNI_SME_MEASUREMENT_REQ
    tANI_U16                  length;
    tSirMeasControl      measControl;
    tSirMeasDuration     measDuration;
    // This indicates how often SME_MEASUREMENT_IND message
    // is sent to host
    tANI_U32                  measIndPeriod;
    // This channel list will have current channel also
    tSirChannelList      channelList;
} tSirSmeMeasurementReq, *tpSirSmeMeasurementReq;

/// Definition for response message to previously
/// issued Measurement request
/// MAC --->
typedef struct sSirSmeMeasurementRsp
{
    tANI_U16                    messageType; // eWNI_SME_MEASUREMENT_RSP
    tANI_U16                    length;
    tSirResultCodes        statusCode;
} tSirSmeMeasurementRsp, *tpSirSmeMeasurementRsp;

/// Definition for Measurement Matrix info
// NOTE: This should include current channel measurements
typedef struct sSirMeasMatrixInfo
{
    tANI_U8          channelNumber;
    tANI_S8          compositeRssi;
    tANI_S32         aggrRssi;     // Due to all packets in this channel
    tANI_U32         totalPackets;
} tSirMeasMatrixInfo, *tpSirMeasMatrixInfo;

/// Definition for Measurement Matrix List
typedef struct sSirMeasMatrixList
{
    tANI_U8                  numChannels;
    tSirMeasMatrixInfo  measMatrixList[1];
} tSirMeasMatrixList, *tpSirMeasMatrixList;
/// Definition for Measurement indication
/// MAC --->
/// Note : This is only sent when there was prior
///        SME_MEASUREMENT_REQ received by MAC
typedef struct sSirSmeMeasurementInd
{
    tANI_U16                    messageType; // eWNI_SME_MEASUREMENT_IND
    tANI_U16                    length;
    tANI_U32               duration;
    tSirLoad               currentLoad; // on this AP radio
    tSirMeasMatrixList     measMatrixList;
    tSirNeighborBssWdsList neighborBssWdsList;
} tSirSmeMeasurementInd, *tpSirSmeMeasurementInd;

/// Definition for Backhaul Link status change Indication
/// MAC --->
typedef struct sSirSmeWdsInfoInd
{
    tANI_U16                messageType; // eWNI_SME_WDS_INFO_IND
    tANI_U16                length;
    tSirWdsInfo        wdsInfo;
} tSirSmeWdsInfoInd, *tpSirSmeWdsInfoInd;

/// Definition for Backhaul Link Info Set Request
/// ---> MAC
typedef struct sSirSmeSetWdsInfoReq
{
    tANI_U16                messageType; // eWNI_SME_SET_WDS_INFO_REQ
    tANI_U16                length;
    tSirWdsInfo        wdsInfo;
} tSirSmeSetWdsInfoReq, *tpSirSmeSetWdsInfoReq;

/// Definition for Backhaul Link Lnfo Set Response
/// MAC --->
typedef struct sSirSmeSetWdsInfoRsp
{
    tANI_U16                messageType; // eWNI_SME_SET_WDS_INFO_RSP
    tANI_U16                length;
    tSirResultCodes    statusCode;
} tSirSmeSetWdsInfoRsp, *tpSirSmeSetWdsInfoRsp;
#endif
#endif

/// Definition for Radar Info
typedef struct sSirRadarInfo
{
    tANI_U8          channelNumber;
    tANI_U16         radarPulseWidth; // in usecond
    tANI_U16         numRadarPulse;
} tSirRadarInfo, *tpSirRadarInfo;

#define SIR_RADAR_INFO_SIZE                (sizeof(tANI_U8) + 2 *sizeof(tANI_U16))

/// Two Background Scan mode
typedef enum eSirBackgroundScanMode
{
    eSIR_AGGRESSIVE_BACKGROUND_SCAN = 0,
    eSIR_NORMAL_BACKGROUND_SCAN = 1
} tSirBackgroundScanMode;

/// Two types of traffic check
typedef enum eSirLinkTrafficCheck
{
    eSIR_DONT_CHECK_LINK_TRAFFIC_BEFORE_SCAN = 0,
    eSIR_CHECK_LINK_TRAFFIC_BEFORE_SCAN = 1
} tSirLinkTrafficCheck;

#define SIR_BG_SCAN_RETURN_CACHED_RESULTS              0x0
#define SIR_BG_SCAN_PURGE_RESUTLS                      0x80
#define SIR_BG_SCAN_RETURN_FRESH_RESULTS               0x01

/// Definition for scan request
typedef struct sSirSmeScanReq
{
    tANI_U16             messageType; // eWNI_SME_SCAN_REQ
    tANI_U16             length;
    tSirBssType     bssType;
    tSirMacAddr     bssId;
    tSirMacSSid     ssId;
    tSirScanType    scanType;
    tANI_U32             minChannelTime;
    tANI_U32             maxChannelTime;
    /**
     * returnAfterFirstMatch can take following values:
     * 0x00 - Return SCAN_RSP message after complete channel scan
     * 0x01 -  Return SCAN_RSP message after collecting BSS description
     *        that matches scan criteria.
     * 0xC0 - Return after collecting first 11d IE from 2.4 GHz &
     *        5 GHz band channels
     * 0x80 - Return after collecting first 11d IE from 5 GHz band
     *        channels
     * 0x40 - Return after collecting first 11d IE from 2.4 GHz
     *        band channels
     *
     * Values of 0xC0, 0x80 & 0x40 are to be used by
     * Roaming/application when 11d is enabled.
     */
    tANI_U8              returnAfterFirstMatch;

    /**
     * returnUniqueResults can take following values:
     * 0 - Collect & report all received BSS descriptions from same BSS.
     * 1 - Collect & report unique BSS description from same BSS.
     */
    tANI_U8              returnUniqueResults;

    /**
     * returnFreshResults can take following values:
     * 0x00 - Return background scan results.
     * 0x80 - Return & purge background scan results
     * 0x01 - Trigger fresh scan instead of returning background scan
     *        results.
     * 0x81 - Trigger fresh scan instead of returning background scan
     *        results and purge background scan results.
     */
    tANI_U8              returnFreshResults;

    /*  backgroundScanMode can take following values:
     *  0x0 - agressive scan
     *  0x1 - normal scan where HAL will check for link traffic 
     *        prior to proceeding with the scan
     */
    tSirBackgroundScanMode   backgroundScanMode;

    tSirChannelList channelList;
} tSirSmeScanReq, *tpSirSmeScanReq;

/// Definition for response message to previously issued scan request
typedef struct sSirSmeScanRsp
{
    tANI_U16                messageType; // eWNI_SME_SCAN_RSP
    tANI_U16                length;
    tSirResultCodes    statusCode;
    tSirBssDescription bssDescription[1];
} tSirSmeScanRsp, *tpSirSmeScanRsp;

/// Sme Req message to set the Background Scan mode
typedef struct sSirSmeBackgroundScanModeReq
{
    tANI_U16                      messageType; // eWNI_SME_BACKGROUND_SCAN_MODE_REQ
    tANI_U16                      length;
    tSirBackgroundScanMode   mode;
} tSirSmeBackgroundScanModeReq, *tpSirSmeBackgroundScanModeReq;

/// Background Scan Statisics
typedef struct sSirBackgroundScanInfo {
    tANI_U32        numOfScanSuccess;
    tANI_U32        numOfScanFailure;
    tANI_U32        reserved;
} tSirBackgroundScanInfo, *tpSirBackgroundScanInfo;

#define SIR_BACKGROUND_SCAN_INFO_SIZE        (3 * sizeof(tANI_U32))

/// Definition for Authentication request
typedef struct sSirSmeAuthReq
{
    tANI_U16                messageType; // eWNI_SME_AUTH_REQ
    tANI_U16                length;
    tSirMacAddr        peerMacAddr;
    tAniAuthType       authType;
    tANI_U8                 channelNumber;
} tSirSmeAuthReq, *tpSirSmeAuthReq;

/// Definition for reponse message to previously issued Auth request
typedef struct sSirSmeAuthRsp
{
    tANI_U16                messageType; // eWNI_SME_AUTH_RSP
    tANI_U16                length;
    tSirMacAddr        peerMacAddr;
    tAniAuthType       authType;
    tSirResultCodes    statusCode;
    tANI_U16           protStatusCode; //It holds reasonCode when Pre-Auth fails due to deauth frame.
                                       //Otherwise it holds status code.
} tSirSmeAuthRsp, *tpSirSmeAuthRsp;

#if (WNI_POLARIS_FW_PACKAGE == ADVANCED)
/// Association type
typedef enum eSirAssocType
{
    eSIR_NORMAL,
    eSIR_TRANSFERRED,
    eSIR_DONOT_USE_ASSOC_TYPE = SIR_MAX_ENUM_SIZE
} tSirAssocType;
#endif

#if (WNI_POLARIS_FW_PACKAGE == ADVANCED) && (WNI_POLARIS_FW_PRODUCT == AP)
typedef enum eSirBpIndicatorType
{
    eSIR_WIRELESS_BP,
    eSIR_WIRED_BP
} tSirBpIndicatorType;

#endif

/// Definition for join request
/// ---> MAC
typedef struct sSirSmeJoinReq
{
    tANI_U16                 messageType; // eWNI_SME_JOIN_REQ
    tANI_U16                 length;
#if (WNI_POLARIS_FW_PACKAGE == ADVANCED)
    tSirAssocType       assocType; // Indicates whether STA is
                                   // sending (Re) Association
                                   // due to load balance or not
#endif
    tSirRSNie           rsnIE;     // RSN IE to be sent in
                                   // (Re) Association Request
#if (WNI_POLARIS_FW_PACKAGE == ADVANCED) && (WNI_POLARIS_FW_PRODUCT == AP)
    tAniBool            bpIndicator;
    tSirBpIndicatorType bpType;
    tSirNeighborBssList neighborBssList; // TBD Move this outside 'AP'
                                         // flag
#endif
    tAniTitanCBNeighborInfo cbNeighbors;

    tAniBool                spectrumMgtIndicator;
    tSirMacPowerCapInfo     powerCap;
    tSirSupChnl             supportedChannels;

	/*This contains the UAPSD Flag for all 4 AC
	 * B0: AC_VO UAPSD FLAG
	 * B1: AC_VI UAPSD FLAG
	 * B2: AC_BK UAPSD FLAG
	 * B3: AC_BE UASPD FLAG
	 */
    tANI_U8                 uapsdPerAcBitmask;
#if (WNI_POLARIS_FW_PRODUCT == WLAN_STA )
    tSirBssDescription  bssDescription;
#endif
} tSirSmeJoinReq, *tpSirSmeJoinReq;

/// Definition for reponse message to previously issued join request
/// MAC --->
typedef struct sSirSmeJoinRsp
{
    tANI_U16                messageType; // eWNI_SME_JOIN_RSP
    tANI_U16                length;
    tSirResultCodes    statusCode;
#if (WNI_POLARIS_FW_PRODUCT == WLAN_STA)
    tAniAuthType       authType;
    tANI_U16           staId;             // Station ID for peer
#endif
#if (WNI_POLARIS_FW_PACKAGE == ADVANCED)
    // Following are needed for Roaming algorithm
    // to 'associate' with an alternate BSS.
    tSirMacAddr        alternateBssId;
    tANI_U8                 alternateChannelId;
#endif
    tANI_U16        protStatusCode; //It holds reasonCode when join fails due to deauth/disassoc frame.
                                    //Otherwise it holds status code.
    tANI_U16        aid;
    tANI_U32        beaconLength;
    tANI_U32        assocReqLength;
    tANI_U32        assocRspLength;
    tANI_U8         frames[ 1 ];

} tSirSmeJoinRsp, *tpSirSmeJoinRsp;

/// Definition for Authentication indication from peer
typedef struct sSirSmeAuthInd
{
    tANI_U16                messageType; // eWNI_SME_AUTH_IND
    tANI_U16                length;
    tSirMacAddr        peerMacAddr;
    tAniAuthType       authType;
} tSirSmeAuthInd, *tpSirSmeAuthInd;

/// probereq from peer, when wsc is enabled
typedef struct sSirSmeProbereq
{
    tANI_U16           messageType; // eWNI_SME_PROBE_REQ
    tANI_U16           length;
    tSirMacAddr        peerMacAddr;
    tANI_U16           devicePasswdId;
} tSirSmeProbeReq, *tpSirSmeProbeReq;


#if (WNI_POLARIS_FW_PRODUCT == AP)
/// Definition for Association indication from peer
/// MAC --->
typedef struct sSirSmeAssocInd
{
    tANI_U16                  messageType; // eWNI_SME_ASSOC_IND
    tANI_U16                  length;
    tSirMacAddr          peerMacAddr;
    tANI_U16                  aid;
#if (WNI_POLARIS_FW_PACKAGE == ADVANCED)
    tANI_U16                  seqNum;
    tAniBool             wniIndicator;
    tAniBool             bpIndicator;
    tSirBpIndicatorType  bpType;
    tSirAssocType        assocType; // Indicates whether STA is LB'ed or not
#endif
    tAniAuthType         authType;
#if (WNI_POLARIS_FW_PACKAGE == ADVANCED)
    tSirLoad             load; // Current load on the radio for LB
    tSirNeighborBssList  neighborList; // List received from STA
    tAniSSID             ssId; // SSID used by STA to associate
    tSirRSNie            rsnIE;// RSN IE received from peer
    tANI_U16                  capabilityInfo; // STA capability
    tSirNwType           nwType;            // Indicates 11a/b/g
#endif
    tAniTitanHtCapabilityInfo titanHtCaps;
    // powerCap & supportedChannels are present only when
    // spectrumMgtIndicator flag is set
    tAniBool                spectrumMgtIndicator;
    tSirMacPowerCapInfo     powerCap;
    tSirSupChnl             supportedChannels;
    /**************** QNE updated - BEGIN **********************/
    tSirMacWscInfo       wscInfo;
    /**************** QNE updated - END   **********************/
} tSirSmeAssocInd, *tpSirSmeAssocInd;
#endif


/// Definition for Association confirm
/// ---> MAC
typedef struct sSirSmeAssocCnf
{
    tANI_U16                  messageType; // eWNI_SME_ASSOC_Cnf
    tANI_U16                  length;
    tSirMacAddr          peerMacAddr;
    tSirResultCodes      statusCode;
    tANI_U16                  aid;
    tSirMacAddr          alternateBssId;
    tANI_U8                   alternateChannelId;
} tSirSmeAssocCnf, *tpSirSmeAssocCnf;

/// Definition for Reassociation request
/// ---> MAC
typedef struct sSirSmeReassocReq
{
    tANI_U16                  messageType; // eWNI_SME_REASSOC_REQ
    tANI_U16                  length;
#if (WNI_POLARIS_FW_PACKAGE == ADVANCED)
    tSirAssocType       assocType; // Indicates whether STA is
                                   // sending (Re) Association
                                   // due to load balance or not
#endif

#ifdef WLAN_FEATURE_P2P
	tSirP2Pie		    p2pIE;
#endif

    tSirRSNie           rsnIE;     // RSN IE to be sent in
                                   // (Re) Association Request
#if (WNI_POLARIS_FW_PACKAGE == ADVANCED) && (WNI_POLARIS_FW_PRODUCT == AP)
    tAniBool             bpIndicator;
    tSirBpIndicatorType  bpType;
    tSirNeighborBssList  neighborBssList;
#endif
    tAniTitanCBNeighborInfo cbNeighbors;

    tAniBool                spectrumMgtIndicator;
    tSirMacPowerCapInfo     powerCap;
    tSirSupChnl             supportedChannels;

    /*This contains the UAPSD Flag for all 4 AC
     * B0: AC_VO UAPSD FLAG
     * B1: AC_VI UAPSD FLAG
     * B2: AC_BK UAPSD FLAG
     * B3: AC_BE UASPD FLAG
     */
    tANI_U8                 uapsdPerAcBitmask;

#if (WNI_POLARIS_FW_PRODUCT == WLAN_STA )
    tSirBssDescription  bssDescription;
#endif	
} tSirSmeReassocReq, *tpSirSmeReassocReq;

/// Definition for reponse message to previously issued
/// Reassociation request
typedef struct sSirSmeReassocRsp
{
    tANI_U16                messageType; // eWNI_SME_REASSOC_RSP
    tANI_U16                length;
    tSirResultCodes    statusCode;
#if (WNI_POLARIS_FW_PRODUCT == WLAN_STA)
    tAniAuthType       authType;
#endif
#if (WNI_POLARIS_FW_PACKAGE == ADVANCED)
    tSirMacAddr        alternateBssId;
    tANI_U8                 alternateChannelId;
#endif
} tSirSmeReassocRsp, *tpSirSmeReassocRsp;

#if (WNI_POLARIS_FW_PRODUCT == AP)
/// Definition for Reassociation indication from peer
typedef struct sSirSmeReassocInd
{
    tANI_U16                  messageType; // eWNI_SME_REASSOC_IND
    tANI_U16                  length;
    tSirMacAddr          peerMacAddr;
    tSirMacAddr          oldMacAddr;
    tANI_U16                  aid;
#if (WNI_POLARIS_FW_PACKAGE == ADVANCED)
    tANI_U16                  seqNum;
    tAniBool             wniIndicator;
    tAniBool             bpIndicator;
    tSirBpIndicatorType  bpType;
    tSirAssocType        reassocType; // Indicates whether STA is LB'ed or not
#endif
    tAniAuthType         authType;
#if (WNI_POLARIS_FW_PACKAGE == ADVANCED)
    tSirLoad             load; // Current load on the radio for LB
    tSirNeighborBssList  neighborList; // List received from STA
    tAniSSID             ssId; // SSID used by STA to reassociate
    tSirRSNie            rsnIE;// RSN IE received from peer
    tANI_U16                  capabilityInfo; // STA capability
    tSirNwType           nwType;            // Indicates 11a/b/g
#endif
    tAniTitanHtCapabilityInfo titanHtCaps;
    // powerCap & supportedChannels are present only when
    // spectrumMgtIndicator flag is set
    tAniBool                spectrumMgtIndicator;
    tSirMacPowerCapInfo     powerCap;
    tSirSupChnl             supportedChannels;
    /**************** QNE updated - BEGIN **********************/
    tSirMacWscInfo       wscInfo;
    /**************** QNE updated - END   **********************/
} tSirSmeReassocInd, *tpSirSmeReassocInd;
#endif

/// Definition for Reassociation confirm
/// ---> MAC
typedef struct sSirSmeReassocCnf
{
    tANI_U16                  messageType; // eWNI_SME_REASSOC_CNF
    tANI_U16                  length;
    tSirMacAddr          peerMacAddr;
    tSirResultCodes      statusCode;
    tANI_U16                  aid;
    tSirMacAddr          alternateBssId;
    tANI_U8                   alternateChannelId;
} tSirSmeReassocCnf, *tpSirSmeReassocCnf;

#if (WNI_POLARIS_FW_PACKAGE == ADVANCED) && (WNI_POLARIS_FW_PRODUCT == AP)
typedef enum {
    // Based on Periodic evaluation
    eSIR_PERIODIC_EVALATION,

    // Dectection of primary users such as RADARs
    eSIR_DETECTION_OF_RADAR,

    // Degradation in system performance (eg. Triggered by
    //  increase in PER beyond a certain threshold)
    eSIR_PERFORMANCE_DEGRADATION,

    // Frequency changed due to previously issued SWITCH_CHANNEL_REQ
    eSIR_UPPER_LAYER_TRIGGERED
}tSirFreqChangeReason;
#endif

/// Enum definition for  Wireless medium status change codes
typedef enum eSirSmeStatusChangeCode
{
    eSIR_SME_DEAUTH_FROM_PEER,
    eSIR_SME_DISASSOC_FROM_PEER,
    eSIR_SME_LOST_LINK_WITH_PEER,
    eSIR_SME_CHANNEL_SWITCH,
    eSIR_SME_JOINED_NEW_BSS,
    eSIR_SME_LEAVING_BSS,
    eSIR_SME_IBSS_ACTIVE,
    eSIR_SME_IBSS_INACTIVE,
    eSIR_SME_IBSS_PEER_DEPARTED,
    eSIR_SME_RADAR_DETECTED,
    eSIR_SME_IBSS_NEW_PEER,
    eSIR_SME_AP_CAPS_CHANGED,
    eSIR_SME_BACKGROUND_SCAN_FAIL,
    eSIR_SME_CB_LEGACY_BSS_FOUND_BY_AP,
    eSIR_SME_CB_LEGACY_BSS_FOUND_BY_STA
} tSirSmeStatusChangeCode;

typedef struct sSirSmeNewBssInfo
{
    tSirMacAddr   bssId;
    tANI_U8            channelNumber;
    tANI_U8            reserved;
    tSirMacSSid   ssId;
} tSirSmeNewBssInfo, *tpSirSmeNewBssInfo;

typedef struct sSirSmeApNewCaps
{
    tANI_U16           capabilityInfo;
    tSirMacAddr   bssId;
    tANI_U8            channelId;
    tANI_U8            reserved[3];
    tSirMacSSid   ssId;
} tSirSmeApNewCaps, *tpSirSmeApNewCaps;

/**
 * Table below indicates what information is passed for each of
 * the Wireless Media status change notifications:
 *
 * Status Change code           Status change info
 * ----------------------------------------------------------------------
 * eSIR_SME_DEAUTH_FROM_PEER        Reason code received in DEAUTH frame
 * eSIR_SME_DISASSOC_FROM_PEER      Reason code received in DISASSOC frame
 * eSIR_SME_LOST_LINK_WITH_PEER     None
 * eSIR_SME_CHANNEL_SWITCH          New channel number
 * eSIR_SME_JOINED_NEW_BSS          BSSID, SSID and channel number
 * eSIR_SME_LEAVING_BSS             None
 * eSIR_SME_IBSS_ACTIVE             Indicates that another STA joined
 *                                  IBSS apart from this STA that
 *                                  started IBSS
 * eSIR_SME_IBSS_INACTIVE           Indicates that only this STA is left
 *                                  in IBSS
 * eSIR_SME_RADAR_DETECTED          Indicates that radar is detected
 * eSIR_SME_IBSS_NEW_PEER           Indicates that a new peer is detected
 * eSIR_SME_AP_CAPS_CHANGED         Indicates that capabilities of the AP
 *                                  that STA is currently associated with
 *                                  have changed.
 * eSIR_SME_BACKGROUND_SCAN_FAIL    Indicates background scan failure
 */

/// Definition for Wireless medium status change notification
typedef struct sSirSmeWmStatusChangeNtf
{
    tANI_U16                     messageType; // eWNI_SME_WM_STATUS_CHANGE_NTF
    tANI_U16                     length;
    tSirSmeStatusChangeCode statusChangeCode;
    union
    {
        tANI_U16                 deAuthReasonCode; // eSIR_SME_DEAUTH_FROM_PEER
        tANI_U16                 disassocReasonCode; // eSIR_SME_DISASSOC_FROM_PEER
        // none for eSIR_SME_LOST_LINK_WITH_PEER
        tANI_U8                  newChannelId;   // eSIR_SME_CHANNEL_SWITCH
        tSirSmeNewBssInfo   newBssInfo;     // eSIR_SME_JOINED_NEW_BSS
        // none for eSIR_SME_LEAVING_BSS
        // none for eSIR_SME_IBSS_ACTIVE
        // none for eSIR_SME_IBSS_INACTIVE
#if (WNI_POLARIS_FW_PACKAGE == ADVANCED) && (WNI_POLARIS_FW_PRODUCT == AP)
        tSirRadarInfo          radarInfo;         // eSIR_SME_RADAR_DETECTED
#endif
        tSirNewIbssPeerInfo     newIbssPeerInfo;  // eSIR_SME_IBSS_NEW_PEER
        tSirSmeApNewCaps        apNewCaps;        // eSIR_SME_AP_CAPS_CHANGED
        tSirBackgroundScanInfo  bkgndScanInfo;    // eSIR_SME_BACKGROUND_SCAN_FAIL
        tAniTitanCBNeighborInfo cbNeighbors;      // eSIR_SME_CB_LEGACY_BSS_FOUND_BY_STA
#if (WNI_POLARIS_FW_PACKAGE == ADVANCED)
        tSirNeighborBssWdsInfo  neighborWdsInfo;  // eSIR_SME_CB_LEGACY_BSS_FOUND_BY_AP
#endif
    } statusChangeInfo;
} tSirSmeWmStatusChangeNtf, *tpSirSmeWmStatusChangeNtf;

/// Definition for Disassociation request
typedef struct sSirSmeDisassocReq
{
    tANI_U16         messageType; // eWNI_SME_DISASSOC_REQ
    tANI_U16         length;
    tSirMacAddr peerMacAddr;
    tANI_U16         reasonCode;
#if (WNI_POLARIS_FW_PRODUCT == AP)
    tANI_U16         aid;
#if (WNI_POLARIS_FW_PACKAGE == ADVANCED)
    tANI_U16         seqNum;
#endif
#endif
} tSirSmeDisassocReq, *tpSirSmeDisassocReq;

typedef struct sAni64BitCounters
{
    tANI_U32 Hi;
    tANI_U32 Lo;
}tAni64BitCounters, *tpAni64BitCounters;

typedef struct sAniSecurityStat
{
    tAni64BitCounters txBlks;
    tAni64BitCounters rxBlks;
    tAni64BitCounters formatErrorCnt;
    tAni64BitCounters decryptErr;
    tAni64BitCounters protExclCnt;
    tAni64BitCounters unDecryptableCnt;
    tAni64BitCounters decryptOkCnt;

}tAniSecurityStat, *tpAniSecurityStat;

typedef struct sAniTxRxCounters
{
    tANI_U32 txFrames; // Incremented for every packet tx
    tANI_U32 rxFrames;    
    tANI_U32 nRcvBytes;
    tANI_U32 nXmitBytes;
}tAniTxRxCounters, *tpAniTxRxCounters;

typedef struct sAniTxRxStats
{
    tAni64BitCounters txFrames;
    tAni64BitCounters rxFrames;
    tAni64BitCounters nRcvBytes;
    tAni64BitCounters nXmitBytes;

}tAniTxRxStats,*tpAniTxRxStats;

typedef struct sAniSecStats
{
    tAniSecurityStat aes;
    tAni64BitCounters aesReplays;
    tAniSecurityStat tkip;
    tAni64BitCounters tkipReplays;
    tAni64BitCounters tkipMicError;

    tAniSecurityStat wep;

}tAniSecStats, *tpAniSecStats;    

#define SIR_MAX_RX_CHAINS 3

typedef struct sAniStaStatStruct
{
    /* following statistic elements till expandPktRxCntLo are not filled with valid data.
     * These are kept as it is, since WSM is using this structure.
     * These elements can be removed whenever WSM is updated.
     * Phystats is used to hold phystats from BD.
     */
    tANI_U32 sentAesBlksUcastHi;
    tANI_U32 sentAesBlksUcastLo;
    tANI_U32 recvAesBlksUcastHi;
    tANI_U32 recvAesBlksUcastLo;
    tANI_U32 aesFormatErrorUcastCnts;
    tANI_U32 aesReplaysUcast;
    tANI_U32 aesDecryptErrUcast;
    tANI_U32 singleRetryPkts;
    tANI_U32 failedTxPkts;
    tANI_U32 ackTimeouts;
    tANI_U32 multiRetryPkts;
    tANI_U32 fragTxCntsHi;
    tANI_U32 fragTxCntsLo;
    tANI_U32 transmittedPktsHi;
    tANI_U32 transmittedPktsLo;
    tANI_U32 phyStatHi; //These are used to fill in the phystats.
    tANI_U32 phyStatLo; //This is only for private use.

    tANI_U32 uplinkRssi;
    tANI_U32 uplinkSinr;
    tANI_U32 uplinkRate;
    tANI_U32 downlinkRssi;
    tANI_U32 downlinkSinr;
    tANI_U32 downlinkRate;
    tANI_U32 nRcvBytes;
    tANI_U32 nXmitBytes;

    // titan 3c stats
    tANI_U32 chunksTxCntHi;          // Number of Chunks Transmitted
    tANI_U32 chunksTxCntLo;
    tANI_U32 compPktRxCntHi;         // Number of Packets Received that were actually compressed
    tANI_U32 compPktRxCntLo;
    tANI_U32 expanPktRxCntHi;        // Number of Packets Received that got expanded
    tANI_U32 expanPktRxCntLo;


    /* Following elements are valid and filled in correctly. They have valid values.
     */

    //Unicast frames and bytes.
    tAniTxRxStats ucStats;

    //Broadcast frames and bytes.
    tAniTxRxStats bcStats;

    //Multicast frames and bytes.
    tAniTxRxStats mcStats;

    tANI_U32      currentTxRate; 
    tANI_U32      currentRxRate; //Rate in 100Kbps

    tANI_U32      maxTxRate;
    tANI_U32      maxRxRate;

    tANI_S8       rssi[SIR_MAX_RX_CHAINS]; 


    tAniSecStats   securityStats;

    tANI_U8       currentRxRateIdx; //This the softmac rate Index.
    tANI_U8       currentTxRateIdx;

} tAniStaStatStruct, *tpAniStaStatStruct;

//Statistics that are not maintained per stations.
typedef struct sAniGlobalStatStruct
{
  tAni64BitCounters txError;
  tAni64BitCounters rxError;
  tAni64BitCounters rxDropNoBuffer;
  tAni64BitCounters rxDropDup;
  tAni64BitCounters rxCRCError;

  tAni64BitCounters singleRetryPkts;
  tAni64BitCounters failedTxPkts;
  tAni64BitCounters ackTimeouts;
  tAni64BitCounters multiRetryPkts;
  tAni64BitCounters fragTxCnts;
  tAni64BitCounters fragRxCnts;

  tAni64BitCounters txRTSSuccess;
  tAni64BitCounters txCTSSuccess;
  tAni64BitCounters rxRTSSuccess;
  tAni64BitCounters rxCTSSuccess;

  tAniSecStats      securityStats;

  tAniTxRxStats     mcStats;
  tAniTxRxStats     bcStats;
  	
}tAniGlobalStatStruct,*tpAniGlobalStatStruct;

typedef enum sPacketType
{
    ePACKET_TYPE_UNKNOWN,
    ePACKET_TYPE_11A,
    ePACKET_TYPE_11G,
    ePACKET_TYPE_11B,
    ePACKET_TYPE_11N

}tPacketType, *tpPacketType;

typedef struct sAniStatSummaryStruct
{
    tAniTxRxStats uc; //Unicast counters.
    tAniTxRxStats bc; //Broadcast counters.
    tAniTxRxStats mc; //Multicast counters.
    tAni64BitCounters txError;
    tAni64BitCounters rxError;
    tANI_S8     rssi[SIR_MAX_RX_CHAINS]; //For each chain.
    tANI_U32    rxRate; // Rx rate of the last recieved packet.
    tANI_U32    txRate;
    tANI_U16    rxMCSId; //MCS index is valid only when packet type is ePACKET_TYPE_11N
    tANI_U16    txMCSId;
    tPacketType rxPacketType;
    tPacketType txPacketType;
    tSirMacAddr macAddr; //Mac Address of the station from which above RSSI and rate is from.
}tAniStatSummaryStruct,*tpAniStatSummaryStruct;

#if (WNI_POLARIS_FW_PRODUCT == WLAN_STA)
//structure for stats that may be reset, like the ones in sta descriptor
//The stats are saved into here before reset. It should be tANI_U32 aligned.
typedef struct _sPermStaStats
{
    //tANI_U32 sentAesBlksUcastHi;
    //tANI_U32 sentAesBlksUcastLo;
    //tANI_U32 recvAesBlksUcastHi;
    //tANI_U32 recvAesBlksUcastLo;
    tANI_U32 aesFormatErrorUcastCnts;
    tANI_U32 aesReplaysUcast;
    tANI_U32 aesDecryptErrUcast;
    tANI_U32 singleRetryPkts;
    tANI_U32 failedTxPkts;
    tANI_U32 ackTimeouts;
    tANI_U32 multiRetryPkts;
    tANI_U32 fragTxCntsHi;
    tANI_U32 fragTxCntsLo;
    tANI_U32 transmittedPktsHi;
    tANI_U32 transmittedPktsLo;

    // titan 3c stats
    tANI_U32 chunksTxCntHi;          // Number of Chunks Transmitted
    tANI_U32 chunksTxCntLo;
    tANI_U32 compPktRxCntHi;         // Number of Packets Received that were actually compressed
    tANI_U32 compPktRxCntLo;
    tANI_U32 expanPktRxCntHi;        // Number of Packets Received that got expanded
    tANI_U32 expanPktRxCntLo;
}tPermanentStaStats;

#endif//#if (WNI_POLARIS_FW_PRODUCT == WLAN_STA)



/// Definition for Disassociation response
typedef struct sSirSmeDisassocRsp
{
    tANI_U16                messageType; // eWNI_SME_DISASSOC_RSP
    tANI_U16                length;
    tSirMacAddr        peerMacAddr;
    tSirResultCodes    statusCode;
#if (WNI_POLARIS_FW_PRODUCT == AP)
    tANI_U16                aid;
#endif
    tAniStaStatStruct  perStaStats; // STA stats
} tSirSmeDisassocRsp, *tpSirSmeDisassocRsp;

/// Definition for Disassociation indication from peer
typedef struct sSirSmeDisassocInd
{
    tANI_U16                messageType; // eWNI_SME_DISASSOC_IND
    tANI_U16                length;
    tSirResultCodes    statusCode;
    tSirMacAddr        peerMacAddr;
#if (WNI_POLARIS_FW_PRODUCT == AP)
    tANI_U16                aid;
#endif
    tAniStaStatStruct  perStaStats; // STA stats
} tSirSmeDisassocInd, *tpSirSmeDisassocInd;

/// Definition for Disassociation confirm
/// MAC --->
typedef struct sSirSmeDisassocCnf
{
    tANI_U16                messageType; // eWNI_SME_DISASSOC_CNF
    tANI_U16                length;
    tSirMacAddr        peerMacAddr;
    tSirResultCodes    statusCode;
#if (WNI_POLARIS_FW_PRODUCT == AP)
    tANI_U16                aid;
#endif
} tSirSmeDisassocCnf, *tpSirSmeDisassocCnf;

/// Definition for Deauthetication request
typedef struct sSirSmeDeauthReq
{
    tANI_U16         messageType; // eWNI_SME_DEAUTH_REQ
    tANI_U16         length;
    tSirMacAddr peerMacAddr;
    tANI_U16         reasonCode;
#if (WNI_POLARIS_FW_PRODUCT == AP)
    tANI_U16         aid;
#endif
} tSirSmeDeauthReq, *tpSirSmeDeauthReq;

/// Definition for Deauthetication response
typedef struct sSirSmeDeauthRsp
{
    tANI_U16                messageType; // eWNI_SME_DEAUTH_RSP
    tANI_U16                length;
    tSirMacAddr        peerMacAddr;
    tSirResultCodes    statusCode;
#if (WNI_POLARIS_FW_PRODUCT == AP)
    tANI_U16                aid;
#endif
} tSirSmeDeauthRsp, *tpSirSmeDeauthRsp;

/// Definition for Deauthetication indication from peer
typedef struct sSirSmeDeauthInd
{
    tANI_U16                messageType; // eWNI_SME_DEAUTH_IND
    tANI_U16                length;
    tSirResultCodes    statusCode;
    tSirMacAddr        peerMacAddr;
#if (WNI_POLARIS_FW_PRODUCT == AP)
    tANI_U16                aid;
#endif
} tSirSmeDeauthInd, *tpSirSmeDeauthInd;

/// Definition for Deauthentication confirm
/// MAC --->
typedef struct sSirSmeDeauthCnf
{
    tANI_U16                messageType; // eWNI_SME_DEAUTH_CNF
    tANI_U16                length;
    tSirMacAddr        peerMacAddr;
    tSirResultCodes    statusCode;
#if (WNI_POLARIS_FW_PRODUCT == AP)
    tANI_U16                aid;
#endif
} tSirSmeDeauthCnf, *tpSirSmeDeauthCnf;

/// Definition for stop BSS request message
typedef struct sSirSmeStopBssReq
{
    tANI_U16                messageType; // eWNI_SME_STOP_BSS_REQ
    tANI_U16                length;
    tSirResultCodes    reasonCode;
} tSirSmeStopBssReq, *tpSirSmeStopBssReq;

/// Definition for stop BSS response message
typedef struct sSirSmeStopBssRsp
{
    tANI_U16             messageType; // eWNI_SME_STOP_BSS_RSP
    tANI_U16             length;
    tSirResultCodes statusCode;
} tSirSmeStopBssRsp, *tpSirSmeStopBssRsp;

#if (WNI_POLARIS_FW_PACKAGE == ADVANCED) && (WNI_POLARIS_FW_PRODUCT == AP)
/// Definition for Channel Select request
/// ---> MAC
typedef struct sSirSmeSelectChannelReq
{
    tANI_U16                messageType; // eWNI_SME_SELECT_CHL_REQ
    tANI_U16                length;
    tANI_U8                 channelId;
} tSirSmeSelectChannelReq, *tpSirSmeSelectChannelReq;

/// Definition for Channel Select response
/// MAC --->
typedef struct sSirSmeSelectChannelRsp
{
    tANI_U16                messageType; // eWNI_SME_SELECT_CHL_RSP
    tANI_U16                length;
    tSirResultCodes    statusCode;
} tSirSmeSelectChannelRsp, *tpSirSmeSelectChannelRsp;

/// Definition for Channel Switch request
/// ---> MAC
typedef struct sSirSmeSwitchChannelReq
{
    // eWNI_SME_SWITCH_CHL_REQ,
    // eWNI_SME_SWITCH_CHL_CB_PRIMARY_REQ,
    // eWNI_SME_SWITCH_CHL_CB_SECONDARY_REQ
    tANI_U16                messageType;

    tANI_U16                length;
    tANI_U8                 channelId;
    //
    // The cbMode field is applicable to TITAN only.
    // This indicates as to how the CB secondary
    // channel will be used (if at all).
    //
    // In a non-CB environment, with 11H enabled,
    // this field will be ignored
    //
    tAniCBSecondaryMode cbMode;

    // dtimFactor indicates the number of DTIM
    // Beacon before LIM switches channel
    tANI_U32                dtimFactor;
} tSirSmeSwitchChannelReq, *tpSirSmeSwitchChannelReq;

/// Definition for Channel Switch response
/// MAC --->
typedef struct sSirSmeSwitchChannelRsp
{
    tANI_U16                messageType; // eWNI_SME_SWITCH_CHL_RSP
    tANI_U16                length;
    tSirResultCodes    statusCode;
} tSirSmeSwitchChannelRsp, *tpSirSmeSwitchChannelRsp;

#endif

/// Definition for ULA complete indication message
typedef struct sirUlaCompleteInd
{
    tANI_U16                messageType; // eWNI_ULA_COMPLETE_IND
    tANI_U16                length;
    tSirResultCodes    statusCode;
    tSirMacAddr        peerMacAddr;
#if (WNI_POLARIS_FW_PRODUCT == AP)
    tANI_U16                aid;
#endif
} tSirUlaCompleteInd, *tpSirUlaCompleteInd;

/// Definition for ULA complete confirmation message
typedef struct sirUlaCompleteCnf
{
    tANI_U16                messageType; // eWNI_ULA_COMPLETE_CNF
    tANI_U16                length;
    tSirResultCodes    statusCode;
    tSirMacAddr        peerMacAddr;
#if (WNI_POLARIS_FW_PRODUCT == AP)
    tANI_U16                aid;
#endif
} tSirUlaCompleteCnf, *tpSirUlaCompleteCnf;

/// Definition for Neighbor BSS indication
/// MAC --->
/// MAC reports this each time a new I/BSS is detected
typedef struct sSirSmeNeighborBssInd
{
    tANI_U16                    messageType; // eWNI_SME_NEIGHBOR_BSS_IND
    tANI_U16                    length;
#if (WNI_POLARIS_FW_PACKAGE == ADVANCED) && (WNI_POLARIS_FW_PRODUCT == AP)
    tSirNeighborBssInfo    neighborInfo;
    tSirWdsInfo            wdsInfo;
#else
    tSirBssDescription     bssDescription[1];
#endif
} tSirSmeNeighborBssInd, *tpSirSmeNeighborBssInd;

/// Definition for MIC failure indication
/// MAC --->
/// MAC reports this each time a MIC failure occures on Rx TKIP packet
typedef struct sSirSmeMicFailureInd
{
    tANI_U16                    messageType; // eWNI_SME_MIC_FAILURE_IND
    tANI_U16                    length;
    tSirMicFailureInfo     info;
} tSirSmeMicFailureInd, *tpSirSmeMicFailureInd;


/// Definition for Set Context request
/// ---> MAC
typedef struct sSirSmeSetContextReq
{
    tANI_U16                messageType; // eWNI_SME_SET_CONTEXT_REQ
    tANI_U16                length;
    tSirMacAddr        peerMacAddr;
#if (WNI_POLARIS_FW_PRODUCT == AP)
    tANI_U16                aid;
#endif
    // TBD Following QOS fields to be uncommented
    //tAniBool           qosInfoPresent;
    //tSirQos            qos;
    tSirKeyMaterial    keyMaterial;
} tSirSmeSetContextReq, *tpSirSmeSetContextReq;

/// Definition for Set Context response
/// MAC --->
typedef struct sSirSmeSetContextRsp
{
    tANI_U16                messageType; // eWNI_SME_SET_CONTEXT_RSP
    tANI_U16                length;
    tSirMacAddr        peerMacAddr;
    tSirResultCodes    statusCode;
#if (WNI_POLARIS_FW_PRODUCT == AP)
    tANI_U16                aid;
#endif
} tSirSmeSetContextRsp, *tpSirSmeSetContextRsp;

/// Definition for Remove Key Context request
/// ---> MAC
typedef struct sSirSmeRemoveKeyReq
{
    tANI_U16                messageType; // eWNI_SME_REMOVE_KEY_REQ
    tANI_U16                length;
    tSirMacAddr        peerMacAddr;
#if (WNI_POLARIS_FW_PRODUCT == AP)
    tANI_U16                aid;
#endif
    tANI_U8    edType;
    tANI_U8    wepType;
    tANI_U8    keyId;
    tANI_BOOLEAN unicast;
} tSirSmeRemoveKeyReq, *tpSirSmeRemoveKeyReq;

/// Definition for Remove Key Context response
/// MAC --->
typedef struct sSirSmeRemoveKeyRsp
{
    tANI_U16                messageType; // eWNI_SME_REMOVE_KEY_RSP
    tANI_U16                length;
    tSirMacAddr        peerMacAddr;
    tSirResultCodes    statusCode;
#if (WNI_POLARIS_FW_PRODUCT == AP)
    tANI_U16                aid;
#endif
} tSirSmeRemoveKeyRsp, *tpSirSmeRemoveKeyRsp;

/// Definition for Set Power request
/// ---> MAC
typedef struct sSirSmeSetPowerReq
{
    tANI_U16                messageType; // eWNI_SME_SET_POWER_REQ
    tANI_U16                length;
    tANI_S8                 powerLevel;
} tSirSmeSetPowerReq, *tpSirSmeSetPowerReq;

/// Definition for Set Power response
/// MAC --->
typedef struct sSirSmeSetPowerRsp
{
    tANI_U16                messageType; // eWNI_SME_SET_POWER_RSP
    tANI_U16                length;
    tSirResultCodes    statusCode;
} tSirSmeSetPowerRsp, *tpSirSmeSetPowerRsp;

#if (WNI_POLARIS_FW_PACKAGE == ADVANCED) && (WNI_POLARIS_FW_PRODUCT == AP)
/// Definition for Client Side Load Balancing request
/// ---> MAC
typedef struct sSirSmeSetClientLoadBalanceReq
{
    tANI_U16                messageType; // eWNI_SME_CLIENT_LOAD_BALANCE_REQ
    tANI_U16                length;
    tSirMacAddr        alternateBssId;
    tANI_U8                 alternateChannelId;
    tANI_U8                 numberStas;
} tSirSmeClientLoadBalanceReq, *tpSirSmeClientLoadBalanceReq;

/// Definition for Client Side Load Balancing response
/// MAC --->
typedef struct sSirSmeSetClientLoadBalanceRsp
{
    tANI_U16                messageType; // eWNI_SME_CLIENT_LOAD_BALANCE_RSP
    tANI_U16                length;
    tSirResultCodes    statusCode;
} tSirSmeClientLoadBalanceRsp, *tpSirSmeClientLoadBalanceRsp;
#endif

/// Definition for Link Test Start response
/// MAC --->
typedef struct sSirSmeLinkTestStartRsp
{
    tANI_U16                messageType; // eWNI_SME_LINK_TEST_START_RSP
    tANI_U16                length;
    tSirMacAddr        peerMacAddr;
    tSirResultCodes    statusCode;
#if (WNI_POLARIS_FW_PRODUCT == AP)
    tANI_U16                aid;
#endif
} tSirSmeLinkTestStartRsp, *tpSirSmeLinkTestStartRsp;

/// Definition for Link Test Stop response
/// WSM ---> MAC
typedef struct sSirSmeLinkTestStopRsp
{
    tANI_U16                messageType; // eWNI_SME_LINK_TEST_STOP_RSP
    tANI_U16                length;
    tSirMacAddr        peerMacAddr;
    tSirResultCodes    statusCode;
#if (WNI_POLARIS_FW_PRODUCT == AP)
    tANI_U16                aid;
#endif
} tSirSmeLinkTestStopRsp, *tpSirSmeLinkTestStopRsp;

/// Definition for kick starting DFS measurements
typedef struct sSirSmeDFSreq
{
    tANI_U16             messageType; // eWNI_SME_DFS_REQ
    tANI_U16             length;
} tSirSmeDFSrequest, *tpSirSmeDFSrequest;

/// Definition for response message to previously
/// issued DFS request
typedef struct sSirSmeDFSrsp
{
    tANI_U16             messageType; // eWNI_SME_DFS_RSP
    tANI_U16             length;
    tSirResultCodes statusCode;
    tANI_U32             dfsReport[1];
} tSirSmeDFSrsp, *tpSirSmeDFSrsp;

/// Statistic definitions
//=============================================================
// Per STA statistic structure; This same struct will be used for Aggregate
// STA stats as well.

// Clear radio stats and clear per sta stats
typedef enum
{
    eANI_CLEAR_ALL_STATS, // Clears all stats
    eANI_CLEAR_RX_STATS,  // Clears RX statistics of the radio interface
    eANI_CLEAR_TX_STATS,  // Clears TX statistics of the radio interface
    eANI_CLEAR_RADIO_STATS,   // Clears all the radio stats
    eANI_CLEAR_PER_STA_STATS, // Clears Per STA stats
    eANI_CLEAR_AGGR_PER_STA_STATS, // Clears aggregate stats

    // Used to distinguish between per sta to security stats.
    // Used only by AP, FW just returns the same parameter as it received.
    eANI_LINK_STATS,     // Get Per STA stats
    eANI_SECURITY_STATS, // Get Per STA security stats

    eANI_CLEAR_STAT_TYPES_END
} tAniStatSubTypes;

typedef struct sAniTxCtrs
{
    // add the rate counters here
    tANI_U32 tx1Mbps;
    tANI_U32 tx2Mbps;
    tANI_U32 tx5_5Mbps;
    tANI_U32 tx6Mbps;
    tANI_U32 tx9Mbps;
    tANI_U32 tx11Mbps;
    tANI_U32 tx12Mbps;
    tANI_U32 tx18Mbps;
    tANI_U32 tx24Mbps;
    tANI_U32 tx36Mbps;
    tANI_U32 tx48Mbps;
    tANI_U32 tx54Mbps;
    tANI_U32 tx72Mbps;
    tANI_U32 tx96Mbps;
    tANI_U32 tx108Mbps;

    // tx path radio counts
    tANI_U32 txFragHi;
    tANI_U32 txFragLo;
    tANI_U32 txFrameHi;
    tANI_U32 txFrameLo;
    tANI_U32 txMulticastFrameHi;
    tANI_U32 txMulticastFrameLo;
    tANI_U32 txFailedHi;
    tANI_U32 txFailedLo;
    tANI_U32 multipleRetryHi;
    tANI_U32 multipleRetryLo;
    tANI_U32 singleRetryHi;
    tANI_U32 singleRetryLo;
    tANI_U32 ackFailureHi;
    tANI_U32 ackFailureLo;
    tANI_U32 xmitBeacons;

    // titan 3c stats
    tANI_U32 txCbEscPktCntHi;            // Total Number of Channel Bonded/Escort Packet Transmitted
    tANI_U32 txCbEscPktCntLo;
    tANI_U32 txChunksCntHi;              // Total Number of Chunks Transmitted
    tANI_U32 txChunksCntLo;
    tANI_U32 txCompPktCntHi;             // Total Number of Compresssed Packet Transmitted
    tANI_U32 txCompPktCntLo;
    tANI_U32 tx50PerCompPktCntHi;        // Total Number of Packets with 50% or more compression
    tANI_U32 tx50PerCompPktCntLo;
    tANI_U32 txExpanPktCntHi;            // Total Number of Packets Transmitted that got expanded
    tANI_U32 txExpanPktCntLo;
} tAniTxCtrs, *tpAniTxCtrs;

typedef struct sAniRxCtrs
{
    // receive frame rate counters
    tANI_U32 rx1Mbps;
    tANI_U32 rx2Mbps;
    tANI_U32 rx5_5Mbps;
    tANI_U32 rx6Mbps;
    tANI_U32 rx9Mbps;
    tANI_U32 rx11Mbps;
    tANI_U32 rx12Mbps;
    tANI_U32 rx18Mbps;
    tANI_U32 rx24Mbps;
    tANI_U32 rx36Mbps;
    tANI_U32 rx48Mbps;
    tANI_U32 rx54Mbps;
    tANI_U32 rx72Mbps;
    tANI_U32 rx96Mbps;
    tANI_U32 rx108Mbps;

    // receive size counters; 'Lte' = Less than or equal to
    tANI_U32 rxLte64;
    tANI_U32 rxLte128Gt64;
    tANI_U32 rxLte256Gt128;
    tANI_U32 rxLte512Gt256;
    tANI_U32 rxLte1kGt512;
    tANI_U32 rxLte1518Gt1k;
    tANI_U32 rxLte2kGt1518;
    tANI_U32 rxLte4kGt2k;

    // rx radio stats
    tANI_U32 rxFrag;
    tANI_U32 rxFrame;
    tANI_U32 fcsError;
    tANI_U32 rxMulticast;
    tANI_U32 duplicate;
    tANI_U32 rtsSuccess;
    tANI_U32 rtsFailed;
    tANI_U32 wepUndecryptables;
    tANI_U32 drops;
    tANI_U32 aesFormatErrorUcastCnts;
    tANI_U32 aesReplaysUcast;
    tANI_U32 aesDecryptErrUcast;

    // titan 3c stats
    tANI_U32 rxDecompPktCntHi;           // Total Number of Packets that got decompressed
    tANI_U32 rxDecompPktCntLo;
    tANI_U32 rxCompPktCntHi;             // Total Number of Packets received that were actually compressed
    tANI_U32 rxCompPktCntLo;
    tANI_U32 rxExpanPktCntHi;            // Total Number of Packets received that got expanded
    tANI_U32 rxExpanPktCntLo;
} tAniRxCtrs, *tpAniRxCtrs;

// Radio stats
typedef struct sAniRadioStats
{
    tAniTxCtrs tx;
    tAniRxCtrs rx;
} tAniRadioStats, *tpAniRadioStats;

// Get Radio Stats request structure
// This structure shall be used for both Radio stats and Aggregate stats
// A valid request must contain entire structure with/without valid fields.
// Based on the request type, the valid fields will be checked.
typedef struct sAniGetStatsReq
{
    // Common for all types are requests
    tANI_U16                msgType; // message type is same as the request type
    tANI_U16                msgLen;  // length of the entire request
    tANI_U32                transactionId;
    // only used for clear stats and per sta stats clear
    tAniStatSubTypes   stat;   // Clears the stats of the described types.
    tANI_U32                staId;  // Per STA stats request must contain valid
                               // values
    tANI_U8                 macAddr[6];
} tAniGetStatsReq, *tpAniGetStatsReq;

// Get Radio Stats response struct
typedef struct sAniGetRadioStatsRsp
{
    tANI_U16            type;   // message type is same as the request type
    tANI_U16            msgLen; // length of the entire request
    tANI_U32            rc;
    tANI_U32            transactionId;
    tAniRadioStats radio;
} tAniGetRadioStatsRsp, *tpAniGetRadioStatsRsp;

// Per Sta stats response struct
typedef struct sAniGetPerStaStatsRsp
{
    tANI_U16               type;   // message type is same as the request type
    tANI_U16               msgLen; // length of the entire request
    tANI_U32               rc;
    tANI_U32               transactionId;
    tAniStatSubTypes  stat;   // Sub type needed by AP. Returns the same value
    tAniStaStatStruct sta;
    tANI_U32               staId;
    tANI_U8                macAddr[6];
} tAniGetPerStaStatsRsp, *tpAniGetPerStaStatsRsp;

// Get Aggregate stats
typedef struct sAniGetAggrStaStatsRsp
{
    tANI_U16               type;   // message type is same as the request type
    tANI_U16               msgLen; // length of the entire request
    tANI_U32               rc;
    tANI_U32               transactionId;
    tAniStaStatStruct sta;
} tAniGetAggrStaStatsRsp, *tpAniGetAggrStaStatsRsp;

// Clear stats request and response structure. 'rc' field is unused in
// request and this field is used in response field.
typedef struct sAniClearStatsRsp
{
    tANI_U16                type;   // message type is same as the request type
    tANI_U16                msgLen; // length of the entire request
    tANI_U32                rc;     // return code - will be filled by FW on
                               // response.
                       // Same transaction ID will be returned by the FW
    tANI_U32                transactionId;
    tAniStatSubTypes   stat;       // Clears the stats of the described types.
    tANI_U32                staId;      // Applicable only to PER STA stats clearing
    tANI_U8                 macAddr[6]; // Applicable only to PER STA stats clearing
} tAniClearStatsRsp, *tpAniClearStatsRsp;

typedef struct sAniGetGlobalStatsRsp
{
    tANI_U16            type;   // message type is same as the request type
    tANI_U16            msgLen; // length of the entire request
    tANI_U32            rc;
    tANI_U32            transactionId;
    tAniGlobalStatStruct global;
} tAniGetGlobalStatsRsp, *tpAniGetGlobalStatsRsp;

typedef struct sAniGetStatSummaryRsp
{
    tANI_U16               type;   // message type is same as the request type
    tANI_U16               msgLen; // length of the entire request --Why?
    tANI_U32               rc;
    tANI_U32               transactionId;
    tAniStatSummaryStruct stat;
} tAniGetStatSummaryRsp, *tpAniGetStatSummaryRsp;

//***************************************************************


/*******************PE Statistics*************************/
typedef enum
{
    PE_SUMMARY_STATS_INFO           = 0x00000001,
    PE_GLOBAL_CLASS_A_STATS_INFO    = 0x00000002,
    PE_GLOBAL_CLASS_B_STATS_INFO    = 0x00000004,
    PE_GLOBAL_CLASS_C_STATS_INFO    = 0x00000008,
    PE_GLOBAL_CLASS_D_STATS_INFO    = 0x00000010,
    PE_PER_STA_STATS_INFO           = 0x00000020
}ePEStatsMask;

/*
 * tpAniGetPEStatsReq is tied to 
 * for SME ==> PE eWNI_SME_GET_STATISTICS_REQ msgId  and 
 * for PE ==> HAL SIR_HAL_GET_STATISTICS_REQ msgId
 */
typedef struct sAniGetPEStatsReq
{
    // Common for all types are requests
    tANI_U16                msgType;    // message type is same as the request type
    tANI_U16                msgLen;  // length of the entire request
    tANI_U32                staId;  // Per STA stats request must contain valid
    tANI_U32                statsMask;  // categories of stats requested. look at ePEStatsMask
} tAniGetPEStatsReq, *tpAniGetPEStatsReq;

/*
 * tpAniGetPEStatsRsp is tied to 
 * for PE ==> SME eWNI_SME_GET_STATISTICS_RSP msgId  and 
 * for HAL ==> PE SIR_HAL_GET_STATISTICS_RSP msgId
 */
typedef struct sAniGetPEStatsRsp
{
    // Common for all types are responses
    tANI_U16                msgType;    // message type is same as the request type
    tANI_U16                msgLen;  // length of the entire request, includes the pStatsBuf length too
    tANI_U32                rc;         //success/failure
    tANI_U32                staId;  // Per STA stats request must contain valid
    tANI_U32                statsMask;  // categories of stats requested. look at ePEStatsMask
/**********************************************************************************************
    //void                  *pStatsBuf;
    The Stats buffer starts here and can be an aggregate of more than one statistics 
    structure depending on statsMask.The void pointer "pStatsBuf" is commented out 
    intentionally and the src code that uses this structure should take that into account. 
**********************************************************************************************/                                        
} tAniGetPEStatsRsp, *tpAniGetPEStatsRsp;

typedef struct sAniSummaryStatsInfo
{
    tANI_U32 retry_cnt[4];         //Total number of packets(per AC) that were successfully transmitted with retries
    tANI_U32 multiple_retry_cnt[4];//The number of MSDU packets and MMPDU frames per AC that the 802.11 
    // station successfully transmitted after more than one retransmission attempt

    tANI_U32 tx_frm_cnt[4];        //Total number of packets(per AC) that were successfully transmitted 
                                   //(with and without retries, including multi-cast, broadcast)     
    //tANI_U32 tx_fail_cnt;
    //tANI_U32 num_rx_frm_crc_err;   //Total number of received frames with CRC Error
    //tANI_U32 num_rx_frm_crc_ok;    //Total number of successfully received frames with out CRC Error
    tANI_U32 rx_frm_cnt;           //Total number of packets that were successfully received 
                                   //(after appropriate filter rules including multi-cast, broadcast)    
    tANI_U32 frm_dup_cnt;          //Total number of duplicate frames received successfully
    tANI_U32 fail_cnt[4];          //Total number packets(per AC) failed to transmit
    tANI_U32 rts_fail_cnt;         //Total number of RTS/CTS sequence failures for transmission of a packet
    tANI_U32 ack_fail_cnt;         //Total number packets failed transmit because of no ACK from the remote entity
    tANI_U32 rts_succ_cnt;         //Total number of RTS/CTS sequence success for transmission of a packet 
    tANI_U32 rx_discard_cnt;       //The sum of the receive error count and dropped-receive-buffer error count. 
                                   //HAL will provide this as a sum of (FCS error) + (Fail get BD/PDU in HW)
    tANI_U32 rx_error_cnt;         //The receive error count. HAL will provide the RxP FCS error global counter.
    tANI_U32 tx_byte_cnt;          //The sum of the transmit-directed byte count, transmit-multicast byte count 
                                   //and transmit-broadcast byte count. HAL will sum TPE UC/MC/BCAST global counters 
                                   //to provide this.
#if 0                                   
    //providing the following stats, in case of wrap around for tx_byte_cnt                                   
    tANI_U32 tx_unicast_lower_byte_cnt;
    tANI_U32 tx_unicast_upper_byte_cnt;
    tANI_U32 tx_multicast_lower_byte_cnt;
    tANI_U32 tx_multicast_upper_byte_cnt;
    tANI_U32 tx_broadcast_lower_byte_cnt;
    tANI_U32 tx_broadcast_upper_byte_cnt;
#endif

}tAniSummaryStatsInfo, *tpAniSummaryStatsInfo;

typedef struct sAniGlobalClassAStatsInfo
{
    tANI_U32 rx_frag_cnt;             //The number of MPDU frames received by the 802.11 station for MSDU packets 
                                     //or MMPDU frames
    tANI_U32 promiscuous_rx_frag_cnt; //The number of MPDU frames received by the 802.11 station for MSDU packets 
                                     //or MMPDU frames when a promiscuous packet filter was enabled
    //tANI_U32 rx_fcs_err;              //The number of MPDU frames that the 802.11 station received with FCS errors
    tANI_U32 rx_input_sensitivity;    //The receiver input sensitivity referenced to a FER of 8% at an MPDU length 
                                     //of 1024 bytes at the antenna connector. Each element of the array shall correspond 
                                     //to a supported rate and the order shall be the same as the supporteRates parameter.
    tANI_U32 max_pwr;                 //The maximum transmit power in dBm upto one decimal. 
                                      //for eg: if it is 10.5dBm, the value would be 105 
    //tANI_U32 default_pwr;             //The nominal transmit level used after normal power on sequence
    tANI_U32 sync_fail_cnt;           //Number of times the receiver failed to synchronize with the incoming signal 
                                     //after detecting the sync in the preamble of the transmitted PLCP protocol data unit. 
    tANI_U32 tx_rate;                 //Transmit rate, in units of 500 kbit/sec, for the most recently transmitted frame 

}tAniGlobalClassAStatsInfo, *tpAniGlobalClassAStatsInfo;


typedef struct sAniGlobalSecurityStats
{
    tANI_U32 rx_wep_unencrypted_frm_cnt; //The number of unencrypted received MPDU frames that the MAC layer discarded when 
                                        //the IEEE 802.11 dot11ExcludeUnencrypted management information base (MIB) object 
                                        //is enabled
    tANI_U32 rx_mic_fail_cnt;            //The number of received MSDU packets that that the 802.11 station discarded 
                                        //because of MIC failures
    tANI_U32 tkip_icv_err;               //The number of encrypted MPDU frames that the 802.11 station failed to decrypt 
                                        //because of a TKIP ICV error
    tANI_U32 aes_ccmp_format_err;        //The number of received MPDU frames that the 802.11 discarded because of an 
                                        //invalid AES-CCMP format
    tANI_U32 aes_ccmp_replay_cnt;        //The number of received MPDU frames that the 802.11 station discarded because of 
                                        //the AES-CCMP replay protection procedure
    tANI_U32 aes_ccmp_decrpt_err;        //The number of received MPDU frames that the 802.11 station discarded because of 
                                        //errors detected by the AES-CCMP decryption algorithm
    tANI_U32 wep_undecryptable_cnt;      //The number of encrypted MPDU frames received for which a WEP decryption key was 
                                        //not available on the 802.11 station
    tANI_U32 wep_icv_err;                //The number of encrypted MPDU frames that the 802.11 station failed to decrypt 
                                        //because of a WEP ICV error
    tANI_U32 rx_decrypt_succ_cnt;        //The number of received encrypted packets that the 802.11 station successfully 
                                        //decrypted
    tANI_U32 rx_decrypt_fail_cnt;        //The number of encrypted packets that the 802.11 station failed to decrypt

}tAniGlobalSecurityStats, *tpAniGlobalSecurityStats;
   
typedef struct sAniGlobalClassBStatsInfo
{
    tAniGlobalSecurityStats ucStats;
    tAniGlobalSecurityStats mcbcStats;
}tAniGlobalClassBStatsInfo, *tpAniGlobalClassBStatsInfo;

typedef struct sAniGlobalClassCStatsInfo
{
    tANI_U32 rx_amsdu_cnt;           //This counter shall be incremented for a received A-MSDU frame with the station�s 
                                    //MAC address in the address 1 field or an A-MSDU frame with a group address in the 
                                    //address 1 field
    tANI_U32 rx_ampdu_cnt;           //This counter shall be incremented when the MAC receives an AMPDU from the PHY
    tANI_U32 tx_20_frm_cnt;          //This counter shall be incremented when a Frame is transmitted only on the 
                                    //primary channel
    tANI_U32 rx_20_frm_cnt;          //This counter shall be incremented when a Frame is received only on the primary channel
    tANI_U32 rx_mpdu_in_ampdu_cnt;   //This counter shall be incremented by the number of MPDUs received in the A-MPDU 
                                    //when an A-MPDU is received
    tANI_U32 ampdu_delimiter_crc_err;//This counter shall be incremented when an MPDU delimiter has a CRC error when this 
                                    //is the first CRC error in the received AMPDU or when the previous delimiter has been 
                                    //decoded correctly

}tAniGlobalClassCStatsInfo, *tpAniGlobalClassCStatsInfo;

typedef struct sAniPerStaStatsInfo
{
    tANI_U32 tx_frag_cnt[4];       //The number of MPDU frames that the 802.11 station transmitted and acknowledged 
                                  //through a received 802.11 ACK frame
    tANI_U32 tx_ampdu_cnt;         //This counter shall be incremented when an A-MPDU is transmitted 
    tANI_U32 tx_mpdu_in_ampdu_cnt; //This counter shall increment by the number of MPDUs in the AMPDU when an A-MPDU 
                                  //is transmitted

}tAniPerStaStatsInfo, *tpAniPerStaStatsInfo;

/**********************PE Statistics end*************************/



typedef struct sSirRSSIThresholds
{
#ifdef ANI_BIG_BYTE_ENDIAN
    tANI_S8   ucRssiThreshold1     : 8;
    tANI_S8   ucRssiThreshold2     : 8;
    tANI_S8   ucRssiThreshold3     : 8;
    tANI_U8   bRssiThres1PosNotify : 1;
    tANI_U8   bRssiThres1NegNotify : 1;
    tANI_U8   bRssiThres2PosNotify : 1;
    tANI_U8   bRssiThres2NegNotify : 1;
    tANI_U8   bRssiThres3PosNotify : 1;
    tANI_U8   bRssiThres3NegNotify : 1;
    tANI_U8   bReserved10          : 2;
#else
    tANI_U8   bReserved10          : 2;
    tANI_U8   bRssiThres3NegNotify : 1;
    tANI_U8   bRssiThres3PosNotify : 1;
    tANI_U8   bRssiThres2NegNotify : 1;
    tANI_U8   bRssiThres2PosNotify : 1;
    tANI_U8   bRssiThres1NegNotify : 1;
    tANI_U8   bRssiThres1PosNotify : 1;
    tANI_S8   ucRssiThreshold3     : 8;
    tANI_S8   ucRssiThreshold2     : 8;
    tANI_S8   ucRssiThreshold1     : 8;
#endif

}tSirRSSIThresholds, *tpSirRSSIThresholds;

typedef struct sSirRSSINotification
{
#ifdef ANI_BIG_BYTE_ENDIAN
    tANI_U32             bRssiThres1PosCross : 1;
    tANI_U32             bRssiThres1NegCross : 1;
    tANI_U32             bRssiThres2PosCross : 1;
    tANI_U32             bRssiThres2NegCross : 1;
    tANI_U32             bRssiThres3PosCross : 1;
    tANI_U32             bRssiThres3NegCross : 1;
    tANI_U32             bReserved           : 26;
#else
    tANI_U32             bReserved           : 26;
    tANI_U32             bRssiThres3NegCross : 1;
    tANI_U32             bRssiThres3PosCross : 1;
    tANI_U32             bRssiThres2NegCross : 1;
    tANI_U32             bRssiThres2PosCross : 1;
    tANI_U32             bRssiThres1NegCross : 1;
    tANI_U32             bRssiThres1PosCross : 1;
#endif
    
}tSirRSSINotification, *tpSirRSSINotification;

typedef __ani_attr_pre_packed struct sSirTclasInfo
{
    tSirMacTclasIE   tclas;
    tANI_U8               version; // applies only for classifier type ip
    __ani_attr_pre_packed union {
        tSirMacTclasParamEthernet eth;
        tSirMacTclasParamIPv4     ipv4;
        tSirMacTclasParamIPv6     ipv6;
        tSirMacTclasParam8021dq   t8021dq;
    }__ani_attr_packed tclasParams;
} __ani_attr_packed tSirTclasInfo;

typedef struct sSirAddtsReqInfo
{
    tANI_U8               dialogToken;
    tSirMacTspecIE   tspec;

    tANI_U8               numTclas; // number of Tclas elements
    tSirTclasInfo    tclasInfo[SIR_MAC_TCLASIE_MAXNUM];
    tANI_U8               tclasProc;

    tANI_U8               wmeTspecPresent:1;
    tANI_U8               wsmTspecPresent:1;
    tANI_U8               lleTspecPresent:1;
    tANI_U8               tclasProcPresent:1;
} tSirAddtsReqInfo, *tpSirAddtsReqInfo;

typedef struct sSirAddtsRspInfo
{
    tANI_U8                 dialogToken;
    tSirMacStatusCodes status;
    tSirMacTsDelayIE   delay;

    tSirMacTspecIE     tspec;
    tANI_U8                 numTclas; // number of Tclas elements
    tSirTclasInfo      tclasInfo[SIR_MAC_TCLASIE_MAXNUM];
    tANI_U8                 tclasProc;
    tSirMacScheduleIE  schedule;

    tANI_U8                 wmeTspecPresent:1;
    tANI_U8                 wsmTspecPresent:1;
    tANI_U8                 lleTspecPresent:1;
    tANI_U8                 tclasProcPresent:1;
    tANI_U8                 schedulePresent:1;
} tSirAddtsRspInfo, *tpSirAddtsRspInfo;

typedef struct sSirDeltsReqInfo
{
    tSirMacTSInfo      tsinfo;
    tSirMacTspecIE     tspec;
    tANI_U8                 wmeTspecPresent:1;
    tANI_U8                 wsmTspecPresent:1;
    tANI_U8                 lleTspecPresent:1;
} tSirDeltsReqInfo, *tpSirDeltsReqInfo;

/// Add a tspec as defined
typedef struct sSirAddtsReq
{
    tANI_U16                messageType; // eWNI_SME_ADDTS_REQ
    tANI_U16                length;
    tANI_U32                timeout; // in ms
    tANI_U8                 rspReqd;
    tSirAddtsReqInfo   req;
} tSirAddtsReq, *tpSirAddtsReq;

typedef struct sSirAddtsRsp
{
    tANI_U16                messageType; // eWNI_SME_ADDTS_RSP
    tANI_U16                length;
    tANI_U32                rc;          // return code
    tSirAddtsRspInfo   rsp;
} tSirAddtsRsp, *tpSirAddtsRsp;

typedef struct sSirDeltsReq
{
    tANI_U16                messageType; // eWNI_SME_DELTS_REQ
    tANI_U16                length;
    tANI_U16                aid;  // use 0 if macAddr is being specified
    tANI_U8                 macAddr[6]; // only on AP to specify the STA
    tANI_U8                 rspReqd;
    tSirDeltsReqInfo   req;
} tSirDeltsReq, *tpSirDeltsReq;

typedef struct sSirDeltsRsp
{
    tANI_U16                messageType; // eWNI_SME_DELTS_RSP
    tANI_U16                length;
    tANI_U32                rc;
    tANI_U16                aid;  // use 0 if macAddr is being specified
    tANI_U8                 macAddr[6]; // only on AP to specify the STA
    tSirDeltsReqInfo   rsp;
} tSirDeltsRsp, *tpSirDeltsRsp;

typedef struct sSirSetTxPowerReq
{
    tANI_U16    messageType;
    tANI_U16    length;
    tANI_U32    txPower;
} tSirSetTxPowerReq, *tpSirSetTxPowerReq;

typedef struct sSirSetTxPowerRsp
{
    tANI_U16         messageType;
    tANI_U16         length;
    tANI_U32         status;
} tSirSetTxPowerRsp, *tpSirSetTxPowerRsp;

typedef struct sSirGetTxPowerReq
{
    tANI_U16    messageType;
    tANI_U16    length;
    tANI_U16    staid;
} tSirGetTxPowerReq, *tpSirGetTxPowerReq;

typedef struct sSirGetTxPowerRsp
{
    tANI_U16            messageType;
    tANI_U16            length; // length of the entire request
    tANI_U32            power;  // units of milliwatts
    tANI_U32            status;
} tSirGetTxPowerRsp, *tpSirGetTxPowerRsp;


typedef tANI_U32 tSirMacNoise[3];

typedef struct sSirGetNoiseRsp 
{
    tANI_U16            messageType;
    tANI_U16            length; 
    tSirMacNoise        noise;
} tSirGetNoiseRsp, *tpSirGetNoiseRsp;


//
// PMC --> PE --> HAL
// Power save configuration parameters
//
typedef struct sSirPowerSaveCfg
{
    tANI_U16    listenInterval;
   
    /* Number of consecutive missed beacons before 
     * hardware generates an interrupt to wake up 
     * the host. In units of listen interval.
     */
    tANI_U32 HeartBeatCount;

    /* specifies which beacons are to be forwarded
     * to host when beacon filtering is enabled.
     * In units of listen interval.
     */
    tANI_U32    nthBeaconFilter;

    /* Maximum number of PS-Poll send before 
     * firmware sends data null with PM set to 0.
     */
    tANI_U32    maxPsPoll;                                                 

    /* If the average RSSI value falls below the 
     * minRssiThreshold, then FW will send an 
     * interrupt to wake up the host. 
     */
    tANI_U32    minRssiThreshold;                                       

    /* Number of beacons for which firmware will 
     * collect the RSSI values and compute the average.
     */
    tANI_U8     numBeaconPerRssiAverage;                        

    /* FW collects the RSSI stats for this period
     * in BMPS mode.  
     */
    tANI_U8     rssiFilterPeriod;

    // Enabling/disabling broadcast frame filter feature
    tANI_U8     broadcastFrameFilter;    

    // Enabling/disabling the ignore DTIM feature
    tANI_U8     ignoreDtim;

    /* The following configuration parameters are kept
     * in order to be backward compatible for Gen5. 
     * These will NOT be used for Gen6 Libra chip
     */
    tBeaconForwarding beaconFwd;
    tANI_U16 nthBeaconFwd;
    tANI_U8 fEnablePwrSaveImmediately;
    tANI_U8 fPSPoll;

}tSirPowerSaveCfg, *tpSirPowerSaveCfg;

/* Reason code for requesting Full Power. This reason code is used by 
   any module requesting full power from PMC and also by PE when it
   sends the eWNI_PMC_EXIT_BMPS_IND to PMC*/
typedef enum eRequestFullPowerReason
{
   eSME_MISSED_BEACON_IND_RCVD,    /* PE received a MAX_MISSED_BEACON_IND */
   eSME_BMPS_STATUS_IND_RCVD,      /* PE received a SIR_HAL_BMPS_STATUS_IND */
   eSME_BMPS_MODE_DISABLED,        /* BMPS mode was disabled by HDD in SME */
   eSME_LINK_DISCONNECTED_BY_HDD,  /* Link has been disconnected requested by HDD */
   eSME_LINK_DISCONNECTED_BY_OTHER,/* Disconnect due to linklost or requested by peer */
   eSME_FULL_PWR_NEEDED_BY_HDD,    /* HDD request full power for some reason */
   eSME_FULL_PWR_NEEDED_BY_BAP,    /* BAP request full power for BT_AMP */
   eSME_FULL_PWR_NEEDED_BY_CSR,    /* CSR requests full power */
   eSME_FULL_PWR_NEEDED_BY_QOS,    /* QOS requests full power */
   eSME_REASON_OTHER               /* No specific reason. General reason code */ 
} tRequestFullPowerReason, tExitBmpsReason;

//This is sent alongwith eWNI_PMC_EXIT_BMPS_REQ message
typedef struct sExitBmpsInfo
{
   tExitBmpsReason exitBmpsReason;  /*Reason for exiting BMPS */
}tExitBmpsInfo, *tpExitBmpsInfo;


// MAC SW --> SME
// Message indicating to SME to exit BMPS sleep mode
typedef struct sSirSmeExitBmpsInd
{
    tANI_U16  mesgType;               /* eWNI_PMC_EXIT_BMPS_IND */
    tANI_U16  mesgLen;
    tSirResultCodes  statusCode;
    tExitBmpsReason  exitBmpsReason;  /*Reason for exiting BMPS */

} tSirSmeExitBmpsInd, *tpSirSmeExitBmpsInd;


//
// HDD -> LIM
// tSirMsgQ.type = eWNI_SME_DEL_BA_PEER_IND
// tSirMsgQ.reserved = 0
// tSirMsgQ.body = instance of tDelBAParams
//
typedef struct sSmeDelBAPeerInd
{
    // Message Type
    tANI_U16 mesgType;

    // Message Length
    tANI_U16 mesgLen;

    // Station Index
    tANI_U16 staIdx;

    // TID for which the BA session is being deleted
    tANI_U8 baTID;

    // DELBA direction
    // eBA_INITIATOR - Originator
    // eBA_RECEIPIENT - Recipient
    tANI_U8 baDirection;
} tSmeDelBAPeerInd, *tpSmeDelBAPeerInd;

typedef struct sSmeIbssPeerInd
{
    tANI_U16 	mesgType;
    tANI_U16 mesgLen;

    tSirMacAddr    peerAddr;
    tANI_U16        staId;

    //Beacon will be appended for new Peer indication.
}tSmeIbssPeerInd, *tpSmeIbssPeerInd;

typedef struct sLimScanChn
{
    tANI_U16 numTimeScan;   //how many time this channel is scan
    tANI_U8 channelId;
}tLimScanChn;

typedef struct sSmeGetScanChnRsp
{
    // Message Type
    tANI_U16 mesgType;
    // Message Length
    tANI_U16 mesgLen;
    tANI_U8 numChn;
    tLimScanChn scanChn[1];
} tSmeGetScanChnRsp, *tpSmeGetScanChnRsp;

typedef struct sLimScanChnInfo
{
    tANI_U8 numChnInfo;     //number of channels in scanChn
    tLimScanChn scanChn[SIR_MAX_SUPPORTED_CHANNEL_LIST];
}tLimScanChnInfo;


/*--------------------------------------------------------------------*/
/* BootLoader message definition                                      */
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
/* FW image size                                                      */
/*--------------------------------------------------------------------*/
#define SIR_FW_IMAGE_SIZE            146332


#define SIR_BOOT_MODULE_ID           1

#define SIR_BOOT_SETUP_IND           ((SIR_BOOT_MODULE_ID << 8) | 0x11)
#define SIR_BOOT_POST_RESULT_IND     ((SIR_BOOT_MODULE_ID << 8) | 0x12)
#define SIR_BOOT_DNLD_RESULT_IND     ((SIR_BOOT_MODULE_ID << 8) | 0x13)
#define SIR_BOOT_DNLD_DEV_REQ        ((SIR_BOOT_MODULE_ID << 8) | 0x41)
#define SIR_BOOT_DNLD_DEV_RSP        ((SIR_BOOT_MODULE_ID << 8) | 0x81)
#define SIR_BOOT_DNLD_REQ            ((SIR_BOOT_MODULE_ID << 8) | 0x42)
#define SIR_BOOT_DNLD_RSP            ((SIR_BOOT_MODULE_ID << 8) | 0x82)

/*--------------------------------------------------------------------*/
/* Bootloader message syntax                                          */
/*--------------------------------------------------------------------*/

// Message header
#define SIR_BOOT_MB_HEADER                 0
#define SIR_BOOT_MB_HEADER2                1

#define SIR_BOOT_MSG_HDR_MASK              0xffff0000
#define SIR_BOOT_MSG_LEN_MASK              0x0000ffff

// BOOT_SETUP_IND parameter indices
#define SIR_BOOT_SETUP_IND_MBADDR          2
#define SIR_BOOT_SETUP_IND_MBSIZE          3
#define SIR_BOOT_SETUP_IND_MEMOPT          4
#define SIR_BOOT_SETUP_IND_LEN             \
                                      ((SIR_BOOT_SETUP_IND_MEMOPT+1)<<2)

// BOOT_POST_RESULT_IND parameter indices
#define SIR_BOOT_POST_RESULT_IND_RES       2
#define SIR_BOOT_POST_RESULT_IND_LEN       \
                                  ((SIR_BOOT_POST_RESULT_IND_RES+1)<<2)

#define SIR_BOOT_POST_RESULT_IND_SUCCESS       1
#define SIR_BOOT_POST_RESULT_IND_MB_FAILED     2
#define SIR_BOOT_POST_RESULT_IND_SDRAM_FAILED  3
#define SIR_BOOT_POST_RESULT_IND_ESRAM_FAILED  4


// BOOT_DNLD_RESULT_IND parameter indices
#define SIR_BOOT_DNLD_RESULT_IND_RES       2
#define SIR_BOOT_DNLD_RESULT_IND_LEN       \
                                   ((SIR_BOOT_DNLD_RESULT_IND_RES+1)<<2)

#define SIR_BOOT_DNLD_RESULT_IND_SUCCESS   1
#define SIR_BOOT_DNLD_RESULT_IND_HDR_ERR   2
#define SIR_BOOT_DNLD_RESULT_IND_ERR       3

// BOOT_DNLD_DEV_REQ
#define SIR_BOOT_DNLD_DEV_REQ_SDRAMSIZE    2
#define SIR_BOOT_DNLD_DEV_REQ_FLASHSIZE    3
#define SIR_BOOT_DNLD_DEV_REQ_LEN          \
                                 ((SIR_BOOT_DNLD_DEV_REQ_FLASHSIZE+1)<<2)

// BOOT_DNLD_DEV_RSP
#define SIR_BOOT_DNLD_DEV_RSP_DEVTYPE      2
#define SIR_BOOT_DNLD_DEV_RSP_LEN          \
                                   ((SIR_BOOT_DNLD_DEV_RSP_DEVTYPE+1)<<2)

#define SIR_BOOT_DNLD_DEV_RSP_SRAM         1
#define SIR_BOOT_DNLD_DEV_RSP_FLASH        2

// BOOT_DNLD_REQ
#define SIR_BOOT_DNLD_REQ_OFFSET           2
#define SIR_BOOT_DNLD_REQ_WRADDR           3
#define SIR_BOOT_DNLD_REQ_SIZE             4
#define SIR_BOOT_DNLD_REQ_LEN              ((SIR_BOOT_DNLD_REQ_SIZE+1)<<2)

// BOOT_DNLD_RSP
#define SIR_BOOT_DNLD_RSP_SIZE             2
#define SIR_BOOT_DNLD_RSP_LEN              ((SIR_BOOT_DNLD_RSP_SIZE+1)<<2)

#if defined(ANI_OS_TYPE_RTAI_LINUX) || defined(ANI_OS_TYPE_LINUX)
// RTAI WRAPPER definition - Buffer block control entry
// the new block Control entry is initialized by HDD
// memory buffers (blkPool) are allocated by HDD
// The table resides in plmac_rtai.c
#define BLOCK_ALLOCATED_BY_HDD 1
#define RED_ZONE               16 // 16 bytes between buffers
#define NUM_POOL               16 // NUM_POOL defined here for now, not
                                  // so good
typedef struct
{
    tANI_U16              blkSize;
    tANI_U16              blkNum;
    void             *blkPool; // pointer to memory buffer

} t_mac_block_table;

#endif

// board capabilities fields are defined here.
typedef __ani_attr_pre_packed struct sSirBoardCapabilities
{
#ifndef ANI_LITTLE_BIT_ENDIAN
    tANI_U32 concat:1;        // 0 - Concat is not supported, 1 - Concat is supported
    tANI_U32 compression:1;   // 0 - Compression is not supported, 1 - Compression is supported
    tANI_U32 chnlBonding:1;   // 0 - Channel Bonding is not supported, 1 - Channel Bonding is supported
    tANI_U32 reverseFCS:1;    // 0 - Reverse FCS is not supported, 1 - Reverse FCS is supported
    tANI_U32 rsvd1:2;
    // (productId derives sub-category in the following three families)
    tANI_U32 cbFamily:1;      // 0 - Not CB family, 1 - Cardbus
    tANI_U32 apFamily:1;      // 0 - Not AP family, 1 - AP
    tANI_U32 mpciFamily:1;    // 0 - Not MPCI family, 1 - MPCI
    tANI_U32 bgOnly:1;        // 0 - default a/b/g; 1 - b/g only
    tANI_U32 bbChipVer:4;     // Baseband chip version
    tANI_U32 loType:2;        // 0 = no LO, 1 = SILABS, 2 = ORION
    tANI_U32 radioOn:2;       // Not supported is 3 or 2, 0 = Off and 1 = On
    tANI_U32 nReceivers:2;    // 0 based.
    tANI_U32 nTransmitters:1; // 0 = 1 transmitter, 1 = 2 transmitters
    tANI_U32 sdram:1;         // 0 = no SDRAM, 1 = SDRAM
    tANI_U32 rsvd:1;
    tANI_U32 extVsIntAnt:1;   // 0 = ext antenna, 1 = internal antenna
#else

    tANI_U32 extVsIntAnt:1;   // 0 = ext antenna, 1 = internal antenna
    tANI_U32 rsvd:1;
    tANI_U32 sdram:1;         // 0 = no SDRAM, 1 = SDRAM
    tANI_U32 nTransmitters:1; // 0 = 1 transmitter, 1 = 2 transmitters
    tANI_U32 nReceivers:2;    // 0 based.
    tANI_U32 radioOn:2;       // Not supported is 3 or 2, 0 = Off and 1 = On
    tANI_U32 loType:2;        // 0 = no LO, 1 = SILABS, 2 = ORION
    tANI_U32 bbChipVer:4;     // Baseband chip version
    tANI_U32 bgOnly:1;        // 0 - default a/b/g; 1 - b/g only
    // (productId derives sub-category in the following three families)
    tANI_U32 mpciFamily:1;    // 0 - Not MPCI family, 1 - MPCI
    tANI_U32 apFamily:1;      // 0 - Not AP family, 1 - AP
    tANI_U32 cbFamily:1;      // 0 - Not CB family, 1 - Cardbus
    tANI_U32 rsvd1:2;
    tANI_U32 reverseFCS:1;    // 0 - Reverse FCS is not supported, 1 - Reverse FCS is supported
    tANI_U32 chnlBonding:1;   // 0 - Channel Bonding is not supported, 1 - Channel Bonding is supported
    tANI_U32 compression:1;   // 0 - Compression is not supported, 1 - Compression is supported
    tANI_U32 concat:1;        // 0 - Concat is not supported, 1 - Concat is supported
#endif
} __ani_attr_packed  tSirBoardCapabilities, *tpSirBoardCapabilities;

# define ANI_BCAP_EXT_VS_INT_ANT_MASK   0x1
# define ANI_BCAP_EXT_VS_INT_ANT_OFFSET 0

# define ANI_BCAP_GAL_ON_BOARD_MASK     0x2
# define ANI_BCAP_GAL_ON_BOARD_OFFSET   1

# define ANI_BCAP_SDRAM_MASK            0x4
# define ANI_BCAP_SDRAM_OFFSET          2

# define ANI_BCAP_NUM_TRANSMITTERS_MASK   0x8
# define ANI_BCAP_NUM_TRANSMITTERS_OFFSET 3

# define ANI_BCAP_NUM_RECEIVERS_MASK    0x30
# define ANI_BCAP_NUM_RECEIVERS_OFFSET  4

# define ANI_BCAP_RADIO_ON_MASK         0xC0
# define ANI_BCAP_RADIO_ON_OFFSET       6

# define ANI_BCAP_LO_TYPE_MASK          0x300
# define ANI_BCAP_LO_TYPE_OFFSET        8

# define ANI_BCAP_BB_CHIP_VER_MASK      0xC00
# define ANI_BCAP_BB_CHIP_VER_OFFSET    10

# define ANI_BCAP_CYG_DATE_CODE_MASK    0xFF000
# define ANI_BCAP_CYG_DATE_CODE_OFFSET  12

# define ANI_BCAP_RADIO_OFF              0
# define ANI_BCAP_RADIO_ON               1
# define ANI_BCAP_RADIO_ON_NOT_SUPPORTED 3


/// WOW related structures
// SME -> PE <-> HAL
#define SIR_WOWL_BCAST_PATTERN_MAX_SIZE 128
#define SIR_WOWL_BCAST_MAX_NUM_PATTERNS 8

// SME -> PE -> HAL - This is to add WOWL BCAST wake-up pattern. 
// SME/HDD maintains the list of the BCAST wake-up patterns.
// This is a pass through message for PE
typedef struct sSirWowlAddBcastPtrn
{
    tANI_U8  ucPatternId;  			// Pattern ID
    // Pattern byte offset from beginning of the 802.11 packet to start of the
    // wake-up pattern
    tANI_U8  ucPatternByteOffset; 	
    tANI_U8  ucPatternSize; 		// Non-Zero Pattern size
    tANI_U8  ucPattern[SIR_WOWL_BCAST_PATTERN_MAX_SIZE]; // Pattern
    tANI_U8  ucPatternMaskSize; 	// Non-zero pattern mask size
    tANI_U8  ucPatternMask[SIR_WOWL_BCAST_PATTERN_MAX_SIZE]; // Pattern mask
	
} tSirWowlAddBcastPtrn, *tpSirWowlAddBcastPtrn;


// SME -> PE -> HAL - This is to delete WOWL BCAST wake-up pattern. 
// SME/HDD maintains the list of the BCAST wake-up patterns.
// This is a pass through message for PE
typedef struct sSirWowlDelBcastPtrn
{
    /* Pattern ID of the wakeup pattern to be deleted */
    tANI_U8  ucPatternId;
}tSirWowlDelBcastPtrn, *tpSirWowlDelBcastPtrn;


// SME->PE: Enter WOWLAN parameters 
typedef struct sSirSmeWowlEnterParams
{
    /* Enables/disables magic packet filtering */
    tANI_U8   ucMagicPktEnable;	

    /* Magic pattern */
    tSirMacAddr magicPtrn;

    /* Enables/disables packet pattern filtering */
    tANI_U8   ucPatternFilteringEnable;	

} tSirSmeWowlEnterParams, * tpSirSmeWowlEnterParams;


// PE<->HAL: Enter WOWLAN parameters 
typedef struct sSirHalWowlEnterParams
{
    /* Enables/disables magic packet filtering */
    tANI_U8   ucMagicPktEnable;	

    /* Magic pattern */
    tSirMacAddr magicPtrn;

    /* Enables/disables packet pattern filtering in firmware. 
       Enabling this flag enables broadcast pattern matching 
       in Firmware. If unicast pattern matching is also desired,  
       ucUcastPatternFilteringEnable flag must be set tot true 
       as well 
    */
    tANI_U8   ucPatternFilteringEnable;

    /* Enables/disables unicast packet pattern filtering. 
       This flag specifies whether we want to do pattern match 
       on unicast packets as well and not just broadcast packets. 
       This flag has no effect if the ucPatternFilteringEnable 
       (main controlling flag) is set to false
    */
    tANI_U8   ucUcastPatternFilteringEnable;                     

    /* This configuration is valid only when magicPktEnable=1. 
     * It requests hardware to wake up when it receives the 
     * Channel Switch Action Frame.
     */
    tANI_U8   ucWowChnlSwitchRcv;

    /* This configuration is valid only when magicPktEnable=1. 
     * It requests hardware to wake up when it receives the 
     * Deauthentication Frame. 
     */
    tANI_U8   ucWowDeauthRcv;

    /* This configuration is valid only when magicPktEnable=1. 
     * It requests hardware to wake up when it receives the 
     * Disassociation Frame. 
     */
    tANI_U8   ucWowDisassocRcv;

    /* This configuration is valid only when magicPktEnable=1. 
     * It requests hardware to wake up when it has missed
     * consecutive beacons. This is a hardware register
     * configuration (NOT a firmware configuration). 
     */
    tANI_U8   ucWowMaxMissedBeacons;

    /* This configuration is valid only when magicPktEnable=1. 
     * This is a timeout value in units of microsec. It requests
     * hardware to unconditionally wake up after it has stayed
     * in WoWLAN mode for some time. Set 0 to disable this feature. 	 
     */
    tANI_U8   ucWowMaxSleepUsec;

    /* Status code to be filled by HAL when it sends
     * SIR_HAL_WOWL_ENTER_RSP to PE. 
     */  
    eHalStatus  status;

} tSirHalWowlEnterParams, *tpSirHalWowlEnterParams;


#define SIR_IS_FULL_POWER_REASON_DISCONNECTED(eReason) \
    ( ( eSME_LINK_DISCONNECTED_BY_HDD == (eReason) ) || ( eSME_LINK_DISCONNECTED_BY_OTHER == (eReason) ) )
#define SIR_IS_FULL_POWER_NEEDED_BY_HDD(eReason) \
    ( ( eSME_LINK_DISCONNECTED_BY_HDD == (eReason) ) || ( eSME_FULL_PWR_NEEDED_BY_HDD == (eReason) ) )

#endif /* __SIR_API_H */

