/*
 * Airgo Networks, Inc proprietary. All rights reserved.
 * This file limUtils.cc contains the utility functions
 * LIM uses.
 * Author:        Chandra Modumudi
 * Date:          02/13/02
 * History:-
 * Date           Modified by    Modification Information
 * --------------------------------------------------------------------
 */

#include "schApi.h"
#include "limUtils.h"
#include "limTypes.h"
#include "limSecurityUtils.h"
#include "limPropExtsUtils.h"
#include "limSendMessages.h"
#include "limSerDesUtils.h"
#include "limAdmitControl.h"
#include "limStaHashApi.h"
#include "dot11f.h"
#include "wmmApsd.h"
#include "limTrace.h"
#ifdef FEATURE_WLAN_DIAG_SUPPORT 
#include "vos_diag_core_event.h"
#endif //FEATURE_WLAN_DIAG_SUPPORT
#include "limIbssPeerMgmt.h"
#include "limSession.h"

/* Static global used to mark situations where pMac->lim.gLimTriggerBackgroundScanDuringQuietBss is SET
 * and limTriggerBackgroundScanDuringQuietBss() returned failure.  In this case, we will stop data
 * traffic instead of going into scan.  The recover function limProcessQuietBssTimeout() needs to have
 * this information. */
static tAniBool glimTriggerBackgroundScanDuringQuietBss_Status = eSIR_TRUE;

/* 11A Channel list to decode RX BD channel information */
static const tANI_U8 abChannel[]= {36,40,44,48,52,56,60,64,100,104,108,112,116,
            120,124,128,132,136,140,149,153,157,161,165};

//#define LIM_MAX_ACTIVE_SESSIONS 3  //defined temporarily for BT-AMP SUPPORT 
#define SUCCESS 1                   //defined temporarily for BT-AMP

/** -------------------------------------------------------------
\fn limAssignDialogueToken
\brief Assigns dialogue token.
\param     tpAniSirGlobal    pMac
\return tpDialogueToken - dialogueToken data structure.
  -------------------------------------------------------------*/

tpDialogueToken
limAssignDialogueToken(tpAniSirGlobal pMac)
{
    static tANI_U8 token = 0;
    tpDialogueToken pCurrNode;
    if(eHAL_STATUS_SUCCESS !=
          palAllocateMemory(pMac->hHdd, (void **) &pCurrNode, sizeof(tDialogueToken)))
    {
        PELOGE(limLog(pMac, LOGE, FL("palAllocateMemory failed\n"));)
        return NULL;
    }

    palZeroMemory(pMac->hHdd, (void *) pCurrNode, sizeof(tDialogueToken));
    //first node in the list is being added.
    if(NULL == pMac->lim.pDialogueTokenHead)
    {
        pMac->lim.pDialogueTokenHead = pMac->lim.pDialogueTokenTail = pCurrNode;
    }
    else
    {
        pMac->lim.pDialogueTokenTail->next = pCurrNode;
        pMac->lim.pDialogueTokenTail = pCurrNode;
    }
    //assocId and tid of the node will be filled in by caller.
    pCurrNode->next = NULL;
    pCurrNode->token = token++;
    PELOG4(limLog(pMac, LOG4, FL("token assigned = %d\n"), token);)    
    return pCurrNode;
}

/** -------------------------------------------------------------
\fn limSearchAndDeleteDialogueToken
\brief search dialogue token in the list and deletes it if found. returns failure if not found.
\param     tpAniSirGlobal    pMac
\param     tANI_U8 token
\param     tANI_U16 assocId
\param     tANI_U16 tid
\return eSirRetStatus - status of the search
  -------------------------------------------------------------*/


tSirRetStatus
limSearchAndDeleteDialogueToken(tpAniSirGlobal pMac, tANI_U8 token, tANI_U16 assocId, tANI_U16 tid)
{
    tpDialogueToken pCurrNode = pMac->lim.pDialogueTokenHead;
    tpDialogueToken pPrevNode = pMac->lim.pDialogueTokenHead;

    //if the list is empty
    if(NULL == pCurrNode)
      return eSIR_FAILURE;

    // if the matching node is the first node.
    if(pCurrNode &&
        (assocId == pCurrNode->assocId) &&
        (tid == pCurrNode->tid))
    {
        pMac->lim.pDialogueTokenHead = pCurrNode->next;        
        //there was only one node in the list. So tail pointer also needs to be adjusted.
        if(NULL == pMac->lim.pDialogueTokenHead)
            pMac->lim.pDialogueTokenTail = NULL;
        palFreeMemory(pMac->hHdd, (void *) pCurrNode);
        return eSIR_SUCCESS;
    }

    //first node did not match. so move to the next one.
    pCurrNode = pCurrNode->next;
    while(NULL != pCurrNode )
    {
        if(token == pCurrNode->token)
        {
            break;
        }

        pPrevNode = pCurrNode;
        pCurrNode = pCurrNode->next;
    }

    if(pCurrNode &&
        (assocId == pCurrNode->assocId) &&
        (tid == pCurrNode->tid))
    {
        pPrevNode->next = pCurrNode->next;
        //if the node being deleted is the last one then we also need to move the tail pointer to the prevNode.
        if(NULL == pCurrNode->next)
              pMac->lim.pDialogueTokenTail = pPrevNode;
        return eSIR_SUCCESS;
    }

    PELOGW(limLog(pMac, LOGW, FL("LIM does not have matching dialogue token node\n"));)
    return eSIR_FAILURE;

}


/** -------------------------------------------------------------
\fn limDeleteDialogueTokenList
\brief deletes the complete lim dialogue token linked list.
\param     tpAniSirGlobal    pMac
\return     None
  -------------------------------------------------------------*/
void
limDeleteDialogueTokenList(tpAniSirGlobal pMac)
{
    tpDialogueToken pCurrNode = pMac->lim.pDialogueTokenHead;

    while(NULL != pMac->lim.pDialogueTokenHead)
    {
        pCurrNode = pMac->lim.pDialogueTokenHead;    
        pMac->lim.pDialogueTokenHead = pMac->lim.pDialogueTokenHead->next;
        palFreeMemory(pMac->hHdd, (void *) pCurrNode);
        pCurrNode = NULL;
    }
    pMac->lim.pDialogueTokenTail = NULL;
}

void
limGetBssidFromBD(tpAniSirGlobal pMac, tANI_U8 * pRxPacketInfo, tANI_U8 *bssId, tANI_U32 *pIgnore)
{
    tpSirMacDataHdr3a pMh = WDA_GET_RX_MPDUHEADER3A(pRxPacketInfo);
    *pIgnore = 0;

    if (pMh->fc.toDS == 1 && pMh->fc.fromDS == 0)
    {
        palCopyMemory( pMac->hHdd, bssId, pMh->addr1, 6);
        *pIgnore = 1;
    }
    else if (pMh->fc.toDS == 0 && pMh->fc.fromDS == 1)
    {
        palCopyMemory( pMac->hHdd, bssId, pMh->addr2, 6);
        *pIgnore = 1;
    }
    else if (pMh->fc.toDS == 0 && pMh->fc.fromDS == 0)
    {
        palCopyMemory( pMac->hHdd, bssId, pMh->addr3, 6);
        *pIgnore = 0;
    }
    else
    {
        palCopyMemory( pMac->hHdd, bssId, pMh->addr1, 6);
        *pIgnore = 1;
    }
}

char *
limMlmStateStr(tLimMlmStates state)
{

#ifdef FIXME_GEN6
    switch (state)
    {
        case eLIM_MLM_OFFLINE_STATE:
            return "eLIM_MLM_OFFLINE_STATE\n";
        case eLIM_MLM_IDLE_STATE:
            return "eLIM_MLM_IDLE_STATE\n";
        case eLIM_MLM_WT_PROBE_RESP_STATE:
            return "eLIM_MLM_WT_PROBE_RESP_STATE\n";
        case eLIM_MLM_PASSIVE_SCAN_STATE:
            return "eLIM_MLM_PASSIVE_SCAN_STATE\n";
        case eLIM_MLM_WT_JOIN_BEACON_STATE:
            return "eLIM_MLM_WT_JOIN_BEACON_STATE\n";
        case eLIM_MLM_JOINED_STATE:
            return "eLIM_MLM_JOINED_STATE\n";
        case eLIM_MLM_BSS_STARTED_STATE:
            return "eLIM_MLM_BSS_STARTED_STATE\n";
        case eLIM_MLM_WT_AUTH_FRAME2_STATE:
            return "eLIM_MLM_WT_AUTH_FRAME2_STATE\n";
        case eLIM_MLM_WT_AUTH_FRAME3_STATE:
            return "eLIM_MLM_WT_AUTH_FRAME3_STATE\n";
        case eLIM_MLM_WT_AUTH_FRAME4_STATE:
            return "eLIM_MLM_WT_AUTH_FRAME4_STATE\n";
        case eLIM_MLM_AUTH_RSP_TIMEOUT_STATE:
            return "eLIM_MLM_AUTH_RSP_TIMEOUT_STATE\n";
        case eLIM_MLM_AUTHENTICATED_STATE:
            return "eLIM_MLM_AUTHENTICATED_STATE\n";
        case eLIM_MLM_WT_ASSOC_RSP_STATE:
            return "eLIM_MLM_WT_ASSOC_RSP_STATE\n";
        case eLIM_MLM_WT_REASSOC_RSP_STATE:
            return "eLIM_MLM_WT_REASSOC_RSP_STATE\n";
	case eLIM_MLM_WT_FT_REASSOC_RSP_STATE:
            return "eLIM_MLM_WT_FT_REASSOC_RSP_STATE";
        case eLIM_MLM_WT_DEL_STA_RSP_STATE:
            return "eLIM_MLM_WT_DEL_STA_RSP_STATE\n";
      case eLIM_MLM_WT_DEL_BSS_RSP_STATE:
            return "eLIM_MLM_WT_DEL_BSS_RSP_STATE\n";
       case eLIM_MLM_WT_ADD_STA_RSP_STATE:
            return "eLIM_MLM_WT_ADD_STA_RSP_STATE\n";
      case eLIM_MLM_WT_ADD_BSS_RSP_STATE:
            return "eLIM_MLM_WT_ADD_BSS_RSP_STATE\n";
        case eLIM_MLM_REASSOCIATED_STATE:
            return "eLIM_MLM_REASSOCIATED_STATE\n";
        case eLIM_MLM_LINK_ESTABLISHED_STATE:
            return "eLIM_MLM_LINK_ESTABLISHED_STATE\n";
        case eLIM_MLM_WT_ASSOC_CNF_STATE:
            return "eLIM_MLM_WT_ASSOC_CNF_STATE\n";
        case eLIM_MLM_WT_ADD_BSS_RSP_ASSOC_STATE:
            return "eLIM_MLM_WT_ADD_BSS_RSP_ASSOC_STATE\n";
        case eLIM_MLM_WT_ADD_BSS_RSP_REASSOC_STATE:
            return "eLIM_MLM_WT_ADD_BSS_RSP_REASSOC_STATE\n";
	case eLIM_MLM_WT_ADD_BSS_RSP_FT_REASSOC_STATE:
            return "eLIM_MLM_WT_ADD_BSS_RSP_FT_REASSOC_STATE";
        case eLIM_MLM_WT_ASSOC_DEL_STA_RSP_STATE:
            return "eLIM_MLM_WT_ASSOC_DEL_STA_RSP_STATE\n";
        case eLIM_MLM_WT_SET_BSS_KEY_STATE:
            return "eLIM_MLM_WT_SET_BSS_KEY_STATE\n";
        case eLIM_MLM_WT_SET_STA_KEY_STATE:
            return "eLIM_MLM_WT_SET_STA_KEY_STATE\n";
        default:
            return "INVALID MLM state\n";
    }
#endif
return "";
}

void
limPrintMlmState(tpAniSirGlobal pMac, tANI_U16 logLevel, tLimMlmStates state)
{
    limLog(pMac, logLevel, limMlmStateStr(state));
}

char *
limSmeStateStr(tLimSmeStates state)
{
#ifdef FIXME_GEN6
    switch (state)
    {
        case eLIM_SME_OFFLINE_STATE:
            return "eLIM_SME_OFFLINE_STATE\n";
        case eLIM_SME_IDLE_STATE:
            return "eLIM_SME_IDLE_STATE\n";
        case eLIM_SME_SUSPEND_STATE:
            return "eLIM_SME_SUSPEND_STATE\n";
        case eLIM_SME_WT_SCAN_STATE:
            return "eLIM_SME_WT_SCAN_STATE\n";
        case eLIM_SME_WT_JOIN_STATE:
            return "eLIM_SME_WT_JOIN_STATE\n";
        case eLIM_SME_WT_AUTH_STATE:
            return "eLIM_SME_WT_AUTH_STATE\n";
        case eLIM_SME_WT_ASSOC_STATE:
            return "eLIM_SME_WT_ASSOC_STATE\n";
        case eLIM_SME_WT_REASSOC_STATE:
            return "eLIM_SME_WT_REASSOC_STATE\n";
        case eLIM_SME_WT_REASSOC_LINK_FAIL_STATE:
            return "eLIM_SME_WT_REASSOC_LINK_FAIL_STATE\n";
        case eLIM_SME_JOIN_FAILURE_STATE:
            return "eLIM_SME_JOIN_FAILURE_STATE\n";
        case eLIM_SME_ASSOCIATED_STATE:
            return "eLIM_SME_ASSOCIATED_STATE\n";
        case eLIM_SME_REASSOCIATED_STATE:
            return "eLIM_SME_REASSOCIATED_STATE\n";
        case eLIM_SME_LINK_EST_STATE:
            return "eLIM_SME_LINK_EST_STATE\n";
        case eLIM_SME_LINK_EST_WT_SCAN_STATE:
            return "eLIM_SME_LINK_EST_WT_SCAN_STATE\n";
        case eLIM_SME_WT_PRE_AUTH_STATE:
            return "eLIM_SME_WT_PRE_AUTH_STATE\n";
        case eLIM_SME_WT_DISASSOC_STATE:
            return "eLIM_SME_WT_DISASSOC_STATE\n";
        case eLIM_SME_WT_DEAUTH_STATE:
            return "eLIM_SME_WT_DEAUTH_STATE\n";
        case eLIM_SME_WT_START_BSS_STATE:
            return "eLIM_SME_WT_START_BSS_STATE\n";
        case eLIM_SME_WT_STOP_BSS_STATE:
            return "eLIM_SME_WT_STOP_BSS_STATE\n";
        case eLIM_SME_NORMAL_STATE:
            return "eLIM_SME_NORMAL_STATE\n";
        case eLIM_SME_CHANNEL_SCAN_STATE:
            return "eLIM_SME_CHANNEL_SCAN_STATE\n";
        case eLIM_SME_NORMAL_CHANNEL_SCAN_STATE:
            return "eLIM_SME_NORMAL_CHANNEL_SCAN_STATE\n";
        default:
            return "INVALID SME state\n";
    }
#endif
return "";
}


char* limDot11ModeStr(tpAniSirGlobal pMac, tANI_U8 dot11Mode)
{
#ifdef FIXME_GEN6

    switch(dot11Mode)
        {
            case WNI_CFG_DOT11_MODE_ALL:
                return "ALL\n";
            case WNI_CFG_DOT11_MODE_11A:
                return "11A\n";
            case WNI_CFG_DOT11_MODE_11B:  
                return "11B\n";
            case WNI_CFG_DOT11_MODE_11G:  
                return "11G\n";
            case WNI_CFG_DOT11_MODE_11N:  
                return "11N\n";
            case WNI_CFG_DOT11_MODE_POLARIS:  
                return "Polaris\n";
            case WNI_CFG_DOT11_MODE_TITAN:  
                return "Titan\n";
            case WNI_CFG_DOT11_MODE_TAURUS:
                return "Taurus\n";
            default:
                return "Invalid Dot11 Mode\n";
        }
#endif
return "";
}


char* limStaOpRateModeStr(tStaRateMode opRateMode)
{
#ifdef FIXME_GEN6

    switch(opRateMode)
        {
            case eSTA_TAURUS:
                return "Taurus\n";
            case eSTA_11a:
                return "11A\n";
            case eSTA_11b:  
                return "11B\n";
            case eSTA_11bg:  
                return "11G\n";
            case eSTA_11n:  
                return "11N\n";
            case eSTA_POLARIS:  
                return "Polaris\n";
            case eSTA_TITAN:
                return "Titan\n";
            default:
                return "Invalid Dot11 Mode\n";
        }
#endif
return "";
}

char* limBssTypeStr(tSirBssType bssType)
{
    switch(bssType)
    {
        case eSIR_INFRASTRUCTURE_MODE:
            return "eSIR_INFRASTRUCTURE_MODE";
        case eSIR_IBSS_MODE:
            return "eSIR_IBSS_MODE";
        case eSIR_BTAMP_STA_MODE:  
            return "eSIR_BTAMP_STA_MODE";
        case eSIR_BTAMP_AP_MODE:  
            return "eSIR_BTAMP_AP_MODE";
        case eSIR_AUTO_MODE:  
            return "eSIR_AUTO_MODE";
        default:
            return "Invalid BSS Type";
    }
}

void
limPrintSmeState(tpAniSirGlobal pMac, tANI_U16 logLevel, tLimSmeStates state)
{
    limLog(pMac, logLevel, limSmeStateStr(state));
}

char *limMsgStr(tANI_U32 msgType)
{
#ifdef FIXME_GEN6
    switch (msgType)
    {
        case eWNI_SME_START_REQ:
            return "eWNI_SME_START_REQ\n";
        case eWNI_SME_START_RSP:
            return "eWNI_SME_START_RSP\n";
        case eWNI_SME_SYS_READY_IND:
            return "eWNI_SME_SYS_READY_IND\n";
        case eWNI_SME_SCAN_REQ:
            return "eWNI_SME_SCAN_REQ\n";
#ifdef FEATURE_INNAV_SUPPORT
        case eWNI_SME_INNAV_MEAS_REQ:
            return "eWNI_SME_INNAV_MEAS_REQ\n";
        case eWNI_SME_INNAV_MEAS_RSP:
            return "eWNI_SME_INNAV_MEAS_RSP\n";
#endif
        case eWNI_SME_SCAN_RSP:
            return "eWNI_SME_SCAN_RSP\n";
        case eWNI_SME_JOIN_REQ:
            return "eWNI_SME_JOIN_REQ\n";
        case eWNI_SME_JOIN_RSP:
            return "eWNI_SME_JOIN_RSP\n";
        case eWNI_SME_SETCONTEXT_REQ:
            return "eWNI_SME_SETCONTEXT_REQ\n";
        case eWNI_SME_SETCONTEXT_RSP:
            return "eWNI_SME_SETCONTEXT_RSP\n";
        case eWNI_SME_REASSOC_REQ:
            return "eWNI_SME_REASSOC_REQ\n";
        case eWNI_SME_REASSOC_RSP:
            return "eWNI_SME_REASSOC_RSP\n";
        case eWNI_SME_AUTH_REQ:
            return "eWNI_SME_AUTH_REQ\n";
        case eWNI_SME_AUTH_RSP:
            return "eWNI_SME_AUTH_RSP\n";
        case eWNI_SME_DISASSOC_REQ:
            return "eWNI_SME_DISASSOC_REQ\n";
        case eWNI_SME_DISASSOC_RSP:
            return "eWNI_SME_DISASSOC_RSP\n";
        case eWNI_SME_DISASSOC_IND:
            return "eWNI_SME_DISASSOC_IND\n";
        case eWNI_SME_DISASSOC_CNF:
            return "eWNI_SME_DISASSOC_CNF\n";
        case eWNI_SME_DEAUTH_REQ:
            return "eWNI_SME_DEAUTH_REQ\n";
        case eWNI_SME_DEAUTH_RSP:
            return "eWNI_SME_DEAUTH_RSP\n";
        case eWNI_SME_DEAUTH_IND:
            return "eWNI_SME_DEAUTH_IND\n";
        case eWNI_SME_WM_STATUS_CHANGE_NTF:
            return "eWNI_SME_WM_STATUS_CHANGE_NTF\n";
        case eWNI_SME_START_BSS_REQ:
            return "eWNI_SME_START_BSS_REQ\n";
        case eWNI_SME_START_BSS_RSP:
            return "eWNI_SME_START_BSS_RSP\n";
        case eWNI_SME_AUTH_IND:
            return "eWNI_SME_AUTH_IND\n";
        case eWNI_SME_ASSOC_IND:
            return "eWNI_SME_ASSOC_IND\n";
        case eWNI_SME_ASSOC_CNF:
            return "eWNI_SME_ASSOC_CNF\n";
        case eWNI_SME_REASSOC_IND:
            return "eWNI_SME_REASSOC_IND\n";
        case eWNI_SME_REASSOC_CNF:
            return "eWNI_SME_REASSOC_CNF\n";
        case eWNI_SME_SWITCH_CHL_REQ:
            return "eWNI_SME_SWITCH_CHL_REQ\n";
        case eWNI_SME_SWITCH_CHL_RSP:
            return "eWNI_SME_SWITCH_CHL_RSP\n";
        case eWNI_SME_SWITCH_CHL_CB_PRIMARY_REQ:
            return "eWNI_SME_SWITCH_CHL_CB_PRIMARY_REQ\n";
        case eWNI_SME_SWITCH_CHL_CB_SECONDARY_REQ:
            return "eWNI_SME_SWITCH_CHL_CB_SECONDARY_REQ\n";
        case eWNI_SME_STOP_BSS_REQ:
            return "eWNI_SME_STOP_BSS_REQ\n";
        case eWNI_SME_STOP_BSS_RSP:
            return "eWNI_SME_STOP_BSS_RSP\n";
        case eWNI_SME_PROMISCUOUS_MODE_REQ:
            return "eWNI_SME_PROMISCUOUS_MODE_REQ\n";
        case eWNI_SME_PROMISCUOUS_MODE_RSP:
            return "eWNI_SME_PROMISCUOUS_MODE_RSP\n";
        case eWNI_SME_NEIGHBOR_BSS_IND:
            return "eWNI_SME_NEIGHBOR_BSS_IND\n";
        case eWNI_SME_MEASUREMENT_REQ:
            return "eWNI_SME_MEASUREMENT_REQ\n";
        case eWNI_SME_MEASUREMENT_RSP:
            return "eWNI_SME_MEASUREMENT_RSP\n";
        case eWNI_SME_MEASUREMENT_IND:
            return "eWNI_SME_MEASUREMENT_IND\n";
        case eWNI_SME_SET_WDS_INFO_REQ:
            return "eWNI_SME_SET_WDS_INFO_REQ\n";
        case eWNI_SME_SET_WDS_INFO_RSP:
            return "eWNI_SME_SET_WDS_INFO_RSP\n";
        case eWNI_SME_WDS_INFO_IND:
            return "eWNI_SME_WDS_INFO_IND\n";
        case eWNI_SME_DEAUTH_CNF:
            return "eWNI_SME_DEAUTH_CNF\n";
        case eWNI_SME_MIC_FAILURE_IND:
            return "eWNI_SME_MIC_FAILURE_IND\n";
        case eWNI_SME_ADDTS_REQ:
            return "eWNI_SME_ADDTS_REQ\n";
        case eWNI_SME_ADDTS_RSP:
            return "eWNI_SME_ADDTS_RSP\n";
        case eWNI_SME_ADDTS_CNF:
            return "eWNI_SME_ADDTS_CNF\n";
        case eWNI_SME_ADDTS_IND:
            return "eWNI_SME_ADDTS_IND\n";
        case eWNI_SME_DELTS_REQ:
            return "eWNI_SME_DELTS_REQ\n";
        case eWNI_SME_DELTS_RSP:
            return "eWNI_SME_DELTS_RSP\n";
        case eWNI_SME_DELTS_IND:
            return "eWNI_SME_DELTS_IND\n";

        case SIR_LIM_RESUME_ACTIVITY_NTF:
            return "SIR_LIM_RESUME_ACTIVITY_NTF\n";
        case SIR_LIM_SUSPEND_ACTIVITY_REQ:
            return "SIR_LIM_SUSPEND_ACTIVITY_REQ\n";
        case WDA_SUSPEND_ACTIVITY_RSP:
            return "WDA_SUSPEND_ACTIVITY_RSP\n";
        case SIR_LIM_RETRY_INTERRUPT_MSG:
            return "SIR_LIM_RETRY_INTERRUPT_MSG\n";
        case SIR_BB_XPORT_MGMT_MSG:
            return "SIR_BB_XPORT_MGMT_MSG\n";
        case SIR_LIM_INV_KEY_INTERRUPT_MSG:
            return "SIR_LIM_INV_KEY_INTERRUPT_MSG\n";
        case SIR_LIM_KEY_ID_INTERRUPT_MSG:
            return "SIR_LIM_KEY_ID_INTERRUPT_MSG\n";
        case SIR_LIM_REPLAY_THRES_INTERRUPT_MSG:
            return "SIR_LIM_REPLAY_THRES_INTERRUPT_MSG\n";
        case SIR_LIM_MIN_CHANNEL_TIMEOUT:
            return "SIR_LIM_MIN_CHANNEL_TIMEOUT\n";
        case SIR_LIM_MAX_CHANNEL_TIMEOUT:
            return "SIR_LIM_MAX_CHANNEL_TIMEOUT\n";
        case SIR_LIM_JOIN_FAIL_TIMEOUT:
            return "SIR_LIM_JOIN_FAIL_TIMEOUT\n";
        case SIR_LIM_AUTH_FAIL_TIMEOUT:
            return "SIR_LIM_AUTH_FAIL_TIMEOUT\n";
        case SIR_LIM_AUTH_RSP_TIMEOUT:
            return "SIR_LIM_AUTH_RSP_TIMEOUT\n";
        case SIR_LIM_ASSOC_FAIL_TIMEOUT:
            return "SIR_LIM_ASSOC_FAIL_TIMEOUT\n";
        case SIR_LIM_REASSOC_FAIL_TIMEOUT:
            return "SIR_LIM_REASSOC_FAIL_TIMEOUT\n";
        case SIR_LIM_HEART_BEAT_TIMEOUT:
            return "SIR_LIM_HEART_BEAT_TIMEOUT\n";
#ifdef ANI_PRODUCT_TYPE_AP
        case SIR_LIM_PREAUTH_CLNUP_TIMEOUT:
            return "SIR_LIM_PREAUTH_CLNUP_TIMEOUT\n";
#endif
        case SIR_LIM_ADDTS_RSP_TIMEOUT:
            return "SIR_LIM_ADDTS_RSP_TIMEOUT\n";
        case SIR_LIM_CHANNEL_SCAN_TIMEOUT:
            return "SIR_LIM_CHANNEL_SCAN_TIMEOUT\n";
#if defined(ANI_PRODUCT_TYPE_AP) && (WNI_POLARIS_FW_PACKAGE == ADVANCED)
        case SIR_LIM_MEASUREMENT_IND_TIMEOUT:
            return "SIR_LIM_MEASUREMENT_IND_TIMEOUT\n";
        case SIR_LIM_LEARN_INTERVAL_TIMEOUT:
            return "SIR_LIM_LEARN_INTERVAL_TIMEOUT\n";
        case SIR_LIM_LEARN_DURATION_TIMEOUT:
            return "SIR_LIM_LEARN_DURATION_TIMEOUT\n";
#endif
        case SIR_LIM_LINK_TEST_DURATION_TIMEOUT:
            return "SIR_LIM_LINK_TEST_DURATION_TIMEOUT\n";
        case SIR_LIM_HASH_MISS_THRES_TIMEOUT:
            return "SIR_LIM_HASH_MISS_THRES_TIMEOUT\n";
        case SIR_LIM_KEEPALIVE_TIMEOUT:
            return "SIR_LIM_KEEPALIVE_TIMEOUT\n";
        case SIR_LIM_UPDATE_OLBC_CACHEL_TIMEOUT:
            return "SIR_LIM_UPDATE_OLBC_CACHEL_TIMEOUT\n";
        case SIR_LIM_CNF_WAIT_TIMEOUT:
            return "SIR_LIM_CNF_WAIT_TIMEOUT\n";
        case SIR_LIM_RADAR_DETECT_IND:
            return "SIR_LIM_RADAR_DETECT_IND\n";
#ifdef WLAN_FEATURE_VOWIFI_11R
        case SIR_LIM_FT_PREAUTH_RSP_TIMEOUT:
            return "SIR_LIM_FT_PREAUTH_RSP_TIMEOUT\n";
#endif

        case SIR_HAL_APP_SETUP_NTF:
            return "SIR_HAL_APP_SETUP_NTF\n";
        case SIR_HAL_INITIAL_CAL_FAILED_NTF:
            return "SIR_HAL_INITIAL_CAL_FAILED_NTF\n";
        case SIR_HAL_NIC_OPER_NTF:
            return "SIR_HAL_NIC_OPER_NTF\n";
        case SIR_HAL_INIT_START_REQ:
            return "SIR_HAL_INIT_START_REQ\n";
        case SIR_HAL_SHUTDOWN_REQ:
            return "SIR_HAL_SHUTDOWN_REQ\n";
        case SIR_HAL_SHUTDOWN_CNF:
            return "SIR_HAL_SHUTDOWN_CNF\n";
        case SIR_HAL_RESET_REQ:
            return "SIR_HAL_RESET_REQ\n";
        case SIR_HAL_RESET_CNF:
            return "SIR_HAL_RESET_CNF\n";
        case SIR_WRITE_TO_TD:
            return "SIR_WRITE_TO_TD\n";

        case WNI_CFG_PARAM_UPDATE_IND:
            return "WNI_CFG_PARAM_UPDATE_IND\n";
        case WNI_CFG_DNLD_REQ:
            return "WNI_CFG_DNLD_REQ\n";
        case WNI_CFG_DNLD_CNF:
            return "WNI_CFG_DNLD_CNF\n";
        case WNI_CFG_GET_RSP:
            return "WNI_CFG_GET_RSP\n";
        case WNI_CFG_SET_CNF:
            return "WNI_CFG_SET_CNF\n";
        case WNI_CFG_GET_ATTRIB_RSP:
            return "WNI_CFG_GET_ATTRIB_RSP\n";
        case WNI_CFG_ADD_GRP_ADDR_CNF:
            return "WNI_CFG_ADD_GRP_ADDR_CNF\n";
        case WNI_CFG_DEL_GRP_ADDR_CNF:
            return "WNI_CFG_DEL_GRP_ADDR_CNF\n";
        case ANI_CFG_GET_RADIO_STAT_RSP:
            return "ANI_CFG_GET_RADIO_STAT_RSP\n";
        case ANI_CFG_GET_PER_STA_STAT_RSP:
            return "ANI_CFG_GET_PER_STA_STAT_RSP\n";
        case ANI_CFG_GET_AGG_STA_STAT_RSP:
            return "ANI_CFG_GET_AGG_STA_STAT_RSP\n";
        case ANI_CFG_CLEAR_STAT_RSP:
            return "ANI_CFG_CLEAR_STAT_RSP\n";
        case WNI_CFG_DNLD_RSP:
            return "WNI_CFG_DNLD_RSP\n";
        case WNI_CFG_GET_REQ:
            return "WNI_CFG_GET_REQ\n";
        case WNI_CFG_SET_REQ:
            return "WNI_CFG_SET_REQ\n";
        case WNI_CFG_SET_REQ_NO_RSP:
            return "WNI_CFG_SET_REQ_NO_RSP\n";
        case eWNI_PMC_ENTER_IMPS_RSP:
            return "eWNI_PMC_ENTER_IMPS_RSP\n";
        case eWNI_PMC_EXIT_IMPS_RSP:
            return "eWNI_PMC_EXIT_IMPS_RSP\n";
        case eWNI_PMC_ENTER_BMPS_RSP:
            return "eWNI_PMC_ENTER_BMPS_RSP\n";
        case eWNI_PMC_EXIT_BMPS_RSP:
            return "eWNI_PMC_EXIT_BMPS_RSP\n";
        case eWNI_PMC_EXIT_BMPS_IND:
            return "eWNI_PMC_EXIT_BMPS_IND\n";
        default:
            return "INVALID SME message\n";
    }
#endif
return "";
}



char *limResultCodeStr(tSirResultCodes resultCode)
{
#ifdef FIXME_GEN6
    switch (resultCode)
    {
      case eSIR_SME_SUCCESS:
            return "eSIR_SME_SUCCESS\n";
      case eSIR_EOF_SOF_EXCEPTION:
            return "eSIR_EOF_SOF_EXCEPTION\n";
      case eSIR_BMU_EXCEPTION:
            return "eSIR_BMU_EXCEPTION\n";
      case eSIR_LOW_PDU_EXCEPTION:
            return "eSIR_LOW_PDU_EXCEPTION\n";
      case eSIR_USER_TRIG_RESET:
            return"eSIR_USER_TRIG_RESET\n";
      case eSIR_LOGP_EXCEPTION:
            return "eSIR_LOGP_EXCEPTION\n";
      case eSIR_CP_EXCEPTION:
            return "eSIR_CP_EXCEPTION\n";
      case eSIR_STOP_BSS:
            return "eSIR_STOP_BSS\n";
      case eSIR_AHB_HANG_EXCEPTION:
            return "eSIR_AHB_HANG_EXCEPTION\n";
      case eSIR_DPU_EXCEPTION:
            return "eSIR_DPU_EXCEPTION\n";
      case eSIR_RXP_EXCEPTION:
            return "eSIR_RXP_EXCEPTION\n";
      case eSIR_MCPU_EXCEPTION:
            return "eSIR_MCPU_EXCEPTION\n";
      case eSIR_MCU_EXCEPTION:
            return "eSIR_MCU_EXCEPTION\n";
      case eSIR_MTU_EXCEPTION:
            return "eSIR_MTU_EXCEPTION\n";
      case eSIR_MIF_EXCEPTION:
            return "eSIR_MIF_EXCEPTION\n";
      case eSIR_FW_EXCEPTION:
            return "eSIR_FW_EXCEPTION\n";
      case eSIR_MAILBOX_SANITY_CHK_FAILED:
            return "eSIR_MAILBOX_SANITY_CHK_FAILED\n";
      case eSIR_RADIO_HW_SWITCH_STATUS_IS_OFF:
            return "eSIR_RADIO_HW_SWITCH_STATUS_IS_OFF\n";
      case eSIR_CFB_FLAG_STUCK_EXCEPTION:
            return "eSIR_CFB_FLAG_STUCK_EXCEPTION\n";
      case eSIR_SME_BASIC_RATES_NOT_SUPPORTED_STATUS:
            return "eSIR_SME_BASIC_RATES_NOT_SUPPORTED_STATUS\n";
      case eSIR_SME_INVALID_PARAMETERS:
            return "eSIR_SME_INVALID_PARAMETERS\n";
      case eSIR_SME_UNEXPECTED_REQ_RESULT_CODE:
            return "eSIR_SME_UNEXPECTED_REQ_RESULT_CODE\n";
      case eSIR_SME_RESOURCES_UNAVAILABLE:
            return "eSIR_SME_RESOURCES_UNAVAILABLE\n";
      case eSIR_SME_SCAN_FAILED:
            return "eSIR_SME_SCAN_FAILED\n";
      case eSIR_SME_BSS_ALREADY_STARTED_OR_JOINED:
            return "eSIR_SME_BSS_ALREADY_STARTED_OR_JOINED\n";
      case eSIR_SME_LOST_LINK_WITH_PEER_RESULT_CODE:
            return "eSIR_SME_LOST_LINK_WITH_PEER_RESULT_CODE\n";
      case eSIR_SME_REFUSED:
            return "eSIR_SME_REFUSED\n";
      case eSIR_SME_JOIN_TIMEOUT_RESULT_CODE:
            return "eSIR_SME_JOIN_TIMEOUT_RESULT_CODE\n";
      case eSIR_SME_AUTH_TIMEOUT_RESULT_CODE:
            return "eSIR_SME_AUTH_TIMEOUT_RESULT_CODE\n";
      case eSIR_SME_ASSOC_TIMEOUT_RESULT_CODE:
            return "eSIR_SME_ASSOC_TIMEOUT_RESULT_CODE\n";
      case eSIR_SME_REASSOC_TIMEOUT_RESULT_CODE:
            return "eSIR_SME_REASSOC_TIMEOUT_RESULT_CODE\n";
      case eSIR_SME_MAX_NUM_OF_PRE_AUTH_REACHED:
            return "eSIR_SME_MAX_NUM_OF_PRE_AUTH_REACHED\n";
      case eSIR_SME_AUTH_REFUSED:
            return "eSIR_SME_AUTH_REFUSED\n";
      case eSIR_SME_INVALID_WEP_DEFAULT_KEY:
            return "eSIR_SME_INVALID_WEP_DEFAULT_KEY\n";
      case eSIR_SME_ASSOC_REFUSED:
            return "eSIR_SME_ASSOC_REFUSED\n";
      case eSIR_SME_REASSOC_REFUSED:
            return "eSIR_SME_REASSOC_REFUSED\n";
      case eSIR_SME_STA_NOT_AUTHENTICATED:
            return "eSIR_SME_STA_NOT_AUTHENTICATED\n";
      case eSIR_SME_STA_NOT_ASSOCIATED:
            return "eSIR_SME_STA_NOT_ASSOCIATED\n";
      case eSIR_SME_STA_DISASSOCIATED:
            return "eSIR_SME_STA_DISASSOCIATED\n";
      case eSIR_SME_ALREADY_JOINED_A_BSS:
            return "eSIR_SME_ALREADY_JOINED_A_BSS\n";
      case eSIR_ULA_COMPLETED:
            return "eSIR_ULA_COMPLETED\n";
      case eSIR_ULA_FAILURE:
            return "eSIR_ULA_FAILURE\n";
      case eSIR_SME_LINK_ESTABLISHED:
            return "eSIR_SME_LINK_ESTABLISHED\n";
      case eSIR_SME_UNABLE_TO_PERFORM_MEASUREMENTS:
            return "eSIR_SME_UNABLE_TO_PERFORM_MEASUREMENTS\n";
      case eSIR_SME_UNABLE_TO_PERFORM_DFS:
            return "eSIR_SME_UNABLE_TO_PERFORM_DFS\n";
      case eSIR_SME_DFS_FAILED:
            return "eSIR_SME_DFS_FAILED\n";
      case eSIR_SME_TRANSFER_STA:
            return "eSIR_SME_TRANSFER_STA\n";
      case eSIR_SME_INVALID_LINK_TEST_PARAMETERS:
            return "eSIR_SME_INVALID_LINK_TEST_PARAMETERS\n";
      case eSIR_SME_LINK_TEST_MAX_EXCEEDED:
            return "eSIR_SME_LINK_TEST_MAX_EXCEEDED\n";
      case eSIR_SME_UNSUPPORTED_RATE:
            return "eSIR_SME_UNSUPPORTED_RATE\n";
      case eSIR_SME_LINK_TEST_TIMEOUT:
            return "eSIR_SME_LINK_TEST_TIMEOUT\n";
      case eSIR_SME_LINK_TEST_COMPLETE:
            return "eSIR_SME_LINK_TEST_COMPLETE\n";
      case eSIR_SME_LINK_TEST_INVALID_STATE:
            return "eSIR_SME_LINK_TEST_INVALID_STATE\n";
      case eSIR_SME_LINK_TEST_INVALID_ADDRESS:
            return "eSIR_SME_LINK_TEST_INVALID_ADDRESS\n";
      case eSIR_SME_POLARIS_RESET:
            return "eSIR_SME_POLARIS_RESET\n";
      case eSIR_SME_SETCONTEXT_FAILED:
            return "eSIR_SME_SETCONTEXT_FAILED\n";
      case eSIR_SME_BSS_RESTART:
            return "eSIR_SME_BSS_RESTART\n";
      case eSIR_SME_MORE_SCAN_RESULTS_FOLLOW:
            return "eSIR_SME_MORE_SCAN_RESULTS_FOLLOW\n";
      case eSIR_SME_INVALID_ASSOC_RSP_RXED:
            return "eSIR_SME_INVALID_ASSOC_RSP_RXED\n";
      case eSIR_SME_MIC_COUNTER_MEASURES:
            return "eSIR_SME_MIC_COUNTER_MEASURES\n";
      case eSIR_SME_ADDTS_RSP_TIMEOUT:
            return "eSIR_SME_ADDTS_RSP_TIMEOUT\n";
      case eSIR_SME_RECEIVED:
            return "eSIR_SME_RECEIVED\n";
      case eSIR_SME_CHANNEL_SWITCH_FAIL:
            return "eSIR_SME_CHANNEL_SWITCH_FAIL\n";
#ifdef GEN4_SCAN
      case eSIR_SME_CHANNEL_SWITCH_DISABLED:
            return "eSIR_SME_CHANNEL_SWITCH_DISABLED\n";
      case eSIR_SME_HAL_SCAN_INIT_FAILED:
            return "eSIR_SME_HAL_SCAN_INIT_FAILED\n";
      case eSIR_SME_HAL_SCAN_START_FAILED:
            return "eSIR_SME_HAL_SCAN_START_FAILED\n";
      case eSIR_SME_HAL_SCAN_END_FAILED:
            return "eSIR_SME_HAL_SCAN_END_FAILED\n";
      case eSIR_SME_HAL_SCAN_FINISH_FAILED:
            return "eSIR_SME_HAL_SCAN_FINISH_FAILED\n";
      case eSIR_SME_HAL_SEND_MESSAGE_FAIL:
            return "eSIR_SME_HAL_SEND_MESSAGE_FAIL\n";
#else // GEN4_SCAN
      case eSIR_SME_CHANNEL_SWITCH_DISABLED:
            return "eSIR_SME_CHANNEL_SWITCH_DISABLED\n";
      case eSIR_SME_HAL_SEND_MESSAGE_FAIL:
            return "eSIR_SME_HAL_SEND_MESSAGE_FAIL\n";
#endif // GEN4_SCAN

        default:
            return "INVALID resultCode\n";
    }
#endif
return "";
}

void
limPrintMsgName(tpAniSirGlobal pMac, tANI_U16 logLevel, tANI_U32 msgType)
{
    limLog(pMac, logLevel, limMsgStr(msgType));
}


#if defined(ANI_MIPS) || defined(ANI_ARM)
#define LINK 0
#else
#define LINK 1
#endif

void
limPrintMsgInfo(tpAniSirGlobal pMac, tANI_U16 logLevel, tSirMsgQ *msg)
{
    //tSirMacFrameCtl  fc; // FIXME_GEN4 - ASAP!!
#if defined (ANI_OS_TYPE_LINUX) || defined (ANI_OS_TYPE_OSX)
    tANI_U32              *pRxPacketInfo;
#endif
    if (logLevel <= pMac->utils.gLogDbgLevel[SIR_LIM_MODULE_ID - LOG_FIRST_MODULE_ID])
    {
        switch (msg->type)
        {
            case SIR_BB_XPORT_MGMT_MSG:
#if defined (ANI_OS_TYPE_LINUX) || defined (ANI_OS_TYPE_OSX)
#ifndef GEN6_ONWARDS //PAL does not provide this API GEN6 onwards.
                palGetPacketDataPtr( pMac->hHdd, HAL_TXRX_FRM_802_11_MGMT, (void *) msg->bodyptr, (void **) &pRxPacketInfo );
#endif //GEN6_ONWARDS
#else
                limPrintMsgName(pMac, logLevel,msg->type);
#endif
                break;
            default:
                limPrintMsgName(pMac, logLevel,msg->type);
                break;
        }
    }
}

/**
 * limInitMlm()
 *
 *FUNCTION:
 * This function is called by limProcessSmeMessages() to
 * initialize MLM state machine on STA
 *
 *PARAMS:
 *
 *LOGIC:
 *
 *ASSUMPTIONS:
 * NA
 *
 *NOTE:
 * NA
 *
 * @param  pMac      Pointer to Global MAC structure
 * @return None
 */
void
limInitMlm(tpAniSirGlobal pMac)
{
    MTRACE(macTrace(pMac, TRACE_CODE_MLM_STATE, 0, pMac->lim.gLimMlmState));

    /// Initialize scan result hash table
    limReInitScanResults(pMac); //sep26th review

  
    /// Initialize number of pre-auth contexts
    pMac->lim.gLimNumPreAuthContexts = 0;

    /// Initialize MAC based Authentication STA list
    limInitPreAuthList(pMac);

    //pMac->lim.gpLimMlmJoinReq = NULL;

    if (pMac->lim.gLimTimersCreated)
        return;

    // Create timers used by LIM
    limCreateTimers(pMac);

    pMac->lim.gLimTimersCreated = 1;
} /*** end limInitMlm() ***/



/**
 * limCleanupMlm()
 *
 *FUNCTION:
 * This function is called to cleanup any resources
 * allocated by the  MLM state machine.
 *
 *PARAMS:
 *
 *LOGIC:
 *
 *ASSUMPTIONS:
 * NA
 *
 *NOTE:
 * It is assumed that BSS is already informed that we're leaving it
 * before this function is called.
 *
 * @param  pMac      Pointer to Global MAC structure
 * @param  None
 * @return None
 */
void
limCleanupMlm(tpAniSirGlobal pMac)
{
    tANI_U32   n;
    tLimPreAuthNode *pAuthNode;

    if (pMac->lim.gLimTimersCreated == 1)
    {
        // Deactivate and delete MIN/MAX channel timers.
        tx_timer_deactivate(&pMac->lim.limTimers.gLimMinChannelTimer);
        tx_timer_delete(&pMac->lim.limTimers.gLimMinChannelTimer);
        tx_timer_deactivate(&pMac->lim.limTimers.gLimMaxChannelTimer);
        tx_timer_delete(&pMac->lim.limTimers.gLimMaxChannelTimer);


        // Deactivate and delete channel switch timer.
        tx_timer_deactivate(&pMac->lim.limTimers.gLimChannelSwitchTimer);
        tx_timer_delete(&pMac->lim.limTimers.gLimChannelSwitchTimer);


        // Deactivate and delete addts response timer.
        tx_timer_deactivate(&pMac->lim.limTimers.gLimAddtsRspTimer);
        tx_timer_delete(&pMac->lim.limTimers.gLimAddtsRspTimer);

        // Deactivate and delete Join failure timer.
        tx_timer_deactivate(&pMac->lim.limTimers.gLimJoinFailureTimer);
        tx_timer_delete(&pMac->lim.limTimers.gLimJoinFailureTimer);

        // Deactivate and delete Association failure timer.
        tx_timer_deactivate(&pMac->lim.limTimers.gLimAssocFailureTimer);
        tx_timer_delete(&pMac->lim.limTimers.gLimAssocFailureTimer);

        // Deactivate and delete Reassociation failure timer.
        tx_timer_deactivate(&pMac->lim.limTimers.gLimReassocFailureTimer);
        tx_timer_delete(&pMac->lim.limTimers.gLimReassocFailureTimer);

        // Deactivate and delete Authentication failure timer.
        tx_timer_deactivate(&pMac->lim.limTimers.gLimAuthFailureTimer);
        tx_timer_delete(&pMac->lim.limTimers.gLimAuthFailureTimer);

        // Deactivate and delete Heartbeat timer.
        tx_timer_deactivate(&pMac->lim.limTimers.gLimHeartBeatTimer);
        tx_timer_delete(&pMac->lim.limTimers.gLimHeartBeatTimer);

        // Deactivate and delete wait-for-probe-after-Heartbeat timer.
        tx_timer_deactivate(&pMac->lim.limTimers.gLimProbeAfterHBTimer);
        tx_timer_delete(&pMac->lim.limTimers.gLimProbeAfterHBTimer);

        // Deactivate and delete Quiet timer.
        tx_timer_deactivate(&pMac->lim.limTimers.gLimQuietTimer);
        tx_timer_delete(&pMac->lim.limTimers.gLimQuietTimer);

        // Deactivate and delete Quiet BSS timer.
        tx_timer_deactivate(&pMac->lim.limTimers.gLimQuietBssTimer);
        tx_timer_delete(&pMac->lim.limTimers.gLimQuietBssTimer);

#if defined(ANI_PRODUCT_TYPE_CLIENT) || defined(ANI_AP_CLIENT_SDK)
        // Deactivate and delete LIM background scan timer.
        tx_timer_deactivate(&pMac->lim.limTimers.gLimBackgroundScanTimer);
        tx_timer_delete(&pMac->lim.limTimers.gLimBackgroundScanTimer);
#endif


        // Deactivate and delete cnf wait timer
        for (n = 0; n < pMac->lim.maxStation; n++)
        {
            tx_timer_deactivate(&pMac->lim.limTimers.gpLimCnfWaitTimer[n]);
            tx_timer_delete(&pMac->lim.limTimers.gpLimCnfWaitTimer[n]);
        }

        // Deactivate and delete keepalive timer
        tx_timer_deactivate(&pMac->lim.limTimers.gLimKeepaliveTimer);
        tx_timer_delete(&pMac->lim.limTimers.gLimKeepaliveTimer);

        pAuthNode = pMac->lim.gLimPreAuthTimerTable.pTable;
        
        //Deactivate any Authentication response timers
        limDeletePreAuthList(pMac);

	for (n = 0; n < pMac->lim.gLimPreAuthTimerTable.numEntry; n++,pAuthNode++)
	{
		// Delete any Authentication response
		// timers, which might have been started.
		tx_timer_delete(&pAuthNode->timer);
	}

#ifdef ANI_PRODUCT_TYPE_AP

        if (pMac->lim.gLimSystemRole == eLIM_AP_ROLE)
        {
            // Deactivate and cleanup the periodic pre-auth
            // cleanup timer
      
            tx_timer_deactivate(&pMac->lim.limTimers.gLimPreAuthClnupTimer);
            tx_timer_delete(&pMac->lim.limTimers.gLimPreAuthClnupTimer);

            /// Deactivate and delete OLBC cache update timeout
            tx_timer_deactivate(&pMac->lim.limTimers.gLimUpdateOlbcCacheTimer);
            tx_timer_delete(&pMac->lim.limTimers.gLimUpdateOlbcCacheTimer);

        }
#endif


        // Deactivate and delete Hash Miss throttle timer
        tx_timer_deactivate(&pMac->lim.limTimers.gLimSendDisassocFrameThresholdTimer);
        tx_timer_delete(&pMac->lim.limTimers.gLimSendDisassocFrameThresholdTimer);
        
#ifdef WLAN_SOFTAP_FEATURE
        tx_timer_deactivate(&pMac->lim.limTimers.gLimUpdateOlbcCacheTimer);
        tx_timer_delete(&pMac->lim.limTimers.gLimUpdateOlbcCacheTimer);
		tx_timer_deactivate(&pMac->lim.limTimers.gLimPreAuthClnupTimer);
        tx_timer_delete(&pMac->lim.limTimers.gLimPreAuthClnupTimer);

#if 0 // The WPS PBC clean up timer is disabled
        if (pMac->lim.gLimSystemRole == eLIM_AP_ROLE)
        {
            if(pMac->lim.limTimers.gLimWPSOverlapTimerObj.isTimerCreated == eANI_BOOLEAN_TRUE)
            {
                tx_timer_deactivate(&pMac->lim.limTimers.gLimWPSOverlapTimerObj.gLimWPSOverlapTimer);
                tx_timer_delete(&pMac->lim.limTimers.gLimWPSOverlapTimerObj.gLimWPSOverlapTimer);
                pMac->lim.limTimers.gLimWPSOverlapTimerObj.isTimerCreated = eANI_BOOLEAN_FALSE;
            }
        }
#endif
#endif
#ifdef WLAN_FEATURE_VOWIFI_11R
        // Deactivate and delete FT Preauth response timer
        tx_timer_deactivate(&pMac->lim.limTimers.gLimFTPreAuthRspTimer);
        tx_timer_delete(&pMac->lim.limTimers.gLimFTPreAuthRspTimer);
#endif
        pMac->lim.gLimTimersCreated = 0;
    }

    /// Cleanup cached scan list
    limReInitScanResults(pMac);

} /*** end limCleanupMlm() ***/



/**
 * limCleanupLmm()
 *
 *FUNCTION:
 * This function is called to cleanup any resources
 * allocated by LMM sub-module.
 *
 *PARAMS:
 *
 *LOGIC:
 *
 *ASSUMPTIONS:
 * NA
 *
 *NOTE:
 * NA
 *
 * @param  pMac      Pointer to Global MAC structure
 * @return None
 */

void
limCleanupLmm(tpAniSirGlobal pMac)
{
#if (WNI_POLARIS_FW_PACKAGE == ADVANCED) && defined (ANI_PRODUCT_TYPE_AP)
    limCleanupMeasResources(pMac);
    pMac->sys.gSysEnableLearnMode = eANI_BOOLEAN_FALSE;
#endif
} /*** end limCleanupLmm() ***/



/**
 * limIsAddrBC()
 *
 *FUNCTION:
 * This function is called in various places within LIM code
 * to determine whether passed MAC address is a broadcast or not
 *
 *LOGIC:
 *
 *ASSUMPTIONS:
 * NA
 *
 *NOTE:
 * NA
 *
 * @param macAddr  Indicates MAC address that need to be determined
 *                 whether it is Broadcast address or not
 *
 * @return true if passed address is Broadcast address else false
 */

tANI_U8
limIsAddrBC(tSirMacAddr macAddr)
{
    int i;
    for (i = 0; i < 6; i++)
    {
        if ((macAddr[i] & 0xFF) != 0xFF)
            return false;
    }

    return true;
} /****** end limIsAddrBC() ******/



/**
 * limIsGroupAddr()
 *
 *FUNCTION:
 * This function is called in various places within LIM code
 * to determine whether passed MAC address is a group address or not
 *
 *LOGIC:
 * If least significant bit of first octet of the MAC address is
 * set to 1, it is a Group address.
 *
 *ASSUMPTIONS:
 * NA
 *
 *NOTE:
 * NA
 *
 * @param macAddr  Indicates MAC address that need to be determined
 *                 whether it is Group address or not
 *
 * @return true if passed address is Group address else false
 */

tANI_U8
limIsGroupAddr(tSirMacAddr macAddr)
{
    if ((macAddr[0] & 0x01) == 0x01)
        return true;
    else
        return false;
} /****** end limIsGroupAddr() ******/

/**
 * limPostMsgApiNoWait()
 *
 *FUNCTION:
 * This function is called from other thread while posting a
 * message to LIM message Queue gSirLimMsgQ with NO_WAIT option
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
 * @param  pMsg - Pointer to the Global MAC structure
 * @param  pMsg - Pointer to the message structure
 * @return None
 */

tANI_U32
limPostMsgApiNoWait(tpAniSirGlobal pMac, tSirMsgQ *pMsg)
{
#ifdef ANI_OS_TYPE_WINDOWS
    tANI_U32 retCode;

    if ((retCode = tx_queue_send(&pMac->sys.gSirLimMsgQ, pMsg, TX_NO_WAIT))
        != TX_SUCCESS)
    {
        // Failure in sending DFS duration timeout indication
        // to LIM thread

        // Log error
        limLog(pMac, LOGP,
               FL("could not post a message %X to LIM msgq, status=%d\n"),
               pMsg->type, retCode);
    }

    return retCode;
#else
    limProcessMessages(pMac, pMsg);
    return TX_SUCCESS;
#endif
} /*** end limPostMsgApiNoWait() ***/



/**
 * limPrintMacAddr()
 *
 *FUNCTION:
 * This function is called to print passed MAC address
 * in : format.
 *
 *LOGIC:
 *
 *ASSUMPTIONS:
 * NA
 *
 *NOTE:
 * @param  macAddr  - MacAddr to be printed
 * @param  logLevel - Loglevel to be used
 *
 * @return None.
 */

void
limPrintMacAddr(tpAniSirGlobal pMac, tSirMacAddr macAddr, tANI_U8 logLevel)
{
    limLog(pMac, logLevel,
           FL("%X:%X:%X:%X:%X:%X\n"),
           macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4],
           macAddr[5]);
} /****** end limPrintMacAddr() ******/






/*
 * limResetDeferredMsgQ()
 *
 *FUNCTION:
 * This function resets the deferred message queue parameters.
 *
 *PARAMS:
 * @param pMac     - Pointer to Global MAC structure
 *
 *LOGIC:
 *
 *ASSUMPTIONS:
 * NA
 *
 *NOTE:
 * NA
 *
 *RETURNS:
 * None
 */

void limResetDeferredMsgQ(tpAniSirGlobal pMac)
{
    pMac->lim.gLimDeferredMsgQ.size =
    pMac->lim.gLimDeferredMsgQ.write =
    pMac->lim.gLimDeferredMsgQ.read = 0;

}


#define LIM_DEFERRED_Q_CHECK_THRESHOLD  (MAX_DEFERRED_QUEUE_LEN/2)
#define LIM_MAX_NUM_MGMT_FRAME_DEFERRED (MAX_DEFERRED_QUEUE_LEN/2)

/*
 * limWriteDeferredMsgQ()
 *
 *FUNCTION:
 * This function queues up a deferred message for later processing on the
 * STA side.
 *
 *PARAMS:
 * @param pMac     - Pointer to Global MAC structure
 * @param limMsg   - a LIM message
 *
 *LOGIC:
 *
 *ASSUMPTIONS:
 * NA
 *
 *NOTE:
 * NA
 *
 *RETURNS:
 * None
 */

tANI_U8 limWriteDeferredMsgQ(tpAniSirGlobal pMac, tpSirMsgQ limMsg)
{
    PELOG1(limLog(pMac, LOG1,
           FL("**  Queue a deferred message (size %d, write %d) - type 0x%x  **\n"),
           pMac->lim.gLimDeferredMsgQ.size, pMac->lim.gLimDeferredMsgQ.write,
           limMsg->type);)

    /*
    ** check if the deferred message queue is full
    **/
    if (pMac->lim.gLimDeferredMsgQ.size >= MAX_DEFERRED_QUEUE_LEN)
    {
        PELOGE(limLog(pMac, LOGE, FL("Deferred Message Queue is full. Msg: %d\n"), limMsg->type);)
        return TX_QUEUE_FULL;
    }

    /*
    ** In the application, there should not be more than 1 message get
    ** queued up. If happens, flags a warning. In the future, this can
    ** happen.
    **/
    if (pMac->lim.gLimDeferredMsgQ.size > 0)
    {
        PELOGW(limLog(pMac, LOGW, FL("%d Deferred messages (type 0x%x, scan %d, sme %d, mlme %d, addts %d)\n"),
               pMac->lim.gLimDeferredMsgQ.size, limMsg->type,
               limIsSystemInScanState(pMac),
               pMac->lim.gLimSmeState, pMac->lim.gLimMlmState,
               pMac->lim.gLimAddtsSent);)
    }

    /*
    ** To prevent the deferred Q is full of management frames, only give them certain space
    **/
    if( SIR_BB_XPORT_MGMT_MSG == limMsg->type )
    {
        if( LIM_DEFERRED_Q_CHECK_THRESHOLD < pMac->lim.gLimDeferredMsgQ.size )
        {
            tANI_U16 idx, count = 0;
            for(idx = 0; idx < pMac->lim.gLimDeferredMsgQ.size; idx++)
            {
                if( SIR_BB_XPORT_MGMT_MSG == pMac->lim.gLimDeferredMsgQ.deferredQueue[idx].type )
                {
                    count++;
                }
            }
            if( LIM_MAX_NUM_MGMT_FRAME_DEFERRED < count )
            {
                //We reach the quota for management frames, drop this one
                PELOGE(limLog(pMac, LOGE, FL("Cannot deferred. Msg: %d Too many (count=%d) already\n"), limMsg->type, count);)
               //Return error, caller knows what to do
               return TX_QUEUE_FULL;
            }
        }
    }

    ++pMac->lim.gLimDeferredMsgQ.size;

    /*
    ** if the write pointer hits the end of the queue, rewind it
    **/
    if (pMac->lim.gLimDeferredMsgQ.write >= MAX_DEFERRED_QUEUE_LEN)
        pMac->lim.gLimDeferredMsgQ.write = 0;

    /*
    ** save the message to the queue and advanced the write pointer
    **/
    palCopyMemory(pMac->hHdd,
                  (tANI_U8 *)&pMac->lim.gLimDeferredMsgQ.deferredQueue[pMac->lim.gLimDeferredMsgQ.write++],
                  (tANI_U8 *)limMsg,
                  sizeof(tSirMsgQ));
    return TX_SUCCESS;

}

/*
 * limReadDeferredMsgQ()
 *
 *FUNCTION:
 * This function dequeues a deferred message for processing on the
 * STA side.
 *
 *PARAMS:
 * @param pMac     - Pointer to Global MAC structure
 *
 *LOGIC:
 *
 *ASSUMPTIONS:
 * NA
 *
 *NOTE:
 *
 *
 *RETURNS:
 * Returns the message at the head of the deferred message queue
 */

tSirMsgQ* limReadDeferredMsgQ(tpAniSirGlobal pMac)
{
    tSirMsgQ    *msg;

    /*
    ** check any messages left. If no, return
    **/
    if (pMac->lim.gLimDeferredMsgQ.size <= 0)
        return NULL;

    /*
    ** decrement the queue size
    **/
    pMac->lim.gLimDeferredMsgQ.size--;

    /*
    ** retrieve the message from the head of the queue
    **/
    msg = &pMac->lim.gLimDeferredMsgQ.deferredQueue[pMac->lim.gLimDeferredMsgQ.read];

    /*
    ** advance the read pointer
    **/
    pMac->lim.gLimDeferredMsgQ.read++;

    /*
    ** if the read pointer hits the end of the queue, rewind it
    **/
    if (pMac->lim.gLimDeferredMsgQ.read >= MAX_DEFERRED_QUEUE_LEN)
        pMac->lim.gLimDeferredMsgQ.read = 0;

   PELOG1(limLog(pMac, LOG1,
           FL("**  DeQueue a deferred message (size %d read %d) - type 0x%x  **\n"),
           pMac->lim.gLimDeferredMsgQ.size, pMac->lim.gLimDeferredMsgQ.read,
           msg->type);)

   PELOG1(limLog(pMac, LOG1, FL("DQ msg -- scan %d, sme %d, mlme %d, addts %d\n"),
           limIsSystemInScanState(pMac),
           pMac->lim.gLimSmeState, pMac->lim.gLimMlmState,
           pMac->lim.gLimAddtsSent);)

    return(msg);
}

tSirRetStatus
limSysProcessMmhMsgApi(tpAniSirGlobal pMac,
                    tSirMsgQ *pMsg,
                    tANI_U8 qType)
{
// FIXME
#if defined( FEATURE_WLAN_INTEGRATED_SOC )
   SysProcessMmhMsg(pMac, pMsg);
   return eSIR_SUCCESS;
#else
   return(halMmhPostMsgApi(pMac, pMsg, qType));
#endif
}

char *limFrameStr(tANI_U32 type, tANI_U32 subType)
{
#ifdef FIXME_GEN6

    if (type == SIR_MAC_MGMT_FRAME)
    {
        switch (subType)
        {
            case SIR_MAC_MGMT_ASSOC_REQ:
                return "MAC_MGMT_ASSOC_REQ";
            case SIR_MAC_MGMT_ASSOC_RSP:
                return "MAC_MGMT_ASSOC_RSP";
            case SIR_MAC_MGMT_REASSOC_REQ:
                return "MAC_MGMT_REASSOC_REQ";
            case SIR_MAC_MGMT_REASSOC_RSP:
                return "MAC_MGMT_REASSOC_RSP";
            case SIR_MAC_MGMT_PROBE_REQ:
                return "MAC_MGMT_PROBE_REQ";
            case SIR_MAC_MGMT_PROBE_RSP:
                return "MAC_MGMT_PROBE_RSP";
            case SIR_MAC_MGMT_BEACON:
                return "MAC_MGMT_BEACON";
            case SIR_MAC_MGMT_ATIM:
                return "MAC_MGMT_ATIM";
            case SIR_MAC_MGMT_DISASSOC:
                return "MAC_MGMT_DISASSOC";
            case SIR_MAC_MGMT_AUTH:
                return "MAC_MGMT_AUTH";
            case SIR_MAC_MGMT_DEAUTH:
                return "MAC_MGMT_DEAUTH";
            case SIR_MAC_MGMT_ACTION:
                return "MAC_MGMT_ACTION";
            case SIR_MAC_MGMT_RESERVED15:
                return "MAC_MGMT_RESERVED15";
            default:
                return "Unknown MGMT Frame";
        }
    }

    else if (type == SIR_MAC_CTRL_FRAME)
    {
        switch (subType)
        {
            case SIR_MAC_CTRL_RR:
                return "MAC_CTRL_RR";
            case SIR_MAC_CTRL_BAR:
                return "MAC_CTRL_BAR";
            case SIR_MAC_CTRL_BA:
                return "MAC_CTRL_BA";
            case SIR_MAC_CTRL_PS_POLL:
                return "MAC_CTRL_PS_POLL";
            case SIR_MAC_CTRL_RTS:
                return "MAC_CTRL_RTS";
            case SIR_MAC_CTRL_CTS:
                return "MAC_CTRL_CTS";
            case SIR_MAC_CTRL_ACK:
                return "MAC_CTRL_ACK";
            case SIR_MAC_CTRL_CF_END:
                return "MAC_CTRL_CF_END";
            case SIR_MAC_CTRL_CF_END_ACK:
                return "MAC_CTRL_CF_END_ACK";
            default:
                return "Unknown CTRL Frame";
        }
    }

    else if (type == SIR_MAC_DATA_FRAME)
    {
        switch (subType)
        {
            case SIR_MAC_DATA_DATA:
                return "MAC_DATA_DATA";
            case SIR_MAC_DATA_DATA_ACK:
                return "MAC_DATA_DATA_ACK";
            case SIR_MAC_DATA_DATA_POLL:
                return "MAC_DATA_DATA_POLL";
            case SIR_MAC_DATA_DATA_ACK_POLL:
                return "MAC_DATA_DATA_ACK_POLL";
            case SIR_MAC_DATA_NULL:
                return "MAC_DATA_NULL";
            case SIR_MAC_DATA_NULL_ACK:
                return "MAC_DATA_NULL_ACK";
            case SIR_MAC_DATA_NULL_POLL:
                return "MAC_DATA_NULL_POLL";
            case SIR_MAC_DATA_NULL_ACK_POLL:
                return "MAC_DATA_NULL_ACK_POLL";
            case SIR_MAC_DATA_QOS_DATA:
                return "MAC_DATA_QOS_DATA";
            case SIR_MAC_DATA_QOS_DATA_ACK:
                return "MAC_DATA_QOS_DATA_ACK";
            case SIR_MAC_DATA_QOS_DATA_POLL:
                return "MAC_DATA_QOS_DATA_POLL";
            case SIR_MAC_DATA_QOS_DATA_ACK_POLL:
                return "MAC_DATA_QOS_DATA_ACK_POLL";
            case SIR_MAC_DATA_QOS_NULL:
                return "MAC_DATA_QOS_NULL";
            case SIR_MAC_DATA_QOS_NULL_ACK:
                return "MAC_DATA_QOS_NULL_ACK";
            case SIR_MAC_DATA_QOS_NULL_POLL:
                return "MAC_DATA_QOS_NULL_POLL";
            case SIR_MAC_DATA_QOS_NULL_ACK_POLL:
                return "MAC_DATA_QOS_NULL_ACK_POLL";
            default:
                return "Unknown Data Frame";
        }
    }
    else
        return "Unknown";
#endif
return "";
}

#ifdef WLAN_SOFTAP_FEATURE
void limHandleUpdateOlbcCache(tpAniSirGlobal pMac)
{
    int i;
    static int enable;
    tUpdateBeaconParams beaconParams;

    tpPESession       psessionEntry = limIsApSessionActive(pMac);
               
    if (psessionEntry == NULL)
        return;
    
    beaconParams.paramChangeBitmap = 0;
    /*
    ** This is doing a 2 pass check. The first pass is to invalidate
    ** all the cache entries. The second pass is to decide whether to
    ** disable protection.
    **/
    if (!enable)
    {

        if (eLIM_AP_ROLE == psessionEntry->limSystemRole)// Clear Session Cache
        {
        PELOG2(limLog(pMac, LOG2, FL("Resetting OLBC cache\n"));)
            psessionEntry->gLimOlbcParams.numSta = 0;
            psessionEntry->gLimOverlap11gParams.numSta = 0;
            psessionEntry->gLimOverlapHt20Params.numSta = 0;
            psessionEntry->gLimNonGfParams.numSta = 0;
            psessionEntry->gLimLsigTxopParams.numSta = 0;
        }

        pMac->lim.gLimOlbcParams.numSta = 0;
        pMac->lim.gLimOverlap11gParams.numSta = 0;
        pMac->lim.gLimOverlapHt20Params.numSta = 0;
        pMac->lim.gLimNonGfParams.numSta = 0;
        pMac->lim.gLimLsigTxopParams.numSta = 0;

        for (i=0; i < LIM_PROT_STA_OVERLAP_CACHE_SIZE; i++)
            pMac->lim.protStaOverlapCache[i].active = false;

        enable = 1;
    }
    else
    {

        if (!psessionEntry->gLimOverlap11gParams.numSta)
        {
            if (psessionEntry->gLimOlbcParams.protectionEnabled)
            {
                if (!psessionEntry->gLim11bParams.protectionEnabled)
                {
                    PELOG1(limLog(pMac, LOG1, FL("Overlap cache all clear and no 11B STA detected\n"));)
                    limEnable11gProtection(pMac, false, true, &beaconParams,psessionEntry);
                }
            }
        }

        if (!psessionEntry->gLimOverlap11gParams.numSta)
        {
            if (psessionEntry->gLimOverlap11gParams.protectionEnabled)
            {
                if (!psessionEntry->gLim11gParams.protectionEnabled)
                {
                    PELOG1(limLog(pMac, LOG1, FL("Overlap cache all clear and no 11G STA detected\n"));)
                    limEnableHtProtectionFrom11g(pMac, false, true, &beaconParams,psessionEntry);
                }
            }
        }

        if (!psessionEntry->gLimOverlapHt20Params.numSta)
        {
            if (psessionEntry->gLimOverlapHt20Params.protectionEnabled)
            {
                if (!psessionEntry->gLimHt20Params.protectionEnabled)
                {
                    PELOG1(limLog(pMac, LOG1, FL("Overlap cache all clear and no HT20 STA detected\n"));)
                    limEnable11gProtection(pMac, false, true, &beaconParams,psessionEntry);
                }
            }
        }

        enable = 0;
    }

    if(beaconParams.paramChangeBitmap)
    {
        schSetFixedBeaconFields(pMac,psessionEntry);
        limSendBeaconParams(pMac, &beaconParams, psessionEntry);
    }

    // Start OLBC timer
    if (tx_timer_activate(&pMac->lim.limTimers.gLimUpdateOlbcCacheTimer) != TX_SUCCESS)
    {
        limLog(pMac, LOGE, FL("tx_timer_activate failed\n"));
    }
}
#endif

/**
 * limIsNullSsid()
 *
 *FUNCTION:
 * This function checks if Ssid supplied is Null SSID
 *
 *
 *LOGIC:
 *
 *ASSUMPTIONS:
 * NA
 *
 *NOTE:
 * NA
 *
 * @param tSirMacSSid *
 *
 *
 * @return true if SSID is Null SSID else false
 */

tANI_U8
limIsNullSsid( tSirMacSSid *pSsid )
{
    tANI_U8 fNullSsid = false;
    tANI_U32 SsidLength;
    tANI_U8 *pSsidStr;

    do
    {
        if ( 0 == pSsid->length )
        {
            fNullSsid = true;
            break;
        }

        if ( (1 == pSsid->length) && (0x20 == pSsid->ssId[0]) )
        {
            fNullSsid = true;
            break;
        }

        SsidLength = pSsid->length;
        pSsidStr = pSsid->ssId;

        while ( SsidLength )
        {
            if( *pSsidStr )
                break;

            pSsidStr++;
            SsidLength--;
        }

        if( 0 == SsidLength )
        {
            fNullSsid = true;
            break;
        }
    }
    while( 0 );

    return fNullSsid;
} /****** end limIsNullSsid() ******/



#ifdef WLAN_SOFTAP_FEATURE

/** -------------------------------------------------------------
\fn limUpdateProtStaParams
\brief updates protection related counters.
\param      tpAniSirGlobal    pMac
\param      tSirMacAddr peerMacAddr
\param      tLimProtStaCacheType protStaCacheType
\param      tHalBitVal gfSupported
\param      tHalBitVal lsigTxopSupported
\return      None
  -------------------------------------------------------------*/
void
limUpdateProtStaParams(tpAniSirGlobal pMac,
tSirMacAddr peerMacAddr, tLimProtStaCacheType protStaCacheType,
tHalBitVal gfSupported, tHalBitVal lsigTxopSupported,
tpPESession psessionEntry)
{
  tANI_U32 i;

  PELOG1(limLog(pMac,LOG1, FL("A STA is associated:"));
  limLog(pMac,LOG1, FL("Addr : "));
  limPrintMacAddr(pMac, peerMacAddr, LOG1);)

  for (i=0; i<LIM_PROT_STA_CACHE_SIZE; i++)
  {
      if (psessionEntry->protStaCache[i].active)
      {
          PELOG1(limLog(pMac, LOG1, FL("Addr: "));)
          PELOG1(limPrintMacAddr(pMac, psessionEntry->protStaCache[i].addr, LOG1);)

          if (palEqualMemory( pMac->hHdd,
              psessionEntry->protStaCache[i].addr,
              peerMacAddr, sizeof(tSirMacAddr)))
          {
              PELOG1(limLog(pMac, LOG1, FL("matching cache entry at %d already active.\n"), i);)
              return;
          }
      }
  }

  for (i=0; i<LIM_PROT_STA_CACHE_SIZE; i++)
  {
      if (!psessionEntry->protStaCache[i].active)
          break;
  }

  if (i >= LIM_PROT_STA_CACHE_SIZE)
  {
      PELOGE(limLog(pMac, LOGE, FL("No space in ProtStaCache\n"));)
      return;
  }

  palCopyMemory( pMac->hHdd, psessionEntry->protStaCache[i].addr,
                peerMacAddr,
                sizeof(tSirMacAddr));

  psessionEntry->protStaCache[i].protStaCacheType = protStaCacheType;
  psessionEntry->protStaCache[i].active = true;
  if(eLIM_PROT_STA_CACHE_TYPE_llB == protStaCacheType)
  {
      psessionEntry->gLim11bParams.numSta++;
      limLog(pMac,LOG1, FL("11B, "));
  }
  else if(eLIM_PROT_STA_CACHE_TYPE_llG == protStaCacheType)
  {
      psessionEntry->gLim11gParams.numSta++;
      limLog(pMac,LOG1, FL("11G, "));
  }
  else   if(eLIM_PROT_STA_CACHE_TYPE_HT20 == protStaCacheType)
  {
      psessionEntry->gLimHt20Params.numSta++;
      limLog(pMac,LOG1, FL("HT20, "));
  }

  if(!gfSupported)
  {
     psessionEntry->gLimNonGfParams.numSta++;
      limLog(pMac,LOG1, FL("NonGf, "));
  }
  if(!lsigTxopSupported)
  {
      psessionEntry->gLimLsigTxopParams.numSta++;
      limLog(pMac,LOG1, FL("!lsigTxopSupported\n"));
  }
}// ---------------------------------------------------------------------

/** -------------------------------------------------------------
\fn limDecideApProtection
\brief Decides all the protection related staiton coexistence and also sets
\        short preamble and short slot appropriately. This function will be called
\        when AP is ready to send assocRsp tp the station joining right now.
\param      tpAniSirGlobal    pMac
\param      tSirMacAddr peerMacAddr
\return      None
  -------------------------------------------------------------*/
void
limDecideApProtection(tpAniSirGlobal pMac, tSirMacAddr peerMacAddr, tpUpdateBeaconParams pBeaconParams,tpPESession psessionEntry)
{
    tANI_U16              tmpAid;
    tpDphHashNode    pStaDs;
    tSirRFBand  rfBand = SIR_BAND_UNKNOWN;
    tANI_U32 phyMode;
    tLimProtStaCacheType protStaCacheType = eLIM_PROT_STA_CACHE_TYPE_INVALID;
    tHalBitVal gfSupported = eHAL_SET, lsigTxopSupported = eHAL_SET;

    pBeaconParams->paramChangeBitmap = 0;
    // check whether to enable protection or not
    pStaDs = dphLookupHashEntry(pMac, peerMacAddr, &tmpAid, &psessionEntry->dph.dphHashTable);
    if(NULL == pStaDs)
    {
      PELOG1(limLog(pMac, LOG1, FL("pStaDs is NULL\n"));)
      return;
    }
    limGetRfBand(pMac, &rfBand);
    //if we are in 5 GHZ band
    if(SIR_BAND_5_GHZ == rfBand)
    {
        //We are 11N. we need to protect from 11A and Ht20. we don't need any other protection in 5 GHZ.
        //HT20 case is common between both the bands and handled down as common code.
        if(true == psessionEntry->htCapabality)
        {
            //we are 11N and 11A station is joining.        
            //protection from 11A required.            
            if(false == pStaDs->mlmStaContext.htCapability)
            {
                limEnable11aProtection(pMac, true, false, pBeaconParams,psessionEntry);
                return;
            }
        }
    }
    else if(SIR_BAND_2_4_GHZ== rfBand)
    {
        limGetPhyMode(pMac, &phyMode);
        //We are 11G. Check if we need protection from 11b Stations.
        if ((phyMode == WNI_CFG_PHY_MODE_11G) &&
              (false == psessionEntry->htCapabality))
        {

            if (pStaDs->erpEnabled== eHAL_CLEAR)
            {
                protStaCacheType = eLIM_PROT_STA_CACHE_TYPE_llB;
                // enable protection
                PELOG3(limLog(pMac, LOG3, FL("Enabling protection from 11B\n"));)
                limEnable11gProtection(pMac, true, false, pBeaconParams,psessionEntry);
            }
        }

        //HT station.
        if (true == psessionEntry->htCapabality)
        {
            //check if we need protection from 11b station
            if ((pStaDs->erpEnabled== eHAL_CLEAR) &&
                (!pStaDs->mlmStaContext.htCapability))
            {
                protStaCacheType = eLIM_PROT_STA_CACHE_TYPE_llB;
                // enable protection
                PELOG3(limLog(pMac, LOG3, FL("Enabling protection from 11B\n"));)
                limEnable11gProtection(pMac, true, false, pBeaconParams, psessionEntry);
            }
            //station being joined is non-11b and non-ht ==> 11g device
            else if(!pStaDs->mlmStaContext.htCapability)
            {
                protStaCacheType = eLIM_PROT_STA_CACHE_TYPE_llG;
                //enable protection
                limEnableHtProtectionFrom11g(pMac, true, false, pBeaconParams, psessionEntry);       
            }
            //ERP mode is enabled for the latest station joined
            //latest station joined is HT capable
            //This case is being handled in common code (commn between both the bands) below.
        }
    }

    //we are HT and HT station is joining. This code is common for both the bands.
    if((true == psessionEntry->htCapabality) &&
        (true == pStaDs->mlmStaContext.htCapability))
    {
        if(!pStaDs->htGreenfield)
        {
          limEnableHTNonGfProtection(pMac, true, false, pBeaconParams,psessionEntry);
          gfSupported = eHAL_CLEAR;
        }
        //Station joining is HT 20Mhz
        if(eHT_CHANNEL_WIDTH_20MHZ == pStaDs->htSupportedChannelWidthSet)
        {
            protStaCacheType = eLIM_PROT_STA_CACHE_TYPE_HT20;
            limEnableHT20Protection(pMac, true, false, pBeaconParams,psessionEntry);
        }
        //Station joining does not support LSIG TXOP Protection
        if(!pStaDs->htLsigTXOPProtection)
        {
            limEnableHTLsigTxopProtection(pMac, false, false, pBeaconParams,psessionEntry);
            lsigTxopSupported = eHAL_CLEAR;
        }
    }

    limUpdateProtStaParams(pMac, peerMacAddr, protStaCacheType,
              gfSupported, lsigTxopSupported, psessionEntry);

    return;
}
#endif


/** -------------------------------------------------------------
\fn limEnableOverlap11gProtection
\brief wrapper function for setting overlap 11g protection.
\param      tpAniSirGlobal    pMac
\param      tpUpdateBeaconParams pBeaconParams
\param      tpSirMacMgmtHdr         pMh
\return      None
  -------------------------------------------------------------*/
void
limEnableOverlap11gProtection(tpAniSirGlobal pMac,
tpUpdateBeaconParams pBeaconParams, tpSirMacMgmtHdr pMh,tpPESession psessionEntry)
{
    limUpdateOverlapStaParam(pMac, pMh->bssId, &(pMac->lim.gLimOlbcParams));

    if (pMac->lim.gLimOlbcParams.numSta &&
        !pMac->lim.gLimOlbcParams.protectionEnabled)
    {
        // enable protection
        PELOG1(limLog(pMac, LOG1, FL("OLBC happens!!!\n"));)
        limEnable11gProtection(pMac, true, true, pBeaconParams,psessionEntry);
    }
}


/** -------------------------------------------------------------
\fn limUpdateShortPreamble
\brief Updates short preamble if needed when a new station joins.
\param      tpAniSirGlobal    pMac
\param      tSirMacAddr peerMacAddr
\param      tpUpdateBeaconParams pBeaconParams
\return      None
  -------------------------------------------------------------*/
void
limUpdateShortPreamble(tpAniSirGlobal pMac, tSirMacAddr peerMacAddr,
    tpUpdateBeaconParams pBeaconParams, tpPESession psessionEntry)
{
    tANI_U16              tmpAid;
    tpDphHashNode    pStaDs;
    tANI_U32 phyMode;
    tANI_U16 i;

    // check whether to enable protection or not
    pStaDs = dphLookupHashEntry(pMac, peerMacAddr, &tmpAid, &psessionEntry->dph.dphHashTable);
    limGetPhyMode(pMac, &phyMode);
    if (pStaDs != NULL && phyMode == WNI_CFG_PHY_MODE_11G)

    {        
        if (pStaDs->shortPreambleEnabled == eHAL_CLEAR)
        {
            PELOG1(limLog(pMac,LOG1,FL("Short Preamble is not enabled in Assoc Req from "));
            limPrintMacAddr(pMac, peerMacAddr, LOG1);)

            for (i=0; i<LIM_PROT_STA_CACHE_SIZE; i++)
            {
#ifdef WLAN_SOFTAP_FEATURE				
                if ((psessionEntry->limSystemRole == eLIM_AP_ROLE )  &&
				     psessionEntry->gLimNoShortParams.staNoShortCache[i].active)
                {
                    if (palEqualMemory( pMac->hHdd,
                        psessionEntry->gLimNoShortParams.staNoShortCache[i].addr,
                        peerMacAddr, sizeof(tSirMacAddr)))
                        return;
                }else if(psessionEntry->limSystemRole != eLIM_AP_ROLE)
#endif				
                {
					 if (pMac->lim.gLimNoShortParams.staNoShortCache[i].active)
                     {
                         if (palEqualMemory( pMac->hHdd,
                             pMac->lim.gLimNoShortParams.staNoShortCache[i].addr,
                             peerMacAddr, sizeof(tSirMacAddr)))
                             return;
                      }
				}
            }

            for (i=0; i<LIM_PROT_STA_CACHE_SIZE; i++)
            {
#ifdef WLAN_SOFTAP_FEATURE				
                if ( (psessionEntry->limSystemRole == eLIM_AP_ROLE )  &&
				     !psessionEntry->gLimNoShortParams.staNoShortCache[i].active)
                     break;
				else	
#endif				
                {
	                if (!pMac->lim.gLimNoShortParams.staNoShortCache[i].active)
                       break;
				}
            }

            if (i >= LIM_PROT_STA_CACHE_SIZE)
            {
#ifdef WLAN_SOFTAP_FEATURE
                if(psessionEntry->limSystemRole == eLIM_AP_ROLE){
				    limLog(pMac, LOGE, FL("No space in Short cache (#active %d, #sta %d) for sta "),
                           i, psessionEntry->gLimNoShortParams.numNonShortPreambleSta);
                    limPrintMacAddr(pMac, peerMacAddr, LOGE);
                    return;	
				}
				else
#endif		
                {
					limLog(pMac, LOGE, FL("No space in Short cache (#active %d, #sta %d) for sta "),
                       i, pMac->lim.gLimNoShortParams.numNonShortPreambleSta);
                    limPrintMacAddr(pMac, peerMacAddr, LOGE);
                    return;
				}
                
            }

#ifdef WLAN_SOFTAP_FEATURE
            if(psessionEntry->limSystemRole == eLIM_AP_ROLE){
			    palCopyMemory( pMac->hHdd, psessionEntry->gLimNoShortParams.staNoShortCache[i].addr,
                                        peerMacAddr,  sizeof(tSirMacAddr));
                psessionEntry->gLimNoShortParams.staNoShortCache[i].active = true;
                psessionEntry->gLimNoShortParams.numNonShortPreambleSta++;	
			}else
#endif
            {
			    palCopyMemory( pMac->hHdd, pMac->lim.gLimNoShortParams.staNoShortCache[i].addr,
                                        peerMacAddr,  sizeof(tSirMacAddr));
                pMac->lim.gLimNoShortParams.staNoShortCache[i].active = true;
                pMac->lim.gLimNoShortParams.numNonShortPreambleSta++;	
			} 
            

            // enable long preamble
            PELOG1(limLog(pMac, LOG1, FL("Disabling short preamble\n"));)
				
#ifdef WLAN_SOFTAP_FEATURE
            if (limEnableShortPreamble(pMac, false, pBeaconParams, psessionEntry) != eSIR_SUCCESS)
					PELOGE(limLog(pMac, LOGE, FL("Cannot enable long preamble\n"));)
#else
			if (limEnableShortPreamble(pMac, false, pBeaconParams) != eSIR_SUCCESS)
				PELOGE(limLog(pMac, LOGE, FL("Cannot enable long preamble\n"));)

#endif
        }
      }
}

/** -------------------------------------------------------------
\fn limUpdateShortSlotTime
\brief Updates short slot time if needed when a new station joins.
\param      tpAniSirGlobal    pMac
\param      tSirMacAddr peerMacAddr
\param      tpUpdateBeaconParams pBeaconParams
\return      None
  -------------------------------------------------------------*/

void
limUpdateShortSlotTime(tpAniSirGlobal pMac, tSirMacAddr peerMacAddr,
    tpUpdateBeaconParams pBeaconParams, tpPESession psessionEntry)
{
    tANI_U16              tmpAid;
    tpDphHashNode    pStaDs;
    tANI_U32 phyMode;
    tANI_U32 val;
    tANI_U32 cShortSlot;
    tANI_U16 i;

    // check whether to enable protection or not
    pStaDs = dphLookupHashEntry(pMac, peerMacAddr, &tmpAid, &psessionEntry->dph.dphHashTable);
    limGetPhyMode(pMac, &phyMode);
    if (pStaDs != NULL && phyMode == WNI_CFG_PHY_MODE_11G)
    {
        if (wlan_cfgGetInt(pMac, WNI_CFG_SHORT_SLOT_TIME, &cShortSlot) != eSIR_SUCCESS)
            limLog(pMac, LOGP, FL("unable to get short slot time\n"));

        if (pStaDs->shortSlotTimeEnabled == eHAL_CLEAR)
        {
            PELOG1(limLog(pMac, LOG1, FL("Short Slot Time is not enabled in Assoc Req from "));
            limPrintMacAddr(pMac, peerMacAddr, LOG1);)
            for (i=0; i<LIM_PROT_STA_CACHE_SIZE; i++)
            {
#ifdef WLAN_SOFTAP_FEATURE
                if ((psessionEntry->limSystemRole == eLIM_AP_ROLE ) &&
				     psessionEntry->gLimNoShortSlotParams.staNoShortSlotCache[i].active)
                {
                    if (palEqualMemory( pMac->hHdd,
                         psessionEntry->gLimNoShortSlotParams.staNoShortSlotCache[i].addr,
                         peerMacAddr, sizeof(tSirMacAddr)))
                        return;
                }
				else if(psessionEntry->limSystemRole != eLIM_AP_ROLE )
#endif
                {
					if (pMac->lim.gLimNoShortSlotParams.staNoShortSlotCache[i].active)
                    {
                        if (palEqualMemory( pMac->hHdd,
                            pMac->lim.gLimNoShortSlotParams.staNoShortSlotCache[i].addr,
                            peerMacAddr, sizeof(tSirMacAddr)))
                            return;
                     }
				}
                
            }

            for (i=0; i<LIM_PROT_STA_CACHE_SIZE; i++)
            {
#ifdef WLAN_SOFTAP_FEATURE
                if ((psessionEntry->limSystemRole == eLIM_AP_ROLE ) &&
				    !psessionEntry->gLimNoShortSlotParams.staNoShortSlotCache[i].active)
                    break;
                 else
#endif
                 {
                     if (!pMac->lim.gLimNoShortSlotParams.staNoShortSlotCache[i].active)
                          break;
				 }
            }

            if (i >= LIM_PROT_STA_CACHE_SIZE)
            {
#ifdef WLAN_SOFTAP_FEATURE
                if(psessionEntry->limSystemRole == eLIM_AP_ROLE){
                    limLog(pMac, LOGE, FL("No space in ShortSlot cache (#active %d, #sta %d) for sta "),
                            i, psessionEntry->gLimNoShortSlotParams.numNonShortSlotSta);
                    limPrintMacAddr(pMac, peerMacAddr, LOGE);
                    return;
				}else
#endif
                {
				    limLog(pMac, LOGE, FL("No space in ShortSlot cache (#active %d, #sta %d) for sta "),
                       i, pMac->lim.gLimNoShortSlotParams.numNonShortSlotSta);
                    limPrintMacAddr(pMac, peerMacAddr, LOGE);
                    return;	
				}
            }

#ifdef WLAN_SOFTAP_FEATURE
            if(psessionEntry->limSystemRole == eLIM_AP_ROLE){
	            palCopyMemory( pMac->hHdd, psessionEntry->gLimNoShortSlotParams.staNoShortSlotCache[i].addr,
                               peerMacAddr, sizeof(tSirMacAddr));
                psessionEntry->gLimNoShortSlotParams.staNoShortSlotCache[i].active = true;
                psessionEntry->gLimNoShortSlotParams.numNonShortSlotSta++;
			}else
#endif
            {
                palCopyMemory( pMac->hHdd, pMac->lim.gLimNoShortSlotParams.staNoShortSlotCache[i].addr,
                          peerMacAddr, sizeof(tSirMacAddr));
                pMac->lim.gLimNoShortSlotParams.staNoShortSlotCache[i].active = true;
                pMac->lim.gLimNoShortSlotParams.numNonShortSlotSta++;
			}
            wlan_cfgGetInt(pMac, WNI_CFG_11G_SHORT_SLOT_TIME_ENABLED, &val);

#ifdef WLAN_SOFTAP_FEATURE
			if ( (psessionEntry->limSystemRole == eLIM_AP_ROLE) && 
				 (val && psessionEntry->gLimNoShortSlotParams.numNonShortSlotSta && cShortSlot))
			{
				// enable long slot time
				pBeaconParams->fShortSlotTime = false;
				pBeaconParams->paramChangeBitmap |= PARAM_SHORT_SLOT_TIME_CHANGED;
				PELOG1(limLog(pMac, LOG1, FL("Disable short slot time. Enable long slot time.\n"));)
				if (cfgSetInt(pMac, WNI_CFG_SHORT_SLOT_TIME, false) != eSIR_SUCCESS)
					PELOGE(limLog(pMac, LOGE,	FL("could not update short slot time at CFG\n"));)
			}
			else if ( psessionEntry->limSystemRole != eLIM_AP_ROLE)
#endif            
			{
				if (val && pMac->lim.gLimNoShortSlotParams.numNonShortSlotSta && cShortSlot)
				{
					// enable long slot time
					pBeaconParams->fShortSlotTime = false;
					pBeaconParams->paramChangeBitmap |= PARAM_SHORT_SLOT_TIME_CHANGED;
					PELOG1(limLog(pMac, LOG1, FL("Disable short slot time. Enable long slot time.\n"));)
					if (cfgSetInt(pMac, WNI_CFG_SHORT_SLOT_TIME, false) != eSIR_SUCCESS)
						PELOGE(limLog(pMac, LOGE,	FL("could not update short slot time at CFG\n"));)
				 }

			}
		}
	}
}

#if (WNI_POLARIS_FW_PACKAGE == ADVANCED) && defined (ANI_PRODUCT_TYPE_AP)
/**
 * limDetectRadar()
 *
 *FUNCTION:
 * This function is invoked when LIM receives
 * SIR_LIM_RADAR_DETECT_IND in lim mesage queue.
 * LIM will notify WSM of this event by sending
 * WSM the wireless medium status change
 * notification message.
 *
 *
 *LOGIC:
 *
 *ASSUMPTIONS:
 * NA
 *
 *NOTE:
 * NA
 *
 * @param  pMac           - Pointer to Global MAC structure
 * @param  message        - a tSirRadarInfo struct type message send by HAL.
 *                          This message is not serialized.
 *
 * @return None
 */

void
limDetectRadar(tpAniSirGlobal pMac, tANI_U32 *pMsg)
{
    tpSirRadarInfo      pRadarInfo;

    if (!pMsg)
    {
        limLog(pMac, LOGP, FL("Message is NULL\n"));
        return;
    }

    pRadarInfo = (tpSirRadarInfo)pMsg;

    if ((pMac->lim.gLimCurrentChannelId == pRadarInfo->channelNumber) &&
                          (pMac->lim.gLimSystemRole != eLIM_UNKNOWN_ROLE))
    {
        LIM_SET_RADAR_DETECTED(pMac, eANI_BOOLEAN_TRUE);
        limStopMeasTimers(pMac);
        /* Stop the transmission of all packets except beacons.
         * This is done here, assuming that we will definitely get a channel switch 
         * request from WSM.
         */
        PELOG1(limLog(pMac, LOG1, "Stopping the transmission on AP\n");)
        limFrameTransmissionControl(pMac, eLIM_TX_BSS_BUT_BEACON, eLIM_STOP_TX);
    }

   PELOG1(limLog(pMac, LOG1,
        FL("limDetectRadar():: pulsewidth = %d, numPulse=%d, channelId=%d\n"),
        pRadarInfo->radarPulseWidth, pRadarInfo->numRadarPulse,
        pRadarInfo->channelNumber);)

    limSendSmeWmStatusChangeNtf(pMac,
                                eSIR_SME_RADAR_DETECTED,
                                pMsg,
                                (tANI_U16)sizeof(*pRadarInfo));
    
}
#endif

# if 0
//TBD_RAJESH :: TO SOLVE LINKING ISSUE
void
limUpdateShortSlotTime(tpAniSirGlobal pMac, tSirMacAddr peerMacAddr, //dummy to avoid linking problem
    tpUpdateBeaconParams pBeaconParams)
{

}

//TBD_RAJESH :: TO SOLVE LINKING ISSUE
void
limUpdateShortPreamble(tpAniSirGlobal pMac, tSirMacAddr peerMacAddr,    //dummy to avoid linking problem
    tpUpdateBeaconParams pBeaconParams)
    
{
}

//TBD_RAJESH :: TO SOLVE LINKING ISSUE

void   //dummy to avoid linking problem
limDecideApProtection(tpAniSirGlobal pMac, tSirMacAddr peerMacAddr, tpUpdateBeaconParams pBeaconParams,tpPESession psessionEntry)
{
}

#endif


/** -------------------------------------------------------------
\fn limDecideStaProtectionOnAssoc
\brief Decide protection related settings on Sta while association.
\param      tpAniSirGlobal    pMac
\param      tpSchBeaconStruct pBeaconStruct
\return      None
  -------------------------------------------------------------*/
void
limDecideStaProtectionOnAssoc(tpAniSirGlobal pMac,
    tpSchBeaconStruct pBeaconStruct, tpPESession psessionEntry)
{
    tSirRFBand rfBand = SIR_BAND_UNKNOWN;
    tANI_U32 phyMode = WNI_CFG_PHY_MODE_NONE;

    limGetRfBand(pMac, &rfBand);
    limGetPhyMode(pMac, &phyMode);      
	
	if(SIR_BAND_5_GHZ == rfBand)
    {
        if((eSIR_HT_OP_MODE_MIXED == pBeaconStruct->HTInfo.opMode)  ||
                    (eSIR_HT_OP_MODE_OVERLAP_LEGACY == pBeaconStruct->HTInfo.opMode))
        {
            if(pMac->lim.cfgProtection.fromlla)
                psessionEntry->llaCoexist = true;
        }
        else if(eSIR_HT_OP_MODE_NO_LEGACY_20MHZ_HT == pBeaconStruct->HTInfo.opMode)
        {
            if(pMac->lim.cfgProtection.ht20)
                psessionEntry->ht20Coexist = true;
        }

    }
    else if(SIR_BAND_2_4_GHZ == rfBand)
    {
        //spec 7.3.2.13
        //UseProtection will be set when nonERP STA is associated.
        //NonERPPresent bit will be set when:
        //--nonERP Sta is associated OR
        //--nonERP Sta exists in overlapping BSS
        //when useProtection is not set then protection from nonERP stations is optional.

        //CFG protection from 11b is enabled and
        //11B device in the BSS
         /* TODO, This is not sessionized */
        if (phyMode != WNI_CFG_PHY_MODE_11B) 
		{
            if (pMac->lim.cfgProtection.fromllb &&
                pBeaconStruct->erpPresent &&
                (pBeaconStruct->erpIEInfo.useProtection ||
                pBeaconStruct->erpIEInfo.nonErpPresent))
            {
                psessionEntry->llbCoexist = true;
            }
            //AP has no 11b station associated.
            else
            {
                psessionEntry->llbCoexist = false;
            }
	 }
        //following code block is only for HT station.
        if((psessionEntry->htCapabality) &&
              (pBeaconStruct->HTInfo.present))
        {
            tDot11fIEHTInfo htInfo = pBeaconStruct->HTInfo;
           
            //Obss Non HT STA present mode 
            pMac->lim.gHTObssMode =  (tANI_U8)htInfo.obssNonHTStaPresent;

            
          //CFG protection from 11G is enabled and
            //our AP has at least one 11G station associated.       
            if(pMac->lim.cfgProtection.fromllg &&
                  ((eSIR_HT_OP_MODE_MIXED == htInfo.opMode)  ||
                        (eSIR_HT_OP_MODE_OVERLAP_LEGACY == htInfo.opMode))&&
                      (!psessionEntry->llbCoexist))
            {
                if(pMac->lim.cfgProtection.fromllg)
                    psessionEntry->llgCoexist = true;
            }

            //AP has only HT stations associated and at least one station is HT 20
            //disable protection from any non-HT devices.
            //decision for disabling protection from 11b has already been taken above.
            if(eSIR_HT_OP_MODE_NO_LEGACY_20MHZ_HT == htInfo.opMode)
            {
                //Disable protection from 11G station.
                psessionEntry->llgCoexist = false;
          //CFG protection from HT 20 is enabled.
          if(pMac->lim.cfgProtection.ht20)
                psessionEntry->ht20Coexist = true;
            }
            //Disable protection from non-HT and HT20 devices.
            //decision for disabling protection from 11b has already been taken above.
            if(eSIR_HT_OP_MODE_PURE == htInfo.opMode)
            {
                psessionEntry->llgCoexist = false;
                psessionEntry->ht20Coexist = false;
            }

        }
    }

    //protection related factors other than HT operating mode. Applies to 2.4 GHZ as well as 5 GHZ.
    if((psessionEntry->htCapabality) &&
          (pBeaconStruct->HTInfo.present))
    {
        tDot11fIEHTInfo htInfo = pBeaconStruct->HTInfo;

        pMac->lim.gHTRifsMode       = ( tANI_U8 ) htInfo.rifsMode;
        pMac->lim.gHTNonGFDevicesPresent = ( tANI_U8 )htInfo.nonGFDevicesPresent;
        pMac->lim.gHTLSigTXOPFullSupport        = ( tANI_U8 )htInfo.lsigTXOPProtectionFullSupport;
    }
}


/** -------------------------------------------------------------
\fn limDecideStaProtection
\brief Decides protection related settings on Sta while processing beacon.
\param      tpAniSirGlobal    pMac
\param      tpUpdateBeaconParams pBeaconParams
\return      None
  -------------------------------------------------------------*/
void
limDecideStaProtection(tpAniSirGlobal pMac,
    tpSchBeaconStruct pBeaconStruct, tpUpdateBeaconParams pBeaconParams, tpPESession psessionEntry)
{

    tSirRFBand rfBand = SIR_BAND_UNKNOWN;
    tANI_U32 phyMode = WNI_CFG_PHY_MODE_NONE;

    limGetRfBand(pMac, &rfBand);
    limGetPhyMode(pMac, &phyMode);
       
    if(SIR_BAND_5_GHZ == rfBand)
    {
        //we are HT capable.
        if((true == psessionEntry->htCapabality) &&
            (pBeaconStruct->HTInfo.present))
        {
            //we are HT capable, AP's HT OPMode is mixed / overlap legacy ==> need protection from 11A.        
            if((eSIR_HT_OP_MODE_MIXED == pBeaconStruct->HTInfo.opMode) ||
              (eSIR_HT_OP_MODE_OVERLAP_LEGACY == pBeaconStruct->HTInfo.opMode))
            {
                limEnable11aProtection(pMac, true, false, pBeaconParams,psessionEntry);
            }
            //we are HT capable, AP's HT OPMode is HT20 ==> disable protection from 11A if enabled. enabled 
            //protection from HT20 if needed.
            else if(eSIR_HT_OP_MODE_NO_LEGACY_20MHZ_HT== pBeaconStruct->HTInfo.opMode)
            {
                limEnable11aProtection(pMac, false, false, pBeaconParams,psessionEntry);            
                limEnableHT20Protection(pMac, true, false, pBeaconParams,psessionEntry);
            }
            else if(eSIR_HT_OP_MODE_PURE == pBeaconStruct->HTInfo.opMode)
            {
                limEnable11aProtection(pMac, false, false, pBeaconParams,psessionEntry);            
                limEnableHT20Protection(pMac, false, false, pBeaconParams,psessionEntry);
            }
        }
    }
    else if(SIR_BAND_2_4_GHZ == rfBand)
    {
        /* spec 7.3.2.13
         * UseProtection will be set when nonERP STA is associated.
         * NonERPPresent bit will be set when:
         * --nonERP Sta is associated OR
         * --nonERP Sta exists in overlapping BSS
         * when useProtection is not set then protection from nonERP stations is optional.
         */

        if (phyMode != WNI_CFG_PHY_MODE_11B) 
		{
            if (pBeaconStruct->erpPresent &&
                  (pBeaconStruct->erpIEInfo.useProtection ||
                  pBeaconStruct->erpIEInfo.nonErpPresent))
            {
                limEnable11gProtection(pMac, true, false, pBeaconParams, psessionEntry);
            }
            //AP has no 11b station associated.
            else
            {
                //disable protection from 11b station
                limEnable11gProtection(pMac, false, false, pBeaconParams, psessionEntry);
            }
	}

        //following code block is only for HT station.
        if((psessionEntry->htCapabality) &&
              (pBeaconStruct->HTInfo.present))
        {
          
            tDot11fIEHTInfo htInfo = pBeaconStruct->HTInfo;
            //AP has at least one 11G station associated.
            if(((eSIR_HT_OP_MODE_MIXED == htInfo.opMode)  ||
                  (eSIR_HT_OP_MODE_OVERLAP_LEGACY == htInfo.opMode))&&
                (!psessionEntry->llbCoexist))
            {
                limEnableHtProtectionFrom11g(pMac, true, false, pBeaconParams,psessionEntry);
        
            }

            //no HT operating mode change  ==> no change in protection settings except for MIXED_MODE/Legacy Mode.
            //in Mixed mode/legacy Mode even if there is no change in HT operating mode, there might be change in 11bCoexist
            //or 11gCoexist. that is why this check is being done after mixed/legacy mode check.
            if ( pMac->lim.gHTOperMode != ( tSirMacHTOperatingMode )htInfo.opMode )
            {
                pMac->lim.gHTOperMode       = ( tSirMacHTOperatingMode )htInfo.opMode;

                 //AP has only HT stations associated and at least one station is HT 20
                 //disable protection from any non-HT devices.
                 //decision for disabling protection from 11b has already been taken above.
                if(eSIR_HT_OP_MODE_NO_LEGACY_20MHZ_HT == htInfo.opMode)
                {
                    //Disable protection from 11G station.
                    limEnableHtProtectionFrom11g(pMac, false, false, pBeaconParams,psessionEntry);
        
                    limEnableHT20Protection(pMac, true, false, pBeaconParams,psessionEntry);
                }
                //Disable protection from non-HT and HT20 devices.
                //decision for disabling protection from 11b has already been taken above.
                else if(eSIR_HT_OP_MODE_PURE == htInfo.opMode)
                {
                    limEnableHtProtectionFrom11g(pMac, false, false, pBeaconParams,psessionEntry);
                    limEnableHT20Protection(pMac, false, false, pBeaconParams,psessionEntry);
            
                }
            }
        }
    }

    //following code block is only for HT station. ( 2.4 GHZ as well as 5 GHZ)
    if((psessionEntry->htCapabality) &&
          (pBeaconStruct->HTInfo.present))
    {
        tDot11fIEHTInfo htInfo = pBeaconStruct->HTInfo;    
        //Check for changes in protection related factors other than HT operating mode.
        //Check for changes in RIFS mode, nonGFDevicesPresent, lsigTXOPProtectionFullSupport.
        if ( pMac->lim.gHTRifsMode != ( tANI_U8 ) htInfo.rifsMode )
        {
            pBeaconParams->fRIFSMode = pMac->lim.gHTRifsMode       = ( tANI_U8 ) htInfo.rifsMode;
            pBeaconParams->paramChangeBitmap |= PARAM_RIFS_MODE_CHANGED;
        }

        if ( pMac->lim.gHTNonGFDevicesPresent != htInfo.nonGFDevicesPresent )
        {
            pBeaconParams->llnNonGFCoexist = pMac->lim.gHTNonGFDevicesPresent = ( tANI_U8 )htInfo.nonGFDevicesPresent;
            pBeaconParams->paramChangeBitmap      |= PARAM_NON_GF_DEVICES_PRESENT_CHANGED;
        }

        if ( pMac->lim.gHTLSigTXOPFullSupport != ( tANI_U8 )htInfo.lsigTXOPProtectionFullSupport )
        {
            pBeaconParams->fLsigTXOPProtectionFullSupport = pMac->lim.gHTLSigTXOPFullSupport        = ( tANI_U8 )htInfo.lsigTXOPProtectionFullSupport;
            pBeaconParams->paramChangeBitmap             |= PARAM_LSIG_TXOP_FULL_SUPPORT_CHANGED;
        }
        
    // For Station just update the global lim variable, no need to send message to HAL
    // Station already taking care of HT OPR Mode=01, meaning AP is seeing legacy
    //stations in overlapping BSS.
       if ( pMac->lim.gHTObssMode != ( tANI_U8 )htInfo.obssNonHTStaPresent )
            pMac->lim.gHTObssMode = ( tANI_U8 )htInfo.obssNonHTStaPresent ;
            
    }
}


/**
 * limProcessChannelSwitchTimeout()
 *
 *FUNCTION:
 * This function is invoked when Channel Switch Timer expires at
 * the STA.  Now, STA must stop traffic, and then change/disable
 * primary or secondary channel.
 *
 *
 *NOTE:
 * @param  pMac           - Pointer to Global MAC structure
 * @return None
 */
void limProcessChannelSwitchTimeout(tpAniSirGlobal pMac)
{
    tpPESession psessionEntry = NULL;
#ifdef GEN6_TODO
    //fetch the sessionEntry based on the sessionId
    //priority - MEDIUM
   
    if((psessionEntry = peFindSessionBySessionId(pMac, pMac->lim.limTimers.gLimChannelSwitchTimer.sessionId))== NULL) 
    {
        limLog(pMac, LOGP,FL("Session Does not exist for given sessionID\n"));
        return;
    }
#endif

#if defined(ANI_PRODUCT_TYPE_CLIENT) || defined(ANI_AP_CLIENT_SDK)
    tSirMsgQ    mmhMsg;
    tSirMbMsg   *msg2Hdd;

    tANI_U8    channel = pMac->lim.gLimChannelSwitch.primaryChannel;   // This is received and stored from channelSwitch Action frame

    if (psessionEntry->limSystemRole != eLIM_STA_ROLE)
    {
        PELOGW(limLog(pMac, LOGW, "Channel switch can be done only in STA role, Current Role = %d\n", psessionEntry->limSystemRole);)
        return;
    }
    /*
     *  This potentially can create issues if the function tries to set
     * channel while device is in power-save, hence putting an extra check
     * to verify if the device is in power-save or not
     */
    if(!limIsSystemInActiveState(pMac))
    {
        PELOGW(limLog(pMac, LOGW, FL("Device is not in active state, cannot switch channel\n"));)
        return;
    }
         
    // Restore Channel Switch parameters to default
    pMac->lim.gLimChannelSwitch.switchTimeoutValue = 0;

    /* Channel-switch timeout has occured. reset the state */
    pMac->lim.gLimSpecMgmt.dot11hChanSwState = eLIM_11H_CHANSW_END;
    
    /* Check if the AP is switching to a channel that we support.
     * Else, just don't bother to switch. Indicate HDD to look for a 
     * better AP to associate
     */
    if(!limIsChannelValidForChannelSwitch(pMac, channel))
    {
        /* We need to restore pre-channelSwitch state on the STA */
        if(limRestorePreChannelSwitchState(pMac, psessionEntry) != eSIR_SUCCESS)
        {
            limLog(pMac, LOGP, FL("Could not restore pre-channelSwitch (11h) state, resetting the system\n"));
            return;
        }

        /* If the channel-list that AP is asking us to switch is invalid,
         * then we cannot switch the channel. Just disassociate from AP. 
         * We will find a better AP !!!
         */
        limTearDownLinkWithAp(pMac);
        return;
    }
    switch(pMac->lim.gLimChannelSwitch.state)
    {
        case eLIM_CHANNEL_SWITCH_PRIMARY_ONLY:
            PELOGW(limLog(pMac, LOGW, FL("CHANNEL_SWITCH_PRIMARY_ONLY \n"));)
            limSwitchPrimaryChannel(pMac, pMac->lim.gLimChannelSwitch.primaryChannel,psessionEntry);
            pMac->lim.gLimChannelSwitch.state = eLIM_CHANNEL_SWITCH_IDLE;
            break;

        case eLIM_CHANNEL_SWITCH_SECONDARY_ONLY:
            PELOGW(limLog(pMac, LOGW, FL("CHANNEL_SWITCH_SECONDARY_ONLY \n"));)
            limSwitchPrimarySecondaryChannel(pMac,
                                             psessionEntry->currentOperChannel,
                                             pMac->lim.gLimChannelSwitch.secondarySubBand);
            pMac->lim.gLimChannelSwitch.state = eLIM_CHANNEL_SWITCH_IDLE;
            break;

        case eLIM_CHANNEL_SWITCH_PRIMARY_AND_SECONDARY:
            PELOGW(limLog(pMac, LOGW, FL("CHANNEL_SWITCH_PRIMARY_AND_SECONDARY\n"));)
            limSwitchPrimarySecondaryChannel(pMac,
                                             pMac->lim.gLimChannelSwitch.primaryChannel,
                                             pMac->lim.gLimChannelSwitch.secondarySubBand);
            pMac->lim.gLimChannelSwitch.state = eLIM_CHANNEL_SWITCH_IDLE;
            break;

        case eLIM_CHANNEL_SWITCH_IDLE:
        default:
            PELOGE(limLog(pMac, LOGE, FL("incorrect state \n"));)
            if(limRestorePreChannelSwitchState(pMac, psessionEntry) != eSIR_SUCCESS)
            {
                limLog(pMac, LOGP, FL("Could not restore pre-channelSwitch (11h) state, resetting the system\n"));
            }
            return;  /* Please note, this is 'return' and not 'break' */
    }

    /* We need to restore pre-channelSwitch state on the STA */
    if (limRestorePreChannelSwitchState(pMac, psessionEntry) != eSIR_SUCCESS)
    {
        limLog(pMac, LOGP, FL("Could not restore pre-channelSwitch (11h) state, resetting the system\n"));
        return;
    }

    mmhMsg.type = eWNI_SME_SWITCH_CHL_REQ;
    if( eHAL_STATUS_SUCCESS != palAllocateMemory( pMac->hHdd, (void **)&msg2Hdd, sizeof(tSirMbMsg)))
    {
        limLog(pMac, LOGP, FL("Failed to allocate buffer for buffer descriptor\n"));
        return;
    }


#if defined (ANI_PRODUCT_TYPE_AP) && defined (ANI_LITTLE_BYTE_ENDIAN)
    sirStoreU16N((tANI_U8*)&msg2Hdd->type, eWNI_SME_SWITCH_CHL_REQ);
    sirStoreU16N((tANI_U8*)&msg2Hdd->msgLen, sizeof(tSirMbMsg));
#else
    msg2Hdd->type = eWNI_SME_SWITCH_CHL_REQ;
    msg2Hdd->msgLen = sizeof(tSirMbMsg);
#endif
    
    msg2Hdd->data[0] = (tANI_U32) pMac->lim.gLimChannelSwitch.primaryChannel;
    mmhMsg.bodyptr = msg2Hdd;
    mmhMsg.bodyval = 0;

    MTRACE(macTraceMsgTx(pMac, 0, mmhMsg.type));
// FIXME
#if defined( FEATURE_WLAN_INTEGRATED_SOC )
    SysProcessMmhMsg(pMac, &mmhMsg);
#else
    if(halMmhPostMsgApi(pMac, &mmhMsg, ePROT) != eSIR_SUCCESS)
    {
        palFreeMemory(pMac->hHdd, (void *)msg2Hdd);
        limLog(pMac, LOGP, FL("Message posting to HAL failed\n"));
    }
#endif
#endif
}

/**
 * limUpdateChannelSwitch()
 *
 *FUNCTION:
 * This function is invoked whenever Station receives
 * either 802.11h channel switch IE or airgo proprietary
 * channel switch IE.
 *
 *NOTE:
 * @param  pMac           - Pointer to Global MAC structure
 * @return  tpSirProbeRespBeacon - Pointer to Beacon/Probe Rsp
 * @param psessionentry
 */
void
limUpdateChannelSwitch(struct sAniSirGlobal *pMac,  tpSirProbeRespBeacon pBeacon, tpPESession psessionEntry)
{

    tANI_U16                         beaconPeriod;
    tANI_U32                         val;
    tChannelSwitchPropIEStruct       *pPropChnlSwitch;
    tDot11fIEChanSwitchAnn           *pChnlSwitch;

    if (!pMac->lim.gLim11hEnable)
        return;

    if (wlan_cfgGetInt(pMac, WNI_CFG_BEACON_INTERVAL, &val) != eSIR_SUCCESS)
    {
        limLog(pMac, LOGP, FL("Could not retrieve Beacon interval\n"));
        return;
    }
    beaconPeriod = (tANI_U16) val;

    /* STA either received proprietary channel switch IE or 802.11h
     * standard channel switch IE.
     */
    if (pBeacon->propIEinfo.propChannelSwitchPresent)
    {
        pPropChnlSwitch = &(pBeacon->propIEinfo.channelSwitch);

        /* Add logic to determine which change this is:  */
        /*      primary, secondary, both.  For now assume both. */
        pMac->lim.gLimChannelSwitch.state = eLIM_CHANNEL_SWITCH_PRIMARY_AND_SECONDARY;
        pMac->lim.gLimChannelSwitch.primaryChannel = pPropChnlSwitch->primaryChannel;
        pMac->lim.gLimChannelSwitch.secondarySubBand = (tAniCBSecondaryMode)pPropChnlSwitch->subBand;
        pMac->lim.gLimChannelSwitch.switchCount = pPropChnlSwitch->channelSwitchCount;
        pMac->lim.gLimChannelSwitch.switchTimeoutValue =
                 SYS_MS_TO_TICKS(beaconPeriod)* (pPropChnlSwitch->channelSwitchCount);
        pMac->lim.gLimChannelSwitch.switchMode = pPropChnlSwitch->mode;
    }
    else
    {
       pChnlSwitch = &(pBeacon->channelSwitchIE);
       pMac->lim.gLimChannelSwitch.primaryChannel = pChnlSwitch->newChannel;
       pMac->lim.gLimChannelSwitch.switchCount = pChnlSwitch->switchCount;
       pMac->lim.gLimChannelSwitch.switchTimeoutValue =
                 SYS_MS_TO_TICKS(beaconPeriod)* (pChnlSwitch->switchCount);
       pMac->lim.gLimChannelSwitch.switchMode = pChnlSwitch->switchMode; 

        /* Only primary channel switch element is present */
        pMac->lim.gLimChannelSwitch.state = eLIM_CHANNEL_SWITCH_PRIMARY_ONLY;
        pMac->lim.gLimChannelSwitch.secondarySubBand = eANI_CB_SECONDARY_NONE;

        /* Do not bother to look and operate on extended channel switch element
         * if our own channel-bonding state is not enabled
         */
        if(GET_CB_ADMIN_STATE(pMac->lim.gCbState))
        {
            if (pBeacon->extChannelSwitchPresent)
            {
                switch(pBeacon->extChannelSwitchIE.secondaryChannelOffset)
                {
                    case eHT_SECONDARY_CHANNEL_OFFSET_UP:
                        pMac->lim.gLimChannelSwitch.state = eLIM_CHANNEL_SWITCH_PRIMARY_AND_SECONDARY;
                        pMac->lim.gLimChannelSwitch.secondarySubBand = eANI_CB_SECONDARY_UP;
                        break;

                    case eHT_SECONDARY_CHANNEL_OFFSET_DOWN:
                        pMac->lim.gLimChannelSwitch.state = eLIM_CHANNEL_SWITCH_PRIMARY_AND_SECONDARY;
                        pMac->lim.gLimChannelSwitch.secondarySubBand = eANI_CB_SECONDARY_DOWN;
                        break;

                    case eHT_SECONDARY_CHANNEL_OFFSET_NONE:
                    default:
                        /* Nothing to be done here as of now!! */
                        break;
                }
           }
       }
    }

    if (eSIR_SUCCESS != limStartChannelSwitch(pMac, psessionEntry))
    {
        PELOGW(limLog(pMac, LOGW, FL("Could not start Channel Switch\n"));)
    }

    limLog(pMac, LOGW,
        FL("primary chl %d, subband %d, count  %d (%d ticks) \n"),
        pMac->lim.gLimChannelSwitch.primaryChannel,
        pMac->lim.gLimChannelSwitch.secondarySubBand,
        pMac->lim.gLimChannelSwitch.switchCount,
        pMac->lim.gLimChannelSwitch.switchTimeoutValue);
    return;
}

/**
 * limCancelDot11hChannelSwitch
 *
 *FUNCTION:
 * This function is called when STA does not send updated channel-swith IE
 * after indicating channel-switch start. This will cancel the channel-swith
 * timer which is alredy running.
 * 
 *LOGIC:
 *
 *ASSUMPTIONS:
 *
 *NOTE:
 *
 * @param  pMac    - Pointer to Global MAC structure
 *
 * @return None
 */
void limCancelDot11hChannelSwitch(tpAniSirGlobal pMac, tpPESession psessionEntry)
{
#if defined(ANI_PRODUCT_TYPE_CLIENT) || defined(ANI_AP_CLIENT_SDK)
    if (psessionEntry->limSystemRole != eLIM_STA_ROLE)
        return;
        
    PELOGW(limLog(pMac, LOGW, FL("Received a beacon without channel switch IE\n"));)
    MTRACE(macTrace(pMac, TRACE_CODE_TIMER_DEACTIVATE, 0, eLIM_CHANNEL_SWITCH_TIMER));

    if (tx_timer_deactivate(&pMac->lim.limTimers.gLimChannelSwitchTimer) != eSIR_SUCCESS)
    {
        PELOGE(limLog(pMac, LOGE, FL("tx_timer_deactivate failed!\n"));)
    }

    /* We need to restore pre-channelSwitch state on the STA */
    if (limRestorePreChannelSwitchState(pMac, psessionEntry) != eSIR_SUCCESS)
    {
        PELOGE(limLog(pMac, LOGE, FL("LIM: Could not restore pre-channelSwitch (11h) state, reseting the system\n"));)
                
    }
#endif
}

/**----------------------------------------------
\fn     limCancelDot11hQuiet
\brief  Cancel the quieting on Station if latest
        beacon doesn't contain quiet IE in it.

\param  pMac
\return NONE
-----------------------------------------------*/
void limCancelDot11hQuiet(tpAniSirGlobal pMac, tpPESession psessionEntry)
{
#if defined(ANI_PRODUCT_TYPE_CLIENT) || defined(ANI_AP_CLIENT_SDK)
    if (psessionEntry->limSystemRole != eLIM_STA_ROLE)
        return;

    if (pMac->lim.gLimSpecMgmt.quietState == eLIM_QUIET_BEGIN) 
    {
         MTRACE(macTrace(pMac, TRACE_CODE_TIMER_DEACTIVATE, 0, eLIM_QUIET_TIMER));
        if (tx_timer_deactivate(&pMac->lim.limTimers.gLimQuietTimer) != TX_SUCCESS)
        {
            PELOGE(limLog(pMac, LOGE, FL("tx_timer_deactivate failed\n"));)
        }
    }
    else if (pMac->lim.gLimSpecMgmt.quietState == eLIM_QUIET_RUNNING)
    {
        MTRACE(macTrace(pMac, TRACE_CODE_TIMER_DEACTIVATE, 0, eLIM_QUIET_BSS_TIMER));
        if (tx_timer_deactivate(&pMac->lim.limTimers.gLimQuietBssTimer) != TX_SUCCESS)
        {
            PELOGE(limLog(pMac, LOGE, FL("tx_timer_deactivate failed\n"));)
        }
        /**
         * If the channel switch is already running in silent mode, dont resume the
         * transmission. Channel switch timer when timeout, transmission will be resumed.
         */
        if(!((pMac->lim.gLimSpecMgmt.dot11hChanSwState == eLIM_11H_CHANSW_RUNNING) &&
                (pMac->lim.gLimChannelSwitch.switchMode == eSIR_CHANSW_MODE_SILENT)))
        {
            limFrameTransmissionControl(pMac, eLIM_TX_ALL, eLIM_RESUME_TX);
            limRestorePreQuietState(pMac);
        }
    }
    pMac->lim.gLimSpecMgmt.quietState = eLIM_QUIET_INIT;
#endif
}

/**
 * limProcessQuietTimeout
 *
 * FUNCTION:
 * This function is active only on the STA.
 * Handles SIR_LIM_QUIET_TIMEOUT
 *
 * LOGIC:
 * This timeout can occur under only one circumstance:
 *
 * 1) When gLimQuietState = eLIM_QUIET_BEGIN
 * This indicates that the timeout "interval" has
 * expired. This is a trigger for the STA to now
 * shut-off Tx/Rx for the specified gLimQuietDuration
 * -> The TIMER object gLimQuietBssTimer is
 * activated
 * -> With timeout = gLimQuietDuration
 * -> gLimQuietState is set to eLIM_QUIET_RUNNING
 *
 * ASSUMPTIONS:
 * Using two TIMER objects -
 * gLimQuietTimer & gLimQuietBssTimer
 *
 * NOTE:
 *
 * @param  pMac - Pointer to Global MAC structure
 *
 * @return None
 */
void limProcessQuietTimeout(tpAniSirGlobal pMac)
{
#ifdef GEN6_TODO
    //fetch the sessionEntry based on the sessionId
    //priority - MEDIUM
    tpPESession sessionEntry;

    if((sessionEntry = peFindSessionBySessionId(pMac, pMac->lim.limTimers.gLimQuietTimer.sessionId))== NULL) 
    {
        limLog(pMac, LOGP,FL("Session Does not exist for given sessionID\n"));
        return;
    }
#endif

  PELOG1(limLog(pMac, LOG1, FL("quietState = %d\n"), pMac->lim.gLimSpecMgmt.quietState);)
  switch( pMac->lim.gLimSpecMgmt.quietState )
  {
    case eLIM_QUIET_BEGIN:
      // Time to Stop data traffic for quietDuration
      limDeactivateAndChangeTimer(pMac, eLIM_QUIET_BSS_TIMER);
      MTRACE(macTrace(pMac, TRACE_CODE_TIMER_ACTIVATE, 0, eLIM_QUIET_BSS_TIMER));
#ifdef GEN6_TODO
        /* revisit this piece of code to assign the appropriate sessionId below
         * priority - HIGH
         */
        pMac->lim.limTimers.gLimQuietBssTimer.sessionId = sessionId;
#endif              
      if( TX_SUCCESS !=
          tx_timer_activate( &pMac->lim.limTimers.gLimQuietBssTimer ))
      {
        limLog( pMac, LOGW,
            FL("Unable to activate gLimQuietBssTimer! The STA will be unable to honor Quiet BSS...\n"));
      }
      else
      {
        // Transition to eLIM_QUIET_RUNNING
        pMac->lim.gLimSpecMgmt.quietState = eLIM_QUIET_RUNNING;

        /* If we have sta bk scan triggered and trigger bk scan actually started successfully, */
        /* print message, otherwise, stop data traffic and stay quiet */
        if( pMac->lim.gLimTriggerBackgroundScanDuringQuietBss &&
          (eSIR_TRUE == (glimTriggerBackgroundScanDuringQuietBss_Status = limTriggerBackgroundScanDuringQuietBss( pMac ))) )
        {
           limLog( pMac, LOG2,
               FL("Attempting to trigger a background scan...\n"));
        }
        else
        {
           // Shut-off Tx/Rx for gLimSpecMgmt.quietDuration
           /* freeze the transmission */
           limFrameTransmissionControl(pMac, eLIM_TX_ALL, eLIM_STOP_TX);

           limLog( pMac, LOG2,
                FL("Quiet BSS: STA shutting down for %d ticks\n"),
                pMac->lim.gLimSpecMgmt.quietDuration );
        }
      }
      break;

    case eLIM_QUIET_RUNNING:
    case eLIM_QUIET_INIT:
    case eLIM_QUIET_END:
    default:
      //
      // As of now, nothing to be done
      //
      break;
  }
}

/**
 * limProcessQuietBssTimeout
 *
 * FUNCTION:
 * This function is active on the AP and STA.
 * Handles SIR_LIM_QUIET_BSS_TIMEOUT
 *
 * LOGIC:
 * On the AP -
 * When the SIR_LIM_QUIET_BSS_TIMEOUT is triggered, it is
 * an indication for the AP to START sending out the
 * Quiet BSS IE.
 * If 802.11H is enabled, the Quiet BSS IE is sent as per
 * the 11H spec
 * If 802.11H is not enabled, the Quiet BSS IE is sent as
 * a Proprietary IE. This will be understood by all the
 * TITAN STA's
 * Transitioning gLimQuietState to eLIM_QUIET_BEGIN will
 * initiate the SCH to include the Quiet BSS IE in all
 * its subsequent Beacons/PR's.
 * The Quiet BSS IE will be included in all the Beacons
 * & PR's until the next DTIM period
 *
 * On the STA -
 * When gLimQuietState = eLIM_QUIET_RUNNING
 * This indicates that the STA was successfully shut-off
 * for the specified gLimQuietDuration. This is a trigger
 * for the STA to now resume data traffic.
 * -> gLimQuietState is set to eLIM_QUIET_INIT
 *
 * ASSUMPTIONS:
 *
 * NOTE:
 *
 * @param  pMac - Pointer to Global MAC structure
 *
 * @return None
 */
void limProcessQuietBssTimeout( tpAniSirGlobal pMac )
{
    tpPESession sessionEntry;

    if((sessionEntry = peFindSessionBySessionId(pMac, pMac->lim.limTimers.gLimQuietBssTimer.sessionId))== NULL) 
    {
        limLog(pMac, LOGP,FL("Session Does not exist for given sessionID\n"));
        return;
    }

  PELOG1(limLog(pMac, LOG1, FL("quietState = %d\n"), pMac->lim.gLimSpecMgmt.quietState);)
  if (eLIM_AP_ROLE == sessionEntry->limSystemRole)
  {
#ifdef ANI_PRODUCT_TYPE_AP
    if (!pMac->sys.gSysEnableLearnMode)
    {
        pMac->lim.gLimSpecMgmt.quietState = eLIM_QUIET_END;
        return;
    }

    if( eLIM_QUIET_INIT == pMac->lim.gLimSpecMgmt.quietState )
    {
        //QuietCount = 0 is reserved
        pMac->lim.gLimSpecMgmt.quietCount  = 2;
        // In ms.
        pMac->lim.gLimSpecMgmt.quietDuration = 
        pMac->lim.gpLimMeasReq->measDuration.shortChannelScanDuration;
        // TU is in multiples of 1024 (2^10) us.
        pMac->lim.gLimSpecMgmt.quietDuration_TU = 
            SYS_MS_TO_TU(pMac->lim.gLimSpecMgmt.quietDuration); 
        // Transition to eLIM_QUIET_BEGIN
        limLog( pMac, LOG2, FL("Quiet BSS state = eLIM_QUIET_BEGIN\n"));
        pMac->lim.gLimSpecMgmt.quietState = eLIM_QUIET_BEGIN;
    }
#endif
  }
  else
  {
    // eLIM_STA_ROLE
    switch( pMac->lim.gLimSpecMgmt.quietState )
    {
      case eLIM_QUIET_RUNNING:
        // Transition to eLIM_QUIET_INIT
        pMac->lim.gLimSpecMgmt.quietState = eLIM_QUIET_INIT;

        if( !pMac->lim.gLimTriggerBackgroundScanDuringQuietBss || (glimTriggerBackgroundScanDuringQuietBss_Status == eSIR_FALSE) )
        {
          // Resume data traffic only if channel switch is not running in silent mode.
          if (!((pMac->lim.gLimSpecMgmt.dot11hChanSwState == eLIM_11H_CHANSW_RUNNING) &&
                  (pMac->lim.gLimChannelSwitch.switchMode == eSIR_CHANSW_MODE_SILENT)))
          {
              limFrameTransmissionControl(pMac, eLIM_TX_ALL, eLIM_RESUME_TX);
              limRestorePreQuietState(pMac);
          }
      
          /* Reset status flag */
          if(glimTriggerBackgroundScanDuringQuietBss_Status == eSIR_FALSE)
              glimTriggerBackgroundScanDuringQuietBss_Status = eSIR_TRUE;

          limLog( pMac, LOG2,
              FL("Quiet BSS: Resuming traffic...\n"));
        }
        else
        {
          //
          // Nothing specific to be done in this case
          // A background scan that was triggered during
          // SIR_LIM_QUIET_TIMEOUT will complete on its own
          //
          limLog( pMac, LOG2,
              FL("Background scan should be complete now...\n"));
        }
        break;

      case eLIM_QUIET_INIT:
      case eLIM_QUIET_BEGIN:
      case eLIM_QUIET_END:
        PELOG2(limLog(pMac, LOG2, FL("Quiet state not in RUNNING\n"));)
        /* If the quiet period has ended, then resume the frame transmission */
        limFrameTransmissionControl(pMac, eLIM_TX_ALL, eLIM_RESUME_TX);
        limRestorePreQuietState(pMac);
        pMac->lim.gLimSpecMgmt.quietState = eLIM_QUIET_INIT;
        break;

      default:
        //
        // As of now, nothing to be done
        //
        break;
    }
  }
}
#ifdef WLAN_SOFTAP_FEATURE
/**
 * limProcessWPSOverlapTimeout
 *
 * FUNCTION: This function call limWPSPBCTimeout() to clean WPS PBC probe request entries
 *
 * LOGIC:
 *
 * ASSUMPTIONS:
 *
 * NOTE:
 *
 * @param pMac - Pointer to Global MAC structure
 *
 * @return None
 */
#if 0
void limProcessWPSOverlapTimeout(tpAniSirGlobal pMac)
{

    tpPESession psessionEntry;
    tANI_U32 sessionId;
    
    if (tx_timer_activate(&pMac->lim.limTimers.gLimWPSOverlapTimerObj.gLimWPSOverlapTimer) != TX_SUCCESS)
    {
            limLog(pMac, LOGP, FL("tx_timer_activate failed\n"));
    }

    sessionId = pMac->lim.limTimers.gLimWPSOverlapTimerObj.sessionId;

    PELOGE(limLog(pMac, LOGE, FL("WPS overlap timeout, sessionId=%d\n"), sessionId);)

    if((psessionEntry = peFindSessionBySessionId(pMac, sessionId)) == NULL) 
    {
        PELOGE(limLog(pMac, LOGP,FL("Session Does not exist for given sessionID\n"));)
        return;
    }
    
    limWPSPBCTimeout(pMac, psessionEntry);
}
#endif
#endif

/**
 * limUpdateQuietIEFromBeacon
 *
 * FUNCTION:
 *
 * LOGIC:
 *  When the Quiet BSS IE is encountered when parsing for
 *  Beacons/PR's, this function is called.
 *  Based on the configuration of the Quiet Interval -
 *  a) A TIMER (gLimQuietTimer) is started for the
 *  specified interval
 *  b) Upon expiry of the TIMER interval, the STA will
 *  shut-off Tx/Rx
 *  c) The STA will then start another TIMER (in this
 *  case, gLimQuietBssTimer TIMER object), with the
 *  timeout duration set to the value specified in the
 *  Quiet BSS IE
 *
 * 1) To setup the Quiet BSS "interval". In this case,
 * this TIMER indicates that the STA has to shut-off its
 * Tx/Rx AFTER the specified interval
 * Set gLimQuietState = eLIM_QUIET_BEGIN
 *
 * 2) To setup the Quiet BSS "duration". After the said
 * specified "interval", the STA has to shut-off Tx/Rx
 * for this duration
 * Set gLimQuietState = eLIM_QUIET_RUNNING
 *
 * ASSUMPTIONS:
 *
 * NOTE:
 *
 * @param pMac - Pointer to Global MAC structure
 * @param pQuietIE - Pointer to tDot11fIEQuiet
 *
 * @return None
 */
void limUpdateQuietIEFromBeacon( struct sAniSirGlobal *pMac,
    tDot11fIEQuiet *pQuietIE, tpPESession psessionEntry )
{
#if defined(ANI_PRODUCT_TYPE_CLIENT) || defined(ANI_AP_CLIENT_SDK)
    tANI_U16 beaconPeriod;
    tANI_U32 val;

    if (psessionEntry->limSystemRole != eLIM_STA_ROLE)
        return;
    
   PELOG1(limLog(pMac, LOG1, FL("Quiet state = %d, Quiet Count = %d\n"),
        pMac->lim.gLimSpecMgmt.quietState, pQuietIE->count);)
    if (!pMac->lim.gLim11hEnable)
        return;
    // The (Titan) AP is requesting this (Titan) STA to
    // honor this Quiet IE REQ and shut-off Tx/Rx. If we're
    // receiving this a second time around (because the AP
    // could be down-counting the Quiet BSS Count), the STA
    // will have to reset the timeout interval accordingly
    //

    if (pQuietIE->count == 0)
    {
        PELOG1(limLog(pMac, LOG1, FL("Ignoring quiet count == 0->reserved\n"));)
        return;
    }
    
    if( eSIR_SUCCESS !=
        wlan_cfgGetInt( pMac, WNI_CFG_BEACON_INTERVAL, &val ))
        beaconPeriod = (tANI_U16) WNI_CFG_BEACON_INTERVAL_APDEF;
    else
        beaconPeriod = (tANI_U16) val;

     /* (qd * 2^10)/1000 */
    pMac->lim.gLimSpecMgmt.quietDuration_TU = pQuietIE->duration;
    // The STA needs to shut-off Tx/Rx "for" this interval (in milliSeconds)
    /* Need to convert from TU to system TICKS */
    pMac->lim.gLimSpecMgmt.quietDuration = SYS_MS_TO_TICKS(
                           SYS_TU_TO_MS(pMac->lim.gLimSpecMgmt.quietDuration_TU));

    if (pMac->lim.gLimSpecMgmt.quietDuration_TU == 0)
    {
        PELOG1(limLog(pMac, LOG1, FL("Zero duration in quiet IE\n"));)
        return;
    }
    
    // The STA needs to shut-off Tx/Rx "after" this interval
    pMac->lim.gLimSpecMgmt.quietTimeoutValue =
              (beaconPeriod * pQuietIE->count) + pQuietIE->offset;

    limLog( pMac, LOG2,
        FL( "STA shut-off will begin in %d milliseconds & last for %d ticks\n"),
        pMac->lim.gLimSpecMgmt.quietTimeoutValue,
        pMac->lim.gLimSpecMgmt.quietDuration );

    /* Disable, Stop background scan if enabled and running */
    limDeactivateAndChangeTimer(pMac, eLIM_BACKGROUND_SCAN_TIMER);

    /* Stop heart-beat timer to stop heartbeat disassociation */
    limHeartBeatDeactivateAndChangeTimer(pMac, psessionEntry);

    if(pMac->lim.gLimSmeState == eLIM_SME_LINK_EST_WT_SCAN_STATE ||
        pMac->lim.gLimSmeState == eLIM_SME_CHANNEL_SCAN_STATE)
    {
        PELOGW(limLog(pMac, LOGW, FL("Posting finish scan as we are in scan state\n"));)
        /* Stop ongoing scanning if any */
        if (GET_LIM_PROCESS_DEFD_MESGS(pMac))
            limSendHalFinishScanReq(pMac, eLIM_HAL_FINISH_SCAN_WAIT_STATE);
        else
        {
            limRestorePreQuietState(pMac);
        }
    }
    else
    {
        PELOG1(limLog(pMac, LOG1, FL("Not in scan state, start quiet timer\n"));)
        /** We are safe to switch channel at this point */
        limStartQuietTimer(pMac, psessionEntry->peSessionId);
    }
    
    // Transition to eLIM_QUIET_BEGIN
    pMac->lim.gLimSpecMgmt.quietState = eLIM_QUIET_BEGIN;
#endif
}


/**----------------------------------------------
\fn        limStartQuietTimer
\brief    Starts the quiet timer.

\param pMac
\return NONE
-----------------------------------------------*/
void limStartQuietTimer(tpAniSirGlobal pMac, tANI_U8 sessionId)
{
    tpPESession psessionEntry;
    psessionEntry = peFindSessionBySessionId(pMac , sessionId);

    if(psessionEntry == NULL) {
        limLog(pMac, LOGP,FL("Session Does not exist for given sessionID\n"));
        return;
    }

#if defined(ANI_PRODUCT_TYPE_CLIENT) || defined(ANI_AP_CLIENT_SDK)

    if (psessionEntry->limSystemRole != eLIM_STA_ROLE)
        return;
    // First, de-activate Timer, if its already active
    limCancelDot11hQuiet(pMac, psessionEntry);
    
    limDeactivateAndChangeTimer(pMac, eLIM_QUIET_TIMER);
    MTRACE(macTrace(pMac, TRACE_CODE_TIMER_ACTIVATE, 0, eLIM_QUIET_TIMER));
    
    pMac->lim.limTimers.gLimQuietTimer.sessionId = sessionId;
    if( TX_SUCCESS != tx_timer_activate(&pMac->lim.limTimers.gLimQuietTimer))
    {
        limLog( pMac, LOGE,
            FL("Unable to activate gLimQuietTimer! STA cannot honor Quiet BSS!\n"));
        limRestorePreQuietState(pMac);

        pMac->lim.gLimSpecMgmt.quietState = eLIM_QUIET_INIT;
        return;
    }
#endif
}

#ifdef ANI_PRODUCT_TYPE_AP

/**
 * computeChannelSwitchCount
 *
 * FUNCTION:
 *     Function used by limProcessSmeSwitchChlReq()
 *     to compute channel switch count.
 *
 * LOGIC:
 *    Channel Switch Count is the number of TBTT until AP switches
 *    to a new channel.  The value of Channel Switch Count is computed
 *    in a way, such that channel switch will always take place after
 *    a DTIM.  By doing so, it is guaranteed that station in power save
 *    mode can receive the message and switch to new channel accordingly.
 *    AP can also announce the channel switch several dtims ahead of time.
 *    by setting the dtimFactor value greater than 1.
 *
 * ASSUMPTIONS:
 *
 * NOTE:
 *
 * @param   dtimFactor
 * @return  channel switch count
 */
tANI_U32 computeChannelSwitchCount(tpAniSirGlobal pMac, tANI_U32 dtimFactor)
{
    tANI_U32 dtimPeriod;
    tANI_U32 dtimCount;

    if (wlan_cfgGetInt(pMac, WNI_CFG_DTIM_PERIOD, &dtimPeriod) != eSIR_SUCCESS)
        PELOGE(limLog(pMac, LOGE, FL("wlan_cfgGetInt failed for WNI_CFG_DTIM_PERIOD \n"));)

    dtimCount = pMac->pmm.gPmmTim.dtimCount;

    if (dtimFactor <= 1)
        return (dtimCount + 1);
    else
        return (((dtimFactor -1)*dtimPeriod) + 1 + dtimCount);
}
#endif

/** ------------------------------------------------------------------------ **/
/**
 * keep track of the number of ANI peers associated in the BSS
 * For the first and last ANI peer, we have to update EDCA params as needed
 *
 * When the first ANI peer joins the BSS, we notify SCH
 * When the last ANI peer leaves the BSS, we notfiy SCH
 */
void
limUtilCountStaAdd(
    tpAniSirGlobal pMac,
    tpDphHashNode  pSta,
    tpPESession psessionEntry)
{

    if ((! pSta) || (! pSta->valid) || (! pSta->aniPeer) || (pSta->fAniCount))
        return;

    pSta->fAniCount = 1;

    if (pMac->lim.gLimNumOfAniSTAs++ != 0)
        return;

    // get here only if this is the first ANI peer in the BSS
    schEdcaProfileUpdate(pMac, psessionEntry);
}

void
limUtilCountStaDel(
    tpAniSirGlobal pMac,
    tpDphHashNode  pSta,
    tpPESession psessionEntry)
{

    if ((pSta == NULL) || (pSta->aniPeer == eHAL_CLEAR) || (! pSta->fAniCount))
        return;

    /* Only if sta is invalid and the validInDummyState bit is set to 1,
     * then go ahead and update the count and profiles. This ensures
     * that the "number of ani station" count is properly incremented/decremented.
     */
    if (pSta->valid == 1)
         return;

    pSta->fAniCount = 0;

    if (pMac->lim.gLimNumOfAniSTAs <= 0)
    {
        limLog(pMac, LOGE, FL("CountStaDel: ignoring Delete Req when AniPeer count is %d\n"),
               pMac->lim.gLimNumOfAniSTAs);
        return;
    }

    pMac->lim.gLimNumOfAniSTAs--;

    if (pMac->lim.gLimNumOfAniSTAs != 0)
        return;

    // get here only if this is the last ANI peer in the BSS
    schEdcaProfileUpdate(pMac, psessionEntry);
}

/** -------------------------------------------------------------
\fn limGetHtCbAdminState
\brief provides CB Admin state
\param     tpAniSirGlobal    pMac
\param     tDot11fIEHTCaps htCaps,
\param     tANI_U8 *titanHtCaps 
\return     none
  -------------------------------------------------------------*/
void limGetHtCbAdminState( tpAniSirGlobal pMac,
        tDot11fIEHTCaps htCaps,
        tANI_U8 *titanHtCaps )
{
    // Extract secondary channel info wrt Channel Bonding
    if(htCaps.supportedChannelWidthSet)
        SME_SET_CB_ADMIN_STATE( *titanHtCaps, eHAL_SET );    
    else
        SME_SET_CB_ADMIN_STATE( *titanHtCaps, eHAL_CLEAR);
    

  // And the final TITAN HT capabilities bitmap is...
  limLog( pMac, LOG2,
      FL("TITAN HT capabilities in BSS Description = %1d\n"),
        *titanHtCaps);
}
/** -------------------------------------------------------------
\fn limGetHtCbOpState
\brief provides CB operational state
\param     tpAniSirGlobal    pMac
\param     tDot11fIEHTInfo htInfo,
\param     tANI_U8 *titanHtCaps
\return     none
  -------------------------------------------------------------*/
void limGetHtCbOpState( tpAniSirGlobal pMac,
        tDot11fIEHTInfo htInfo,
        tANI_U8 *titanHtCaps )
{
    // Extract secondary channel info wrt Channel Bonding
    if(htInfo.secondaryChannelOffset)
    {
      if(PHY_DOUBLE_CHANNEL_LOW_PRIMARY == htInfo.secondaryChannelOffset)
          SME_SET_CB_OPER_STATE( *titanHtCaps,
            eANI_CB_SECONDARY_UP );
      else if(PHY_DOUBLE_CHANNEL_HIGH_PRIMARY == htInfo.secondaryChannelOffset)
          SME_SET_CB_OPER_STATE( *titanHtCaps,
            eANI_CB_SECONDARY_DOWN );
    }

  // And the final TITAN HT capabilities bitmap is...
  limLog( pMac, LOG2,
      FL("TITAN HT capabilities in BSS Description = %1d\n"),
        *titanHtCaps);
}

/**
 * limSwitchPrimaryChannel()
 *
 *FUNCTION:
 *  This function changes the current operating channel
 *  and sets the new new channel ID in WNI_CFG_CURRENT_CHANNEL.
 *
 *NOTE:
 * @param  pMac        Pointer to Global MAC structure
 * @param  newChannel  new chnannel ID
 * @return NONE
 */
void limSwitchPrimaryChannel(tpAniSirGlobal pMac, tANI_U8 newChannel,tpPESession psessionEntry)
{
#if !defined WLAN_FEATURE_VOWIFI  
    tANI_U32 localPwrConstraint;
#endif
    
    PELOG3(limLog(pMac, LOG3, FL("limSwitchPrimaryChannel: old chnl %d --> new chnl %d \n"),
           psessionEntry->currentOperChannel, newChannel);)
    psessionEntry->currentOperChannel= newChannel;
    pMac->lim.gLimRFBand = limGetRFBand(newChannel);

    #if 0
    if (cfgSetInt(pMac, WNI_CFG_CURRENT_CHANNEL, newChannel) != eSIR_SUCCESS)
    {
        limLog(pMac, LOGP, FL("set CURRENT_CHANNEL at CFG fail.\n"));
        return;
    }
    #endif //  TO SUPPORT BT-AMP 
#if defined WLAN_FEATURE_VOWIFI  
    limSendSwitchChnlParams(pMac, newChannel, eHT_SECONDARY_CHANNEL_OFFSET_NONE,
                                                   psessionEntry->maxTxPower, psessionEntry->peSessionId);
#else
    if(wlan_cfgGetInt(pMac, WNI_CFG_LOCAL_POWER_CONSTRAINT, &localPwrConstraint) != eSIR_SUCCESS)
    {
        limLog( pMac, LOGP, FL( "Unable to read Local Power Constraint from cfg\n" ));
        return;
    }
    limSendSwitchChnlParams(pMac, newChannel, eHT_SECONDARY_CHANNEL_OFFSET_NONE,
                                                   (tPowerdBm)localPwrConstraint, psessionEntry->peSessionId);
#endif
    return;
}


/**
 * limSwitchPrimarySecondaryChannel()
 *
 *FUNCTION:
 *  This function changes the primary and secondary channel.
 *  If 11h is enabled and user provides a "new channel ID"
 *  that is different from the current operating channel,
 *  then we must set this new channel in WNI_CFG_CURRENT_CHANNEL,
 *  assign notify LIM of such change.
 *
 *NOTE:
 * @param  pMac        Pointer to Global MAC structure
 * @param  newChannel  New chnannel ID (or current channel ID)
 * @param  subband     CB secondary info:
 *                       - eANI_CB_SECONDARY_NONE
 *                       - eANI_CB_SECONDARY_UP
 *                       - eANI_CB_SECONDARY_DOWN
 * @return NONE
 */
void limSwitchPrimarySecondaryChannel(tpAniSirGlobal pMac, tANI_U8 newChannel, tAniCBSecondaryMode subband)
{
#if !defined WLAN_FEATURE_VOWIFI  
    tANI_U32 localPwrConstraint;
#endif

    tpPESession psessionEntry =  &pMac->lim.gpSession[0]; //TBD-RAJESH HOW TO GET sessionEntry?????
#if !defined WLAN_FEATURE_VOWIFI  
    if(wlan_cfgGetInt(pMac, WNI_CFG_LOCAL_POWER_CONSTRAINT, &localPwrConstraint) != eSIR_SUCCESS) {
        limLog( pMac, LOGP, FL( "Unable to get Local Power Constraint from cfg\n" ));
        return;
    }
#endif

    switch(subband)
    {
        case eANI_CB_SECONDARY_NONE:
            PELOGW(limLog(pMac, LOGW, FL("Disable CB SECONDARY\n"));)
            /* If CB was on, turn it off, otherwise, do nothing */
            if(GET_CB_OPER_STATE(pMac->lim.gCbState))
            {
                /* Turn off CB in HW and SW.  SW and HW cbstate must match!  Otherwise, will hit ASSERT case */
                SET_CB_OPER_STATE(pMac->lim.gCbState, eHAL_CLEAR);
                /* Clean up station entry if we're not STA */
            }
            if (cfgSetInt(pMac, WNI_CFG_CB_SECONDARY_CHANNEL_STATE, WNI_CFG_CB_SECONDARY_CHANNEL_STATE_NONE) != eSIR_SUCCESS)
                limLog(pMac, LOGP, FL("cfgSetInt WNI_CFG_CB_SECONDARY_CHANNEL_STATE failed \n"));

#if defined WLAN_FEATURE_VOWIFI  
            limSendSwitchChnlParams(pMac, newChannel, eHT_SECONDARY_CHANNEL_OFFSET_NONE, psessionEntry->maxTxPower, psessionEntry->peSessionId);
#else
            //Send Message to HAL to update the channel
            limSendSwitchChnlParams(pMac, newChannel, eHT_SECONDARY_CHANNEL_OFFSET_NONE, (tPowerdBm)localPwrConstraint, psessionEntry->peSessionId);
#endif
            break;

        case eANI_CB_SECONDARY_UP:
            PELOGW(limLog(pMac, LOGW, FL("Switch CB SECONDARY to UP.\n"));)
            SET_CB_SEC_CHANNEL(pMac->lim.gCbState, eHAL_SET);
            if (cfgSetInt(pMac, WNI_CFG_CB_SECONDARY_CHANNEL_STATE, WNI_CFG_CB_SECONDARY_CHANNEL_STATE_HIGHER) != eSIR_SUCCESS)
                limLog(pMac, LOGP, FL("cfgSetInt WNI_CFG_CB_SECONDARY_CHANNEL_STATE failed \n"));

            /* If CB was off, turn it on, otherwise, do nothing */
            if(!GET_CB_OPER_STATE(pMac->lim.gCbState))
            {
                /* Turn on CB in HW and SW.  SW and HW cbstate must match!  Otherwise, will hit ASSERT case */
                SET_CB_OPER_STATE(pMac->lim.gCbState, eHAL_SET);
            }
            //Send Message to HAL to update the channel
            //enums for secondary channel offset for Titan and 11n are different
#if defined WLAN_FEATURE_VOWIFI  
            limSendSwitchChnlParams(pMac, newChannel, eHT_SECONDARY_CHANNEL_OFFSET_UP, psessionEntry->maxTxPower, psessionEntry->peSessionId);
#else
            limSendSwitchChnlParams(pMac, newChannel, eHT_SECONDARY_CHANNEL_OFFSET_UP, (tPowerdBm)localPwrConstraint, psessionEntry->peSessionId);
#endif
            break;

        case eANI_CB_SECONDARY_DOWN:
            PELOGW(limLog(pMac, LOGW, FL("Switch CB SECONDARY to LOWER.\n"));)
            SET_CB_SEC_CHANNEL(pMac->lim.gCbState, eHAL_CLEAR);
            if (cfgSetInt(pMac, WNI_CFG_CB_SECONDARY_CHANNEL_STATE, WNI_CFG_CB_SECONDARY_CHANNEL_STATE_LOWER) != eSIR_SUCCESS)
                limLog(pMac, LOGP, FL("cfgSetInt WNI_CFG_CB_SECONDARY_CHANNEL_STATE failed \n"));
            /* If CB was off, turn it on, otherwise, do nothing */
            if(!GET_CB_OPER_STATE(pMac->lim.gCbState))
            {
                /* Turn on CB in HW and SW.  SW and HW cbstate must match!  Otherwise, will hit ASSERT case */
                SET_CB_OPER_STATE(pMac->lim.gCbState, eHAL_SET);
                /* Update station entry if we're not STA */
            }
            //Send Message to HAL to update the channel
            //enums for secondary channel offset for Titan and 11n are different
#if defined WLAN_FEATURE_VOWIFI  
            limSendSwitchChnlParams(pMac, newChannel, eHT_SECONDARY_CHANNEL_OFFSET_NONE, psessionEntry->maxTxPower, psessionEntry->peSessionId);
#else
            limSendSwitchChnlParams(pMac, newChannel, eHT_SECONDARY_CHANNEL_OFFSET_DOWN, (tPowerdBm)localPwrConstraint, psessionEntry->peSessionId);
#endif
            break;

		case eANI_DONOT_USE_SECONDARY_MODE:
            break;
    }


    // We should only be changing primary and secondary channels on the fly
    // if this is 11h enabled.
    if (
#if 0
        pMac->lim.gLim11hEnable &&
#endif
        psessionEntry->currentOperChannel != newChannel)
    {
        limLog(pMac, LOGW,
            FL("switch old chnl %d --> new chnl %d \n"),
            psessionEntry->currentOperChannel, newChannel);

        #if 0

        if (cfgSetInt(pMac, WNI_CFG_CURRENT_CHANNEL, newChannel) != eSIR_SUCCESS)
            limLog(pMac, LOGP, FL("set CURRENT_CHANNEL at CFG fail.\n"));
        #endif // TO SUPPORT BT-AMP

        psessionEntry->currentOperChannel = newChannel;
    }

    return;
}


/**
 * limActiveScanAllowed()
 *
 *FUNCTION:
 * Checks if active scans are permitted on the given channel
 *
 *LOGIC:
 * The config variable SCAN_CONTROL_LIST contains pairs of (channelNum, activeScanAllowed)
 * Need to check if the channelNum matches, then depending on the corresponding
 * scan flag, return true (for activeScanAllowed==1) or false (otherwise).
 *
 *ASSUMPTIONS:
 *
 *NOTE:
 *
 * @param  pMac       Pointer to Global MAC structure
 * @param  channelNum channel number
 * @return None
 */

tANI_U8 limActiveScanAllowed(
    tpAniSirGlobal pMac,
    tANI_U8             channelNum)
{
    tANI_U32 i;
    tANI_U8  channelPair[WNI_CFG_SCAN_CONTROL_LIST_LEN];
    tANI_U32 len = WNI_CFG_SCAN_CONTROL_LIST_LEN;
    if (wlan_cfgGetStr(pMac, WNI_CFG_SCAN_CONTROL_LIST, channelPair, &len)
        != eSIR_SUCCESS)
    {
        PELOGE(limLog(pMac, LOGE, FL("Unable to get scan control list\n"));)
        return false;
    }

    if (len > WNI_CFG_SCAN_CONTROL_LIST_LEN)
    {
        limLog(pMac, LOGE, FL("Invalid scan control list length:%d\n"),
               len);
        return false;
    }

    for (i=0; (i+1) < len; i+=2)
    {
        if (channelPair[i] == channelNum)
            return ((channelPair[i+1] == eSIR_ACTIVE_SCAN) ? true : false);
    }
    return false;
}

/**
 * limTriggerBackgroundScanDuringQuietBss()
 *
 *FUNCTION:
 * This function is applicable to the STA only.
 * This function is called by limProcessQuietTimeout(),
 * when it is time to honor the Quiet BSS IE from the AP.
 *
 *LOGIC:
 * If 11H is enabled:
 * We cannot trigger a background scan. The STA needs to
 * shut-off Tx/Rx.
 * If 11 is not enabled:
 * Determine if the next channel that we are going to
 * scan is NOT the same channel (or not) on which the
 * Quiet BSS was requested.
 * If yes, then we cannot trigger a background scan on
 * this channel. Return with a false.
 * If no, then trigger a background scan. Return with
 * a true.
 *
 *ASSUMPTIONS:
 *
 *NOTE:
 * This API is redundant if the existing API,
 * limTriggerBackgroundScan(), were to return a valid
 * response instead of returning void.
 * If possible, try to revisit this API
 *
 * @param  pMac Pointer to Global MAC structure
 * @return eSIR_TRUE, if a background scan was attempted
 *         eSIR_FALSE, if not
 */
tAniBool limTriggerBackgroundScanDuringQuietBss( tpAniSirGlobal pMac )
{
    tAniBool bScanTriggered = eSIR_FALSE;
#if defined(ANI_PRODUCT_TYPE_CLIENT) || defined(ANI_AP_CLIENT_SDK)

    

    //TBD-RAJESH HOW TO GET sessionEntry?????
    tpPESession psessionEntry = &pMac->lim.gpSession[0];

    if (psessionEntry->limSystemRole != eLIM_STA_ROLE)
        return bScanTriggered;
    
    if( !pMac->lim.gLim11hEnable )
    {
        tSirMacChanNum bgScanChannelList[WNI_CFG_BG_SCAN_CHANNEL_LIST_LEN];
        tANI_U32 len = WNI_CFG_BG_SCAN_CHANNEL_LIST_LEN;

        // Determine the next scan channel

        // Get background scan channel list from CFG
        if( eSIR_SUCCESS == wlan_cfgGetStr( pMac,
          WNI_CFG_BG_SCAN_CHANNEL_LIST,
          (tANI_U8 *) bgScanChannelList,
          (tANI_U32 *) &len ))
        {
            // Ensure that we do not go off scanning on the same
        // channel on which the Quiet BSS was requested
            if( psessionEntry->currentOperChannel!=
                bgScanChannelList[pMac->lim.gLimBackgroundScanChannelId] )
            {
            // For now, try and attempt a background scan. It will
            // be ideal if this API actually returns a success or
            // failure instead of having a void return type
            limTriggerBackgroundScan( pMac );

            bScanTriggered = eSIR_TRUE;
        }
        else
        {
            limLog( pMac, LOGW,
                FL("The next SCAN channel is the current operating channel on which a Quiet BSS is requested.! A background scan will not be triggered during this Quiet BSS period...\n"));
        }
    }
    else
    {
      limLog( pMac, LOGW,
          FL("Unable to retrieve WNI_CFG_VALID_CHANNEL_LIST from CFG! A background scan will not be triggered during this Quiet BSS period...\n"));
    }
  }
#endif
  return bScanTriggered;
}


/**
 * limGetHTCapability()
 *
 *FUNCTION:
 * A utility function that returns the "current HT capability state" for the HT
 * capability of interest (as requested in the API)
 *
 *LOGIC:
 * This routine will return with the "current" setting of a requested HT
 * capability. This state info could be retrieved from -
 * a) CFG (for static entries)
 * b) Run time info
 *   - Dynamic state maintained by LIM
 *   - Configured at radio init time by SME
 *
 *
 *ASSUMPTIONS:
 * NA
 *
 *NOTE:
 *
 * @param  pMac  Pointer to Global MAC structure
 * @param  htCap The HT capability being queried
 * @return tANI_U8 The current state of the requested HT capability is returned in a
 *            tANI_U8 variable
 */

#ifdef WLAN_SOFTAP_FEATURE 
tANI_U8 limGetHTCapability( tpAniSirGlobal pMac,
        tANI_U32 htCap, tpPESession psessionEntry)
#else
tANI_U8 limGetHTCapability( tpAniSirGlobal pMac,
        tANI_U32 htCap )
#endif
{
tANI_U8 retVal = 0;
tANI_U8 *ptr;
tANI_U32  cfgValue;
tSirMacHTCapabilityInfo macHTCapabilityInfo = {0};
tSirMacExtendedHTCapabilityInfo macExtHTCapabilityInfo = {0};
tSirMacTxBFCapabilityInfo macTxBFCapabilityInfo = {0};
tSirMacASCapabilityInfo macASCapabilityInfo = {0};

  //
  // Determine which CFG to read from. Not ALL of the HT
  // related CFG's need to be read each time this API is
  // accessed
  //
  if( htCap >= eHT_ANTENNA_SELECTION &&
      htCap < eHT_SI_GRANULARITY )
  {
    // Get Antenna Seletion HT Capabilities
    if( eSIR_SUCCESS != wlan_cfgGetInt( pMac, WNI_CFG_AS_CAP, &cfgValue ))
      cfgValue = 0;
    ptr = (tANI_U8 *) &macASCapabilityInfo;
    *((tANI_U8 *)ptr) =  (tANI_U8) (cfgValue & 0xff);
  }
  else
  {
    if( htCap >= eHT_TX_BEAMFORMING &&
        htCap < eHT_ANTENNA_SELECTION )
    {
      // Get Transmit Beam Forming HT Capabilities
      if( eSIR_SUCCESS != wlan_cfgGetInt( pMac, WNI_CFG_TX_BF_CAP, &cfgValue ))
        cfgValue = 0;
      ptr = (tANI_U8 *) &macTxBFCapabilityInfo;
      *((tANI_U32 *)ptr) =  (tANI_U32) (cfgValue);
    }
    else
    {
      if( htCap >= eHT_PCO &&
          htCap < eHT_TX_BEAMFORMING )
      {
        // Get Extended HT Capabilities
        if( eSIR_SUCCESS != wlan_cfgGetInt( pMac, WNI_CFG_EXT_HT_CAP_INFO, &cfgValue ))
          cfgValue = 0;
        ptr = (tANI_U8 *) &macExtHTCapabilityInfo;
        *((tANI_U16 *)ptr) =  (tANI_U16) (cfgValue & 0xffff);
      }
      else
      {
        if( htCap < eHT_MAX_RX_AMPDU_FACTOR )
        {
          // Get HT Capabilities
          if( eSIR_SUCCESS != wlan_cfgGetInt( pMac, WNI_CFG_HT_CAP_INFO, &cfgValue ))
            cfgValue = 0;
          ptr = (tANI_U8 *) &macHTCapabilityInfo;
          // CR 265282 MDM SoftAP 2.4PL: SoftAP boot up crash in 2.4 PL builds while same WLAN SU is working on 2.1 PL
          *ptr++ = cfgValue & 0xff;
          *ptr = (cfgValue >> 8) & 0xff;
        }
      }
    }
  }

  switch( htCap )
  {
    case eHT_LSIG_TXOP_PROTECTION:
      retVal = pMac->lim.gHTLsigTXOPProtection;
      break;

    case eHT_STBC_CONTROL_FRAME:
      retVal = (tANI_U8) macHTCapabilityInfo.stbcControlFrame;
      break;

    case eHT_PSMP:
      retVal = pMac->lim.gHTPSMPSupport;
      break;

    case eHT_DSSS_CCK_MODE_40MHZ:
      retVal = pMac->lim.gHTDsssCckRate40MHzSupport;
      break;

    case eHT_MAX_AMSDU_LENGTH:
      retVal = (tANI_U8) macHTCapabilityInfo.maximalAMSDUsize;
      break;

    case eHT_DELAYED_BA:
      retVal = (tANI_U8) macHTCapabilityInfo.delayedBA;
      break;

    case eHT_RX_STBC:
      retVal = (tANI_U8) macHTCapabilityInfo.rxSTBC;
      break;

    case eHT_TX_STBC:
      retVal = (tANI_U8) macHTCapabilityInfo.txSTBC;
      break;

    case eHT_SHORT_GI_40MHZ:
      retVal = (tANI_U8) macHTCapabilityInfo.shortGI40MHz;
      break;

    case eHT_SHORT_GI_20MHZ:
      retVal = (tANI_U8) macHTCapabilityInfo.shortGI20MHz;
      break;

    case eHT_GREENFIELD:
      retVal = (tANI_U8) macHTCapabilityInfo.greenField;
      break;

    case eHT_MIMO_POWER_SAVE:
      retVal = (tANI_U8) pMac->lim.gHTMIMOPSState;
      break;

    case eHT_SUPPORTED_CHANNEL_WIDTH_SET:
      retVal = (tANI_U8) macHTCapabilityInfo.supportedChannelWidthSet;
      break;

    case eHT_ADVANCED_CODING:
      retVal = (tANI_U8) macHTCapabilityInfo.advCodingCap;
      break;

    case eHT_MAX_RX_AMPDU_FACTOR:
      retVal = pMac->lim.gHTMaxRxAMpduFactor;
      break;

    case eHT_MPDU_DENSITY:
      retVal = pMac->lim.gHTAMpduDensity;
      break;

    case eHT_PCO:
      retVal = (tANI_U8) macExtHTCapabilityInfo.pco;
      break;

    case eHT_TRANSITION_TIME:
      retVal = (tANI_U8) macExtHTCapabilityInfo.transitionTime;
      break;

    case eHT_MCS_FEEDBACK:
      retVal = (tANI_U8) macExtHTCapabilityInfo.mcsFeedback;
      break;

    case eHT_TX_BEAMFORMING:
      retVal = (tANI_U8) macTxBFCapabilityInfo.txBF;
      break;

    case eHT_ANTENNA_SELECTION:
      retVal = (tANI_U8) macASCapabilityInfo.antennaSelection;
      break;

    case eHT_SI_GRANULARITY:
      retVal = pMac->lim.gHTServiceIntervalGranularity;
      break;

    case eHT_CONTROLLED_ACCESS:
      retVal = pMac->lim.gHTControlledAccessOnly;
      break;

    case eHT_RIFS_MODE:
#ifdef WLAN_SOFTAP_FEATURE
      if( psessionEntry->limSystemRole == eLIM_AP_ROLE )
          retVal = psessionEntry->fRIFSMode;
	  else
#endif	  	
	      retVal = pMac->lim.gHTRifsMode;
      break;

    case eHT_RECOMMENDED_TX_WIDTH_SET:
      retVal = pMac->lim.gHTRecommendedTxWidthSet;
      break;

    case eHT_EXTENSION_CHANNEL_OFFSET:
      retVal = pMac->lim.gHTSecondaryChannelOffset;
      break;

    case eHT_OP_MODE:
#ifdef WLAN_SOFTAP_FEATURE
      if(psessionEntry->limSystemRole == eLIM_AP_ROLE )
          retVal = psessionEntry->htOperMode;
	  else
#endif
          retVal = pMac->lim.gHTOperMode;
      break;

    case eHT_BASIC_STBC_MCS:
      retVal = pMac->lim.gHTSTBCBasicMCS;
      break;

    case eHT_DUAL_CTS_PROTECTION:
      retVal = pMac->lim.gHTDualCTSProtection;
      break;

    case eHT_LSIG_TXOP_PROTECTION_FULL_SUPPORT:
      retVal = pMac->lim.gHTLSigTXOPFullSupport;
      break;

    case eHT_PCO_ACTIVE:
      retVal = pMac->lim.gHTPCOActive;
      break;

    case eHT_PCO_PHASE:
      retVal = pMac->lim.gHTPCOPhase;
      break;

    default:
      break;
  }

  return retVal;
}

#if 0
void limSetBssid(tpAniSirGlobal pMac, tANI_U8 *bssId)
{
    palCopyMemory( pMac->hHdd, pMac->lim.gLimBssid, bssId, sizeof(tSirMacAddr));
    return;
}

void limGetBssid(tpAniSirGlobal pMac, tANI_U8 *bssId)
{
    palCopyMemory( pMac->hHdd, bssId, pMac->lim.gLimBssid, sizeof(tSirMacAddr));
    return;
}

#endif
void limGetMyMacAddr(tpAniSirGlobal pMac, tANI_U8 *mac)
{
    palCopyMemory( pMac->hHdd, mac, pMac->lim.gLimMyMacAddr, sizeof(tSirMacAddr));
    return;
}




/** -------------------------------------------------------------
\fn limEnable11aProtection
\brief based on config setting enables\disables 11a protection.
\param      tANI_U8 enable : 1=> enable protection, 0=> disable protection.
\param      tANI_U8 overlap: 1=> called from overlap context, 0 => called from assoc context.
\param      tpUpdateBeaconParams pBeaconParams
\return      None
  -------------------------------------------------------------*/
tSirRetStatus
limEnable11aProtection(tpAniSirGlobal pMac, tANI_U8 enable,
    tANI_U8 overlap, tpUpdateBeaconParams pBeaconParams,tpPESession psessionEntry)
{

        //overlapping protection configuration check.
        if(overlap)
        {
#if (defined(ANI_PRODUCT_TYPE_AP) || defined(ANI_PRODUCT_TYPE_AP_SDK))
        if(psessionEntry->limSystemRole == eLIM_AP_ROLE && !pMac->lim.cfgProtection.overlapFromlla)
            {
                // protection disabled.
            PELOG3(limLog(pMac, LOG3, FL("overlap protection from 11a is disabled\n"));)
                return eSIR_SUCCESS;
            }
#endif
        }
        else
        {
            //normal protection config check
            if(!pMac->lim.cfgProtection.fromlla)
            {
                // protection disabled.
                PELOG3(limLog(pMac, LOG3, FL("protection from 11a is disabled\n"));)
                return eSIR_SUCCESS;
            }
        }

    if (enable)
    {
        //If we are AP and HT capable, we need to set the HT OP mode
        //appropriately.
        if(((eLIM_AP_ROLE == psessionEntry->limSystemRole)||(eLIM_BT_AMP_AP_ROLE == psessionEntry->limSystemRole))&&
              (true == psessionEntry->htCapabality))
        {
            if(overlap)
            {
                pMac->lim.gLimOverlap11aParams.protectionEnabled = true;
                if((eSIR_HT_OP_MODE_OVERLAP_LEGACY != pMac->lim.gHTOperMode) &&
                	(eSIR_HT_OP_MODE_MIXED != pMac->lim.gHTOperMode))
                {
                	pMac->lim.gHTOperMode = eSIR_HT_OP_MODE_OVERLAP_LEGACY;
                    limEnableHtRifsProtection(pMac, true, overlap, pBeaconParams,psessionEntry);          
                }
            }
            else
            {
                pMac->lim.gLim11aParams.protectionEnabled = true;
                if(eSIR_HT_OP_MODE_MIXED != pMac->lim.gHTOperMode)
                {
                	pMac->lim.gHTOperMode = eSIR_HT_OP_MODE_MIXED;
                    limEnableHtRifsProtection(pMac, true, overlap, pBeaconParams,psessionEntry);
                    limEnableHtOBSSProtection(pMac,  true, overlap, pBeaconParams,psessionEntry);         
                    
                }
            }
        }

        //This part is common for staiton as well.
        if(false == psessionEntry->llaCoexist)
        {
            PELOG1(limLog(pMac, LOG1, FL(" => protection from 11A Enabled\n"));)
            pBeaconParams->llaCoexist = psessionEntry->llaCoexist = true;
            pBeaconParams->paramChangeBitmap |= PARAM_llACOEXIST_CHANGED;
        }
    }
    else if (true == psessionEntry->llaCoexist)
    {
        //for AP role.
        //we need to take care of HT OP mode change if needed.
        //We need to take care of Overlap cases.
        if(eLIM_AP_ROLE == psessionEntry->limSystemRole)
        {
            if(overlap)
            {
                //Overlap Legacy protection disabled.
                pMac->lim.gLimOverlap11aParams.protectionEnabled = false;

                //We need to take care of HT OP mode iff we are HT AP.
                if(psessionEntry->htCapabality)
                {
					// no HT op mode change if any of the overlap protection enabled.
                    if(!(pMac->lim.gLimOverlap11aParams.protectionEnabled ||
                      	pMac->lim.gLimOverlapHt20Params.protectionEnabled ||
                    	pMac->lim.gLimOverlapNonGfParams.protectionEnabled))

                    {
                    	//Check if there is a need to change HT OP mode.
                     	if(eSIR_HT_OP_MODE_OVERLAP_LEGACY == pMac->lim.gHTOperMode)
                    	{
                            limEnableHtRifsProtection(pMac, false, overlap, pBeaconParams,psessionEntry);
                            limEnableHtOBSSProtection(pMac,  false, overlap, pBeaconParams,psessionEntry);        

                            if(pMac->lim.gLimHt20Params.protectionEnabled)
                            	pMac->lim.gHTOperMode = eSIR_HT_OP_MODE_NO_LEGACY_20MHZ_HT;
                            else
                            	pMac->lim.gHTOperMode = eSIR_HT_OP_MODE_PURE;
                        }
                    }
                }
            }
            else
            {
         	//Disable protection from 11A stations.
         	pMac->lim.gLim11aParams.protectionEnabled = false;
        	limEnableHtOBSSProtection(pMac,  false, overlap, pBeaconParams,psessionEntry);		  

         	//Check if any other non-HT protection enabled.
        	//Right now we are in HT OP Mixed mode.
         	//Change HT op mode appropriately.

        	//Change HT OP mode to 01 if any overlap protection enabled
        	if(pMac->lim.gLimOverlap11aParams.protectionEnabled ||
		       pMac->lim.gLimOverlapHt20Params.protectionEnabled ||
        	   pMac->lim.gLimOverlapNonGfParams.protectionEnabled)

	       {
        		pMac->lim.gHTOperMode = eSIR_HT_OP_MODE_OVERLAP_LEGACY;
		        limEnableHtRifsProtection(pMac, true, overlap, pBeaconParams,psessionEntry);
	        }
         	else if(pMac->lim.gLimHt20Params.protectionEnabled)
	       {
        		pMac->lim.gHTOperMode = eSIR_HT_OP_MODE_NO_LEGACY_20MHZ_HT;
		        limEnableHtRifsProtection(pMac, false, overlap, pBeaconParams,psessionEntry);
	        }
	        else
	        {
        		pMac->lim.gHTOperMode = eSIR_HT_OP_MODE_PURE;
		        limEnableHtRifsProtection(pMac, false, overlap, pBeaconParams,psessionEntry);
	        }
        }
        if(!pMac->lim.gLimOverlap11aParams.protectionEnabled &&
       	  !pMac->lim.gLim11aParams.protectionEnabled)
            {
                PELOG1(limLog(pMac, LOG1, FL("===> Protection from 11A Disabled\n"));)
                pBeaconParams->llaCoexist = psessionEntry->llaCoexist = false;
                pBeaconParams->paramChangeBitmap |= PARAM_llACOEXIST_CHANGED;
            }
        }
        //for station role
        else
        {
            PELOG1(limLog(pMac, LOG1, FL("===> Protection from 11A Disabled\n"));)
            pBeaconParams->llaCoexist = psessionEntry->llaCoexist = false;
            pBeaconParams->paramChangeBitmap |= PARAM_llACOEXIST_CHANGED;
        }
    }

    return eSIR_SUCCESS;
}

/** -------------------------------------------------------------
\fn limEnable11gProtection
\brief based on config setting enables\disables 11g protection.
\param      tANI_U8 enable : 1=> enable protection, 0=> disable protection.
\param      tANI_U8 overlap: 1=> called from overlap context, 0 => called from assoc context.
\param      tpUpdateBeaconParams pBeaconParams
\return      None
  -------------------------------------------------------------*/

tSirRetStatus
limEnable11gProtection(tpAniSirGlobal pMac, tANI_U8 enable,
    tANI_U8 overlap, tpUpdateBeaconParams pBeaconParams,tpPESession psessionEntry)
{

    //overlapping protection configuration check.
    if(overlap)
    {
#if (defined(ANI_PRODUCT_TYPE_AP) || defined(ANI_PRODUCT_TYPE_AP_SDK))
        if(((psessionEntry->limSystemRole == eLIM_AP_ROLE) ||(psessionEntry->limSystemRole == eLIM_BT_AMP_AP_ROLE ))  && !pMac->lim.cfgProtection.overlapFromllb)
        {
            // protection disabled.
            PELOG1(limLog(pMac, LOG1, FL("overlap protection from 11b is disabled\n"));)
            return eSIR_SUCCESS;
        }
#endif
    }
    else
    {
        //normal protection config check
#ifdef WLAN_SOFTAP_FEATURE
        if((psessionEntry->limSystemRole == eLIM_AP_ROLE ) &&
            !psessionEntry->cfgProtection.fromllb)
        {
            // protection disabled.
            PELOG1(limLog(pMac, LOG1, FL("protection from 11b is disabled\n"));)
            return eSIR_SUCCESS;
        } else if(psessionEntry->limSystemRole != eLIM_AP_ROLE)
#endif
        {
            if(!pMac->lim.cfgProtection.fromllb)
            {
                // protection disabled.
                PELOG1(limLog(pMac, LOG1, FL("protection from 11b is disabled\n"));)
                return eSIR_SUCCESS;
            }
        }
    }

    if (enable)
    {
        //If we are AP and HT capable, we need to set the HT OP mode
        //appropriately.
#ifdef WLAN_SOFTAP_FEATURE
        if((eLIM_AP_ROLE == psessionEntry->limSystemRole) &&
              (true == psessionEntry->htCapabality))
        {
            if(overlap)
            {
                psessionEntry->gLimOlbcParams.protectionEnabled = true;
                PELOGE(limLog(pMac, LOGE, FL("protection from olbc is enabled\n"));)
                if((eSIR_HT_OP_MODE_OVERLAP_LEGACY != psessionEntry->htOperMode) &&
                    (eSIR_HT_OP_MODE_MIXED != psessionEntry->htOperMode))
                {
                    psessionEntry->htOperMode = eSIR_HT_OP_MODE_OVERLAP_LEGACY;
                }
                //CR-263021: OBSS bit is not switching back to 0 after disabling the overlapping legacy BSS
                // This fixes issue of OBSS bit not set after 11b, 11g station leaves
                    limEnableHtRifsProtection(pMac, true, overlap, pBeaconParams,psessionEntry);
              //Not processing OBSS bit from other APs, as we are already taking care
              //of Protection from overlapping BSS based on erp IE or useProtection bit
                limEnableHtOBSSProtection(pMac,  true, overlap, pBeaconParams, psessionEntry);
            }
            else
            {
                psessionEntry->gLim11bParams.protectionEnabled = true;
                PELOGE(limLog(pMac, LOGE, FL("protection from 11b is enabled\n"));)
                if(eSIR_HT_OP_MODE_MIXED != psessionEntry->htOperMode)
                {
                    psessionEntry->htOperMode = eSIR_HT_OP_MODE_MIXED;
                    limEnableHtRifsProtection(pMac, true, overlap, pBeaconParams,psessionEntry);
                    limEnableHtOBSSProtection(pMac,  true, overlap, pBeaconParams,psessionEntry);     
                }
            }
        }else if ((eLIM_BT_AMP_AP_ROLE == psessionEntry->limSystemRole) &&
              (true == psessionEntry->htCapabality))
#else		
        if(((eLIM_AP_ROLE == psessionEntry->limSystemRole)|| (eLIM_BT_AMP_AP_ROLE == psessionEntry->limSystemRole)) &&
              (true == psessionEntry->htCapabality))
#endif
        {
            if(overlap)
            {
                pMac->lim.gLimOlbcParams.protectionEnabled = true;
                if((eSIR_HT_OP_MODE_OVERLAP_LEGACY != pMac->lim.gHTOperMode) &&
                   (eSIR_HT_OP_MODE_MIXED != pMac->lim.gHTOperMode))
                {
                    pMac->lim.gHTOperMode = eSIR_HT_OP_MODE_OVERLAP_LEGACY;
                    limEnableHtRifsProtection(pMac, true, overlap, pBeaconParams,psessionEntry);
                    //Not processing OBSS bit from other APs, as we are already taking care
                    //of Protection from overlapping BSS based on erp IE or useProtection bit
                }
            }
            else
            {
                pMac->lim.gLim11bParams.protectionEnabled = true;
                if(eSIR_HT_OP_MODE_MIXED != pMac->lim.gHTOperMode)
                { 
                    pMac->lim.gHTOperMode = eSIR_HT_OP_MODE_MIXED;
                    limEnableHtRifsProtection(pMac, true, overlap, pBeaconParams,psessionEntry);
                    limEnableHtOBSSProtection(pMac,  true, overlap, pBeaconParams,psessionEntry);     
                }
            }
        }

        //This part is common for staiton as well.
        if(false == psessionEntry->llbCoexist)
        {
            PELOG1(limLog(pMac, LOG1, FL("=> 11G Protection Enabled\n"));)
            pBeaconParams->llbCoexist = psessionEntry->llbCoexist = true;
            pBeaconParams->paramChangeBitmap |= PARAM_llBCOEXIST_CHANGED;
        }
    }
    else if (true == psessionEntry->llbCoexist)
    {
        //for AP role.
        //we need to take care of HT OP mode change if needed.
        //We need to take care of Overlap cases.
#ifdef WLAN_SOFTAP_FEATURE
        if(eLIM_AP_ROLE == psessionEntry->limSystemRole)
        {
            if(overlap)
            {
                //Overlap Legacy protection disabled.
                psessionEntry->gLimOlbcParams.protectionEnabled = false;

                //We need to take care of HT OP mode if we are HT AP.
                if(psessionEntry->htCapabality)
                {
                    // no HT op mode change if any of the overlap protection enabled.
                    if(!(psessionEntry->gLimOverlap11gParams.protectionEnabled ||
                        psessionEntry->gLimOverlapHt20Params.protectionEnabled ||
                        psessionEntry->gLimOverlapNonGfParams.protectionEnabled))
                    {
                        //Check if there is a need to change HT OP mode.
                        if(eSIR_HT_OP_MODE_OVERLAP_LEGACY == pMac->lim.gHTOperMode)
                        {
                            limEnableHtRifsProtection(pMac, false, overlap, pBeaconParams,psessionEntry);
                            limEnableHtOBSSProtection(pMac,  false, overlap, pBeaconParams,psessionEntry);            
                            if(psessionEntry->gLimHt20Params.protectionEnabled){
                                //Commenting out beacuse of CR 258588 WFA cert
                                //psessionEntry->htOperMode = eSIR_HT_OP_MODE_NO_LEGACY_20MHZ_HT;
                                psessionEntry->htOperMode = eSIR_HT_OP_MODE_PURE;
                            }
                            else
                                psessionEntry->htOperMode = eSIR_HT_OP_MODE_PURE;
                        }
                    }
                }
            }
            else
            {
                //Disable protection from 11B stations.
                psessionEntry->gLim11bParams.protectionEnabled = false;
                PELOGE(limLog(pMac, LOGE, FL("===> 11B Protection Disabled\n"));)
                //Check if any other non-HT protection enabled.
                if(!psessionEntry->gLim11gParams.protectionEnabled)
                {
                    //Right now we are in HT OP Mixed mode.
                    //Change HT op mode appropriately.
                    limEnableHtOBSSProtection(pMac,  false, overlap, pBeaconParams,psessionEntry);            

                    //Change HT OP mode to 01 if any overlap protection enabled
                    if(psessionEntry->gLimOlbcParams.protectionEnabled ||
                        psessionEntry->gLimOverlap11gParams.protectionEnabled ||
                        psessionEntry->gLimOverlapHt20Params.protectionEnabled ||
                        psessionEntry->gLimOverlapNonGfParams.protectionEnabled)
                    {
                        psessionEntry->htOperMode = eSIR_HT_OP_MODE_OVERLAP_LEGACY;
                        PELOGE(limLog(pMac, LOGE, FL("===> 11G Protection Disabled\n"));)
                        limEnableHtRifsProtection(pMac, true, overlap, pBeaconParams,psessionEntry);
                    }
                    else if(psessionEntry->gLimHt20Params.protectionEnabled)
                    {
                        //Commenting because of CR 258588 WFA cert
                        //psessionEntry->htOperMode = eSIR_HT_OP_MODE_NO_LEGACY_20MHZ_HT;
                        psessionEntry->htOperMode = eSIR_HT_OP_MODE_PURE;
                        PELOGE(limLog(pMac, LOGE, FL("===> 11G Protection Disabled\n"));)
                        limEnableHtRifsProtection(pMac, false, overlap, pBeaconParams,psessionEntry);
                    }
                    else
                    {
                        psessionEntry->htOperMode = eSIR_HT_OP_MODE_PURE;
                        limEnableHtRifsProtection(pMac, false, overlap, pBeaconParams,psessionEntry);
                    }
                }
            }
            if(!psessionEntry->gLimOlbcParams.protectionEnabled &&
                  !psessionEntry->gLim11bParams.protectionEnabled)
            {
                PELOGE(limLog(pMac, LOGE, FL("===> 11G Protection Disabled\n"));)
                pBeaconParams->llbCoexist = psessionEntry->llbCoexist = false;
                pBeaconParams->paramChangeBitmap |= PARAM_llBCOEXIST_CHANGED;
            }
        }else if(eLIM_BT_AMP_AP_ROLE == psessionEntry->limSystemRole)
#else
        if((eLIM_AP_ROLE == psessionEntry->limSystemRole)||((eLIM_BT_AMP_AP_ROLE == psessionEntry->limSystemRole)))
#endif
        {
            if(overlap)
            {
                //Overlap Legacy protection disabled.
                pMac->lim.gLimOlbcParams.protectionEnabled = false;

                //We need to take care of HT OP mode iff we are HT AP.
                if(psessionEntry->htCapabality)
                {
                    // no HT op mode change if any of the overlap protection enabled.
                    if(!(pMac->lim.gLimOverlap11gParams.protectionEnabled ||
                        pMac->lim.gLimOverlapHt20Params.protectionEnabled ||
                        pMac->lim.gLimOverlapNonGfParams.protectionEnabled))

                    {
                        //Check if there is a need to change HT OP mode.
                        if(eSIR_HT_OP_MODE_OVERLAP_LEGACY == pMac->lim.gHTOperMode)
                        {
                            limEnableHtRifsProtection(pMac, false, overlap, pBeaconParams,psessionEntry);
                            limEnableHtOBSSProtection(pMac,  false, overlap, pBeaconParams,psessionEntry);            
                            if(pMac->lim.gLimHt20Params.protectionEnabled)
                                pMac->lim.gHTOperMode = eSIR_HT_OP_MODE_NO_LEGACY_20MHZ_HT;
                            else
                                pMac->lim.gHTOperMode = eSIR_HT_OP_MODE_PURE;
                        }
                    }
                }
            }
            else
            {
                //Disable protection from 11B stations.
                pMac->lim.gLim11bParams.protectionEnabled = false;
                //Check if any other non-HT protection enabled.
                if(!pMac->lim.gLim11gParams.protectionEnabled)
                {
                    //Right now we are in HT OP Mixed mode.
                    //Change HT op mode appropriately.
                    limEnableHtOBSSProtection(pMac,  false, overlap, pBeaconParams,psessionEntry);            

                    //Change HT OP mode to 01 if any overlap protection enabled
                    if(pMac->lim.gLimOlbcParams.protectionEnabled ||
                        pMac->lim.gLimOverlap11gParams.protectionEnabled ||
                        pMac->lim.gLimOverlapHt20Params.protectionEnabled ||
                        pMac->lim.gLimOverlapNonGfParams.protectionEnabled)

                    {
                        pMac->lim.gHTOperMode = eSIR_HT_OP_MODE_OVERLAP_LEGACY;
                        limEnableHtRifsProtection(pMac, true, overlap, pBeaconParams,psessionEntry);
                    }
                    else if(pMac->lim.gLimHt20Params.protectionEnabled)
                    {
                        pMac->lim.gHTOperMode = eSIR_HT_OP_MODE_NO_LEGACY_20MHZ_HT;
                        limEnableHtRifsProtection(pMac, false, overlap, pBeaconParams,psessionEntry);
                    }
                    else
                    {
                        pMac->lim.gHTOperMode = eSIR_HT_OP_MODE_PURE;
                        limEnableHtRifsProtection(pMac, false, overlap, pBeaconParams,psessionEntry);
                    }
                }
            }
            if(!pMac->lim.gLimOlbcParams.protectionEnabled &&
                  !pMac->lim.gLim11bParams.protectionEnabled)
            {
                PELOG1(limLog(pMac, LOG1, FL("===> 11G Protection Disabled\n"));)
                pBeaconParams->llbCoexist = psessionEntry->llbCoexist = false;
                pBeaconParams->paramChangeBitmap |= PARAM_llBCOEXIST_CHANGED;
            }
        }
        //for station role
        else
        {
            PELOG1(limLog(pMac, LOG1, FL("===> 11G Protection Disabled\n"));)
            pBeaconParams->llbCoexist = psessionEntry->llbCoexist = false;
            pBeaconParams->paramChangeBitmap |= PARAM_llBCOEXIST_CHANGED;
        }
    }
    return eSIR_SUCCESS;
}
    
/** -------------------------------------------------------------
\fn limEnableHtProtectionFrom11g
\brief based on cofig enables\disables protection from 11g.
\param      tANI_U8 enable : 1=> enable protection, 0=> disable protection.
\param      tANI_U8 overlap: 1=> called from overlap context, 0 => called from assoc context.
\param      tpUpdateBeaconParams pBeaconParams
\return      None
  -------------------------------------------------------------*/
tSirRetStatus
limEnableHtProtectionFrom11g(tpAniSirGlobal pMac, tANI_U8 enable,
    tANI_U8 overlap, tpUpdateBeaconParams pBeaconParams,tpPESession psessionEntry)
{
    if(!psessionEntry->htCapabality)
        return eSIR_SUCCESS; // protection from 11g is only for HT stations.

    //overlapping protection configuration check.
    if(overlap)
    {
#ifdef WLAN_SOFTAP_FEATURE
        if((psessionEntry->limSystemRole == eLIM_AP_ROLE ) && (!psessionEntry->cfgProtection.overlapFromllg))
		{
			// protection disabled.
            PELOG3(limLog(pMac, LOG3, FL("overlap protection from 11g is disabled\n")););
            return eSIR_SUCCESS;
		}else if ((psessionEntry->limSystemRole == eLIM_BT_AMP_AP_ROLE) && (!pMac->lim.cfgProtection.overlapFromllg))
#else
        if(((psessionEntry->limSystemRole == eLIM_AP_ROLE ) ||(psessionEntry->limSystemRole == eLIM_BT_AMP_AP_ROLE)) && (!pMac->lim.cfgProtection.overlapFromllg))
#endif
        {
            // protection disabled.
            PELOG3(limLog(pMac, LOG3, FL("overlap protection from 11g is disabled\n")););
            return eSIR_SUCCESS;
        }
	}
    else
    {
        //normal protection config check
#ifdef WLAN_SOFTAP_FEATURE
       if((psessionEntry->limSystemRole == eLIM_AP_ROLE ) && 
		   !psessionEntry->cfgProtection.fromllg){
			// protection disabled.
            PELOG3(limLog(pMac, LOG3, FL("protection from 11g is disabled\n"));)
            return eSIR_SUCCESS;
		}else if(psessionEntry->limSystemRole != eLIM_AP_ROLE )
#endif
       {
	       if(!pMac->lim.cfgProtection.fromllg)
           {
                // protection disabled.
                PELOG3(limLog(pMac, LOG3, FL("protection from 11g is disabled\n"));)
                return eSIR_SUCCESS;
            }
        }
     }
    if (enable)
    {
        //If we are AP and HT capable, we need to set the HT OP mode
        //appropriately.

#ifdef WLAN_SOFTAP_FEATURE
        if(eLIM_AP_ROLE == psessionEntry->limSystemRole)
        {
            if(overlap)
            {
                psessionEntry->gLimOverlap11gParams.protectionEnabled = true;
                //11g exists in overlap BSS.
                //need not to change the operating mode to overlap_legacy
                //if higher or same protection operating mode is enabled right now.
                if((eSIR_HT_OP_MODE_OVERLAP_LEGACY != psessionEntry->htOperMode) &&
                    (eSIR_HT_OP_MODE_MIXED != psessionEntry->htOperMode))
                {
                    psessionEntry->htOperMode = eSIR_HT_OP_MODE_OVERLAP_LEGACY;
                }
                limEnableHtRifsProtection(pMac, true, overlap, pBeaconParams,psessionEntry);
                limEnableHtOBSSProtection(pMac,  true , overlap, pBeaconParams, psessionEntry);
            }
            else
            {
                //11g is associated to an AP operating in 11n mode.
                //Change the HT operating mode to 'mixed mode'.
                psessionEntry->gLim11gParams.protectionEnabled = true;
                if(eSIR_HT_OP_MODE_MIXED != psessionEntry->htOperMode)
                {
                    psessionEntry->htOperMode = eSIR_HT_OP_MODE_MIXED;
                    limEnableHtRifsProtection(pMac, true, overlap, pBeaconParams,psessionEntry);
                    limEnableHtOBSSProtection(pMac,  true , overlap, pBeaconParams,psessionEntry);        
                }
            }
        }else if(eLIM_BT_AMP_AP_ROLE == psessionEntry->limSystemRole)
#else
        if((eLIM_AP_ROLE == psessionEntry->limSystemRole)||(eLIM_BT_AMP_AP_ROLE == psessionEntry->limSystemRole))
#endif
        {
            if(overlap)
            {
                pMac->lim.gLimOverlap11gParams.protectionEnabled = true;
                //11g exists in overlap BSS.
                //need not to change the operating mode to overlap_legacy
                //if higher or same protection operating mode is enabled right now.
                if((eSIR_HT_OP_MODE_OVERLAP_LEGACY != pMac->lim.gHTOperMode) &&
                    (eSIR_HT_OP_MODE_MIXED != pMac->lim.gHTOperMode))
                {
                    pMac->lim.gHTOperMode = eSIR_HT_OP_MODE_OVERLAP_LEGACY;
                    limEnableHtRifsProtection(pMac, true, overlap, pBeaconParams,psessionEntry);
                }
            }
            else
            {
                //11g is associated to an AP operating in 11n mode.
                //Change the HT operating mode to 'mixed mode'.
                pMac->lim.gLim11gParams.protectionEnabled = true;
                if(eSIR_HT_OP_MODE_MIXED != pMac->lim.gHTOperMode)
                {
                    pMac->lim.gHTOperMode = eSIR_HT_OP_MODE_MIXED;
                    limEnableHtRifsProtection(pMac, true, overlap, pBeaconParams,psessionEntry);
                    limEnableHtOBSSProtection(pMac,  true , overlap, pBeaconParams,psessionEntry);        
                }
            }
        }

        //This part is common for staiton as well.
        if(false == psessionEntry->llgCoexist)
        {
            pBeaconParams->llgCoexist = psessionEntry->llgCoexist = true;
            pBeaconParams->paramChangeBitmap |= PARAM_llGCOEXIST_CHANGED;
        }
#ifdef WLAN_SOFTAP_FEATURE
        else if (true == psessionEntry->gLimOverlap11gParams.protectionEnabled)
        {
            // As operating mode changed after G station assoc some way to update beacon
            // This addresses the issue of mode not changing to - 11 in beacon when OBSS overlap is enabled
            //pMac->sch.schObject.fBeaconChanged = 1;
            pBeaconParams->paramChangeBitmap |= PARAM_llGCOEXIST_CHANGED;
        }
#endif
    }
    else if (true == psessionEntry->llgCoexist)
    {
        //for AP role.
        //we need to take care of HT OP mode change if needed.
        //We need to take care of Overlap cases.

#ifdef WLAN_SOFTAP_FEATURE
        if(eLIM_AP_ROLE == psessionEntry->limSystemRole)
        {
            if(overlap)
            {
                //Overlap Legacy protection disabled.
                if (psessionEntry->gLim11gParams.numSta == 0)
                psessionEntry->gLimOverlap11gParams.protectionEnabled = false;

                // no HT op mode change if any of the overlap protection enabled.
                if(!(psessionEntry->gLimOlbcParams.protectionEnabled ||
                    psessionEntry->gLimOverlapHt20Params.protectionEnabled ||
                    psessionEntry->gLimOverlapNonGfParams.protectionEnabled))
                {
                    //Check if there is a need to change HT OP mode.
                    if(eSIR_HT_OP_MODE_OVERLAP_LEGACY == psessionEntry->htOperMode)
                    {
                        limEnableHtRifsProtection(pMac, false, overlap, pBeaconParams,psessionEntry);
                        limEnableHtOBSSProtection(pMac,  false, overlap, pBeaconParams,psessionEntry);            

                        if(psessionEntry->gLimHt20Params.protectionEnabled){
                            //Commenting because of CR 258588 WFA cert
                            //psessionEntry->htOperMode = eSIR_HT_OP_MODE_NO_LEGACY_20MHZ_HT;
                            psessionEntry->htOperMode = eSIR_HT_OP_MODE_PURE;
                        }
                        else
                            psessionEntry->htOperMode = eSIR_HT_OP_MODE_PURE;
                    }
                }
            }
            else
            {
                //Disable protection from 11G stations.
                psessionEntry->gLim11gParams.protectionEnabled = false;
                //Check if any other non-HT protection enabled.
                if(!psessionEntry->gLim11bParams.protectionEnabled)
                {

                    //Right now we are in HT OP Mixed mode.
                    //Change HT op mode appropriately.
                    limEnableHtOBSSProtection(pMac,  false, overlap, pBeaconParams,psessionEntry);            

                    //Change HT OP mode to 01 if any overlap protection enabled
                    if(psessionEntry->gLimOlbcParams.protectionEnabled ||
                        psessionEntry->gLimOverlap11gParams.protectionEnabled ||
                        psessionEntry->gLimOverlapHt20Params.protectionEnabled ||
                        psessionEntry->gLimOverlapNonGfParams.protectionEnabled)

                    {
                        psessionEntry->htOperMode = eSIR_HT_OP_MODE_OVERLAP_LEGACY;
                        limEnableHtRifsProtection(pMac, true, overlap, pBeaconParams,psessionEntry);
                    }
                    else if(psessionEntry->gLimHt20Params.protectionEnabled)
                    {
                        //Commenting because of CR 258588 WFA cert
                        //psessionEntry->htOperMode = eSIR_HT_OP_MODE_NO_LEGACY_20MHZ_HT;
                        psessionEntry->htOperMode = eSIR_HT_OP_MODE_PURE;
                        limEnableHtRifsProtection(pMac, false, overlap, pBeaconParams,psessionEntry);
                    }
                    else
                    {
                        psessionEntry->htOperMode = eSIR_HT_OP_MODE_PURE;
                        limEnableHtRifsProtection(pMac, false, overlap, pBeaconParams,psessionEntry);
                    }
                }
            }
            if(!psessionEntry->gLimOverlap11gParams.protectionEnabled &&
                  !psessionEntry->gLim11gParams.protectionEnabled)
            {
                PELOG1(limLog(pMac, LOG1, FL("===> Protection from 11G Disabled\n"));)
                pBeaconParams->llgCoexist = psessionEntry->llgCoexist = false;
                pBeaconParams->paramChangeBitmap |= PARAM_llGCOEXIST_CHANGED;
            }
		}else if(eLIM_BT_AMP_AP_ROLE == psessionEntry->limSystemRole)
#else
        if((eLIM_AP_ROLE == psessionEntry->limSystemRole)||(eLIM_BT_AMP_AP_ROLE == psessionEntry->limSystemRole))
#endif
        {
            if(overlap)
            {
                //Overlap Legacy protection disabled.
                pMac->lim.gLimOverlap11gParams.protectionEnabled = false;

                // no HT op mode change if any of the overlap protection enabled.
                if(!(pMac->lim.gLimOlbcParams.protectionEnabled ||
                    pMac->lim.gLimOverlapHt20Params.protectionEnabled ||
                    pMac->lim.gLimOverlapNonGfParams.protectionEnabled))
                {
                    //Check if there is a need to change HT OP mode.
                    if(eSIR_HT_OP_MODE_OVERLAP_LEGACY == pMac->lim.gHTOperMode)
                    {
                        limEnableHtRifsProtection(pMac, false, overlap, pBeaconParams,psessionEntry);
                        limEnableHtOBSSProtection(pMac,  false, overlap, pBeaconParams,psessionEntry);            

                        if(pMac->lim.gLimHt20Params.protectionEnabled)
                            pMac->lim.gHTOperMode = eSIR_HT_OP_MODE_NO_LEGACY_20MHZ_HT;
                        else
                            pMac->lim.gHTOperMode = eSIR_HT_OP_MODE_PURE;
                    }
                }
            }
            else
            {
                //Disable protection from 11G stations.
                pMac->lim.gLim11gParams.protectionEnabled = false;
                //Check if any other non-HT protection enabled.
                if(!pMac->lim.gLim11bParams.protectionEnabled)
                {

                    //Right now we are in HT OP Mixed mode.
                    //Change HT op mode appropriately.
                    limEnableHtOBSSProtection(pMac,  false, overlap, pBeaconParams,psessionEntry);            

                    //Change HT OP mode to 01 if any overlap protection enabled
                    if(pMac->lim.gLimOlbcParams.protectionEnabled ||
                        pMac->lim.gLimOverlap11gParams.protectionEnabled ||
                        pMac->lim.gLimOverlapHt20Params.protectionEnabled ||
                        pMac->lim.gLimOverlapNonGfParams.protectionEnabled)

                    {
                        pMac->lim.gHTOperMode = eSIR_HT_OP_MODE_OVERLAP_LEGACY;
                        limEnableHtRifsProtection(pMac, true, overlap, pBeaconParams,psessionEntry);
                    }
                    else if(pMac->lim.gLimHt20Params.protectionEnabled)
                    {
                        pMac->lim.gHTOperMode = eSIR_HT_OP_MODE_NO_LEGACY_20MHZ_HT;
                        limEnableHtRifsProtection(pMac, false, overlap, pBeaconParams,psessionEntry);
                    }
                    else
                    {
                        pMac->lim.gHTOperMode = eSIR_HT_OP_MODE_PURE;
                        limEnableHtRifsProtection(pMac, false, overlap, pBeaconParams,psessionEntry);
                    }
                }
            }
            if(!pMac->lim.gLimOverlap11gParams.protectionEnabled &&
                  !pMac->lim.gLim11gParams.protectionEnabled)
            {
                PELOG1(limLog(pMac, LOG1, FL("===> Protection from 11G Disabled\n"));)
                pBeaconParams->llgCoexist = psessionEntry->llgCoexist = false;
                pBeaconParams->paramChangeBitmap |= PARAM_llGCOEXIST_CHANGED;
            }
        }
        //for station role
        else
        {
            PELOG1(limLog(pMac, LOG1, FL("===> Protection from 11G Disabled\n"));)
            pBeaconParams->llgCoexist = psessionEntry->llgCoexist = false;
            pBeaconParams->paramChangeBitmap |= PARAM_llGCOEXIST_CHANGED;
        }
    }
    return eSIR_SUCCESS;
}
//FIXME_PROTECTION : need to check for no APSD whenever we want to enable this protection.
//This check will be done at the caller.

/** -------------------------------------------------------------
\fn limEnableHtObssProtection
\brief based on cofig enables\disables obss protection.
\param      tANI_U8 enable : 1=> enable protection, 0=> disable protection.
\param      tANI_U8 overlap: 1=> called from overlap context, 0 => called from assoc context.
\param      tpUpdateBeaconParams pBeaconParams
\return      None
  -------------------------------------------------------------*/
tSirRetStatus
limEnableHtOBSSProtection(tpAniSirGlobal pMac, tANI_U8 enable,
    tANI_U8 overlap, tpUpdateBeaconParams pBeaconParams,tpPESession psessionEntry)
{


    if(!psessionEntry->htCapabality)
        return eSIR_SUCCESS; // this protection  is only for HT stations.

    //overlapping protection configuration check.
    if(overlap)
    {
        //overlapping protection configuration check.
    #if (defined(ANI_PRODUCT_TYPE_AP) || defined(ANI_PRODUCT_TYPE_AP_SDK))
        if((psessionEntry->limSystemRole == eLIM_AP_ROLE)||(psessionEntry->limSystemRole == eLIM_BT_AMP_AP_ROLE)) && !pMac->lim.cfgProtection.overlapOBSS) 
        { // ToDo Update this field
            // protection disabled.
            PELOG1(limLog(pMac, LOG1, FL("overlap protection from Obss is disabled\n"));)
            return eSIR_SUCCESS;
        }
    #endif
    } 
    else 
    {
        //normal protection config check
#ifdef WLAN_SOFTAP_FEATURE
        if((psessionEntry->limSystemRole == eLIM_AP_ROLE) && !psessionEntry->cfgProtection.obss) 
        { //ToDo Update this field
            // protection disabled.
            PELOG1(limLog(pMac, LOG1, FL("protection from Obss is disabled\n"));)
            return eSIR_SUCCESS;
        }else if(psessionEntry->limSystemRole != eLIM_AP_ROLE)
#endif
        {
		    if(!pMac->lim.cfgProtection.obss) 
            { //ToDo Update this field
                // protection disabled.
                PELOG1(limLog(pMac, LOG1, FL("protection from Obss is disabled\n"));)
                return eSIR_SUCCESS;
            }	
		}
    }


#ifdef WLAN_SOFTAP_FEATURE
    if (eLIM_AP_ROLE == psessionEntry->limSystemRole){
        if ((enable) && (false == psessionEntry->gHTObssMode) )
        {
            PELOG1(limLog(pMac, LOG1, FL("=>obss protection enabled\n"));)
            psessionEntry->gHTObssMode = true;
            pBeaconParams->paramChangeBitmap |= PARAM_OBSS_MODE_CHANGED; // UPDATE AN ENUM FOR OBSS MODE <todo>
        
         }
         else if (!enable && (true == psessionEntry->gHTObssMode)) 
         {
            PELOG1(limLog(pMac, LOG1, FL("===> obss Protection disabled\n"));)
            psessionEntry->gHTObssMode = false;
            pBeaconParams->paramChangeBitmap |= PARAM_OBSS_MODE_CHANGED;

         }
//CR-263021: OBSS bit is not switching back to 0 after disabling the overlapping legacy BSS
         if (!enable && !overlap)
         {
             psessionEntry->gLimOverlap11gParams.protectionEnabled = false;
         }
    } else
#endif
    {
        if ((enable) && (false == pMac->lim.gHTObssMode) )
        {
            PELOG1(limLog(pMac, LOG1, FL("=>obss protection enabled\n"));)
            pMac->lim.gHTObssMode = true;
            pBeaconParams->paramChangeBitmap |= PARAM_OBSS_MODE_CHANGED; // UPDATE AN ENUM FOR OBSS MODE <todo>

        }
        else if (!enable && (true == pMac->lim.gHTObssMode)) 
        {

            PELOG1(limLog(pMac, LOG1, FL("===> obss Protection disabled\n"));)
            pMac->lim.gHTObssMode = false;
            pBeaconParams->paramChangeBitmap |= PARAM_OBSS_MODE_CHANGED;

        }
	}
    return eSIR_SUCCESS;
}
/** -------------------------------------------------------------
\fn limEnableHT20Protection
\brief based on cofig enables\disables protection from Ht20.
\param      tANI_U8 enable : 1=> enable protection, 0=> disable protection.
\param      tANI_U8 overlap: 1=> called from overlap context, 0 => called from assoc context.
\param      tpUpdateBeaconParams pBeaconParams
\return      None
  -------------------------------------------------------------*/
tSirRetStatus
limEnableHT20Protection(tpAniSirGlobal pMac, tANI_U8 enable,
    tANI_U8 overlap, tpUpdateBeaconParams pBeaconParams,tpPESession psessionEntry)
{
    if(!psessionEntry->htCapabality)
        return eSIR_SUCCESS; // this protection  is only for HT stations.

        //overlapping protection configuration check.
        if(overlap)
        {
#if (defined(ANI_PRODUCT_TYPE_AP) || defined(ANI_PRODUCT_TYPE_AP_SDK))
        if(((psessionEntry->limSystemRole == eLIM_AP_ROLE )||(psessionEntry->limSystemRoleS == eLIM_BT_AMP_AP_ROLE ))&& !pMac->lim.cfgProtection.overlapHt20)
            {
                // protection disabled.
            PELOG3(limLog(pMac, LOG3, FL("overlap protection from HT 20 is disabled\n"));)
                return eSIR_SUCCESS;
            }
#endif
        }
        else
        {
            //normal protection config check
#ifdef WLAN_SOFTAP_FEATURE			
            if((psessionEntry->limSystemRole == eLIM_AP_ROLE ) &&
			   !psessionEntry->cfgProtection.ht20)
            {
                // protection disabled.
                PELOG3(limLog(pMac, LOG3, FL("protection from HT20 is disabled\n"));)
                return eSIR_SUCCESS;
            }else if(psessionEntry->limSystemRole != eLIM_AP_ROLE )
#endif		
            {
				if(!pMac->lim.cfgProtection.ht20)
                {
                    // protection disabled.
                    PELOG3(limLog(pMac, LOG3, FL("protection from HT20 is disabled\n"));)
                    return eSIR_SUCCESS;
                }
			}
        }

    if (enable)
    {
        //If we are AP and HT capable, we need to set the HT OP mode
        //appropriately.
		
#ifdef WLAN_SOFTAP_FEATURE
        if(eLIM_AP_ROLE == psessionEntry->limSystemRole){
			if(overlap)
            {
                psessionEntry->gLimOverlapHt20Params.protectionEnabled = true;
                if((eSIR_HT_OP_MODE_OVERLAP_LEGACY != psessionEntry->htOperMode) &&
                    (eSIR_HT_OP_MODE_MIXED != psessionEntry->htOperMode))
                {
                    psessionEntry->htOperMode = eSIR_HT_OP_MODE_OVERLAP_LEGACY;
                    limEnableHtRifsProtection(pMac, true, overlap, pBeaconParams,psessionEntry);
                }
            }
            else
            {
               psessionEntry->gLimHt20Params.protectionEnabled = true;
                if(eSIR_HT_OP_MODE_PURE == psessionEntry->htOperMode)
                {
                    //Commenting because of CR 258588 WFA cert
                    //psessionEntry->htOperMode = eSIR_HT_OP_MODE_NO_LEGACY_20MHZ_HT;
                    psessionEntry->htOperMode = eSIR_HT_OP_MODE_PURE;
                    limEnableHtRifsProtection(pMac, false, overlap, pBeaconParams,psessionEntry);
                    limEnableHtOBSSProtection(pMac,  false, overlap, pBeaconParams,psessionEntry);
                }
            }
		}else if(eLIM_BT_AMP_AP_ROLE == psessionEntry->limSystemRole)
#else
        if((eLIM_AP_ROLE == psessionEntry->limSystemRole)||(eLIM_BT_AMP_AP_ROLE == psessionEntry->limSystemRole))
#endif
        {
            if(overlap)
            {
                pMac->lim.gLimOverlapHt20Params.protectionEnabled = true;
                if((eSIR_HT_OP_MODE_OVERLAP_LEGACY != pMac->lim.gHTOperMode) &&
                    (eSIR_HT_OP_MODE_MIXED != pMac->lim.gHTOperMode))
                {
                    pMac->lim.gHTOperMode = eSIR_HT_OP_MODE_OVERLAP_LEGACY;
                    limEnableHtRifsProtection(pMac, true, overlap, pBeaconParams,psessionEntry);
                }
            }
            else
            {
                pMac->lim.gLimHt20Params.protectionEnabled = true;
                if(eSIR_HT_OP_MODE_PURE == pMac->lim.gHTOperMode)
                {
                    pMac->lim.gHTOperMode = eSIR_HT_OP_MODE_NO_LEGACY_20MHZ_HT;
                    limEnableHtRifsProtection(pMac, false, overlap, pBeaconParams,psessionEntry);
                    limEnableHtOBSSProtection(pMac,  false, overlap, pBeaconParams,psessionEntry);
                }
            }
        }

        //This part is common for staiton as well.
        if(false == psessionEntry->ht20Coexist)
        {
            PELOG1(limLog(pMac, LOG1, FL("=> Prtection from HT20 Enabled\n"));)
            pBeaconParams->ht20MhzCoexist = psessionEntry->ht20Coexist = true;
            pBeaconParams->paramChangeBitmap |= PARAM_HT20MHZCOEXIST_CHANGED;
        }
    }
    else if (true == psessionEntry->ht20Coexist)
    {
        //for AP role.
        //we need to take care of HT OP mode change if needed.
        //We need to take care of Overlap cases.
#ifdef WLAN_SOFTAP_FEATURE
        if(eLIM_AP_ROLE == psessionEntry->limSystemRole){
            if(overlap)
            {
                //Overlap Legacy protection disabled.
                psessionEntry->gLimOverlapHt20Params.protectionEnabled = false;

                // no HT op mode change if any of the overlap protection enabled.
                if(!(psessionEntry->gLimOlbcParams.protectionEnabled ||
                    psessionEntry->gLimOverlap11gParams.protectionEnabled ||
                    psessionEntry->gLimOverlapHt20Params.protectionEnabled ||
                    psessionEntry->gLimOverlapNonGfParams.protectionEnabled))
                {

                    //Check if there is a need to change HT OP mode.
                    if(eSIR_HT_OP_MODE_OVERLAP_LEGACY == psessionEntry->htOperMode)
                    {
                        if(pMac->lim.gLimHt20Params.protectionEnabled)
                        {
                            //Commented beacuse of CR 258588 for WFA Cert
                            //psessionEntry->htOperMode = eSIR_HT_OP_MODE_NO_LEGACY_20MHZ_HT;
                            psessionEntry->htOperMode = eSIR_HT_OP_MODE_PURE;
                            limEnableHtRifsProtection(pMac, false, overlap, pBeaconParams,psessionEntry);
                            limEnableHtOBSSProtection(pMac,  false, overlap, pBeaconParams,psessionEntry);            
                        }
                        else
                        {
                            psessionEntry->htOperMode = eSIR_HT_OP_MODE_PURE;
                        }
                    }
                }
            }
            else
            {
                //Disable protection from 11G stations.
                psessionEntry->gLimHt20Params.protectionEnabled = false;

                //Change HT op mode appropriately.
                if(eSIR_HT_OP_MODE_NO_LEGACY_20MHZ_HT == psessionEntry->htOperMode)
                {
                    psessionEntry->htOperMode = eSIR_HT_OP_MODE_PURE;
                    limEnableHtRifsProtection(pMac, false, overlap, pBeaconParams,psessionEntry);
                    limEnableHtOBSSProtection(pMac,  false, overlap, pBeaconParams,psessionEntry);        
                }
            }
            PELOG1(limLog(pMac, LOG1, FL("===> Protection from HT 20 Disabled\n"));)
            pBeaconParams->ht20MhzCoexist = psessionEntry->ht20Coexist = false;
            pBeaconParams->paramChangeBitmap |= PARAM_HT20MHZCOEXIST_CHANGED;			
		}else if(eLIM_BT_AMP_AP_ROLE == psessionEntry->limSystemRole)
#else
        if((eLIM_AP_ROLE == psessionEntry->limSystemRole)||(eLIM_BT_AMP_AP_ROLE == psessionEntry->limSystemRole))
#endif
        {
            if(overlap)
            {
                //Overlap Legacy protection disabled.
                pMac->lim.gLimOverlapHt20Params.protectionEnabled = false;

                // no HT op mode change if any of the overlap protection enabled.
                if(!(pMac->lim.gLimOlbcParams.protectionEnabled ||
                    pMac->lim.gLimOverlap11gParams.protectionEnabled ||
                    pMac->lim.gLimOverlapHt20Params.protectionEnabled ||
                    pMac->lim.gLimOverlapNonGfParams.protectionEnabled))
                {

                    //Check if there is a need to change HT OP mode.
                    if(eSIR_HT_OP_MODE_OVERLAP_LEGACY == pMac->lim.gHTOperMode)
                    {
                        if(pMac->lim.gLimHt20Params.protectionEnabled)
                        {
                            pMac->lim.gHTOperMode = eSIR_HT_OP_MODE_NO_LEGACY_20MHZ_HT;
                            limEnableHtRifsProtection(pMac, false, overlap, pBeaconParams,psessionEntry);
                            limEnableHtOBSSProtection(pMac,  false, overlap, pBeaconParams,psessionEntry);            
                        }
                        else
                        {
                            pMac->lim.gHTOperMode = eSIR_HT_OP_MODE_PURE;
                        }
                    }
                }
            }
            else
            {
                //Disable protection from 11G stations.
                pMac->lim.gLimHt20Params.protectionEnabled = false;

                //Change HT op mode appropriately.
                if(eSIR_HT_OP_MODE_NO_LEGACY_20MHZ_HT == pMac->lim.gHTOperMode)
                {
                    pMac->lim.gHTOperMode = eSIR_HT_OP_MODE_PURE;
                    limEnableHtRifsProtection(pMac, false, overlap, pBeaconParams,psessionEntry);
                    limEnableHtOBSSProtection(pMac,  false, overlap, pBeaconParams,psessionEntry);        
                }
            }
            PELOG1(limLog(pMac, LOG1, FL("===> Protection from HT 20 Disabled\n"));)
            pBeaconParams->ht20MhzCoexist = psessionEntry->ht20Coexist = false;
            pBeaconParams->paramChangeBitmap |= PARAM_HT20MHZCOEXIST_CHANGED;
        }
        //for station role
        else
        {
            PELOG1(limLog(pMac, LOG1, FL("===> Protection from HT20 Disabled\n"));)
            pBeaconParams->ht20MhzCoexist = psessionEntry->ht20Coexist = false;
            pBeaconParams->paramChangeBitmap |= PARAM_HT20MHZCOEXIST_CHANGED;
        }
    }

    return eSIR_SUCCESS;
}

/** -------------------------------------------------------------
\fn limEnableHTNonGfProtection
\brief based on cofig enables\disables protection from NonGf.
\param      tANI_U8 enable : 1=> enable protection, 0=> disable protection.
\param      tANI_U8 overlap: 1=> called from overlap context, 0 => called from assoc context.
\param      tpUpdateBeaconParams pBeaconParams
\return      None
  -------------------------------------------------------------*/
tSirRetStatus
limEnableHTNonGfProtection(tpAniSirGlobal pMac, tANI_U8 enable,
    tANI_U8 overlap, tpUpdateBeaconParams pBeaconParams,tpPESession psessionEntry)
{
    if(!psessionEntry->htCapabality)
        return eSIR_SUCCESS; // this protection  is only for HT stations.

        //overlapping protection configuration check.
        if(overlap)
        {
#if (defined(ANI_PRODUCT_TYPE_AP) || defined(ANI_PRODUCT_TYPE_AP_SDK))
        if(((psessionEntry->limSystemRole == eLIM_AP_ROLE)||(psessionEntry->limSystemRole == eLIM_BT_AMP_AP_ROLE)) && !pMac->lim.cfgProtection.overlapNonGf)
            {
                // protection disabled.
            PELOG3(limLog(pMac, LOG3, FL("overlap protection from NonGf is disabled\n"));)
                return eSIR_SUCCESS;
            }
#endif
        }
        else
        {
#ifdef WLAN_SOFTAP_FEATURE
            //normal protection config check
            if((psessionEntry->limSystemRole == eLIM_AP_ROLE ) &&
			    !psessionEntry->cfgProtection.nonGf)
            {
                // protection disabled.
                PELOG3(limLog(pMac, LOG3, FL("protection from NonGf is disabled\n"));)
                return eSIR_SUCCESS;
            }else if(psessionEntry->limSystemRole != eLIM_AP_ROLE)
#endif
            {
				//normal protection config check
                if(!pMac->lim.cfgProtection.nonGf)
                {
                    // protection disabled.
                    PELOG3(limLog(pMac, LOG3, FL("protection from NonGf is disabled\n"));)
                    return eSIR_SUCCESS;
                 }
			}
        }
#ifdef WLAN_SOFTAP_FEATURE
    if(psessionEntry->limSystemRole == eLIM_AP_ROLE){
		if ((enable) && (false == psessionEntry->gHTNonGFDevicesPresent))
        {
            PELOG1(limLog(pMac, LOG1, FL(" => Prtection from non GF Enabled\n"));)
            pBeaconParams->llnNonGFCoexist = psessionEntry->gHTNonGFDevicesPresent = true;
            pBeaconParams->paramChangeBitmap |= PARAM_NON_GF_DEVICES_PRESENT_CHANGED;
        }
        else if (!enable && (true == psessionEntry->gHTNonGFDevicesPresent))
        {
            PELOG1(limLog(pMac, LOG1, FL("===> Protection from Non GF Disabled\n"));)
            pBeaconParams->llnNonGFCoexist = psessionEntry->gHTNonGFDevicesPresent = false;
            pBeaconParams->paramChangeBitmap |= PARAM_NON_GF_DEVICES_PRESENT_CHANGED;
        }
	}else
#endif
    {
        if ((enable) && (false == pMac->lim.gHTNonGFDevicesPresent))
        {
            PELOG1(limLog(pMac, LOG1, FL(" => Prtection from non GF Enabled\n"));)
            pBeaconParams->llnNonGFCoexist = pMac->lim.gHTNonGFDevicesPresent = true;
            pBeaconParams->paramChangeBitmap |= PARAM_NON_GF_DEVICES_PRESENT_CHANGED;
        }
        else if (!enable && (true == pMac->lim.gHTNonGFDevicesPresent))
        {
            PELOG1(limLog(pMac, LOG1, FL("===> Protection from Non GF Disabled\n"));)
            pBeaconParams->llnNonGFCoexist = pMac->lim.gHTNonGFDevicesPresent = false;
            pBeaconParams->paramChangeBitmap |= PARAM_NON_GF_DEVICES_PRESENT_CHANGED;
        }
	}

    return eSIR_SUCCESS;
}

/** -------------------------------------------------------------
\fn limEnableHTLsigTxopProtection
\brief based on cofig enables\disables LsigTxop protection.
\param      tANI_U8 enable : 1=> enable protection, 0=> disable protection.
\param      tANI_U8 overlap: 1=> called from overlap context, 0 => called from assoc context.
\param      tpUpdateBeaconParams pBeaconParams
\return      None
  -------------------------------------------------------------*/
tSirRetStatus
limEnableHTLsigTxopProtection(tpAniSirGlobal pMac, tANI_U8 enable,
    tANI_U8 overlap, tpUpdateBeaconParams pBeaconParams,tpPESession psessionEntry)
{
    if(!psessionEntry->htCapabality)
        return eSIR_SUCCESS; // this protection  is only for HT stations.

        //overlapping protection configuration check.
        if(overlap)
        {
#if (defined(ANI_PRODUCT_TYPE_AP) || defined(ANI_PRODUCT_TYPE_AP_SDK))
        if(((psessionEntry->limSystemRole == eLIM_AP_ROLE)||(psessionEntry->limSystemRole == eLIM_BT_AMP_AP_ROLE)) && !pMac->lim.cfgProtection.overlapLsigTxop)
            {
                // protection disabled.
                PELOG3(limLog(pMac, LOG3, FL(" overlap protection from LsigTxop not supported is disabled\n"));)
                return eSIR_SUCCESS;
            }
#endif
        }
        else
        {
#ifdef WLAN_SOFTAP_FEATURE
            //normal protection config check
            if((psessionEntry->limSystemRole == eLIM_AP_ROLE ) &&
			   !psessionEntry->cfgProtection.lsigTxop)
            {
                // protection disabled.
                PELOG3(limLog(pMac, LOG3, FL(" protection from LsigTxop not supported is disabled\n"));)
                return eSIR_SUCCESS;
            }else if(psessionEntry->limSystemRole != eLIM_AP_ROLE)
#endif
            {
                //normal protection config check
                if(!pMac->lim.cfgProtection.lsigTxop)
                {
                    // protection disabled.
                    PELOG3(limLog(pMac, LOG3, FL(" protection from LsigTxop not supported is disabled\n"));)
                    return eSIR_SUCCESS;
                }
			}
        }


#ifdef WLAN_SOFTAP_FEATURE
    if(psessionEntry->limSystemRole == eLIM_AP_ROLE){
	    if ((enable) && (false == psessionEntry->fLsigTXOPProtectionFullSupport))
        {
            PELOG1(limLog(pMac, LOG1, FL(" => Prtection from LsigTxop Enabled\n"));)
            pBeaconParams->fLsigTXOPProtectionFullSupport = psessionEntry->fLsigTXOPProtectionFullSupport = true;
            pBeaconParams->paramChangeBitmap |= PARAM_LSIG_TXOP_FULL_SUPPORT_CHANGED;
        }
        else if (!enable && (true == psessionEntry->fLsigTXOPProtectionFullSupport))
        {
            PELOG1(limLog(pMac, LOG1, FL("===> Protection from LsigTxop Disabled\n"));)
            pBeaconParams->fLsigTXOPProtectionFullSupport= psessionEntry->fLsigTXOPProtectionFullSupport = false;
            pBeaconParams->paramChangeBitmap |= PARAM_LSIG_TXOP_FULL_SUPPORT_CHANGED;
        }
	}else
#endif
    {
	if ((enable) && (false == pMac->lim.gHTLSigTXOPFullSupport))
    {
        PELOG1(limLog(pMac, LOG1, FL(" => Prtection from LsigTxop Enabled\n"));)
        pBeaconParams->fLsigTXOPProtectionFullSupport = pMac->lim.gHTLSigTXOPFullSupport = true;
        pBeaconParams->paramChangeBitmap |= PARAM_LSIG_TXOP_FULL_SUPPORT_CHANGED;
    }
    else if (!enable && (true == pMac->lim.gHTLSigTXOPFullSupport))
    {
        PELOG1(limLog(pMac, LOG1, FL("===> Protection from LsigTxop Disabled\n"));)
        pBeaconParams->fLsigTXOPProtectionFullSupport= pMac->lim.gHTLSigTXOPFullSupport = false;
        pBeaconParams->paramChangeBitmap |= PARAM_LSIG_TXOP_FULL_SUPPORT_CHANGED;
    }
	}
    return eSIR_SUCCESS;
}
//FIXME_PROTECTION : need to check for no APSD whenever we want to enable this protection.
//This check will be done at the caller.
/** -------------------------------------------------------------
\fn limEnableHtRifsProtection
\brief based on cofig enables\disables Rifs protection.
\param      tANI_U8 enable : 1=> enable protection, 0=> disable protection.
\param      tANI_U8 overlap: 1=> called from overlap context, 0 => called from assoc context.
\param      tpUpdateBeaconParams pBeaconParams
\return      None
  -------------------------------------------------------------*/
tSirRetStatus
limEnableHtRifsProtection(tpAniSirGlobal pMac, tANI_U8 enable,
    tANI_U8 overlap, tpUpdateBeaconParams pBeaconParams,tpPESession psessionEntry)
{
    if(!psessionEntry->htCapabality)
        return eSIR_SUCCESS; // this protection  is only for HT stations.

        //overlapping protection configuration check.
        if(overlap)
        {
#if (defined(ANI_PRODUCT_TYPE_AP) || defined(ANI_PRODUCT_TYPE_AP_SDK))
            if(((psessionEntry->limSystemRole == eLIM_AP_ROLE) ||(psessionEntry == eLIM_BT_AMP_AP_ROLE))&& !pMac->lim.cfgProtection.overlapRifs)
            {
                // protection disabled.
                PELOG3(limLog(pMac, LOG3, FL(" overlap protection from Rifs is disabled\n"));)
                return eSIR_SUCCESS;
            }
#endif
        }
        else
        {
#ifdef WLAN_SOFTAP_FEATURE
             //normal protection config check
            if((psessionEntry->limSystemRole == eLIM_AP_ROLE) &&
			   !psessionEntry->cfgProtection.rifs)
            {
                // protection disabled.
                PELOG3(limLog(pMac, LOG3, FL(" protection from Rifs is disabled\n"));)
                return eSIR_SUCCESS;
            }else if(psessionEntry->limSystemRole != eLIM_AP_ROLE )
#endif
            {
            //normal protection config check
            if(!pMac->lim.cfgProtection.rifs)
            {
                // protection disabled.
                PELOG3(limLog(pMac, LOG3, FL(" protection from Rifs is disabled\n"));)
                return eSIR_SUCCESS;
            }
			}
        }

#ifdef WLAN_SOFTAP_FEATURE
    if(psessionEntry->limSystemRole == eLIM_AP_ROLE){
	    // Disabling the RIFS Protection means Enable the RIFS mode of operation in the BSS
        if ((!enable) && (false == psessionEntry->fRIFSMode))
        {
            PELOG1(limLog(pMac, LOG1, FL(" => Rifs protection Disabled\n"));)
            pBeaconParams->fRIFSMode = psessionEntry->fRIFSMode = true;
            pBeaconParams->paramChangeBitmap |= PARAM_RIFS_MODE_CHANGED;
        }
        // Enabling the RIFS Protection means Disable the RIFS mode of operation in the BSS
        else if (enable && (true == psessionEntry->fRIFSMode))
        {
            PELOG1(limLog(pMac, LOG1, FL("===> Rifs Protection Enabled\n"));)
            pBeaconParams->fRIFSMode = psessionEntry->fRIFSMode = false;
            pBeaconParams->paramChangeBitmap |= PARAM_RIFS_MODE_CHANGED;
        }	
	}else
#endif
    {
	// Disabling the RIFS Protection means Enable the RIFS mode of operation in the BSS
    if ((!enable) && (false == pMac->lim.gHTRifsMode))
    {
        PELOG1(limLog(pMac, LOG1, FL(" => Rifs protection Disabled\n"));)
        pBeaconParams->fRIFSMode = pMac->lim.gHTRifsMode = true;
        pBeaconParams->paramChangeBitmap |= PARAM_RIFS_MODE_CHANGED;
    }
    // Enabling the RIFS Protection means Disable the RIFS mode of operation in the BSS
    else if (enable && (true == pMac->lim.gHTRifsMode))
    {
        PELOG1(limLog(pMac, LOG1, FL("===> Rifs Protection Enabled\n"));)
        pBeaconParams->fRIFSMode = pMac->lim.gHTRifsMode = false;
        pBeaconParams->paramChangeBitmap |= PARAM_RIFS_MODE_CHANGED;
    }	
	}
    return eSIR_SUCCESS;
}

// ---------------------------------------------------------------------
/**
 * limEnableShortPreamble
 *
 * FUNCTION:
 * Enable/Disable short preamble
 *
 * LOGIC:
 *
 * ASSUMPTIONS:
 *
 * NOTE:
 *
 * @param enable        Flag to enable/disable short preamble
 * @return None
 */

#ifdef WLAN_SOFTAP_FEATURE
tSirRetStatus
limEnableShortPreamble(tpAniSirGlobal pMac, tANI_U8 enable, tpUpdateBeaconParams pBeaconParams, tpPESession psessionEntry)
#else
tSirRetStatus
limEnableShortPreamble(tpAniSirGlobal pMac, tANI_U8 enable, tpUpdateBeaconParams pBeaconParams)
#endif
{
    tANI_U32 val;

    if (wlan_cfgGetInt(pMac, WNI_CFG_SHORT_PREAMBLE, &val) != eSIR_SUCCESS)
    {
        /* Could not get short preamble enabled flag from CFG. Log error. */
        limLog(pMac, LOGP, FL("could not retrieve short preamble flag\n"));
        return eSIR_FAILURE;
    }

    if (!val)  
        return eSIR_SUCCESS;

    if (wlan_cfgGetInt(pMac, WNI_CFG_11G_SHORT_PREAMBLE_ENABLED, &val) != eSIR_SUCCESS)
    {
        limLog(pMac, LOGP, FL("could not retrieve 11G short preamble switching  enabled flag\n"));
        return eSIR_FAILURE;
    }

    if (!val)   // 11G short preamble switching is disabled.
        return eSIR_SUCCESS;

#ifdef WLAN_SOFTAP_FEATURE
    if ( psessionEntry->limSystemRole == eLIM_AP_ROLE )
    {
        if (enable && (psessionEntry->fShortPreamble == 0))
        {
            PELOG1(limLog(pMac, LOG1, FL("===> Short Preamble Enabled\n"));)
            psessionEntry->fShortPreamble = true;
            pBeaconParams->fShortPreamble = (tANI_U8) psessionEntry->fShortPreamble;
            pBeaconParams->paramChangeBitmap |= PARAM_SHORT_PREAMBLE_CHANGED;
        }
        else if (!enable && (psessionEntry->fShortPreamble == 1))
        {
            PELOG1(limLog(pMac, LOG1, FL("===> Short Preamble Disabled\n"));)
            psessionEntry->fShortPreamble = false;
            pBeaconParams->fShortPreamble = (tANI_U8) psessionEntry->fShortPreamble;
            pBeaconParams->paramChangeBitmap |= PARAM_SHORT_PREAMBLE_CHANGED;
        }
    }
    else
#endif
    {

        if (enable && (pMac->lim.gLimShortPreamble == 0))
        {
            PELOG1(limLog(pMac, LOG1, FL("===> Short Preamble Enabled\n"));)
            pMac->lim.gLimShortPreamble = true;
            pBeaconParams->fShortPreamble = (tANI_U8) pMac->lim.gLimShortPreamble;
            pBeaconParams->paramChangeBitmap |= PARAM_SHORT_PREAMBLE_CHANGED;
        }
        else if (!enable && (pMac->lim.gLimShortPreamble == 1))
        {
            PELOG1(limLog(pMac, LOG1, FL("===> Short Preamble Disabled\n"));)
            pMac->lim.gLimShortPreamble = false;
            pBeaconParams->fShortPreamble = (tANI_U8) pMac->lim.gLimShortPreamble;
            pBeaconParams->paramChangeBitmap |= PARAM_SHORT_PREAMBLE_CHANGED;
        }
    }

    return eSIR_SUCCESS;
}


/**
 * limTxComplete
 *
 * Function:
 * This is LIM's very own "TX MGMT frame complete" completion routine.
 *
 * Logic:
 * LIM wants to send a MGMT frame (broadcast or unicast)
 * LIM allocates memory using palPktAlloc( ..., **pData, **pPacket )
 * LIM transmits the MGMT frame using the API:
 *  halTxFrame( ... pPacket, ..., (void *) limTxComplete, pData )
 * HDD, via halTxFrame/DXE, "transfers" the packet over to BMU
 * HDD, if it determines that a TX completion routine (in this case
 * limTxComplete) has been provided, will invoke this callback
 * LIM will try to free the TX MGMT packet that was earlier allocated, in order
 * to send this MGMT frame, using the PAL API palPktFree( ... pData, pPacket )
 *
 * Assumptions:
 * Presently, this is ONLY being used for MGMT frames/packets
 * TODO:
 * Would it do good for LIM to have some sort of "signature" validation to
 * ensure that the pData argument passed in was a buffer that was actually
 * allocated by LIM and/or is not corrupted?
 *
 * Note: FIXME and TODO
 * Looks like palPktFree() is interested in pPacket. But, when this completion
 * routine is called, only pData is made available to LIM!!
 *
 * @param void A pointer to pData. Shouldn't it be pPacket?!
 *
 * @return none
 */
void limTxComplete( tHalHandle hHal, void *pData )
{
  tpAniSirGlobal pMac;
  pMac = (tpAniSirGlobal)hHal;

#ifdef FIXME_PRIMA
  /* the trace logic needs to be fixed for Prima.  Refer to CR 306075 */
#ifdef TRACE_RECORD
	{
	    tpSirMacMgmtHdr mHdr;
	    v_U8_t         *pRxBd;
	    vos_pkt_t      *pVosPkt;
	    VOS_STATUS      vosStatus;



	    pVosPkt = (vos_pkt_t *)pData;
	    vosStatus = vos_pkt_peek_data( pVosPkt, 0, (v_PVOID_t *)&pRxBd, WLANHAL_RX_BD_HEADER_SIZE);

	    if(VOS_IS_STATUS_SUCCESS(vosStatus))
	    {
	        mHdr = WDA_GET_RX_MAC_HEADER(pRxBd);
	        MTRACE(macTrace(pMac, TRACE_CODE_TX_COMPLETE, 0, mHdr->fc.subType);)

	    }   
	}
#endif
#endif

  palPktFree( pMac->hHdd,
              HAL_TXRX_FRM_802_11_MGMT,
              (void *) NULL,           // this is ignored and will likely be removed from this API
              (void *) pData );        // lim passed in pPacket in the pData pointer that is given in this completion routine
}

/**
 * \brief This function updates lim global structure, if CB parameters in the BSS
 *  have changed, and sends an indication to HAL also with the
 * updated HT Parameters.
 * This function does not detect the change in the primary channel, that is done as part
 * of channel Swtich IE processing.
 * If STA is configured with '20Mhz only' mode, then this function does not do anything
 * This function changes the CB mode, only if the self capability is set to '20 as well as 40Mhz'
 *
 *
 * \param pMac Pointer to global MAC structure
 *
 * \param pRcvdHTInfo Pointer to HT Info IE obtained from a  Beacon or
 * Probe Response
 *
 * \param bssIdx BSS Index of the Bss to which Station is associated.
 *
 *
 */

void limUpdateStaRunTimeHTSwitchChnlParams( tpAniSirGlobal   pMac,
                                  tDot11fIEHTInfo *pHTInfo,
                                  tANI_U8          bssIdx,
                                  tpPESession      psessionEntry)
{
    tSirMacHTSecondaryChannelOffset secondaryChnlOffset = eHT_SECONDARY_CHANNEL_OFFSET_NONE;
#if !defined WLAN_FEATURE_VOWIFI  
    tANI_U32 localPwrConstraint;
#endif
    
   //If self capability is set to '20Mhz only', then do not change the CB mode.
#ifdef WLAN_SOFTAP_FEATURE
   if( !limGetHTCapability( pMac, eHT_SUPPORTED_CHANNEL_WIDTH_SET, psessionEntry ))
#else
   if( !limGetHTCapability( pMac, eHT_SUPPORTED_CHANNEL_WIDTH_SET ))
#endif
        return;

#if !defined WLAN_FEATURE_VOWIFI  
    if(wlan_cfgGetInt(pMac, WNI_CFG_LOCAL_POWER_CONSTRAINT, &localPwrConstraint) != eSIR_SUCCESS) {
        limLog( pMac, LOGP, FL( "Unable to get Local Power Constraint from cfg\n" ));
        return;
    }
#endif

    if ( pMac->lim.gHTSecondaryChannelOffset != ( tANI_U8 ) pHTInfo->secondaryChannelOffset ||
         pMac->lim.gHTRecommendedTxWidthSet  != ( tANI_U8 ) pHTInfo->recommendedTxWidthSet )
    {
        pMac->lim.gHTSecondaryChannelOffset = ( tSirMacHTSecondaryChannelOffset ) pHTInfo->secondaryChannelOffset;
        pMac->lim.gHTRecommendedTxWidthSet  = ( tANI_U8 ) pHTInfo->recommendedTxWidthSet;
        //Also update the Proprietary(Titan) CB mode settings, as this setting is used during 
        //background scanning to set the original channel and CB mode as part of finish scan.
        setupCBState( pMac,  limGetAniCBState(pMac->lim.gHTSecondaryChannelOffset));

        // If the Channel Width is 20Mhz, set the channel offset to
        // NONE.  If the Channel Width is 40Mhz, set the channel offset
        // to what ever is present in beacon.
        if ( eHT_CHANNEL_WIDTH_40MHZ == pMac->lim.gHTRecommendedTxWidthSet )
            secondaryChnlOffset = (tSirMacHTSecondaryChannelOffset)pHTInfo->secondaryChannelOffset;

        // Notify HAL
        limLog( pMac, LOGW,  FL( "Channel Information in HT IE change"
                                 "d; sending notification to HAL.\n" ) );
        limLog( pMac, LOGW,  FL( "Primary Channel: %d, Secondary Chan"
                                 "nel Offset: %d, Channel Width: %d\n" ),
                pHTInfo->primaryChannel, secondaryChnlOffset,
                pMac->lim.gHTRecommendedTxWidthSet );

#if defined WLAN_FEATURE_VOWIFI  
		limSendSwitchChnlParams( pMac, ( tANI_U8 ) pHTInfo->primaryChannel,
								 secondaryChnlOffset, psessionEntry->maxTxPower, psessionEntry->peSessionId);
#else
		limSendSwitchChnlParams( pMac, ( tANI_U8 ) pHTInfo->primaryChannel,
								 secondaryChnlOffset, (tPowerdBm)localPwrConstraint, psessionEntry->peSessionId);
#endif

        //In case of IBSS, if STA should update HT Info IE in its beacons.
       if (eLIM_STA_IN_IBSS_ROLE == psessionEntry->limSystemRole)
        {
            schSetFixedBeaconFields(pMac,psessionEntry);
        }

    }
} // End limUpdateStaRunTimeHTParams.

/**
 * \brief This function updates the lim global structure, if any of the
 * HT Capabilities have changed.
 *
 *
 * \param pMac Pointer to Global MAC structure
 *
 * \param pHTCapability Pointer to HT Capability Information Element
 * obtained from a Beacon or Probe Response
 *
 *
 *
 */

void limUpdateStaRunTimeHTCapability( tpAniSirGlobal   pMac,
                                      tDot11fIEHTCaps *pHTCaps )
{

    if ( pMac->lim.gHTLsigTXOPProtection != ( tANI_U8 ) pHTCaps->lsigTXOPProtection )
    {
        pMac->lim.gHTLsigTXOPProtection = ( tANI_U8 ) pHTCaps->lsigTXOPProtection;
       // Send change notification to HAL
    }

    if ( pMac->lim.gHTAMpduDensity != ( tANI_U8 ) pHTCaps->mpduDensity )
    {
       pMac->lim.gHTAMpduDensity = ( tANI_U8 ) pHTCaps->mpduDensity;
       // Send change notification to HAL
    }

    if ( pMac->lim.gHTMaxRxAMpduFactor != ( tANI_U8 ) pHTCaps->maxRxAMPDUFactor )
    {
       pMac->lim.gHTMaxRxAMpduFactor = ( tANI_U8 ) pHTCaps->maxRxAMPDUFactor;
       // Send change notification to HAL
    }


} // End limUpdateStaRunTimeHTCapability.

/**
 * \brief This function updates lim global structure, if any of the HT
 * Info Parameters have changed.
 *
 *
 * \param pMac Pointer to the global MAC structure
 *
 * \param pHTInfo Pointer to the HT Info IE obtained from a Beacon or
 * Probe Response
 *
 *
 */

void limUpdateStaRunTimeHTInfo( tpAniSirGlobal  pMac,
                                tDot11fIEHTInfo *pHTInfo )
{
    if ( pMac->lim.gHTSecondaryChannelOffset != ( tANI_U8)pHTInfo->secondaryChannelOffset)
    {
        pMac->lim.gHTSecondaryChannelOffset = ( tSirMacHTSecondaryChannelOffset )pHTInfo->secondaryChannelOffset;
        // Send change notification to HAL
    }

    if ( pMac->lim.gHTRecommendedTxWidthSet != ( tANI_U8 )pHTInfo->recommendedTxWidthSet )
    {
        pMac->lim.gHTRecommendedTxWidthSet = ( tANI_U8 )pHTInfo->recommendedTxWidthSet;
        // Send change notification to HAL
    }

    if ( pMac->lim.gHTRifsMode != ( tANI_U8 )pHTInfo->rifsMode )
    {
        pMac->lim.gHTRifsMode = ( tANI_U8 )pHTInfo->rifsMode;
        // Send change notification to HAL
    }

    if ( pMac->lim.gHTServiceIntervalGranularity != ( tANI_U8 )pHTInfo->serviceIntervalGranularity )
    {
        pMac->lim.gHTServiceIntervalGranularity = ( tANI_U8 )pHTInfo->serviceIntervalGranularity;
        // Send change notification to HAL
    }

    if ( pMac->lim.gHTOperMode != ( tSirMacHTOperatingMode )pHTInfo->opMode )
    {
        pMac->lim.gHTOperMode = ( tSirMacHTOperatingMode )pHTInfo->opMode;
        // Send change notification to HAL
    }

    if ( pMac->lim.gHTNonGFDevicesPresent != pHTInfo->nonGFDevicesPresent )
    {
        pMac->lim.gHTNonGFDevicesPresent = ( tANI_U8 )pHTInfo->nonGFDevicesPresent;
    }

    if ( pMac->lim.gHTSTBCBasicMCS != ( tANI_U8 )pHTInfo->basicSTBCMCS )
    {
        pMac->lim.gHTSTBCBasicMCS = ( tANI_U8 )pHTInfo->basicSTBCMCS;
        // Send change notification to HAL
    }

    if ( pMac->lim.gHTDualCTSProtection != ( tANI_U8 )pHTInfo->dualCTSProtection )
    {
        pMac->lim.gHTDualCTSProtection = ( tANI_U8 )pHTInfo->dualCTSProtection;
        // Send change notification to HAL
    }

    if ( pMac->lim.gHTSecondaryBeacon != ( tANI_U8 )pHTInfo->secondaryBeacon )
    {
        pMac->lim.gHTSecondaryBeacon = ( tANI_U8 )pHTInfo->secondaryBeacon;
        // Send change notification to HAL
    }

    if ( pMac->lim.gHTLSigTXOPFullSupport != ( tANI_U8 )pHTInfo->lsigTXOPProtectionFullSupport )
    {
        pMac->lim.gHTLSigTXOPFullSupport = ( tANI_U8 )pHTInfo->lsigTXOPProtectionFullSupport;
        // Send change notification to HAL
    }

    if ( pMac->lim.gHTPCOActive != ( tANI_U8 )pHTInfo->pcoActive )
    {
        pMac->lim.gHTPCOActive = ( tANI_U8 )pHTInfo->pcoActive;
        // Send change notification to HAL
    }

    if ( pMac->lim.gHTPCOPhase != ( tANI_U8 )pHTInfo->pcoPhase )
    {
        pMac->lim.gHTPCOPhase = ( tANI_U8 )pHTInfo->pcoPhase;
        // Send change notification to HAL
    }

} // End limUpdateStaRunTimeHTInfo.


/** -------------------------------------------------------------
\fn limProcessHalIndMessages
\brief callback function for HAL indication
\param   tpAniSirGlobal pMac
\param    tANI_U32 mesgId
\param    void *mesgParam
\return tSirRetStatu - status
  -------------------------------------------------------------*/

tSirRetStatus limProcessHalIndMessages(tpAniSirGlobal pMac, tANI_U32 msgId, void *msgParam )
{
  //its PE's responsibility to free msgparam when its done extracting the message parameters.
  tSirMsgQ msg;

  switch(msgId)
  {
    case SIR_LIM_DEL_TS_IND:
    case SIR_LIM_ADD_BA_IND:    
    case SIR_LIM_DEL_BA_ALL_IND:
    case SIR_LIM_DELETE_STA_CONTEXT_IND:        
    case SIR_LIM_BEACON_GEN_IND:
      msg.type = (tANI_U16) msgId;
      msg.bodyptr = msgParam;
      msg.bodyval = 0;
      break;

    default:
      palFreeMemory(pMac->hHdd, msgParam);
      limLog(pMac, LOGP, FL("invalid message id = %d received\n"), msgId);
      return eSIR_FAILURE;
  }

  if (limPostMsgApi(pMac, &msg) != eSIR_SUCCESS)
  {
    palFreeMemory(pMac->hHdd, msgParam);
    limLog(pMac, LOGP, FL("limPostMsgApi failed for msgid = %d"), msg.type);
    return eSIR_FAILURE;
  }
  return eSIR_SUCCESS;
}

/** -------------------------------------------------------------
\fn limValidateDeltsReq
\brief Validates DelTs req originated by SME or by HAL and also sends halMsg_DelTs to HAL
\param   tpAniSirGlobal pMac
\param     tpSirDeltsReq pDeltsReq
\param   tSirMacAddr peerMacAddr
\return eSirRetStatus - status
  -------------------------------------------------------------*/

tSirRetStatus
limValidateDeltsReq(tpAniSirGlobal pMac, tpSirDeltsReq pDeltsReq, tSirMacAddr peerMacAddr,tpPESession psessionEntry)
{
    tpDphHashNode pSta;
    tANI_U8            tsStatus;
    tSirMacTSInfo *tsinfo;
    tANI_U32 i;
    tANI_U8 tspecIdx;
    /* if sta
     *  - verify assoc state
     *  - del tspec locally
     * if ap,
     *  - verify sta is in assoc state
     *  - del sta tspec locally
     */
    if(pDeltsReq == NULL)
    {
      PELOGE(limLog(pMac, LOGE, FL("Delete TS request pointer is NULL\n"));)
      return eSIR_FAILURE;
    }

    if ((psessionEntry->limSystemRole == eLIM_STA_ROLE)||(psessionEntry->limSystemRole == eLIM_BT_AMP_STA_ROLE))
    {
        tANI_U32 val;

        // station always talks to the AP
        pSta = dphGetHashEntry(pMac, DPH_STA_HASH_INDEX_PEER, &psessionEntry->dph.dphHashTable);

        val = sizeof(tSirMacAddr);
        #if 0
        if (wlan_cfgGetStr(pMac, WNI_CFG_BSSID, peerMacAddr, &val) != eSIR_SUCCESS)
        {
            /// Could not get BSSID from CFG. Log error.
            limLog(pMac, LOGP, FL("could not retrieve BSSID\n"));
            return eSIR_FAILURE;
        }
       #endif// TO SUPPORT BT-AMP
       sirCopyMacAddr(peerMacAddr,psessionEntry->bssId);
       
    }
    else
    {
        tANI_U16 assocId;
        tANI_U8 *macaddr = (tANI_U8 *) peerMacAddr;

        assocId = pDeltsReq->aid;
        if (assocId != 0)
            pSta = dphGetHashEntry(pMac, assocId, &psessionEntry->dph.dphHashTable);
        else
            pSta = dphLookupHashEntry(pMac, pDeltsReq->macAddr, &assocId, &psessionEntry->dph.dphHashTable);

        if (pSta != NULL)
            // TBD: check sta assoc state as well
            for (i =0; i < sizeof(tSirMacAddr); i++)
                macaddr[i] = pSta->staAddr[i];
    }

    if (pSta == NULL)
    {
        PELOGE(limLog(pMac, LOGE, "Cannot find station context for delts req\n");)
        return eSIR_FAILURE;
    }

    if ((! pSta->valid) ||
        (pSta->mlmStaContext.mlmState != eLIM_MLM_LINK_ESTABLISHED_STATE))
    {
        PELOGE(limLog(pMac, LOGE, "Ivalid Sta (or state) for DelTsReq\n");)
        return eSIR_FAILURE;
    }

    pDeltsReq->req.wsmTspecPresent = 0;
    pDeltsReq->req.wmeTspecPresent = 0;
    pDeltsReq->req.lleTspecPresent = 0;

    if ((pSta->wsmEnabled) &&
        (pDeltsReq->req.tspec.tsinfo.traffic.accessPolicy != SIR_MAC_ACCESSPOLICY_EDCA))
        pDeltsReq->req.wsmTspecPresent = 1;
    else if (pSta->wmeEnabled)
        pDeltsReq->req.wmeTspecPresent = 1;
    else if (pSta->lleEnabled)
        pDeltsReq->req.lleTspecPresent = 1;
    else
    {
        PELOGW(limLog(pMac, LOGW, FL("DELTS_REQ ignore - qos is disabled\n"));)
        return eSIR_FAILURE;
    }

    tsinfo = pDeltsReq->req.wmeTspecPresent ? &pDeltsReq->req.tspec.tsinfo
                                            : &pDeltsReq->req.tsinfo;
   PELOG1(limLog(pMac, LOG1,
           FL("received DELTS_REQ message (wmeTspecPresent = %d, lleTspecPresent = %d, wsmTspecPresent = %d, tsid %d,  up %d, direction = %d)\n"),
           pDeltsReq->req.wmeTspecPresent, pDeltsReq->req.lleTspecPresent, pDeltsReq->req.wsmTspecPresent,
           tsinfo->traffic.tsid, tsinfo->traffic.userPrio, tsinfo->traffic.direction);)

       // if no Access Control, ignore the request
#if (defined(ANI_PRODUCT_TYPE_AP) || defined(ANI_PRODUCT_TYPE_AP_SDK))
       if ((tsinfo->traffic.accessPolicy == SIR_MAC_ACCESSPOLICY_EDCA))
           if (((psessionEntry->limSystemRole == eLIM_AP_ROLE) || (psessionEntry->limSystemRole == eLIM_BT_AMP_AP_ROLE))&& 
                       (! pMac->sch.schObject.gSchEdcaParamsBC[upToAc(tsinfo->traffic.userPrio)].aci.acm))
                   || (((psessionEntry->limSystemRole != eLIM_AP_ROLE) ||(psessionEntry->limSystemRole == eLIM_BT_AMP_AP_ROLE)) && 
                       (! pMac->sch.schObject.gSchEdcaParams[upToAc(tsinfo->traffic.userPrio)].aci.acm)))
       {
           limLog(pMac, LOGW, FL("DelTs with acecssPolicy = %d and UP %d , AC = %d has no AC - ignoring request\n"),
                  tsinfo->traffic.accessPolicy, tsinfo->traffic.userPrio, upToAc(tsinfo->traffic.userPrio));
           return eSIR_FAILURE;
       }
#endif

    if (limAdmitControlDeleteTS(pMac, pSta->assocId, tsinfo, &tsStatus, &tspecIdx)
        != eSIR_SUCCESS)
    {
       PELOG1(limLog(pMac, LOG1, "DELTS request for sta assocId %d (tsid %d, up %d): OK\n",
               pSta->assocId, tsinfo->traffic.tsid, tsinfo->traffic.userPrio);)
        return eSIR_FAILURE;
    }
    else if ((tsinfo->traffic.accessPolicy == SIR_MAC_ACCESSPOLICY_HCCA) ||
             (tsinfo->traffic.accessPolicy == SIR_MAC_ACCESSPOLICY_BOTH))
    {
      //edca only now.
    }
    else
    {
        if((tsinfo->traffic.accessPolicy == SIR_MAC_ACCESSPOLICY_EDCA) && 
           pMac->sch.schObject.gSchEdcaParams[upToAc(tsinfo->traffic.userPrio)].aci.acm)
        {
            //send message to HAL to delete TS
            if(eSIR_SUCCESS != limSendHalMsgDelTs(pMac, pSta->staIndex, tspecIdx, pDeltsReq->req))
            {
                limLog(pMac, LOGW, FL("DelTs with UP %d failed in limSendHalMsgDelTs - ignoring request\n"),
                                 tsinfo->traffic.userPrio);
                return eSIR_FAILURE;
            }
        }
    }
    return eSIR_SUCCESS;
}

/** -------------------------------------------------------------
\fn limRegisterHalIndCallBack
\brief registers callback function to HAL for any indication.
\param   tpAniSirGlobal pMac
\return none.
  -------------------------------------------------------------*/
void
limRegisterHalIndCallBack(tpAniSirGlobal pMac)
{
    tSirMsgQ msg;
    tpHalIndCB pHalCB;

    if( eHAL_STATUS_SUCCESS != palAllocateMemory( pMac->hHdd, (void **)&pHalCB, sizeof(tHalIndCB)))
    {
       limLog(pMac, LOGP, FL("palAllocateMemory() failed\n"));
       return;
    }

    pHalCB->pHalIndCB = limProcessHalIndMessages;

    msg.type = WDA_REGISTER_PE_CALLBACK;
    msg.bodyptr = pHalCB;
    msg.bodyval = 0;
    
    MTRACE(macTraceMsgTx(pMac, 0, msg.type));
    if(eSIR_SUCCESS != wdaPostCtrlMsg(pMac, &msg))
    {
        palFreeMemory(pMac->hHdd, pHalCB);
        limLog(pMac, LOGP, FL("wdaPostCtrlMsg() failed\n"));
    }
    
    return;
}


/** -------------------------------------------------------------
\fn limProcessAddBaInd

\brief handles the BA activity check timeout indication coming from HAL.
         Validates the request, posts request for sending addBaReq message for every candidate in the list.
\param   tpAniSirGlobal pMac
\param  tSirMsgQ limMsg
\return None
-------------------------------------------------------------*/
void
limProcessAddBaInd(tpAniSirGlobal pMac, tpSirMsgQ limMsg)
{
    tANI_U8             i;
    tANI_U8             tid;
    tANI_U16            assocId;
    tpDphHashNode       pSta;
    tpAddBaCandidate    pBaCandidate;
    tANI_U32            baCandidateCnt;
    tpBaActivityInd     pBaActivityInd;
    tpPESession         psessionEntry;
    tANI_U8             sessionId;

    

    if(limMsg->bodyptr == NULL)
        return;
    
    pBaActivityInd = (tpBaActivityInd)limMsg->bodyptr;
    baCandidateCnt = pBaActivityInd->baCandidateCnt;

    if((psessionEntry = peFindSessionByBssid(pMac,pBaActivityInd->bssId,&sessionId))== NULL)
    {
        limLog(pMac, LOGE,FL("session does not exist for given BSSId\n"));
        return;
    }
       
    //if we are not HT capable we don't need to handle BA timeout indication from HAL.
    if((baCandidateCnt  > pMac->lim.maxStation) ||   (!pMac->lim.htCapability))
    {
        palFreeMemory(pMac->hHdd, limMsg->bodyptr);
        return;
    }
  
    //delete the complete dialoguetoken linked list
    limDeleteDialogueTokenList(pMac);
    pBaCandidate =  (tpAddBaCandidate) (((tANI_U8*)pBaActivityInd) + sizeof(tBaActivityInd));

    for(i=0; i<baCandidateCnt; i++, pBaCandidate++) 
    {
       pSta = dphLookupHashEntry(pMac, pBaCandidate->staAddr, &assocId, &psessionEntry->dph.dphHashTable);
       if( (NULL == pSta) || (!pSta->valid))
        continue;

        for (tid=0; tid<STACFG_MAX_TC; tid++)
        {
            if( (eBA_DISABLE == pSta->tcCfg[tid].fUseBATx) && 
                 (pBaCandidate->baInfo[tid].fBaEnable))
            {
               PELOG2(limLog(pMac, LOG2, FL("BA setup for staId = %d, TID: %d, SSN:%d.\n"),
                        pSta->staIndex, tid, pBaCandidate->baInfo[tid].startingSeqNum);)
                limPostMlmAddBAReq(pMac, pSta, tid, pBaCandidate->baInfo[tid].startingSeqNum,psessionEntry);  
    }
}
    }
    palFreeMemory(pMac->hHdd, limMsg->bodyptr);
    return;
}


/** -------------------------------------------------------------
\fn limDelAllBASessions
\brief Deletes all the exisitng BA sessions.
\        Note : This API is provided for Mac OSx only. The reason for this is that Mac OSx may not
\                  restart after CFG update.
\param   tpAniSirGlobal pMac
\return None
-------------------------------------------------------------*/

void 
limDelAllBASessions(tpAniSirGlobal pMac)
{
    tANI_U32 i;
    tANI_U8 tid;
    tpDphHashNode pSta;

    tpPESession psessionEntry =  &pMac->lim.gpSession[0]; //TBD-RAJESH HOW TO GET sessionEntry?????
    for(tid = 0; tid < STACFG_MAX_TC; tid++)
    {
        if((eLIM_AP_ROLE == psessionEntry->limSystemRole) ||(psessionEntry->limSystemRole == eLIM_BT_AMP_AP_ROLE)||
              (eLIM_STA_IN_IBSS_ROLE == psessionEntry->limSystemRole))
        {
            for(i = 0; i < pMac->lim.maxStation; i++)
            {
                pSta = psessionEntry->dph.dphHashTable.pDphNodeArray + i;
                if (pSta && pSta->added)
                {
                    if(eBA_ENABLE == pSta->tcCfg[tid].fUseBATx)
                    {
                        limPostMlmDelBAReq(pMac, pSta, eBA_INITIATOR, tid, eSIR_MAC_UNSPEC_FAILURE_REASON,psessionEntry);
                    }
                    else if(eBA_ENABLE == pSta->tcCfg[tid].fUseBARx)
                    {
                        limPostMlmDelBAReq(pMac, pSta, eBA_RECIPIENT, tid, eSIR_MAC_UNSPEC_FAILURE_REASON,psessionEntry);
                    }
                }
            }
        }
        else if((eLIM_STA_ROLE == psessionEntry->limSystemRole)||(eLIM_BT_AMP_STA_ROLE == psessionEntry->limSystemRole))
        {
            pSta = dphGetHashEntry(pMac, DPH_STA_HASH_INDEX_PEER, &psessionEntry->dph.dphHashTable);
            if (pSta && pSta->added)
            {
                if(eBA_ENABLE == pSta->tcCfg[tid].fUseBATx)
                {
                    limPostMlmDelBAReq(pMac, pSta, eBA_INITIATOR, tid, eSIR_MAC_UNSPEC_FAILURE_REASON,psessionEntry);
                }
                if(eBA_ENABLE == pSta->tcCfg[tid].fUseBARx)
                {
                    limPostMlmDelBAReq(pMac, pSta, eBA_RECIPIENT, tid, eSIR_MAC_UNSPEC_FAILURE_REASON,psessionEntry);
                }
            }
        }
    }
}
/** -------------------------------------------------------------
\fn limProcessDelTsInd
\brief handles the DeleteTS indication coming from HAL or generated by PE itself in some error cases.
         Validates the request, sends the DelTs action frame to the Peer and sends DelTs indicatoin to HDD.
\param   tpAniSirGlobal pMac
\param  tSirMsgQ limMsg
\return None
-------------------------------------------------------------*/
void
limProcessDelTsInd(tpAniSirGlobal pMac, tpSirMsgQ limMsg)
{
  tpDphHashNode         pSta;
  tpDelTsParams         pDelTsParam = (tpDelTsParams) (limMsg->bodyptr);
  tpSirDeltsReq         pDelTsReq = NULL;
  tSirMacAddr           peerMacAddr;
  tpSirDeltsReqInfo     pDelTsReqInfo;
  tpLimTspecInfo        pTspecInfo;
  tpPESession           psessionEntry;
  tANI_U8               sessionId;  

if((psessionEntry = peFindSessionByBssid(pMac,pDelTsParam->bssId,&sessionId))== NULL)
    {
         limLog(pMac, LOGE,FL("session does not exist for given BssId\n"));
         return;
    }

  pTspecInfo = &(pMac->lim.tspecInfo[pDelTsParam->tspecIdx]);
  if(pTspecInfo->inuse == false)
  {
    PELOGE(limLog(pMac, LOGE, FL("tspec entry with index %d is not in use\n"), pDelTsParam->tspecIdx);)
    goto error1;
  }

  pSta = dphGetHashEntry(pMac, pTspecInfo->assocId, &psessionEntry->dph.dphHashTable);
  if(pSta == NULL)
  {
    limLog(pMac, LOGE, FL("Could not find entry in DPH table for assocId = %d\n"),
                pTspecInfo->assocId);
    goto error1;
  }

  if( eHAL_STATUS_SUCCESS != palAllocateMemory( pMac->hHdd, (void **)&pDelTsReq, sizeof(tSirDeltsReq)))
  {
     PELOGE(limLog(pMac, LOGE, FL("palAllocateMemory() failed\n"));)
     goto error1;
  }

  palZeroMemory( pMac->hHdd, (tANI_U8 *)pDelTsReq, sizeof(tSirDeltsReq));

  if(pSta->wmeEnabled)
    palCopyMemory(pMac->hHdd, &(pDelTsReq->req.tspec), &(pTspecInfo->tspec), sizeof(tSirMacTspecIE));
  else
    palCopyMemory(pMac->hHdd, &(pDelTsReq->req.tsinfo), &(pTspecInfo->tspec.tsinfo), sizeof(tSirMacTSInfo));


  //validate the req
  if (eSIR_SUCCESS != limValidateDeltsReq(pMac, pDelTsReq, peerMacAddr,psessionEntry))
  {
    PELOGE(limLog(pMac, LOGE, FL("limValidateDeltsReq failed\n"));)
    goto error2;
  }
 PELOG1(limLog(pMac, LOG1, "Sent DELTS request to station with assocId = %d MacAddr = %x:%x:%x:%x:%x:%x\n",
            pDelTsReq->aid, peerMacAddr[0], peerMacAddr[1], peerMacAddr[2],
            peerMacAddr[3], peerMacAddr[4], peerMacAddr[5]);)

  limSendDeltsReqActionFrame(pMac, peerMacAddr, pDelTsReq->req.wmeTspecPresent, &pDelTsReq->req.tsinfo, &pDelTsReq->req.tspec,
          psessionEntry);

  // prepare and send an sme indication to HDD
  if( eHAL_STATUS_SUCCESS != palAllocateMemory( pMac->hHdd, (void **)&pDelTsReqInfo, sizeof(tSirDeltsReqInfo)))
  {
     PELOGE(limLog(pMac, LOGE, FL("palAllocateMemory() failed\n"));)
     goto error3;
  }
  palZeroMemory( pMac->hHdd, (tANI_U8 *)pDelTsReqInfo, sizeof(tSirDeltsReqInfo));

  if(pSta->wmeEnabled)
    palCopyMemory(pMac->hHdd, &(pDelTsReqInfo->tspec), &(pTspecInfo->tspec), sizeof(tSirMacTspecIE));
  else
    palCopyMemory(pMac->hHdd, &(pDelTsReqInfo->tsinfo), &(pTspecInfo->tspec.tsinfo), sizeof(tSirMacTSInfo));

  limSendSmeDeltsInd(pMac, pDelTsReqInfo, pDelTsReq->aid,psessionEntry);

error3:
  palFreeMemory(pMac->hHdd, (void *) pDelTsReqInfo);
error2:
  palFreeMemory(pMac->hHdd, (void *) pDelTsReq);
error1:
  palFreeMemory(pMac->hHdd, (void *)(limMsg->bodyptr));
  return;
}

/**
 * \brief Setup an A-MPDU/BA session
 *
 * \sa limPostMlmAddBAReq
 *
 * \param pMac The global tpAniSirGlobal object
 *
 * \param pStaDs DPH Hash Node object of peer STA
 *
 * \param tid TID for which a BA is being setup.
 *            If this is set to 0xFFFF, then we retrieve
 *            the default TID from the CFG
 *
 * \return eSIR_SUCCESS if setup completes successfully
 *         eSIR_FAILURE is some problem is encountered
 */
tSirRetStatus limPostMlmAddBAReq( tpAniSirGlobal pMac,
    tpDphHashNode pStaDs,
    tANI_U8 tid, tANI_U16 startingSeqNum,tpPESession psessionEntry)
{
    tSirRetStatus status = eSIR_SUCCESS;
    tpLimMlmAddBAReq pMlmAddBAReq;
    tpDialogueToken dialogueTokenNode;
    tANI_U32        val = 0;
  
  // Check if the peer is a 11n capable STA
  // FIXME - Need a 11n peer indication in DPH.
  // For now, using the taurusPeer attribute
  //if( 0 == pStaDs->taurusPeer == )
    //return eSIR_SUCCESS;

  // Allocate for LIM_MLM_ADDBA_REQ
  if( eHAL_STATUS_SUCCESS != palAllocateMemory( pMac->hHdd,
                                   (void **) &pMlmAddBAReq,
                                   sizeof( tLimMlmAddBAReq )))
  {
    limLog( pMac, LOGP, FL("palAllocateMemory failed\n"));
    status = eSIR_MEM_ALLOC_FAILED;
    goto returnFailure;
  }

  palZeroMemory( pMac->hHdd, (void *) pMlmAddBAReq, sizeof( tLimMlmAddBAReq ));

  // Copy the peer MAC
  palCopyMemory( pMac->hHdd,
      pMlmAddBAReq->peerMacAddr,
      pStaDs->staAddr,
      sizeof( tSirMacAddr ));

  // Update the TID
  pMlmAddBAReq->baTID = tid;

  // Determine the supported BA policy of local STA
  // for the TID of interest
  pMlmAddBAReq->baPolicy = (pStaDs->baPolicyFlag >> tid) & 0x1;

  // BA Buffer Size
  // Requesting the ADDBA recipient to populate the size.
  // If ADDBA is accepted, a non-zero buffer size should
  // be returned in the ADDBA Rsp
  pMlmAddBAReq->baBufferSize = 0;

  limLog( pMac, LOGW,
      FL( "Requesting an ADDBA to setup a %s BA session with STA %d for TID %d\n" ),
      (pMlmAddBAReq->baPolicy ? "Immediate": "Delayed"),
      pStaDs->staIndex,
      tid );

  // BA Timeout
  // pMlmAddBAReq->baTimeout = pMac->hal.halMac.baTimeout; // In TU's
  if (wlan_cfgGetInt(pMac, WNI_CFG_BA_TIMEOUT, &val) != eSIR_SUCCESS) 
  {
     limLog(pMac, LOGE, FL("could not retrieve BA TIME OUT Param CFG\n"));
     status = eSIR_FAILURE;
     goto returnFailure;
  }
  pMlmAddBAReq->baTimeout = val; // In TU's

  // ADDBA Failure Timeout
  // FIXME_AMPDU - Need to retrieve this from CFG. 
  //right now we are not checking for response timeout. so this field is dummy just to be compliant with the spec.
  pMlmAddBAReq->addBAFailureTimeout = 2000; // In TU's

  // BA Starting Sequence Number
  pMlmAddBAReq->baSSN = startingSeqNum;

  /* Update PE session Id*/
  pMlmAddBAReq->sessionId = psessionEntry->peSessionId;

  LIM_SET_STA_BA_STATE(pStaDs, tid, eLIM_BA_STATE_WT_ADD_RSP);

  if( NULL == (dialogueTokenNode =  limAssignDialogueToken(pMac)))
      goto returnFailure;
  
  pMlmAddBAReq->baDialogToken = dialogueTokenNode->token;
  //set assocId and tid information in the lim linked list 
  dialogueTokenNode->assocId = pStaDs->assocId;
  dialogueTokenNode->tid = tid;
  // Send ADDBA Req to MLME
  limPostMlmMessage( pMac,
      LIM_MLM_ADDBA_REQ,
      (tANI_U32 *) pMlmAddBAReq );

returnFailure:

  return status;
}

/**
 * \brief Post LIM_MLM_ADDBA_RSP to MLME. MLME
 * will then send an ADDBA Rsp to peer MAC entity
 * with the appropriate ADDBA status code
 *
 * \sa limPostMlmAddBARsp
 *
 * \param pMac The global tpAniSirGlobal object
 *
 * \param peerMacAddr MAC address of peer entity that will
 * be the recipient of this ADDBA Rsp
 *
 * \param baStatusCode ADDBA Rsp status code
 *
 * \param baDialogToken ADDBA Rsp dialog token
 *
 * \param baTID TID of interest
 *
 * \param baPolicy The BA policy
 *
 * \param baBufferSize The BA buffer size
 *
 * \param baTimeout BA timeout in TU's
 *
 * \return eSIR_SUCCESS if setup completes successfully
 *         eSIR_FAILURE is some problem is encountered
 */
tSirRetStatus limPostMlmAddBARsp( tpAniSirGlobal pMac,
    tSirMacAddr peerMacAddr,
    tSirMacStatusCodes baStatusCode,
    tANI_U8 baDialogToken,
    tANI_U8 baTID,
    tANI_U8 baPolicy,
    tANI_U16 baBufferSize,
    tANI_U16 baTimeout,
    tpPESession psessionEntry)
{
tSirRetStatus status = eSIR_SUCCESS;
tpLimMlmAddBARsp pMlmAddBARsp;

  // Allocate for LIM_MLM_ADDBA_RSP
  if( eHAL_STATUS_SUCCESS != palAllocateMemory( pMac->hHdd,
        (void **) &pMlmAddBARsp,
        sizeof( tLimMlmAddBARsp )))
  {
    limLog( pMac, LOGE,
        FL("palAllocateMemory failed with error code %d\n"),
        status );

    status = eSIR_MEM_ALLOC_FAILED;
    goto returnFailure;
  }

  palZeroMemory( pMac->hHdd, (void *) pMlmAddBARsp, sizeof( tLimMlmAddBARsp ));

  // Copy the peer MAC
  palCopyMemory( pMac->hHdd,
      pMlmAddBARsp->peerMacAddr,
      peerMacAddr,
      sizeof( tSirMacAddr ));

  pMlmAddBARsp->baDialogToken = baDialogToken;
  pMlmAddBARsp->addBAResultCode = baStatusCode;
  pMlmAddBARsp->baTID = baTID;
  pMlmAddBARsp->baPolicy = baPolicy;
  pMlmAddBARsp->baBufferSize = baBufferSize;
  pMlmAddBARsp->baTimeout = baTimeout;

  /* UPdate PE session ID*/
  pMlmAddBARsp->sessionId = psessionEntry->peSessionId;

  // Send ADDBA Rsp to MLME
  limPostMlmMessage( pMac,
      LIM_MLM_ADDBA_RSP,
      (tANI_U32 *) pMlmAddBARsp );

returnFailure:

  return status;
}

/**
 * \brief Post LIM_MLM_DELBA_REQ to MLME. MLME
 * will then send an DELBA Ind to peer MAC entity
 * with the appropriate DELBA status code
 *
 * \sa limPostMlmDelBAReq
 *
 * \param pMac The global tpAniSirGlobal object
 *
 * \param pSta DPH Hash Node object of peer MAC entity
 * for which the BA session is being deleted
 *
 * \param baDirection DELBA direction
 *
 * \param baTID TID for which the BA session is being deleted
 *
 * \param baReasonCode DELBA Req reason code
 *
 * \return eSIR_SUCCESS if setup completes successfully
 *         eSIR_FAILURE is some problem is encountered
 */
tSirRetStatus limPostMlmDelBAReq( tpAniSirGlobal pMac,
    tpDphHashNode pSta,
    tANI_U8 baDirection,
    tANI_U8 baTID,
    tSirMacReasonCodes baReasonCode,
    tpPESession psessionEntry)
{
tSirRetStatus status = eSIR_SUCCESS;
tpLimMlmDelBAReq pMlmDelBAReq;
tLimBAState curBaState;

if(NULL == pSta)
    return eSIR_FAILURE;

LIM_GET_STA_BA_STATE(pSta, baTID, &curBaState);

  // Need to validate the current BA State.
  if( eLIM_BA_STATE_IDLE != curBaState)
  {
    limLog( pMac, LOGE,
        FL( "Received unexpected DELBA REQ when STA BA state for tid = %d is %d\n" ),
        baTID,
        curBaState);

    status = eSIR_FAILURE;
    goto returnFailure;
  }

  // Allocate for LIM_MLM_DELBA_REQ
  if( eHAL_STATUS_SUCCESS != palAllocateMemory( pMac->hHdd,
        (void **) &pMlmDelBAReq,
        sizeof( tLimMlmDelBAReq )))
  {
    limLog( pMac, LOGE,
        FL("palAllocateMemory failed with error code %d\n"),
        status );

    status = eSIR_MEM_ALLOC_FAILED;
    goto returnFailure;
  }

  palZeroMemory( pMac->hHdd, (void *) pMlmDelBAReq, sizeof( tLimMlmDelBAReq ));

  // Copy the peer MAC
  palCopyMemory( pMac->hHdd,
      pMlmDelBAReq->peerMacAddr,
      pSta->staAddr,
      sizeof( tSirMacAddr ));

  pMlmDelBAReq->baDirection = baDirection;
  pMlmDelBAReq->baTID = baTID;
  pMlmDelBAReq->delBAReasonCode = baReasonCode;

  /* Update PE session ID*/
  pMlmDelBAReq->sessionId = psessionEntry->peSessionId;

  //we don't have valid BA session for the given direction. 
  // HDD wants to get the BA session deleted on PEER in this case. 
  // in this case we just need to send DelBA to the peer.
  if(((eBA_RECIPIENT == baDirection) && (eBA_DISABLE == pSta->tcCfg[baTID].fUseBARx)) ||
      ((eBA_INITIATOR == baDirection) && (eBA_DISABLE == pSta->tcCfg[baTID].fUseBATx)))
  {
        // Send DELBA Ind over the air
        if( eSIR_SUCCESS !=
            (status = limSendDelBAInd( pMac, pMlmDelBAReq,psessionEntry)))
          status = eSIR_FAILURE;
  
        palFreeMemory(pMac->hHdd, (void*) pMlmDelBAReq);
        return status;
  }


  // Update the BA state in STA
  LIM_SET_STA_BA_STATE(pSta, pMlmDelBAReq->baTID, eLIM_BA_STATE_WT_DEL_RSP);

  // Send DELBA Req to MLME
  limPostMlmMessage( pMac,
      LIM_MLM_DELBA_REQ,
      (tANI_U32 *) pMlmDelBAReq );

returnFailure:

  return status;
}

/**
 * \brief Send WDA_ADDBA_REQ to HAL, in order
 * to setup a new BA session with a peer
 *
 * \sa limPostMsgAddBAReq
 *
 * \param pMac The global tpAniSirGlobal object
 *
 * \param pSta Runtime, STA-related configuration cached
 * in the HashNode object
 *
 * \param baDialogToken The Action Frame dialog token
 *
 * \param baTID TID for which the BA session is being setup
 *
 * \param baPolicy BA Policy
 *
 * \param baBufferSize The requested BA buffer size
 *
 * \param baTimeout BA Timeout. 0 indicates no BA timeout enforced
 *
 * \param baSSN Starting Sequence Number for this BA session
 *
 * \param baDirection BA Direction: 1 - Initiator, 0 - Recipient
 *
 * \return none
 *
 */
tSirRetStatus limPostMsgAddBAReq( tpAniSirGlobal pMac,
    tpDphHashNode pSta,
    tANI_U8 baDialogToken,
    tANI_U8 baTID,
    tANI_U8 baPolicy,
    tANI_U16 baBufferSize,
    tANI_U16 baTimeout,
    tANI_U16 baSSN,
    tANI_U8 baDirection, 
    tpPESession psessionEntry)
{
tpAddBAParams pAddBAParams = NULL;
tSirRetStatus retCode = eSIR_SUCCESS;
eHalStatus status;
tSirMsgQ msgQ;

#ifdef WLAN_SOFTAP_VSTA_FEATURE
  // we can only do BA on "hard" STAs
  if (!(IS_HWSTA_IDX(pSta->staIndex)))
  {
    retCode = eHAL_STATUS_FAILURE;
    goto returnFailure;
  }
#endif //WLAN_SOFTAP_VSTA_FEATURE

  // Allocate for WDA_ADDBA_REQ
  if( eHAL_STATUS_SUCCESS !=
      (status = palAllocateMemory( pMac->hHdd,
                                   (void **) &pAddBAParams,
                                   sizeof( tAddBAParams ))))
  {
    limLog( pMac, LOGE,
        FL("palAllocateMemory failed with error code %d\n"),
        status );

    retCode = eSIR_MEM_ALLOC_FAILED;
    goto returnFailure;
  }

  palZeroMemory( pMac->hHdd, (void *) pAddBAParams, sizeof( tAddBAParams ));

  // Copy the peer MAC address
  palCopyMemory( pMac->hHdd,
      (void *) pAddBAParams->peerMacAddr,
      (void *) pSta->staAddr,
      sizeof( tSirMacAddr ));

  // Populate the REQ parameters
  pAddBAParams->staIdx = pSta->staIndex;
  pAddBAParams->baDialogToken = baDialogToken;
  pAddBAParams->baTID = baTID;
  pAddBAParams->baPolicy = baPolicy;
  pAddBAParams->baBufferSize = baBufferSize;
  pAddBAParams->baTimeout = baTimeout;
  pAddBAParams->baSSN = baSSN;
  pAddBAParams->baDirection = baDirection;
  pAddBAParams->respReqd = 1;

  /* UPdate PE session ID */
  pAddBAParams->sessionId = psessionEntry->peSessionId;

  // Post WDA_ADDBA_REQ to HAL.
  msgQ.type = WDA_ADDBA_REQ;
  //
  // FIXME_AMPDU
  // A global counter (dialog token) is required to keep track of
  // all PE <-> HAL communication(s)
  //
  msgQ.reserved = 0;
  msgQ.bodyptr = pAddBAParams;
  msgQ.bodyval = 0;

  limLog( pMac, LOGW,
      FL( "Sending WDA_ADDBA_REQ..." ));

  //defer any other message until we get response back.
  SET_LIM_PROCESS_DEFD_MESGS(pMac, false);

  MTRACE(macTraceMsgTx(pMac, 0, msgQ.type));
#ifdef FEATURE_WLAN_DIAG_SUPPORT_LIM //FEATURE_WLAN_DIAG_SUPPORT 
    limDiagEventReport(pMac, WLAN_PE_DIAG_HAL_ADDBA_REQ_EVENT, psessionEntry, 0, 0);
#endif //FEATURE_WLAN_DIAG_SUPPORT
  
  if( eSIR_SUCCESS != (retCode = wdaPostCtrlMsg( pMac, &msgQ )))
    limLog( pMac, LOGE,
        FL("Posting WDA_ADDBA_REQ to HAL failed! Reason = %d\n"),
        retCode );
  else
    return retCode;

returnFailure:

  // Clean-up...
  if( NULL != pAddBAParams )
    palFreeMemory( pMac->hHdd, (void *) pAddBAParams );

  return retCode;

}

/**
 * \brief Send WDA_DELBA_IND to HAL, in order
 * to delete an existing BA session with peer
 *
 * \sa limPostMsgDelBAInd
 *
 * \param pMac The global tpAniSirGlobal object
 *
 * \param pSta Runtime, STA-related configuration cached
 * in the HashNode object
 *
 * \param baTID TID for which the BA session is being setup
 *
 * \param baDirection Identifies whether the DELBA Ind was
 * sent by the BA initiator or recipient
 *
 * \return none
 *
 */
tSirRetStatus limPostMsgDelBAInd( tpAniSirGlobal pMac,
    tpDphHashNode pSta,
    tANI_U8 baTID,
    tANI_U8 baDirection,
    tpPESession psessionEntry)
{
tpDelBAParams pDelBAParams = NULL;
tSirRetStatus retCode = eSIR_SUCCESS;
eHalStatus status;
tSirMsgQ msgQ;

  // Allocate for SIR_HAL_DELBA_IND
  if( eHAL_STATUS_SUCCESS !=
      (status = palAllocateMemory( pMac->hHdd,
                                   (void **) &pDelBAParams,
                                   sizeof( tDelBAParams ))))
  {
    limLog( pMac, LOGE,
        FL("palAllocateMemory failed with error code %d\n"),
        status );

    retCode = eSIR_MEM_ALLOC_FAILED;
    goto returnFailure;
  }

  palZeroMemory( pMac->hHdd, (void *) pDelBAParams, sizeof( tDelBAParams ));

  // Populate the REQ parameters
  pDelBAParams->staIdx = pSta->staIndex;
  pDelBAParams->baTID = baTID;
  pDelBAParams->baDirection = baDirection;

  /* Update PE session ID */
  

  //TBD-RAJESH Updating of the session ID is requird for SIR_HAL_DELBA_IND?????
  //pDelBAParams->sessionId = psessionEntry->peSessionId;

  // Post WDA_DELBA_IND to HAL.
  msgQ.type = WDA_DELBA_IND;
  //
  // FIXME:
  // A global counter (dialog token) is required to keep track of
  // all PE <-> HAL communication(s)
  //
  msgQ.reserved = 0;
  msgQ.bodyptr = pDelBAParams;
  msgQ.bodyval = 0;

  limLog( pMac, LOGW,
      FL( "Sending SIR_HAL_DELBA_IND..." ));

  MTRACE(macTraceMsgTx(pMac, 0, msgQ.type));
#ifdef FEATURE_WLAN_DIAG_SUPPORT_LIM //FEATURE_WLAN_DIAG_SUPPORT 
    limDiagEventReport(pMac, WLAN_PE_DIAG_HAL_DELBA_IND_EVENT, psessionEntry, 0, 0);
#endif //FEATURE_WLAN_DIAG_SUPPORT

  if( eSIR_SUCCESS != (retCode = wdaPostCtrlMsg( pMac, &msgQ )))
    limLog( pMac, LOGE,
        FL("Posting WDA_DELBA_IND to HAL failed! Reason = %d\n"),
        retCode );
  else
  {
    // Update LIM's internal cache...
    if( eBA_INITIATOR == baDirection)
    {
      pSta->tcCfg[baTID].fUseBATx = 0;
      pSta->tcCfg[baTID].txBufSize = 0;
    }
    else
    {
      pSta->tcCfg[baTID].fUseBARx = 0;
      pSta->tcCfg[baTID].rxBufSize = 0;
    }
  
    return retCode;
  }

returnFailure:

  // Clean-up...
  if( NULL != pDelBAParams )
    palFreeMemory( pMac->hHdd, (void *) pDelBAParams );

  return retCode;

}

/**
 * @function :  limPostSMStateUpdate() 
 *
 * @brief  :  This function Updates the HAL and Softmac about the change in the STA's SMPS state.
 *
 *      LOGIC:
 *
 *      ASSUMPTIONS:
 *          NA
 *
 *      NOTE:
 *          NA
 *
 * @param  pMac - Pointer to Global MAC structure
 * @param  limMsg - Lim Message structure object with the MimoPSparam in body
 * @return None
 */
tSirRetStatus 
limPostSMStateUpdate(tpAniSirGlobal pMac, 
        tANI_U16 staIdx, tSirMacHTMIMOPowerSaveState state)
{
    tSirRetStatus             retCode = eSIR_SUCCESS;
    tSirMsgQ                    msgQ;
    eHalStatus                 status;
    tpSetMIMOPS            pMIMO_PSParams;

    msgQ.reserved = 0;
    msgQ.type = WDA_SET_MIMOPS_REQ;

    // Allocate for WDA_SET_MIMOPS_REQ
    status = palAllocateMemory( pMac->hHdd, (void **) &pMIMO_PSParams, sizeof( tSetMIMOPS));
    if( eHAL_STATUS_SUCCESS !=  status) {
        limLog( pMac, LOGP,FL(" palAllocateMemory failed with error code %d\n"), status );
        return eSIR_MEM_ALLOC_FAILED;
    }

    pMIMO_PSParams->htMIMOPSState = state;
    pMIMO_PSParams->staIdx = staIdx;
    pMIMO_PSParams->fsendRsp = true;
    msgQ.bodyptr = pMIMO_PSParams;
    msgQ.bodyval = 0;

    limLog( pMac, LOG2, FL( "Sending WDA_SET_MIMOPS_REQ..." ));

    MTRACE(macTraceMsgTx(pMac, 0, msgQ.type));
    retCode = wdaPostCtrlMsg( pMac, &msgQ );
    if (eSIR_SUCCESS != retCode)
    {
        limLog( pMac, LOGP, FL("Posting WDA_SET_MIMOPS_REQ to HAL failed! Reason = %d\n"), retCode );
        palFreeMemory(pMac->hHdd, (void *) pMIMO_PSParams);
        return retCode;
    }
    
    return retCode;
}

void limPktFree (
    tpAniSirGlobal  pMac,
    eFrameType      frmType,
    tANI_U8         *pRxPacketInfo,
    void            *pBody)
{
    (void) pMac; (void) frmType; (void) pRxPacketInfo; (void) pBody;
#if defined ANI_OS_TYPE_LINUX || defined ANI_OS_TYPE_OSX
    // Free up allocated SK BUF
    palPktFree( pMac->hHdd, frmType, pRxPacketInfo, pBody) ;
#endif
}

/**
 * limGetBDfromRxPacket()
 *
 *FUNCTION:
 * This function is called to get pointer to Polaris
 * Buffer Descriptor containing MAC header & other control
 * info from the body of the message posted to LIM.
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
 * @param  body    - Received message body
 * @param  pRxPacketInfo     - Pointer to received BD
 * @return None
 */

void
limGetBDfromRxPacket(tpAniSirGlobal pMac, void *body, tANI_U32 **pRxPacketInfo)
{
#if defined (ANI_OS_TYPE_LINUX) || defined (ANI_OS_TYPE_OSX)
#ifndef GEN6_ONWARDS
    palGetPacketDataPtr( pMac->hHdd, HAL_TXRX_FRM_802_11_MGMT, (void *) body, (void **) pRxPacketInfo );
#endif //GEN6_ONWARDS
#else
    *pRxPacketInfo = (tANI_U32 *) body;
#endif
} /*** end limGetBDfromRxPacket() ***/





void limRessetScanChannelInfo(tpAniSirGlobal pMac)
{
    palZeroMemory(pMac->hHdd, &pMac->lim.scanChnInfo, sizeof(tLimScanChnInfo));
}


void limAddScanChannelInfo(tpAniSirGlobal pMac, tANI_U8 channelId)
{
    tANI_U8 i;
    tANI_BOOLEAN fFound = eANI_BOOLEAN_FALSE;

    for(i = 0; i < pMac->lim.scanChnInfo.numChnInfo; i++)
    {
        if(pMac->lim.scanChnInfo.scanChn[i].channelId == channelId)
        {
            pMac->lim.scanChnInfo.scanChn[i].numTimeScan++;
            fFound = eANI_BOOLEAN_TRUE;
            break;
        }
    }
    if(eANI_BOOLEAN_FALSE == fFound)
    {
        if(pMac->lim.scanChnInfo.numChnInfo < SIR_MAX_SUPPORTED_CHANNEL_LIST)
        {
            pMac->lim.scanChnInfo.scanChn[pMac->lim.scanChnInfo.numChnInfo].channelId = channelId;
            pMac->lim.scanChnInfo.scanChn[pMac->lim.scanChnInfo.numChnInfo++].numTimeScan = 1;
        }
        else
        {
            PELOGW(limLog(pMac, LOGW, FL(" -- number of channels exceed mac\n"));)
        }
    }
}


/**
 * @function :  limIsChannelValidForChannelSwitch() 
 *
 * @brief  :  This function checks if the channel to which AP
 *            is expecting us to switch, is a valid channel for us.
 *      LOGIC:
 *
 *      ASSUMPTIONS:
 *          NA
 *
 *      NOTE:
 *          NA
 *
 * @param  pMac - Pointer to Global MAC structure
 * @param  channel - New channel to which we are expected to move
 * @return None
 */
tAniBool 
limIsChannelValidForChannelSwitch(tpAniSirGlobal pMac, tANI_U8 channel)
{
    tANI_U8  index;
    tANI_U32    validChannelListLen = WNI_CFG_VALID_CHANNEL_LIST_LEN;
    tSirMacChanNum   validChannelList[WNI_CFG_VALID_CHANNEL_LIST_LEN];

    if (wlan_cfgGetStr(pMac, WNI_CFG_VALID_CHANNEL_LIST,
          (tANI_U8 *)validChannelList,
          (tANI_U32 *)&validChannelListLen) != eSIR_SUCCESS)
    {
        PELOGE(limLog(pMac, LOGE, FL("could not retrieve valid channel list\n"));)
        return (eSIR_FALSE);
    }

    for(index = 0; index < validChannelListLen; index++)
    {
        if(validChannelList[index] == channel)
            return (eSIR_TRUE);
    }

    /* channel does not belong to list of valid channels */
    return (eSIR_FALSE);
}

/**------------------------------------------------------
\fn     __limFillTxControlParams
\brief  Fill the message for stopping/resuming tx.

\param  pMac
\param  pTxCtrlMsg - Pointer to tx control message.
\param  type - Which way we want to stop/ resume tx.
\param  mode - To stop/resume.
 -------------------------------------------------------*/
static eHalStatus
__limFillTxControlParams(tpAniSirGlobal pMac, tpTxControlParams  pTxCtrlMsg,
                                        tLimQuietTxMode type, tLimControlTx mode)
{

    //TBD-RAJESH HOW TO GET sessionEntry?????
    tpPESession psessionEntry = &pMac->lim.gpSession[0];
    
    if (mode == eLIM_STOP_TX)
        pTxCtrlMsg->stopTx =  eANI_BOOLEAN_TRUE;
    else
        pTxCtrlMsg->stopTx =  eANI_BOOLEAN_FALSE;
    
    switch (type)
    {
        case eLIM_TX_ALL:
            /** Stops/resumes transmission completely */
            pTxCtrlMsg->fCtrlGlobal = 1;
            break;

        case eLIM_TX_BSS_BUT_BEACON:
            /** Stops/resumes transmission on a particular BSS. Stoping BSS, doesnt
              *  stop beacon transmission.
              */
            pTxCtrlMsg->ctrlBss = 1;
            pTxCtrlMsg->bssBitmap    |= (1 << psessionEntry->bssIdx);
            break;

        case eLIM_TX_STA:
            /** Memory for station bitmap is allocated dynamically in caller of this
              *  so decode properly here and fill the bitmap. Now not implemented,
              *  fall through.
              */
        case eLIM_TX_BSS:
            //Fall thru...
        default:
            PELOGW(limLog(pMac, LOGW, FL("Invalid case: Not Handled\n"));)
            return eHAL_STATUS_FAILURE;
    }

    return eHAL_STATUS_SUCCESS;
}

/**
 * @function :  limFrameTransmissionControl() 
 *
 * @brief  :  This API is called by the user to halt/resume any frame
 *       transmission from the device. If stopped, all frames will be
 *            queued starting from hardware. Then back-pressure
 *            is built till the driver.
 *      LOGIC:
 *
 *      ASSUMPTIONS:
 *          NA
 *
 *      NOTE:
 *          NA
 *
 * @param  pMac - Pointer to Global MAC structure
 * @return None
 */
void limFrameTransmissionControl(tpAniSirGlobal pMac, tLimQuietTxMode type, tLimControlTx mode)
{

    eHalStatus status = eHAL_STATUS_FAILURE;
    tpTxControlParams pTxCtrlMsg;
    tSirMsgQ          msgQ;
    tANI_U8           nBytes = 0;  // No of bytes required for station bitmap.

    /** Allocate only required number of bytes for station bitmap
     * Make it to align to 4 byte boundary  */
    nBytes = (tANI_U8)HALMSG_NUMBYTES_STATION_BITMAP(pMac->lim.maxStation);

    status = palAllocateMemory(pMac->hHdd, (void **) &pTxCtrlMsg,
                                    (sizeof(*pTxCtrlMsg) + nBytes));
    if (status != eHAL_STATUS_SUCCESS)
    {
        limLog(pMac, LOGP, FL("palAllocateMemory() failed\n"));
        return;
    }

    status = palZeroMemory(pMac->hHdd, (void *) pTxCtrlMsg,
                                       (sizeof(*pTxCtrlMsg) + nBytes));
    if (status != eHAL_STATUS_SUCCESS)
    {
        palFreeMemory(pMac->hHdd, (void *) pTxCtrlMsg);
        limLog(pMac, LOGP, FL("palZeroMemory() failed, status = %d\n"), status);
        return;
    }
    
    status = __limFillTxControlParams(pMac, pTxCtrlMsg, type, mode);
    if (status != eHAL_STATUS_SUCCESS)
    {
        palFreeMemory(pMac->hHdd, (void *) pTxCtrlMsg);
        limLog(pMac, LOGP, FL("__limFillTxControlParams failed, status = %d\n"), status);
        return;
    }
    
    msgQ.bodyptr = (void *) pTxCtrlMsg;
    msgQ.bodyval = 0;
    msgQ.reserved = 0;
    msgQ.type = WDA_TRANSMISSION_CONTROL_IND;

    MTRACE(macTraceMsgTx(pMac, 0, msgQ.type));
    if(wdaPostCtrlMsg( pMac, &msgQ) != eSIR_SUCCESS)
    {
        palFreeMemory(pMac->hHdd, (void *) pTxCtrlMsg);
        limLog( pMac, LOGP, FL("Posting Message to HAL failed\n"));
        return;
    }

    if (mode == eLIM_STOP_TX)
        {
            PELOG1(limLog(pMac, LOG1, FL("Stopping the transmission of all packets, indicated softmac\n"));)
        }
    else
        {
            PELOG1(limLog(pMac, LOG1, FL("Resuming the transmission of all packets, indicated softmac\n"));)
        }
    return;
}


/**
 * @function :  limRestorePreChannelSwitchState() 
 *
 * @brief  :  This API is called by the user to undo any 
 *            specific changes done on the device during
 *            channel switch.
 *      LOGIC:
 *
 *      ASSUMPTIONS:
 *          NA
 *
 *      NOTE:
 *          NA
 *
 * @param  pMac - Pointer to Global MAC structure
 * @return None
 */

tSirRetStatus 
limRestorePreChannelSwitchState(tpAniSirGlobal pMac, tpPESession psessionEntry)
{

    tSirRetStatus retCode = eSIR_SUCCESS;
#if defined(ANI_PRODUCT_TYPE_CLIENT) || defined(ANI_AP_CLIENT_SDK)
    tANI_U32      val = 0;

    if (psessionEntry->limSystemRole != eLIM_STA_ROLE)
        return retCode;
    
    /* Channel switch should be ready for the next time */
    pMac->lim.gLimSpecMgmt.dot11hChanSwState = eLIM_11H_CHANSW_INIT;

    /* Restore the frame transmission, all the time. */
    limFrameTransmissionControl(pMac, eLIM_TX_ALL, eLIM_RESUME_TX);


    //Background scan is now enabled by SME    
    if(pMac->lim.gLimBackgroundScanTerminate == FALSE)
    {       
        /* Enable background scan if already enabled, else don't bother */
        if ((retCode = wlan_cfgGetInt(pMac, WNI_CFG_BACKGROUND_SCAN_PERIOD,
                      &val)) != eSIR_SUCCESS)

        {
            limLog(pMac, LOGP, FL("could not retrieve Background scan period value\n"));
            return (retCode);   
        }

        if (val > 0 && TX_TIMER_VALID(pMac->lim.limTimers.gLimBackgroundScanTimer))
        {
            MTRACE(macTrace(pMac, TRACE_CODE_TIMER_ACTIVATE, 0, eLIM_BACKGROUND_SCAN_TIMER));
            if(tx_timer_activate(&pMac->lim.limTimers.gLimBackgroundScanTimer) != TX_SUCCESS)
            {
                limLog(pMac, LOGP, FL("Could not restart background scan timer, doing LOGP"));
                return (eSIR_FAILURE);
            }

        }
    }

    /* Enable heartbeat timer */
    if (TX_TIMER_VALID(pMac->lim.limTimers.gLimHeartBeatTimer))
    {
        MTRACE(macTrace(pMac, TRACE_CODE_TIMER_ACTIVATE, 0, eLIM_HEART_BEAT_TIMER));
        if(limActivateHearBeatTimer(pMac) != TX_SUCCESS)
        {
            limLog(pMac, LOGP, FL("Could not restart heartbeat timer, doing LOGP"));
            return (eSIR_FAILURE);
        }
    }
#endif
    return (retCode);
}


/**--------------------------------------------
\fn       limRestorePreQuietState
\brief   Restore the pre quiet state

\param pMac
\return NONE
---------------------------------------------*/
tSirRetStatus limRestorePreQuietState(tpAniSirGlobal pMac)
{

    tSirRetStatus retCode = eSIR_SUCCESS;
#if defined(ANI_PRODUCT_TYPE_CLIENT) || defined(ANI_AP_CLIENT_SDK)
    tANI_U32      val = 0;
 
    if (pMac->lim.gLimSystemRole != eLIM_STA_ROLE)
             return retCode;
 
    /* Quiet should be ready for the next time */
    pMac->lim.gLimSpecMgmt.quietState = eLIM_QUIET_INIT;

    /* Restore the frame transmission, all the time. */
    if (pMac->lim.gLimSpecMgmt.quietState == eLIM_QUIET_RUNNING)
        limFrameTransmissionControl(pMac, eLIM_TX_ALL, eLIM_RESUME_TX);


    //Background scan is now enabled by SME
    if(pMac->lim.gLimBackgroundScanTerminate == FALSE)
    {     
        /* Enable background scan if already enabled, else don't bother */
        if ((retCode = wlan_cfgGetInt(pMac, WNI_CFG_BACKGROUND_SCAN_PERIOD,
                      &val)) != eSIR_SUCCESS)

        {
            limLog(pMac, LOGP, FL("could not retrieve Background scan period value\n"));
            return (retCode);   
        }

        if (val > 0 && TX_TIMER_VALID(pMac->lim.limTimers.gLimBackgroundScanTimer))
        {
            MTRACE(macTrace(pMac, TRACE_CODE_TIMER_ACTIVATE, 0, eLIM_BACKGROUND_SCAN_TIMER));
            if(tx_timer_activate(&pMac->lim.limTimers.gLimBackgroundScanTimer) != TX_SUCCESS)
            {
                limLog(pMac, LOGP, FL("Could not restart background scan timer, doing LOGP"));
                return (eSIR_FAILURE);
            }

        }
    }

    /* Enable heartbeat timer */
    if (TX_TIMER_VALID(pMac->lim.limTimers.gLimHeartBeatTimer))
    {
        MTRACE(macTrace(pMac, TRACE_CODE_TIMER_ACTIVATE, 0, eLIM_HEART_BEAT_TIMER));
        if(limActivateHearBeatTimer(pMac) != TX_SUCCESS)
        {
            limLog(pMac, LOGP, FL("Could not restart heartbeat timer, doing LOGP"));
            return (eSIR_FAILURE);
        }
    }
#endif    
    return (retCode);
}


/**
 * @function: limPrepareFor11hChannelSwitch() 
 *
 * @brief  :  This API is called by the user to prepare for
 *            11h channel switch. As of now, the API does 
 *            very minimal work. User can add more into the
 *            same API if needed.
 *      LOGIC:
 *
 *      ASSUMPTIONS:
 *          NA
 *
 *      NOTE:
 *          NA
 *
 * @param  pMac - Pointer to Global MAC structure
 * @param  psessionEntry
 * @return None
 */
void 
limPrepareFor11hChannelSwitch(tpAniSirGlobal pMac, tpPESession psessionEntry)
{
#if defined(ANI_PRODUCT_TYPE_CLIENT) || defined(ANI_AP_CLIENT_SDK)
    if (psessionEntry->limSystemRole != eLIM_STA_ROLE)
        return;
     
    /* Flag to indicate 11h channel switch in progress */
    pMac->lim.gLimSpecMgmt.dot11hChanSwState = eLIM_11H_CHANSW_RUNNING;

    /* Disable, Stop background scan if enabled and running */
    limDeactivateAndChangeTimer(pMac, eLIM_BACKGROUND_SCAN_TIMER);

    /* Stop heart-beat timer to stop heartbeat disassociation */
    limHeartBeatDeactivateAndChangeTimer(pMac, psessionEntry);

    if(pMac->lim.gLimSmeState == eLIM_SME_LINK_EST_WT_SCAN_STATE ||
        pMac->lim.gLimSmeState == eLIM_SME_CHANNEL_SCAN_STATE)
    {
        PELOGE(limLog(pMac, LOGE, FL("Posting finish scan as we are in scan state\n"));)
        /* Stop ongoing scanning if any */
        if (GET_LIM_PROCESS_DEFD_MESGS(pMac))
            limSendHalFinishScanReq(pMac, eLIM_HAL_FINISH_SCAN_WAIT_STATE);
        else
        {
            limRestorePreChannelSwitchState(pMac, psessionEntry);
        }
        return;
    }
    else
    {
        PELOGE(limLog(pMac, LOGE, FL("Not in scan state, start channel switch timer\n"));)
        /** We are safe to switch channel at this point */
        limStopTxAndSwitchChannel(pMac);
    }
#endif
}



/**----------------------------------------------------
\fn        limGetNwType

\brief    Get type of the network from data packet or beacon
\param pMac
\param channelNum - Channel number
\param type - Type of packet.
\param pBeacon - Pointer to beacon or probe response

\return Network type a/b/g.
-----------------------------------------------------*/
tSirNwType limGetNwType(tpAniSirGlobal pMac, tANI_U8 channelNum, tANI_U32 type, tpSchBeaconStruct pBeacon)
{
    tSirNwType nwType = eSIR_11B_NW_TYPE;

    if (type == SIR_MAC_DATA_FRAME)
    {
        if ((channelNum > 0) && (channelNum < 15))
        {
            nwType = eSIR_11G_NW_TYPE;
        }
        else
        {
            nwType = eSIR_11A_NW_TYPE;
        }
    }
    else
    {
        if ((channelNum > 0) && (channelNum < 15))
        {
            int i;
            // 11b or 11g packet
            // 11g iff extended Rate IE is present or
            // if there is an A rate in suppRate IE
            for (i = 0; i < pBeacon->supportedRates.numRates; i++)
            {
                if (sirIsArate(pBeacon->supportedRates.rate[i] & 0x7f))
                {
                    nwType = eSIR_11G_NW_TYPE;
                    break;
                }
            }
            if (pBeacon->extendedRatesPresent)
            {
                PELOG3(limLog(pMac, LOG3, FL("Beacon, nwtype=G\n"));)
                nwType = eSIR_11G_NW_TYPE;
            }
        }
        else
        {
            // 11a packet
            PELOG3(limLog(pMac, LOG3,FL("Beacon, nwtype=A\n"));)
            nwType = eSIR_11A_NW_TYPE;
        }
    }
    return nwType;
}


/**---------------------------------------------------------
\fn        limGetChannelFromBeacon
\brief    To extract channel number from beacon

\param pMac
\param pBeacon - Pointer to beacon or probe rsp
\return channel number
-----------------------------------------------------------*/
tANI_U8 limGetChannelFromBeacon(tpAniSirGlobal pMac, tpSchBeaconStruct pBeacon)
{
    tANI_U8 channelNum = 0;

    if (pBeacon->dsParamsPresent)
        channelNum = pBeacon->channelNumber;
    else if(pBeacon->HTInfo.present)
        channelNum = pBeacon->HTInfo.primaryChannel;
    else
        channelNum = pBeacon->channelNumber;

    return channelNum;
}


/** ---------------------------------------------------------
\fn      limSetTspecUapsdMask
\brief   This function sets the PE global variable:
\        1) gUapsdPerAcTriggerEnableMask and 
\        2) gUapsdPerAcDeliveryEnableMask
\        based on the user priority field and direction field
\        in the TS Info Fields. 
\
\        An AC is a trigger-enabled AC if the PSB subfield 
\        is set to 1 in the uplink direction. 
\        An AC is a delivery-enabled AC if the PSB subfield 
\        is set to 1 in the down-link direction. 
\
\param   tpAniSirGlobal  pMac
\param   tSirMacTSInfo   pTsInfo
\param   tANI_U32        action
\return  None
 ------------------------------------------------------------*/
void limSetTspecUapsdMask(tpAniSirGlobal pMac, tSirMacTSInfo *pTsInfo, tANI_U32 action)
{
    tANI_U8   userPrio = (tANI_U8)pTsInfo->traffic.userPrio;
    tANI_U16  direction = pTsInfo->traffic.direction;  
	tANI_U8   ac = upToAc(userPrio);

    PELOG1(limLog(pMac, LOG1, FL(" Set UAPSD mask for AC %d, direction %d, action=%d (1=set,0=clear) \n"),ac, direction, action );)

    /* Converting AC to appropriate Uapsd Bit Mask
     * AC_BE(0) --> UAPSD_BITOFFSET_ACVO(3)
     * AC_BK(1) --> UAPSD_BITOFFSET_ACVO(2)
     * AC_VI(2) --> UAPSD_BITOFFSET_ACVO(1)
     * AC_VO(3) --> UAPSD_BITOFFSET_ACVO(0)
     */
	ac = ((~ac) & 0x3);

    if (action == CLEAR_UAPSD_MASK) 
	{
        if (direction == SIR_MAC_DIRECTION_UPLINK)
            pMac->lim.gUapsdPerAcTriggerEnableMask &= ~(1 << ac);
        else if (direction == SIR_MAC_DIRECTION_DNLINK)   
            pMac->lim.gUapsdPerAcDeliveryEnableMask &= ~(1 << ac);
        else if (direction == SIR_MAC_DIRECTION_BIDIR)
        {
            pMac->lim.gUapsdPerAcTriggerEnableMask &= ~(1 << ac);
            pMac->lim.gUapsdPerAcDeliveryEnableMask &= ~(1 << ac);
        }
	}
	else if (action == SET_UAPSD_MASK)
	{
        if (direction == SIR_MAC_DIRECTION_UPLINK)
            pMac->lim.gUapsdPerAcTriggerEnableMask |= (1 << ac);
        else if (direction == SIR_MAC_DIRECTION_DNLINK)   
            pMac->lim.gUapsdPerAcDeliveryEnableMask |= (1 << ac);    
        else if (direction == SIR_MAC_DIRECTION_BIDIR)
        {
            pMac->lim.gUapsdPerAcTriggerEnableMask |= (1 << ac);
			pMac->lim.gUapsdPerAcDeliveryEnableMask |= (1 << ac);      
        }
	}

    limLog(pMac, LOGE, FL("New pMac->lim.gUapsdPerAcTriggerEnableMask = 0x%x \n"), pMac->lim.gUapsdPerAcTriggerEnableMask );
    limLog(pMac, LOGE, FL("New pMac->lim.gUapsdPerAcDeliveryEnableMask = 0x%x \n"), pMac->lim.gUapsdPerAcDeliveryEnableMask );

    return;
}



void limHandleHeartBeatTimeout(tpAniSirGlobal pMac )
{

    tANI_U8 i;
	for(i =0;i < pMac->lim.maxBssId;i++)
    {
        if(pMac->lim.gpSession[i].valid == TRUE )
        {
            if(pMac->lim.gpSession[i].bssType == eSIR_IBSS_MODE)
            {
                limIbssHeartBeatHandle(pMac,&pMac->lim.gpSession[i]);
                break;
            }

            if(((pMac->lim.gpSession[i].bssType == eSIR_INFRASTRUCTURE_MODE) &&
				      (pMac->lim.gpSession[i].limSystemRole == eLIM_STA_ROLE))||
                    ( (pMac->lim.gpSession[i].bssType == eSIR_BTAMP_AP_MODE)&&
                      (pMac->lim.gpSession[i].statypeForBss == STA_ENTRY_PEER)) ) 
            {           
                limHandleHeartBeatFailure(pMac,&pMac->lim.gpSession[i]);
                /* The following is to take care of the case where if heartbeat is fine for
                 * the first session but not for the second session */
                if(tx_timer_running(&pMac->lim.limTimers.gLimProbeAfterHBTimer) == VOS_TRUE)
                  break;
            }
            
                        
        }
                 
     }
}  

void limProcessAddStaRsp(tpAniSirGlobal pMac,tpSirMsgQ limMsgQ)
{
     
    tpPESession         psessionEntry;
//    tANI_U8             sessionId;
    tpAddStaParams      pAddStaParams;

    pAddStaParams = (tpAddStaParams)limMsgQ->bodyptr;    
    
    if((psessionEntry = peFindSessionBySessionId(pMac,pAddStaParams->sessionId))==NULL)
    {
        limLog(pMac, LOGP,FL("Session Does not exist for given sessionID\n"));
        return;
    }
    if (psessionEntry->limSystemRole == eLIM_STA_IN_IBSS_ROLE)
        (void) limIbssAddStaRsp(pMac, limMsgQ->bodyptr,psessionEntry);
    
    else
        limProcessMlmAddStaRsp(pMac, limMsgQ,psessionEntry);
                
}


void limUpdateBeacon(tpAniSirGlobal pMac)
{
    tANI_U8 i;
   
    for(i =0;i < pMac->lim.maxBssId;i++)
    {
        if(pMac->lim.gpSession[i].valid == TRUE )
        {
            if( ( (pMac->lim.gpSession[i].limSystemRole == eLIM_AP_ROLE) || 
                (pMac->lim.gpSession[i].limSystemRole == eLIM_STA_IN_IBSS_ROLE) )
                && (eLIM_SME_NORMAL_STATE == pMac->lim.gpSession[i].limSmeState)
               )
            {
                schSetFixedBeaconFields(pMac,&pMac->lim.gpSession[i]);
                limSendBeaconInd(pMac, &pMac->lim.gpSession[i]);
            }
            else
            {
                if( (pMac->lim.gpSession[i].limSystemRole == eLIM_BT_AMP_AP_ROLE)||
                    (pMac->lim.gpSession[i].limSystemRole == eLIM_BT_AMP_STA_ROLE))
                {
                    
                    if(pMac->lim.gpSession[i].statypeForBss == STA_ENTRY_SELF)
                    {
                        schSetFixedBeaconFields(pMac,&pMac->lim.gpSession[i]);
                    }
                }
            }
        }
    }   
}

void limHandleHeartBeatFailureTimeout(tpAniSirGlobal pMac)
{
    tpPESession psessionEntry;
    if((psessionEntry = peFindSessionBySessionId(pMac, pMac->lim.limTimers.gLimProbeAfterHBTimer.sessionId))== NULL) 
    {
        limLog(pMac, LOGE,FL("Session Does not exist for given sessionID\n"));
        return;
    }
    
    /* Probe response is not received  after HB failure.  This is handled by LMM sub module. */
    limLog(pMac, LOGE, FL("Probe_hb_failure: SME %d, MLME %d, HB-Count %d\n"),psessionEntry->limSmeState, psessionEntry->limMlmState, psessionEntry->LimRxedBeaconCntDuringHB);
    if (psessionEntry->limMlmState == eLIM_MLM_LINK_ESTABLISHED_STATE)
    {
        if (!LIM_IS_CONNECTION_ACTIVE(psessionEntry))
        {
            tx_timer_deactivate(&pMac->lim.limTimers.gLimProbeAfterHBTimer);
            /* AP did not respond to Probe Request. Tear down link with it.*/
            limTearDownLinkWithAp(pMac);
            pMac->lim.gLimProbeFailureAfterHBfailedCnt++ ;
        }
        else // restart heartbeat timer  
        {
            limLog(pMac, LOGE, FL("***** **** ProbeReponse timeout with RxedBeaconCount = %d\n"), psessionEntry->LimRxedBeaconCntDuringHB);
            limReactivateHeartBeatTimer(pMac, psessionEntry);
        }
              
    }
    else
    {
        limLog(pMac, LOGE, FL("Unexpected wt-probe-timeout in state \n"));
        limPrintMlmState(pMac, LOGE, psessionEntry->limMlmState);
        limReactivateHeartBeatTimer(pMac, psessionEntry);  
    }
    
              
    
}


/*
* This function assumes there will not be more than one IBSS session active at any time.
*/
tpPESession limIsIBSSSessionActive(tpAniSirGlobal pMac)
{
    tANI_U8 i;
   
    for(i =0;i < pMac->lim.maxBssId;i++)
    {
        if( (pMac->lim.gpSession[i].valid) &&
             (pMac->lim.gpSession[i].limSystemRole == eLIM_STA_IN_IBSS_ROLE))
             return (&pMac->lim.gpSession[i]);
    }

    return NULL;
}

tpPESession limIsApSessionActive(tpAniSirGlobal pMac)
{
    tANI_U8 i;
   
    for(i =0;i < pMac->lim.maxBssId;i++)
    {
        if( (pMac->lim.gpSession[i].valid) &&
             ( (pMac->lim.gpSession[i].limSystemRole == eLIM_AP_ROLE) ||
               (pMac->lim.gpSession[i].limSystemRole == eLIM_BT_AMP_AP_ROLE)))
             return (&pMac->lim.gpSession[i]);
    }

    return NULL;
}

/**---------------------------------------------------------
\fn        limHandleDeferMsgError
\brief    handles error scenario, when the msg can not be deferred.
\param pMac
\param pLimMsg LIM msg, which could not be deferred.
\return void
-----------------------------------------------------------*/

void limHandleDeferMsgError(tpAniSirGlobal pMac, tpSirMsgQ pLimMsg)
{
      if(SIR_BB_XPORT_MGMT_MSG == pLimMsg->type) 
        {
            vos_pkt_return_packet((vos_pkt_t*)pLimMsg->bodyptr);
        }
      else if(pLimMsg->bodyptr != NULL)
            palFreeMemory( pMac->hHdd, (tANI_U8 *) pLimMsg->bodyptr);

}


#ifdef FEATURE_WLAN_DIAG_SUPPORT
/**---------------------------------------------------------
\fn    limDiagEventReport
\brief This function reports Diag event 
\param pMac
\param eventType
\param bssid
\param status
\param reasonCode
\return void
-----------------------------------------------------------*/
void limDiagEventReport(tpAniSirGlobal pMac, tANI_U16 eventType, tpPESession pSessionEntry, tANI_U16 status, tANI_U16 reasonCode)
{
    tSirMacAddr nullBssid = { 0, 0, 0, 0, 0, 0 };
    WLAN_VOS_DIAG_EVENT_DEF(peEvent, vos_event_wlan_pe_payload_type);

    palZeroMemory(pMac->hHdd, &peEvent, sizeof(vos_event_wlan_pe_payload_type));

    if (NULL == pSessionEntry)
    {
       palCopyMemory(pMac->hHdd, peEvent.bssid, nullBssid, sizeof(tSirMacAddr));
       peEvent.sme_state = (tANI_U16)pMac->lim.gLimSmeState;
       peEvent.mlm_state = (tANI_U16)pMac->lim.gLimMlmState;

    }
	 else
    {
       palCopyMemory(pMac->hHdd, peEvent.bssid, pSessionEntry->bssId, sizeof(tSirMacAddr));
       peEvent.sme_state = (tANI_U16)pSessionEntry->limSmeState;
       peEvent.mlm_state = (tANI_U16)pSessionEntry->limMlmState;
    }
    peEvent.event_type = eventType;
    peEvent.status = status;
    peEvent.reason_code = reasonCode;

    WLAN_VOS_DIAG_EVENT_REPORT(&peEvent, EVENT_WLAN_PE);
    return;
}

#endif /* FEATURE_WLAN_DIAG_SUPPORT */

void limProcessAddStaSelfRsp(tpAniSirGlobal pMac,tpSirMsgQ limMsgQ)
{

   tpAddStaSelfParams      pAddStaSelfParams;
   tSirMsgQ                mmhMsg;
   tpSirSmeAddStaSelfRsp   pRsp;

   
   pAddStaSelfParams = (tpAddStaSelfParams)limMsgQ->bodyptr;    

   if( eHAL_STATUS_SUCCESS != palAllocateMemory( pMac->hHdd, (void **)&pRsp, sizeof(tSirSmeAddStaSelfRsp)))
   {
      /// Buffer not available. Log error
      limLog(pMac, LOGP, FL("call to palAllocateMemory failed for Add Sta self RSP\n"));
      return;
   }

   palZeroMemory(pMac, (tANI_U8*)pRsp, sizeof(tSirSmeAddStaSelfRsp));

   pRsp->mesgType = eWNI_SME_ADD_STA_SELF_RSP;
   pRsp->mesgLen = (tANI_U16) sizeof(tSirSmeAddStaSelfRsp);
   pRsp->status = pAddStaSelfParams->status;

   palCopyMemory( pMac->hHdd, pRsp->selfMacAddr, pAddStaSelfParams->selfMacAddr, sizeof(tSirMacAddr) );

   palFreeMemory( pMac->hHdd, (tANI_U8 *)pAddStaSelfParams);

   mmhMsg.type = eWNI_SME_ADD_STA_SELF_RSP;
   mmhMsg.bodyptr = pRsp;
   mmhMsg.bodyval = 0;
   MTRACE(macTraceMsgTx(pMac, 0, mmhMsg.type));
   limSysProcessMmhMsgApi(pMac, &mmhMsg,  ePROT);

}

void limProcessDelStaSelfRsp(tpAniSirGlobal pMac,tpSirMsgQ limMsgQ)
{

   tpDelStaSelfParams      pDelStaSelfParams;
   tSirMsgQ                mmhMsg;
   tpSirSmeDelStaSelfRsp   pRsp;

   
   pDelStaSelfParams = (tpDelStaSelfParams)limMsgQ->bodyptr;    

   if( eHAL_STATUS_SUCCESS != palAllocateMemory( pMac->hHdd, (void **)&pRsp, sizeof(tSirSmeDelStaSelfRsp)))
   {
      /// Buffer not available. Log error
      limLog(pMac, LOGP, FL("call to palAllocateMemory failed for Add Sta self RSP\n"));
      return;
   }

   palZeroMemory(pMac, (tANI_U8*)pRsp, sizeof(tSirSmeDelStaSelfRsp));

   pRsp->mesgType = eWNI_SME_DEL_STA_SELF_RSP;
   pRsp->mesgLen = (tANI_U16) sizeof(tSirSmeDelStaSelfRsp);
   pRsp->status = pDelStaSelfParams->status;

   palCopyMemory( pMac->hHdd, pRsp->selfMacAddr, pDelStaSelfParams->selfMacAddr, sizeof(tSirMacAddr) );

   palFreeMemory( pMac->hHdd, (tANI_U8 *)pDelStaSelfParams);

   mmhMsg.type = eWNI_SME_DEL_STA_SELF_RSP;
   mmhMsg.bodyptr = pRsp;
   mmhMsg.bodyval = 0;
   MTRACE(macTraceMsgTx(pMac, 0, mmhMsg.type));
   limSysProcessMmhMsgApi(pMac, &mmhMsg,  ePROT);

}

/***************************************************************
* tANI_U8 limUnmapChannel(tANI_U8 mapChannel)
* To unmap the channel to reverse the effect of mapping 
* a band channel in hal .Mapping was done hal to overcome the
* limitation of the rxbd which use only 4 bit for channel number.  
*****************************************************************/
tANI_U8 limUnmapChannel(tANI_U8 mapChannel)
{
   if( mapChannel > 0 && mapChannel < 25 )
     return abChannel[mapChannel -1];
   else
     return 0;
}


v_U8_t* limGetIEPtr(tpAniSirGlobal pMac, v_U8_t *pIes, int length, v_U8_t eid,eSizeOfLenField size_of_len_field)
{
    int left = length;
    v_U8_t *ptr = pIes;
    v_U8_t elem_id;
    v_U16_t elem_len;
   
    while(left >= (size_of_len_field+1))
    {   
        elem_id  =  ptr[0];
        if (size_of_len_field == TWO_BYTE)
        {
            elem_len = ((v_U16_t) ptr[1]) | (ptr[2]<<8);
        }
        else
        {
            elem_len =  ptr[1];
        }
    
            
        left -= (size_of_len_field+1);
        if(elem_len > left)
        {
            limLog(pMac, LOGE,
                    "****Invalid IEs eid = %d elem_len=%d left=%d*****\n",
                                                    eid,elem_len,left);
            return NULL;
        }
        if (elem_id == eid) 
        {
            return ptr;
        }
   
        left -= elem_len;
        ptr += (elem_len + (size_of_len_field+1));
    }
    return NULL;
}

/* return NULL if oui is not found in ie
   return !NULL pointer to vendor IE (starting from 0xDD) if oui is found 
 */
v_U8_t* limGetVendorIEOuiPtr(tpAniSirGlobal pMac, tANI_U8 *oui, tANI_U8 oui_size, tANI_U8 *ie, tANI_U16 ie_len)
{   
    int left = ie_len;
    v_U8_t *ptr = ie;
    v_U8_t elem_id, elem_len;

    while(left >= 2)
    {
        elem_id  = ptr[0];
        elem_len = ptr[1];
        left -= 2;
        if(elem_len > left)
        {
            limLog( pMac, LOGE, 
               FL("****Invalid IEs eid = %d elem_len=%d left=%d*****\n"), 
                                               elem_id,elem_len,left);
            return NULL;
        }
        if (SIR_MAC_EID_VENDOR == elem_id) 
        {
            if(memcmp(&ptr[2], oui, oui_size)==0)
                return ptr;
        }
 
        left -= elem_len;
        ptr += (elem_len + 2);
    }
    return NULL;
}

#ifdef WLAN_FEATURE_P2P
//Returns length of NoA stream and Pointer pNoaStream passed to this function is filled with noa stream

v_U8_t limGetNoaAttrStreamInMultP2pIes(tpAniSirGlobal pMac,v_U8_t* noaStream,v_U8_t noaLen,v_U8_t overFlowLen)
{
   v_U8_t overFlowP2pStream[SIR_MAX_NOA_ATTR_LEN];
   palCopyMemory( pMac->hHdd, overFlowP2pStream, noaStream + noaLen - overFlowLen, overFlowLen); 
   noaStream[noaLen - overFlowLen] = SIR_MAC_EID_VENDOR;
   noaStream[noaLen - overFlowLen+1] = overFlowLen + SIR_MAC_P2P_OUI_SIZE;
   palCopyMemory( pMac->hHdd, noaStream+ noaLen - overFlowLen+2,SIR_MAC_P2P_OUI,SIR_MAC_P2P_OUI_SIZE);
   
   palCopyMemory( pMac->hHdd, noaStream+ noaLen - overFlowLen+2+SIR_MAC_P2P_OUI_SIZE,overFlowP2pStream,overFlowLen);
   return (noaLen + SIR_P2P_IE_HEADER_LEN);

}

//Returns length of NoA stream and Pointer pNoaStream passed to this function is filled with noa stream
v_U8_t limGetNoaAttrStream(tpAniSirGlobal pMac, v_U8_t*pNoaStream,tpPESession psessionEntry)
{
    v_U8_t len=0;

    v_U8_t   *pBody = pNoaStream; 
    
   
    if   ( (psessionEntry != NULL) && (psessionEntry->valid) && 
           (psessionEntry->pePersona == VOS_P2P_GO_MODE))
    { 
       if ((!(psessionEntry->p2pGoPsUpdate.uNoa1Duration)) && (!(psessionEntry->p2pGoPsUpdate.uNoa2Duration))
            && (!psessionEntry->p2pGoPsUpdate.oppPsFlag)
          )
         return 0; //No NoA Descriptor then return 0


        pBody[0] = SIR_P2P_NOA_ATTR;
        
        pBody[3] = psessionEntry->p2pGoPsUpdate.index;
        pBody[4] = psessionEntry->p2pGoPsUpdate.ctWin | (psessionEntry->p2pGoPsUpdate.oppPsFlag<<7);
        len = 5;
        pBody += len;
        
        
        if (psessionEntry->p2pGoPsUpdate.uNoa1Duration)
        {
            *pBody = psessionEntry->p2pGoPsUpdate.uNoa1IntervalCnt; 
            pBody += 1;
            len +=1;
             
            *((tANI_U32 *)(pBody)) = sirSwapU32ifNeeded(psessionEntry->p2pGoPsUpdate.uNoa1Duration);
            pBody   += sizeof(tANI_U32);               
            len +=4;
            
            *((tANI_U32 *)(pBody)) = sirSwapU32ifNeeded(psessionEntry->p2pGoPsUpdate.uNoa1Interval);
            pBody   += sizeof(tANI_U32);               
            len +=4;
            
            *((tANI_U32 *)(pBody)) = sirSwapU32ifNeeded(psessionEntry->p2pGoPsUpdate.uNoa1StartTime);
            pBody   += sizeof(tANI_U32);               
            len +=4;
            
        }
        
        if (psessionEntry->p2pGoPsUpdate.uNoa2Duration)
        {
            *pBody = psessionEntry->p2pGoPsUpdate.uNoa2IntervalCnt; 
            pBody += 1;
            len +=1;
             
            *((tANI_U32 *)(pBody)) = sirSwapU32ifNeeded(psessionEntry->p2pGoPsUpdate.uNoa2Duration);
            pBody   += sizeof(tANI_U32);               
            len +=4;
            
            *((tANI_U32 *)(pBody)) = sirSwapU32ifNeeded(psessionEntry->p2pGoPsUpdate.uNoa2Interval);
            pBody   += sizeof(tANI_U32);               
            len +=4;
            
            *((tANI_U32 *)(pBody)) = sirSwapU32ifNeeded(psessionEntry->p2pGoPsUpdate.uNoa2StartTime);
            pBody   += sizeof(tANI_U32);               
            len +=4;

        }
    

        pBody = pNoaStream + 1;            
        *((tANI_U16 *)(pBody)) = sirSwapU16ifNeeded(len-3);/*one byte for Attr and 2 bytes for length*/

        return (len);

    }    
    return 0;
        
}
#endif
