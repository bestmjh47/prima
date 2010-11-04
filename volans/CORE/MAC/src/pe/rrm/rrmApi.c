
/**=========================================================================
  
  \file  rrmApi.c
  
  \brief implementation for PE RRM APIs
  
   Copyright 2008 (c) Qualcomm, Incorporated.  All Rights Reserved.
   
   Qualcomm Confidential and Proprietary.
  
  ========================================================================*/

/* $Header$ */

#if defined WLAN_FEATURE_VOWIFI

/*--------------------------------------------------------------------------
  Include Files
  ------------------------------------------------------------------------*/
#include "palTypes.h"
#include "wniApi.h"
#include "sirApi.h"
#include "aniGlobal.h"
#include "halDataStruct.h"
#if (WNI_POLARIS_FW_PRODUCT == AP)
#include "wniCfgAp.h"
#else
#include "wniCfgSta.h"
#endif
#include "limTypes.h"
#include "limUtils.h"
#include "limSendSmeRspMessages.h"
#include "parserApi.h"
#include "limSendMessages.h"
#include "rrmGlobal.h"
#include "rrmApi.h"


// --------------------------------------------------------------------
/**
 * rrmCacheMgmtTxPower
 **
 * FUNCTION:  Store Tx power for management frames.
 *
 * LOGIC:
 *
 * ASSUMPTIONS:
 *
 * NOTE:
 *
 * @param pSessionEntry session entry.
 * @return None
 */
void
rrmCacheMgmtTxPower ( tpAniSirGlobal pMac, tPowerdBm txPower, tpPESession pSessionEntry )
{
#if defined WLAN_VOWIFI_DEBUG
   PELOGE(limLog( pMac, LOGE, "Cache Mgmt Tx Power = %d\n", txPower );) 
#endif
   if( pSessionEntry == NULL )
       pMac->rrm.rrmPEContext.txMgmtPower = txPower;
   else
       pSessionEntry->txMgmtPower = txPower;
}

// --------------------------------------------------------------------
/**
 * rrmGetMgmtTxPower
 *
 * FUNCTION:  Get the Tx power for management frames.
 *
 * LOGIC:
 *
 * ASSUMPTIONS:
 *
 * NOTE:
 *
 * @param pSessionEntry session entry.
 * @return txPower
 */
tPowerdBm
rrmGetMgmtTxPower ( tpAniSirGlobal pMac, tpPESession pSessionEntry )
{
#if defined WLAN_VOWIFI_DEBUG
   PELOGE(limLog( pMac, LOGE, "RrmGetMgmtTxPower called\n" );) 
#endif
   if( pSessionEntry == NULL )
      return pMac->rrm.rrmPEContext.txMgmtPower;
   
   return pSessionEntry->txMgmtPower;
}
// --------------------------------------------------------------------
/**
 * rrmSendSetMaxTxPowerReq
 *
 * FUNCTION:  Send SIR_HAL_SET_MAX_TX_POWER_REQ message to change the max tx power.
 *
 * LOGIC:
 *
 * ASSUMPTIONS:
 *
 * NOTE:
 *
 * @param txPower txPower to be set.
 * @param pSessionEntry session entry.
 * @return None
 */
tSirRetStatus
rrmSendSetMaxTxPowerReq ( tpAniSirGlobal pMac, tPowerdBm txPower, tpPESession pSessionEntry )
{
   tpMaxTxPowerParams pMaxTxParams;
   tSirRetStatus  retCode = eSIR_SUCCESS;
   tSirMsgQ       msgQ;

   if( pSessionEntry == NULL )
   {
      PELOGE(limLog(pMac, LOGE, "%s:%d: Inavalid parameters\n", __func__, __LINE__ );)
      return eSIR_FAILURE;
   }
   if( eHAL_STATUS_SUCCESS != palAllocateMemory( pMac->hHdd,
            (void **) &pMaxTxParams, sizeof(tMaxTxPowerParams) ) ) 
   {
      limLog( pMac, LOGP, "%s:%d:Unable to allocate memory for pMaxTxParams \n", __func__, __LINE__);
      return eSIR_MEM_ALLOC_FAILED;

   }
#if defined WLAN_VOWIFI_DEBUG
   PELOGE(limLog( pMac, LOGE, "%s:%d: Allocated memory for pMaxTxParams...will be freed in other module\n", __func__, __LINE__ );)
#endif
   pMaxTxParams->power = txPower;
   palCopyMemory( pMac->hHdd, pMaxTxParams->bssId, pSessionEntry->bssId, sizeof(tSirMacAddr) );
   palCopyMemory( pMac->hHdd, pMaxTxParams->selfStaMacAddr, pSessionEntry->selfMacAddr, sizeof(tSirMacAddr) );


   msgQ.type = SIR_HAL_SET_MAX_TX_POWER_REQ;
   msgQ.reserved = 0;
   msgQ.bodyptr = pMaxTxParams;
   msgQ.bodyval = 0;

   PELOGW(limLog(pMac, LOGW, FL( "Sending SIR_HAL_SET_MAX_TX_POWER_REQ to HAL"));)

      MTRACE(macTraceMsgTx(pMac, 0, msgQ.type));
   if( eSIR_SUCCESS != (retCode = halPostMsgApi( pMac, &msgQ )))
   {
      limLog( pMac, LOGP, FL("Posting SIR_HAL_SET_MAX_TX_POWER_REQ to HAL failed, reason=%X"), retCode );
      if (NULL != pMaxTxParams)
      {
         palFreeMemory( pMac->hHdd, (tANI_U8 *) pMaxTxParams );
      }
      return retCode;
   }
   return retCode;
}
// --------------------------------------------------------------------
/**
 * rrmSetMaxTxPowerRsp
 *
 * FUNCTION:  Process SIR_HAL_SET_MAX_TX_POWER_RSP message.
 *
 * LOGIC:
 *
 * ASSUMPTIONS:
 *
 * NOTE:
 *
 * @param txPower txPower to be set.
 * @param pSessionEntry session entry.
 * @return None
 */
tSirRetStatus
rrmSetMaxTxPowerRsp ( tpAniSirGlobal pMac, tpSirMsgQ limMsgQ )
{
   tSirRetStatus  retCode = eSIR_SUCCESS;
   tpMaxTxPowerParams pMaxTxParams = (tpMaxTxPowerParams) limMsgQ->bodyptr;
   tpPESession     pSessionEntry;
   tANI_U8 sessionId;

   if((pSessionEntry = peFindSessionByBssid(pMac, pMaxTxParams->bssId, &sessionId))==NULL)
   {
      PELOGE(limLog(pMac, LOGE,FL("%s:%d:Unable to find session:\n"),__func__,__LINE__ );)
      retCode = eSIR_FAILURE;
   }
   else
   {
      rrmCacheMgmtTxPower ( pMac, pMaxTxParams->power, pSessionEntry );
   }

   palFreeMemory(pMac->hHdd, (void*)limMsgQ->bodyptr);
   return retCode;
}
// --------------------------------------------------------------------
/**
 * rrmProcessLinkMeasurementRequest
 *
 * FUNCTION:  Processes the Link measurement request and send the report.
 *
 * LOGIC:
 *
 * ASSUMPTIONS:
 *
 * NOTE:
 *
 * @param pBd pointer to BD to extract RSSI and SNR
 * @param pLinkReq pointer to the Link request frame structure.
 * @param pSessionEntry session entry.
 * @return None
 */
tSirRetStatus
rrmProcessLinkMeasurementRequest( tpAniSirGlobal pMac, 
                                  tANI_U8 *pBd,
                                  tDot11fLinkMeasurementRequest *pLinkReq,
                                  tpPESession pSessionEntry )
{
   tSirMacLinkReport LinkReport;
   tpSirMacMgmtHdr   pHdr;
   v_S7_t            currentRSSI = 0;

#if defined WLAN_VOWIFI_DEBUG
   PELOGE(limLog( pMac, LOGE, "Recieved Link measurement request\n");)
#endif
   if( pBd == NULL || pLinkReq == NULL || pSessionEntry == NULL )
   {
      PELOGE(limLog( pMac, LOGE, "%s:%d: Invalid parameters - Ignoring the request\n");)
   }
   pHdr = SIR_MAC_BD_TO_MPDUHEADER( pBd );
#if defined WLAN_VOWIFI_DEBUG
   if( pSessionEntry->maxTxPower != (tPowerdBm) pLinkReq->MaxTxPower.maxTxPower )
   {
      PELOGE(limLog( pMac, LOGE, "%s:%d: maxTx power in link request is not same as local...Local = %d LinkReq = %d\n", __func__, __LINE__,  pSessionEntry->maxTxPower, pLinkReq->MaxTxPower.maxTxPower );)
   }
#endif

   LinkReport.dialogToken = pLinkReq->DialogToken.token;
   LinkReport.txPower = pSessionEntry->txMgmtPower;
   LinkReport.rxAntenna = 0;
   LinkReport.txAntenna = 0;
   currentRSSI = SIR_MAC_BD_TO_RSSI_DB(pBd);

#if defined WLAN_VOWIFI_DEBUG
   PELOGE(limLog( pMac, LOGE, "Received Link report frame with %d\n", currentRSSI);)
#endif

   // 2008 11k spec reference: 18.4.8.5 RCPI Measurement
   if ((currentRSSI) <= RCPI_LOW_RSSI_VALUE)
       LinkReport.rcpi = 0; 
   else if ((currentRSSI > RCPI_LOW_RSSI_VALUE) && (currentRSSI <= 0))
       LinkReport.rcpi = CALCULATE_RCPI(currentRSSI);
   else 
       LinkReport.rcpi = RCPI_MAX_VALUE; 

   LinkReport.rsni = WLANHAL_RX_BD_GET_SNR(pBd); 
   
#if defined WLAN_VOWIFI_DEBUG
   PELOGE(limLog( pMac, LOGE, "Sending Link report frame\n");)
#endif
   return limSendLinkReportActionFrame( pMac, &LinkReport, pHdr->sa, pSessionEntry ); 


}

// --------------------------------------------------------------------
/**
 * rrmProcessNeighborReportResponse
 *
 * FUNCTION:  Processes the Neighbor Report response from the peer AP.
 *
 * LOGIC:
 *
 * ASSUMPTIONS:
 *
 * NOTE:
 *
 * @param pNeighborRep pointer to the Neighbor report frame structure.
 * @param pSessionEntry session entry.
 * @return None
 */
tSirRetStatus
rrmProcessNeighborReportResponse( tpAniSirGlobal pMac,
                                  tDot11fNeighborReportResponse *pNeighborRep,
                                  tpPESession pSessionEntry )
{
   tSirRetStatus status = eSIR_FAILURE;
   tpSirNeighborReportInd pSmeNeighborRpt = NULL;
   tANI_U16 length;
   tANI_U8 i;
   tSirMsgQ                  mmhMsg;

   if( pNeighborRep == NULL || pSessionEntry == NULL )
   {
      PELOGE(limLog( pMac, LOGE, "%s: Invalid parameters\n", __func__);)
      return status;
   }

#if defined WLAN_VOWIFI_DEBUG
   PELOGE(limLog( pMac, LOGE, "%s:%d:Neighbor report response recieved \n",__func__, __LINE__);)
#endif

   // Dialog token
   if( pMac->rrm.rrmPEContext.DialogToken != pNeighborRep->DialogToken.token )
   {
      PELOGE(limLog( pMac, LOGE, "Dialog token mismatch in the recieved Neighbor report\n");)
      return eSIR_FAILURE;
   }
   if( pNeighborRep->num_NeighborReport == 0 )
   {
      PELOGE(limLog( pMac, LOGE, "No neighbor report in the frame...Dropping it\n");)
      return eSIR_FAILURE;
   }
   length = (sizeof( tSirNeighborReportInd )) +
            (sizeof( tSirNeighborBssDescription ) * (pNeighborRep->num_NeighborReport - 1) ) ; 
            
   //Prepare the request to send to SME.
   if( eHAL_STATUS_SUCCESS != palAllocateMemory( pMac->hHdd,
            (void **) &pSmeNeighborRpt, length ) )
   {
      PELOGE(limLog( pMac, LOGP, "%s:%d:Unable to allocate memory\n", __func__, __LINE__ );)
      return eSIR_MEM_ALLOC_FAILED;

   }
   palZeroMemory( pMac->hHdd, pSmeNeighborRpt, length ); 
#if defined WLAN_VOWIFI_DEBUG
   PELOGE(limLog( pMac, LOGE, "%s:%d: Allocated memory for pSmeNeighborRpt...will be freed by other module\n", __func__, __LINE__ );)
#endif

   for( i = 0 ; i < pNeighborRep->num_NeighborReport ; i++ )
   {
      pSmeNeighborRpt->sNeighborBssDescription[i].length = sizeof( tSirNeighborBssDescription ); /*+ any optional ies */
      palCopyMemory( pMac->hHdd, pSmeNeighborRpt->sNeighborBssDescription[i].bssId,
            pNeighborRep->NeighborReport[i].bssid, sizeof(tSirMacAddr) );
      pSmeNeighborRpt->sNeighborBssDescription[i].bssidInfo.fApPreauthReachable = pNeighborRep->NeighborReport[i].APReachability;
      pSmeNeighborRpt->sNeighborBssDescription[i].bssidInfo.fSameSecurityMode = pNeighborRep->NeighborReport[i].Security;
      pSmeNeighborRpt->sNeighborBssDescription[i].bssidInfo.fSameAuthenticator = pNeighborRep->NeighborReport[i].KeyScope;
      pSmeNeighborRpt->sNeighborBssDescription[i].bssidInfo.fCapSpectrumMeasurement = pNeighborRep->NeighborReport[i].SpecMgmtCap;
      pSmeNeighborRpt->sNeighborBssDescription[i].bssidInfo.fCapQos = pNeighborRep->NeighborReport[i].QosCap;
      pSmeNeighborRpt->sNeighborBssDescription[i].bssidInfo.fCapApsd = pNeighborRep->NeighborReport[i].apsd;
      pSmeNeighborRpt->sNeighborBssDescription[i].bssidInfo.fCapRadioMeasurement = pNeighborRep->NeighborReport[i].rrm;
      pSmeNeighborRpt->sNeighborBssDescription[i].bssidInfo.fCapDelayedBlockAck = pNeighborRep->NeighborReport[i].DelayedBA;
      pSmeNeighborRpt->sNeighborBssDescription[i].bssidInfo.fCapImmediateBlockAck = pNeighborRep->NeighborReport[i].ImmBA;
      pSmeNeighborRpt->sNeighborBssDescription[i].bssidInfo.fMobilityDomain = pNeighborRep->NeighborReport[i].MobilityDomain;

      pSmeNeighborRpt->sNeighborBssDescription[i].regClass = pNeighborRep->NeighborReport[i].regulatoryClass;
      pSmeNeighborRpt->sNeighborBssDescription[i].channel = pNeighborRep->NeighborReport[i].channel;
      pSmeNeighborRpt->sNeighborBssDescription[i].phyType = pNeighborRep->NeighborReport[i].PhyType;
   }

   pSmeNeighborRpt->messageType = eWNI_SME_NEIGHBOR_REPORT_IND;
   pSmeNeighborRpt->length = length;
   pSmeNeighborRpt->numNeighborReports = pNeighborRep->num_NeighborReport;
   palCopyMemory( pMac->hHdd, pSmeNeighborRpt->bssId, pSessionEntry->bssId, sizeof(tSirMacAddr) );

   //Send request to SME.
   mmhMsg.type    = pSmeNeighborRpt->messageType;
   mmhMsg.bodyptr = pSmeNeighborRpt;
   MTRACE(macTraceMsgTx(pMac, 0, mmhMsg.type));
   status = limHalMmhPostMsgApi(pMac, &mmhMsg, ePROT);

   return status;

}

// --------------------------------------------------------------------
/**
 * rrmProcessNeighborReportReq
 *
 * FUNCTION:  
 *
 * LOGIC: Create a Neighbor report request and send it to peer.
 *
 * ASSUMPTIONS:
 *
 * NOTE:
 *
 * @param pNeighborReq Neighbor report request params .
 * @return None
 */
static tSirRetStatus
rrmProcessNeighborReportReq( tpAniSirGlobal pMac,
                            tpSirNeighborReportReqInd pNeighborReq )
{
   tSirRetStatus status = eSIR_SUCCESS;
   tSirMacNeighborReportReq NeighborReportReq;
   tpPESession pSessionEntry ;
   tANI_U8 sessionId;

   if( pNeighborReq == NULL )
   {
      PELOGE(limLog( pMac, LOGE, "NeighborReq is NULL\n" );)
      return eSIR_FAILURE;
   }
   if ((pSessionEntry = peFindSessionByBssid(pMac,pNeighborReq->bssId,&sessionId))==NULL)
   {
      PELOGE(limLog(pMac, LOGE,FL("session does not exist for given bssId\n"));)
      return eSIR_FAILURE;
   }

#if defined WLAN_VOWIFI_DEBUG
   PELOGE(limLog( pMac, LOGE, "%s:%d:Neighbor Request recieved \n",__func__, __LINE__);)
   PELOGE(limLog( pMac, LOGE, "SSID present = %d \n", pNeighborReq->noSSID );)
#endif

   palZeroMemory( pMac->hHdd, &NeighborReportReq, sizeof( tSirMacNeighborReportReq ) );

   NeighborReportReq.dialogToken = ++pMac->rrm.rrmPEContext.DialogToken; 
   NeighborReportReq.ssid_present = !pNeighborReq->noSSID; 
   if( NeighborReportReq.ssid_present )
   {
      palCopyMemory( pMac->hHdd, &NeighborReportReq.ssid, &pNeighborReq->ucSSID, sizeof(tSirMacSSid) );
#if defined WLAN_VOWIFI_DEBUG
      PELOGE(sirDumpBuf( pMac, SIR_LIM_MODULE_ID, LOGE, (tANI_U8*) NeighborReportReq.ssid.ssId, NeighborReportReq.ssid.length );)
#endif
   }

   status = limSendNeighborReportRequestFrame( pMac, &NeighborReportReq, pNeighborReq->bssId, pSessionEntry );

   return status;
}

#define ABS(x)      ((x < 0) ? -x : x)
// --------------------------------------------------------------------
/**
 * rrmProcessBeaconReportReq
 *
 * FUNCTION:  Processes the Beacon report request from the peer AP.
 *
 * LOGIC:
 *
 * ASSUMPTIONS:
 *
 * NOTE:
 *
 * @param pCurrentReq pointer to the current Req comtext.
 * @param pBeaconReq pointer to the beacon report request IE from the peer.
 * @param pSessionEntry session entry.
 * @return None
 */
static tRrmRetStatus
rrmProcessBeaconReportReq( tpAniSirGlobal pMac, 
                           tpRRMReq pCurrentReq,
                           tDot11fIEMeasurementRequest *pBeaconReq,
                           tpPESession pSessionEntry )
{
   tSirMsgQ                  mmhMsg;
   tpSirBeaconReportReqInd pSmeBcnReportReq;
   tANI_U8 num_channels = 0, num_APChanReport;
   tANI_U16 measDuration, maxMeasduration;
   tANI_S8  maxDuration;
   tANI_U8  sign;

   if( pBeaconReq->measurement_request.Beacon.BeaconReporting.present && 
       (pBeaconReq->measurement_request.Beacon.BeaconReporting.reportingCondition != 0) )
   {
      //Repeated measurement is not supported. This means number of repetitions should be zero.(Already checked)
      //All test case in VoWifi(as of version 0.36)  use zero for number of repetitions.
      //Beacon reporting should not be included in request if number of repetitons is zero.
      // IEEE Std 802.11k-2008 Table 7-29g and section 11.10.8.1

      PELOGE(limLog( pMac, LOGE, "Droping the request: Reporting condition included in beacon report request and it is not zero\n");)
      return eRRM_INCAPABLE;
   }

   /* The logic here is to check the measurement duration passed in the beacon request. Following are the cases handled.
      Case 1: If measurement duration received in the beacon request is greater than the max measurement duration advertised 
                in the RRM capabilities(Assoc Req), and Duration Mandatory bit is set to 1, REFUSE the beacon request
      Case 2: If measurement duration received in the beacon request is greater than the max measurement duration advertised 
                in the RRM capabilities(Assoc Req), and Duration Mandatory bit is set to 0, perform measurement for 
                the duration advertised in the RRM capabilities
      
      maxMeasurementDuration = 2^(nonOperatingChanMax - 4) * BeaconInterval
    */
   maxDuration = pMac->rrm.rrmPEContext.rrmEnabledCaps.nonOperatingChanMax - 4;
   sign = (maxDuration < 0) ? 1 : 0;
   maxDuration = (1L << ABS(maxDuration));
   if (!sign)
      maxMeasduration = maxDuration * pSessionEntry->beaconInterval;
   else
      maxMeasduration = pSessionEntry->beaconInterval / maxDuration;

   measDuration = pBeaconReq->measurement_request.Beacon.meas_duration; 

#if defined WLAN_VOWIFI_DEBUG
   limLog( pMac, LOGE, "maxDuration = %d sign = %d maxMeasduration = %d measDuration = %d\n",
        maxDuration, sign, maxMeasduration, measDuration ); 
#endif

   if( maxMeasduration < measDuration )
   {
      if( pBeaconReq->durationMandatory )
      {
         limLog( pMac, LOGE, "Droping the request: duration mandatory and maxduration > measduration\n");
         return eRRM_REFUSED;
      }
      else
         measDuration = maxMeasduration;
   }

   //Cache the data required for sending report.
   pCurrentReq->request.Beacon.reportingDetail = pBeaconReq->measurement_request.Beacon.BcnReportingDetail.present ?
      pBeaconReq->measurement_request.Beacon.BcnReportingDetail.reportingDetail :
      BEACON_REPORTING_DETAIL_ALL_FF_IE ;

   if( pBeaconReq->measurement_request.Beacon.RequestedInfo.present )
   {
      if( eHAL_STATUS_SUCCESS != palAllocateMemory( pMac->hHdd, (void**) &pCurrentReq->request.Beacon.reqIes.pElementIds, 
                                                    ( sizeof( tANI_U8) * 
                                                    pBeaconReq->measurement_request.Beacon.RequestedInfo.num_requested_eids ) ) )
      {
            limLog( pMac, LOGP,
               FL( "Unable to PAL allocate memory for request IEs buffer\n" ));
            return eRRM_FAILURE;
      }
#if defined WLAN_VOWIFI_DEBUG
      PELOGE(limLog( pMac, LOGE, "%s:%d: Allocated memory for pElementIds\n", __func__, __LINE__ );)
#endif
      pCurrentReq->request.Beacon.reqIes.num = pBeaconReq->measurement_request.Beacon.RequestedInfo.num_requested_eids;
      palCopyMemory ( pMac->hHdd, pCurrentReq->request.Beacon.reqIes.pElementIds, 
            pBeaconReq->measurement_request.Beacon.RequestedInfo.requested_eids, 
            pCurrentReq->request.Beacon.reqIes.num );      
   }

   if( pBeaconReq->measurement_request.Beacon.num_APChannelReport )
   {
      for( num_APChanReport = 0 ; num_APChanReport < pBeaconReq->measurement_request.Beacon.num_APChannelReport ; num_APChanReport++ )
         num_channels += pBeaconReq->measurement_request.Beacon.APChannelReport[num_APChanReport].num_channelList;
   }

   //Prepare the request to send to SME.
   if( eHAL_STATUS_SUCCESS != palAllocateMemory( pMac->hHdd,
            (void **) &pSmeBcnReportReq,
            (sizeof( tSirBeaconReportReqInd ) + num_channels) ) )
   {
      limLog( pMac, LOGP,
            FL( "Unable to PAL allocate memory during Beacon Report Req Ind to SME\n" ));

      return eRRM_FAILURE;

   }

   palZeroMemory( pMac->hHdd, pSmeBcnReportReq, sizeof( tSirBeaconReportReqInd ) + num_channels );

#if defined WLAN_VOWIFI_DEBUG
   PELOGE(limLog( pMac, LOGE, "%s:%d: Allocated memory for pSmeBcnReportReq....will be freed by other module\n", __func__, __LINE__ );)
#endif
   palCopyMemory( pMac->hHdd, pSmeBcnReportReq->bssId, pSessionEntry->bssId, sizeof(tSirMacAddr) );
   pSmeBcnReportReq->messageType = eWNI_SME_BEACON_REPORT_REQ_IND;
   pSmeBcnReportReq->length = sizeof( tSirBeaconReportReqInd ) + num_channels;
   pSmeBcnReportReq->uDialogToken = pBeaconReq->measurement_token;
   //pSmeBcnReportReq->measurementDuration = SYS_TU_TO_MS(pBeaconReq->measurement_request.Beacon.meas_duration);
   pSmeBcnReportReq->measurementDuration = SYS_TU_TO_MS(measDuration /*pBeaconReq->measurement_request.Beacon.meas_duration*/);
   pSmeBcnReportReq->randomizationInterval = SYS_TU_TO_MS (pBeaconReq->measurement_request.Beacon.randomization);
   pSmeBcnReportReq->fMeasurementtype = pBeaconReq->measurement_request.Beacon.meas_mode;
   pSmeBcnReportReq->channelInfo.regulatoryClass = pBeaconReq->measurement_request.Beacon.regClass;
   pSmeBcnReportReq->channelInfo.channelNum = pBeaconReq->measurement_request.Beacon.channel;
   palCopyMemory( pMac->hHdd, pSmeBcnReportReq->macaddrBssid, pBeaconReq->measurement_request.Beacon.BSSID, sizeof(tSirMacAddr) );

   if( pBeaconReq->measurement_request.Beacon.SSID.present )
   {
      pSmeBcnReportReq->ssId.length = pBeaconReq->measurement_request.Beacon.SSID.num_ssid;
      palCopyMemory( pMac->hHdd, pSmeBcnReportReq->ssId.ssId,  pBeaconReq->measurement_request.Beacon.SSID.ssid, 
            pSmeBcnReportReq->ssId.length );
   }

   pCurrentReq->token = pBeaconReq->measurement_token;

   pSmeBcnReportReq->channelList.numChannels = num_channels;
   if( pBeaconReq->measurement_request.Beacon.num_APChannelReport )
   {
      tANI_U8 *pChanList = pSmeBcnReportReq->channelList.channelNumber;
      for( num_APChanReport = 0 ; num_APChanReport < pBeaconReq->measurement_request.Beacon.num_APChannelReport ; num_APChanReport++ )
      {
         palCopyMemory( pMac->hHdd, pChanList, 
               pBeaconReq->measurement_request.Beacon.APChannelReport[num_APChanReport].channelList, 
               pBeaconReq->measurement_request.Beacon.APChannelReport[num_APChanReport].num_channelList );

         pChanList += pBeaconReq->measurement_request.Beacon.APChannelReport[num_APChanReport].num_channelList;
      }
   }

   //Send request to SME.
   mmhMsg.type    = eWNI_SME_BEACON_REPORT_REQ_IND;
   mmhMsg.bodyptr = pSmeBcnReportReq;
   MTRACE(macTraceMsgTx(pMac, 0, mmhMsg.type));
   return limHalMmhPostMsgApi(pMac, &mmhMsg, ePROT);
}

// --------------------------------------------------------------------
/**
 * rrmFillBeaconIes
 *
 * FUNCTION:  
 *
 * LOGIC: Fills Fixed fields and Ies in bss description to an array of tANI_U8.
 *
 * ASSUMPTIONS:
 *
 * NOTE:
 *
 * @param pIes - pointer to the buffer that should be populated with ies.
 * @param pNumIes - returns the num of ies filled in this param.
 * @param pIesMaxSize - Max size of the buffer pIes.
 * @param eids - pointer to array of eids. If NULL, all ies will be populated.
 * @param numEids - number of elements in array eids.
 * @param pBssDesc - pointer to Bss Description.
 * @return None
 */
static void
rrmFillBeaconIes( tpAniSirGlobal pMac, 
                  tANI_U8 *pIes, tANI_U8 *pNumIes, tANI_U8 pIesMaxSize, 
                  tANI_U8 *eids, tANI_U8 numEids,
                  tpSirBssDescription pBssDesc )
{
   tANI_U8 len, *pBcnIes, BcnNumIes, count = 0, i;

   if( (pIes == NULL) || (pNumIes == NULL) || (pBssDesc == NULL) )
   {
      PELOGE(limLog( pMac, LOGE, "%s:%d: Invalid parameters\n", __func__, __LINE__ );)
      return;
   }

   //Make sure that if eid is null, numEids is set to zero.
   numEids = (eids == NULL) ? 0 : numEids;

   pBcnIes = (tANI_U8*) &pBssDesc->ieFields[0];
   BcnNumIes = GET_IE_LEN_IN_BSS( pBssDesc->length );

   *pNumIes = 0;

   *((tANI_U32*)pIes) = pBssDesc->timeStamp[0];
   *pNumIes+=sizeof(tANI_U32); pIes+=sizeof(tANI_U32);
   *((tANI_U32*)pIes) = pBssDesc->timeStamp[1];
   *pNumIes+=sizeof(tANI_U32); pIes+=sizeof(tANI_U32);
   *((tANI_U16*)pIes) =  pBssDesc->beaconInterval;
   *pNumIes+=sizeof(tANI_U16); pIes+=sizeof(tANI_U16);
   *((tANI_U16*)pIes) = pBssDesc->capabilityInfo;
   *pNumIes+=sizeof(tANI_U16); pIes+=sizeof(tANI_U16);

   while ( BcnNumIes > 0 ) 
   {
      len = *(pBcnIes + 1) + 2; //element id + length.
#if defined WLAN_VOWIFI_DEBUG
      PELOGE(limLog( pMac, LOGE, "EID = %d, len = %d total = %d\n", *pBcnIes, *(pBcnIes+1), len );)
#endif

      i = 0;
      do
      {  
         if( ( (eids == NULL) || ( *pBcnIes == eids[i] ) )  &&
             ( (*pNumIes) + len) < pIesMaxSize )
         {
#if defined WLAN_VOWIFI_DEBUG
            PELOGE(limLog( pMac, LOGE, "Adding Eid %d, len=%d\n", *pBcnIes, len );)
#endif
            palCopyMemory( pMac->hHdd, pIes, pBcnIes, len ); 
            pIes += len;
            *pNumIes += len;
            count++;
            break;
         }
         i++;
      }while( i < numEids );

      pBcnIes += len;
      BcnNumIes -= len;
   }
#if defined WLAN_VOWIFI_DEBUG
   PELOGE(limLog( pMac, LOGE, "Total length of Ies added = %d\n", *pNumIes );)
#endif
}
      
// --------------------------------------------------------------------
/**
 * rrmProcessBeaconReportXmit
 *
 * FUNCTION:  
 *
 * LOGIC: Create a Radio measurement report action frame and send it to peer.
 *
 * ASSUMPTIONS:
 *
 * NOTE:
 *
 * @param pBcnReport Data for beacon report IE from SME.
 * @return None
 */
tSirRetStatus
rrmProcessBeaconReportXmit( tpAniSirGlobal pMac,
                            tpSirBeaconReportXmitInd pBcnReport)
{
   tSirRetStatus status = eSIR_SUCCESS;
   tSirMacRadioMeasureReport report, *pReport;
   tpRRMReq pCurrentReq = pMac->rrm.rrmPEContext.pCurrentReq; 
   tpPESession pSessionEntry ;
   tANI_U8 sessionId;

#if defined WLAN_VOWIFI_DEBUG
   PELOGE(limLog( pMac, LOGE, "Recieved beacon report xmit indication\n");)  
#endif
   if(NULL == pBcnReport)
      return eSIR_FAILURE;

   if ( pCurrentReq == NULL )
   {
      PELOGE(limLog( pMac, LOGE, "Recieved report xmit while there is no request pending in PE\n");)
      return eSIR_FAILURE;
   }
   if ((pSessionEntry = peFindSessionByBssid(pMac,pBcnReport->bssId,&sessionId))==NULL)
   {
      PELOGE(limLog(pMac, LOGE,FL("session does not exist for given bssId\n"));)
      return eSIR_FAILURE;
   }

   pReport = &report;
   palZeroMemory( pMac->hHdd, pReport, sizeof(tSirMacRadioMeasureReport) );
   //Prepare the beacon report and send it to the peer.
   pReport->token = pBcnReport->uDialogToken;
   pReport->refused = 0;
   pReport->incapable = 0;
   pReport->type = SIR_MAC_RRM_BEACON_TYPE;

   //Valid response is included if the size of beacon xmit is == size of beacon xmit ind + ies 
   if ( pBcnReport->length > sizeof( tSirBeaconReportXmitInd ) )
   {
      pReport->report.beaconReport.regClass =  pBcnReport->regClass;  
      pReport->report.beaconReport.channel = pBcnReport->bssDescription[0].channelId;
      palCopyMemory( pMac->hHdd, pReport->report.beaconReport.measStartTime, pBcnReport->bssDescription[0].startTSF, sizeof( pBcnReport->bssDescription[0].startTSF) );
      pReport->report.beaconReport.measDuration = SYS_MS_TO_TU(pBcnReport->duration);
      pReport->report.beaconReport.phyType = pBcnReport->bssDescription[0].nwType; //TODO: check this.
      pReport->report.beaconReport.bcnProbeRsp = 1;
      pReport->report.beaconReport.rsni = pBcnReport->bssDescription[0].sinr;
      pReport->report.beaconReport.rcpi = pBcnReport->bssDescription[0].rssi;

      pReport->report.beaconReport.antennaId = 0;
      pReport->report.beaconReport.parentTSF = pBcnReport->bssDescription[0].parentTSF; 
      palCopyMemory(pMac->hHdd, pReport->report.beaconReport.bssid, pBcnReport->bssDescription[0].bssId, sizeof(tSirMacAddr));

      switch ( pCurrentReq->request.Beacon.reportingDetail )
      {
         case BEACON_REPORTING_DETAIL_NO_FF_IE: //0 No need to include any elements.
#if defined WLAN_VOWIFI_DEBUG
            PELOGE(limLog(pMac, LOGE, "No reporting detail requested\n");)
#endif
            break;
         case BEACON_REPORTING_DETAIL_ALL_FF_REQ_IE: //1: Include all FFs and Requested Ies.
#if defined WLAN_VOWIFI_DEBUG
            PELOGE(limLog(pMac, LOGE, "Only requested IEs in reporting detail requested\n");)
#endif

            rrmFillBeaconIes( pMac, (tANI_U8*) &pReport->report.beaconReport.Ies[0], 
                  (tANI_U8*) &pReport->report.beaconReport.numIes, BEACON_REPORT_MAX_IES,
                  pCurrentReq->request.Beacon.reqIes.pElementIds, pCurrentReq->request.Beacon.reqIes.num,
                  &pBcnReport->bssDescription[0] );

            break;
         case BEACON_REPORTING_DETAIL_ALL_FF_IE: //2 / default - Include all FFs and all Ies.
         default:
#if defined WLAN_VOWIFI_DEBUG
            PELOGE(limLog(pMac, LOGE, "Default all IEs and FFs\n");)
#endif
            rrmFillBeaconIes( pMac, (tANI_U8*) &pReport->report.beaconReport.Ies[0], 
                  (tANI_U8*) &pReport->report.beaconReport.numIes, BEACON_REPORT_MAX_IES,
                  NULL, 0,
                  &pBcnReport->bssDescription[0] );
            break;
      }

#if defined WLAN_VOWIFI_DEBUG
      PELOGE(limLog( pMac, LOGE, "Sending Action frame \n");)
#endif
      limSendRadioMeasureReportActionFrame( pMac, pCurrentReq->dialog_token, 1, 
            pReport, pBcnReport->bssId, pSessionEntry );
   }

   if( pBcnReport->fMeasureDone )
   {
      PELOGE(limLog( pMac, LOGE, "Measurement done....cleanup the context\n");)

      rrmCleanup(pMac);
   }
   return status;
}

void rrmProcessBeaconRequestFailure(tpAniSirGlobal pMac, tpPESession pSessionEntry, 
                                                tSirMacAddr peer, tRrmRetStatus status)
{
    tpSirMacRadioMeasureReport pReport = NULL;
    tpRRMReq pCurrentReq = pMac->rrm.rrmPEContext.pCurrentReq; 

    if( eHAL_STATUS_SUCCESS != palAllocateMemory( pMac->hHdd,
               (void **) &pReport,
               sizeof( tSirMacRadioMeasureReport ) ) )
    {
         limLog( pMac, LOGP,
               FL( "Unable to PAL allocate memory during RRM Req processing\n" ));
         return;
    }
    palZeroMemory( pMac->hHdd, pReport, sizeof(tSirMacRadioMeasureReport) );
    pReport->token = pCurrentReq->token;
    pReport->type = SIR_MAC_RRM_BEACON_TYPE;

    switch (status)
    {
        case eRRM_REFUSED:
            pReport->refused = 1;
            break;            
        case eRRM_INCAPABLE:
            pReport->incapable = 1;
            break;
        default:
            PELOGE(limLog( pMac, LOGE, "%s: Beacon request processing failed no report sent with status %d \n", __func__, status););
            palFreeMemory( pMac->hHdd, pReport );
            return;
    }

    limSendRadioMeasureReportActionFrame( pMac, pCurrentReq->dialog_token, 1, 
                                                        pReport, peer, pSessionEntry );

    palFreeMemory( pMac->hHdd, pReport );
#if defined WLAN_VOWIFI_DEBUG
    PELOGE(limLog( pMac, LOGE, "%s:%d: Free memory for pReport\n", __func__, __LINE__ );)
#endif
    return;
}

// --------------------------------------------------------------------
/**
 * rrmProcessRadioMeasurementRequest
 *
 * FUNCTION:  Processes the Radio Resource Measurement request.
 *
 * LOGIC:
 

*
 * ASSUMPTIONS:
 *
 * NOTE:
 *
 * @param peer Macaddress of the peer requesting the radio measurement.
 * @param pRRMReq Array of Measurement request IEs
 * @param pSessionEntry session entry.
 * @return None
 */
tSirRetStatus
rrmProcessRadioMeasurementRequest( tpAniSirGlobal pMac, 
                                  tSirMacAddr peer,
                                  tDot11fRadioMeasurementRequest *pRRMReq,
                                  tpPESession pSessionEntry )
{
   tANI_U8 i;
   tSirRetStatus status = eSIR_SUCCESS;
   tpSirMacRadioMeasureReport pReport = NULL;
   tANI_U8 num_report = 0;
   tpRRMReq pCurrentReq = pMac->rrm.rrmPEContext.pCurrentReq; 
   tRrmRetStatus    rrmStatus = eRRM_SUCCESS;

   if( !pRRMReq->num_MeasurementRequest )
   {
      //No measurement requests....
      //
      PELOGE(limLog( pMac, LOGE, "No requestIes in the measurement request\n" );)
      return eSIR_FAILURE;
   }

   // PF Fix
   if( pRRMReq->NumOfRepetitions.repetitions > 0 )
   {
      //Send a report with incapable bit set. Not supporting repetitions.
      if( eHAL_STATUS_SUCCESS != palAllocateMemory( pMac->hHdd,
               (void **) &pReport,
               sizeof( tSirMacRadioMeasureReport ) ) )
      {
         limLog( pMac, LOGP,
               FL( "Unable to PAL allocate memory during RRM Req processing\n" ));
         return eSIR_MEM_ALLOC_FAILED;
      }
      palZeroMemory( pMac->hHdd, pReport, sizeof(tSirMacRadioMeasureReport) );
#if defined WLAN_VOWIFI_DEBUG
      PELOGE(limLog( pMac, LOGE, "%s:%d: Allocated memory for pReport\n", __func__, __LINE__ );)
#endif
      pReport->incapable = 1;
      pReport->type = pRRMReq->MeasurementRequest[0].measurement_type;
      num_report = 1;
      goto end;

   }

   for( i= 0; i < pRRMReq->num_MeasurementRequest; i++ )
   {
      switch( pRRMReq->MeasurementRequest[i].measurement_type )
      {
         case SIR_MAC_RRM_BEACON_TYPE:
            //Process beacon request.
            if( pCurrentReq )
            {
               if ( pReport == NULL ) //Allocate memory to send reports for any subsequent requests.
               {
                  if( eHAL_STATUS_SUCCESS != palAllocateMemory( pMac->hHdd,
                           (void **) &pReport,
                           sizeof( tSirMacRadioMeasureReport ) * (pRRMReq->num_MeasurementRequest - i) ) )
                  {
                     limLog( pMac, LOGP,
                           FL( "Unable to PAL allocate memory during RRM Req processing\n" ));
                     return eSIR_MEM_ALLOC_FAILED;
                  }
                  palZeroMemory( pMac->hHdd, pReport, sizeof( tSirMacRadioMeasureReport ) * (pRRMReq->num_MeasurementRequest - i) ); 
#if defined WLAN_VOWIFI_DEBUG
                  limLog( pMac, LOGE, "%s:%d: Allocated memory for pReport\n", __func__, __LINE__ );
#endif

               }
               pReport[num_report].refused = 1;
               pReport[num_report].type = SIR_MAC_RRM_BEACON_TYPE;
               pReport[num_report].token = pRRMReq->MeasurementRequest[i].measurement_token;
               num_report++;
               continue;
            }
            else
            {
               if( eHAL_STATUS_SUCCESS != palAllocateMemory( pMac->hHdd,
                        (void **) &pCurrentReq,
                        sizeof( *pCurrentReq ) ) )
               {
                  limLog( pMac, LOGP,
                        FL( "Unable to PAL allocate memory during RRM Req processing\n" ));
                  return eSIR_MEM_ALLOC_FAILED;
               }
#if defined WLAN_VOWIFI_DEBUG
               PELOGE(limLog( pMac, LOGE, "%s:%d: Allocated memory for pCurrentReq\n", __func__, __LINE__ );)
#endif
               palZeroMemory( pMac->hHdd, pCurrentReq, sizeof( *pCurrentReq ) );
               pCurrentReq->dialog_token = pRRMReq->DialogToken.token;
               pCurrentReq->token = pRRMReq->MeasurementRequest[i].measurement_token;
               pMac->rrm.rrmPEContext.pCurrentReq = pCurrentReq;
               rrmStatus = rrmProcessBeaconReportReq( pMac, pCurrentReq, &pRRMReq->MeasurementRequest[i], pSessionEntry );
               if (eRRM_SUCCESS != rrmStatus)
               {
                   rrmProcessBeaconRequestFailure(pMac, pSessionEntry, peer, rrmStatus);
                   rrmCleanup(pMac);
               }
            }
            break;            
         default:
            //Send a report with incapabale bit set.
            if ( pReport == NULL ) //Allocate memory to send reports for any subsequent requests.
            {
               if( eHAL_STATUS_SUCCESS != palAllocateMemory( pMac->hHdd,
                        (void **) &pReport,
                        sizeof( tSirMacRadioMeasureReport ) * (pRRMReq->num_MeasurementRequest - i) ) )
               {
                  limLog( pMac, LOGP,
                        FL( "Unable to PAL allocate memory during RRM Req processing\n" ));
                  return eSIR_MEM_ALLOC_FAILED;
               }
               palZeroMemory( pMac->hHdd, pReport, sizeof( tSirMacRadioMeasureReport ) * (pRRMReq->num_MeasurementRequest - i) ); 
#if defined WLAN_VOWIFI_DEBUG
               PELOGE(limLog( pMac, LOGE, "%s:%d: Allocated memory for pReport\n", __func__, __LINE__ );)
#endif

            }
            pReport[num_report].incapable = 1;
            pReport[num_report].type = pRRMReq->MeasurementRequest[i].measurement_type;
            pReport[num_report].token = pRRMReq->MeasurementRequest[i].measurement_token;
            num_report++;
            break;
      }
   }

end:
   if( pReport )
   {
      limSendRadioMeasureReportActionFrame( pMac, pRRMReq->DialogToken.token, num_report, 
            pReport, peer, pSessionEntry );

      palFreeMemory( pMac->hHdd, pReport );
#if defined WLAN_VOWIFI_DEBUG
      PELOGE(limLog( pMac, LOGE, "%s:%d: Free memory for pReport\n", __func__, __LINE__ );)
#endif
   }
   return status;

}

// --------------------------------------------------------------------
/**
 * rrmUpdateStartTSF
 **
 * FUNCTION:  Store start TSF of measurement.
 *
 * LOGIC:
 *
 * ASSUMPTIONS:
 *
 * NOTE:
 *
 * @param startTSF - TSF value at the start of measurement.
 * @return None
 */
void
rrmUpdateStartTSF ( tpAniSirGlobal pMac, tANI_U32 startTSF[2] )
{
#if 0 //defined WLAN_VOWIFI_DEBUG
   limLog( pMac, LOGE, "Update Start TSF = %d %d\n", startTSF[0], startTSF[1] ); 
#endif
   pMac->rrm.rrmPEContext.startTSF[0] = startTSF[0];
   pMac->rrm.rrmPEContext.startTSF[1] = startTSF[1];
}

// --------------------------------------------------------------------
/**
 * rrmGetStartTSF
 *
 * FUNCTION:  Get the Start TSF.
 *
 * LOGIC:
 *
 * ASSUMPTIONS:
 *
 * NOTE:
 *
 * @param startTSF - store star TSF in this buffer.
 * @return txPower
 */
void
rrmGetStartTSF ( tpAniSirGlobal pMac, tANI_U32 *pStartTSF )
{
#if 0 //defined WLAN_VOWIFI_DEBUG
   limLog( pMac, LOGE, "Get the start TSF, TSF = %d %d \n", pMac->rrm.rrmPEContext.startTSF[0], pMac->rrm.rrmPEContext.startTSF[1] ); 
#endif
   pStartTSF[0] = pMac->rrm.rrmPEContext.startTSF[0];
   pStartTSF[1] = pMac->rrm.rrmPEContext.startTSF[1];
   
}
// --------------------------------------------------------------------
/**
 * rrmGetCapabilities
 *
 * FUNCTION:
 * Returns a pointer to tpRRMCaps with all the caps enabled in RRM
 *
 * LOGIC:
 *
 * ASSUMPTIONS:
 *
 * NOTE:
 *
 * @param pSessionEntry
 * @return pointer to tRRMCaps
 */
tpRRMCaps rrmGetCapabilities ( tpAniSirGlobal pMac,
                               tpPESession pSessionEntry )
{
   return &pMac->rrm.rrmPEContext.rrmEnabledCaps;
}

// --------------------------------------------------------------------
/**
 * rrmUpdateConfig
 *
 * FUNCTION:
 * Update the configuration. This is called from limUpdateConfig.
 *
 * LOGIC:
 *
 * ASSUMPTIONS:
 *
 * NOTE:
 *
 * @param pSessionEntry
 * @return pointer to tRRMCaps
 */
void rrmUpdateConfig ( tpAniSirGlobal pMac,
                               tpPESession pSessionEntry )
{      
   tANI_U32 val;
   tpRRMCaps pRRMCaps = &pMac->rrm.rrmPEContext.rrmEnabledCaps;

   if (wlan_cfgGetInt(pMac, WNI_CFG_RRM_ENABLED, &val) != eSIR_SUCCESS)
      limLog(pMac, LOGP, FL("cfg get rrm enabled failed\n"));
   pMac->rrm.rrmPEContext.rrmEnable = (val) ? 1 : 0;    

   if (wlan_cfgGetInt(pMac, WNI_CFG_RRM_OPERATING_CHAN_MAX, &val) != eSIR_SUCCESS)
      limLog(pMac, LOGP, FL("cfg get rrm enabled failed\n"));
   pRRMCaps->operatingChanMax = val;

   if (wlan_cfgGetInt(pMac, WNI_CFG_RRM_NON_OPERATING_CHAN_MAX, &val) != eSIR_SUCCESS)
      limLog(pMac, LOGP, FL("cfg get rrm enabled failed\n"));
   pRRMCaps->nonOperatingChanMax = val;

#if defined WLAN_VOWIFI_DEBUG
   PELOGE(limLog( pMac, LOGE, "RRM enabled = %d  OperatingChanMax = %d  NonOperatingMax = %d\n", pMac->rrm.rrmPEContext.rrmEnable,
                     pRRMCaps->operatingChanMax, pRRMCaps->nonOperatingChanMax );)
#endif
}
// --------------------------------------------------------------------
/**
 * rrmInitialize
 *
 * FUNCTION:
 * Initialize RRM module
 *
 * LOGIC:
 *
 * ASSUMPTIONS:
 *
 * NOTE:
 *
 * @return None
 */

tSirRetStatus
rrmInitialize(tpAniSirGlobal pMac)
{
   tpRRMCaps pRRMCaps = &pMac->rrm.rrmPEContext.rrmEnabledCaps;

   pMac->rrm.rrmPEContext.pCurrentReq = NULL; 
   pMac->rrm.rrmPEContext.txMgmtPower = 0;
   pMac->rrm.rrmPEContext.DialogToken = 0;

   pMac->rrm.rrmPEContext.rrmEnable = 0;

   palZeroMemory( pMac->hHdd, pRRMCaps, sizeof(tRRMCaps) );
   pRRMCaps->LinkMeasurement = 1;
   pRRMCaps->NeighborRpt = 1;
   pRRMCaps->BeaconPassive = 1;
   pRRMCaps->BeaconActive = 1;
   pRRMCaps->BeaconTable = 1;
   pRRMCaps->APChanReport = 1;

   //pRRMCaps->TCMCapability = 1;
   //pRRMCaps->triggeredTCM = 1;
   pRRMCaps->operatingChanMax = 3;
   pRRMCaps->nonOperatingChanMax = 3;

   return eSIR_SUCCESS;
}

// --------------------------------------------------------------------
/**
 * rrmCleanup
 *
 * FUNCTION:
 * cleanup RRM module
 *
 * LOGIC:
 *
 * ASSUMPTIONS:
 *
 * NOTE:
 *
 * @param mode
 * @param rate
 * @return None
 */

tSirRetStatus
rrmCleanup(tpAniSirGlobal pMac)
{
   if( pMac->rrm.rrmPEContext.pCurrentReq )
   {
      if( pMac->rrm.rrmPEContext.pCurrentReq->request.Beacon.reqIes.pElementIds )
      {
         palFreeMemory( pMac->hHdd, pMac->rrm.rrmPEContext.pCurrentReq->request.Beacon.reqIes.pElementIds );
#if defined WLAN_VOWIFI_DEBUG
         PELOGE(limLog( pMac, LOGE, "%s:%d: Free memory for pElementIds\n", __func__, __LINE__ );)
#endif
      }

      palFreeMemory( pMac->hHdd, pMac->rrm.rrmPEContext.pCurrentReq ); 
#if defined WLAN_VOWIFI_DEBUG
      PELOGE(limLog( pMac, LOGE, "%s:%d: Free memory for pCurrentReq\n", __func__, __LINE__ );)
#endif
   }

   pMac->rrm.rrmPEContext.pCurrentReq = NULL; 
   return eSIR_SUCCESS;
}

// --------------------------------------------------------------------
/**
 * rrmProcessMessage
 *
 * FUNCTION:  Processes the next received Radio Resource Management message
 *
 * LOGIC:
 *
 * ASSUMPTIONS:
 *
 * NOTE:
 *
 * @param None
 * @return None
 */

void rrmProcessMessage(tpAniSirGlobal pMac, tpSirMsgQ pMsg)
{
   switch (pMsg->type)
   {
      case eWNI_SME_NEIGHBOR_REPORT_REQ_IND:
         rrmProcessNeighborReportReq( pMac, pMsg->bodyptr );
         break;
      case eWNI_SME_BEACON_REPORT_RESP_XMIT_IND:
         rrmProcessBeaconReportXmit( pMac, pMsg->bodyptr );
         break;
   }

}

#endif
