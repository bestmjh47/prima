/**========================================================================
  
  \file  wlan_hdd_main.c

  \brief WLAN Host Device Driver implementation
               
   Copyright 2008 (c) Qualcomm, Incorporated.  All Rights Reserved.
   
   Qualcomm Confidential and Proprietary.
  
  ========================================================================*/

/**========================================================================= 

                       EDIT HISTORY FOR FILE 
   
   
  This section contains comments describing changes made to the module. 
  Notice that changes are listed in reverse chronological order. 
   
   
  $Header:$   $DateTime: $ $Author: $ 
   
   
  when        who    what, where, why 
  --------    ---    --------------------------------------------------------
  04/5/09     Shailender     Created module. 

  ==========================================================================*/

/*--------------------------------------------------------------------------
  Include Files
  ------------------------------------------------------------------------*/
  
//#include <wlan_qct_driver.h>
#include <wlan_hdd_includes.h>
#include <wlan_sal_misc.h>
#include <vos_api.h>
#include <vos_sched.h>
#include <vos_power.h>
#include <linux/etherdevice.h>
#include <linux/firmware.h>
#include <linux/mmc/sdio_func.h>
#include <wlan_hdd_tx_rx.h>
#include <palTimer.h>
#include <wniApi.h>
#include <wlan_nlink_srv.h>
#include <wlan_btc_svc.h>
#include <wlan_hdd_cfg.h>
#include <wlan_ptt_sock_svc.h>
#include <wlan_hdd_wowl.h>
#include <wlan_hdd_misc.h>

#ifdef ANI_MANF_DIAG
int wlan_hdd_ftm_start(hdd_adapter_t *pAdapter);
#endif


/*--------------------------------------------------------------------------- 
 *   Function definitions
 *-------------------------------------------------------------------------*/
#ifdef CONFIG_HAS_EARLYSUSPEND
extern void register_wlan_suspend(void);
extern void unregister_wlan_suspend(void);
#endif

int hdd_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
   return 0;
}

/**---------------------------------------------------------------------------
  
  \brief hdd_open() - HDD Open function
  
  This is called in response to ifconfig up
  
  \param  - dev Pointer to net_device structure
  
  \return - 0 for success non-zero for failure
              
  --------------------------------------------------------------------------*/
int hdd_open (struct net_device *dev)
{
   hdd_adapter_t* pAdapter = netdev_priv(dev);

   if(pAdapter == NULL) {
      VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
         "%s: HDD adapter context is Null", __FUNCTION__);
      return -1;
   }

   if(!hdd_connIsConnected(pAdapter)) {
      VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR, 
                 "%s: STA not associated, leaving Tx queues disabled" ,
                 __FUNCTION__);
   }
   else
   {   
      VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR, 
                 "%s: Enabling Tx Queues" , __FUNCTION__);

      netif_carrier_on(dev);
      netif_start_queue(dev);
   }

   return 0;
}

/**---------------------------------------------------------------------------
  
  \brief hdd_stop() - HDD stop function
  
  This is called in response to ifconfig down
  
  \param  - dev Pointer to net_device structure
  
  \return - 0 for success non-zero for failure
              
  --------------------------------------------------------------------------*/

int hdd_stop (struct net_device *dev)
{
   //Stop the Interface TX queue. netif_stop_queue should not be used when
   //transmission is being disabled anywhere other than hard_start_xmit
   hddLog(VOS_TRACE_LEVEL_ERROR,"%s: Disabling OS Tx queues",__func__);
   netif_tx_disable(dev);
   netif_carrier_off(dev);

   return 0;
}

/**---------------------------------------------------------------------------
  
  \brief hdd_release_firmware() - 

   This function calls the release firmware API to free the firmware buffer.
   
  \param  - pFileName Pointer to the File Name. 
                  pCtx - Pointer to the adapter .
              
              
  \return - 0 for success, non zero for failure
     
  --------------------------------------------------------------------------*/

VOS_STATUS hdd_release_firmware(char *pFileName,v_VOID_t *pCtx)
{
   VOS_STATUS status = VOS_STATUS_SUCCESS;
   hdd_adapter_t *pAdapter = (hdd_adapter_t*)pCtx;
   ENTER();
   
   
   if( !strcmp(LIBRA_FW_FILE,pFileName)) {
   
       if(pAdapter->fw) {
          release_firmware(pAdapter->fw);
          pAdapter->fw = NULL;
       }
       else
          status = VOS_STATUS_E_FAILURE;
   }
   else if (!strcmp(LIBRA_NV_FILE,pFileName)) {
       if(pAdapter->nv) {
          release_firmware(pAdapter->nv);
          pAdapter->nv = NULL;
       }
       else
          status = VOS_STATUS_E_FAILURE;
        
   }

   EXIT();
   return status;  
} 

/**---------------------------------------------------------------------------
  
  \brief hdd_request_firmware() - 

   This function reads the firmware file using the request firmware 
   API and returns the the firmware data and the firmware file size.
   
  \param  - pfileName - Pointer to the file name. 
              - pCtx - Pointer to the adapter .
              - ppfw_data - Pointer to the pointer of the firmware data.
              - pSize - Pointer to the file size.
              
  \return - 0 for success, non zero for failure
     
  --------------------------------------------------------------------------*/


VOS_STATUS hdd_request_firmware(char *pfileName,v_VOID_t *pCtx,v_VOID_t **ppfw_data, v_SIZE_t *pSize)
{
   int status;
   hdd_adapter_t *pAdapter = (hdd_adapter_t*)pCtx;
   ENTER();

   if( !strcmp(LIBRA_FW_FILE,pfileName)) {
   
       status = request_firmware(&pAdapter->fw, pfileName, &pAdapter->hsdio_func_dev->dev);
   
       if(status || !pAdapter->fw || !pAdapter->fw->data) {
           hddLog(VOS_TRACE_LEVEL_ERROR,"%s: Firmware download failed",__func__);
           status = VOS_STATUS_E_FAILURE;
       } 
   
       else {
         *ppfw_data = (v_VOID_t *)pAdapter->fw->data;  
         *pSize = pAdapter->fw->size; 
          hddLog(VOS_TRACE_LEVEL_ERROR,"%s: Firmware size = %d",__func__, *pSize);
       }
   }
   else if(!strcmp(LIBRA_NV_FILE,pfileName)) {
       
       status = request_firmware(&pAdapter->nv, pfileName, &pAdapter->hsdio_func_dev->dev);
   
       if(status || !pAdapter->nv || !pAdapter->nv->data) {
           hddLog(VOS_TRACE_LEVEL_ERROR,"%s: nv download failed",__func__);
           status = VOS_STATUS_E_FAILURE;
       } 
   
       else {
         *ppfw_data = (v_VOID_t *)pAdapter->nv->data;  
         *pSize = pAdapter->nv->size; 
          hddLog(VOS_TRACE_LEVEL_ERROR,"%s: nv file size = %d",__func__, *pSize);
       }
   }

   EXIT();
   return VOS_STATUS_SUCCESS;  
} 

/**---------------------------------------------------------------------------
  
  \brief hdd_get_cfg_file_size() - 

   This function reads the configuration file using the request firmware 
   API and returns the configuration file size.
   
  \param  - pCtx - Pointer to the adapter .
              - pFileName - Pointer to the file name.
              - pBufSize - Pointer to the buffer size.
              
  \return - 0 for success, non zero for failure
     
  --------------------------------------------------------------------------*/

VOS_STATUS hdd_get_cfg_file_size(v_VOID_t *pCtx, char *pFileName, v_SIZE_t *pBufSize)
{
   int status; 
   hdd_adapter_t *pAdapter = (hdd_adapter_t*)pCtx;

   ENTER();
   
   status = request_firmware(&pAdapter->fw, pFileName, &pAdapter->hsdio_func_dev->dev);
   
   if(status || !pAdapter->fw || !pAdapter->fw->data) {
      hddLog(VOS_TRACE_LEVEL_ERROR,"%s: CFG download failed",__func__);
      status = VOS_STATUS_E_FAILURE;
   } 
   else {
      *pBufSize = pAdapter->fw->size;
      hddLog(VOS_TRACE_LEVEL_ERROR,"%s: CFG size = %d",__func__, *pBufSize);
      release_firmware(pAdapter->fw);
      pAdapter->fw = NULL;
   }
   
   EXIT();
   return VOS_STATUS_SUCCESS;  
} 

/**---------------------------------------------------------------------------
  
  \brief hdd_read_cfg_file() - 

   This function reads the configuration file using the request firmware 
   API and returns the cfg data and the buffer size of the configuration file.
   
  \param  - pCtx - Pointer to the adapter .
              - pFileName - Pointer to the file name.
              - pBuffer - Pointer to the data buffer.
              - pBufSize - Pointer to the buffer size.
              
  \return - 0 for success, non zero for failure
     
  --------------------------------------------------------------------------*/

VOS_STATUS hdd_read_cfg_file(v_VOID_t *pCtx, char *pFileName, 
    v_VOID_t *pBuffer, v_SIZE_t *pBufSize)
{
   int status;
   hdd_adapter_t *pAdapter = (hdd_adapter_t*)pCtx;

   ENTER();
   
   status = request_firmware(&pAdapter->fw, pFileName, &pAdapter->hsdio_func_dev->dev);
   
   if(status || !pAdapter->fw || !pAdapter->fw->data) {
      hddLog(VOS_TRACE_LEVEL_ERROR,"%s: CFG download failed",__func__);
      return VOS_STATUS_E_FAILURE;
   } 
   else {
      if(*pBufSize != pAdapter->fw->size) {
         hddLog(VOS_TRACE_LEVEL_ERROR, "%s: Caller sets invalid CFG "
             "file size", __func__);
         release_firmware(pAdapter->fw);
         pAdapter->fw = NULL;
         return VOS_STATUS_E_FAILURE;
      } 
	    else {
         if(pBuffer) {
            vos_mem_copy(pBuffer,pAdapter->fw->data,*pBufSize);
         }
         release_firmware(pAdapter->fw);
         pAdapter->fw = NULL;
	    }
   }
   
   EXIT();
   
   return VOS_STATUS_SUCCESS;
} 

/**---------------------------------------------------------------------------

  \brief hdd_set_mac_addr_cb() - 

   This function is the call back function for setting the station  
   mac adrress called by ccm module to indicate the 
   success/failure result.
   
  \param  - hHal - Pointer to the hal module.
              - result - returns the result of the set mac address.
              
  \return - void
  
  --------------------------------------------------------------------------*/

static void hdd_set_mac_addr_cb( tHalHandle hHal, tANI_S32 result )
{
  // ignore the STA_ID response for now.
  
  VOS_ASSERT( CCM_IS_RESULT_SUCCESS( result ) );
}



/**---------------------------------------------------------------------------
  
  \brief hdd_set_mac_address() - 

   This function sets the user specified mac address using 
   the command ifconfig wlanX hw ether <mac adress>.
   
  \param  - dev - Pointer to the net device.
              - addr - Pointer to the sockaddr.
  \return - 0 for success, non zero for failure
  
  --------------------------------------------------------------------------*/

static int hdd_set_mac_address(struct net_device *dev, void *addr)
{
   hdd_adapter_t *pAdapter = netdev_priv(dev);
   struct sockaddr *psta_mac_addr = addr;
   eHalStatus halStatus = eHAL_STATUS_SUCCESS;

   ENTER();
   
   memcpy(&pAdapter->macAddressCurrent, psta_mac_addr->sa_data, ETH_ALEN);

   // set the MAC address though the STA ID CFG.
   halStatus = ccmCfgSetStr( pAdapter->hHal, WNI_CFG_STA_ID, 
                             (v_U8_t *)&pAdapter->macAddressCurrent,
                             sizeof( pAdapter->macAddressCurrent ),
                             hdd_set_mac_addr_cb, VOS_FALSE );
	
   memcpy(dev->dev_addr, psta_mac_addr->sa_data, ETH_ALEN);

   EXIT();
   return halStatus;
}

/**---------------------------------------------------------------------------
  
  \brief hdd_wlan_initial_scan() - 

   This function triggers the initial scan
   
  \param  - pAdapter - Pointer to the HDD adapter.
  
  --------------------------------------------------------------------------*/
void hdd_wlan_initial_scan(hdd_adapter_t *pAdapter)
{
   tCsrScanRequest scanReq;
   tCsrChannelInfo channelInfo;
   eHalStatus halStatus;
   unsigned long scanId;

   vos_mem_zero(&scanReq, sizeof(tCsrScanRequest));
   vos_mem_set(&scanReq.bssid, sizeof(tCsrBssid), 0xff);
   scanReq.BSSType = eCSR_BSS_TYPE_ANY;

   if(sme_Is11dSupported(pAdapter->hHal))
   {
      halStatus = sme_ScanGetBaseChannels( pAdapter->hHal, &channelInfo );
      if ( HAL_STATUS_SUCCESS( halStatus ) )
      {
         scanReq.ChannelInfo.ChannelList = vos_mem_malloc(channelInfo.numOfChannels);
         if( !scanReq.ChannelInfo.ChannelList )
         {
            hddLog(VOS_TRACE_LEVEL_ERROR, "%s kmalloc failed", __func__);
            return;
         }
         vos_mem_copy(scanReq.ChannelInfo.ChannelList, channelInfo.ChannelList, 
            channelInfo.numOfChannels);
         scanReq.ChannelInfo.numOfChannels = channelInfo.numOfChannels;
         vos_mem_free(channelInfo.ChannelList);
      }

      scanReq.scanType = eSIR_PASSIVE_SCAN;
      scanReq.requestType = eCSR_SCAN_REQUEST_11D_SCAN;
      scanReq.maxChnTime = pAdapter->cfg_ini->nPassiveMaxChnTime;
      scanReq.minChnTime = pAdapter->cfg_ini->nPassiveMinChnTime;
   }
   else
   {
      scanReq.scanType = eSIR_ACTIVE_SCAN;
      scanReq.requestType = eCSR_SCAN_REQUEST_FULL_SCAN;
      scanReq.maxChnTime = pAdapter->cfg_ini->nActiveMaxChnTime;
      scanReq.minChnTime = pAdapter->cfg_ini->nActiveMinChnTime;
   }

   halStatus = sme_ScanRequest(pAdapter->hHal, &scanReq, &scanId, NULL, NULL);
   if ( !HAL_STATUS_SUCCESS( halStatus ) )
   {
      hddLog(VOS_TRACE_LEVEL_ERROR, "%s: sme_ScanRequest failed status code %d",
         __func__, halStatus );
   }
   
   if(sme_Is11dSupported(pAdapter->hHal))
        vos_mem_free(scanReq.ChannelInfo.ChannelList);
}

/**---------------------------------------------------------------------------
  
  \brief hdd_wlan_sdio_emove() - HDD remove function register with SAL
  
  This is the driver exit point (invoked by SAL during SDIO remove)
  
  \param  - sdio_func_dev - Pointer to the SDIO function device
  
  \return - None
              
  --------------------------------------------------------------------------*/
void hdd_wlan_sdio_remove(struct sdio_func *sdio_func_dev)
{
   hddLog(VOS_TRACE_LEVEL_ERROR,"%s: Libra unregistered "
       "from SDIO bus driver", __func__);
}

/**---------------------------------------------------------------------------
  
  \brief hdd_full_pwr_cbk() - HDD full power callbackfunction

  This is the function invoked by SME to inform the result of a full power
  request issued by HDD
  
  \param  - callbackcontext - Pointer to cookie 
  \param  - status - result of request
  
  \return - None
              
  --------------------------------------------------------------------------*/
void hdd_full_pwr_cbk(void *callbackContext, eHalStatus status)
{
   hdd_adapter_t *pAdapter = (hdd_adapter_t*)callbackContext;

   hddLog(VOS_TRACE_LEVEL_ERROR,"HDD full Power callback status = %d", status);

   complete(&pAdapter->full_pwr_comp_var);
}

/**---------------------------------------------------------------------------
  
  \brief hdd_wlan_exit() - HDD WLAN exit function
  
  This is the driver exit point (invoked during rmmod)
  
  \param  - sdio_func_dev - Pointer to the SDIO function device
  
  \return - None
              
  --------------------------------------------------------------------------*/
void hdd_wlan_exit(hdd_adapter_t *pAdapter)
{
   eHalStatus halStatus;
   v_CONTEXT_t pVosContext = pAdapter->pvosContext;
   struct net_device *pWlanDev = pAdapter->dev;
   VOS_STATUS vosStatus;
  
   //Stop the Interface TX queue.
   netif_tx_disable(pWlanDev);
   netif_carrier_off(pWlanDev);

#ifdef CONFIG_HAS_EARLYSUSPEND
   // unregister suspend/resume callbacks
   if(pAdapter->cfg_ini->nEnableSuspend)
      unregister_wlan_suspend();
#endif

   //Disable IMPS/BMPS as we do not want the device to enter any power
   //save mode during shutdown
   sme_DisablePowerSave(pAdapter->hHal, ePMC_IDLE_MODE_POWER_SAVE);
   sme_DisablePowerSave(pAdapter->hHal, ePMC_BEACON_MODE_POWER_SAVE);

   //Ensure that device is in full power as we will touch H/W during vos_Stop
   init_completion(&pAdapter->full_pwr_comp_var);
   halStatus = sme_RequestFullPower(pAdapter->hHal, hdd_full_pwr_cbk, 
       pAdapter, eSME_FULL_PWR_NEEDED_BY_HDD);

   if(halStatus != eHAL_STATUS_SUCCESS)
   {
     if(halStatus == eHAL_STATUS_PMC_PENDING)
     {
        //Block on a completion variable. Can't wait forever though
        wait_for_completion_interruptible_timeout(&pAdapter->full_pwr_comp_var, 
            msecs_to_jiffies(1000));
     }
     else
     {
        hddLog(VOS_TRACE_LEVEL_ERROR,"%s: Request for Full Power failed\n", __func__);
        VOS_ASSERT(0);
        return;
     }
   }

   init_completion(&pAdapter->disconnect_comp_var);
   halStatus = sme_RoamDisconnect(pAdapter->hHal, eCSR_DISCONNECT_REASON_UNSPECIFIED);

   //success implies disconnect command got queued up successfully
   if(halStatus == eHAL_STATUS_SUCCESS)
   {
       wait_for_completion_interruptible_timeout(&pAdapter->disconnect_comp_var, 
       msecs_to_jiffies(WLAN_WAIT_TIME_DISCONNECT));
   }
  
   //Stop all the modules
   vosStatus = vos_stop( pVosContext );
   if (!VOS_IS_STATUS_SUCCESS(vosStatus))
   {
      VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
         "%s: Failed to stop VOSS",__func__);
      VOS_ASSERT( VOS_IS_STATUS_SUCCESS( vosStatus ) );
   }

    msleep(50); 

   //Put the chip is standby before asserting deep sleep
   vosStatus = WLANBAL_SuspendChip( pAdapter->pvosContext );
   if (!VOS_IS_STATUS_SUCCESS(vosStatus))
   {
      VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
         "%s: Failed to suspend chip ",__func__);
      VOS_ASSERT( VOS_IS_STATUS_SUCCESS( vosStatus ) );
   }

   //Invoke SAL stop
   vosStatus = WLANSAL_Stop( pVosContext );
   if (!VOS_IS_STATUS_SUCCESS(vosStatus))
   {
      VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
         "%s: Failed to stop SAL",__func__);
      VOS_ASSERT( VOS_IS_STATUS_SUCCESS( vosStatus ) );
   }

   //Assert Deep sleep signal now to put Libra HW in lowest power state
   vosStatus = vos_chipAssertDeepSleep( NULL, NULL, NULL );
   VOS_ASSERT( VOS_IS_STATUS_SUCCESS( vosStatus ) );

   //Vote off any PMIC voltage supplies
   vos_chipPowerDown(NULL, NULL, NULL);

   //Clean up HDD Nlink Service
   send_btc_nlink_msg(WLAN_MODULE_DOWN_IND, 0); 
   nl_srv_exit();

   //clean up HDD Data Path
   hdd_deinit_tx_rx(pAdapter);
   hdd_wmm_close(pAdapter);

   //Free up dynamically allocated members inside HDD Adapter
   kfree(pAdapter->cfg_ini);
   pAdapter->cfg_ini= NULL;

   //Deregister the device with the kernel
   hdd_UnregisterWext(pWlanDev);
   if(test_bit(NET_DEVICE_REGISTERED, &pAdapter->event_flags)) 
   {
      unregister_netdev(pWlanDev);
      clear_bit(NET_DEVICE_REGISTERED, &pAdapter->event_flags);
   }

   //Close VOSS
   vos_close(pVosContext);

   //Free the net device
   free_netdev(pWlanDev);
}

/**---------------------------------------------------------------------------
  
  \brief hdd_post_voss_start_config() - HDD post voss start config helper
  
  \param  - pAdapter - Pointer to the HDD
  
  \return - None
              
  --------------------------------------------------------------------------*/
VOS_STATUS hdd_post_voss_start_config(hdd_adapter_t* pAdapter)
{
   struct net_device *pWlanDev = pAdapter->dev;
   eHalStatus halStatus;

   //Apply the cfg.ini to cfg.dat
   if ( hdd_update_config_dat(pAdapter) == FALSE)
   {   
      hddLog(VOS_TRACE_LEVEL_ERROR,"%s: config update failed",__func__ );
      return VOS_STATUS_E_FAILURE;
   }   
         
   // Register the roaming callback with SME...
   halStatus = sme_RoamRegisterCallback( 
       pAdapter->hHal, hdd_smeRoamCallback, pAdapter );

   if ( !HAL_STATUS_SUCCESS( halStatus ) )
   {
      hddLog(VOS_TRACE_LEVEL_ERROR,"%s: sme_RoamRegisterCallback() failed "
          "with status code %08d [x%08x]",__func__, halStatus, halStatus );
      return VOS_STATUS_E_FAILURE;
   } 

   //Update the new mac address based on qcom_cfg.ini
   vos_mem_copy(pWlanDev->dev_addr, 
                &pAdapter->cfg_ini->staMacAddr, 
                sizeof(v_MACADDR_t));

   vos_mem_copy(&pAdapter->macAddressCurrent, 
                &pAdapter->cfg_ini->staMacAddr,
                sizeof(v_MACADDR_t));
	  
   // Set the MAC Address 
   halStatus = ccmCfgSetStr( pAdapter->hHal, WNI_CFG_STA_ID, 
                             (v_U8_t *)&pAdapter->macAddressCurrent,
                             sizeof( pAdapter->macAddressCurrent ),
                             hdd_set_mac_addr_cb, VOS_FALSE );
   
   if (!HAL_STATUS_SUCCESS( halStatus ))
   {         
      hddLog(VOS_TRACE_LEVEL_ERROR,"%s: Failed to set MAC Address. "
          "HALStatus is %08d [x%08x]",__func__, halStatus, halStatus );
      return VOS_STATUS_E_FAILURE;
   }
     
   // Send ready indication to the HDD.  This will kick off the MAC
   // into a 'running' state and should kick off an initial scan.
   halStatus = sme_HDDReadyInd( pAdapter->hHal );
   if ( !HAL_STATUS_SUCCESS( halStatus ) )
   {
      hddLog(VOS_TRACE_LEVEL_ERROR,"%S: sme_HDDReadyInd() failed with status "
          "code %08d [x%08x]",__func__, halStatus, halStatus );
      return VOS_STATUS_E_FAILURE;
   }

   return VOS_STATUS_SUCCESS;
}

/**---------------------------------------------------------------------------
  
  \brief hdd_wlan_sdio_probe() - HDD init function
  
  This is the driver init point (invoked by SAL during SDIO probe)
  
  \param  - sdio_func_dev - Pointer to the SDIO function device
  
  \return -  0 for success -1 for failure
              
  --------------------------------------------------------------------------*/

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,29))
  static struct net_device_ops wlan_drv_ops = {
      .ndo_open = hdd_open,
      .ndo_stop = hdd_stop,
      .ndo_start_xmit = NULL,
      .ndo_tx_timeout = hdd_tx_timeout,
      .ndo_get_stats = hdd_stats,
      .ndo_do_ioctl = hdd_ioctl,
      .ndo_set_mac_address = hdd_set_mac_address,
 };
 #endif

int hdd_wlan_sdio_probe(struct sdio_func *sdio_func_dev )
{
   VOS_STATUS status;
   struct net_device *pWlanDev = NULL;
   hdd_adapter_t *pAdapter = NULL;
   v_CONTEXT_t pVosContext= NULL; 
   static char macAddr[6] =  {0x00, 0x0a, 0xf5, 0x89, 0x89, 0x89};
 
   ENTER();
      
   //Allocate the net_device and HDD Adapter (private data) 
   pWlanDev = alloc_etherdev(sizeof( hdd_adapter_t ));
   
   if(pWlanDev == NULL) 
   {
      hddLog(VOS_TRACE_LEVEL_ERROR,"%s: alloc_netdev failed", __func__);
      return -1; 
   }

   //Init the net_device structure
   ether_setup(pWlanDev); 
      
   pAdapter = netdev_priv(pWlanDev);
   
   //Initialize the adapter context to zeros.
   vos_mem_zero(pAdapter, sizeof( hdd_adapter_t ));

   /*Get vos context here bcoz vos_open requires it*/
   pVosContext = vos_get_global_context(VOS_MODULE_ID_SYS, NULL);

   //Save the Global VOSS context in adapter context for future.
   pAdapter->pvosContext = pVosContext;
   
   //Save the pointer to the net_device in the HDD adapter
   pAdapter->dev = pWlanDev;
   
   //Save the adapter context in global context for future.
   ((VosContextType*)(pVosContext))->pHDDContext = (v_VOID_t*)pAdapter;

   //Init the net_device structure
   strcpy(pWlanDev->name, "wlan%d");

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,29))
    pWlanDev->tx_queue_len = NET_DEV_TX_QUEUE_LEN,
    pWlanDev->netdev_ops = &wlan_drv_ops;
#else
   pWlanDev->open = hdd_open;
   pWlanDev->stop = hdd_stop;
   pWlanDev->hard_start_xmit = NULL;
   pWlanDev->tx_timeout = hdd_tx_timeout;
   pWlanDev->get_stats = hdd_stats;
   pWlanDev->do_ioctl = hdd_ioctl;
   pWlanDev->tx_queue_len = NET_DEV_TX_QUEUE_LEN;
   pWlanDev->set_mac_address = hdd_set_mac_address;
#endif
   vos_mem_copy(pWlanDev->dev_addr, (void *)macAddr,sizeof(macAddr));
   vos_mem_copy( &pAdapter->macAddressCurrent, macAddr,sizeof(macAddr));
   pWlanDev->watchdog_timeo = HDD_TX_TIMEOUT;
   pWlanDev->hard_header_len += LIBRA_HW_NEEDED_HEADROOM;

   /* set pWlanDev's parent to sdio device */
   SET_NETDEV_DEV(pWlanDev, &sdio_func_dev->dev);

   // Set the private data for the device to our adapter.
   libra_sdio_setprivdata (sdio_func_dev, pAdapter);
   atomic_set(&pAdapter->sdio_claim_count, 0);
   pAdapter->hsdio_func_dev = sdio_func_dev;

   init_completion(&pAdapter->full_pwr_comp_var);
   init_completion(&pAdapter->standby_comp_var);
   init_completion(&pAdapter->disconnect_comp_var);

   // Register the net device. Device should be registered to invoke
   // request_firmware API for reading the qcom_cfg.ini file
   if(register_netdev(pWlanDev))
   {
      hddLog(VOS_TRACE_LEVEL_ERROR,"%s: Failed:register_netdev",__func__); 
      goto err_free_netdev;
   }
#ifdef ANI_MANF_DIAG
    wlan_hdd_ftm_open(pAdapter);
    hddLog(VOS_TRACE_LEVEL_ERROR,"%s: FTM driver loaded success fully",__func__);
    return VOS_STATUS_SUCCESS;
#endif

   set_bit(NET_DEVICE_REGISTERED, &pAdapter->event_flags);

   // Load all config first as TL config is needed during vos_open
   pAdapter->cfg_ini = (hdd_config_t*) kmalloc(sizeof(hdd_config_t), GFP_KERNEL);
   if(pAdapter->cfg_ini == NULL)
   {  
      hddLog(VOS_TRACE_LEVEL_ERROR,"%s: Failed kmalloc hdd_config_t",__func__);
      goto err_netdev_unregister;
   }   

   vos_mem_zero(pAdapter->cfg_ini, sizeof( hdd_config_t ));   
   
   // Read and parse the qcom_cfg.ini file
   status = hdd_parse_config_ini( pAdapter );
   if ( VOS_STATUS_SUCCESS != status )
   {  
      hddLog(VOS_TRACE_LEVEL_ERROR,"%s: error parsing qcom_cfg.ini",__func__);
      goto err_config;   
   }   

   // Open VOSS 
   status = vos_open( &pVosContext, 0);
   
   if ( !VOS_IS_STATUS_SUCCESS( status ))
   {
      hddLog(VOS_TRACE_LEVEL_ERROR,"%s: vos_open failed",__func__);
      goto err_config;   
   }

   /* Save the hal context in Adapter */
   pAdapter->hHal = (tHalHandle)vos_get_context( VOS_MODULE_ID_HAL, pVosContext );
      
   if ( NULL == pAdapter->hHal )
   {
      hddLog(VOS_TRACE_LEVEL_ERROR,"%s: HAL context is null",__func__);	  
      goto err_vosclose;
   }
   
   //Set the Connection State to Not Connected
   pAdapter->conn_info.connState = eConnectionState_NotConnected;

   //Set the default operation channel
   pAdapter->conn_info.operationChannel = pAdapter->cfg_ini->OperatingChannel;

   /* Make the default Auth Type as OPEN*/
   pAdapter->conn_info.authType = eCSR_AUTH_TYPE_OPEN_SYSTEM;
  
   // Set the SME configuration parameters...
   status = hdd_set_sme_config( pAdapter );
   
   if ( VOS_STATUS_SUCCESS != status )
   {  
      hddLog(VOS_TRACE_LEVEL_ERROR,"%s: Failed hdd_set_sme_config",__func__); 
      goto err_vosclose;
   } 
   
   //Initialize the data path module
   status = hdd_init_tx_rx(pAdapter);
   if ( !VOS_IS_STATUS_SUCCESS( status ))
   {
      hddLog(VOS_TRACE_LEVEL_ERROR, "%s: hdd_init_tx_rx failed", __FUNCTION__);
      goto err_vosclose;
   }  

   //Initialize the WMM module
   status = hdd_wmm_init(pAdapter);
   if ( !VOS_IS_STATUS_SUCCESS( status ))
   {
      hddLog(VOS_TRACE_LEVEL_ERROR, "%s: hdd_wmm_init failed", __FUNCTION__);
      goto err_vosclose;
   }

   /* Start SAL now */
   status = WLANSAL_Start(pAdapter->pvosContext);
   if (!VOS_IS_STATUS_SUCCESS(status))
   {
      hddLog(VOS_TRACE_LEVEL_ERROR, "%s: Failed to start SAL",__func__);
      goto err_vosclose;
   }

   /*Start VOSS which starts up the SME/MAC/HAL modules and everything else
     Note: Firmware image will be read and downloaded inside vos_start API */
   status = vos_start( pAdapter->pvosContext );
   if ( !VOS_IS_STATUS_SUCCESS( status ) )
   {
      hddLog(VOS_TRACE_LEVEL_ERROR,"%s: vos_start failed",__func__);
      goto err_salstop;
   }

   status = hdd_post_voss_start_config( pAdapter );
   if ( !VOS_IS_STATUS_SUCCESS( status ) )
   {
      hddLog(VOS_TRACE_LEVEL_ERROR,"%s: hdd_post_voss_start_config failed", 
         __func__);
      goto err_vosstop;
   }

#ifdef CONFIG_HAS_EARLYSUSPEND
   // Register suspend/resume callbacks
   if(pAdapter->cfg_ini->nEnableSuspend)
      register_wlan_suspend();
#endif

   // Register wireless extensions         
   hdd_register_wext(pWlanDev);

   //Stop the Interface TX queue.
   netif_tx_disable(pWlanDev);
   netif_carrier_off(pWlanDev);

   //Safe to register the hard_start_xmit function again

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,29))
       wlan_drv_ops.ndo_start_xmit = hdd_hard_start_xmit;
#else
   pWlanDev->hard_start_xmit = hdd_hard_start_xmit;
#endif

   //Initialize the nlink service
   if(nl_srv_init() != 0)
   {
      hddLog(VOS_TRACE_LEVEL_ERROR,"%S: nl_srv_init failed",__func__);
      goto err_vosstop;
   }

   //Initialize the BTC service
   if(btc_activate_service(pAdapter) != 0)
   {
      hddLog(VOS_TRACE_LEVEL_ERROR,"%s: btc_activate_service failed",__func__);
      goto err_nl_srv;
   }

#ifdef PTT_SOCK_SVC_ENABLE
   //Initialize the PTT service
   if(ptt_sock_activate_svc(pAdapter) != 0)
   {
      hddLog(VOS_TRACE_LEVEL_ERROR,"%s: ptt_sock_activate_svc failed",__func__);
      goto err_nl_srv;
   }
#endif

   //Initialize the WoWL service
   if(!hdd_init_wowl(pAdapter))
   {
      hddLog(VOS_TRACE_LEVEL_ERROR,"%s: hdd_init_wowl failed",__func__);
      goto err_nl_srv;
   }

   //Trigger the initial scan
   hdd_wlan_initial_scan(pAdapter);

   goto success;

err_nl_srv:
   nl_srv_exit();
 
err_vosstop:
   vos_stop(pVosContext);

err_salstop:
	WLANSAL_Stop(pVosContext);

err_vosclose:	
   vos_close(pVosContext ); 

err_config:
   kfree(pAdapter->cfg_ini);
   pAdapter->cfg_ini= NULL;

err_netdev_unregister:
   if(test_bit(NET_DEVICE_REGISTERED, &pAdapter->event_flags)) {
      unregister_netdev(pWlanDev);
      clear_bit(NET_DEVICE_REGISTERED, &pAdapter->event_flags);
   }

err_free_netdev:
   free_netdev(pWlanDev);

   return -1;
	
success:	
   EXIT();
   return 0; 
}

/**---------------------------------------------------------------------------
  
  \brief hdd_module_init() - Init Function

   This is the driver entry point (invoked when module is loaded using insmod) 

  \param  - None 
                      
  \return - 0 for success, non zero for failure
  
  --------------------------------------------------------------------------*/

static int __init hdd_module_init (void)
{
   VOS_STATUS status;
   v_CONTEXT_t pVosContext = NULL;
   struct sdio_func *sdio_func_dev = NULL;
   unsigned int attempts = 0;
   ENTER();
   
   //Power Up Libra WLAN card first if not already powered up
   status = vos_chipPowerUp(NULL,NULL,NULL);
   if (!VOS_IS_STATUS_SUCCESS(status)) 
   {
      hddLog(VOS_TRACE_LEVEL_ERROR, "%s: Libra WLAN not Powered Up."
          "exiting", __func__);
      return -1;
   }

   //SDIO Polling should be turned on for card detection. When using Android Wi-Fi GUI
   //users need not trigger SDIO polling explicitly. However when loading drivers via
   //command line (Adb shell), users must turn on SDIO polling prior to loading WLAN.
   do {
      sdio_func_dev = libra_getsdio_funcdev();
      if (NULL == sdio_func_dev) {
         hddLog(VOS_TRACE_LEVEL_ERROR, "%s: Libra WLAN not detected yet.",__func__);
         attempts++;
      }
      else {
         hddLog(VOS_TRACE_LEVEL_ERROR, "%s: Libra WLAN detecton succeeded",__func__);
         break;
      }
      msleep(1000);

   }while (attempts < 3);

   if (NULL == sdio_func_dev) {
      hddLog(VOS_TRACE_LEVEL_ERROR, "%s: Libra WLAN not found!!",__func__);
      return -1;
   }

   /* Preopen VOSS so that it is ready to start at least SAL */
   status = vos_preOpen(&pVosContext);

   if (!VOS_IS_STATUS_SUCCESS(status)) 
   {
      hddLog(VOS_TRACE_LEVEL_ERROR,"%s: Failed to preOpen VOSS", __func__);
      return -1;
   }
   
   /* Now Open SAL */
   status = WLANSAL_Open(pVosContext, 0);

   if(!VOS_IS_STATUS_SUCCESS(status))
   {
      hddLog(VOS_TRACE_LEVEL_ERROR,"%s: Failed to open SAL", __func__);

      /* If unable to open, cleanup and return failure */
      vos_preClose( &pVosContext );
      return -1;
   }

   // Call our sdio probe.
   if(hdd_wlan_sdio_probe(sdio_func_dev)) {
      hddLog(VOS_TRACE_LEVEL_ERROR,"%s: WLAN Driver Initialization failed",
          __func__);
      return -1;
   }

   EXIT();	

   return 0;
}


/**---------------------------------------------------------------------------
  
  \brief hdd_module_exit() - Exit function
  
  This is the driver exit point (invoked when module is unloaded using rmmod)
  
  \param  - None
  
  \return - None
              
  --------------------------------------------------------------------------*/
static void __exit hdd_module_exit(void)
{   
   hdd_adapter_t *pAdapter = NULL;
   v_CONTEXT_t pVosContext = NULL;

   hddLog(VOS_TRACE_LEVEL_FATAL,"%s: Entering module exit",__func__);

   //Get the global vos context
   pVosContext = vos_get_global_context(VOS_MODULE_ID_SYS, NULL);

   if(!pVosContext) 
   {
      hddLog(VOS_TRACE_LEVEL_FATAL,"%s: Global VOS context is Null", __func__);
      return;
   }

   //Get the HDD context.
   pAdapter = (hdd_adapter_t *)vos_get_context(VOS_MODULE_ID_HDD, pVosContext );

   if(!pAdapter) 
   {
      hddLog(VOS_TRACE_LEVEL_FATAL,"%s: module exit called before probe",__func__);
   }
   else
   {
      //Do all the cleanup before deregistering the driver
      hdd_wlan_exit(pAdapter);
   }

   WLANSAL_Close(pVosContext);

   vos_preClose( &pVosContext );

   hddLog(VOS_TRACE_LEVEL_FATAL,"%s: Exiting module exit",__func__);
}


//Register the module init/exit functions
module_init(hdd_module_init);
module_exit(hdd_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("QUALCOMM");
MODULE_DESCRIPTION("WLAN HOST DEVICE DRIVER");
                                                 
