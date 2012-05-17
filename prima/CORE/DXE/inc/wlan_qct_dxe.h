/*
* Copyright (c) 2012 Qualcomm Atheros, Inc.
* All Rights Reserved.
* Qualcomm Atheros Confidential and Proprietary.
*/

#ifndef WLAN_QCT_DXE_H
#define WLAN_QCT_DXE_H

/**=========================================================================
  
  @file  wlan_qct_dxe.h
  
  @brief 
               
   This file contains the external API exposed by the wlan data transfer abstraction layer module.
   Copyright (c) 2008 QUALCOMM Incorporated. All Rights Reserved.
   Qualcomm Confidential and Proprietary
========================================================================*/

/*===========================================================================

                      EDIT HISTORY FOR FILE


  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.


  $Header:$ $DateTime: $ $Author: $


when           who        what, where, why
--------    ---         ----------------------------------------------------------
08/03/10    schang      Created module.

===========================================================================*/

/*===========================================================================

                          INCLUDE FILES FOR MODULE

===========================================================================*/

/*----------------------------------------------------------------------------
 * Include Files
 * -------------------------------------------------------------------------*/
#include "wlan_qct_pal_api.h"
#include "wlan_qct_pal_packet.h"
#include "wlan_qct_pal_status.h"
#include "wlan_qct_pal_type.h"
#include "wlan_qct_pal_msg.h"
#include "wlan_qct_pal_sync.h"
#include "wlan_qct_wdi_dts.h"

/*----------------------------------------------------------------------------
 *  Type Declarations
 * -------------------------------------------------------------------------*/
/* DXE Descriptor contents SWAP option flag */

//#define WLANDXE_ENDIAN_SWAP_ENABLE

/* Default RX OS frame buffer size
 * Size must be same with Vos Packet Size */
#define WLANDXE_DEFAULT_RX_OS_BUFFER_SIZE  1792

/*The maximum number of packets that can be chained in dxe for the Low 
  priority channel
  Note: Increased it to 240 from 128 for Windows(EA) becase Windows is
  able to push 2~6 packet chain in one NET_BUFFER. It causes TX low
  resource condition more easily than LA. It ends up to cause low 
  throughut number and spend more CPU time*/
#ifdef WINDOWS_DT
#define WLANDXE_LO_PRI_RES_NUM 240
#else
#define WLANDXE_LO_PRI_RES_NUM 128
#endif


/*The maximum number of packets that can be chained in dxe for the HI 
  priority channel */
#define WLANDXE_HI_PRI_RES_NUM 10

typedef enum
{
   WLANDXE_POWER_STATE_FULL,
   WLANDXE_POWER_STATE_IMPS,
   WLANDXE_POWER_STATE_BMPS,
   WLANDXE_POWER_STATE_BMPS_PENDING,
   WLANDXE_POWER_STATE_DOWN,
   WLANDXE_POWER_STATE_MAX
} WLANDXE_PowerStateType;

typedef enum
{
   WLANDXE_RIVA_POWER_STATE_ACTIVE,
   WLANDXE_RIVA_POWER_STATE_IMPS_UNKNOWN,
   WLANDXE_RIVA_POWER_STATE_BMPS_UNKNOWN,
   WLANDXE_RIVA_POWER_STATE_DOWN_UNKNOWN,
   WLANDXE_RIVA_POWER_STATE_MAX
} WLANDXE_RivaPowerStateType;

/*==========================================================================
  @  Type Name
      WLANDXE_RxFrameReadyCbType 

  @  Description 
       RX Frame Ready indication CB

  @  Parameters
         pVoid         pAdaptor : Driver global control block pointer
         palPacket     pRXFramePtr : Received Frame Pointer
         pVoid         userCtxt : DTS user contect pointer

  @  Return
        wpt_status
===========================================================================*/
typedef WDTS_RxFrameReadyCbType WLANDXE_RxFrameReadyCbType;

/*==========================================================================
  @  Type Name
       WLANDXE_TxCompleteCbType 

  @  Description 
      TX complete indication CB

  @  Parameters
         pVoid      pAdaptor : Driver global control block pointer
         void       pTXFramePtr : Completed TX Frame Pointer
         pVoid      userCtxt : DTS user contect pointer

  @  Return
       wpt_status
===========================================================================*/
typedef WDTS_TxCompleteCbType WLANDXE_TxCompleteCbType;

/*==========================================================================
  @  Type Name
      WLANDXE_LowResourceCbType 

  @  Description 
       DXE Low resource indication CB

  @  Parameters
      pVoid      pAdaptor : Driver global control block pointer
      BOOL      lowResourceCondition : DXE low resource or not
      pVoid      userCtxt : DTS user contect pointer

  @  Return
      wpt_status
===========================================================================*/
typedef WDTS_LowResourceCbType WLANDXE_LowResourceCbType;

/*==========================================================================
  @  Type Name
      WLANDXE_SetPowerStateCbType 

  @  Description 
       DXE Set power state ACK callback. This callback function should be
       invoked by the DXE to notify WDI that set power state request is complete

  @  Parameters
      status    status of the set operation
      pUserData Cookie that should be passed back to the caller along with the
                callback.

  @  Return
      None
===========================================================================*/
typedef WDTS_SetPSCbType  WLANDXE_SetPowerStateCbType;

/*-------------------------------------------------------------------------
 *Function declarations and documenation
 *-------------------------------------------------------------------------*/
/*==========================================================================
  @  Function Name 
      WLANDXE_Open

  @  Description 
      Open host DXE driver, allocate DXE resources
      Allocate, DXE local control block, DXE descriptor pool, DXE descriptor control block pool

  @  Parameters
      pVoid      pAdaptor : Driver global control block pointer

  @  Return
      pVoid DXE local module control block pointer
===========================================================================*/
void *WLANDXE_Open
(
   void
);

/*==========================================================================
  @  Function Name 
      WLANDXE_ClientRegistration

  @  Description 
      Make callback functions registration into DXE driver from DXE driver client

  @  Parameters
      pVoid                       pDXEContext : DXE module control block
      WDTS_RxFrameReadyCbType     rxFrameReadyCB : RX Frame ready CB function pointer
      WDTS_TxCompleteCbType       txCompleteCB : TX complete CB function pointer
      WDTS_LowResourceCbType      lowResourceCB : Low DXE resource notification CB function pointer
      void                       *userContext : DXE Cliennt control block

  @  Return
      wpt_status
===========================================================================*/
wpt_status WLANDXE_ClientRegistration
(
   void                       *pDXEContext,
   WDTS_RxFrameReadyCbType     rxFrameReadyCB,
   WDTS_TxCompleteCbType       txCompleteCB,
   WDTS_LowResourceCbType      lowResourceCB,
   void                       *userContext
);

/*==========================================================================
  @  Function Name 
      WLANDXE_Start

  @  Description 
      Start Host DXE driver
      Initialize DXE channels and start channel

  @  Parameters
      pVoid                       pDXEContext : DXE module control block

  @  Return
      wpt_status
===========================================================================*/
wpt_status WLANDXE_Start
(
   void  *pDXEContext
);

/*==========================================================================
  @  Function Name 
      WLANDXE_TXFrame

  @  Description 
      Trigger frame transmit from host to RIVA

  @  Parameters
      pVoid            pDXEContext : DXE Control Block
      wpt_packet       pPacket : transmit packet structure
      WDTS_ChannelType channel : TX channel

  @  Return
      wpt_status
===========================================================================*/
wpt_status WLANDXE_TxFrame
(
   void                 *pDXEContext,
   wpt_packet           *pPacket,
   WDTS_ChannelType      channel
);


/*==========================================================================
  @  Function Name 
      WLANDXE_CompleteTX

  @  Description 
      Informs DXE that the current series of Tx packets is complete

  @  Parameters
      pDXEContext : DXE Control Block
      ucTxResReq          TX resource number required by TL/WDI

  @  Return
      wpt_status
===========================================================================*/
wpt_status
WLANDXE_CompleteTX
(
  void* pDXEContext,
  wpt_uint32 ucTxResReq
);

/*==========================================================================
  @  Function Name 
      WLANDXE_Stop

  @  Description 
      Stop DXE channels and DXE engine operations

  @  Parameters
      pVoid            pDXEContext : DXE Control Block

  @  Return
      wpt_status
===========================================================================*/
wpt_status WLANDXE_Stop
(
   void *pDXEContext
);

/*==========================================================================
  @  Function Name 
      WLANDXE_Close

  @  Description 
      Close DXE channels
      Free DXE related resources
      DXE descriptor free
      Descriptor control block free
      Pre allocated RX buffer free

  @  Parameters
      pVoid            pDXEContext : DXE Control Block

  @  Return
      wpt_status
===========================================================================*/
wpt_status WLANDXE_Close
(
   void *pDXEContext
);

/*==========================================================================
  @  Function Name 
      WLANDXE_TriggerTX

  @  Description 
      TBD

  @  Parameters
      pVoid            pDXEContext : DXE Control Block

  @  Return
      wpt_status
===========================================================================*/
wpt_status WLANDXE_TriggerTX
(
   void *pDXEContext
);

/*==========================================================================
  @  Function Name 
      WLANDXE_SetPowerState

  @  Description 
      From Client let DXE knows what is the WLAN HW(RIVA) power state

  @  Parameters
      pVoid                    pDXEContext : DXE Control Block
      WLANDXE_PowerStateType   powerState

  @  Return
      wpt_status
===========================================================================*/
wpt_status WLANDXE_SetPowerState
(
   void                    *pDXEContext,
   WDTS_PowerStateType      powerState,
   WDTS_SetPSCbType         cBack
);

/*==========================================================================
  @  Function Name 
      WLANDXE_GetFreeTxDataResNumber

  @  Description 
      Returns free descriptor numbers for TX data channel (TX high priority)

  @  Parameters
      pVoid            pDXEContext : DXE Control Block

  @  Return
      wpt_uint32      Free descriptor number of TX high pri ch
===========================================================================*/
wpt_uint32 WLANDXE_GetFreeTxDataResNumber
(
   void *pDXEContext
);

#ifdef WLANDXE_TEST_CHANNEL_ENABLE
/*==========================================================================
  @  Function Name 
      WLANDXE_UnitTest

  @  Description 
      Temporary for the DXE module test

  @  Parameters
      NONE

  @  Return
      NONE

===========================================================================*/
void WLANDXE_UnitTestStartDXE
(
   void
);

/*==========================================================================
  @  Function Name 

  @  Description 

  @  Parameters

  @  Return

===========================================================================*/
void WLANDXE_UnitTestDataTransfer
(
   void
);

/*==========================================================================
  @  Function Name 

  @  Description 

  @  Parameters

  @  Return

===========================================================================*/
void WLANDXE_UnitTestEventHandle
(
   void     *dxeCB
);
#endif /* WLANDXE_TEST_CHANNEL_ENABLE */
#endif /* WLAN_QCT_DXE_H */
