/*
* Copyright (c) 2011-2013 Qualcomm Atheros, Inc.
* All Rights Reserved.
* Qualcomm Atheros Confidential and Proprietary.
*/

#if !defined( __SMEINTERNAL_H )
#define __SMEINTERNAL_H


/**=========================================================================
  
  \file  smeInternal.h
  
  \brief prototype for SME internal structures and APIs used for SME and MAC
  
   Copyright 2008 (c) Qualcomm Technologies, Inc.  All Rights Reserved.
   
   Qualcomm Technologies Confidential and Proprietary.
  
  ========================================================================*/

/* $Header$ */

/*--------------------------------------------------------------------------
  Include Files
  ------------------------------------------------------------------------*/
#include "vos_status.h"
#include "vos_lock.h"
#include "vos_trace.h"
#include "vos_memory.h"
#include "vos_types.h"
#include "csrLinkList.h"

/*-------------------------------------------------------------------------- 
  Type declarations
  ------------------------------------------------------------------------*/

// Mask can be only have one bit set
typedef enum eSmeCommandType 
{
    eSmeNoCommand = 0, 
    eSmeDropCommand,
    //CSR
    eSmeCsrCommandMask = 0x10000,   //this is not a command, it is to identify this is a CSR command
    eSmeCommandScan,
    eSmeCommandRoam, 
    eSmeCommandWmStatusChange, 
    eSmeCommandSetKey,
    eSmeCommandRemoveKey,
    eSmeCommandAddStaSession,
    eSmeCommandDelStaSession,
#ifdef FEATURE_WLAN_TDLS
    //eSmeTdlsCommandMask = 0x80000,  //To identify TDLS commands <TODO>
    //These can be considered as csr commands. 
    eSmeCommandTdlsSendMgmt, 
    eSmeCommandTdlsAddPeer, 
    eSmeCommandTdlsDelPeer, 
    eSmeCommandTdlsLinkEstablish,
#ifdef FEATURE_WLAN_TDLS_INTERNAL
    eSmeCommandTdlsDiscovery,
    eSmeCommandTdlsLinkSetup,
    eSmeCommandTdlsLinkTear,
    eSmeCommandTdlsEnterUapsd,
    eSmeCommandTdlsExitUapsd,
#endif
#endif
    //PMC
    eSmePmcCommandMask = 0x20000, //To identify PMC commands
    eSmeCommandEnterImps,
    eSmeCommandExitImps,
    eSmeCommandEnterBmps,
    eSmeCommandExitBmps,
    eSmeCommandEnterUapsd,
    eSmeCommandExitUapsd,
    eSmeCommandEnterWowl,
    eSmeCommandExitWowl,
    eSmeCommandEnterStandby,
    //QOS
    eSmeQosCommandMask = 0x40000,  //To identify Qos commands
    eSmeCommandAddTs,
    eSmeCommandDelTs,
#ifdef FEATURE_OEM_DATA_SUPPORT
    eSmeCommandOemDataReq = 0x80000, //To identify the oem data commands
#endif
    eSmeCommandRemainOnChannel,
    eSmeCommandNoAUpdate,
} eSmeCommandType;


typedef enum eSmeState
{
    SME_STATE_STOP,
    SME_STATE_START,
    SME_STATE_READY,
} eSmeState;

#if  defined (WLAN_FEATURE_VOWIFI_11R) || defined (FEATURE_WLAN_CCX) || defined(FEATURE_WLAN_LFR)
/* enumeration for Korea country revision index,
   index to the list of valid channels */
typedef enum eSmeKRRevision
{
    SME_KR_3         = 3,
    SME_KR_24        = 24,
    SME_KR_25        = 25,
} eSmeKRRevision;
#endif

#define SME_IS_START(pMac)  (SME_STATE_STOP != (pMac)->sme.state)
#define SME_IS_READY(pMac)  (SME_STATE_READY == (pMac)->sme.state)


typedef struct tagSmeStruct
{
    eSmeState state;
    vos_lock_t lkSmeGlobalLock;
    tANI_U32 totalSmeCmd;
    void *pSmeCmdBufAddr;
    tDblLinkList smeCmdActiveList;
    tDblLinkList smeCmdPendingList;
    tDblLinkList smeCmdFreeList;   //preallocated roam cmd list
    void (*pTxPerHitCallback) (void *pCallbackContext); /* callback for Tx PER hit to HDD */ 
    void *pTxPerHitCbContext;
    tVOS_CON_MODE currDeviceMode;
#ifdef FEATURE_WLAN_LPHB
    void (*pLphbWaitTimeoutCb) (void *pAdapter, void *indParam);
#endif /* FEATURE_WLAN_LPHB */
} tSmeStruct, *tpSmeStruct;


#endif //#if !defined( __SMEINTERNAL_H )
