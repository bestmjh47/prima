/*===========================================================================
  @file vos_sched.c
  @brief VOS Scheduler Implementation

  Copyright (c) 2011 QUALCOMM Incorporated.
  All Rights Reserved.
  Qualcomm Confidential and Proprietary
===========================================================================*/
/*===========================================================================
                       EDIT HISTORY FOR FILE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

  $Header:$ $DateTime: $ $Author: $

  when        who    what, where, why
  --------    ---    --------------------------------------------------------
===========================================================================*/
/*---------------------------------------------------------------------------
 * Include Files
 * ------------------------------------------------------------------------*/
#include <vos_mq.h>
#include <vos_api.h>
#include <aniGlobal.h>
#include <sirTypes.h>
#include <halTypes.h>
#include <limApi.h>
#include <sme_Api.h>
#ifndef FEATURE_WLAN_INTEGRATED_SOC
#include <wlan_qct_ssc.h>
#endif
#include <wlan_qct_sys.h>
#include <wlan_qct_tl.h>
#include "vos_sched.h"
#include <wlan_hdd_power.h>
#ifdef FEATURE_WLAN_INTEGRATED_SOC
#include "wlan_qct_wda.h"
#include "wlan_qct_pal_msg.h"
#endif
/*---------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ------------------------------------------------------------------------*/
#define VOS_SCHED_THREAD_HEART_BEAT    INFINITE
/*---------------------------------------------------------------------------
 * Type Declarations
 * ------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------
 * Data definitions
 * ------------------------------------------------------------------------*/
static pVosSchedContext gpVosSchedContext;
static pVosWatchdogContext gpVosWatchdogContext;

/*---------------------------------------------------------------------------
 * Forward declaration
 * ------------------------------------------------------------------------*/
static int VosMCThread(void * Arg);
static int VosWDThread(void * Arg);
#if !defined(ANI_MANF_DIAG) || defined(FEATURE_WLAN_INTEGRATED_SOC)
/* Integrated SOC will have single BIN for FTM driver and Production Driver
 * So, anycase this must be compiled
 * None integrated SOC will compile this part only for FTM Driver */
static int VosTXThread(void * Arg);
#ifdef FEATURE_WLAN_INTEGRATED_SOC
static int VosRXThread(void* Arg);
void vos_sched_flush_rx_mqs(pVosSchedContext SchedContext);
#endif
#endif
extern v_VOID_t vos_core_return_msg(v_PVOID_t pVContext, pVosMsgWrapper pMsgWrapper);
/*---------------------------------------------------------------------------
 * External Function implementation
 * ------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
  \brief vos_sched_open() - initialize the vOSS Scheduler
  The \a vos_sched_open() function initializes the vOSS Scheduler
  Upon successful initialization:
     - All the message queues are initialized
     - The Main Controller thread is created and ready to receive and
       dispatch messages.
     - The Tx thread is created and ready to receive and dispatch messages

  \param  pVosContext - pointer to the global vOSS Context
  \param  pVosSchedContext - pointer to a previously allocated buffer big
          enough to hold a scheduler context.
  \return VOS_STATUS_SUCCESS - Scheduler was successfully initialized and
          is ready to be used.
          VOS_STATUS_E_RESOURCES - System resources (other than memory)
          are unavailable to initilize the scheduler
          VOS_STATUS_E_NOMEM - insufficient memory exists to initialize
          the scheduler
          VOS_STATUS_E_INVAL - Invalid parameter passed to the scheduler Open
          function
          VOS_STATUS_E_FAILURE - Failure to initialize the scheduler/
  \sa vos_sched_open()
  -------------------------------------------------------------------------*/
VOS_STATUS
vos_sched_open
(
  v_PVOID_t        pVosContext,
  pVosSchedContext pSchedContext,
  v_SIZE_t         SchedCtxSize
)
{
  VOS_STATUS  vStatus = VOS_STATUS_SUCCESS;
/*-------------------------------------------------------------------------*/
  VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_INFO_HIGH,
             "%s: Opening the VOSS Scheduler",__func__);
  // Sanity checks
  if ((pVosContext == NULL) || (pSchedContext == NULL)) {
     VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
             "%s: Null params being passed",__func__);
     return VOS_STATUS_E_FAILURE;
  }
  if (sizeof(VosSchedContext) != SchedCtxSize)
  {
     VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_INFO_HIGH,
               "%s: Incorrect VOS Sched Context size passed",__func__);
     return VOS_STATUS_E_INVAL;
  }
  vos_mem_zero(pSchedContext, sizeof(VosSchedContext));
  pSchedContext->pVContext = pVosContext;
  vStatus = vos_sched_init_mqs(pSchedContext);
  if (!VOS_IS_STATUS_SUCCESS(vStatus))
  {
     VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
               "%s: Failed to initialize VOS Scheduler MQs",__func__);
     return vStatus;
  }
  // Initialize the helper events and event queues
  init_completion(&pSchedContext->McStartEvent);
  init_completion(&pSchedContext->TxStartEvent);
#ifdef FEATURE_WLAN_INTEGRATED_SOC
  init_completion(&pSchedContext->RxStartEvent);
#endif
  init_completion(&pSchedContext->McShutdown);
  init_completion(&pSchedContext->TxShutdown);
#ifdef FEATURE_WLAN_INTEGRATED_SOC
  init_completion(&pSchedContext->RxShutdown);
#endif
  init_completion(&pSchedContext->ResumeMcEvent);
  init_completion(&pSchedContext->ResumeTxEvent);
#ifdef FEATURE_WLAN_INTEGRATED_SOC
 //TO DO: init_completion(&pSchedContext->ResumeRxEvent);
#endif

  init_waitqueue_head(&pSchedContext->mcWaitQueue);
  pSchedContext->mcEventFlag = 0;
  init_waitqueue_head(&pSchedContext->txWaitQueue);
  pSchedContext->txEventFlag= 0;
#ifdef FEATURE_WLAN_INTEGRATED_SOC
  init_waitqueue_head(&pSchedContext->rxWaitQueue);
  pSchedContext->rxEventFlag= 0;
#endif
  /*
  ** This initialization is critical as the threads will latter access the
  ** global contexts normally,
  **
  ** I shall put some memory barrier here after the next piece of code but
  ** I am keeping it simple for now.
  */
  gpVosSchedContext = pSchedContext;
  //Create the VOSS Main Controller thread
  pSchedContext->McThread = kernel_thread(VosMCThread, pSchedContext,
     CLONE_FS | CLONE_FILES | CLONE_SIGHAND | SIGCHLD);
  if (pSchedContext->McThread < 0)
  {
     VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_FATAL,
               "%s: Could not Create VOSS Main Thread Controller",__func__);

     goto MC_THREAD_START_FAILURE;

  }
  VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_INFO_HIGH,
            "%s: VOSS Main Controller thread Created",__func__);

#if !defined(ANI_MANF_DIAG) || defined(FEATURE_WLAN_INTEGRATED_SOC)
  pSchedContext->TxThread = kernel_thread(VosTXThread, pSchedContext,
     CLONE_FS | CLONE_FILES | CLONE_SIGHAND | SIGCHLD);
  if (pSchedContext->TxThread < 0)
  {
     VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_FATAL,
               "%s: Could not Create VOSS TX Thread",__func__);

     goto TX_THREAD_START_FAILURE;
  }
  VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_INFO_HIGH,
             ("VOSS TX thread Created\n"));

#ifdef FEATURE_WLAN_INTEGRATED_SOC
  pSchedContext->RxThread = kernel_thread(VosRXThread, pSchedContext,
     CLONE_FS | CLONE_FILES | CLONE_SIGHAND | SIGCHLD);
  if (pSchedContext->RxThread < 0)
  {

     VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_FATAL,
               "%s: Could not Create VOSS RX Thread",__func__);
     goto RX_THREAD_START_FAILURE;

  }
  VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_INFO_HIGH,
             ("VOSS RX thread Created\n"));
#endif
  /*
  ** Now make sure all threads have started before we exit.
  ** Each thread should normally ACK back when it starts.
  */
 #endif
  wait_for_completion_interruptible(&pSchedContext->McStartEvent);
  VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_INFO_HIGH,
               "%s: VOSS MC Thread has started",__func__);
#if !defined(ANI_MANF_DIAG) || defined(FEATURE_WLAN_INTEGRATED_SOC)
  wait_for_completion_interruptible(&pSchedContext->TxStartEvent);
  VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_INFO_HIGH,
               "%s: VOSS Tx Thread has started",__func__);
 #ifdef FEATURE_WLAN_INTEGRATED_SOC
  wait_for_completion_interruptible(&pSchedContext->RxStartEvent);
  VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_INFO_HIGH,
               "%s: VOSS Rx Thread has started",__func__);
 #endif

  /*
  ** We're good now: Let's get the ball rolling!!!
  */
  VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_INFO_HIGH,
            "%s: VOSS Scheduler successfully Opened",__func__);
  #endif
  return VOS_STATUS_SUCCESS;

#if !defined(ANI_MANF_DIAG) || defined(FEATURE_WLAN_INTEGRATED_SOC)

#ifdef FEATURE_WLAN_INTEGRATED_SOC
RX_THREAD_START_FAILURE:
    //Try and force the Tx thread controller to exit
    set_bit(MC_SHUTDOWN_EVENT_MASK, &pSchedContext->txEventFlag);
    set_bit(MC_POST_EVENT_MASK, &pSchedContext->txEventFlag);
    wake_up_interruptible(&pSchedContext->txWaitQueue);
     //Wait for TX to exit
    wait_for_completion_interruptible(&pSchedContext->TxShutdown);
#endif

TX_THREAD_START_FAILURE:
    //Try and force the Main thread controller to exit
    set_bit(MC_SHUTDOWN_EVENT_MASK, &pSchedContext->mcEventFlag);
    set_bit(MC_POST_EVENT_MASK, &pSchedContext->mcEventFlag);
    wake_up_interruptible(&pSchedContext->mcWaitQueue);
    //Wait for MC to exit
    wait_for_completion_interruptible(&pSchedContext->McShutdown);

#endif
MC_THREAD_START_FAILURE:
  //De-initialize all the message queues
  vos_sched_deinit_mqs(pSchedContext);
  return VOS_STATUS_E_RESOURCES;

} /* vos_sched_open() */

VOS_STATUS vos_watchdog_open
(
  v_PVOID_t           pVosContext,
  pVosWatchdogContext pWdContext,
  v_SIZE_t            wdCtxSize
)
{
/*-------------------------------------------------------------------------*/
  VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_INFO_HIGH,
             "%s: Opening the VOSS Watchdog module",__func__);
  //Sanity checks
  if ((pVosContext == NULL) || (pWdContext == NULL)) {
     VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
             "%s: Null params being passed",__func__);
     return VOS_STATUS_E_FAILURE;
  }
  if (sizeof(VosWatchdogContext) != wdCtxSize)
  {
     VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_INFO_HIGH,
               "%s: Incorrect VOS Watchdog Context size passed",__func__);
     return VOS_STATUS_E_INVAL;
  }
  vos_mem_zero(pWdContext, sizeof(VosWatchdogContext));
  pWdContext->pVContext = pVosContext;
  gpVosWatchdogContext = pWdContext;
	 
  //Initialize the helper events and event queues
  init_completion(&pWdContext->WdStartEvent);
  init_completion(&pWdContext->WdShutdown);
  init_waitqueue_head(&pWdContext->wdWaitQueue);
  pWdContext->wdEventFlag = 0;
  //Create the Watchdog thread
  pWdContext->WdThread = kernel_thread(VosWDThread, pWdContext,
     CLONE_FS | CLONE_FILES | CLONE_SIGHAND | SIGCHLD);  
  
  if (pWdContext->WdThread < 0)
  {
     VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_FATAL,
               "%s: Could not Create Watchdog thread",__func__);
     return VOS_STATUS_E_RESOURCES;
  }  
 /*
  ** Now make sure thread has started before we exit.
  ** Each thread should normally ACK back when it starts.
  */
  wait_for_completion_interruptible(&pWdContext->WdStartEvent);
  VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_INFO_HIGH,
               "%s: VOSS Watchdog Thread has started",__func__);
  return VOS_STATUS_SUCCESS;
} /* vos_watchdog_open() */
/*---------------------------------------------------------------------------
  \brief VosMcThread() - The VOSS Main Controller thread
  The \a VosMcThread() is the VOSS main controller thread:
  \param  Arg - pointer to the global vOSS Sched Context
  \return Thread exit code
  \sa VosMcThread()
  -------------------------------------------------------------------------*/
static int
VosMCThread
(
  void * Arg
)
{
  pVosSchedContext pSchedContext = (pVosSchedContext)Arg;
  pVosMsgWrapper pMsgWrapper     = NULL;
  tpAniSirGlobal pMacContext     = NULL;
  tSirRetStatus macStatus        = eSIR_SUCCESS;
  VOS_STATUS vStatus             = VOS_STATUS_SUCCESS;
  int retWaitStatus              = 0;
  v_BOOL_t shutdown              = VOS_FALSE;
  hdd_context_t *pHddCtx         = NULL;
  v_CONTEXT_t pVosContext        = NULL;

  if (Arg == NULL)
  {
     VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
        "%s: Bad Args passed", __FUNCTION__);
     return 0;
  }
  set_user_nice(current, -2);

  daemonize("MC_Thread");
  /*
  ** Ack back to the context from which the main controller thread has been
  ** created.
  */
  complete(&pSchedContext->McStartEvent);
  VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_INFO,
      "%s: MC Thread %d (%s) starting up",__func__, current->pid, current->comm);

  /* Get the Global VOSS Context */
  pVosContext = vos_get_global_context(VOS_MODULE_ID_SYS, NULL);
  if(!pVosContext) {
     hddLog(VOS_TRACE_LEVEL_FATAL,"%s: Global VOS context is Null", __func__);
     return 0;
  }

  /* Get the HDD context */
  pHddCtx = (hdd_context_t *)vos_get_context(VOS_MODULE_ID_HDD, pVosContext );
  if(!pHddCtx) {
     hddLog(VOS_TRACE_LEVEL_FATAL,"%s: HDD context is Null",__func__);
     return 0;
  }

  while(!shutdown)
  {
    // This implements the execution model algorithm
    retWaitStatus = wait_event_interruptible(pSchedContext->mcWaitQueue,
       test_bit(MC_POST_EVENT_MASK, &pSchedContext->mcEventFlag) || 
       test_bit(MC_SUSPEND_EVENT_MASK, &pSchedContext->mcEventFlag));

    if(retWaitStatus == -ERESTARTSYS)
    {
      VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
         "%s: wait_event_interruptible returned -ERESTARTSYS", __FUNCTION__);
      break;
    }
    clear_bit(MC_POST_EVENT_MASK, &pSchedContext->mcEventFlag);

    while(1)
    {
      // Check if MC needs to shutdown
      if(test_bit(MC_SHUTDOWN_EVENT_MASK, &pSchedContext->mcEventFlag))
      {
        VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_FATAL,
                "%s: MC thread signaled to shutdown",__func__);
        shutdown = VOS_TRUE;
        /* Check for any Suspend Indication */
        if(test_bit(MC_SUSPEND_EVENT_MASK, &pSchedContext->mcEventFlag))
        {
           clear_bit(MC_SUSPEND_EVENT_MASK, &pSchedContext->mcEventFlag);
        
           /* Unblock anyone waiting on suspend */
           complete(&pHddCtx->mc_sus_event_var);
        }
        break;
      }
#ifdef FEATURE_WLAN_INTEGRATED_SOC
      /*
      ** Check the WDI queue
      ** Service it till the entire queue is empty
      */
      if (!vos_is_mq_empty(&pSchedContext->wdiMcMq))
      {
        wpt_msg *pWdiMsg;
        /*
        ** Service the WDI message queue
        */
        VOS_TRACE(VOS_MODULE_ID_WDI, VOS_TRACE_LEVEL_INFO,
                  ("Servicing the VOS MC WDI Message queue"));

        pMsgWrapper = vos_mq_get(&pSchedContext->wdiMcMq);
        VOS_ASSERT(NULL != pMsgWrapper);

        pWdiMsg = (wpt_msg *)pMsgWrapper->pVosMsg->bodyptr;
        VOS_ASSERT(pWdiMsg->callback);

        pWdiMsg->callback(pWdiMsg);

        /* 
        ** return message to the Core
        */
        vos_core_return_msg(pSchedContext->pVContext, pMsgWrapper);

        continue;
      }

#endif
      // Check the SYS queue first
      if (!vos_is_mq_empty(&pSchedContext->sysMcMq))
      {
        // Service the SYS message queue
        VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_INFO,
                  "%s: Servicing the VOS SYS MC Message queue",__func__);
        pMsgWrapper = vos_mq_get(&pSchedContext->sysMcMq);
        if (pMsgWrapper == NULL)
        {
           VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
              "%s: pMsgWrapper is NULL", __FUNCTION__);
           VOS_ASSERT(0);
           break;
        }
        vStatus = sysMcProcessMsg(pSchedContext->pVContext,
           pMsgWrapper->pVosMsg);
        if (!VOS_IS_STATUS_SUCCESS(vStatus))
        {
           VOS_TRACE( VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
                     "%s: Issue Processing SYS message",__func__);
        }
        //return message to the Core
        vos_core_return_msg(pSchedContext->pVContext, pMsgWrapper);
        continue;
      }
#ifdef FEATURE_WLAN_INTEGRATED_SOC
      // Check the WDA queue
      if (!vos_is_mq_empty(&pSchedContext->wdaMcMq))
      {
        /* Need some optimization*/
        pMacContext = vos_get_context(VOS_MODULE_ID_WDA, pSchedContext->pVContext);
        // Service the WDA message queue
        VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_INFO,
                 "%s: Servicing the VOS WDA MC Message queue",__func__);
        pMsgWrapper = vos_mq_get(&pSchedContext->wdaMcMq);
        if (pMsgWrapper == NULL)
        {
           VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
              "%s: pMsgWrapper is NULL", __FUNCTION__);
           VOS_ASSERT(0);
           break;
        }
        macStatus = WDA_McProcessMsg( pSchedContext->pVContext, pMsgWrapper->pVosMsg);
        if (eSIR_SUCCESS != macStatus)
        {
           VOS_TRACE( VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
                     "%s: Issue Processing WDA message",__func__);
        }
        // return message to the Core
        vos_core_return_msg(pSchedContext->pVContext, pMsgWrapper);
        continue;
      }
#else

      // Check the HAL queue
      if (!vos_is_mq_empty(&pSchedContext->halMcMq))
      {
        /* Need some optimization*/
        pMacContext = vos_get_context(VOS_MODULE_ID_HAL, pSchedContext->pVContext);
        // Service the HAL message queue
        VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_INFO,
                 "%s: Servicing the VOS HAL MC Message queue",__func__);
        pMsgWrapper = vos_mq_get(&pSchedContext->halMcMq);
        if (pMsgWrapper == NULL)
        {
           VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
              "%s: pMsgWrapper is NULL", __FUNCTION__);
           VOS_ASSERT(0);
           break;
        }
        macStatus = halProcessMsg( pMacContext, (tSirMsgQ*)pMsgWrapper->pVosMsg);
        if (eSIR_SUCCESS != macStatus)
        {
           VOS_TRACE( VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
                     "%s: Issue Processing HAL message",__func__);
        }
        // return message to the Core
        vos_core_return_msg(pSchedContext->pVContext, pMsgWrapper);
        continue;
      }
#endif
#if !defined(ANI_MANF_DIAG) || defined(FEATURE_WLAN_INTEGRATED_SOC)
      // Check the PE queue
      if (!vos_is_mq_empty(&pSchedContext->peMcMq))
      {
        /* Need some optimization*/
        pMacContext = vos_get_context(VOS_MODULE_ID_PE, pSchedContext->pVContext);
        // Service the PE message queue
        VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_INFO,
                  "%s: Servicing the VOS PE MC Message queue",__func__);
        if (pMsgWrapper == NULL)
        {
           VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
              "%s: pMsgWrapper is NULL", __FUNCTION__);
           VOS_ASSERT(0);
           break;
        }
        pMsgWrapper = vos_mq_get(&pSchedContext->peMcMq);
        if(NULL == pMacContext)
        {
           VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_INFO,
                     "MAC Context not ready yet");
           vos_core_return_msg(pSchedContext->pVContext, pMsgWrapper);
           continue;
        }
        macStatus = peProcessMessages( pMacContext, (tSirMsgQ*)pMsgWrapper->pVosMsg);
        if (eSIR_SUCCESS != macStatus)
        {
          VOS_TRACE( VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
                     "%s: Issue Processing PE message",__func__);
        }
        // return message to the Core
        vos_core_return_msg(pSchedContext->pVContext, pMsgWrapper);
        continue;
      }
      /** Check the SME queue **/
      if (!vos_is_mq_empty(&pSchedContext->smeMcMq))
      {
        /* Need some optimization*/
        pMacContext = vos_get_context(VOS_MODULE_ID_SME, pSchedContext->pVContext);
        /* Service the SME message queue */
        VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_INFO,
                  "%s: Servicing the VOS SME MC Message queue",__func__);
        pMsgWrapper = vos_mq_get(&pSchedContext->smeMcMq);
        if (pMsgWrapper == NULL)
        {
           VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
               "%s: pMsgWrapper is NULL", __FUNCTION__);
           VOS_ASSERT(0);
           break;
        }
        vStatus = sme_ProcessMsg( (tHalHandle)pMacContext, pMsgWrapper->pVosMsg);
        if (!VOS_IS_STATUS_SUCCESS(vStatus))
        {
          VOS_TRACE( VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
                     "%s: Issue Processing SME message",__func__);
        }
        // return message to the Core
        vos_core_return_msg(pSchedContext->pVContext, pMsgWrapper);
        continue;
      }
      /** Check the TL queue **/
      if (!vos_is_mq_empty(&pSchedContext->tlMcMq))
      {
        // Service the TL message queue
        VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_INFO,
                  ("Servicing the VOS TL MC Message queue"));
        pMsgWrapper = vos_mq_get(&pSchedContext->tlMcMq);
        if (pMsgWrapper == NULL)
        {
           VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
              "%s: pMsgWrapper is NULL", __FUNCTION__);
           VOS_ASSERT(0);
           break;
        }
        vStatus = WLANTL_McProcessMsg( pSchedContext->pVContext,
            pMsgWrapper->pVosMsg);
        if (!VOS_IS_STATUS_SUCCESS(vStatus))
        {
          VOS_TRACE( VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
                     "%s: Issue Processing TL message",__func__);
        }
        // return message to the Core
        vos_core_return_msg(pSchedContext->pVContext, pMsgWrapper);
        continue;
      }
     /* Check for any Suspend Indication */
      if(test_bit(MC_SUSPEND_EVENT_MASK, &pSchedContext->mcEventFlag))
      {
        clear_bit(MC_SUSPEND_EVENT_MASK, &pSchedContext->mcEventFlag);

        /* Mc Thread Suspended */
        complete(&pHddCtx->mc_sus_event_var);

        INIT_COMPLETION(pSchedContext->ResumeMcEvent);

        /* Wait foe Resume Indication */
        wait_for_completion_interruptible(&pSchedContext->ResumeMcEvent);
      }
#endif /* ANI_MANF_DIAG */
      break; //All queues are empty now
    } // while message loop processing
  } // while TRUE
  // If we get here the MC thread must exit
  VOS_TRACE( VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
      "%s: MC Thread exiting!!!!", __FUNCTION__);
  complete_and_exit(&pSchedContext->McShutdown, 0);
} /* VosMCThread() */
/*---------------------------------------------------------------------------
  \brief VosWdThread() - The VOSS Watchdog thread
  The \a VosWdThread() is the Watchdog thread:
  \param  Arg - pointer to the global vOSS Sched Context
  \return Thread exit code
  \sa VosMcThread()
  -------------------------------------------------------------------------*/
static int
VosWDThread
(
  void * Arg
)
{
  pVosWatchdogContext pWdContext = (pVosWatchdogContext)Arg;
  int retWaitStatus              = 0;
  v_BOOL_t shutdown              = VOS_FALSE;
#ifdef FEATURE_WLAN_NON_INTEGRATED_SOC
  VOS_STATUS vosStatus = VOS_STATUS_SUCCESS;
#endif
  set_user_nice(current, -3);

  if (Arg == NULL)
  {
     VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
        "%s: Bad Args passed", __FUNCTION__);
     return 0;
  }
  daemonize("WD_Thread");
  /*
  ** Ack back to the context from which the Watchdog thread has been
  ** created.
  */
  complete(&pWdContext->WdStartEvent);
  VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_INFO,
      "%s: Watchdog Thread %d (%s) starting up",__func__, current->pid, current->comm);

  while(!shutdown)
  {
    // This implements the Watchdog execution model algorithm
    retWaitStatus = wait_event_interruptible(pWdContext->wdWaitQueue,
       test_bit(WD_POST_EVENT_MASK, &pWdContext->wdEventFlag));
    if(retWaitStatus == -ERESTARTSYS)
    {
      VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
         "%s: wait_event_interruptible returned -ERESTARTSYS", __FUNCTION__);
      break;
    }
    clear_bit(WD_POST_EVENT_MASK, &pWdContext->wdEventFlag);
	 
    while(1)
    {
      // Check if Watchdog needs to shutdown
      if(test_bit(WD_SHUTDOWN_EVENT_MASK, &pWdContext->wdEventFlag))
      {
        VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_FATAL,
                "%s: Watchdog thread signaled to shutdown",__func__);
		  
		  clear_bit(WD_SHUTDOWN_EVENT_MASK, &pWdContext->wdEventFlag);
        shutdown = VOS_TRUE;
        break;
      }
      else if(test_bit(WD_CHIP_RESET_EVENT_MASK, &pWdContext->wdEventFlag))
      {
        VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_FATAL,
                "%s: Watchdog thread signaled to perform WLAN chip reset",__func__);
        clear_bit(WD_CHIP_RESET_EVENT_MASK, &pWdContext->wdEventFlag);

        //Perform WLAN Reset
        if(!pWdContext->resetInProgress)
        {
          pWdContext->resetInProgress = true;
#ifdef CONFIG_HAS_EARLYSUSPEND
#ifdef FEATURE_WLAN_NON_INTEGRATED_SOC
          vosStatus = hdd_wlan_reset();

          if (! VOS_IS_STATUS_SUCCESS(vosStatus))
          {
             VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_FATAL, "%s: Failed to reset WLAN",__func__);
             VOS_ASSERT(0);
             shutdown = VOS_TRUE;
             break;
          }
#endif
#endif
             pWdContext->resetInProgress = false;
        }
        else
        {
          VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_FATAL,
               "%s: Reset already in progress. Ignore recursive reset cmd",__func__);
        }
		  
        break;
      }
      //Unnecessary wakeup - Should never happen!!
      VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_FATAL,
        "%s: Watchdog thread woke up unnecessarily",__func__);
      break;
    } // while message loop processing
    
  } // while TRUE
  // If we get here the Watchdog thread must exit
  VOS_TRACE( VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_FATAL,
      "%s: Watchdog Thread exiting!!!!", __FUNCTION__);
  complete_and_exit(&pWdContext->WdShutdown, 0);
} /* VosMCThread() */

/*---------------------------------------------------------------------------
  \brief VosTXThread() - The VOSS Main Tx thread
  The \a VosTxThread() is the VOSS main controller thread:
  \param  Arg - pointer to the global vOSS Sched Context

  \return Thread exit code
  \sa VosTxThread()
  -------------------------------------------------------------------------*/
#if !defined(ANI_MANF_DIAG) || defined(FEATURE_WLAN_INTEGRATED_SOC)
static int VosTXThread ( void * Arg )
{
  pVosSchedContext pSchedContext = (pVosSchedContext)Arg;
  pVosMsgWrapper   pMsgWrapper   = NULL;
  VOS_STATUS       vStatus       = VOS_STATUS_SUCCESS;
  int              retWaitStatus = 0;
  v_BOOL_t shutdown = VOS_FALSE;
  hdd_context_t *pHddCtx         = NULL;
  v_CONTEXT_t pVosContext        = NULL;

  set_user_nice(current, -1);

  if (Arg == NULL)
  {
     VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
         "%s Bad Args passed", __FUNCTION__);
     return 0;
  }
  daemonize("TX_Thread");
  /*
  ** Ack back to the context from which the main controller thread has been
  ** created.
  */
  complete(&pSchedContext->TxStartEvent);
  VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
      "%s: TX Thread %d (%s) starting up!",__func__, current->pid, current->comm);

  /* Get the Global VOSS Context */
  pVosContext = vos_get_global_context(VOS_MODULE_ID_SYS, NULL);
  if(!pVosContext) {
     hddLog(VOS_TRACE_LEVEL_FATAL,"%s: Global VOS context is Null", __func__);
     return 0;
  }

  /* Get the HDD context */
  pHddCtx = (hdd_context_t *)vos_get_context(VOS_MODULE_ID_HDD, pVosContext );
  if(!pHddCtx) {
     hddLog(VOS_TRACE_LEVEL_FATAL,"%s: HDD context is Null",__func__);
     return 0;
  }


  while(!shutdown)
  {
    // This implements the execution model algorithm
    retWaitStatus = wait_event_interruptible(pSchedContext->txWaitQueue,
        test_bit(TX_POST_EVENT_MASK, &pSchedContext->txEventFlag) || 
        test_bit(TX_SUSPEND_EVENT_MASK, &pSchedContext->txEventFlag));


    if(retWaitStatus == -ERESTARTSYS)
    {
        VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
           "%s: wait_event_interruptible returned -ERESTARTSYS", __FUNCTION__);
        break;
    }
    clear_bit(TX_POST_EVENT_MASK, &pSchedContext->txEventFlag);

    while(1)
    {
      if(test_bit(TX_SHUTDOWN_EVENT_MASK, &pSchedContext->txEventFlag))
      {
        VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_FATAL,
                 "%s: TX thread signaled for shutdown",__func__);
        shutdown = VOS_TRUE;
        /* Check for any Suspend Indication */
        if(test_bit(TX_SUSPEND_EVENT_MASK, &pSchedContext->txEventFlag))
        {
           clear_bit(TX_SUSPEND_EVENT_MASK, &pSchedContext->txEventFlag);
        
           /* Unblock anyone waiting on suspend */
           complete(&pHddCtx->tx_sus_event_var);
        }
        break;
      }
      // Check the SYS queue first
      if (!vos_is_mq_empty(&pSchedContext->sysTxMq))
      {
        // Service the SYS message queue
        VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_INFO,
                "%s: Servicing the VOS SYS TX Message queue",__func__);
        pMsgWrapper = vos_mq_get(&pSchedContext->sysTxMq);
        if (pMsgWrapper == NULL)
        {
           VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
               "%s: pMsgWrapper is NULL", __FUNCTION__);
           VOS_ASSERT(0);
           break;
        }
        vStatus = sysTxProcessMsg( pSchedContext->pVContext,
                                   pMsgWrapper->pVosMsg);
        if (!VOS_IS_STATUS_SUCCESS(vStatus))
        {
          VOS_TRACE( VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
                     "%s: Issue Processing TX SYS message",__func__);
        }
        // return message to the Core
        vos_core_return_msg(pSchedContext->pVContext, pMsgWrapper);
        continue;
      }
      // Check now the TL queue
      if (!vos_is_mq_empty(&pSchedContext->tlTxMq))
      {
        // Service the TL message queue
        VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_INFO,
                "%s: Servicing the VOS TL TX Message queue",__func__);
        pMsgWrapper = vos_mq_get(&pSchedContext->tlTxMq);
        if (pMsgWrapper == NULL)
        {
           VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
               "%s: pMsgWrapper is NULL", __FUNCTION__);
           VOS_ASSERT(0);
           break;
        }
        vStatus = WLANTL_TxProcessMsg( pSchedContext->pVContext,
                                       pMsgWrapper->pVosMsg);
        if (!VOS_IS_STATUS_SUCCESS(vStatus))
        {
          VOS_TRACE( VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
                     "%s: Issue Processing TX TL message",__func__);
        }
        // return message to the Core
        vos_core_return_msg(pSchedContext->pVContext, pMsgWrapper);
        continue;
      }
#ifdef FEATURE_WLAN_INTEGRATED_SOC
      // Check the WDI queue
      if (!vos_is_mq_empty(&pSchedContext->wdiTxMq))
      {
        wpt_msg *pWdiMsg;
        VOS_TRACE(VOS_MODULE_ID_WDI, VOS_TRACE_LEVEL_INFO,
                  "%s: Servicing the VOS TX WDI Message queue",__func__);

        pMsgWrapper = vos_mq_get(&pSchedContext->wdiTxMq);
        VOS_ASSERT(NULL != pMsgWrapper);

        pWdiMsg = (wpt_msg *)pMsgWrapper->pVosMsg->bodyptr;
        VOS_ASSERT(pWdiMsg->callback);

        pWdiMsg->callback(pWdiMsg);

        // return message to the Core
        vos_core_return_msg(pSchedContext->pVContext, pMsgWrapper);

        continue;
      }
#else
      // Check the SSC queue
      if (!vos_is_mq_empty(&pSchedContext->sscTxMq))
      {
        // Service the PE message queue
        VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_INFO,
                  "%s: Servicing the VOS TX SSC Message queue",__func__);
        pMsgWrapper = vos_mq_get(&pSchedContext->sscTxMq);
        if (pMsgWrapper == NULL)
        {
           VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
               "%s: pMsgWrapper is NULL", __FUNCTION__);
           VOS_ASSERT(0);
           break;
        }
        vStatus = WLANSSC_ProcessMsg( pSchedContext->pVContext,
                                      pMsgWrapper->pVosMsg);
        if (!VOS_IS_STATUS_SUCCESS(vStatus))
        {
          VOS_TRACE( VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
                     "%s: Issue Processing TX TL message",__func__);
        }
        // return message to the Core
        vos_core_return_msg(pSchedContext->pVContext, pMsgWrapper);
        continue;
      }

#endif
      /* Check for any Suspend Indication */
      if(test_bit(TX_SUSPEND_EVENT_MASK, &pSchedContext->txEventFlag))
      {
        clear_bit(TX_SUSPEND_EVENT_MASK, &pSchedContext->txEventFlag);

        /* Tx Thread Suspended */
        complete(&pHddCtx->tx_sus_event_var);

        INIT_COMPLETION(pSchedContext->ResumeTxEvent);

        /* Wait foe Resume Indication */
        wait_for_completion_interruptible(&pSchedContext->ResumeTxEvent);
      }

      break; //All queues are empty now
    } // while message loop processing
  } // while TRUE
  // If we get here the TX thread must exit
  VOS_TRACE( VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
      "%s: TX Thread exiting!!!!", __FUNCTION__);
  complete_and_exit(&pSchedContext->TxShutdown, 0);
} /* VosTxThread() */

#ifdef FEATURE_WLAN_INTEGRATED_SOC
/*---------------------------------------------------------------------------
  \brief VosRXThread() - The VOSS Main Rx thread
  The \a VosRxThread() is the VOSS Rx controller thread:
  \param  Arg - pointer to the global vOSS Sched Context

  \return Thread exit code
  \sa VosRxThread()
  -------------------------------------------------------------------------*/

static int VosRXThread ( void * Arg )
{
  pVosSchedContext pSchedContext = (pVosSchedContext)Arg;
  pVosMsgWrapper   pMsgWrapper   = NULL;
  int              retWaitStatus = 0;
  v_BOOL_t shutdown = VOS_FALSE;
  hdd_context_t *pHddCtx         = NULL;
  v_CONTEXT_t pVosContext        = NULL;

  set_user_nice(current, -1);

  if (Arg == NULL)
  {
     VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
         "%s Bad Args passed", __FUNCTION__);
     return 0;
  }
  daemonize("RX_Thread");
  /*
  ** Ack back to the context from which the main controller thread has been
  ** created.
  */
  complete(&pSchedContext->RxStartEvent);
  VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
      "%s: RX Thread %d (%s) starting up!",__func__, current->pid, current->comm);

  /* Get the Global VOSS Context */
  pVosContext = vos_get_global_context(VOS_MODULE_ID_SYS, NULL);
  if(!pVosContext) {
     hddLog(VOS_TRACE_LEVEL_FATAL,"%s: Global VOS context is Null", __func__);
     return 0;
  }

  /* Get the HDD context */
  pHddCtx = (hdd_context_t *)vos_get_context(VOS_MODULE_ID_HDD, pVosContext );
  if(!pHddCtx) {
     hddLog(VOS_TRACE_LEVEL_FATAL,"%s: HDD context is Null",__func__);
     return 0;
  }

  while(!shutdown)
  {
    // This implements the execution model algorithm
    retWaitStatus = wait_event_interruptible(pSchedContext->rxWaitQueue,
        test_bit(RX_POST_EVENT_MASK, &pSchedContext->rxEventFlag) || 
        test_bit(RX_SUSPEND_EVENT_MASK, &pSchedContext->rxEventFlag));


    if(retWaitStatus == -ERESTARTSYS)
    {
        VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
           "%s: wait_event_interruptible returned -ERESTARTSYS", __FUNCTION__);
        break;
    }
    clear_bit(RX_POST_EVENT_MASK, &pSchedContext->rxEventFlag);

    while(1)
    {
      if(test_bit(RX_SHUTDOWN_EVENT_MASK, &pSchedContext->rxEventFlag))
      {
        VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_FATAL,
                 "%s: RX thread signaled for shutdown",__func__);
        shutdown = VOS_TRUE;
        break;
      }
#if 0 
//TO DO: Implement this
        /* Check for any Suspend Indication */
        if(test_bit(RX_SUSPEND_EVENT_MASK, &pSchedContext->rxEventFlag))
        {
           clear_bit(RX_SUSPEND_EVENT_MASK, &pSchedContext->rxEventFlag);
        
           /* Unblock anyone waiting on suspend */
           complete(&pHddCtx->mc_sus_event_var);
        }
#endif
      // Check the WDI queue
      if (!vos_is_mq_empty(&pSchedContext->wdiRxMq))
      {
        wpt_msg *pWdiMsg;
        VOS_TRACE(VOS_MODULE_ID_WDI, VOS_TRACE_LEVEL_INFO,
                  "%s: Servicing the VOS RX WDI Message queue",__func__);

        pMsgWrapper = vos_mq_get(&pSchedContext->wdiRxMq);
        VOS_ASSERT(NULL != pMsgWrapper);

        pWdiMsg = (wpt_msg *)pMsgWrapper->pVosMsg->bodyptr;
        VOS_ASSERT(pWdiMsg->callback);

        pWdiMsg->callback(pWdiMsg);

        // return message to the Core
        vos_core_return_msg(pSchedContext->pVContext, pMsgWrapper);

        continue;
      }

      /* Check for any Suspend Indication */
      if(test_bit(RX_SUSPEND_EVENT_MASK, &pSchedContext->rxEventFlag))
      {
        clear_bit(RX_SUSPEND_EVENT_MASK, &pSchedContext->rxEventFlag);

        /* Rx Thread Suspended */
        complete(&pHddCtx->rx_sus_event_var);

        init_completion(&pSchedContext->ResumeRxEvent);

        /* Wait for Resume Indication */
        wait_for_completion_interruptible(&pSchedContext->ResumeRxEvent);
      }

      break; //All queues are empty now
    } // while message loop processing
  } // while TRUE
  // If we get here the RX thread must exit
  VOS_TRACE( VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
      "%s: RX Thread exiting!!!!", __FUNCTION__);
  complete_and_exit(&pSchedContext->RxShutdown, 0);
} /* VosRxThread() */
#endif

#endif
/*---------------------------------------------------------------------------
  \brief vos_sched_close() - Close the vOSS Scheduler
  The \a vos_sched_closes() function closes the vOSS Scheduler
  Upon successful closing:
     - All the message queues are flushed
     - The Main Controller thread is closed
     - The Tx thread is closed

  \param  pVosContext - pointer to the global vOSS Context
  \return VOS_STATUS_SUCCESS - Scheduler was successfully initialized and
          is ready to be used.
          VOS_STATUS_E_INVAL - Invalid parameter passed to the scheduler Open
          function
          VOS_STATUS_E_FAILURE - Failure to initialize the scheduler/
  \sa vos_sched_close()
---------------------------------------------------------------------------*/
VOS_STATUS vos_sched_close ( v_PVOID_t pVosContext )
{
    VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
        "%s: vos_schdeuler closing now", __FUNCTION__);
    if (gpVosSchedContext == NULL)
    {
       VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
           "%s: gpVosSchedContext == NULL\n",__FUNCTION__);
       return VOS_STATUS_E_FAILURE;
    }
#ifndef FEATURE_WLAN_INTEGRATED_SOC
    /* !!! FIX ME
     * PRIMA this is not working, should be FIXED ASAP */
    set_bit(MC_SHUTDOWN_EVENT_MASK, &gpVosSchedContext->mcEventFlag);
    set_bit(MC_POST_EVENT_MASK, &gpVosSchedContext->mcEventFlag);
    wake_up_interruptible(&gpVosSchedContext->mcWaitQueue);
    //Wait for MC to exit
    wait_for_completion_interruptible(&gpVosSchedContext->McShutdown);
#endif /* FEATURE_WLAN_INTEGRATED_SOC */
#if !defined(ANI_MANF_DIAG) || defined(FEATURE_WLAN_INTEGRATED_SOC)
    set_bit(TX_SHUTDOWN_EVENT_MASK, &gpVosSchedContext->txEventFlag);
    set_bit(TX_POST_EVENT_MASK, &gpVosSchedContext->txEventFlag);
    wake_up_interruptible(&gpVosSchedContext->txWaitQueue);
#endif
    //Wait for TX to exit
    wait_for_completion_interruptible(&gpVosSchedContext->TxShutdown);
#ifdef FEATURE_WLAN_INTEGRATED_SOC
    set_bit(RX_SHUTDOWN_EVENT_MASK, &gpVosSchedContext->rxEventFlag);
    set_bit(RX_POST_EVENT_MASK, &gpVosSchedContext->rxEventFlag);
    wake_up_interruptible(&gpVosSchedContext->rxWaitQueue);
    //Wait for RX to exit
    wait_for_completion_interruptible(&gpVosSchedContext->RxShutdown);
   gpVosSchedContext->RxThread = 0;
#endif
    gpVosSchedContext->McThread = 0;
    gpVosSchedContext->TxThread = 0;
    //Clean up message queues of TX and MC thread
    vos_sched_flush_mc_mqs(gpVosSchedContext);
#if !defined(ANI_MANF_DIAG) || defined(FEATURE_WLAN_INTEGRATED_SOC)
    vos_sched_flush_tx_mqs(gpVosSchedContext);
#ifdef FEATURE_WLAN_INTEGRATED_SOC
    vos_sched_flush_rx_mqs(gpVosSchedContext);
#endif 
#endif

    //Deinit all the queues
    vos_sched_deinit_mqs(gpVosSchedContext);

    return VOS_STATUS_SUCCESS;
} /* vox_sched_close() */

VOS_STATUS vos_watchdog_close ( v_PVOID_t pVosContext )
{
    VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
        "%s: vos_watchdog closing now", __FUNCTION__);
    if (gpVosWatchdogContext == NULL)
    {
       VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
           "%s: gpVosWatchdogContext is NULL\n",__FUNCTION__);
       return VOS_STATUS_E_FAILURE;
    }
    set_bit(WD_SHUTDOWN_EVENT_MASK, &gpVosWatchdogContext->wdEventFlag);
    set_bit(WD_POST_EVENT_MASK, &gpVosWatchdogContext->wdEventFlag);
    wake_up_interruptible(&gpVosWatchdogContext->wdWaitQueue);
    //Wait for Watchdog thread to exit
    wait_for_completion_interruptible(&gpVosWatchdogContext->WdShutdown);
    return VOS_STATUS_SUCCESS;
} /* vos_watchdog_close() */

VOS_STATUS vos_watchdog_chip_reset ( v_VOID_t )
{
    v_CONTEXT_t pVosContext = NULL;
    hdd_adapter_t *pAdapter = NULL;
    VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_FATAL,
        "%s: vos_watchdog resetting WLAN", __FUNCTION__);
    if (gpVosWatchdogContext == NULL)
    {
       VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_FATAL,
           "%s: Watchdog not enabled. LOGP ignored.",__FUNCTION__);
       return VOS_STATUS_E_FAILURE;
    }

    if (gpVosWatchdogContext->resetInProgress)
    {
        VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_FATAL,
            "%s: Reset already in Progress. Ignoring signaling Watchdog",__FUNCTION__);
        return VOS_STATUS_E_FAILURE;
    }

    VOS_ASSERT(0);
    
    pVosContext = vos_get_global_context(VOS_MODULE_ID_HDD, NULL);
    pAdapter = (hdd_adapter_t *)vos_get_context(VOS_MODULE_ID_HDD,pVosContext);

    /* Set the flags so that all future CMD53 and Wext commands get blocked right away */
    vos_set_logp_in_progress(VOS_MODULE_ID_VOSS, TRUE);
    (WLAN_HDD_GET_CTX(pAdapter))->isLogpInProgress = TRUE;

    if ((WLAN_HDD_GET_CTX(pAdapter))->isLoadUnloadInProgress)
    {
        VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_FATAL,
            "%s: Load/unload in Progress. Ignoring signaling Watchdog",__FUNCTION__);
        return VOS_STATUS_E_FAILURE;    
    }

#ifdef CONFIG_HAS_EARLYSUSPEND
    if(VOS_STATUS_SUCCESS != hdd_wlan_reset_initialization())
    {
       /* This can fail if card got removed by SDCC during resume */
       VOS_ASSERT(0);
    }
#endif

    set_bit(WD_CHIP_RESET_EVENT_MASK, &gpVosWatchdogContext->wdEventFlag);
    set_bit(WD_POST_EVENT_MASK, &gpVosWatchdogContext->wdEventFlag);
    wake_up_interruptible(&gpVosWatchdogContext->wdWaitQueue);
    return VOS_STATUS_SUCCESS;
} /* vos_watchdog_chip_reset() */
/*---------------------------------------------------------------------------
  \brief vos_sched_init_mqs: Initialize the vOSS Scheduler message queues
  The \a vos_sched_init_mqs() function initializes the vOSS Scheduler
  message queues.
  \param  pVosSchedContext - pointer to the Scheduler Context.
  \return VOS_STATUS_SUCCESS - Scheduler was successfully initialized and
          is ready to be used.
          VOS_STATUS_E_RESOURCES - System resources (other than memory)
          are unavailable to initilize the scheduler

  \sa vos_sched_init_mqs()
  -------------------------------------------------------------------------*/
VOS_STATUS vos_sched_init_mqs ( pVosSchedContext pSchedContext )
{
  VOS_STATUS vStatus = VOS_STATUS_SUCCESS;
  // Now intialize all the message queues
#ifdef FEATURE_WLAN_INTEGRATED_SOC
  VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_INFO_HIGH,
            "%s: Initializing the WDA MC Message queue",__func__);
  vStatus = vos_mq_init(&pSchedContext->wdaMcMq);
  if (! VOS_IS_STATUS_SUCCESS(vStatus))
  {
    VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
            "%s: Failed to init WDA MC Message queue",__func__);
    VOS_ASSERT(0);
    return vStatus;
  }
#else
  VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_INFO_HIGH,
            "%s: Initializing the HAL MC Message queue",__func__);
  vStatus = vos_mq_init(&pSchedContext->halMcMq);
  if (! VOS_IS_STATUS_SUCCESS(vStatus))
  {
    VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
            "%s: Failed to init HAL MC Message queue",__func__);
    VOS_ASSERT(0);
    return vStatus;
  }
#endif
#if !defined(ANI_MANF_DIAG) || defined(FEATURE_WLAN_INTEGRATED_SOC)
  VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_INFO_HIGH,
            "%s: Initializing the PE MC Message queue",__func__);
  vStatus = vos_mq_init(&pSchedContext->peMcMq);
  if (! VOS_IS_STATUS_SUCCESS(vStatus))
  {
    VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
            "%s: Failed to init PE MC Message queue",__func__);
    VOS_ASSERT(0);
    return vStatus;
  }
  VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_INFO_HIGH,
            "%s: Initializing the SME MC Message queue", __func__);
  vStatus = vos_mq_init(&pSchedContext->smeMcMq);
  if (! VOS_IS_STATUS_SUCCESS(vStatus))
  {
    VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
            "%s: Failed to init SME MC Message queue",__func__);
    VOS_ASSERT(0);
    return vStatus;
  }
  VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_INFO_HIGH,
            "%s: Initializing the TL MC Message queue",__func__);
  vStatus = vos_mq_init(&pSchedContext->tlMcMq);
  if (! VOS_IS_STATUS_SUCCESS(vStatus))
  {
    VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
            "%s: Failed to init TL MC Message queue",__func__);
    VOS_ASSERT(0);
    return vStatus;
  }
#endif
  VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_INFO_HIGH,
            "%s: Initializing the SYS MC Message queue",__func__);
  vStatus = vos_mq_init(&pSchedContext->sysMcMq);
  if (! VOS_IS_STATUS_SUCCESS(vStatus))
  {
    VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
            "%s: Failed to init SYS MC Message queue",__func__);
    VOS_ASSERT(0);
    return vStatus;
  }
#ifdef FEATURE_WLAN_INTEGRATED_SOC
  VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_INFO_HIGH,
            "%s: Initializing the WDI MC Message queue",__func__);

  vStatus = vos_mq_init(&pSchedContext->wdiMcMq);
  if (! VOS_IS_STATUS_SUCCESS(vStatus))
  {
    VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
            "%s: Failed to init WDI MC Message queue",__func__);
    VOS_ASSERT(0);
    return vStatus;
  }
#endif

#if !defined(ANI_MANF_DIAG) || defined(FEATURE_WLAN_INTEGRATED_SOC)
  VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_INFO_HIGH,
            "%s: Initializing the TL Tx Message queue",__func__);
  vStatus = vos_mq_init(&pSchedContext->tlTxMq);
  if (! VOS_IS_STATUS_SUCCESS(vStatus))
  {
    VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
            "%s: Failed to init TL TX Message queue",__func__);
    VOS_ASSERT(0);
    return vStatus;
  }
#ifndef FEATURE_WLAN_INTEGRATED_SOC
  VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_INFO_HIGH,
            "%s: Initializing the SSC Tx Message queue",__func__);
  vStatus = vos_mq_init(&pSchedContext->sscTxMq);
  if (! VOS_IS_STATUS_SUCCESS(vStatus))
  {
    VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
            "%s: Failed to init SSC TX Message queue",__func__);
    VOS_ASSERT(0);
    return vStatus;
  }
#else
  VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_INFO_HIGH,
            "%s: Initializing the WDI Tx Message queue",__func__);
  vStatus = vos_mq_init(&pSchedContext->wdiTxMq);
  if (! VOS_IS_STATUS_SUCCESS(vStatus))
  {
    VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
            "%s: Failed to init WDI TX Message queue",__func__);
    VOS_ASSERT(0);
    return vStatus;
  }

  VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_INFO_HIGH,
            "%s: Initializing the WDI Rx Message queue",__func__);

  vStatus = vos_mq_init(&pSchedContext->wdiRxMq);
  if (! VOS_IS_STATUS_SUCCESS(vStatus))
  {
    VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
            "%s: Failed to init WDI RX Message queue",__func__);
    VOS_ASSERT(0);
    return vStatus;
  }
#endif
  VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_INFO_HIGH,
            "%s: Initializing the SYS Tx Message queue",__func__);
  vStatus = vos_mq_init(&pSchedContext->sysTxMq);
  if (! VOS_IS_STATUS_SUCCESS(vStatus))
  {
    VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
            "%s: Failed to init SYS TX Message queue",__func__);
    VOS_ASSERT(0);
    return vStatus;
  }
#endif
  return VOS_STATUS_SUCCESS;
} /* vos_sched_init_mqs() */

/*---------------------------------------------------------------------------
  \brief vos_sched_deinit_mqs: Deinitialize the vOSS Scheduler message queues
  The \a vos_sched_init_mqs() function deinitializes the vOSS Scheduler
  message queues.
  \param  pVosSchedContext - pointer to the Scheduler Context.
  \return None
  \sa vos_sched_deinit_mqs()
  -------------------------------------------------------------------------*/
void vos_sched_deinit_mqs ( pVosSchedContext pSchedContext )
{
  // Now de-intialize all message queues
#ifndef FEATURE_WLAN_INTEGRATED_SOC
  // MC HAL
  VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_INFO_HIGH,
            "%s De-Initializing the HAL MC Message queue",__func__);
  vos_mq_deinit(&pSchedContext->halMcMq);
#else
 // MC WDA
  VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_INFO_HIGH,
            "%s De-Initializing the WDA MC Message queue",__func__);
  vos_mq_deinit(&pSchedContext->wdaMcMq);
#endif
#if !defined(ANI_MANF_DIAG) || defined(FEATURE_WLAN_INTEGRATED_SOC)
  //MC PE
  VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_INFO_HIGH,
            "%s De-Initializing the PE MC Message queue",__func__);
  vos_mq_deinit(&pSchedContext->peMcMq);
  //MC SME
  VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_INFO_HIGH,
            "%s De-Initializing the SME MC Message queue",__func__);
  vos_mq_deinit(&pSchedContext->smeMcMq);
  //MC TL
  VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_INFO_HIGH,
            "%s De-Initializing the TL MC Message queue",__func__);
  vos_mq_deinit(&pSchedContext->tlMcMq);
#endif
  //MC SYS
  VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_INFO_HIGH,
            "%s De-Initializing the SYS MC Message queue",__func__);
  vos_mq_deinit(&pSchedContext->sysMcMq);
#ifdef FEATURE_WLAN_INTEGRATED_SOC
  // MC WDI
  VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_INFO_HIGH,
            "%s De-Initializing the WDI MC Message queue",__func__);
  vos_mq_deinit(&pSchedContext->wdiMcMq);
#endif

#if !defined(ANI_MANF_DIAG) || defined(FEATURE_WLAN_INTEGRATED_SOC)
  //Tx TL
  VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_INFO_HIGH,
            "%s De-Initializing the TL Tx Message queue",__func__);
  vos_mq_deinit(&pSchedContext->tlTxMq);
#ifndef FEATURE_WLAN_INTEGRATED_SOC
  //Tx SSC
  VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_INFO_HIGH,
            "%s: DeInitializing the SSC Tx Message queue",__func__);
  vos_mq_deinit(&pSchedContext->sscTxMq);

#else
  //Tx WDI
  VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_INFO_HIGH,
            "%s: DeInitializing the WDI Tx Message queue",__func__);
  vos_mq_deinit(&pSchedContext->wdiTxMq);


  //Rx WDI
  VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_INFO_HIGH,
            "%s: DeInitializing the WDI Rx Message queue",__func__);
  vos_mq_deinit(&pSchedContext->wdiRxMq);
#endif

  //Tx SYS
  VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_INFO_HIGH,
            "%s: DeInitializing the SYS Tx Message queue",__func__);
  vos_mq_deinit(&pSchedContext->sysTxMq);
#endif
} /* vos_sched_deinit_mqs() */

/*-------------------------------------------------------------------------
 this helper function flushes all the MC message queues
 -------------------------------------------------------------------------*/
void vos_sched_flush_mc_mqs ( pVosSchedContext pSchedContext )
{
  pVosMsgWrapper pMsgWrapper = NULL;
  pVosContextType vosCtx = (pVosContextType)(pSchedContext->pVContext);
  /*
  ** Here each of the MC thread MQ shall be drained and returned to the
  ** Core. Before returning a wrapper to the Core, the VOS message shall be
  ** freed  first
  */
  VOS_TRACE( VOS_MODULE_ID_VOSS,
             VOS_TRACE_LEVEL_INFO,
             ("Flushing the MC Thread message queue\n") );
  VOS_ASSERT(NULL != pSchedContext);
  /* Flush the SYS Mq */
  while( NULL != (pMsgWrapper = vos_mq_get(&pSchedContext->sysMcMq) ))
  {
    VOS_TRACE( VOS_MODULE_ID_VOSS,
               VOS_TRACE_LEVEL_INFO,
               "%s: Freeing MC SYS message type %d ",__func__,
               pMsgWrapper->pVosMsg->type );
    sysMcFreeMsg(pSchedContext->pVContext, pMsgWrapper->pVosMsg);
    vos_core_return_msg(pSchedContext->pVContext, pMsgWrapper);
  }
#ifndef FEATURE_WLAN_INTEGRATED_SOC
  /* Flush the HAL Mq */
  while( NULL != (pMsgWrapper = vos_mq_get(&pSchedContext->halMcMq) ))
  {
    VOS_TRACE( VOS_MODULE_ID_VOSS,
               VOS_TRACE_LEVEL_INFO,
               "%s: Freeing MC HAL MSG message type %d",__func__,
               pMsgWrapper->pVosMsg->type );
    halFreeMsg(vosCtx->pMACContext, (tSirMsgQ*)pMsgWrapper->pVosMsg);
    vos_core_return_msg(pSchedContext->pVContext, pMsgWrapper);
  }
#else
  /* Flush the WDA Mq */
  while( NULL != (pMsgWrapper = vos_mq_get(&pSchedContext->wdaMcMq) ))
  {
    VOS_TRACE( VOS_MODULE_ID_VOSS,
               VOS_TRACE_LEVEL_INFO,
               "%s: Freeing MC WDA MSG message type %d",__func__,
               pMsgWrapper->pVosMsg->type );
    if(pMsgWrapper->pVosMsg != NULL) 
    {
        if (pMsgWrapper->pVosMsg->bodyptr) {
            vos_mem_free((v_VOID_t*)pMsgWrapper->pVosMsg->bodyptr);
        }

        pMsgWrapper->pVosMsg->bodyptr = NULL;
        pMsgWrapper->pVosMsg->bodyval = 0;
        pMsgWrapper->pVosMsg->type = 0;
    }
    vos_core_return_msg(pSchedContext->pVContext, pMsgWrapper);
  }

  /* Flush the WDI Mq */
  while( NULL != (pMsgWrapper = vos_mq_get(&pSchedContext->wdiMcMq) ))
  {
    VOS_TRACE( VOS_MODULE_ID_VOSS,
               VOS_TRACE_LEVEL_INFO,
               "%s: Freeing MC WDA MSG message type %d",__func__,
               pMsgWrapper->pVosMsg->type );
    if(pMsgWrapper->pVosMsg != NULL)
    {
        if (pMsgWrapper->pVosMsg->bodyptr) {
            vos_mem_free((v_VOID_t*)pMsgWrapper->pVosMsg->bodyptr);
        }

        pMsgWrapper->pVosMsg->bodyptr = NULL;
        pMsgWrapper->pVosMsg->bodyval = 0;
        pMsgWrapper->pVosMsg->type = 0;
    }
    vos_core_return_msg(pSchedContext->pVContext, pMsgWrapper);
  }

#endif
#if !defined(ANI_MANF_DIAG) || defined(FEATURE_WLAN_INTEGRATED_SOC)
  /* Flush the PE Mq */
  while( NULL != (pMsgWrapper = vos_mq_get(&pSchedContext->peMcMq) ))
  {
    VOS_TRACE( VOS_MODULE_ID_VOSS,
               VOS_TRACE_LEVEL_INFO,
               "%s: Freeing MC PE MSG message type %d",__func__,
               pMsgWrapper->pVosMsg->type );
    peFreeMsg(vosCtx->pMACContext, (tSirMsgQ*)pMsgWrapper->pVosMsg);
    vos_core_return_msg(pSchedContext->pVContext, pMsgWrapper);
  }
  /* Flush the SME Mq */
  while( NULL != (pMsgWrapper = vos_mq_get(&pSchedContext->smeMcMq) ))
  {
    VOS_TRACE( VOS_MODULE_ID_VOSS,
               VOS_TRACE_LEVEL_INFO,
               "%s: Freeing MC SME MSG message type %d", __func__,
               pMsgWrapper->pVosMsg->type );
    sme_FreeMsg(vosCtx->pMACContext, pMsgWrapper->pVosMsg);
    vos_core_return_msg(pSchedContext->pVContext, pMsgWrapper);
  }
    /* Flush the TL Mq */
  while( NULL != (pMsgWrapper = vos_mq_get(&pSchedContext->tlMcMq) ))
  {
    VOS_TRACE( VOS_MODULE_ID_VOSS,
               VOS_TRACE_LEVEL_INFO,
               "%s: Freeing MC TL message type %d",__func__,
               pMsgWrapper->pVosMsg->type );
    WLANTL_McFreeMsg(pSchedContext->pVContext, pMsgWrapper->pVosMsg);
    vos_core_return_msg(pSchedContext->pVContext, pMsgWrapper);
  }
#endif
} /* vos_sched_flush_mc_mqs() */

/*-------------------------------------------------------------------------
 This helper function flushes all the TX message queues
 ------------------------------------------------------------------------*/
void vos_sched_flush_tx_mqs ( pVosSchedContext pSchedContext )
{
  pVosMsgWrapper pMsgWrapper = NULL;
  /*
  ** Here each of the TX thread MQ shall be drained and returned to the
  ** Core. Before returning a wrapper to the Core, the VOS message shall
  ** be freed first
  */
  VOS_TRACE( VOS_MODULE_ID_VOSS,
             VOS_TRACE_LEVEL_INFO,
             "%s: Flushing the TX Thread message queue",__func__);
  VOS_ASSERT(NULL != pSchedContext);
  if (pSchedContext == NULL)
  {
     VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
         "%s: pSchedContext is NULL", __FUNCTION__);
     return;
  }
  /* Flush the SYS Mq */
  while( NULL != (pMsgWrapper = vos_mq_get(&pSchedContext->sysTxMq) ))
  {
    VOS_TRACE( VOS_MODULE_ID_VOSS,
               VOS_TRACE_LEVEL_INFO,
               "%s: Freeing TX SYS message type %d",__func__,
               pMsgWrapper->pVosMsg->type );
    sysTxFreeMsg(pSchedContext->pVContext, pMsgWrapper->pVosMsg);
    vos_core_return_msg(pSchedContext->pVContext, pMsgWrapper);
  }
  /* Flush the TL Mq */
  while( NULL != (pMsgWrapper = vos_mq_get(&pSchedContext->tlTxMq) ))
  {
    VOS_TRACE( VOS_MODULE_ID_VOSS,
               VOS_TRACE_LEVEL_INFO,
               "%s: Freeing TX TL MSG message type %d",__func__,
               pMsgWrapper->pVosMsg->type );
    WLANTL_TxFreeMsg(pSchedContext->pVContext, pMsgWrapper->pVosMsg);
    vos_core_return_msg(pSchedContext->pVContext, pMsgWrapper);
  }
#ifndef FEATURE_WLAN_INTEGRATED_SOC
  /* Flush the SSC Mq */
  while( NULL != (pMsgWrapper = vos_mq_get(&pSchedContext->sscTxMq) ))
  {
    VOS_TRACE( VOS_MODULE_ID_VOSS,
               VOS_TRACE_LEVEL_INFO,
               "%s: Freeing TX SSC MSG message type %d",__func__,
               pMsgWrapper->pVosMsg->type );
    WLANSSC_FreeMsg(pSchedContext->pVContext, pMsgWrapper->pVosMsg);
    vos_core_return_msg(pSchedContext->pVContext, pMsgWrapper);
  }
#else
  /* Flush the WDI Mq */
  while( NULL != (pMsgWrapper = vos_mq_get(&pSchedContext->wdiTxMq) ))
  {
    VOS_TRACE( VOS_MODULE_ID_VOSS,
               VOS_TRACE_LEVEL_INFO,
               "%s: Freeing TX WDI MSG message type %d",__func__,
               pMsgWrapper->pVosMsg->type );
    sysTxFreeMsg(pSchedContext->pVContext, pMsgWrapper->pVosMsg);
    vos_core_return_msg(pSchedContext->pVContext, pMsgWrapper);
  }
 #endif
} /* vos_sched_flush_tx_mqs() */
#ifdef FEATURE_WLAN_INTEGRATED_SOC
/*-------------------------------------------------------------------------
 This helper function flushes all the RX message queues
 ------------------------------------------------------------------------*/
void vos_sched_flush_rx_mqs ( pVosSchedContext pSchedContext )
{
  pVosMsgWrapper pMsgWrapper = NULL;
  /*
  ** Here each of the RX thread MQ shall be drained and returned to the
  ** Core. Before returning a wrapper to the Core, the VOS message shall
  ** be freed first
  */
  VOS_TRACE( VOS_MODULE_ID_VOSS,
             VOS_TRACE_LEVEL_INFO,
             "%s: Flushing the RX Thread message queue",__func__);
  VOS_ASSERT(NULL != pSchedContext);
  if (pSchedContext == NULL)
  {
     VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
         "%s: pSchedContext is NULL", __FUNCTION__);
     return;
  }
  while( NULL != (pMsgWrapper = vos_mq_get(&pSchedContext->wdiRxMq) ))
  {
    VOS_TRACE( VOS_MODULE_ID_VOSS,
               VOS_TRACE_LEVEL_INFO,
               "%s: Freeing RX WDI MSG message type %d",__func__,
               pMsgWrapper->pVosMsg->type );
    sysTxFreeMsg(pSchedContext->pVContext, pMsgWrapper->pVosMsg);
  }
}/* vos_sched_flush_rx_mqs() */
#endif

/*-------------------------------------------------------------------------
 This helper function helps determine if thread id is of TX thread
 ------------------------------------------------------------------------*/
int vos_sched_is_tx_thread(int threadID)
{
   // Make sure that Vos Scheduler context has been initialized
   VOS_ASSERT( NULL != gpVosSchedContext);
   if (gpVosSchedContext == NULL)
   {
      VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
          "%s: gpVosSchedContext == NULL",__FUNCTION__);
      return 0;
   }
   return (threadID == gpVosSchedContext->TxThread);
}
/*-------------------------------------------------------------------------
 Helper function to get the scheduler context
 ------------------------------------------------------------------------*/
pVosSchedContext get_vos_sched_ctxt(void)
{
   //Make sure that Vos Scheduler context has been initialized
   if (gpVosSchedContext == NULL)
   {
      VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
         "%s: gpVosSchedContext == NULL",__FUNCTION__);
   }
   return (gpVosSchedContext);
}
/*-------------------------------------------------------------------------
 Helper function to get the watchdog context
 ------------------------------------------------------------------------*/
pVosWatchdogContext get_vos_watchdog_ctxt(void)
{
   //Make sure that Vos Scheduler context has been initialized
   if (gpVosWatchdogContext == NULL)
   {
      VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
         "%s: gpVosWatchdogContext == NULL",__FUNCTION__);
   }
   return (gpVosWatchdogContext);
}
