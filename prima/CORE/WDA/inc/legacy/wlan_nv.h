/** ------------------------------------------------------------------------- *
    ------------------------------------------------------------------------- *


    \file wlan_nv.h

    \brief Types for NV implementation
           Anything that needs to be publicly available should
           be in this file

    $Id$

    Copyright (C) 2006 Airgo Networks, Incorporated


   ========================================================================== */

#if !defined( __WLAN_NV_H )
#define __WLAN_NV_H

#include "palTypes.h"

//From HAL/inc/halNv.h
typedef enum
{
    //Common Nv Fields
    NV_COMMON_PRODUCT_ID,               // 0
    NV_COMMON_PRODUCT_BANDS,            // 1
    NV_COMMON_NUM_OF_TX_CHAINS,         // 2
    NV_COMMON_NUM_OF_RX_CHAINS,         // 3
    NV_COMMON_MAC_ADDR,                 // 4
    NV_COMMON_MFG_SERIAL_NUMBER,        // 5

    NUM_NV_FIELDS,
    NV_MAX_FIELD = 0xFFFFFFFF  /* define as 4 bytes data */

}eNvField;


#define NV_FIELD_MAC_ADDR_SIZE      6
#define NV_FIELD_MFG_SN_SIZE        40
typedef enum
{
    PRODUCT_BAND_11_B_G     = 0,    //Gen6.0 is only this setting
    PRODUCT_BAND_11_A_B_G   = 1,
    PRODUCT_BAND_11_A       = 2,

    NUM_PRODUCT_BANDS
}eNvProductBands;           //NV_COMMON_PRODUCT_BANDS


typedef union
{
    //common NV fields
    tANI_U16  productId;
    tANI_U8   productBands;
    tANI_U8   numOfTxChains;
    tANI_U8   numOfRxChains;
    tANI_U8   macAddr[NV_FIELD_MAC_ADDR_SIZE];
    tANI_U8   mfgSN[NV_FIELD_MFG_SN_SIZE];
}uNvFields;



//format of common part of nv
typedef struct
{
    //always ensure fields are aligned to 32-bit boundaries
    tANI_U16  productId;
    tANI_U8   productBands;
    tANI_U8   unused1[1];

    tANI_U8   numOfTxChains;
    tANI_U8   numOfRxChains;
    tANI_U8   macAddr[NV_FIELD_MAC_ADDR_SIZE];

    tANI_U8   mfgSN[NV_FIELD_MFG_SN_SIZE];
}sNvFields;


//From wlanfw/inc/halPhyTypes.h

typedef tANI_S8 tPowerdBm;   //power in signed 8-bit integer, no decimal places

typedef union
{
    tANI_U32 measurement;      //measured values can be passed to pttApi, but are maintained to 2 decimal places internally
    tANI_S16 reported;         //used internally only - reported values only maintain 2 decimals places
}uAbsPwrPrecision;

typedef enum
{
    PHY_TX_CHAIN_0 = 0,

    PHY_MAX_TX_CHAINS = 1,
    PHY_ALL_TX_CHAINS,

    //possible tx chain combinations
    PHY_NO_TX_CHAINS,
    PHY_TX_CHAIN_MAX = 0xFFFFFFFF  /* define as 4 bytes data */
}ePhyTxChains;

//From wlanfw/inc/halRfTypes.h

typedef enum
{
    REG_DOMAIN_FCC,
    REG_DOMAIN_ETSI,
    REG_DOMAIN_JAPAN,
    REG_DOMAIN_WORLD,
    REG_DOMAIN_N_AMER_EXC_FCC,
    REG_DOMAIN_APAC,
    REG_DOMAIN_KOREA,
    REG_DOMAIN_HI_5GHZ,
    REG_DOMAIN_NO_5GHZ,

    NUM_REG_DOMAINS
}eRegDomainId;

typedef enum
{
    RF_SUBBAND_2_4_GHZ      = 0,

    NUM_RF_SUBBANDS,

    MAX_RF_SUBBANDS,
    INVALID_RF_SUBBAND,

    RF_BAND_2_4_GHZ = 0,
    NUM_RF_BANDS
}eRfSubBand;

typedef enum
{
    //2.4GHz Band
    RF_CHAN_1                 = 0,
    RF_CHAN_2                 = 1,
    RF_CHAN_3                 = 2,
    RF_CHAN_4                 = 3,
    RF_CHAN_5                 = 4,
    RF_CHAN_6                 = 5,
    RF_CHAN_7                 = 6,
    RF_CHAN_8                 = 7,
    RF_CHAN_9                 = 8,
    RF_CHAN_10                = 9,
    RF_CHAN_11                = 10,
    RF_CHAN_12                = 11,
    RF_CHAN_13                = 12,
    RF_CHAN_14                = 13,

    NUM_RF_CHANNELS,

    MIN_2_4GHZ_CHANNEL = RF_CHAN_1,
    MAX_2_4GHZ_CHANNEL = RF_CHAN_14,
    NUM_2_4GHZ_CHANNELS = (MAX_2_4GHZ_CHANNEL - MIN_2_4GHZ_CHANNEL + 1),

    MIN_20MHZ_RF_CHANNEL = RF_CHAN_1,
    MAX_20MHZ_RF_CHANNEL = RF_CHAN_14,
    NUM_20MHZ_RF_CHANNELS = (MAX_20MHZ_RF_CHANNEL - MIN_20MHZ_RF_CHANNEL + 1),

    NUM_TPC_2_4GHZ_CHANNELS = 14,

    INVALID_RF_CHANNEL = 0xBAD
}eRfChannels;

typedef struct
{
    tANI_BOOLEAN enabled;
    tPowerdBm pwrLimit;
}sRegulatoryChannel;

typedef struct
{
    sRegulatoryChannel channels[NUM_RF_CHANNELS];
    uAbsPwrPrecision antennaGain[NUM_RF_SUBBANDS];
    uAbsPwrPrecision bRatePowerOffset[NUM_2_4GHZ_CHANNELS];
    uAbsPwrPrecision gnRatePowerOffset[NUM_2_4GHZ_CHANNELS];
}sRegulatoryDomains;

typedef struct
{
    tANI_S16 bRssiOffset[NUM_2_4GHZ_CHANNELS];
    tANI_S16 gnRssiOffset[NUM_2_4GHZ_CHANNELS];
}sRssiChannelOffsets;

typedef struct
{
    tANI_U16 targetFreq;           //number in MHz
    tANI_U8 channelNum;            //channel number as in the eRfChannels enumeration
    eRfSubBand band;               //band that this channel belongs to
}tRfChannelProps;

typedef enum
{
    MODE_802_11B    = 0,
    MODE_802_11AG   = 1,
    MODE_802_11N    = 2,
    NUM_802_11_MODES
} e80211Modes;


//From wlanfw/inc/halPhyCalMemory.h
typedef struct
{
    tANI_U16    process_monitor;
    tANI_U8     hdet_cal_code;
    tANI_U8     rxfe_gm_2;

    tANI_U8     tx_bbf_rtune;
    tANI_U8     pa_rtune_reg;
    tANI_U8     rt_code;
    tANI_U8     bias_rtune;

    tANI_U8     bb_bw1;
    tANI_U8     bb_bw2;
    tANI_U8     pa_ctune_reg;
    tANI_U8     reserved1;

    tANI_U8     bb_bw3;
    tANI_U8     bb_bw4;
    tANI_U8     bb_bw5;
    tANI_U8     bb_bw6;

    tANI_U16    rcMeasured;
    tANI_U8     tx_bbf_ct;
    tANI_U8     tx_bbf_ctr;

    tANI_U8     csh_maxgain_reg;
    tANI_U8     csh_0db_reg;
    tANI_U8     csh_m3db_reg;
    tANI_U8     csh_m6db_reg;

    tANI_U8     cff_0db_reg;
    tANI_U8     cff_m3db_reg;
    tANI_U8     cff_m6db_reg;
    tANI_U8     rxfe_gpio_ctl_1;

    tANI_U8     mix_bal_cnt_2;
    tANI_S8     rxfe_lna_highgain_bias_ctl_delta;
    tANI_U8     rxfe_lna_load_ctune;
    tANI_U8     rxfe_lna_ngm_rtune;

    tANI_U8     rx_im2_spare0;
    tANI_U8     rx_im2_spare1;
    tANI_U16    hdet_dco;

    tANI_U8     pll_vfc_reg3_b0;
    tANI_U8     pll_vfc_reg3_b1;
    tANI_U8     pll_vfc_reg3_b2;
    tANI_U8     pll_vfc_reg3_b3;

    tANI_U16    tempStart;
	tANI_U16    roomTemp;

}sCalData;

typedef struct
{
    tANI_U32 calStatus;  //use eNvCalID
    sCalData calData;
}sRFCalValues;

//From wlanfw/inc/halPhyCfg.h
typedef tANI_U8 tTpcLutValue;

#define MAX_TPC_CAL_POINTS      (8)

typedef tANI_U8 tPowerDetect;        //7-bit power detect reading

typedef struct
{
    tPowerDetect pwrDetAdc;            //= SENSED_PWR register, which reports the 8-bit ADC
                                       // the stored ADC value gets shifted to 7-bits as the index to the LUT
    tPowerDetect adjustedPwrDet;       //7-bit value that goes into the LUT at the LUT[pwrDet] location
                                       //MSB set if extraPrecision.hi8_adjustedPwrDet is used
}tTpcCaldPowerPoint;

typedef tTpcCaldPowerPoint tTpcCaldPowerTable[PHY_MAX_TX_CHAINS][MAX_TPC_CAL_POINTS];

typedef struct
{
    tTpcCaldPowerTable empirical;                      //calibrated power points
}tTpcConfig;

//From wlanfw/inc/phyTxPower.h
typedef tTpcLutValue tTpcPowerTable[PHY_MAX_TX_CHAINS][128];  

typedef struct
{
    tTpcConfig *pwrSampled;             //points to CLPC data in calMemory
}tPhyTxPowerBand;

//From halPhyRates.h
typedef enum
{
    //802.11b Rates
    HAL_PHY_RATE_11B_LONG_1_MBPS,
    HAL_PHY_RATE_11B_LONG_2_MBPS,
    HAL_PHY_RATE_11B_LONG_5_5_MBPS,
    HAL_PHY_RATE_11B_LONG_11_MBPS,
    HAL_PHY_RATE_11B_SHORT_2_MBPS,
    HAL_PHY_RATE_11B_SHORT_5_5_MBPS,
    HAL_PHY_RATE_11B_SHORT_11_MBPS,

    //SLR Rates
    HAL_PHY_RATE_SLR_0_25_MBPS,
    HAL_PHY_RATE_SLR_0_5_MBPS,

    //Spica_Virgo 11A 20MHz Rates
    HAL_PHY_RATE_11A_6_MBPS,
    HAL_PHY_RATE_11A_9_MBPS,
    HAL_PHY_RATE_11A_12_MBPS,
    HAL_PHY_RATE_11A_18_MBPS,
    HAL_PHY_RATE_11A_24_MBPS,
    HAL_PHY_RATE_11A_36_MBPS,
    HAL_PHY_RATE_11A_48_MBPS,
    HAL_PHY_RATE_11A_54_MBPS,

    //MCS Index #0-15 (20MHz)
    HAL_PHY_RATE_MCS_1NSS_6_5_MBPS,
    HAL_PHY_RATE_MCS_1NSS_13_MBPS,
    HAL_PHY_RATE_MCS_1NSS_19_5_MBPS,
    HAL_PHY_RATE_MCS_1NSS_26_MBPS,
    HAL_PHY_RATE_MCS_1NSS_39_MBPS,
    HAL_PHY_RATE_MCS_1NSS_52_MBPS,
    HAL_PHY_RATE_MCS_1NSS_58_5_MBPS,
    HAL_PHY_RATE_MCS_1NSS_65_MBPS,
    HAL_PHY_RATE_MCS_1NSS_MM_SG_7_2_MBPS,
    HAL_PHY_RATE_MCS_1NSS_MM_SG_14_4_MBPS,
    HAL_PHY_RATE_MCS_1NSS_MM_SG_21_7_MBPS,
    HAL_PHY_RATE_MCS_1NSS_MM_SG_28_9_MBPS,
    HAL_PHY_RATE_MCS_1NSS_MM_SG_43_3_MBPS,
    HAL_PHY_RATE_MCS_1NSS_MM_SG_57_8_MBPS,
    HAL_PHY_RATE_MCS_1NSS_MM_SG_65_MBPS,
    HAL_PHY_RATE_MCS_1NSS_MM_SG_72_2_MBPS,

    NUM_HAL_PHY_RATES,
    HAL_PHY_RATE_INVALID,
    MIN_RATE_INDEX                                    = 0,
    MAX_RATE_INDEX = HAL_PHY_RATE_MCS_1NSS_MM_SG_72_2_MBPS
}eHalPhyRates;

#define NUM_RATE_POWER_GROUPS           NUM_HAL_PHY_RATES  //total number of rate power groups including the CB_RATE_POWER_OFFSET
typedef uAbsPwrPrecision tRateGroupPwr[NUM_RATE_POWER_GROUPS];

//From halNvTables.h
#define NV_FIELD_COUNTRY_CODE_SIZE  3
typedef struct
{
    tANI_U8 regDomain;                                  //from eRegDomainId
    tANI_U8 countryCode[NV_FIELD_COUNTRY_CODE_SIZE];    // string identifier
}sDefaultCountry;

typedef union
{
    tRateGroupPwr           pwrOptimum[NUM_RF_SUBBANDS];             // NV_TABLE_RATE_POWER_SETTINGS
    sRegulatoryDomains      regDomains[NUM_REG_DOMAINS];             // NV_TABLE_REGULATORY_DOMAINS
    sDefaultCountry         defaultCountryTable;                     // NV_TABLE_DEFAULT_COUNTRY
    tTpcPowerTable          plutCharacterized[NUM_2_4GHZ_CHANNELS];  // NV_TABLE_TPC_POWER_TABLE
    tANI_U16                plutPdadcOffset[NUM_2_4GHZ_CHANNELS];    // NV_TABLE_TPC_PDADC_OFFSETS
    //sCalFlashMemory       calFlashMemory;                          // NV_TABLE_CAL_MEMORY
    //sCalStatus            calStatus;                               // NV_TABLE_CAL_STATUS
    sRssiChannelOffsets     rssiChanOffsets[2];                      // NV_TABLE_RSSI_CHANNEL_OFFSETS
    sRFCalValues            rFCalValues;                             // NV_TABLE_RF_CAL_VALUES
    tANI_S16                antennaPathLoss[NUM_2_4GHZ_CHANNELS];    // NV_TABLE_ANTENNA_PATH_LOSS
    tANI_S16                pktTypePwrLimits[NUM_802_11_MODES][NUM_2_4GHZ_CHANNELS]; //NV_TABLE_PACKET_TYPE_POWER_LIMITS
}uNvTables;

//From halPhy.h
typedef tPowerdBm tChannelPwrLimit;

typedef struct
{
    tANI_U8 chanId;
    tChannelPwrLimit pwr;
}tChannelListWithPower;

//From HAL/inc/halNvTables.h
typedef enum
{
    NV_FIELDS_IMAGE                 = 0,    //contains all fields

    NV_TABLE_RATE_POWER_SETTINGS    = 2,
    NV_TABLE_REGULATORY_DOMAINS     = 3,
    NV_TABLE_DEFAULT_COUNTRY        = 4,
    NV_TABLE_TPC_POWER_TABLE        = 5,
    NV_TABLE_TPC_PDADC_OFFSETS      = 6,
    NV_TABLE_RF_CAL_VALUES          = 7,
    NV_TABLE_RSSI_CHANNEL_OFFSETS   = 9,
    NV_TABLE_CAL_MEMORY             = 10,    //cal memory structure from halPhyCalMemory.h preceded by status
    NV_TABLE_CAL_STATUS             = 11,
    NV_TABLE_ANTENNA_PATH_LOSS          = 12,
    NV_TABLE_PACKET_TYPE_POWER_LIMITS   = 13,

    NUM_NV_TABLE_IDS,
    NV_ALL_TABLES                   = 0xFFF,
    NV_BINARY_IMAGE                 = 0x1000,
    NV_MAX_TABLE                    = 0xFFFFFFFF  /* define as 4 bytes data */
}eNvTable;

typedef struct
{
    tRateGroupPwr           pwrOptimum[NUM_RF_SUBBANDS];              // NV_TABLE_RATE_POWER_SETTINGS
    sRegulatoryDomains      regDomains[NUM_REG_DOMAINS];              // NV_TABLE_REGULATORY_DOMAINS
    sDefaultCountry         defaultCountryTable;                      // NV_TABLE_DEFAULT_COUNTRY
    tTpcPowerTable          plutCharacterized[NUM_2_4GHZ_CHANNELS];   // NV_TABLE_TPC_POWER_TABLE
    tANI_U16                plutPdadcOffset[NUM_2_4GHZ_CHANNELS];     // NV_TABLE_TPC_PDADC_OFFSETS
    //sCalFlashMemory         calFlashMemory;                           // NV_TABLE_CAL_MEMORY
    //sCalStatus              calStatus;                                // NV_TABLE_CAL_STATUS
    sRssiChannelOffsets     rssiChanOffsets[2];                       // NV_TABLE_RSSI_CHANNEL_OFFSETS
    sRFCalValues            rFCalValues;                              // NV_TABLE_RF_CAL_VALUES
    tANI_S16                antennaPathLoss[NUM_2_4GHZ_CHANNELS];     // NV_TABLE_ANTENNA_PATH_LOSS
    tANI_S16                pktTypePwrLimits[NUM_802_11_MODES][NUM_2_4GHZ_CHANNELS]; //NV_TABLE_PACKET_TYPE_POWER_LIMITS
}sNvTables;

typedef struct
{
    sNvFields fields;
    sNvTables tables;
}sHalNv;

extern const sHalNv nvDefaults;

#endif
	
