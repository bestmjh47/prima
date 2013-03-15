/*
 * Copyright (c) 2012-2013 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
*/

/*
 * Airgo Networks, Inc proprietary. All rights reserved
 * sysEntryFunc.cc - This file has all the system level entry functions
 *                   for all the defined threads at system level.
 * Author:    V. K. Kandarpa
 * Date:      01/16/2002
 * History:-
 * Date       Modified by            Modification Information
 * --------------------------------------------------------------------------
 *
 */
/* Standard include files */

/* Application Specific include files */
#include "sirCommon.h"
#include "aniGlobal.h"


#include "limApi.h"
#include "schApi.h"
#include "utilsApi.h"
#include "pmmApi.h"

#include "sysDebug.h"
#include "sysDef.h"
#include "sysEntryFunc.h"
#include "sysStartup.h"
#include "limTrace.h"
#include "wlan_qct_wda.h"

tSirRetStatus
postPTTMsgApi(tpAniSirGlobal pMac, tSirMsgQ *pMsg);

#include "vos_types.h"
#include "vos_packet.h"

// ---------------------------------------------------------------------------
/**
 * sysInitGlobals
 *
 * FUNCTION:
 *    Initializes system level global parameters
 *
 * LOGIC:
 *
 * ASSUMPTIONS:
 *
 * NOTE:
 *
 * @param tpAniSirGlobal Sirius software parameter struct pointer
 * @return None
 */

tSirRetStatus
sysInitGlobals(tpAniSirGlobal pMac)
{

    palZeroMemory(pMac->hHdd, (tANI_U8 *) &pMac->sys, sizeof(pMac->sys));

#if defined(ANI_DEBUG)
    //FIXME : right now we want the reset to happen even in diag debug build.
    // later on we need to set this to true.
    //pMac->sys.debugOnReset = true;
    pMac->sys.debugOnReset = false;
#else
    pMac->sys.debugOnReset = false;
#endif

    pMac->sys.gSysEnableScanMode        = 1;
    pMac->sys.gSysEnableLinkMonitorMode = 0;
    pMac->sys.fTestRadar                = false;
    pMac->sys.radarDetected             = false;
    pMac->sys.gSysdropLimPkts           = false;
    schInitGlobals(pMac);

    return eSIR_SUCCESS;
}

// ---------------------------------------------------------------------------
/**
 * sysBbtProcessMessageCore
 *
 * FUNCTION:
 * Process BBT messages
 *
 * LOGIC:
 *
 * ASSUMPTIONS:
 *
 * NOTE:
 *
 * @param tpAniSirGlobal A pointer to MAC params instance
 * @param pMsg message pointer
 * @param tANI_U32 type
 * @param tANI_U32 sub type
 * @return None
 */
tSirRetStatus
sysBbtProcessMessageCore(tpAniSirGlobal pMac, tpSirMsgQ pMsg, tANI_U32 type,
                         tANI_U32 subType)
{
    tSirRetStatus ret;
    void*         pBd;
    tMgmtFrmDropReason dropReason;
    vos_pkt_t  *pVosPkt = (vos_pkt_t *)pMsg->bodyptr;
    VOS_STATUS  vosStatus =
              WDA_DS_PeekRxPacketInfo( pVosPkt, (v_PVOID_t *)&pBd, VOS_FALSE );
    pMac->sys.gSysBbtReceived++;

    if ( !VOS_IS_STATUS_SUCCESS(vosStatus) )
    {
        goto fail;
    }

    PELOG3(sysLog(pMac, LOG3, FL("Rx Mgmt Frame Subtype: %d\n"), subType);
    sirDumpBuf(pMac, SIR_SYS_MODULE_ID, LOG3, (tANI_U8 *)WDA_GET_RX_MAC_HEADER(pBd), WDA_GET_RX_MPDU_LEN(pBd));
    sirDumpBuf(pMac, SIR_SYS_MODULE_ID, LOG3, WDA_GET_RX_MPDU_DATA(pBd), WDA_GET_RX_PAYLOAD_LEN(pBd));)

    pMac->sys.gSysFrameCount[type][subType]++;

    if(type == SIR_MAC_MGMT_FRAME)
    {

            if( (dropReason = limIsPktCandidateForDrop(pMac, pBd, subType)) != eMGMT_DROP_NO_DROP)
            {
                PELOG1(sysLog(pMac, LOG1, FL("Mgmt Frame %d being dropped, reason: %d\n"), subType, dropReason);)
                MTRACE(macTrace(pMac,   TRACE_CODE_RX_MGMT_DROP, NO_SESSION, dropReason);)
                goto fail;
            }
            //Post the message to PE Queue
            ret = (tSirRetStatus) limPostMsgApi(pMac, pMsg);
            if (ret != eSIR_SUCCESS)
            {
                PELOGE(sysLog(pMac, LOGE, FL("posting to LIM2 failed, ret %d\n"), ret);)
                goto fail;
            }
            pMac->sys.gSysBbtPostedToLim++;
    }
    else if (type == SIR_MAC_DATA_FRAME)
    {
#ifdef FEATURE_WLAN_TDLS_INTERNAL
       /*
        * if we reached here, probably this frame can be TDLS frame.
        */
       v_U16_t ethType = 0 ;
       v_U8_t *mpduHdr =  NULL ;
       v_U8_t *ethTypeOffset = NULL ;

       /*
        * Peek into payload and extract ethtype.
        * In TDLS we can recieve TDLS frames with MAC HEADER (802.11) and also
        * without MAC Header (Particularly TDLS action frames on direct link.
        */
       mpduHdr = (v_U8_t *)WDA_GET_RX_MAC_HEADER(pBd) ;

#define SIR_MAC_ETH_HDR_LEN                       (14)
       if(0 != WDA_GET_RX_FT_DONE(pBd))
       {
           ethTypeOffset = mpduHdr + SIR_MAC_ETH_HDR_LEN - sizeof(ethType) ;
       }
       else
       {
           ethTypeOffset = mpduHdr + WDA_GET_RX_MPDU_HEADER_LEN(pBd)
                                                     + RFC1042_HDR_LENGTH ;
       }

       ethType = GET_BE16(ethTypeOffset) ;
       if(ETH_TYPE_89_0d == ethType)
       {

           VOS_TRACE(VOS_MODULE_ID_TL, VOS_TRACE_LEVEL_ERROR,
                                                   ("TDLS Data Frame \n")) ;
           /* Post the message to PE Queue */
           PELOGE(sysLog(pMac, LOGE, FL("posting to TDLS frame to lim\n"));)

           ret = (tSirRetStatus) limPostMsgApi(pMac, pMsg);
           if (ret != eSIR_SUCCESS)
           {
               PELOGE(sysLog(pMac, LOGE, FL("posting to LIM2 failed, \
                                                        ret %d\n"), ret);)
               goto fail;
           }
           else
               return eSIR_SUCCESS;
       }
       /* fall through if ethType != TDLS, which is error case */
#endif
#ifdef FEATURE_WLAN_CCX
        PELOGW(sysLog(pMac, LOGW, FL("IAPP Frame...\n")););
        //Post the message to PE Queue
        ret = (tSirRetStatus) limPostMsgApi(pMac, pMsg);
        if (ret != eSIR_SUCCESS)
        {
            PELOGE(sysLog(pMac, LOGE, FL("posting to LIM2 failed, ret %d\n"), ret);)
            goto fail;
        }
        pMac->sys.gSysBbtPostedToLim++;
#endif
    }
    else
    {
        PELOG3(sysLog(pMac, LOG3, "BBT received Invalid type %d subType %d "
                   "LIM state %X. BD dump is:\n",
                   type, subType, limGetSmeState(pMac));
        sirDumpBuf(pMac, SIR_SYS_MODULE_ID, LOG3,
                       (tANI_U8 *) pBd, WLANHAL_RX_BD_HEADER_SIZE);)

        goto fail;
    }

    return eSIR_SUCCESS;

fail:

    pMac->sys.gSysBbtDropped++;
    return eSIR_FAILURE;
}


void sysLog(tpAniSirGlobal pMac, tANI_U32 loglevel, const char *pString,...)
{
    // Verify against current log level
    if ( loglevel > pMac->utils.gLogDbgLevel[LOG_INDEX_FOR_MODULE( SIR_SYS_MODULE_ID )] )
        return;
    else
    {
        va_list marker;

        va_start( marker, pString );     /* Initialize variable arguments. */

        logDebug(pMac, SIR_SYS_MODULE_ID, loglevel, pString, marker);

        va_end( marker );              /* Reset variable arguments.      */
    }
}






