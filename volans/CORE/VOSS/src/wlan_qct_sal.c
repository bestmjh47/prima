/**=========================================================================

  @file  wlan_qct_sal.c

  @brief WLAN SDIO ABSTRACTION LAYER EXTERNAL API FOR LINUX SPECIFIC PLATFORM

   This file contains the external API exposed by the wlan SDIO abstraction layer module.
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

===========================================================================*/

/*===========================================================================

                          INCLUDE FILES FOR MODULE

===========================================================================*/

/*----------------------------------------------------------------------------
 * Include Files
 * -------------------------------------------------------------------------*/
#include <wlan_hdd_includes.h>
#include <wlan_qct_sal.h>
#include <wlan_sal_misc.h> // Linux specific includes
#include <wlan_qct_hal.h>
#include <wlan_bal_misc.h> // Linux specific includes
#include <linux/mmc/sdio.h>
#include <vos_power.h>

/*----------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * -------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 *  Type Declarations
 * -------------------------------------------------------------------------*/
#ifdef VOLANS_1_0_WORKAROUND
/* The below funxtion definintion is in librasdioif driver */
extern void libra_sdio_set_clock(struct sdio_func *func, unsigned int clk_freq);
extern void libra_sdio_get_card_id(struct sdio_func *func, unsigned short *card_id);
#endif /* VOLANS_1_0_WORKAROUND */

#ifdef MSM_PLATFORM /* Specific to Android Platform */
extern int sdio_reset_comm(struct mmc_card *card);
#endif /* MSM_PLATFORM */
/*-------------------------------------------------------------------------
 * Global variables.
 *-------------------------------------------------------------------------*/
static salHandleType *gpsalHandle;

/*----------------------------------------------------------------------------

   @brief Hardware Interrupt handle

   @param SD_DEVICE_HANDLE  hDevice
   @param DWORD             pAdapter

   @return General status code
           VOS_STATUS_SUCCESS      Handle sunccess
           TBD

----------------------------------------------------------------------------*/
void salRxInterruptCB
(
   struct sdio_func *sdio_func_dev
)
{
   v_PVOID_t pAdapter = NULL;

   VOS_ASSERT(sdio_func_dev);
   VOS_ASSERT(gpsalHandle);

   VOS_ASSERT(gpsalHandle->sscCBs.interruptCB);
   VOS_ASSERT(gpsalHandle->sscCBs.sscUsrData);

   pAdapter =  libra_sdio_getprivdata(sdio_func_dev);

   ((hdd_adapter_t *)pAdapter)->pid_sdio_claimed = current->pid;
   atomic_inc(&((hdd_adapter_t *)pAdapter)->sdio_claim_count);

   /* Release SDIO lock acquired by irq_sdio task before calling salRxInterruptCB. 
    * As we are anyway acquiring the same lock again while doing SDIO operation. 
    * Reason for releasing it here, to have synchronization of SDIO lock with other 
    * locks used in the driver. This is hack, we need to come up with proper soultion
    *  of this.
    */
   sd_release_host(sdio_func_dev);

   gpsalHandle->sscCBs.interruptCB(pAdapter, gpsalHandle->sscCBs.sscUsrData);

   /* Acquring the SDIO lock again as we release earlier. This is done so that
    * irq_sdio thread can release it after returning from here. 
    */
   sd_claim_host(sdio_func_dev);

   ((hdd_adapter_t *)pAdapter)->pid_sdio_claimed = 0; 
   atomic_dec(&((hdd_adapter_t *)pAdapter)->sdio_claim_count);
}

/*----------------------------------------------------------------------------

   @brief Function to workaround an ASIC CR.
   This is a worakound a Liba POR issue whe PMU comes up too early before SIF. This HW
   issue is slated to be fixed in Libra 2.0

   @param none

   @param v_U32_t   none

   @return General status code
        VOS_STATUS_SUCCESS      Open Success
        VOS_STATUS_E_NOMEM      Open Fail, Resource alloc fail
        VOS_STATUS_E_FAILURE    Open Fail, Unknown reason

----------------------------------------------------------------------------*/
void PmuFixWorkAround(int *err_ret)
{
   v_U8_t                     regValue;
   int                        save_func_num = 0;

   VOS_ASSERT(NULL != gpsalHandle);

   SENTER();

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
   gpsalHandle->sdio_func_dev = libra_getsdio_funcdev();
   save_func_num = gpsalHandle->sdio_func_dev->num;
   gpsalHandle->sdio_func_dev->num = 0;

   VOS_TRACE(VOS_MODULE_ID_SAL, VOS_TRACE_LEVEL_INFO,"sdio_dev=%08x\n", (unsigned int)(gpsalHandle->sdio_func_dev));

   libra_sdiocmd52( gpsalHandle->sdio_func_dev, QWLAN_SIF_BAR4_WLAN_CONTROL_REG_REG, &regValue,
         0, err_ret);

   if (VOS_STATUS_SUCCESS != *err_ret)
   {
      VOS_TRACE(VOS_MODULE_ID_SAL, VOS_TRACE_LEVEL_ERROR,"Err1 %d", *err_ret);
      gpsalHandle->sdio_func_dev->num = save_func_num;
      *err_ret = VOS_STATUS_E_FAILURE;
      return;
   }

   // When this bit is set to 0, WLAN will be placed in reset state.
   if ((regValue & QWLAN_SIF_BAR4_WLAN_STATUS_REG_PMU_BLOCKED_BIT_MASK) == 0)
   {
      // WLAN is somehow already awaken for whatever reason
      libra_sdiocmd52( gpsalHandle->sdio_func_dev, QWLAN_SIF_BAR4_WLAN_PWR_SAVE_CONTROL_REG_REG, &regValue,
         0, err_ret);
      if( VOS_STATUS_SUCCESS != *err_ret )
      {
         SMSGERROR("SAL_Start Read reg SIF_BAR4_WLAN_PWR_SAVE_CONTROL_REG_REG Fail", 0, 0, 0);

         VOS_TRACE(VOS_MODULE_ID_SAL, VOS_TRACE_LEVEL_ERROR,"Err2");
         gpsalHandle->sdio_func_dev->num = save_func_num;
         *err_ret = VOS_STATUS_E_FAILURE;
         return;
      }

      regValue |= 0x01;
      libra_sdiocmd52( gpsalHandle->sdio_func_dev, QWLAN_SIF_BAR4_WLAN_PWR_SAVE_CONTROL_REG_REG, &regValue,
         1, err_ret);
      if( VOS_STATUS_SUCCESS != *err_ret)
      {
         SMSGERROR("SAL_Start Write reg SIF_BAR4_WLAN_PWR_SAVE_CONTROL_REG_REG Fail", 0, 0, 0);
         VOS_TRACE(VOS_MODULE_ID_SAL, VOS_TRACE_LEVEL_ERROR,"Err3\n");
         gpsalHandle->sdio_func_dev->num = save_func_num;
         *err_ret = VOS_STATUS_E_FAILURE;
         return;
      }

      vos_sleep (2);

      regValue &= ~0x01;
      libra_sdiocmd52( gpsalHandle->sdio_func_dev, QWLAN_SIF_BAR4_WLAN_PWR_SAVE_CONTROL_REG_REG, &regValue,
         1, err_ret);
      if( VOS_STATUS_SUCCESS != *err_ret)
      {
         SMSGERROR("SAL_Start Write reg SIF_BAR4_WLAN_PWR_SAVE_CONTROL_REG_REG Fail", 0, 0, 0);
         VOS_TRACE(VOS_MODULE_ID_SAL, VOS_TRACE_LEVEL_ERROR,"Err4\n");
         gpsalHandle->sdio_func_dev->num = save_func_num;
         *err_ret = VOS_STATUS_E_FAILURE;
         return;
      }
   }
   gpsalHandle->sdio_func_dev->num = save_func_num;
   VOS_TRACE(VOS_MODULE_ID_SAL, VOS_TRACE_LEVEL_INFO,"Pmu: Success\n");
   *err_ret = 0;
   return;
}


/*----------------------------------------------------------------------------

   @brief Open SAL Module.
        Allocate internal resources, Initialize LOCK element,
        and allocate SDIO handle

   @param v_PVOID_t pAdapter
        Global adapter handle

   @param v_U32_t   sdBusDCtxt
        Platform specific device context

   @return General status code
        VOS_STATUS_SUCCESS      Open Success
        VOS_STATUS_E_NOMEM      Open Fail, Resource alloc fail
        VOS_STATUS_E_FAILURE    Open Fail, Unknown reason

----------------------------------------------------------------------------*/
VOS_STATUS WLANSAL_Open
(
   v_PVOID_t pAdapter,
   v_U32_t   sdBusDCtxt
)
{
   VOS_STATUS        status    = VOS_STATUS_SUCCESS;

   VOS_ASSERT(NULL != pAdapter);

   SENTER();

   status = vos_alloc_context(pAdapter, VOS_MODULE_ID_SAL, (void *)&gpsalHandle, sizeof(salHandleType));

   VOS_ASSERT(VOS_STATUS_SUCCESS == status);
   VOS_ASSERT(gpsalHandle);

   // Initialize the mutex
   mutex_init(&gpsalHandle->lock);
   spin_lock_init(&gpsalHandle->spinlock);

   SEXIT();
   return VOS_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------

   @brief Start SAL module.
        Probing SDIO interface, get and store card information

   @param v_PVOID_t pAdapter
        Global adapter handle

   @return General status code
        VOS_STATUS_SUCCESS       Start Success
        VOS_STATUS_E_FAILURE     Start Fail, BAL Not open yet
        VOS_STATUS_E_INVAL       Invalid argument

----------------------------------------------------------------------------*/
VOS_STATUS WLANSAL_Start
(
   v_PVOID_t pAdapter
)
{
   WLANSAL_CardInfoType       cardConfig;
#define VOS_GET_BAL_CTXT(a)            vos_get_context(VOS_MODULE_ID_BAL, a)
   balHandleType              *balHandle = (balHandleType *)VOS_GET_BAL_CTXT(pAdapter);

   VOS_ASSERT(NULL != gpsalHandle);

   SENTER();

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
   // Function for Rx handler and
   // 2nd function pointer which is a
   // workaround for PMU problems on ASIC CR: 185723.
   // Liba POR issue when PMU comes up too early before SIF. This HW
   // issue is slated to be fixed in Libra 2.0
   if (libra_sdio_configure( salRxInterruptCB, PmuFixWorkAround, LIBRA_FUNC_ENABLE_TIMEOUT, WLANSAL_MAX_BLOCK_SIZE))
   {
      return VOS_STATUS_E_FAILURE;
   }

   // Set the global sdio func dev handle
   gpsalHandle->sdio_func_dev = libra_getsdio_funcdev();
   balHandle->sdio_func_dev   = libra_getsdio_funcdev();

   cardConfig.blockSize = WLANSAL_MAX_BLOCK_SIZE;
   WLANSAL_CardInfoUpdate(pAdapter, &cardConfig);

   gpsalHandle->isINTEnabled = VOS_TRUE;

#ifndef LIBRA_LINUX_PC
   /* Register with SDIO driver as client for Suspend/Resume */
   ///libra_sdio_configure_suspend_resume(wlan_sdio_suspend_hdlr, wlan_sdio_resume_hdlr);
#endif /* LIBRA_LINUX_PC */
   SEXIT();

   return VOS_STATUS_SUCCESS;
}


/*----------------------------------------------------------------------------

   @brief Stop SAL module.
        Initialize internal resources

   @param  v_PVOID_t pAdapter
        Global adapter handle

   @return General status code
        VOS_STATUS_SUCCESS       Stop Success
        VOS_STATUS_E_FAILURE     Stop Fail, BAL not started
        VOS_STATUS_E_INVAL       Invalid argument

----------------------------------------------------------------------------*/
VOS_STATUS WLANSAL_Stop
(
   v_PVOID_t pAdapter
)
{
   VOS_ASSERT(gpsalHandle);

   SENTER();

   // release sdio irq claim from our driver
   libra_sdio_deconfigure(gpsalHandle->sdio_func_dev);

   SEXIT();

   return VOS_STATUS_SUCCESS;
}


/*----------------------------------------------------------------------------

   @brief Close SAL module.
        Free internal resources already allocated.

   @param v_PVOID_t pAdapter
        Global adapter handle

   @return General status code
        VOS_STATUS_SUCCESS       Close Success
        VOS_STATUS_E_FAILURE     Close Fail, BAL not open
        VOS_STATUS_E_INVAL       Invalid argument

----------------------------------------------------------------------------*/
VOS_STATUS WLANSAL_Close
(
   v_PVOID_t pAdapter
)
{
   SENTER();

   // Destroy the mutex
   mutex_destroy(&gpsalHandle->lock);

   vos_free_context(pAdapter, VOS_MODULE_ID_SAL, gpsalHandle);

   SEXIT();
   return VOS_STATUS_SUCCESS;
}


/*----------------------------------------------------------------------------

   @brief
      - TBD

   @param v_PVOID_t pAdapter
        Global adapter handle

   @return General status code

----------------------------------------------------------------------------*/
VOS_STATUS WLANSAL_Reset
(
   v_PVOID_t pAdapter
)
{
   SENTER();
   // TBD
   //
   SEXIT();
   return VOS_STATUS_SUCCESS;
}


/*=========================================================================
 * END Interactions with BAL
 *=========================================================================*/

/*=========================================================================
 * General Functions
 *=========================================================================*/
/*----------------------------------------------------------------------------

   @brief SDIO CMD52 Read or write one byte at a time

   @param v_PVOID_t pAdapter
        Global adapter handle

   @param  WLANSAL_Cmd52ReqType *cmd52Req
           WLANSAL_BUS_DIRECTION_TYPE BUS direction, Read or write direction
           v_U32_t                    Target address
           v_PVOID_t                  Data pointer
           v_PVOID_t                  SDIO internal handle
   @return General status code
        VOS_STATUS_SUCCESS       Read or write success
        VOS_STATUS_E_INVAL       cmd is not valid
        VOS_STATUS_E_FAILURE     SAL is not ready

----------------------------------------------------------------------------*/
VOS_STATUS WLANSAL_Cmd52
(
   v_PVOID_t             pAdapter,
   WLANSAL_Cmd52ReqType *cmd52Req
)
{
   int  err_ret = 0;
   int  save_function_num = 0;

   VOS_ASSERT(gpsalHandle);
   VOS_ASSERT(cmd52Req);

   SENTER();

   // Get the lock, going native
   sd_claim_host(gpsalHandle->sdio_func_dev);

   save_function_num = gpsalHandle->sdio_func_dev->num;
   gpsalHandle->sdio_func_dev->num = 0; // Assign to 0.
   libra_sdiocmd52( gpsalHandle->sdio_func_dev, cmd52Req->address, cmd52Req->dataPtr,
         cmd52Req->busDirection, &err_ret);
   gpsalHandle->sdio_func_dev->num = save_function_num;

   VOS_ASSERT(0 == err_ret);

   // Release lock
   sd_release_host(gpsalHandle->sdio_func_dev);

   SEXIT();

   return VOS_STATUS_SUCCESS;
}


/*----------------------------------------------------------------------------

   @brief SDIO CMD53 Read or write multiple bytes at a time.
        Read or write can be happen with synchronous functions call or
        asynchronous function call, depends on clients request.
   Currently all calls are SYNC.

   @param v_PVOID_t pAdapter
        Global adapter handle

   @param  WLANSAL_Cmd53ReqType *cmd53Req
           WLANSAL_BUS_DIRECTION_TYPE BUS direction, Read or write direction
           WLANSAL_BUS_MODE_TYPE      Byte or block mode
           WLANSAL_BUS_EAPI_TYPE      Sync or Async call
           WLANSAL_BusCompleteCBType  If bus request is done,
                                      this callback have to be issued with
                                      asynchronous function call
           v_U32_t                    Target address
           v_U16_t                    Read or write data size
           v_PVOID_t                  Data pointer
           v_PVOID_t                  SDIO internal handle

   @return General status code
        VOS_STATUS_SUCCESS       Read or write success
        VOS_STATUS_E_INVAL       cmd is not valid
        VOS_STATUS_E_FAILURE     SAL is not ready

----------------------------------------------------------------------------*/
VOS_STATUS WLANSAL_Cmd53
(
   v_PVOID_t             pAdapter,
   WLANSAL_Cmd53ReqType *cmd53Req
)
{
   v_U16_t              transferUnits;
   int                  err_ret=0;
   v_U32_t              numBlocks = 0;
   v_U32_t              numBytes = 0;
   v_U8_t               regValue;
   v_U8_t               *temp_dataPtr;           // Data pointer
   v_U32_t              temp_address;           // Target address

   VOS_ASSERT(gpsalHandle);
   VOS_ASSERT(cmd53Req);
   VOS_ASSERT(cmd53Req->dataSize);

   SENTER();

   if (0 == cmd53Req->dataSize)
   {
      SMSGERROR("%s: CMD53 Data size 0, direction %d, Mode %d, address 0x%x",
         cmd53Req->busDirection, cmd53Req->mode, cmd53Req->address);
      SEXIT();
      return VOS_STATUS_E_FAILURE;
   }

   // The host driver internally takes care of chopping it into blocks.
   transferUnits = (v_U16_t)cmd53Req->dataSize;

   // Get the lock, going native
   sd_claim_host(gpsalHandle->sdio_func_dev);

   if (WLANSAL_DIRECTION_READ == cmd53Req->busDirection)
   {
      if (WLANSAL_ADDRESS_FIXED == cmd53Req->addressHandle) {
         // Will only copy 1 block or less.
         err_ret = libra_sdio_readsb(gpsalHandle->sdio_func_dev,
                                  cmd53Req->dataPtr, cmd53Req->address, transferUnits);
      }
      else {
         err_ret = libra_sdio_memcpy_fromio(gpsalHandle->sdio_func_dev,
                                  cmd53Req->dataPtr, cmd53Req->address, transferUnits);
      }
   }
   else if(WLANSAL_DIRECTION_WRITE == cmd53Req->busDirection)
   {
      // For write we need to make sure that a Dummy CMD52 is sent for every CMD53.
      // So lets do some of the Bus Drivers work here.


      numBlocks = transferUnits / (gpsalHandle->sdio_func_dev->cur_blksize);
      numBytes  = transferUnits % (gpsalHandle->sdio_func_dev->cur_blksize);

      if (numBlocks != 0) {
         if (WLANSAL_ADDRESS_FIXED == cmd53Req->addressHandle) {
            err_ret = libra_sdio_writesb(gpsalHandle->sdio_func_dev,
               cmd53Req->address, cmd53Req->dataPtr, numBlocks*gpsalHandle->sdio_func_dev->cur_blksize);
         }
         else {
            err_ret = libra_sdio_memcpy_toio(gpsalHandle->sdio_func_dev, cmd53Req->address,
               cmd53Req->dataPtr, numBlocks*gpsalHandle->sdio_func_dev->cur_blksize);
         }
         if (err_ret)
         {
            SMSGERROR("%s: Value of ERROR err_ret #1 = %d\n", __func__, err_ret, 0);
         }

         // Send Dummy CMD52 here, this is to scan for PROG_ENABLE
         gpsalHandle->sdio_func_dev->num = 0;
         libra_sdiocmd52( gpsalHandle->sdio_func_dev, 0x0, &regValue, 0, &err_ret);
         gpsalHandle->sdio_func_dev->num = 1;
      }

      if (numBytes != 0) {

         temp_address            = cmd53Req->address + (gpsalHandle->sdio_func_dev->cur_blksize * numBlocks);
         temp_dataPtr            = (v_U8_t *)(cmd53Req->dataPtr + ( gpsalHandle->sdio_func_dev->cur_blksize * numBlocks));

         if (WLANSAL_ADDRESS_FIXED == cmd53Req->addressHandle) {
            err_ret = libra_sdio_writesb(gpsalHandle->sdio_func_dev,
               temp_address, temp_dataPtr, numBytes);
         }
         else {
            err_ret = libra_sdio_memcpy_toio(gpsalHandle->sdio_func_dev, temp_address,
               temp_dataPtr, numBytes);
         }
         if (err_ret)
         {
            SMSGERROR("%s: Value of ERROR err_ret #1 = %d\n", __func__, err_ret, 0);
         }

         // Send Dummy CMD52 here, this is to scan for PROG_ENABLE
         gpsalHandle->sdio_func_dev->num = 0;
         libra_sdiocmd52( gpsalHandle->sdio_func_dev, 0x0, &regValue, 0, &err_ret);
         gpsalHandle->sdio_func_dev->num = 1;
      }
   }

  // Release lock
   sd_release_host(gpsalHandle->sdio_func_dev);

   if (err_ret)
   {
      SMSGERROR("%s: Value of ERROR err_ret = %d\n", __func__, err_ret, 0);
      SEXIT();
      vos_chipReset(NULL, VOS_FALSE, NULL, NULL);
      return err_ret;
   }

   SEXIT();
   return VOS_STATUS_SUCCESS;
}


/*=========================================================================
 * END General Functions
 *=========================================================================*/
/*=========================================================================
 * Interactions with SSC
 *=========================================================================*/

/*----------------------------------------------------------------------------

   @brief Register SSC callback functions to SAL.
        Just after SAL open DONE, callback functions have to be registered.
        Registration functions are TX complete, RX complete and interrupt happen.
        Fatal error callback function is TBD yet.

   @param v_PVOID_t pAdapter
        Global adapter handle

   @param WLANSAL_SscRegType
           WLANSAL_InterruptCBType      interrupt CB function PTR
           WLANSAL_BusCompleteCBType    Bus complete CB function PTR
           v_PVOID_t                    SSC handle
           v_PVOID_t                    SAL handle

   @return General status code
        VOS_STATUS_SUCCESS       Registration success
        VOS_STATUS_E_RESOURCES   SAL resources are not ready
        VOS_STATUS_E_INVAL       Invalid argument

----------------------------------------------------------------------------*/
VOS_STATUS WLANSAL_RegSscCBFunctions
(
   v_PVOID_t           pAdapter,
   WLANSAL_SscRegType *sscReg
)
{

   VOS_ASSERT(sscReg);
   VOS_ASSERT(gpsalHandle);

   SENTER();

   gpsalHandle->sscCBs.interruptCB   = sscReg->interruptCB;
   gpsalHandle->sscCBs.busCompleteCB = sscReg->busCompleteCB;
   gpsalHandle->sscCBs.sscUsrData    = sscReg->sscUsrData;

   SEXIT();
   return VOS_STATUS_SUCCESS;
}


/*----------------------------------------------------------------------------

   @brief De Register SSC callback functions from SAL.

   @param v_PVOID_t pAdapter
        Global adapter handle

   @return General status code
        VOS_STATUS_SUCCESS       Registration success
        VOS_STATUS_E_RESOURCES   SAL resources are not ready
        VOS_STATUS_E_INVAL       Invalid argument

----------------------------------------------------------------------------*/
VOS_STATUS WLANSAL_DeregSscCBFunctions
(
   v_PVOID_t           pAdapter
)
{

   VOS_ASSERT(gpsalHandle);
   SENTER();

   gpsalHandle->sscCBs.interruptCB   = NULL;
   gpsalHandle->sscCBs.busCompleteCB = NULL;

   SEXIT();
   return VOS_STATUS_SUCCESS;
}


/*----------------------------------------------------------------------------

   @brief Query card information.
        Card information will be got during WLANSAL_Start.
        Card information is stored SAL internal structure,

   @param v_PVOID_t pAdapter
        Global adapter handle

   @param WLANSAL_CardInfoType *cardInfo
           WLANSAL_CARD_INTERFACE_TYPE  1bit or 4 bit interface
           v_U32_t                      Card clock rate
           v_U32_t                      Card block size
           v_PVOID_t                    SAL handle

   @return General status code
        VOS_STATUS_SUCCESS       Query success
        VOS_STATUS_E_RESOURCES   SAL resources are not ready
        VOS_STATUS_E_INVAL       Invalid argument

----------------------------------------------------------------------------*/
VOS_STATUS WLANSAL_CardInfoQuery
(
   v_PVOID_t             pAdapter,
   WLANSAL_CardInfoType *cardInfo
)
{

   VOS_ASSERT(gpsalHandle);
   VOS_ASSERT(cardInfo);
   SENTER();

   memcpy(cardInfo, &gpsalHandle->cardInfo, sizeof(WLANSAL_CardInfoType));

   SMSGINFO("%s: Query CR %dKHz, BS %d", __func__, cardInfo->clockRate, cardInfo->blockSize);

   SEXIT();
   return VOS_STATUS_SUCCESS;
}


/*----------------------------------------------------------------------------

   @brief Update card information.

   @param v_PVOID_t pAdapter
        Global adapter handle

   @param WLANSAL_CardInfoType *cardInfo
           WLANSAL_CARD_INTERFACE_TYPE  1bit or 4 bit interface
           v_U32_t                      Card clock rate
           v_U32_t                      Card block size
           v_PVOID_t                    SAL handle

   @return General status code
        VOS_STATUS_SUCCESS       Update success
        VOS_STATUS_E_RESOURCES   SAL resources are not ready
        VOS_STATUS_E_INVAL       Invalid argument

----------------------------------------------------------------------------*/
VOS_STATUS WLANSAL_CardInfoUpdate
(
   v_PVOID_t             pAdapter,
   WLANSAL_CardInfoType *cardInfo
)
{
   VOS_ASSERT(gpsalHandle);
   VOS_ASSERT(cardInfo);

   SENTER();

   // Get the lock, going native
   SGETLOCK(__func__, &gpsalHandle->lock);

   if(gpsalHandle->cardInfo.clockRate != cardInfo->clockRate)
   {
      gpsalHandle->cardInfo.clockRate = cardInfo->clockRate;
   }

   if(gpsalHandle->cardInfo.blockSize != cardInfo->blockSize)
   {
      gpsalHandle->cardInfo.blockSize = cardInfo->blockSize;
   }

   // Release lock
   SRELEASELOCK(__func__, &gpsalHandle->lock);

   SEXIT();
   return VOS_STATUS_SUCCESS;
}

/*=========================================================================
 * END Interactions with SSC
 *=========================================================================*/

/*----------------------------------------------------------------------------

   @brief Set Card presence status path
          Card present status changed notification path
          If Card is removed from slot or put into slot
          Notification may routed to SAL or SDBUS

   @param v_PVOID_t pAdapter
        Global adapter handle

   @param WLANSAL_NOTF_PATH_T   path
        Notification Path, it may SAL or SDBUS

   @return General status code
        VOS_STATUS_SUCCESS       Update success
        VOS_STATUS_E_RESOURCES   SAL resources are not ready
        VOS_STATUS_E_INVAL       Invalid argument

----------------------------------------------------------------------------*/
VOS_STATUS WLANSAL_SetCardStatusNotfPath
(
   v_PVOID_t             pAdapter,
   WLANSAL_NOTF_PATH_T   path
)
{
   return VOS_STATUS_SUCCESS;
}

#ifdef VOLANS_1_0_WORKAROUND
/*----------------------------------------------------------------------------

   @brief API exported from SAL to set the SD clock frequency. This needs to
          have the libra_sdio_set_clock API exported from librasdioif driver and
          mmc_set_clock API exported from the kernel
   @param hz - Frequency to be set

   @return void

----------------------------------------------------------------------------*/
void WLANSAL_SetSDIOClock(unsigned int hz)
{
  VOS_ASSERT(NULL != gpsalHandle);
  libra_sdio_set_clock(gpsalHandle->sdio_func_dev, hz);
}

#endif /* VOLANS_1_0_WORKAROUND */

/*----------------------------------------------------------------------------

   @brief API exported from SAL to get the vendor specific card ID. This needs to
          have the libra_sdio_get_card_id API exported from librasdioif driver
   @param *card_id - To receive the card id

   @return void

----------------------------------------------------------------------------*/
void WLANSAL_GetSDIOCardId(unsigned short *sdioCardId)
{
  VOS_ASSERT(NULL != gpsalHandle);
  libra_sdio_get_card_id(gpsalHandle->sdio_func_dev, sdioCardId);
}

/*----------------------------------------------------------------------------

   @brief Reinitialize LIBRA's SDIO core
          Deep sleep status is same with turn off power
          So, standard SDIO init procedure is needed

   @param v_PVOID_t pAdapter
        Global adapter handle

   @return General status code
        VOS_STATUS_SUCCESS       Update success
        VOS_STATUS_E_RESOURCES   SAL resources are not ready
        VOS_STATUS_E_INVAL       Invalid argument

----------------------------------------------------------------------------*/
#ifndef MSM_PLATFORM
VOS_STATUS WLANSAL_SDIOReInit
(
   v_PVOID_t             pAdapter
)
{
   hdd_adapter_t *pHddAdapter = NULL;
   v_CONTEXT_t pVosContext = NULL;
   struct sdio_func *sdio_func_new;
   struct sdio_func *sdio_func_old;
   unsigned int attempts = 0;

   //Get the global vos context
   pVosContext = vos_get_global_context(VOS_MODULE_ID_SYS, NULL);

   if(!pVosContext)
   {
      SMSGERROR("Global VOS context is Null", 0, 0, 0);
      return VOS_STATUS_E_FAILURE;
   }

   //Get the HDD context.
   pHddAdapter = (hdd_adapter_t *)vos_get_context(VOS_MODULE_ID_HDD, pVosContext );
   if(!pHddAdapter)
   {
      SMSGERROR("Hdd Adapter context is Null", 0, 0, 0);
      return VOS_STATUS_E_FAILURE;;
   }

   //Cache the old device handle first
	sdio_func_old = libra_getsdio_funcdev();

#ifdef SDIO_POLLING_KERNEL_CHANGE_MAINLINED
   if(libra_sdio_enable_polling()) {
      SMSGERROR("libra_sdio_enable_polling failed",0, 0, 0);
      return VOS_STATUS_E_FAILURE;
   }
#endif

   do {
      msleep(1000);
      sdio_func_new = libra_getsdio_funcdev();
      if (NULL == sdio_func_new || sdio_func_old == sdio_func_new) {
         SMSGERROR("Libra WLAN not detected yet.",0, 0, 0);
         attempts++;
      }
      else {
         SMSGERROR("Libra WLAN detecton succeeded",0, 0, 0);
         break;
      }
   }while (attempts < 3);

   if (NULL == sdio_func_new || sdio_func_old == sdio_func_new) {
      SMSGERROR("Libra WLAN not detected after multiple attempts",0, 0, 0);
      return VOS_STATUS_E_FAILURE;
   }

   /* set net_device parent to sdio device */
   SET_NETDEV_DEV(pHddAdapter->dev, &sdio_func_new->dev);

   // Set the private data for the device to our adapter.
   libra_sdio_setprivdata (sdio_func_new, pHddAdapter);
   atomic_set(&pHddAdapter->sdio_claim_count, 0);
   pHddAdapter->hsdio_func_dev = sdio_func_new;

   return VOS_STATUS_SUCCESS;
}
#else /* MSM_PLATFORM */
VOS_STATUS WLANSAL_SDIOReInit
(
   v_PVOID_t             pAdapter
)
{
   hdd_adapter_t *pHddAdapter = NULL;
   v_CONTEXT_t pVosContext = NULL;
   struct sdio_func *func;
#ifndef VOLANS_LINUX_PC
   int err = 0;
#endif

   //Get the global vos context
   pVosContext = vos_get_global_context(VOS_MODULE_ID_SYS, NULL);

   if(!pVosContext)
   {
      SMSGERROR("Global VOS context is Null", 0, 0, 0);
      return VOS_STATUS_E_FAILURE;
   }

   //Get the HDD context.
   pHddAdapter = (hdd_adapter_t *)vos_get_context(VOS_MODULE_ID_HDD, pVosContext );
   if(!pHddAdapter)
   {
      SMSGERROR("Hdd Adapter context is Null", 0, 0, 0);
      return VOS_STATUS_E_FAILURE;;
   }

   func = libra_getsdio_funcdev();
   if (func && func->card) {
      err = sdio_reset_comm(func->card);
#ifndef VOLANS_LINUX_PC
      if(err) {
         SMSGERROR("%s: sdio_reset_comm failed %d", __func__, err, 0);
         return VOS_STATUS_E_FAILURE;
      }
#endif
   }
   else
   {
      SMSGERROR("%s: sdio_func or mmc_card handle is null", __func__, 0, 0);
      return VOS_STATUS_E_FAILURE;
   }

   atomic_set(&pHddAdapter->sdio_claim_count, 0);

   return VOS_STATUS_SUCCESS;
}
#endif /* MSM_PLATFORM */
