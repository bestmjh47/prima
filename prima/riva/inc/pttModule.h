/**
 *
   Airgo Networks, Inc proprietary.
   All Rights Reserved, Copyright 2005
   This program is the confidential and proprietary product of Airgo Networks Inc.
   Any Unauthorized use, reproduction or transfer of this program is strictly prohibited.


   pttModule.h: global variable structure for pttModule
   Author:  Mark Nelson
   Date: 6/27/05

   History -
   Date         Modified by               Modification Information
  --------------------------------------------------------------------------

 */

#ifndef PTTMODULE_H
#define PTTMODULE_H

/* !!! FIX ME
 * To resolve PRIMA build issue this is temp blocker
 * this must have correct solution */
#ifdef FEATURE_WLAN_INTEGRATED_SOC
#ifndef ISOC_HOST_DRIVER
#define ISOC_HOST_DRIVER
#endif /* ISOC_HOST_DRIVER */
#endif /* FEATURE_WLAN_INTEGRATED_SOC */

#ifndef ISOC_HOST_DRIVER
#include "phyTest.h"
#include <phyWfm.h>
#include <wlan_hal_msg.h>       //this includes those types that are needed to store the associate NV tables
#include "phyDbg.h"
#include "pttFrameGen.h"
#endif /* ISOC_HOST_DRIVER */

#if defined(FEATURE_WLAN_INTEGRATED_SOC) && defined(ISOC_HOST_DRIVER)
#include "halCompiler.h"

/* Integrated SOC, Host driver build issue clear */
#ifndef PACKED_PRE
#define PACKED_PRE
#endif

#ifndef PACKED_POST
#define PACKED_POST   __attribute__((packed))
#endif
#endif /*  FEATURE_WLAN_INTEGRATED_SOC && ISOC_HOST_DRIVER */

typedef enum {

   PTT_STATUS_SUCCESS = 0,
   PTT_STATUS_FAILURE = 1,
   PTT_MAX_VAL = 0XFFFFFFFF,    //dummy added to change enum to 4 bytes

} eQWPttStatus;

typedef PACKED_PRE struct PACKED_POST {
   tANI_U16 drvMjr;
   tANI_U16 drvMnr;
   tANI_U16 drvPtch;
   tANI_U16 drvBld;
   tANI_U16 pttMax;
   tANI_U16 pttMin;
   //FwVersionInfo fwVer;
} sBuildReleaseParams;

typedef PACKED_PRE struct PACKED_POST {
   tANI_BOOLEAN agPktsDisabled; //802.11ag
   tANI_BOOLEAN bPktsDisabled;  //802.11b
   tANI_BOOLEAN slrPktsDisabled; //SLR rates
   tANI_BOOLEAN rsvd;
} sRxTypesDisabled;

typedef PACKED_PRE struct PACKED_POST {
   tANI_U32 totalRxPackets;
   tANI_U32 totalMacRxPackets;
   tANI_U32 totalMacFcsErrPackets;
} sRxFrameCounters;




/* GRAB RAM types */
//TODO: Change Grab RAM interface as appropriate to Taurus
typedef enum {
   GRAB_RAM_RXFIR,
   GRAB_RAM_ADC,
   GRAB_RAM_ADC_80,
   GRAB_RAM_MAX_VAL = 0XFFFFFFFF,   //dummy added to change enum to 4 bytes
} eGrabRamType;

#define GRAB_RAM_SIZE 6000
#define GRAB_RAM_SIZE_80MHZ_1_CHAIN 12000



/// Enum used to specify the trigger type for the aniGrabRam API
typedef enum eGramDumpTrigType {
   eGRAM_DUMP_UNTRIGGERED,
   eGRAM_DUMP_TRIG_ON_11A,
   eGRAM_DUMP_TRIG_ON_11B,
   eGRAM_DUMP_TRIG_ON_11A_OR_11B
} tGramDumpTrigType;

#ifndef ISOC_HOST_DRIVER
/* These are not needed within integrated host driver */
typedef PACKED_PRE struct PACKED_POST {
   //common to both transmit chains
   eHalPhyRates rate;           //current rate
   ePhyChanBondState cbState;   //current Channel bonded state

   tANI_U8 channelId;           //current channel Id
   tANI_U8 pwrTemplateIndex;    //5-bit template index used for the current rate
   tANI_U8 reserved[2];

   //specific transmit chain power
   tTxChainPower txChains[PHY_MAX_TX_CHAINS];   //output power for Tx chains
} tTxPowerReport;


typedef PACKED_PRE struct PACKED_POST {
   /*
    * The idea here is to store only those things which cannot be
    * handled directly within the individual function calls.
    * Most things will go straight to registers or come from registers.
    */
   sPttFrameGenParams frameGenParams;
   tANI_U8 payload[MAX_PAYLOAD_SIZE];

   //Tx Waveform Gen Service
   tANI_U16 numWfmSamples;
   tANI_BOOLEAN wfmEnabled;
   tANI_BOOLEAN wfmStored;

   //Tx Frame Power Service
   tTxGain forcedTxGain[PHY_MAX_TX_CHAINS];  //use TXPWR_OVERRIDE for wfm, and fill gain table otherwise
   tANI_U8 tpcPowerLut[PHY_MAX_TX_CHAINS][TPC_MEM_POWER_LUT_DEPTH];
   tTxGain tpcGainLut[PHY_MAX_TX_CHAINS][TPC_MEM_GAIN_LUT_DEPTH];

   //Tx Frame Gen Service
   tANI_BOOLEAN frameGenEnabled;
   tANI_BOOLEAN phyDbgFrameGen; //this says use phyDbg for frames - leave this in place until we know that PhyDbg will suffice
   tANI_U8 reserved[2];

   //Rx Gain Service
   sRxChainsAgcEnable agcEnables;

   tANI_U32 *pADCCaptureCache;  //pointer to allocate ADC capture cache

   //TX_TIMER  adcRssiStatsTimer; //Create adc rssi stat collection timer

   sRxChainsRssi rssi;
} tPttModuleVariables;
#endif /* ISOC_HOST_DRIVER */

#endif /* PTTMODULE_H */
