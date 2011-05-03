/*===========================================================================

                       W L A N _ Q C T _ C T S . C

  OVERVIEW:

  This software unit holds the implementation of the WLAN Control Transport
  Service Layer.

  The functions externalized by this module are to be called by the DAL Core
  that wishes to use a platform agnostic API to communicate with the WLAN SS.

  DEPENDENCIES:

  Are listed for each API below.


  Copyright (c) 2010-2011 QUALCOMM Incorporated.
  All Rights Reserved.
  Qualcomm Confidential and Proprietary
===========================================================================*/

/*===========================================================================

                      EDIT HISTORY FOR FILE


  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.


  $Header$$DateTime$$Author$


  when          who    what, where, why
  ----------    ---    --------------------------------------------------------
  2011-02-28    jtj    Linux/Android implementation which utilizes SMD
  2010-08-09    mss    Created module

===========================================================================*/



/*===========================================================================

                          INCLUDE FILES FOR MODULE

===========================================================================*/

/*----------------------------------------------------------------------------
 * Include Files
 * -------------------------------------------------------------------------*/
#include "wlan_qct_wdi_cts.h"
#include "wlan_qct_pal_msg.h"
#include "wlan_qct_pal_api.h"
#include "wlan_qct_pal_trace.h"
#include "wlan_qct_os_list.h"
#include "wlan_qct_wdi.h"
#include "wlan_qct_wdi_i.h"
#ifdef CONFIG_ANDROID
#include <mach/msm_smd.h>
#else
#include "msm_smd.h"
#endif

/*----------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * -------------------------------------------------------------------------*/

/* Magic value to validate a WCTS CB (value is little endian ASCII: WCTS */
#define WCTS_CB_MAGIC     0x53544357

/*----------------------------------------------------------------------------
 * Type Declarations
 * -------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------
   WCTS_StateType
 ---------------------------------------------------------------------------*/
typedef enum
{
   WCTS_STATE_CLOSED,       /* Closed */
   WCTS_STATE_OPEN_PENDING, /* Waiting for the OPEN event from SMD */
   WCTS_STATE_OPEN,         /* Open event received from SMD */
   WCTS_STATE_DEFERRED,     /* Write pending, SMD chennel is full */
   WCTS_STATE_MAX
} WCTS_StateType;

/*---------------------------------------------------------------------------
   Control Transport Control Block Type
 ---------------------------------------------------------------------------*/
typedef struct
{
   WCTS_NotifyCBType      wctsNotifyCB;
   void*                  wctsNotifyCBData;
   WCTS_RxMsgCBType       wctsRxMsgCB;
   void*                  wctsRxMsgCBData;
   WCTS_StateType         wctsState;
   smd_channel_t*         wctsChannel;
   wpt_list               wctsPendingQueue;
   wpt_uint32             wctsMagic;
   wpt_msg                wctsOpenMsg;
   wpt_msg                wctsDataMsg;
   wpt_msg                wctsCloseMsg;
} WCTS_ControlBlockType;

/*---------------------------------------------------------------------------
   WDI CTS Buffer Type
 ---------------------------------------------------------------------------*/
typedef struct
{
   /*Node for linking pending buffers into a linked list */
   wpt_list_node          node;

   /*Buffer associated with the request */
   void*                  pBuffer;

   /*Buffer Size*/
   int                    bufferSize;

} WCTS_BufferType;

/*----------------------------------------------------------------------------
 * Global Data Definitions
 * -------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 * Static Variable Definitions
 * -------------------------------------------------------------------------*/
#ifdef FEATURE_R33D
/* R33D will not close SMD port
 * If receive close request from WDI, just pretend as port closed,
 * Store control block info static memory, and reuse next open */
static WCTS_ControlBlockType  *ctsCB;

/* If port open once, not try to actual open next time */
static int                     port_open = 0;
#endif /* FEATURE_R33D */
/*----------------------------------------------------------------------------
 * Static Function Declarations and Definitions
 * -------------------------------------------------------------------------*/



/**
 @brief    Callback function for serializing WCTS Open
           processing in the control context
 @param

    pMsg - pointer to the message

 @see
 @return void
*/
static void
WCTS_PALOpenCallback
(
   wpt_msg *pMsg
)
{
   WCTS_ControlBlockType*        pWCTSCb;
   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

   /* extract our context from the message */
   pWCTSCb = pMsg->pContext;

   /*--------------------------------------------------------------------
     Sanity check
     --------------------------------------------------------------------*/
   if ((NULL == pWCTSCb) || (WCTS_CB_MAGIC != pWCTSCb->wctsMagic)) {
      WPAL_TRACE(eWLAN_MODULE_DAL_CTRL, eWLAN_PAL_TRACE_LEVEL_ERROR,
                 "WCTS_PALOpenCallback: Invalid parameters received.");
      return;
   }

   if (WCTS_STATE_OPEN_PENDING != pWCTSCb->wctsState) {
      WPAL_TRACE(eWLAN_MODULE_DAL_CTRL, eWLAN_PAL_TRACE_LEVEL_ERROR,
                 "WCTS_PALOpenCallback: Invoke from invalid state %d.",
                 pWCTSCb->wctsState);
      return;
   }

   /* notified registered client that the channel is open */
   pWCTSCb->wctsState = WCTS_STATE_OPEN;
   pWCTSCb->wctsNotifyCB((WCTS_HandleType)pWCTSCb,
                        WCTS_EVENT_OPEN,
                        pWCTSCb->wctsNotifyCBData);

}/*WCTS_PALOpenCallback*/



/**
 @brief    Callback function for serializing WCTS Close
           processing in the control context
 @param

    pMsg - pointer to the message

 @see
 @return void
*/
static void
WCTS_PALCloseCallback
(
   wpt_msg *pMsg
)
{
   WCTS_ControlBlockType*        pWCTSCb;
   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

   /* extract our context from the message */
   pWCTSCb = pMsg->pContext;

   /*--------------------------------------------------------------------
     Sanity check
     --------------------------------------------------------------------*/
   if ((NULL == pWCTSCb) || (WCTS_CB_MAGIC != pWCTSCb->wctsMagic)) {
      WPAL_TRACE(eWLAN_MODULE_DAL_CTRL, eWLAN_PAL_TRACE_LEVEL_ERROR,
                 "WCTS_PALCloseCallback: Invalid parameter received.");
      return;
   }

   /* notified registered client that the channel is closed */
   pWCTSCb->wctsState = WCTS_STATE_CLOSED;
   pWCTSCb->wctsNotifyCB((WCTS_HandleType)pWCTSCb,
                         WCTS_EVENT_CLOSE,
                         pWCTSCb->wctsNotifyCBData);

   /* release the resource */
   pWCTSCb->wctsMagic = 0;
   wpalMemoryFree(pWCTSCb);

} /*WCTS_PALCloseCallback*/



/**
 @brief    Callback function for serializing WCTS Read
           processing in the control context

 @param    pWCTSCb  WCTS Control Block

 @see
 @return void
*/
static void
WCTS_PALReadCallback
(
   WCTS_ControlBlockType*  pWCTSCb
)
{
   void* buffer;
   int packet_size;
   int available;
   int bytes_read;

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

   /*--------------------------------------------------------------------
     Sanity check
     --------------------------------------------------------------------*/
   if ((NULL == pWCTSCb) || (WCTS_CB_MAGIC != pWCTSCb->wctsMagic)) {
      WPAL_TRACE(eWLAN_MODULE_DAL_CTRL, eWLAN_PAL_TRACE_LEVEL_ERROR,
                 "WCTS_PALReadCallback: Invalid parameter received.");
      return;
   }

   /* iterate until no more packets are available */
   while (1) {
      /* check the length of the next packet */
      packet_size = smd_cur_packet_size(pWCTSCb->wctsChannel);
      if (0 == packet_size) {
         /* No more data to be read */
         return;
      }

      /* Check how much of the data is available */
      available = smd_read_avail(pWCTSCb->wctsChannel);
      if (available < packet_size) {
         /* Entire packet not yet ready to be read --
            There will be another notification when it is ready */
         return;
      }

      buffer = wpalMemoryAllocate(packet_size);
      if (NULL ==  buffer) {
         WPAL_TRACE(eWLAN_MODULE_DAL_CTRL, eWLAN_PAL_TRACE_LEVEL_ERROR,
                    "WCTS_PALReadCallback: Memory allocation failure");
         WPAL_ASSERT(0);
         return;
      }

      bytes_read = smd_read(pWCTSCb->wctsChannel,
                            buffer,
                            packet_size);

      if (bytes_read != packet_size) {
         /*Some problem, do not forward it to WDI.*/
         WPAL_TRACE(eWLAN_MODULE_DAL_CTRL, eWLAN_PAL_TRACE_LEVEL_ERROR,
                    "WCTS_PALReadCallback: Failed to read data from SMD");
         wpalMemoryFree(buffer);
         WPAL_ASSERT(0);
         return;
      }

      /* forward the message to the registered handler */
      pWCTSCb->wctsRxMsgCB((WCTS_HandleType)pWCTSCb,
                           buffer,
                           packet_size,
                           pWCTSCb->wctsRxMsgCBData);

      /* Free the allocated buffer*/
      wpalMemoryFree(buffer);
   }

} /*WCTS_PALReadCallback*/



/**
 @brief    Callback function for serializing WCTS Write
           processing in the control context

 @param    pWCTSCb  WCTS Control Block

 @see
 @return void
*/
static void
WCTS_PALWriteCallback
(
   WCTS_ControlBlockType*  pWCTSCb
)
{
   wpt_list_node*      pNode;
   WCTS_BufferType*    pBufferQueue;
   void*               pBuffer;
   int                 len;
   int                 available;
   int                 written;

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

   /*--------------------------------------------------------------------
     Sanity check
     --------------------------------------------------------------------*/
   if ((NULL == pWCTSCb) || (WCTS_CB_MAGIC != pWCTSCb->wctsMagic)) {
      WPAL_TRACE(eWLAN_MODULE_DAL_CTRL, eWLAN_PAL_TRACE_LEVEL_ERROR,
                 "WCTS_PALWriteCallback: Invalid parameter received.");
      return;
   }

   /* if we are not deferred, then there are no pending packets */
   if (WCTS_STATE_DEFERRED != pWCTSCb->wctsState) {
      return;
   }

   /* Keep sending deferred messages as long as there is room in
      the channel.  Note that we initially peek at the head of the
      list to access the parameters for the next message; we don't
      actually remove the next message from the deferred list until
      we know the channel can handle it */
   while (eWLAN_PAL_STATUS_SUCCESS ==
          wpal_list_peek_front(&pWCTSCb->wctsPendingQueue, &pNode)) {
      pBufferQueue = container_of(pNode, WCTS_BufferType, node);
      pBuffer = pBufferQueue->pBuffer;
      len = pBufferQueue->bufferSize;

      available = smd_write_avail(pWCTSCb->wctsChannel);
      if (available < len) {
         /* channel has no room for the next packet so we are done */
         return;
      }

      /* there is room for the next message, so we can now remove
         it from the deferred message queue and send it */
      wpal_list_remove_front(&pWCTSCb->wctsPendingQueue, &pNode);

      /* note that pNode will be the same as when we peeked, so
         there is no need to update pBuffer or len */

      written = smd_write(pWCTSCb->wctsChannel, pBuffer, len);
      if (written != len) {
         /* Something went wrong */
         WPAL_TRACE(eWLAN_MODULE_DAL_CTRL, eWLAN_PAL_TRACE_LEVEL_ERROR,
                    "WCTS_PALWriteCallback: channel write failure");

         /* we were unable to send the message that was at the head
            of the deferred list.  there is nothing else we can do
            other than drop it, so we will just fall through to the
            "success" processing.
            hopefully the client can recover from this since there is
            nothing else we can do here */
      }

      /* whether we had success or failure, reclaim all memory */
      wpalMemoryFree(pBuffer);
      wpalMemoryFree(pBufferQueue);

      /* we'll continue to iterate until the channel is full or all
         of the deferred messages have been sent */
   }

   /* if we've exited the loop, then we have drained the deferred queue.
      set the state to indicate we are no longer deferred, and turn off
      the remote read interrupt */
   pWCTSCb->wctsState = WCTS_STATE_OPEN;
   smd_disable_read_intr(pWCTSCb->wctsChannel);

} /*WCTS_PALWriteCallback*/



/**
 @brief    Callback function for serializing SMD DATA Event
           processing in the control context
 @param

    pMsg - pointer to the message

 @see
 @return void
*/
static void
WCTS_PALDataCallback
(
   wpt_msg *pMsg
)
{
   WCTS_ControlBlockType*        pWCTSCb;
   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

   /* extract our context from the message */
   pWCTSCb = pMsg->pContext;

   /* process any incoming messages */
   WCTS_PALReadCallback(pWCTSCb);

   /* send any deferred messages */
   WCTS_PALWriteCallback(pWCTSCb);

} /*WCTS_PALDataCallback*/



/**
 * Notification callback when SMD needs to communicate asynchronously with
 * the client.
 *
 * This callback function may be called from interrupt context; clients must
 * not block or call any functions that block.
 *
 * @param[in] data   The user-supplied data provided to smd_named_open_on_edge()
 * @param[in] event  The event that occurred
 *
 * @return void
 */

static void
WCTS_NotifyCallback
(
   void            *data,
   unsigned        event
)
{
   wpt_msg                       *palMsg;
   WCTS_ControlBlockType*        pWCTSCb = (WCTS_ControlBlockType*) data;

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

   /*--------------------------------------------------------------------
     Sanity check
     --------------------------------------------------------------------*/
   if (WCTS_CB_MAGIC != pWCTSCb->wctsMagic) {
      WPAL_TRACE(eWLAN_MODULE_DAL_CTRL, eWLAN_PAL_TRACE_LEVEL_ERROR,
                 "WCTS_NotifyCallback: Invalid parameter received");

      /* TODO_PRIMA what error recovery options do we have? */
      return;
   }

   /* Serialize processing in the control thread */

   switch (event) {
   case SMD_EVENT_OPEN:
      palMsg = &pWCTSCb->wctsOpenMsg;
      break;

   case SMD_EVENT_DATA:
      palMsg = &pWCTSCb->wctsDataMsg;
      break;

   case SMD_EVENT_CLOSE:
      palMsg = &pWCTSCb->wctsCloseMsg;
      break;

   default:
      WPAL_TRACE(eWLAN_MODULE_DAL_CTRL, eWLAN_PAL_TRACE_LEVEL_ERROR,
                 "WCTS_NotifyCallback: Unexpected event %u received from SMD",
                 event);

      WPAL_ASSERT(0);
      return;
   }

   /* serialize this event */
   wpalPostCtrlMsg(WDI_GET_PAL_CTX(), palMsg);

} /*WCTS_NotifyCallback*/



/*----------------------------------------------------------------------------
 * Externalized Function Definitions
 * -------------------------------------------------------------------------*/


/**
 @brief     This function is used by the DAL Core to initialize the Control
            Transport for processing. It must be called prior to calling any
            other APIs of the Control Transport.


 @param szName:   unique name for the channel that is to be opened
         uSize:   size of the channel that must be opened (should fit the
                  largest size of  packet that the Dal Core wishes to send)
         wctsCBs:  a list of callbacks that the CT needs to use to send
                  notification and messages back to DAL

 @see
 @return  A handle that must be used for further communication with the CTS.
         This is an opaque structure for the caller and it will be used in
         all communications to and from the CTS.

*/
WCTS_HandleType
WCTS_OpenTransport
(
   const wpt_uint8*         szName,
   wpt_uint32               uSize,
   WCTS_TransportCBsType*   wctsCBs
)
{
   WCTS_ControlBlockType*    pWCTSCb;
   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

   /*---------------------------------------------------------------------
     Sanity check
     ---------------------------------------------------------------------*/
   if ((NULL == wctsCBs) || (NULL == szName) ||
       (NULL == wctsCBs->wctsNotifyCB) || (NULL == wctsCBs->wctsRxMsgCB)) {
      WPAL_TRACE(eWLAN_MODULE_DAL_CTRL, eWLAN_PAL_TRACE_LEVEL_ERROR,
                 "WCTS_OpenTransport: Invalid parameters received.");

      return NULL;
   }

#ifdef FEATURE_R33D
   if(port_open)
   {
      /* Port open before, not need to open again */
      /* notified registered client that the channel is open */
      ctsCB->wctsState = WCTS_STATE_OPEN;
      ctsCB->wctsNotifyCB((WCTS_HandleType)ctsCB,
                           WCTS_EVENT_OPEN,
                           ctsCB->wctsNotifyCBData);
      return (WCTS_HandleType)ctsCB;
   }
#endif /* FEATURE_R33D */

   /* allocate a ControlBlock to hold all context */
   pWCTSCb = wpalMemoryAllocate(sizeof(*pWCTSCb));
   if (NULL == pWCTSCb) {
      WPAL_TRACE(eWLAN_MODULE_DAL_CTRL, eWLAN_PAL_TRACE_LEVEL_ERROR,
                 "WCTS_OpenTransport: Memory allocation failure.");
      return NULL;
   }

#ifdef FEATURE_R33D
   smd_init(0);
   port_open = 1;
   ctsCB = pWCTSCb;
#endif /* FEATURE_R33D */

   /* save the user-supplied information */
   pWCTSCb->wctsNotifyCB       = wctsCBs->wctsNotifyCB;
   pWCTSCb->wctsNotifyCBData   = wctsCBs->wctsNotifyCBData;
   pWCTSCb->wctsRxMsgCB        = wctsCBs->wctsRxMsgCB;
   pWCTSCb->wctsRxMsgCBData    = wctsCBs->wctsRxMsgCBData;

   /* initialize the remaining fields */
   wpal_list_init(&pWCTSCb->wctsPendingQueue, NULL);
   pWCTSCb->wctsMagic   = WCTS_CB_MAGIC;
   pWCTSCb->wctsState   = WCTS_STATE_OPEN_PENDING;
   pWCTSCb->wctsChannel = NULL;

   /* since SMD will callback in interrupt context, we will used
    * canned messages to serialize the SMD events into a thread
    * context
    */
   pWCTSCb->wctsOpenMsg.callback = WCTS_PALOpenCallback;
   pWCTSCb->wctsOpenMsg.pContext = pWCTSCb;

   pWCTSCb->wctsDataMsg.callback = WCTS_PALDataCallback;
   pWCTSCb->wctsDataMsg.pContext = pWCTSCb;

   pWCTSCb->wctsCloseMsg.callback = WCTS_PALCloseCallback;
   pWCTSCb->wctsCloseMsg.pContext = pWCTSCb;

   /*---------------------------------------------------------------------
     Open the SMD channel
     ---------------------------------------------------------------------*/

   if (smd_named_open_on_edge(szName,
                              SMD_APPS_WCNSS,
                              &pWCTSCb->wctsChannel,
                              pWCTSCb,
                              WCTS_NotifyCallback)) {
      WPAL_TRACE(eWLAN_MODULE_DAL_CTRL, eWLAN_PAL_TRACE_LEVEL_ERROR,
                 "WCTS_OpenTransport: smd_named_open_on_edge failed.");
      pWCTSCb->wctsMagic = 0;
      wpalMemoryFree(pWCTSCb);
      return NULL;
   }

   /* we have opened the SMD channel */
   return (WCTS_HandleType)pWCTSCb;

}/*WCTS_OpenTransport*/



/**
 @brief    This function is used by the DAL Core to close the
           Control Transport when its services are no longer
           needed. Full close notification will be receive
           asynchronously on the notification callback
           registered on Open


 @param wctsHandlehandle:  received upon open

 @see
 @return   0 for success
*/
wpt_uint32
WCTS_CloseTransport
(
   WCTS_HandleType      wctsHandle
)
{
   WCTS_ControlBlockType* pWCTSCb = (WCTS_ControlBlockType*) wctsHandle;
   wpt_list_node*      pNode = NULL;
   WCTS_BufferType*    pBufferQueue = NULL;
   void*               pBuffer = NULL;

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

   if ((NULL == pWCTSCb) || (WCTS_CB_MAGIC != pWCTSCb->wctsMagic)) {
      WPAL_TRACE(eWLAN_MODULE_DAL_CTRL, eWLAN_PAL_TRACE_LEVEL_ERROR,
                 "WCTS_CloseTransport: Invalid parameters received.");
      return eWLAN_PAL_STATUS_E_INVAL;
   }

#ifdef FEATURE_R33D
   /* Not actually close port, just pretend */
   /* notified registered client that the channel is closed */
   pWCTSCb->wctsState = WCTS_STATE_CLOSED;
   pWCTSCb->wctsNotifyCB((WCTS_HandleType)pWCTSCb,
                         WCTS_EVENT_CLOSE,
                         pWCTSCb->wctsNotifyCBData);

   printk(KERN_ERR "R33D Not need to close");
   return eWLAN_PAL_STATUS_SUCCESS;
#endif /* FEATURE_R33D */

   /*Free the buffers in the pending queue.*/
   while (eWLAN_PAL_STATUS_SUCCESS ==
          wpal_list_remove_front(&pWCTSCb->wctsPendingQueue, &pNode)) {
      pBufferQueue = container_of(pNode, WCTS_BufferType, node);
      pBuffer = pBufferQueue->pBuffer;
      wpalMemoryFree(pBuffer);
      wpalMemoryFree(pBufferQueue);
   }

   /* Reset the state */
   pWCTSCb->wctsState = WCTS_STATE_CLOSED;

   if (0 != smd_close(pWCTSCb->wctsChannel)) {

      /* SMD did not successfully close the channel, therefore we
         won't receive an asynchronous close notification.  Since
         we rely upon that notification to clean up our context,
         replicate some of that logic here */
      pWCTSCb->wctsNotifyCB((WCTS_HandleType)pWCTSCb,
                            WCTS_EVENT_CLOSE,
                            pWCTSCb->wctsNotifyCBData);
      pWCTSCb->wctsMagic = 0;
      wpalMemoryFree(pWCTSCb);
   }

   return eWLAN_PAL_STATUS_SUCCESS;

}/*WCTS_CloseTransport*/



/**
 @brief    This function is used by the DAL Core to to send a
           message over to the WLAN sub-system.

           Once a buffer has been passed into the Send Message
 API, CT takes full ownership of it and it is responsible for
 freeing the associated resources. (This prevents a memcpy in
 case of a deferred write)

 The messages transported through the CT on both RX and TX are
 flat memory buffers that can be accessed and manipulated
 through standard memory functions.

 @param wctsHandlehandle:  received upon open
        pMsg:  the message to be sent
        uLen: the length of the message

 @see
 @return   0 for success
*/
wpt_uint32
WCTS_SendMessage
(
   WCTS_HandleType      wctsHandle,
   void*                pMsg,
   wpt_uint32           uLen
)
{
   WCTS_ControlBlockType*    pWCTSCb = (WCTS_ControlBlockType*) wctsHandle;
   WCTS_BufferType*          pBufferQueue;
   int                       len;
   int                       written = 0;
   int                       available;

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

   /*--------------------------------------------------------------------
     Sanity check
     --------------------------------------------------------------------*/
   if ((NULL == pWCTSCb) || (WCTS_CB_MAGIC != pWCTSCb->wctsMagic) ||
       (NULL == pMsg) || (0 == uLen) || (0x7fffffff < uLen)) {
      WPAL_TRACE(eWLAN_MODULE_DAL_CTRL, eWLAN_PAL_TRACE_LEVEL_ERROR,
                 "WCTS_SendMessage: Invalid parameters received.");
      WPAL_ASSERT(0);
      if (NULL != pMsg) {
         wpalMemoryFree(pMsg);
      }
      return eWLAN_PAL_STATUS_E_INVAL;
   }

   /* the SMD API uses int instead of uint, so change types here */
   len = (int)uLen;

   if (WCTS_STATE_OPEN == pWCTSCb->wctsState) {
      available = smd_write_avail(pWCTSCb->wctsChannel);
      if (available >= len) {
         written = smd_write(pWCTSCb->wctsChannel, pMsg, len);
      }
   } else if (WCTS_STATE_DEFERRED == pWCTSCb->wctsState) {
      WPAL_TRACE(eWLAN_MODULE_DAL_CTRL, eWLAN_PAL_TRACE_LEVEL_INFO,
                 "WCTS_SendMessage: FIFO space not available, the packets will be queued");
   } else {
      WPAL_TRACE(eWLAN_MODULE_DAL_CTRL, eWLAN_PAL_TRACE_LEVEL_ERROR,
                 "WCTS_SendMessage: Channel in illegal state [%d].",
                 pWCTSCb->wctsState);
      /* force following logic to reclaim the buffer */
      written = -1;
   }

   if (-1 == written) {
      /*Something wrong*/
      WPAL_TRACE(eWLAN_MODULE_DAL_CTRL, eWLAN_PAL_TRACE_LEVEL_ERROR,
                 "WCTS_SendMessage: Failed to send message over the bus.");
      wpalMemoryFree(pMsg);
      WPAL_ASSERT(0);
      return eWLAN_PAL_STATUS_E_FAILURE;
   } else if (written == len) {
      /* Message sent! No deferred state, free the buffer*/
      wpalMemoryFree(pMsg);
   } else {
      /* This much data cannot be written at this time,
         queue the rest of the data for later*/
      pBufferQueue = wpalMemoryAllocate(sizeof(WCTS_BufferType));
      if (NULL == pBufferQueue) {
         WPAL_TRACE(eWLAN_MODULE_DAL_CTRL, eWLAN_PAL_TRACE_LEVEL_ERROR,
                    "WCTS_SendMessage: Cannot allocate memory for queuing the buffer");
         wpalMemoryFree(pMsg);
         WPAL_ASSERT(0);
         return eWLAN_PAL_STATUS_E_NOMEM;
      }

      pBufferQueue->bufferSize = len;
      pBufferQueue->pBuffer = pMsg;
      wpal_list_insert_back(&pWCTSCb->wctsPendingQueue, &pBufferQueue->node);

      /* if we are not already in the deferred state, then transition
         to that state.  when we do so, we enable the remote read
         interrupt so that we'll be notified when messages are read
         from the remote end */
      if (WCTS_STATE_DEFERRED != pWCTSCb->wctsState) {

         /* Mark the state as deferred.
            Later: We may need to protect wctsState by locks*/
         pWCTSCb->wctsState = WCTS_STATE_DEFERRED;

         smd_enable_read_intr(pWCTSCb->wctsChannel);
      }
   }

   return eWLAN_PAL_STATUS_SUCCESS;

}/*WCTS_SendMessage*/
