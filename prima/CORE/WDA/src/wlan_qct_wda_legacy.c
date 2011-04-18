
/*===========================================================================

                       wlan_qct_wda_legacy.c

  OVERVIEW:

  This software unit holds the implementation of the WLAN Device Adaptation
  Layer for the legacy functionalities that were part of the old HAL.

  The functions externalized by this module are to be called ONLY by other
  WLAN modules that properly register with the Transport Layer initially.

  DEPENDENCIES:

  Are listed for each API below.


  Copyright (c) 2008 QUALCOMM Incorporated.
  All Rights Reserved.
  Qualcomm Confidential and Proprietary
===========================================================================*/

/* Standard include files */
/* Application Specific include files */
#include "limApi.h"
#include "pmmApi.h"
#include "cfgApi.h"
#include "wlan_qct_wda_debug.h"

/* Locally used Defines */

#define HAL_MMH_MB_MSG_TYPE_MASK    0xFF00

// -------------------------------------------------------------
/**
 * wdaPostCtrlMsg
 *
 * FUNCTION:
 *     Posts WDA messages to MC thread
 *
 * LOGIC:
 *
 * ASSUMPTIONS:pl
 *
 *
 * NOTE:
 *
 * @param tpAniSirGlobal MAC parameters structure
 * @param pMsg pointer with message
 * @return Success or Failure
 */

tSirRetStatus
wdaPostCtrlMsg(tpAniSirGlobal pMac, tSirMsgQ *pMsg)
{
   if(VOS_STATUS_SUCCESS != vos_mq_post_message(VOS_MQ_ID_WDA, (vos_msg_t *) pMsg))
      return eSIR_FAILURE;
   else
      return eSIR_SUCCESS;
} // halPostMsg()

/**
 * wdaPostCfgMsg
 *
 * FUNCTION:
 *     Posts MNT messages to gSirMntMsgQ
 *
 * LOGIC:
 *
 * ASSUMPTIONS:
 *
 *
 * NOTE:
 *
 * @param tpAniSirGlobal MAC parameters structure
 * @param pMsg A pointer to the msg
 * @return Success or Failure
 */

tSirRetStatus
wdaPostCfgMsg(tpAniSirGlobal pMac, tSirMsgQ *pMsg)
{
   tSirRetStatus rc = eSIR_SUCCESS;

   do
   {
#ifdef ANI_OS_TYPE_RTAI_LINUX

      // Posts message to the queue

      if (tx_queue_send(&pMac->sys.gSirMntMsgQ, pMsg,
                       TX_NO_WAIT) != TX_SUCCESS)
      {
         wdaLog(pMac, LOGP, FL("Queue send Failed! rc (%X)\n"),
                eSIR_SYS_TX_Q_SEND_FAILED);
         rc = eSIR_SYS_TX_Q_SEND_FAILED;
         break;
      }

#else
      // For Windows based MAC, instead of posting message to different
      // queues we will call the handler routines directly

      cfgProcessMbMsg(pMac, (tSirMbMsg*)pMsg->bodyptr);
      rc = eSIR_SUCCESS;
#endif
   } while (0);

   return rc;
} // halMntPostMsg()


#if defined(ANI_MANF_DIAG) || defined(ANI_PHY_DEBUG)
#include "pttModuleApi.h"
// -------------------------------------------------------------
/**
 * halNimPTTPostMsgApi
 *
 * FUNCTION:
 *     Posts NIM messages to gNIM thread
 *
 * LOGIC:
 *
 * ASSUMPTIONS:pl
 *
 *
 * NOTE:
 *
 * @param tpAniSirGlobal MAC parameters structure
 * @param pMsg pointer with message
 * @return Success or Failure
 */

tSirRetStatus
halNimPTTPostMsgApi(tpAniSirGlobal pMac, tSirMsgQ *pMsg)
{
   tSirRetStatus rc = eSIR_SUCCESS;

   do
   {
#ifdef ANI_OS_TYPE_RTAI_LINUX

      // Posts message to the queue
      if (tx_queue_send(&pMac->sys.gSirNimRDMsgQ, pMsg,
                       TX_NO_WAIT) != TX_SUCCESS)
      {
         rc = eSIR_FAILURE;
         wdaLog(pMac, LOGP,
                FL("Posting a Msg to nimMsgQ failed!\n"));
         break;
      }
#else
      // For Windows based MAC, instead of posting message to different
      // queues, we will call the handler routines directly
      wdaLog(pMac, LOGE, "ERROR: Received PTT message in obsolete code path.\n");
      wdaLog(pMac, LOGP, "This indicates that the wrong OID is being used - clean registry and previous inf files.\n");
      /*
      tPttMsgbuffer *msgPtr = (tPttMsgbuffer *)(pMsg->body);  //for some reason, body is actually being used as if it were a void *
      pttProcessMsg(pMac, msgPtr);
      */

      //TODO: the resonse is now packaged in ((tPttMsgbuffer *)&pMsg->body)->msgResponse and needs to be sent back to the application

      rc = eSIR_SUCCESS;
#endif
   }
   while (0);

   return rc;
} // halNimPTTPostMsgApi()


#endif  //ANI_MANF_DIAG

// -------------------------------------------------------------
/**
 * uMacPostCtrlMsg
 *
 * FUNCTION:
 *     Forwards the completely received message to the respective
 *    modules for further processing.
 *
 * LOGIC:
 *
 * ASSUMPTIONS:
 *    Freeing up of the message buffer is left to the destination module.
 *
 * NOTE:
 *  This function has been moved to the API file because for MAC running
 *  on Windows host, the host module will call this routine directly to
 *  send any mailbox messages. Making this function an API makes sure that
 *  outside world (any module outside MMH) only calls APIs to use MMH
 *  services and not an internal function.
 *
 * @param pMb A pointer to the maibox message
 * @return NONE
 */

tSirRetStatus uMacPostCtrlMsg(void* pSirGlobal, tSirMbMsg* pMb)
{
   tSirMsgQ msg;
   tpAniSirGlobal pMac = (tpAniSirGlobal)pSirGlobal;

#ifdef ANI_OS_TYPE_RTAI_LINUX

   msg.type = pMb->type;
   msg.bodyptr = pMb;
   msg.bodyval = 0;
   WDALOG3( wdaLog(pMac, LOG3, FL("msgType %d, msgLen %d\n" ),
        pMb->type, pMb->msgLen));
#else

   tSirMbMsg* pMbLocal;
   msg.type = pMb->type;
   msg.bodyval = 0;

   WDALOG3(wdaLog(pMac, LOG3, FL("msgType %d, msgLen %d\n" ),
        pMb->type, pMb->msgLen));

   // copy the message from host buffer to firmware buffer
   // this will make sure that firmware allocates, uses and frees
   // it's own buffers for mailbox message instead of working on
   // host buffer

   // second parameter, 'wait option', to palAllocateMemory is ignored on Windows
   if( eHAL_STATUS_SUCCESS != palAllocateMemory( pMac->hHdd, (void **)&pMbLocal, pMb->msgLen))
   {
      WDALOGE( wdaLog(pMac, LOGE, FL("Buffer Allocation failed!\n")));
      return eSIR_FAILURE;
   }

   palCopyMemory(pMac, (void *)pMbLocal, (void *)pMb, pMb->msgLen);
   msg.bodyptr = pMbLocal;
#endif

   switch (msg.type & HAL_MMH_MB_MSG_TYPE_MASK)
   {
   case WDA_MSG_TYPES_BEGIN:    // Posts a message to the HAL MsgQ
      wdaPostCtrlMsg(pMac, &msg);
      break;

   case SIR_LIM_MSG_TYPES_BEGIN:    // Posts a message to the LIM MsgQ
      limPostMsgApi(pMac, &msg);
      break;

   case SIR_CFG_MSG_TYPES_BEGIN:    // Posts a message to the CFG MsgQ
      wdaPostCfgMsg(pMac, &msg);
      break;

   case SIR_PMM_MSG_TYPES_BEGIN:    // Posts a message to the PMM MsgQ
      pmmPostMessage(pMac, &msg);
      break;

#if defined(ANI_MANF_DIAG) || defined(ANI_PHY_DEBUG)
   case SIR_PTT_MSG_TYPES_BEGIN:
      halNimPTTPostMsgApi(pMac, &msg); // Posts a message to the NIM PTT MsgQ
      break;

#endif

   default:
      WDALOGW( wdaLog(pMac, LOGW, FL("Unknown message type = " \
             "0x%X\n"),
             msg.type));

      // Release the memory.
      if (palFreeMemory( pMac->hHdd, (void*)(msg.bodyptr)) != eSIR_SUCCESS)
      {
         WDALOGE( wdaLog(pMac, LOGE, FL("Buffer Allocation failed!\n")));
         return eSIR_FAILURE;
      }
      break;
   }

   return eSIR_SUCCESS;

} // uMacPostCtrlMsg()


/* ---------------------------------------------------------
 * FUNCTION:  wdaGetGlobalSystemRole()
 *
 * Get the global HAL system role. 
 * ---------------------------------------------------------
 */
tBssSystemRole wdaGetGlobalSystemRole(tpAniSirGlobal pMac)
{
   WDALOGE( wdaLog(pMac, LOGE, FL(" already return hardcoded STA role\n")));
   return  eSYSTEM_STA_ROLE;
}

