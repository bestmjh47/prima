/*
 * Copyright (c) 2012 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */
/**=========================================================================
  
  \file  wlan_qct_pal_msg.c
  
  \brief Implementation message APIs PAL exports. wpt = (Wlan Pal Type) wpal = (Wlan PAL)
               
   Definitions for platform with legacy UMAC support.
  
   Copyright 2010 (c) Qualcomm, Incorporated.  All Rights Reserved.
   
   Qualcomm Confidential and Proprietary.
  
  ========================================================================*/

#include "wlan_qct_pal_msg.h"
#include "wlan_qct_pal_api.h"
#include "wlan_qct_pal_trace.h"
#include "vos_mq.h"



/*---------------------------------------------------------------------------
     wpalPostCtrlMsg - Post a message to control context so it can be processed in that context.
    Param: 
        pPalContext - A PAL context
        pMsg - a pointer to called allocated opaque object;
---------------------------------------------------------------------------*/
wpt_status wpalPostCtrlMsg(void *pPalContext, wpt_msg *pMsg)
{
   wpt_status status = eWLAN_PAL_STATUS_E_FAILURE;
   vos_msg_t msg;

   if (NULL == pMsg)
   {
      WPAL_TRACE(eWLAN_MODULE_PAL, eWLAN_PAL_TRACE_LEVEL_ERROR,
                 "%s: NULL message pointer", __func__);
      WPAL_ASSERT(0);
      return status;
   }

   msg.type = 0;  //This field is not used because VOSS doesn't check it.
   msg.reserved = 0;
   msg.bodyval = 0;
   msg.bodyptr = pMsg;
   if(VOS_IS_STATUS_SUCCESS(vos_mq_post_message(VOS_MQ_ID_WDI, &msg)))
   {
      status = eWLAN_PAL_STATUS_SUCCESS;
   }
   else
   {
      WPAL_TRACE(eWLAN_MODULE_PAL, eWLAN_PAL_TRACE_LEVEL_ERROR, "%s fail to post msg %d\n",
                  __func__, pMsg->type);
   }

   return status;
}



/*---------------------------------------------------------------------------
     wpalPostTxMsg - Post a message to TX context so it can be processed in that context.
    Param: 
        pPalContext - A PAL context PAL
        pMsg - a pointer to called allocated opaque object;
---------------------------------------------------------------------------*/
wpt_status wpalPostTxMsg(void *pPalContext, wpt_msg *pMsg)
{
   wpt_status status = eWLAN_PAL_STATUS_E_FAILURE;
   vos_msg_t msg;

   if (NULL == pMsg)
   {
      WPAL_TRACE(eWLAN_MODULE_PAL, eWLAN_PAL_TRACE_LEVEL_ERROR,
                 "%s: NULL message pointer", __func__);
      WPAL_ASSERT(0);
      return status;
   }

   msg.type = 0; //This field is not used because VOSS doesn't check it.
   msg.reserved = 0;
   msg.bodyval = 0;
   msg.bodyptr = pMsg;
   if(VOS_IS_STATUS_SUCCESS(vos_tx_mq_serialize(VOS_MQ_ID_WDI, &msg)))
   {
      status = eWLAN_PAL_STATUS_SUCCESS;
   }
   else
   {
      WPAL_TRACE(eWLAN_MODULE_PAL, eWLAN_PAL_TRACE_LEVEL_ERROR, "%s fail to post msg %d\n",
                  __func__, pMsg->type);
   }

   return status;
}


/*---------------------------------------------------------------------------
     wpalPostRxMsg - Post a message to RX context so it can be processed in that context.
    Param: 
        pPalContext - A PAL context
        pMsg - a pointer to called allocated opaque object;
---------------------------------------------------------------------------*/
wpt_status wpalPostRxMsg(void *pPalContext, wpt_msg *pMsg)
{
   wpt_status status = eWLAN_PAL_STATUS_E_FAILURE;
   vos_msg_t msg;

   if (NULL == pMsg)
   {
      WPAL_TRACE(eWLAN_MODULE_PAL, eWLAN_PAL_TRACE_LEVEL_ERROR,
                 "%s: NULL message pointer", __func__);
      WPAL_ASSERT(0);
      return status;
   }

   msg.type = 0; //This field is not used because VOSS doesn't check it.
   msg.reserved = 0;
   msg.bodyval = 0;
   msg.bodyptr = pMsg;
   if(VOS_IS_STATUS_SUCCESS(vos_rx_mq_serialize(VOS_MQ_ID_WDI, &msg)))
   {
      status = eWLAN_PAL_STATUS_SUCCESS;
   }
   else
   {
      WPAL_TRACE(eWLAN_MODULE_PAL, eWLAN_PAL_TRACE_LEVEL_ERROR, "%s fail to post msg %d\n",
                  __func__, pMsg->type);
   }

   return status;
}

