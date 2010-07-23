/*
 * Airgo Networks, Inc proprietary. All rights reserved.
 * This file lim ProcessMessageQueue.cc contains the code
 * for processing LIM message Queue.
 * Author:        Chandra Modumudi
 * Date:          02/11/02
 * History:-
 * Date           Modified by    Modification Information
 * --------------------------------------------------------------------
 *
 */
#include "palTypes.h"
#include "wniApi.h"
#include "halDataStruct.h"
#ifdef ANI_PRODUCT_TYPE_AP
#include "wniCfgAp.h"
#else
#include "wniCfgSta.h"
#endif
#include "cfgApi.h"
#include "sirCommon.h"
#include "utilsApi.h"
#include "limTypes.h"
#include "limUtils.h"
#include "limAssocUtils.h"
#include "limPropExtsUtils.h"
#include "halCommonApi.h"
#include "limAdmitControl.h"
#include "pmmApi.h"
#include "limIbssPeerMgmt.h"
#include "schApi.h"

#ifdef WMM_APSD
#include "wmmApsd.h"
#endif
#include "pal_skbPoolTracking.h"

#ifdef VOSS_ENABLED
#include "vos_types.h"
#include "vos_packet.h"
#include "vos_memory.h"
#endif

/** -------------------------------------------------------------
\fn defMsgDecision
\brief The function decides whether to defer a message or not in limProcessMessage function
\param   tpAniSirGlobal pMac
\param       tSirMsgQ  limMsg
\param       tSirMacTspecIE   *ppInfo
\return none
  -------------------------------------------------------------*/

tANI_U8 static
defMsgDecision(tpAniSirGlobal pMac, tpSirMsgQ  limMsg)
{
  if((pMac->lim.gLimSmeState == eLIM_SME_SUSPEND_STATE) &&
      (limMsg->type != SIR_LIM_RESUME_ACTIVITY_NTF))
  {
      // Defer processsing this message
      if (limDeferMsg(pMac, limMsg) != TX_SUCCESS)
      {
          limLog(pMac, LOGE, FL("Unable to Defer message %s\n"),
                 limMsgStr(limMsg->type));
          limHandleDeferMsgError(pMac, limMsg);
      }
      return true;
  }

  //When defer is requested then defer all the messages except HAL responses.
  if((!limIsSystemInScanState(pMac)) && (true != GET_LIM_PROCESS_DEFD_MESGS(pMac)) &&
      !pMac->lim.gLimSystemInScanLearnMode)
  {
    if ((limMsg->type != SIR_HAL_ADD_BSS_RSP) &&
        (limMsg->type != SIR_HAL_DELETE_BSS_RSP) &&
        (limMsg->type != SIR_HAL_ADD_STA_RSP) &&
        (limMsg->type != SIR_HAL_DELETE_STA_RSP)&&
        (limMsg->type != SIR_HAL_SET_BSSKEY_RSP)&&
        (limMsg->type != SIR_HAL_SET_STAKEY_RSP)&&
        (limMsg->type != SIR_HAL_SET_STA_BCASTKEY_RSP) &&
        (limMsg->type != SIR_LIM_RESUME_ACTIVITY_NTF)&&
        (limMsg->type != eWNI_SME_START_REQ) &&
        (limMsg->type != SIR_HAL_REMOVE_BSSKEY_RSP) &&
        (limMsg->type != SIR_HAL_REMOVE_STAKEY_RSP) &&
        (limMsg->type != SIR_HAL_SET_MIMOPS_RSP)&&
        (limMsg->type != SIR_HAL_ADDBA_RSP) &&
        (limMsg->type != SIR_HAL_ENTER_BMPS_RSP) &&
        (limMsg->type != SIR_HAL_EXIT_BMPS_RSP) &&
        (limMsg->type != SIR_HAL_ENTER_IMPS_RSP) &&
        (limMsg->type != SIR_HAL_EXIT_IMPS_RSP) &&
        (limMsg->type != SIR_HAL_ENTER_UAPSD_RSP) &&
        (limMsg->type != SIR_HAL_EXIT_UAPSD_RSP) &&
        (limMsg->type != SIR_HAL_WOWL_ENTER_RSP) &&
        (limMsg->type != SIR_HAL_WOWL_EXIT_RSP) &&
        (limMsg->type != SIR_HAL_SWITCH_CHANNEL_RSP) &&
        (limMsg->type != SIR_HAL_ADD_TS_RSP))
    {
        PELOG1(limLog(pMac, LOG1, FL("Defer the current message %s , gLimProcessDefdMsgs is false and system is not in scan/learn mode\n"),
               limMsgStr(limMsg->type));)

        // Defer processsing this message
        if (limDeferMsg(pMac, limMsg) != TX_SUCCESS)
        {
            limLog(pMac, LOGE, FL("Unable to Defer message %s\n"),
                   limMsgStr(limMsg->type));
            limHandleDeferMsgError(pMac, limMsg);

        }
        return true;
    }
  }
  return false;
}

static void
__limHandleBeacon(tpAniSirGlobal pMac, tpSirMsgQ pMsg)
{
    tLimSmeStates state = limGetSmeState(pMac);

    if ((state == eLIM_SME_LINK_EST_STATE) ||
            (state == eLIM_SME_NORMAL_STATE))
        schProcessMessage(pMac, pMsg);
    else if (state != eLIM_SME_IDLE_STATE)
    {
        tANI_U32 *pBD;
        limGetBDfromRxPacket(pMac, pMsg->bodyptr, &pBD);
        limProcessBeaconFrame(pMac, pBD);
    }
    else /** Drop the Beacons in IDLE State*/
        return;
}


//Fucntion prototype
void limProcessNormalHddMsg(tpAniSirGlobal pMac, tSirMsgQ *pLimMsg, tANI_U8 fRspReqd);

/**
 * limProcessMessageQueue
 *
 *FUNCTION:
 * This function is called by LIM thread entry function. This
 * function fetches messages posted to the message queue
 * limMsgQ.
 *
 *LOGIC:
 *
 *ASSUMPTIONS:
 * NA
 *
 *NOTE:
 * NA
 *
 * @param  pMac - Pointer to Global MAC structure
 * @return None
 */

void
limProcessMessageQueue(tpAniSirGlobal pMac)
{

#ifndef ANI_MANF_DIAG
    tSirMsgQ  limMsg = { 0, 0, 0 };
#endif
	if(pMac->gDriverType == eDRIVER_TYPE_MFG)
	{
	    return;
	}

#if defined(ANI_OS_TYPE_RTAI_LINUX)
    ULONG param;
    while(get_timer_event(LIM_TIMER_EXPIRY_LIST,&param))
    {
        limMsg.type = (tANI_U16) param;
        limMsg.bodyval = 0;
        limMsg.bodyptr = NULL;
        limMessageProcessor(pMac, &limMsg);
    }
#endif

#ifndef ANI_MANF_DIAG

    if (tx_queue_receive( &pMac->sys.gSirLimMsgQ, (void *) &limMsg, TX_WAIT_FOREVER)
            == TX_SUCCESS)
    {
        PELOG3(limLog(pMac, LOG3, FL("LIM Received message %s\n"), limMsgStr(limMsg.type));)
        limPrintMsgInfo(pMac, LOG3, &limMsg);
        limMessageProcessor(pMac, &limMsg);
    } // if (tx_queue_receive)
#endif
    
} /*** end limProcessMessageQueue() ***/



/**
 * limDeferMsg()
 *
 *FUNCTION:
 * This function is called to defer the messages received
 * during Learn mode
 *
 *LOGIC:
 * NA
 *
 *ASSUMPTIONS:
 * NA
 *
 *NOTE:
 * NA
 *
 * @param  pMac - Pointer to Global MAC structure
 * @param  pMsg of type tSirMsgQ - Pointer to the message structure
 * @return None
 */

tANI_U32
limDeferMsg(tpAniSirGlobal pMac, tSirMsgQ *pMsg)
{
    tANI_U32 retCode = TX_SUCCESS;
#if defined(ANI_OS_TYPE_LINUX) || defined(ANI_OS_TYPE_OSX)
   PELOG3(limLog(pMac, LOG3, FL("Deferring message %X in Learn mode\n"),
           pMsg->type);
    limPrintMsgName(pMac, LOG3, pMsg->type);)
    retCode = tx_queue_send(&pMac->sys.gSirLimDeferredMsgQ,
                            pMsg,
                            TX_NO_WAIT);
    if (retCode == TX_SUCCESS)
        pMac->lim.gLimNumDeferredMsgs++;
#else

        retCode = limWriteDeferredMsgQ(pMac, pMsg);

#endif
    if(retCode == TX_SUCCESS)
        {
            MTRACE(macTraceMsgRx(pMac, 0, LIM_TRACE_MAKE_RXMSG(pMsg->type, LIM_MSG_DEFERRED));)
        }
    else
        {
            MTRACE(macTraceMsgRx(pMac, 0, LIM_TRACE_MAKE_RXMSG(pMsg->type, LIM_MSG_DROPPED));)
        }


    return retCode;
} /*** end limDeferMsg() ***/



/**
 * limHandleFramesInScanState()
 *
 *FUNCTION:
 * This function is called to process 802.11 frames
 * received by LIM in scan state.
 *
 *LOGIC:
 * NA
 *
 *ASSUMPTIONS:
 * NA
 *
 *NOTE:
 * NA
 *
 * @param  pMac    - Pointer to Global MAC structure
 * @param  limMsg  - Received message
 * @param  pBD     - Pointer to the received Buffer Descriptor+payload
 * @param  deferMsg - Indicates whether the frame shall be deferred
 * @return None
 */

static void
limHandleFramesInScanState(tpAniSirGlobal pMac, tpSirMsgQ limMsg, tANI_U32 *pBD, tANI_U8 *deferMsg)
{
    tSirMacFrameCtl  fc;
    tpSirMacMgmtHdr  pHdr;
#if (WNI_POLARIS_FW_PACKAGE == ADVANCED) && defined(ANI_PRODUCT_TYPE_AP)
    tANI_U32         ignore = 0;
    tSirMacAddr      bssIdRcv;
#endif

    *deferMsg = false;
    pHdr = SIR_MAC_BD_TO_MPDUHEADER(pBD);
    fc = pHdr->fc;
    limLog( pMac, LOG2, FL("ProtVersion %d, Type %d, Subtype %d\n"),
            fc.protVer, fc.type, fc.subType );

#if (WNI_POLARIS_FW_PACKAGE == ADVANCED) && defined(ANI_PRODUCT_TYPE_AP)
    // System is in DFS (Learn) mode
    pMac->lim.numLearn++;

    // Process all BDs and extract PHY stats
    limGetBssidFromBD(pMac, (tpHalBufDesc) pBD, bssIdRcv, &ignore);

    if ((pMac->lim.gLimSystemRole == eLIM_AP_ROLE) &&
        palEqualMemory( pMac->hHdd,bssIdRcv, pMac->lim.gLimBssid, sizeof(tSirMacAddr)))
    {
        /**
                   * Frame from current BSS. Defer processing of
                   * Disassociation/Deauthentication from
                   * STAs that are currently associated.
                   * Collect stats for other received frames.
                   */
        if ((fc.subType == SIR_MAC_MGMT_DISASSOC) ||
            (fc.subType == SIR_MAC_MGMT_DEAUTH))
        {
            if (limDeferMsg(pMac, limMsg) != TX_SUCCESS)
            {
                limLog(pMac, LOGE, FL("Unable to Defer message %X\n"),
                       limMsg->type);
                limPrintMsgName(pMac, LOGE, limMsg->type);
                limPktFree(pMac, HAL_TXRX_FRM_802_11_MGMT, pBD, limMsg->bodyptr);
            }
            return;
        }
        pMac->lim.numLearnIgnore++;
    }
    else
    {
        // Frame received from other BSS
        if (fc.type == SIR_MAC_DATA_FRAME && pMac->lim.gLimSystemRole == eLIM_AP_ROLE)
        {
            /**
              * Data Frame from neighbor BSS.
              * Extract neighbor BSS info as much as possible.
              */
            limCollectMeasurementData(pMac, pBD, NULL);
        }
        else if ((fc.type == SIR_MAC_MGMT_FRAME) &&
                 (fc.subType == SIR_MAC_MGMT_BEACON))
            limProcessBeaconFrame(pMac, pBD);
        else if ((fc.type == SIR_MAC_MGMT_FRAME) &&
                 (fc.subType == SIR_MAC_MGMT_PROBE_RSP))
            limProcessProbeRspFrame(pMac, pBD);
    }

#else
    // defer all message in scan state except for Beacons and Probe Response
    if ((fc.type == SIR_MAC_MGMT_FRAME) &&
        (fc.subType == SIR_MAC_MGMT_BEACON))
        limProcessBeaconFrame(pMac, pBD);
    else if ((fc.type == SIR_MAC_MGMT_FRAME) &&
             (fc.subType == SIR_MAC_MGMT_PROBE_RSP))
        limProcessProbeRspFrame(pMac, pBD);
    else 
    {
        *deferMsg = true;
        return; 
    }
 
#endif
    limPktFree(pMac, HAL_TXRX_FRM_802_11_MGMT, pBD, (void *) limMsg->bodyptr);
    return;

} /*** end limHandleFramesInScanState() ***/

/** ------------------------------------------------------------
\brief    This function handles Unknown Unicast (A2 Index)
\         packets.
\param    tpAniSirGlobal pMac Global Mac data structure
\param    void *pBd Pointer to Buffer Descriptor
\return   none
\
\ -------------------------------------------------------------- */
static void limHandleUnknownA2IndexFrames(tpAniSirGlobal pMac, void *pBd)
{
#ifndef ANI_CHIPSET_VOLANS
    tpSirMacDataHdr3a pMacHdr;

    /** This prevents from disassoc/deauth being sent in a burst,
        and gLimDisassocFrameCredit is reset for every 10 seconds.*/
    if (pMac->lim.gLimDisassocFrameCredit > pMac->lim.gLimDisassocFrameThreshold)
        return;

    pMac->lim.gLimDisassocFrameCredit++;

    pMacHdr = SIR_MAC_BD_TO_MPDUHEADER3A(pBd);

    if (limIsGroupAddr(pMacHdr->addr2))
    {
        PELOG2(limLog(pMac, LOG2, FL("Ignoring A2 Invalid Packet received for MC/BC:\n"));
        limPrintMacAddr(pMac, pMacHdr->addr2, LOG2);)

        return;
    }

    if ((pMac->lim.gLimSystemRole == eLIM_AP_ROLE) &&
        (pMac->lim.gLimMlmState == eLIM_MLM_BSS_STARTED_STATE))
    {
        switch (pMacHdr->fc.type)
        {
            case SIR_MAC_MGMT_FRAME:
                switch (pMacHdr->fc.subType)
                {
                    case SIR_MAC_MGMT_ACTION:
                        // Send Disassociation frame to
                        // sender if role is AP
                        PELOG1(limLog(pMac, LOG1, FL("Send Disassoc Frame due to Invalid Addr2 packet"));
                        limPrintMacAddr(pMac, pMacHdr->addr2, LOG1);)
                        limSendDisassocMgmtFrame(pMac,
                           eSIR_MAC_CLASS3_FRAME_FROM_NON_ASSOC_STA_REASON, pMacHdr->addr2);
                        break;

                    default:
                        break;

                }
                break;

            case SIR_MAC_CTRL_FRAME:
                switch (pMacHdr->fc.subType)
                {
                    case SIR_MAC_CTRL_PS_POLL:
                    case SIR_MAC_CTRL_BAR:
                        // Send Disassociation frame to
                        // sender if role is AP
                        PELOG1(limLog(pMac, LOG1, FL("Send Disassoc Frame due to Invalid Addr2 packet"));
                        limPrintMacAddr(pMac, pMacHdr->addr2, LOG1);)
                        limSendDisassocMgmtFrame(pMac,
                           eSIR_MAC_CLASS3_FRAME_FROM_NON_ASSOC_STA_REASON, pMacHdr->addr2);
                        break;

                    default:
                        break;
                }
                break;

            case SIR_MAC_DATA_FRAME:
                switch (pMacHdr->fc.subType)
                {
                    case SIR_MAC_DATA_NULL:
                    case SIR_MAC_DATA_QOS_NULL:
                        // Send Disassociation frame to
                        // sender if role is AP
                        PELOG1(limLog(pMac, LOG1, FL("Send Disassoc Frame due to Invalid Addr2 packet"));
                        limPrintMacAddr(pMac, pMacHdr->addr2, LOG1);)
                        limSendDisassocMgmtFrame(pMac,
                           eSIR_MAC_CLASS3_FRAME_FROM_NON_ASSOC_STA_REASON, pMacHdr->addr2);
                        break;

                    default:
                        // Send Deauthentication frame to
                        // sender if role is AP
                        PELOG1(limLog(pMac, LOG1, FL("Sending Deauth frame due to Invalid Addr2 packet"));
                        limPrintMacAddr(pMac, pMacHdr->addr2, LOG1);)
                        limSendDeauthMgmtFrame(pMac,
                           eSIR_MAC_CLASS3_FRAME_FROM_NON_ASSOC_STA_REASON, pMacHdr->addr2);
                        break;
                }
                break;
        }
    }
#else
      /* addr2 mismatch interrupt occured this means previous 
       disassociation was not successful
       In Volans pBd only contains pointer 48-bit address2 field */
       /*Send disassociation message again*/
       limSendDisassocMgmtFrame(pMac, eSIR_MAC_CLASS3_FRAME_FROM_NON_ASSOC_STA_REASON,(tANI_U8 *) pBd);
#endif

    return;
}

/**
 * limHandle80211Frames()
 *
 *FUNCTION:
 * This function is called to process 802.11 frames
 * received by LIM.
 *
 *LOGIC:
 * NA
 *
 *ASSUMPTIONS:
 * NA
 *
 *NOTE:
 * NA
 *
 * @param  pMac - Pointer to Global MAC structure
 * @param  pMsg of type tSirMsgQ - Pointer to the message structure
 * @return None
 */

static void 
limHandle80211Frames(tpAniSirGlobal pMac, tpSirMsgQ limMsg, tANI_U8 *pDeferMsg)
{
    tANI_U32         *pBD;
    tSirMacFrameCtl  fc;
    tANI_U32         len = sizeof(tSirMacAddr);
    tSirMacAddr      bssId;
    tpSirMacMgmtHdr    pHdr;

    *pDeferMsg= false;
    limGetBDfromRxPacket(pMac, limMsg->bodyptr, &pBD);

    pHdr = SIR_MAC_BD_TO_MPDUHEADER(pBD);
    fc = pHdr->fc;
    limLog( pMac, LOG2, FL("ProtVersion %d, Type %d, Subtype %d\n"),
            fc.protVer, fc.type, fc.subType );

#ifdef ANI_PRODUCT_TYPE_AP
    if ((pMac->lim.gLimSystemRole == eLIM_AP_ROLE) && (LIM_IS_RADAR_DETECTED(pMac)))
    {
        PELOGW(limLog(pMac, LOGW, FL("Droping the received packets as radar is detected\n"));)
        limPktFree(pMac, HAL_TXRX_FRM_802_11_MGMT, pBD, (void *) limMsg->bodyptr);
        return;
    }
#endif

    if (fc.protVer != SIR_MAC_PROTOCOL_VERSION)
    {   // Received Frame with non-zero Protocol Version
        limLog(pMac, LOGE, FL("Unexpected frame with protVersion %d received\n"),
           fc.protVer);
        limPktFree(pMac, HAL_TXRX_FRM_802_11_MGMT, pBD, (void *) limMsg->bodyptr);
#ifdef WLAN_DEBUG            
        pMac->lim.numProtErr++;
#endif
        return;
    }

    if (wlan_cfgGetStr(pMac, WNI_CFG_BSSID, bssId, &len) != eSIR_SUCCESS)
    {
        limPktFree(pMac, HAL_TXRX_FRM_802_11_MGMT, pBD, (void *) limMsg->bodyptr);
        limLog(pMac, LOGP, FL("could not retrive BSSID\n"));
        return;
    }

    if (limIsSystemInScanState(pMac))
    {
        limHandleFramesInScanState(pMac, limMsg, pBD, pDeferMsg);
        return;
    }

    if (pMac->lim.gLimSystemRole == eLIM_UNKNOWN_ROLE) {
        limLog( pMac, LOGW, FL( "gLimSystemRole is %d. Exiting...\n" ),
              pMac->lim.gLimSystemRole );
        limPktFree(pMac, HAL_TXRX_FRM_802_11_MGMT, pBD, (void *) limMsg->bodyptr);

#ifdef WLAN_DEBUG        
        pMac->lim.numProtErr++;
#endif
        return;
    }

#ifdef WLAN_DEBUG    
    pMac->lim.numMAC[fc.type][fc.subType]++;
#endif
    
    switch (fc.type)
    {
        case SIR_MAC_MGMT_FRAME: {

            if (limIsReassocInProgress( pMac) && (fc.subType != SIR_MAC_MGMT_DISASSOC) &&
                                                (fc.subType != SIR_MAC_MGMT_DEAUTH) && (fc.subType != SIR_MAC_MGMT_REASSOC_RSP)) {
                    limLog(pMac, LOGE, FL("Frame with Type - %d, Subtype - %d received in ReAssoc Wait state, dropping...\n"),
                                                                fc.type, fc.subType);
                    return;
            }
            // Received Management frame
            switch (fc.subType)
            {
                case SIR_MAC_MGMT_ASSOC_REQ:
#ifdef ANI_PRODUCT_TYPE_AP
                    if (pMac->lim.gLimSystemRole == eLIM_AP_ROLE)
                        limProcessAssocReqFrame(pMac, pBD, LIM_ASSOC);
                    else
#endif
                    {
                        // Unwanted messages - Log error
                        limLog(pMac, LOGE, FL("unexpected message received %X\n"),
                               limMsg->type);
                        limPrintMsgName(pMac, LOGE, limMsg->type);
                    }
                    break;

                case SIR_MAC_MGMT_ASSOC_RSP:
                    limProcessAssocRspFrame(pMac, pBD, LIM_ASSOC);
                    break;

                case SIR_MAC_MGMT_REASSOC_REQ:
#ifdef ANI_PRODUCT_TYPE_AP
                    // Do not support Reassociation in STA image
                    if (pMac->lim.gLimSystemRole == eLIM_AP_ROLE)
                        limProcessAssocReqFrame(pMac, pBD, LIM_REASSOC);
                    else
#endif
                    {
                        // Unwanted messages - Log error
                        limLog(pMac, LOGE, FL("unexpected message received %X\n"),
                                                                    limMsg->type);
                        limPrintMsgName(pMac, LOGE, limMsg->type);
                    }
                    break;

                case SIR_MAC_MGMT_REASSOC_RSP:
                    limProcessAssocRspFrame(pMac, pBD, LIM_REASSOC);
                    break;

                case SIR_MAC_MGMT_PROBE_REQ:
                    limProcessProbeReqFrame(pMac, pBD);
                    break;

                case SIR_MAC_MGMT_PROBE_RSP:
                    limProcessProbeRspFrame(pMac, pBD);
                    break;

                case SIR_MAC_MGMT_BEACON:
                    __limHandleBeacon(pMac, limMsg);
                    break;

                case SIR_MAC_MGMT_DISASSOC:
                    limProcessDisassocFrame(pMac, pBD);
                    break;

                case SIR_MAC_MGMT_AUTH:
                    limProcessAuthFrame(pMac, pBD);
                    break;

                case SIR_MAC_MGMT_DEAUTH:
                    limProcessDeauthFrame(pMac, pBD);
                    break;

                case SIR_MAC_MGMT_ACTION:
                    if (SIR_MAC_BD_IS_UNKNOWN_UCAST_FRAME(pBD))
                        limHandleUnknownA2IndexFrames(pMac, pBD);
                    else
                        limProcessActionFrame(pMac, pBD);
                    break;

                default:
                    // Received Management frame of 'reserved' subtype
                    break;
            } // switch (fc.subType)

        }
        break;

        default:
            // Received frame of type 'reserved'
            break;

    } // switch (fc.type)

    limPktFree(pMac, HAL_TXRX_FRM_802_11_MGMT, pBD, (void *) limMsg->bodyptr) ;
	return;
} /*** end limHandle80211Frames() ***/


/**
 * limProcessAbortScanInd()
 *
 *FUNCTION:
 * This function is called from HDD to abort the scan which is presently being run
 *
 *
 *NOTE:
 *
 * @param  pMac      Pointer to Global MAC structure
 * @param  *pMsgBuf  A pointer to the SME message buffer
 * @return None
 */
void
limProcessAbortScanInd(tpAniSirGlobal pMac)
{
#ifdef FEATURE_WLAN_DIAG_SUPPORT 
    limDiagEventReport(pMac, WLAN_PE_DIAG_SCAN_ABORT_IND_EVENT, NULL, 0, 0);
#endif //FEATURE_WLAN_DIAG_SUPPORT
	
	/* Deactivate the gLimBackgroundScanTimer as part of the abort scan.
     * SME should send WNI_CFG_BACKGROUND_SCAN_PERIOD indication 
     * to start the background scan again
     */
    PELOGE(limLog(pMac, LOGE, FL("Processing AbortScan Ind\n"));)

    limAbortBackgroundScan(pMac);

    /* Abort the scan if its running, else just return */
    if(limIsSystemInScanState(pMac))
    {
        if( (eLIM_HAL_INIT_SCAN_WAIT_STATE == pMac->lim.gLimHalScanState ) ||
            (eLIM_HAL_START_SCAN_WAIT_STATE == pMac->lim.gLimHalScanState ) ||
            (eLIM_HAL_END_SCAN_WAIT_STATE == pMac->lim.gLimHalScanState ) ||
            (eLIM_HAL_FINISH_SCAN_WAIT_STATE == pMac->lim.gLimHalScanState) )
        {
            //Simply signal we need to abort
            limLog( pMac, LOGW, FL(" waiting for HAL, simply signal abort gLimHalScanState = %d\n"), pMac->lim.gLimHalScanState );
            pMac->lim.abortScan = 1;
        }
        else
        {
            //Force abort
            limLog( pMac, LOGW, FL(" Force aborting scan\n") );
            pMac->lim.abortScan = 0;
            limDeactivateAndChangeTimer(pMac, eLIM_MIN_CHANNEL_TIMER);
            limDeactivateAndChangeTimer(pMac, eLIM_MAX_CHANNEL_TIMER);
            limSendHalFinishScanReq(pMac, eLIM_HAL_FINISH_SCAN_WAIT_STATE);
        }
    }
    return;
}

/**
 * limMessageProcessor
 *
 *FUNCTION:
 * Wrapper function for limProcessMessages when handling messages received by LIM.
 * Could either defer messages or process them.
 * @param  pMac   Pointer to Global MAC structure
 * @param  limMsg Received LIM message
 * @return None
 */

void limMessageProcessor(tpAniSirGlobal pMac, tpSirMsgQ limMsg)
{
    if (pMac->lim.gLimMlmState == eLIM_MLM_OFFLINE_STATE)
	{
        if (limMsg->bodyptr != NULL)
            palFreeMemory(pMac->hHdd, (tANI_U8 *)limMsg->bodyptr);
        return;
	}

    if (!defMsgDecision(pMac, limMsg))
    {
        limProcessMessages(pMac, limMsg);
#ifdef ANI_PRODUCT_TYPE_CLIENT
        // process deferred message queue if allowed
        {
            if ( (! (pMac->lim.gLimAddtsSent))
                            &&
                    (! (limIsSystemInScanState(pMac)))
                )
            {
                if (true == GET_LIM_PROCESS_DEFD_MESGS(pMac))
                  limProcessDeferredMessageQueue(pMac);
            }
        }
#else
        {
        // process deferred message queue if allowed
            if (! (pMac->lim.gLimSystemInScanLearnMode))
            {
#if defined(ANI_AP_CLIENT_SDK)
                if (pMac->lim.gLimSystemRole != eLIM_AP_ROLE && (pMac->lim.gLimAddtsSent))
                    return;
#endif

                if (true == GET_LIM_PROCESS_DEFD_MESGS(pMac))
                        limProcessDeferredMessageQueue(pMac);
            }
        }
#endif
    }
}

/**
 * limProcessMessages
 *
 *FUNCTION:
 * This function is called by limProcessMessageQueue function. This
 * function processes messages received by LIM.
 *
 *LOGIC:
 * Depending on the message type, corresponding function will be
 * called, for example limProcessSmeMessages() will be called to
 * process SME messages received from HDD/Upper layer software module.
 *
 *ASSUMPTIONS:
 * NA
 *
 *NOTE:
 * NA
 *
 * @param  pMac   Pointer to Global MAC structure
 * @param  limMsg Received LIM message
 * @return None
 */

void
limProcessMessages(tpAniSirGlobal pMac, tpSirMsgQ  limMsg)
{
    tANI_U8  deferMsg = false;
#if defined(ANI_DVT_DEBUG)
    tSirMsgQ  msgQ;
#endif
	if(pMac->gDriverType == eDRIVER_TYPE_MFG)
	{
    	return;
	}
#ifdef WLAN_DEBUG    
    pMac->lim.numTot++;
#endif


   PELOG3(limLog(pMac, LOG3, FL("rcvd msgType = %s, sme state = %s, mlm state = %s\n"),
      limMsgStr(limMsg->type), limSmeStateStr(pMac->lim.gLimSmeState),
      limMlmStateStr(pMac->lim.gLimMlmState));)

    MTRACE(macTraceMsgRx(pMac, 0, LIM_TRACE_MAKE_RXMSG(limMsg->type, LIM_MSG_PROCESSED));)


    switch (limMsg->type)
    {
#if defined(ANI_DVT_DEBUG)
        case SIR_LIM_SUSPEND_ACTIVITY_REQ:
            // This message is from HAL notifying LIM
            // to suspend activity. (PTT needs)
            // Disable TFP & RHP
            //halSetStaTxEnable(pMac, 1, eHAL_CLEAR);
            //halStopDataTraffic(pMac);
            //halSetRxEnable(pMac, eHAL_CLEAR);

            pMac->lim.gLimPrevSmeState = pMac->lim.gLimSmeState;
            pMac->lim.gLimSmeState     = eLIM_SME_SUSPEND_STATE;
         MTRACE(macTrace(pMac, TRACE_CODE_SME_STATE, 0, pMac->lim.gLimSmeState));

            // Post message back to HAL
            msgQ.type = SIR_HAL_SUSPEND_ACTIVITY_RSP;
            MTRACE(macTraceMsgTx(pMac, 0, msgQ.type));
            halPostMsgApi(pMac, &msgQ);
            break;
#endif

        case SIR_LIM_RESUME_ACTIVITY_NTF:
            // This message is from HAL notifying LIM
            // to resume activity.
            if (pMac->lim.gLimSmeState == eLIM_SME_SUSPEND_STATE)
            {
                limLog(pMac, LOGE,
                   FL("Received RESUME_NTF in State %s on Role %d\n"),
                   limSmeStateStr(pMac->lim.gLimSmeState), pMac->lim.gLimSystemRole);
                pMac->lim.gLimSmeState = pMac->lim.gLimPrevSmeState;
             MTRACE(macTrace(pMac, TRACE_CODE_SME_STATE, 0, pMac->lim.gLimSmeState));

                 handleCBCFGChange( pMac, ANI_IGNORE_CFG_ID );
                 handleHTCapabilityandHTInfo(pMac);
                 //initialize the TSPEC admission control table.
                 limAdmitControlInit(pMac);
                 limRegisterHalIndCallBack(pMac);
            }
            else
            {
                limLog(pMac, LOGE,
                   FL("Received RESUME_NTF in inval State %X on Role %d\n"),
                   pMac->lim.gLimSmeState, pMac->lim.gLimSystemRole);
                limPrintSmeState(pMac, LOGE, pMac->lim.gLimSmeState);
            }

            break;

        case SIR_CFG_PARAM_UPDATE_IND:
            /// CFG parameter updated
            if (limIsSystemInScanState(pMac))
            {
                // System is in DFS (Learn) mode
                // Defer processsing this message
                if (limDeferMsg(pMac, limMsg) != TX_SUCCESS)
                {
                    limLog(pMac, LOGE, FL("Unable to Defer message %X\n"),
                           limMsg->type);
                    limPrintMsgName(pMac, LOGE, limMsg->type);
                }
            }
            else
            {
                limHandleCFGparamUpdate(pMac, limMsg->bodyval);
            }

            break;

        case SIR_HAL_INIT_SCAN_RSP:
            limProcessInitScanRsp(pMac, limMsg->bodyptr);
            break;

        case SIR_HAL_START_SCAN_RSP:
            limProcessStartScanRsp(pMac, limMsg->bodyptr);
            break;

        case SIR_HAL_END_SCAN_RSP:
            limProcessEndScanRsp(pMac, limMsg->bodyptr);
            break;

        case SIR_HAL_FINISH_SCAN_RSP:
            limProcessFinishScanRsp(pMac, limMsg->bodyptr);
            break;

        case SIR_HAL_SWITCH_CHANNEL_RSP:
            limProcessSwitchChannelRsp(pMac, limMsg->bodyptr);
            break;
            
#ifdef ANI_SIR_IBSS_PEER_CACHING
        case SIR_HAL_IBSS_STA_ADD:
            limIbssStaAdd(pMac, limMsg->bodyptr);
            break;
#endif
        case SIR_BB_XPORT_MGMT_MSG:
            // These messages are from Peer MAC entity.
#ifdef WLAN_DEBUG                
            pMac->lim.numBbt++;
#endif

#ifdef VOSS_ENABLED
            {
                v_U16_t     pktLen = 0;
                vos_pkt_t  *pVosPkt;
                VOS_STATUS  vosStatus;
                tSirMsgQ    limMsgNew;

                /* The original limMsg which we were deferring have the 
                 * bodyPointer point to �BD� instead of �Vos pkt�. If we don't make a copy
                 * of limMsg, then vos_pkt_peek_data will overwrite the limMsg->bodyPointer. 
                 * and next time when we try to process the msg, we will try to use �BD� as 
                 * �Vos Pkt� which will cause a crash
                 */
                palCopyMemory(pMac, (tANI_U8*)&limMsgNew, (tANI_U8*)limMsg, sizeof(tSirMsgQ));
                pVosPkt = (vos_pkt_t *)limMsgNew.bodyptr;
                vos_pkt_get_packet_length(pVosPkt, &pktLen);
                vosStatus = vos_pkt_peek_data( pVosPkt, 0, (v_PVOID_t *)&limMsgNew.bodyptr, WLANHAL_RX_BD_HEADER_SIZE );

                if( !VOS_IS_STATUS_SUCCESS(vosStatus) )
                {
                    vos_pkt_return_packet(pVosPkt);
                    break;
                }

                limHandle80211Frames(pMac, &limMsgNew, &deferMsg);

                if ( deferMsg == true )
                {
                    PELOG1(limLog(pMac, LOG1, FL("Defer message type=%X \n"), limMsg->type);)
                        if (limDeferMsg(pMac, limMsg) != TX_SUCCESS)
                        {
                            limLog(pMac, LOGE, FL("Unable to Defer message %X\n"), limMsg->type);
                            limPrintMsgName(pMac, LOGE, limMsg->type);
                            vos_pkt_return_packet(pVosPkt);
                        }
                }
                else
                {
                    /* PE is not deferring this 802.11 frame so we need to call vos_pkt_return. 
                     * Asumption here is when Rx mgmt frame processing is done,
                     * voss packet could be freed here.
                     */
                    vos_pkt_return_packet(pVosPkt);
                }
            }
#else
            limHandle80211Frames(pMac, limMsg);
#endif
            break;

        case eWNI_SME_SCAN_REQ:
        case eWNI_SME_DISASSOC_REQ:
        case eWNI_SME_DEAUTH_REQ:
        case eWNI_SME_STA_STAT_REQ:
        case eWNI_SME_AGGR_STAT_REQ:
        case eWNI_SME_GLOBAL_STAT_REQ:
        case eWNI_SME_STAT_SUMM_REQ:
        case eWNI_SME_GET_SCANNED_CHANNEL_REQ:
        case eWNI_SME_GET_STATISTICS_REQ:
            // These messages are from HDD
            limProcessNormalHddMsg(pMac, limMsg, true);  //need to response to hdd
            break;

        case eWNI_SME_SCAN_ABORT_IND:
            vos_mem_free((v_VOID_t *)limMsg->bodyptr);
            limMsg->bodyptr = NULL;
            limProcessAbortScanInd(pMac);
            break;

        case eWNI_SME_START_REQ:
        case eWNI_SME_SYS_READY_IND:
#ifndef WNI_ASKEY_NON_SUPPORT_FEATURE
        case eWNI_SME_JOIN_REQ:
#endif
        case eWNI_SME_AUTH_REQ:
        case eWNI_SME_REASSOC_REQ:
        case eWNI_SME_START_BSS_REQ:
        case eWNI_SME_STOP_BSS_REQ:
#ifndef WNI_ASKEY_NON_SUPPORT_FEATURE
        case eWNI_SME_DEFINE_QOS_REQ:
        case eWNI_SME_DELETE_QOS_REQ:
#endif
        case eWNI_SME_SWITCH_CHL_REQ:
        case eWNI_SME_SWITCH_CHL_CB_PRIMARY_REQ:
        case eWNI_SME_SWITCH_CHL_CB_SECONDARY_REQ:
        case eWNI_SME_SETCONTEXT_REQ:
        case eWNI_SME_REMOVEKEY_REQ:
#ifndef WNI_ASKEY_NON_SUPPORT_FEATURE
        case eWNI_SME_LINK_TEST_START_REQ:
        case eWNI_SME_LINK_TEST_STOP_REQ:
        case eWNI_SME_PROMISCUOUS_MODE_REQ:
#endif
        case eWNI_SME_DISASSOC_CNF:
        case eWNI_SME_DEAUTH_CNF:
        case eWNI_SME_ASSOC_CNF:
        case eWNI_SME_REASSOC_CNF:
        case eWNI_SME_ADDTS_REQ:
        case eWNI_SME_DELTS_REQ:
        case eWNI_SME_DEL_BA_PEER_IND:
        case eWNI_SME_SET_TX_POWER_REQ:
        case eWNI_SME_GET_TX_POWER_REQ:
        case eWNI_SME_GET_NOISE_REQ:
            // These messages are from HDD
            limProcessNormalHddMsg(pMac, limMsg, false);   //no need to response to hdd
            break;

        //Power Save Messages From HDD
        case eWNI_PMC_PWR_SAVE_CFG:
        case eWNI_PMC_ENTER_BMPS_REQ:
        case eWNI_PMC_EXIT_BMPS_REQ:
        case eWNI_PMC_ENTER_IMPS_REQ:
        case eWNI_PMC_EXIT_IMPS_REQ:
        case eWNI_PMC_ENTER_UAPSD_REQ:
        case eWNI_PMC_EXIT_UAPSD_REQ:
        case eWNI_PMC_ENTER_WOWL_REQ:
        case eWNI_PMC_EXIT_WOWL_REQ:
        case eWNI_PMC_WOWL_ADD_BCAST_PTRN:
        case eWNI_PMC_WOWL_DEL_BCAST_PTRN:
            pmmProcessMessage(pMac, limMsg);
            break;

        case eWNI_PMC_SMPS_STATE_IND :
#if 0
        {
            tSirMbMsg *pMBMsg;
            tSirMacHTMIMOPowerSaveState mimoPSstate;
            /** Is System processing any SMPS Indication*/
            if (!limIsSystemInSetMimopsState(pMac))
            {
                pMBMsg = (tSirMbMsg *)limMsg->bodyptr;
                palCopyMemory(pMac->hHdd, &mimoPSstate, pMBMsg->data, sizeof(tSirMacHTMIMOPowerSaveState));
                limSMPowerSaveStateInd(pMac, mimoPSstate);
                palFreeMemory(pMac->hHdd, (tANI_U8 *)limMsg->bodyptr);
            }
            else
            {
                if (limDeferMsg(pMac, limMsg) != TX_SUCCESS)
                {
                    PELOGE(limLog(pMac, LOGE, FL("Unable to Defer message %x\n"), limMsg->type);)
                    limPrintMsgName(pMac, LOGE, limMsg->type);
                    palFreeMemory(pMac->hHdd, (tANI_U8 *)limMsg->bodyptr);
                }
            }
        }
#endif
            break;


        //Power Save Related Messages From HAL
        case SIR_HAL_ENTER_BMPS_RSP:
        case SIR_HAL_EXIT_BMPS_RSP:
        case SIR_HAL_EXIT_BMPS_IND:
        case SIR_HAL_ENTER_IMPS_RSP:
        case SIR_HAL_EXIT_IMPS_RSP:
        case SIR_HAL_ENTER_UAPSD_RSP:
        case SIR_HAL_EXIT_UAPSD_RSP:
        case SIR_HAL_WOWL_ENTER_RSP:
        case SIR_HAL_WOWL_EXIT_RSP:
            pmmProcessMessage(pMac, limMsg);
            break;

        case SIR_HAL_LOW_RSSI_IND:
            //limHandleLowRssiInd(pMac);
            break;

        case SIR_HAL_BMPS_STATUS_IND:
            limHandleBmpsStatusInd(pMac);
            break;

        case SIR_HAL_MISSED_BEACON_IND:
            limHandleMissedBeaconInd(pMac);
            break;

#if (WNI_POLARIS_FW_PACKAGE == ADVANCED) && defined(ANI_PRODUCT_TYPE_AP)
        case eWNI_SME_MEASUREMENT_REQ:
        case eWNI_SME_SET_WDS_INFO_REQ:
        case eWNI_SME_SET_POWER_REQ:
        case eWNI_SME_CLIENT_SIDE_LOAD_BALANCE_REQ:
        case eWNI_SME_SELECT_CHANNEL_REQ:
        case eWNI_SME_SET_PROPRIETARY_IE_REQ:
            // Message to support ANI feature set
            // These are handled by LMM sub module
            if (limIsSystemInScanState(pMac))
            {
                // System is in DFS (Learn) mode
                // Defer processsing this message
                if (limDeferMsg(pMac, limMsg) != TX_SUCCESS)
                {
                    pMac->lim.numSme++;
                    limLog(pMac, LOGE, FL("Unable to Defer message %X\n"),
                           limMsg->type);
                    limPrintMsgName(pMac, LOGE, limMsg->type);
                    // Release body
                    palFreeMemory( pMac->hHdd, (tANI_U8 *) limMsg->bodyptr);
                    break;
                }

                if (limMsg->type == eWNI_SME_MEASUREMENT_REQ)
                {
                    if (GET_LIM_PROCESS_DEFD_MESGS(pMac))
                        limSendHalFinishScanReq(pMac, eLIM_HAL_FINISH_LEARN_WAIT_STATE);
                }
            }
            else
            {
                pMac->lim.numSme++;
                limProcessLmmMessages(pMac,
                                      limMsg->type,
                                      (tANI_U32 *) limMsg->bodyptr);

                // Release body
                palFreeMemory( pMac->hHdd, (tANI_U8 *) limMsg->bodyptr);
            }
            break;

        case SIR_LIM_LEARN_INTERVAL_TIMEOUT:
            if ((pMac->lim.gLimSystemRole == eLIM_STA_ROLE) &&
                ((pMac->lim.gLimMlmState == eLIM_MLM_WT_DEL_STA_RSP_STATE) ||
                 (pMac->lim.gLimMlmState == eLIM_MLM_WT_DEL_BSS_RSP_STATE)))
            {
                // BP is in the process of cleaning up
                // its state with previously assocaited AP.
                // Discard processsing this message.
                PELOG1(limLog(pMac, LOG1,
                       FL("Discarding LEARN_INTERVAL_TO message\n"));)
            }
            else
                limProcessLmmMessages(pMac,
                                      limMsg->type,
                                      (tANI_U32 *) limMsg->bodyptr);
            break;

        case SIR_LIM_MEASUREMENT_IND_TIMEOUT:
        case SIR_LIM_LEARN_DURATION_TIMEOUT:
            // These measurement related timeouts are
            // handled by LMM sub module.
            limProcessLmmMessages(pMac,
                                  limMsg->type,
                                  (tANI_U32 *) limMsg->bodyptr);

            break;

        case SIR_LIM_RADAR_DETECT_IND:
            limDetectRadar(pMac, (tANI_U32*)limMsg->bodyptr);
            palFreeMemory( pMac->hHdd, (tANI_U32*)limMsg->bodyptr);
            break;

#endif

        case SIR_LIM_ADDTS_RSP_TIMEOUT:
            limProcessSmeReqMessages(pMac,limMsg);
            break;

        case SIR_HAL_ADD_TS_RSP:
            limProcessHalAddTsRsp(pMac, limMsg);
            break;

        case SIR_LIM_DEL_TS_IND:
          limProcessDelTsInd(pMac, limMsg);
          break;
        case SIR_LIM_ADD_BA_IND:
            limProcessAddBaInd(pMac, limMsg);
            break;
        case SIR_LIM_DEL_BA_ALL_IND:
            limDelAllBASessions(pMac);
            break;
        case SIR_LIM_DEL_BA_IND:
            limProcessMlmHalBADeleteInd( pMac, limMsg );
            break;

         case SIR_LIM_BEACON_GEN_IND: {
#ifdef ANI_PRODUCT_TYPE_AP
                    if (pMac->lim.gLimSystemRole == eLIM_AP_ROLE)
                        pmmUpdateTIM(pMac, (tpBeaconGenParams)limMsg->bodyptr);
#endif
                    palFreeMemory(pMac->hHdd, (void*)limMsg->bodyptr);
                    schProcessPreBeaconInd(pMac);
                }
                break;

        case SIR_LIM_DELETE_STA_CONTEXT_IND:
            limDeleteStaContext(pMac, limMsg);
            break;

        case SIR_LIM_MIN_CHANNEL_TIMEOUT:
        case SIR_LIM_MAX_CHANNEL_TIMEOUT:
        case SIR_LIM_JOIN_FAIL_TIMEOUT:
        case SIR_LIM_AUTH_FAIL_TIMEOUT:
        case SIR_LIM_AUTH_RSP_TIMEOUT:
        case SIR_LIM_ASSOC_FAIL_TIMEOUT:
        case SIR_LIM_REASSOC_FAIL_TIMEOUT:
            // These timeout messages are handled by MLM sub module

            limProcessMlmReqMessages(pMac,
                                     limMsg);

            break;

        case SIR_LIM_HEART_BEAT_TIMEOUT:
            /** check if heart beat failed, even if one Beacon
                    * is rcvd within the Heart Beat interval continue
                  * normal processing
                    */
            PELOG1(limLog(pMac, LOG1, FL("Heartbeat timeout, SME %d, MLME %d, #bcn %d\n"),
                   pMac->lim.gLimSmeState, pMac->lim.gLimMlmState,
                   pMac->lim.gLimRxedBeaconCntDuringHB);)

            if(pMac->lim.gLimSystemRole == eLIM_STA_IN_IBSS_ROLE)
                limIbssHeartBeatHandle(pMac); //HeartBeat for peers.
            else
                /**
                        * Heartbeat failure occurred on STA
                      * This is handled by LMM sub module.
                        */
                limHandleHeartBeatFailure(pMac);

            break;

        case SIR_LIM_PROBE_HB_FAILURE_TIMEOUT:
            /**
             * Probe response is not received
             * after HB failure.
             * This is handled by LMM sub module.
             */
            limLog(pMac, LOGE, FL("Probe_hb_failure: SME %d, MLME %d, HB-Count %d\n"),
                   pMac->lim.gLimSmeState, pMac->lim.gLimMlmState, pMac->lim.gLimRxedBeaconCntDuringHB);
            if (pMac->lim.gLimMlmState == eLIM_MLM_LINK_ESTABLISHED_STATE)
            {
                if (!LIM_IS_CONNECTION_ACTIVE(pMac))
                {
                    /**
                     * AP did not respond to Probe Request.
                     * Tear down link with it.
                     */
                    limTearDownLinkWithAp(pMac);
                    pMac->lim.gLimProbeFailureAfterHBfailedCnt++ ;
                }
                else // restart heartbeat timer
                {
                    limLog(pMac, LOGE, FL("***** **** ProbeReponse timeout with RxedBeaconCount = %d\n"),
                           pMac->lim.gLimRxedBeaconCntDuringHB);
                    limReactivateTimer(pMac, eLIM_HEART_BEAT_TIMER);
                }
            }
            else
            {
                limLog(pMac, LOGE,
                       FL("Unexpected wt-probe-timeout in state \n"));
                limPrintMlmState(pMac, LOGE, pMac->lim.gLimMlmState);
                limReactivateTimer(pMac, eLIM_HEART_BEAT_TIMER);
            }

            break;

        case SIR_LIM_CHANNEL_SCAN_TIMEOUT:
#if defined(ANI_PRODUCT_TYPE_CLIENT) || defined(ANI_AP_CLIENT_SDK)
            /**
             * Background scan timeout occurred on STA.
             * This is handled by LMM sub module.
             */
             limDeactivateAndChangeTimer(pMac, eLIM_BACKGROUND_SCAN_TIMER);

            //We will do background scan even in bcnps mode
            //if (pMac->sys.gSysEnableScanMode)
            pMac->lim.gLimReportBackgroundScanResults = FALSE;
            limTriggerBackgroundScan(pMac);
#endif
            break;

#ifdef ANI_PRODUCT_TYPE_AP
        case SIR_LIM_AID_RELEASE_TIMEOUT:
            // ReleaseAID timeout message
            if (pMac->lim.gLimSystemRole == eLIM_AP_ROLE)
                limReleaseAIDHandler(pMac);

            break;

        case SIR_LIM_PREAUTH_CLNUP_TIMEOUT:
            if (limIsSystemInScanState(pMac))
            {
                // System is in DFS (Learn) mode
                // Defer processsing this message
                if (limDeferMsg(pMac, limMsg) != TX_SUCCESS)
                    limLog(pMac, LOGE, FL("Unable to Defer message %X\n"),
                           limMsg->type);
            }
            else
            {
                // Pre-authentication context cleanup timeout message
                limPreAuthClnupHandler(pMac);
            }

            break;
#endif

        case SIR_LIM_HASH_MISS_THRES_TIMEOUT:

            /*
            ** clear the credit to the send disassociate frame bucket
            **/

            pMac->lim.gLimDisassocFrameCredit = 0;
            break;

        case SIR_LIM_CNF_WAIT_TIMEOUT:

            /*
            ** Does not receive CNF or dummy packet
            **/
            limHandleCnfWaitTimeout(pMac, (tANI_U16) limMsg->bodyval);

            break;

        case SIR_LIM_KEEPALIVE_TIMEOUT:
            limSendKeepAliveToPeer(pMac);

            break;

        case SIR_LIM_RETRY_INTERRUPT_MSG:
            // Message from ISR upon TFP's max retry limit interrupt

            break;

        case SIR_LIM_INV_KEY_INTERRUPT_MSG:
            // Message from ISR upon SP's Invalid session key interrupt

            break;

        case SIR_LIM_KEY_ID_INTERRUPT_MSG:
            // Message from ISR upon SP's Invalid key ID interrupt

            break;

        case SIR_LIM_REPLAY_THRES_INTERRUPT_MSG:
            // Message from ISR upon SP's Replay threshold interrupt

            break;

#if defined(ANI_PRODUCT_TYPE_AP)            
        case SIR_LIM_UPDATE_OLBC_CACHEL_TIMEOUT:
            limHandleUpdateOlbcCache(pMac);
            break;
#endif

        case SIR_LIM_CHANNEL_SWITCH_TIMEOUT:
            limProcessChannelSwitchTimeout(pMac);
            break;

        case SIR_LIM_QUIET_TIMEOUT:
            limProcessQuietTimeout(pMac);
            break;

        case SIR_LIM_QUIET_BSS_TIMEOUT:
            limProcessQuietBssTimeout(pMac);
            break;

        case SIR_HAL_ADD_BSS_RSP:
            limProcessMlmAddBssRsp( pMac, limMsg );
            break;

        case SIR_HAL_ADD_STA_RSP:
            if (pMac->lim.gLimSystemRole == eLIM_STA_IN_IBSS_ROLE)
                (void) limIbssAddStaRsp(pMac, limMsg->bodyptr);
            else
                limProcessMlmAddStaRsp(pMac, limMsg);
            break;

        case SIR_HAL_DELETE_STA_RSP:
            limProcessMlmDelStaRsp(pMac, limMsg);
            break;

        case SIR_HAL_DELETE_BSS_RSP:
            if (pMac->lim.gLimSystemRole == eLIM_STA_IN_IBSS_ROLE)
                (void) limIbssDelBssRsp(pMac, limMsg->bodyptr);
            else if (pMac->lim.gLimSystemRole == eLIM_UNKNOWN_ROLE)
                {
                    vos_mem_free((v_VOID_t*)limMsg->bodyptr);
                    limMsg->bodyptr = NULL;
                    
                    limProcessSmeDelBssRsp(pMac, limMsg->bodyval);
                }
            else
                limProcessMlmDelBssRsp(pMac, limMsg);
            break;

        case SIR_HAL_SET_BSSKEY_RSP:
        case SIR_HAL_SET_STAKEY_RSP:
        case SIR_HAL_SET_STA_BCASTKEY_RSP:
            limProcessMlmSetKeyRsp( pMac, limMsg );
            break;
        case SIR_HAL_REMOVE_BSSKEY_RSP:
        case SIR_HAL_REMOVE_STAKEY_RSP:
            limProcessMlmRemoveKeyRsp( pMac, limMsg );
            break;
        case SIR_HAL_ADDBA_RSP:
            limProcessMlmHalAddBARsp( pMac, limMsg );
            break;

        case SIR_HAL_STA_STAT_RSP:
        case SIR_HAL_AGGR_STAT_RSP:
        case SIR_HAL_GLOBAL_STAT_RSP:
        case SIR_HAL_STAT_SUMM_RSP:
            limSendSmeStatsRsp ( pMac, limMsg->type, (void *)limMsg->bodyptr);
            break;

        case SIR_HAL_GET_STATISTICS_RSP:
            limSendSmePEStatisticsRsp ( pMac, limMsg->type, (void *)limMsg->bodyptr);
            break;

        case SIR_HAL_SET_MIMOPS_RSP:            //limProcessSetMimoRsp(pMac, limMsg);
        case SIR_HAL_SET_TX_POWER_RSP:          //limProcessSetTxPowerRsp(pMac, limMsg);
        case SIR_HAL_GET_TX_POWER_RSP:          //limProcessGetTxPowerRsp(pMac, limMsg);
        case SIR_HAL_GET_NOISE_RSP:
            vos_mem_free((v_VOID_t*)limMsg->bodyptr);
            limMsg->bodyptr = NULL;
            //limProcessGetNoiseRsp(pMac, limMsg);
            break;

#ifdef ANI_CHIPSET_VOLANS
       case SIR_LIM_ADDR2_MISS_IND:
       {
           limLog(pMac, LOGE,
                   FL("Addr2 mismatch interrupt received %X\n"),
                   limMsg->type);
           /*a message from HAL indicating addr2 mismatch interrupt occured
             limMsg->bodyptr contains only pointer to 48-bit addr2 field*/
           limHandleUnknownA2IndexFrames(pMac, (void *)limMsg->bodyptr); 

           /*Free message body pointer*/ 
           vos_mem_free((v_VOID_t *)(limMsg->bodyptr));
           break;
       }
#endif

        default:
            vos_mem_free((v_VOID_t*)limMsg->bodyptr);
            limMsg->bodyptr = NULL;
            // Unwanted messages
            // Log error
            limLog(pMac, LOGE,
                   FL("Discarding unexpected message received %X\n"),
                   limMsg->type);
            limPrintMsgName(pMac, LOGE, limMsg->type);
            break;

    } // switch (limMsg->type)

   PELOG2(limLog(pMac, LOG2, FL("Done Processing msgType = %d, sme state = %s, mlm state = %s\n"),
            limMsg->type, limSmeStateStr(pMac->lim.gLimSmeState),
            limMlmStateStr(pMac->lim.gLimMlmState));)

} /*** end limProcessMessages() ***/



/**
 * limProcessDeferredMessageQueue
 *
 *FUNCTION:
 * This function is called by LIM while exiting from Learn
 * mode. This function fetches messages posted to the LIM
 * deferred message queue limDeferredMsgQ.
 *
 *LOGIC:
 *
 *ASSUMPTIONS:
 * NA
 *
 *NOTE:
 * NA
 *
 * @param  pMac - Pointer to Global MAC structure
 * @return None
 */

void
limProcessDeferredMessageQueue(tpAniSirGlobal pMac)
{
    tSirMsgQ  limMsg = { 0, 0, 0 };

#if defined(ANI_OS_TYPE_LINUX) || defined(ANI_OS_TYPE_OSX)
    while (TX_SUCCESS == tx_queue_receive(&pMac->sys.gSirLimDeferredMsgQ, (void *) &limMsg, TX_NO_WAIT))
    {
        PELOG3(limLog(pMac, LOG3, FL("Processing deferred message %X\n"), limMsg.type);)
        limPrintMsgName(pMac, LOG3, limMsg.type);
        pMac->lim.gLimNumDeferredMsgs--;
        limProcessMessages(pMac, &limMsg);

        if(true != GET_LIM_PROCESS_DEFD_MESGS(pMac))
            break;
    }
#else
    tSirMsgQ *readMsg;
    tANI_U16  size;

    /*
    ** check any deferred messages need to be processed
    **/
    size = pMac->lim.gLimDeferredMsgQ.size;
    if (size > 0)
    {
        while ((readMsg = limReadDeferredMsgQ(pMac)) != NULL)
        {
            palCopyMemory( pMac->hHdd, (tANI_U8*) &limMsg,
                    (tANI_U8*) readMsg, sizeof(tSirMsgQ));
            size--; 
            limProcessMessages(pMac, &limMsg);

            if((limIsSystemInScanState(pMac)) || (true != GET_LIM_PROCESS_DEFD_MESGS(pMac)) ||
                 (pMac->lim.gLimSystemInScanLearnMode))
                break;
        }
    }
#endif
} /*** end limProcessDeferredMessageQueue() ***/


/*
 * limProcessNormalHddMsg
 * Function: this function checks the current lim state and decide whether the message passed shall be deffered.
 * @param  pMac - Pointer to Global MAC structure
 *         pLimMsg -- the message need to be processed
 *         fRspReqd -- whether return result to hdd
 * @return None
 */
void limProcessNormalHddMsg(tpAniSirGlobal pMac, tSirMsgQ *pLimMsg, tANI_U8 fRspReqd)
{
    tANI_BOOLEAN fDeferMsg = eANI_BOOLEAN_TRUE;

    if ((pMac->lim.gLimSystemRole == eLIM_AP_ROLE) ||
            (pMac->lim.gLimSystemRole == eLIM_UNKNOWN_ROLE))
    {
        /** This check is required only for the AP and in 2 cases.
         * 1. If we are in learn mode and we receive any of these messages,
         * you have to come out of scan and process the message, hence dont
         * defer the message here. In handler, these message could be defered
         * till we actually come out of scan mode.
         * 2. If radar is detected, you might have to defer all of these
         * messages except Stop BSS request/ Switch channel request. This
         * decision is also made inside its handler.
         *
         * Please be careful while using the flag fDeferMsg. Possibly you
         * might end up in an infinite loop.
         **/
        if (((pLimMsg->type == eWNI_SME_START_BSS_REQ) ||
             (pLimMsg->type == eWNI_SME_STOP_BSS_REQ) ||
             (pLimMsg->type == eWNI_SME_SWITCH_CHL_REQ) ||
             (pLimMsg->type == eWNI_SME_SWITCH_CHL_CB_SECONDARY_REQ) ||
             (pLimMsg->type == eWNI_SME_SWITCH_CHL_CB_PRIMARY_REQ)))
        {
            fDeferMsg = eANI_BOOLEAN_FALSE;
        }
    }

    if (((pMac->lim.gLimAddtsSent) || (limIsSystemInScanState(pMac)) ||
                (LIM_IS_RADAR_DETECTED(pMac))) && fDeferMsg)
    {
        // System is in DFS (Learn) mode or awaiting addts response
        // or if radar is detected, Defer processsing this message
        if (limDeferMsg(pMac, pLimMsg) != TX_SUCCESS)
        {
#ifdef WLAN_DEBUG            
            pMac->lim.numSme++;
#endif
            limLog(pMac, LOGE, FL("Unable to Defer message %X\n"),
                   pLimMsg->type);
            limPrintMsgName(pMac, LOGE, pLimMsg->type);
            // Release body
            palFreeMemory( pMac->hHdd, (tANI_U8 *) pLimMsg->bodyptr);
        }
    }
    else
    {
        if(fRspReqd)
        {
            // These messages are from HDD
            // Since these requests may also be generated
            // internally within LIM module, need to
            // distinquish and send response to host
            pMac->lim.gLimRspReqd = eANI_BOOLEAN_TRUE;
        }
#ifdef WLAN_DEBUG            
        pMac->lim.numSme++;
#endif
        if(limProcessSmeReqMessages(pMac, pLimMsg))
        {
            // Release body
            // limProcessSmeReqMessage consumed the buffer. We can free it.
            palFreeMemory( pMac->hHdd, (tANI_U8 *) pLimMsg->bodyptr);
        }
    }
}

void
handleHTCapabilityandHTInfo(struct sAniSirGlobal *pMac)
{
    tSirMacHTCapabilityInfo macHTCapabilityInfo;
    tSirMacHTParametersInfo macHTParametersInfo;
    tSirMacHTInfoField1 macHTInfoField1;
    tSirMacHTInfoField2 macHTInfoField2;
    tSirMacHTInfoField3 macHTInfoField3;
    tANI_U32  cfgValue;
    tANI_U8 *ptr;


    pMac->lim.htCapability = IS_DOT11_MODE_HT(pMac->lim.gLimDot11Mode);




    // Get HT Capabilities
    if (wlan_cfgGetInt(pMac, WNI_CFG_HT_CAP_INFO, &cfgValue) != eSIR_SUCCESS)
    {
        limLog(pMac, LOGP, FL("Fail to retrieve WNI_CFG_HT_CAP_INFO value\n"));
        return ;
    }
    ptr = (tANI_U8 *) &macHTCapabilityInfo;
    *((tANI_U16 *)ptr) =  (tANI_U16) (cfgValue & 0xffff);
    pMac->lim.gHTLsigTXOPProtection = (tANI_U8)macHTCapabilityInfo.lsigTXOPProtection;
    pMac->lim.gHTMIMOPSState = (tSirMacHTMIMOPowerSaveState) macHTCapabilityInfo.mimoPowerSave;
    pMac->lim.gHTGreenfield = (tANI_U8)macHTCapabilityInfo.greenField;
    pMac->lim.gHTMaxAmsduLength = (tANI_U8)macHTCapabilityInfo.maximalAMSDUsize;
    pMac->lim.gHTShortGI20Mhz = (tANI_U8)macHTCapabilityInfo.shortGI20MHz;
    pMac->lim.gHTShortGI40Mhz = (tANI_U8)macHTCapabilityInfo.shortGI40MHz;
    pMac->lim.gHTSupportedChannelWidthSet = (tANI_U8)macHTCapabilityInfo.supportedChannelWidthSet;
    pMac->lim.gHTPSMPSupport = (tANI_U8)macHTCapabilityInfo.psmp;
    pMac->lim.gHTDsssCckRate40MHzSupport = (tANI_U8)macHTCapabilityInfo.dsssCckMode40MHz;

    if (wlan_cfgGetInt(pMac, WNI_CFG_HT_AMPDU_PARAMS, &cfgValue) != eSIR_SUCCESS)
    {
        limLog(pMac, LOGP, FL("Fail to retrieve WNI_CFG_HT_PARAM_INFO value\n"));
        return ;
    }
    ptr = (tANI_U8 *) &macHTParametersInfo;
    *ptr =  (tANI_U8) (cfgValue & 0xff);
    pMac->lim.gHTAMpduDensity = (tANI_U8)macHTParametersInfo.mpduDensity;
    pMac->lim.gHTMaxRxAMpduFactor = (tANI_U8)macHTParametersInfo.maxRxAMPDUFactor;

    // Get HT IE Info
    if (wlan_cfgGetInt(pMac, WNI_CFG_HT_INFO_FIELD1, &cfgValue) != eSIR_SUCCESS)
    {
        limLog(pMac, LOGP, FL("Fail to retrieve WNI_CFG_HT_INFO_FIELD1 value\n"));
        return ;
    }
    ptr = (tANI_U8 *) &macHTInfoField1;
    *((tANI_U8 *)ptr) =  (tANI_U8) (cfgValue & 0xff);
    pMac->lim.gHTServiceIntervalGranularity = (tANI_U8)macHTInfoField1.serviceIntervalGranularity;
    pMac->lim.gHTControlledAccessOnly = (tANI_U8)macHTInfoField1.controlledAccessOnly;
    pMac->lim.gHTRifsMode = (tANI_U8)macHTInfoField1.rifsMode;
    pMac->lim.gHTRecommendedTxWidthSet = (tANI_U8)macHTInfoField1.recommendedTxWidthSet;
    pMac->lim.gHTSecondaryChannelOffset = (tSirMacHTSecondaryChannelOffset)macHTInfoField1.secondaryChannelOffset;

    if (wlan_cfgGetInt(pMac, WNI_CFG_HT_INFO_FIELD2, &cfgValue) != eSIR_SUCCESS)
    {
        limLog(pMac, LOGP, FL("Fail to retrieve WNI_CFG_HT_INFO_FIELD2 value\n"));
        return ;
    }
    ptr = (tANI_U8 *) &macHTInfoField2;
    *((tANI_U16 *)ptr) = (tANI_U16) (cfgValue & 0xffff);
    pMac->lim.gHTOperMode = (tSirMacHTOperatingMode) macHTInfoField2.opMode;

    if (wlan_cfgGetInt(pMac, WNI_CFG_HT_INFO_FIELD3, &cfgValue) != eSIR_SUCCESS)
    {
        limLog(pMac, LOGP, FL("Fail to retrieve WNI_CFG_HT_INFO_FIELD3 value\n"));
        return ;
    }
    ptr = (tANI_U8 *) &macHTInfoField3;
    *((tANI_U16 *)ptr) = (tANI_U16) (cfgValue & 0xffff);
    pMac->lim.gHTPCOActive = (tANI_U8)macHTInfoField3.pcoActive;
    pMac->lim.gHTPCOPhase = (tANI_U8)macHTInfoField3.pcoPhase;
    pMac->lim.gHTLSigTXOPFullSupport = (tANI_U8)macHTInfoField3.lsigTXOPProtectionFullSupport;
    pMac->lim.gHTSecondaryBeacon = (tANI_U8)macHTInfoField3.secondaryBeacon;
    pMac->lim.gHTDualCTSProtection = (tANI_U8)macHTInfoField3.dualCTSProtection;
    pMac->lim.gHTSTBCBasicMCS = (tANI_U8)macHTInfoField3.basicSTBCMCS;
}
