/** ------------------------------------------------------------------------- * 
    ------------------------------------------------------------------------- *  
    \file csrCmdProcess.c
  
    Implementation for processing various commands.
  
   Copyright (c) 2011-2012 Qualcomm Atheros, Inc. 
   All Rights Reserved. 
   Qualcomm Atheros Confidential and Proprietary. 

   Copyright (C) 2006 Airgo Networks, Incorporated
 
   ========================================================================== */


#ifdef FEATURE_WLAN_NON_INTEGRATED_SOC
#include "halInternal.h" //Check if the below include of aniGobal.h is sufficient for Volans too.
#endif

#ifdef FEATURE_WLAN_INTEGRATED_SOC
#include "aniGlobal.h"
#endif

#include "palApi.h"
#include "csrInsideApi.h"
#include "smeInside.h"
#include "smsDebug.h"




eHalStatus csrMsgProcessor( tpAniSirGlobal pMac,  void *pMsgBuf )
{
    eHalStatus status = eHAL_STATUS_SUCCESS;
    tSirSmeRsp *pSmeRsp = (tSirSmeRsp *)pMsgBuf;

    smsLog( pMac, LOG2, "  Message %d[0x%04X] received in curState %d and substate %d\n",
                pSmeRsp->messageType, pSmeRsp->messageType, pMac->roam.curState,
                pMac->roam.curSubState );

    // Process the message based on the state of the roaming states...
    
#if defined( ANI_RTT_DEBUG )
    if(!pAdapter->fRttModeEnabled)
    {
#endif//RTT    
        switch (pMac->roam.curState)
        {
        case eCSR_ROAMING_STATE_SCANNING: 
        {
            //Are we in scan state
#if defined( ANI_EMUL_ASSOC )
            emulScanningStateMsgProcessor( pAdapter, pMBBufHdr );
#else
            status = csrScanningStateMsgProcessor(pMac, pMsgBuf);
#endif    
            break;
        }
        
        case eCSR_ROAMING_STATE_JOINED: 
        {
            //are we in joined state
            csrRoamJoinedStateMsgProcessor( pMac, pMsgBuf );
            break;
        }
        
        case eCSR_ROAMING_STATE_JOINING:
        {
            //are we in roaming states
#if defined( ANI_EMUL_ASSOC )
            emulRoamingStateMsgProcessor( pAdapter, pMBBufHdr );
#endif
            csrRoamingStateMsgProcessor( pMac, pMsgBuf );
            break;
        }

        //For all other messages, we ignore it        
        default:
        {
            /*To work-around an issue where checking for set/remove key base on connection state is no longer 
            * workable due to failure or finding the condition meets both SAP and infra/IBSS requirement.
            */
            if( (eWNI_SME_SETCONTEXT_RSP == pSmeRsp->messageType) ||
                (eWNI_SME_REMOVEKEY_RSP == pSmeRsp->messageType) )
            {
                smsLog(pMac, LOGW, FL(" handling msg 0x%X CSR state is %d\n"), pSmeRsp->messageType, pMac->roam.curState);
                csrRoamCheckForLinkStatusChange(pMac, pSmeRsp);
            }
            else
            {
                smsLog(pMac, LOGW, "  Message 0x%04X is not handled by CSR. CSR state is %d \n", pSmeRsp->messageType, pMac->roam.curState);
            }
            break;
        }
        
        }//switch
        
#if defined( ANI_RTT_DEBUG )
    }
#endif//RTT

    return (status);
}



tANI_BOOLEAN csrCheckPSReady(void *pv)
{
    tpAniSirGlobal pMac = PMAC_STRUCT( pv );

    VOS_ASSERT( pMac->roam.sPendingCommands >= 0 );
    return (pMac->roam.sPendingCommands == 0);
}


void csrFullPowerCallback(void *pv, eHalStatus status)
{
    tpAniSirGlobal pMac = PMAC_STRUCT( pv );
    tListElem *pEntry;
    tSmeCmd *pCommand;

    (void)status;
    
    csrLLLock(&pMac->roam.roamCmdPendingList);
    while( NULL != ( pEntry = csrLLRemoveHead( &pMac->roam.roamCmdPendingList, eANI_BOOLEAN_FALSE ) ) )
    {
        pCommand = GET_BASE_ADDR( pEntry, tSmeCmd, Link );
        smePushCommand( pMac, pCommand, eANI_BOOLEAN_FALSE );
    }
    csrLLUnlock(&pMac->roam.roamCmdPendingList);
}


