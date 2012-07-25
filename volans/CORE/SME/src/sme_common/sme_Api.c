/*
* Copyright (c) 2012 Qualcomm Atheros, Inc.
* All Rights Reserved.
* Qualcomm Atheros Confidential and Proprietary.
*/

/**=========================================================================

  \file  smeApi.c

  \brief Definitions for SME APIs

   Copyright 2008 (c) Qualcomm, Incorporated.  All Rights Reserved.

   Qualcomm Confidential and Proprietary.

  ========================================================================*/

/*===========================================================================

                      EDIT HISTORY FOR FILE


  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.



  when        	   who                 what, where, why
----------       ---                --------------------------------------------------------
06/03/10     js                     Added support to hostapd driven
 *                                  deauth/disassoc/mic failure

===========================================================================*/

/*--------------------------------------------------------------------------
  Include Files
  ------------------------------------------------------------------------*/
#include "halInternal.h"
#include "smsDebug.h"
#include "sme_Api.h"
#include "csrInsideApi.h"
#include "smeInside.h"
#include "csrInternal.h"

#ifdef WLAN_SOFTAP_FEATURE
#include "sapApi.h"
#endif

// TxMB Functions
extern tSirRetStatus halMmhForwardMBmsg(void* pSirGlobal, tSirMbMsg* pMb);
extern eHalStatus pmcPrepareCommand( tpAniSirGlobal pMac, eSmeCommandType cmdType, void *pvParam,
                            tANI_U32 size, tSmeCmd **ppCmd );
extern void pmcReleaseCommand( tpAniSirGlobal pMac, tSmeCmd *pCommand );
extern void qosReleaseCommand( tpAniSirGlobal pMac, tSmeCmd *pCommand );
#if defined WLAN_FEATURE_P2P
extern eHalStatus p2pProcessRemainOnChannelCmd(tpAniSirGlobal pMac, tSmeCmd *p2pRemainonChn);
extern eHalStatus sme_remainOnChnRsp( tpAniSirGlobal pMac, tANI_U8 *pMsg);
extern eHalStatus sme_mgmtFrmInd( tHalHandle hHal, tpSirSmeMgmtFrameInd pSmeMgmtFrm);
extern eHalStatus sme_remainOnChnReady( tHalHandle hHal, tANI_U8* pMsg);
extern eHalStatus sme_sendActionCnf( tHalHandle hHal, tANI_U8* pMsg);
#endif

static eHalStatus initSmeCmdList(tpAniSirGlobal pMac);
static void smeAbortCommand( tpAniSirGlobal pMac, tSmeCmd *pCommand, tANI_BOOLEAN fStopping );

eCsrPhyMode sme_GetPhyMode(tHalHandle hHal);

//Internal SME APIs
eHalStatus sme_AcquireGlobalLock( tSmeStruct *psSme)
{
    eHalStatus status = eHAL_STATUS_INVALID_PARAMETER;

    if(psSme)
    {
        if( VOS_IS_STATUS_SUCCESS( vos_lock_acquire( &psSme->lkSmeGlobalLock) ) )
        {
            status = eHAL_STATUS_SUCCESS;
        }
    }

    return (status);
}


eHalStatus sme_ReleaseGlobalLock( tSmeStruct *psSme)
{
    eHalStatus status = eHAL_STATUS_INVALID_PARAMETER;

    if(psSme)
    {
        if( VOS_IS_STATUS_SUCCESS( vos_lock_release( &psSme->lkSmeGlobalLock) ) )
        {
            status = eHAL_STATUS_SUCCESS;
        }
    }

    return (status);
}



static eHalStatus initSmeCmdList(tpAniSirGlobal pMac)
{
    eHalStatus status;
    tSmeCmd *pCmd;

    pMac->sme.totalSmeCmd = SME_TOTAL_COMMAND;
    if(HAL_STATUS_SUCCESS(status = csrLLOpen(pMac->hHdd, &pMac->sme.smeCmdActiveList)))
    {
        if(HAL_STATUS_SUCCESS(status = csrLLOpen(pMac->hHdd, &pMac->sme.smeCmdPendingList)))
        {
            if(HAL_STATUS_SUCCESS(status = csrLLOpen(pMac->hHdd, &pMac->sme.smeCmdFreeList)))
            {
                status = palAllocateMemory(pMac->hHdd, (void **)&pCmd, sizeof(tSmeCmd) * pMac->sme.totalSmeCmd);
                if(HAL_STATUS_SUCCESS(status))
                {
                    tANI_U32 c;

                    palZeroMemory(pMac->hHdd, pCmd, sizeof(tSmeCmd) * pMac->sme.totalSmeCmd);
                    pMac->sme.pSmeCmdBufAddr = pCmd;
                    for(c = 0; c < pMac->sme.totalSmeCmd; c++)
                    {
                        csrLLInsertTail(&pMac->sme.smeCmdFreeList, &pCmd[c].Link, LL_ACCESS_LOCK);
                    }
                }
            }
        }
    }

    return (status);
}


void smeReleaseCommand(tpAniSirGlobal pMac, tSmeCmd *pCmd)
{
    pCmd->command = eSmeNoCommand;
    csrLLInsertTail(&pMac->sme.smeCmdFreeList, &pCmd->Link, LL_ACCESS_LOCK);
}



static void smeReleaseCmdList(tpAniSirGlobal pMac, tDblLinkList *pList)
{
    tListElem *pEntry;
    tSmeCmd *pCommand;

    while((pEntry = csrLLRemoveHead(pList, LL_ACCESS_LOCK)) != NULL)
    {
        //TODO: base on command type to call release functions
        //reinitialize different command types so they can be reused
        pCommand = GET_BASE_ADDR( pEntry, tSmeCmd, Link );
        smeAbortCommand(pMac, pCommand, eANI_BOOLEAN_TRUE);
    }
}

static void purgeSmeCmdList(tpAniSirGlobal pMac)
{
    //release any out standing commands back to free command list
    smeReleaseCmdList(pMac, &pMac->sme.smeCmdPendingList);
    smeReleaseCmdList(pMac, &pMac->sme.smeCmdActiveList);
}

void purgeSmeSessionCmdList(tpAniSirGlobal pMac, tANI_U32 sessionId)
{
    //release any out standing commands back to free command list
    tListElem *pEntry, *pNext;
    tSmeCmd *pCommand;
    tDblLinkList *pList = &pMac->sme.smeCmdPendingList;

    csrLLLock(pList);
    pEntry = csrLLPeekHead(pList,LL_ACCESS_NOLOCK);
    while(pEntry != NULL)
    {
        pNext = csrLLNext(pList, pEntry, LL_ACCESS_NOLOCK);
        pCommand = GET_BASE_ADDR( pEntry, tSmeCmd, Link );
        if(pCommand->sessionId == sessionId)
        {
            if(csrLLRemoveEntry(pList, pEntry, LL_ACCESS_NOLOCK))
            {
                smeAbortCommand(pMac, pCommand, eANI_BOOLEAN_TRUE);
            }
        }
        pEntry = pNext;
    }
    csrLLUnlock(pList);
}

static eHalStatus freeSmeCmdList(tpAniSirGlobal pMac)
{
    eHalStatus status = eHAL_STATUS_SUCCESS;

    purgeSmeCmdList(pMac);
    csrLLClose(&pMac->sme.smeCmdPendingList);
    csrLLClose(&pMac->sme.smeCmdActiveList);
    csrLLClose(&pMac->sme.smeCmdFreeList);

    if(NULL != pMac->sme.pSmeCmdBufAddr)
    {
        status = palFreeMemory(pMac->hHdd, pMac->sme.pSmeCmdBufAddr);
        pMac->sme.pSmeCmdBufAddr = NULL;
    }

    return (status);
}


void dumpCsrCommandInfo(tpAniSirGlobal pMac, tSmeCmd *pCmd)
{
#ifdef WLAN_DEBUG
    switch( pCmd->command )
    {
    case eSmeCommandScan:
        smsLog( pMac, LOGE, " scan command reason is %d", pCmd->u.scanCmd.reason );
        break;

    case eSmeCommandRoam:
        smsLog( pMac, LOGE, " roam command reason is %d", pCmd->u.roamCmd.roamReason );
        break;

    case eSmeCommandWmStatusChange:
        smsLog( pMac, LOGE, " WMStatusChange command type is %d", pCmd->u.wmStatusChangeCmd.Type );
        break;

    case eSmeCommandSetKey:
        smsLog( pMac, LOGE, " setKey command auth(%d) enc(%d)",
                        pCmd->u.setKeyCmd.authType, pCmd->u.setKeyCmd.encType );
        break;

    case eSmeCommandRemoveKey:
        smsLog( pMac, LOGE, " removeKey command auth(%d) enc(%d)",
                        pCmd->u.removeKeyCmd.authType, pCmd->u.removeKeyCmd.encType );
        break;

    default:
        break;
    }
#endif  //#ifdef WLAN_DEBUG
}

tSmeCmd *smeGetCommandBuffer( tpAniSirGlobal pMac )
{
    tSmeCmd *pRetCmd = NULL, *pTempCmd = NULL;
    tListElem *pEntry;

    pEntry = csrLLRemoveHead( &pMac->sme.smeCmdFreeList, LL_ACCESS_LOCK );

    // If we can get another MS Msg buffer, then we are ok.  Just link
    // the entry onto the linked list.  (We are using the linked list
    // to keep track of tfhe message buffers).
    if ( pEntry )
    {
        pRetCmd = GET_BASE_ADDR( pEntry, tSmeCmd, Link );
    }
    else {
        int idx = 1;

        //Cannot change pRetCmd here since it needs to return later.
        pEntry = csrLLPeekHead( &pMac->sme.smeCmdActiveList, LL_ACCESS_LOCK );
        if( pEntry )
        {
           pTempCmd = GET_BASE_ADDR( pEntry, tSmeCmd, Link );
        }
        smsLog( pMac, LOGE, "Out of command buffer.... command (0x%X) stuck\n",
           (pTempCmd) ? pTempCmd->command : eSmeNoCommand );
        if(pTempCmd)
        {
            if( eSmeCsrCommandMask & pTempCmd->command )
            {
                //CSR command is stuck. See what the reason code is for that command
                dumpCsrCommandInfo(pMac, pTempCmd);
            }
        } //if(pTempCmd)

        //dump what is in the pending queue
        csrLLLock(&pMac->sme.smeCmdPendingList);
        pEntry = csrLLPeekHead( &pMac->sme.smeCmdPendingList, LL_ACCESS_NOLOCK );
        while(pEntry)
        {
            pTempCmd = GET_BASE_ADDR( pEntry, tSmeCmd, Link );
            smsLog( pMac, LOGE, "Out of command buffer.... SME pending command #%d (0x%X)\n",
                    idx++, pTempCmd->command );
            if( eSmeCsrCommandMask & pTempCmd->command )
            {
                //CSR command is stuck. See what the reason code is for that command
                dumpCsrCommandInfo(pMac, pTempCmd);
            }
            pEntry = csrLLNext( &pMac->sme.smeCmdPendingList, pEntry, LL_ACCESS_NOLOCK );
        }
        csrLLUnlock(&pMac->sme.smeCmdPendingList);

        //There may be some more command in CSR's own pending queue
        csrLLLock(&pMac->roam.roamCmdPendingList);
        pEntry = csrLLPeekHead( &pMac->roam.roamCmdPendingList, LL_ACCESS_NOLOCK );
        while(pEntry)
        {
            pTempCmd = GET_BASE_ADDR( pEntry, tSmeCmd, Link );
            smsLog( pMac, LOGE, "Out of command buffer.... CSR pending command #%d (0x%X)\n",
                    idx++, pTempCmd->command );
            dumpCsrCommandInfo(pMac, pTempCmd);
            pEntry = csrLLNext( &pMac->roam.roamCmdPendingList, pEntry, LL_ACCESS_NOLOCK );
        }
        csrLLUnlock(&pMac->roam.roamCmdPendingList);
    }

    return( pRetCmd );
}


void smePushCommand( tpAniSirGlobal pMac, tSmeCmd *pCmd, tANI_BOOLEAN fHighPriority )
{
    if ( fHighPriority )
    {
        csrLLInsertHead( &pMac->sme.smeCmdPendingList, &pCmd->Link, LL_ACCESS_LOCK );
    }
    else
    {
        csrLLInsertTail( &pMac->sme.smeCmdPendingList, &pCmd->Link, LL_ACCESS_LOCK );
    }

    // process the command queue...
    smeProcessPendingQueue( pMac );

    return;
}


static eSmeCommandType smeIsFullPowerNeeded( tpAniSirGlobal pMac, tSmeCmd *pCommand )
{
    eSmeCommandType pmcCommand = eSmeNoCommand;
    tANI_BOOLEAN fFullPowerNeeded = eANI_BOOLEAN_FALSE;
    tPmcState pmcState;
    eHalStatus status;

    do
    {
        pmcState = pmcGetPmcState(pMac);

        status = csrIsFullPowerNeeded( pMac, pCommand, NULL, &fFullPowerNeeded );
        if( !HAL_STATUS_SUCCESS(status) )
        {
            //PMC state is not right for the command, drop it
            return ( eSmeDropCommand );
        }
        if( fFullPowerNeeded  ) break;
        fFullPowerNeeded = ( ( eSmeCommandAddTs == pCommand->command ) ||
                    ( eSmeCommandDelTs ==  pCommand->command ) );
        if( fFullPowerNeeded ) break;
#ifdef FEATURE_INNAV_SUPPORT
        fFullPowerNeeded = (pmcState == IMPS && 
                                       eSmeCommandMeas == pCommand->command);
        if(fFullPowerNeeded) break;
#endif
#ifdef WLAN_FEATURE_P2P
        fFullPowerNeeded = (pmcState == IMPS && 
                            eSmeCommandRemainOnChannel == pCommand->command);
        if(fFullPowerNeeded) break;
#endif
    } while(0);

    if( fFullPowerNeeded )
    {
        switch( pmcState )
        {
        case IMPS:
        case STANDBY:
            pmcCommand = eSmeCommandExitImps;
            break;

        case BMPS:
            pmcCommand = eSmeCommandExitBmps;
            break;

        case UAPSD:
            pmcCommand = eSmeCommandExitUapsd;
            break;

        case WOWL:
            pmcCommand = eSmeCommandExitWowl;
            break;

        default:
            break;
        }
    }

    return ( pmcCommand );
}


//For commands that need to do extra cleanup.
static void smeAbortCommand( tpAniSirGlobal pMac, tSmeCmd *pCommand, tANI_BOOLEAN fStopping )
{
    if( eSmePmcCommandMask & pCommand->command )
    {
        pmcAbortCommand( pMac, pCommand, fStopping );
    }
    else if ( eSmeCsrCommandMask & pCommand->command )
    {
        csrAbortCommand( pMac, pCommand, fStopping );
    }
    else
    {
        switch( pCommand->command )
        {
#ifdef WLAN_FEATURE_P2P
            case eSmeCommandRemainOnChannel:
                sme_CancelRemainOnChannel( pMac, pCommand->sessionId );
                smeReleaseCommand( pMac, pCommand );

                break;
#endif
            default:
                smeReleaseCommand( pMac, pCommand );
                break;
        }
    }
}


tANI_BOOLEAN smeProcessCommand( tpAniSirGlobal pMac )
{
    tANI_BOOLEAN fContinue = eANI_BOOLEAN_FALSE;
    eHalStatus status = eHAL_STATUS_SUCCESS;
    tListElem *pEntry;
    tSmeCmd *pCommand;
    eSmeCommandType pmcCommand = eSmeNoCommand;

    // if the ActiveList is empty, then nothing is active so we can process a
    // pending command...
    //alwasy lock active list before locking pending list
    csrLLLock( &pMac->sme.smeCmdActiveList );
    if ( csrLLIsListEmpty( &pMac->sme.smeCmdActiveList, LL_ACCESS_NOLOCK ) )
    {
        if(!csrLLIsListEmpty(&pMac->sme.smeCmdPendingList, LL_ACCESS_LOCK))
        {
            //Peek the command
            pEntry = csrLLPeekHead( &pMac->sme.smeCmdPendingList, LL_ACCESS_LOCK );
            if( pEntry )
            {
                pCommand = GET_BASE_ADDR( pEntry, tSmeCmd, Link );
                //We cannot execute any command in wait-for-key state until setKey is through.
                if( CSR_IS_WAIT_FOR_KEY( pMac ) )
                {
                    if( !CSR_IS_SET_KEY_COMMAND( pCommand ) )
                    {
                        csrLLUnlock( &pMac->sme.smeCmdActiveList );
                        smsLog(pMac, LOGE, "  Cannot process command(%d) while waiting for key\n", pCommand->command);
                        return ( eANI_BOOLEAN_FALSE );
                    }
                }
                pmcCommand = smeIsFullPowerNeeded( pMac, pCommand );
                if( eSmeDropCommand == pmcCommand )
                {
                    //This command is not ok for current PMC state
                    if( csrLLRemoveEntry( &pMac->sme.smeCmdPendingList, pEntry, LL_ACCESS_LOCK ) )
                    {
                        smeAbortCommand( pMac, pCommand, eANI_BOOLEAN_FALSE );
                    }
                    csrLLUnlock( &pMac->sme.smeCmdActiveList );
                    //tell caller to continue
                    return (eANI_BOOLEAN_TRUE);
                }
                else if( eSmeNoCommand != pmcCommand )
                {
                    tExitBmpsInfo exitBmpsInfo;
                    void *pv = NULL;
                    tANI_U32 size = 0;
                    tSmeCmd *pPmcCmd = NULL;

                    if( eSmeCommandExitBmps == pmcCommand )
                    {
                        exitBmpsInfo.exitBmpsReason = eSME_REASON_OTHER;
                        pv = (void *)&exitBmpsInfo;
                        size = sizeof(tExitBmpsInfo);
                    }
                    //pmcCommand has to be one of the exit power save command
                    status = pmcPrepareCommand( pMac, pmcCommand, pv, size, &pPmcCmd );
                    if( HAL_STATUS_SUCCESS( status ) && pPmcCmd )
                    {
                        //Force this command to wake up the chip
                        csrLLInsertHead( &pMac->sme.smeCmdActiveList, &pPmcCmd->Link, LL_ACCESS_NOLOCK );
                        csrLLUnlock( &pMac->sme.smeCmdActiveList );
                        fContinue = pmcProcessCommand( pMac, pPmcCmd );
                        if( fContinue )
                        {
                            //The command failed, remove it
                            if( csrLLRemoveEntry( &pMac->sme.smeCmdActiveList, &pPmcCmd->Link, LL_ACCESS_NOLOCK ) )
                            {
                                pmcReleaseCommand( pMac, pPmcCmd );
                            }
                        }
                    }
                    else
                    {
                        csrLLUnlock( &pMac->sme.smeCmdActiveList );
                        smsLog( pMac, LOGE, FL(  "Cannot issue command(0x%X) to wake up the chip. Status = %d\n"), pmcCommand, status );
                        //Let it retry
                        fContinue = eANI_BOOLEAN_TRUE;
                    }
                    return fContinue;
                }
                if ( csrLLRemoveEntry( &pMac->sme.smeCmdPendingList, pEntry, LL_ACCESS_LOCK ) )
                {
                    // we can reuse the pCommand

                    // Insert the command onto the ActiveList...
                    csrLLInsertHead( &pMac->sme.smeCmdActiveList, &pCommand->Link, LL_ACCESS_NOLOCK );

                    // .... and process the command.

                    switch ( pCommand->command )
                    {

                        case eSmeCommandScan:
                            csrLLUnlock( &pMac->sme.smeCmdActiveList );
                            status = csrProcessScanCommand( pMac, pCommand );
                            break;

                        case eSmeCommandRoam:
                            csrLLUnlock( &pMac->sme.smeCmdActiveList );
                            status  = csrRoamProcessCommand( pMac, pCommand );
                            break;

                        case eSmeCommandWmStatusChange:
                            csrLLUnlock( &pMac->sme.smeCmdActiveList );
                            csrRoamProcessWmStatusChangeCommand(pMac, pCommand);
                            break;

                        case eSmeCommandSetKey:
                            csrLLUnlock( &pMac->sme.smeCmdActiveList );
                            status = csrRoamProcessSetKeyCommand( pMac, pCommand );
                            if(!HAL_STATUS_SUCCESS(status))
                            {
                                if( csrLLRemoveEntry( &pMac->sme.smeCmdActiveList,
                                            &pCommand->Link, LL_ACCESS_LOCK ) )
                                {
                                    csrReleaseCommandSetKey( pMac, pCommand );
                                }
                            }
                            break;

                        case eSmeCommandRemoveKey:
                            csrLLUnlock( &pMac->sme.smeCmdActiveList );
                            status = csrRoamProcessRemoveKeyCommand( pMac, pCommand );
                            if(!HAL_STATUS_SUCCESS(status))
                            {
                                if( csrLLRemoveEntry( &pMac->sme.smeCmdActiveList,
                                            &pCommand->Link, LL_ACCESS_LOCK ) )
                                {
                                    csrReleaseCommandRemoveKey( pMac, pCommand );
                                }
                            }
                            break;

                        case eSmeCommandAddStaSession:
                            csrLLUnlock( &pMac->sme.smeCmdActiveList );
                            csrProcessAddStaSessionCommand( pMac, pCommand );
                            break;
                        case eSmeCommandDelStaSession:    
                            csrLLUnlock( &pMac->sme.smeCmdActiveList );
                            csrProcessDelStaSessionCommand( pMac, pCommand );
                            break;

#ifdef FEATURE_INNAV_SUPPORT
                        case eSmeCommandMeas:
                            csrLLUnlock(&pMac->sme.smeCmdActiveList);
                            measProcessInNavMeasCommand(pMac, pCommand);
                            break;
#endif
#if defined WLAN_FEATURE_P2P
                        case eSmeCommandRemainOnChannel:
                            csrLLUnlock(&pMac->sme.smeCmdActiveList);
                            p2pProcessRemainOnChannelCmd(pMac, pCommand);
                            break;
#endif
                        case eSmeCommandEnterImps:
                        case eSmeCommandExitImps:
                        case eSmeCommandEnterBmps:
                        case eSmeCommandExitBmps:
                        case eSmeCommandEnterUapsd:
                        case eSmeCommandExitUapsd:
                        case eSmeCommandEnterWowl:
                        case eSmeCommandExitWowl:
                            csrLLUnlock( &pMac->sme.smeCmdActiveList );
                            fContinue = pmcProcessCommand( pMac, pCommand );
                            if( fContinue )
                            {
                                //The command failed, remove it
                                if( csrLLRemoveEntry( &pMac->sme.smeCmdActiveList,
                                            &pCommand->Link, LL_ACCESS_LOCK ) )
                                {
                                    pmcReleaseCommand( pMac, pCommand );
                                }
                            }
                            break;

                        //Treat standby differently here because caller may not be able to handle
                        //the failure so we do our best here
                        case eSmeCommandEnterStandby:
                            if( csrIsConnStateDisconnected( pMac, pCommand->sessionId ) )
                            {
                                //It can continue
                                csrLLUnlock( &pMac->sme.smeCmdActiveList );
                                fContinue = pmcProcessCommand( pMac, pCommand );
                                if( fContinue )
                                {
                                    //The command failed, remove it
                                    if( csrLLRemoveEntry( &pMac->sme.smeCmdActiveList,
                                                &pCommand->Link, LL_ACCESS_LOCK ) )
                                    {
                                        pmcReleaseCommand( pMac, pCommand );
                                    }
                                }
                            }
                            else
                            {
                                //Need to issue a disconnect first before processing this command
                                tSmeCmd *pNewCmd;

                                //We need to re-run the command
                                fContinue = eANI_BOOLEAN_TRUE;
                                //Pull off the standby command first
                                if( csrLLRemoveEntry( &pMac->sme.smeCmdActiveList,
                                                &pCommand->Link, LL_ACCESS_NOLOCK ) )
                                {
                                    csrLLUnlock( &pMac->sme.smeCmdActiveList );
                                    //Need to call CSR function here because the disconnect command
                                    //is handled by CSR
                                    pNewCmd = csrGetCommandBuffer( pMac );
                                    if( NULL != pNewCmd )
                                    {
                                        //Put the standby command to the head of the pending list first
                                        csrLLInsertHead( &pMac->sme.smeCmdPendingList, &pCommand->Link,
                                                        LL_ACCESS_LOCK );
                                        pNewCmd->command = eSmeCommandRoam;
                                        pNewCmd->u.roamCmd.roamReason = eCsrForcedDisassoc;
                                        //Put the disassoc command before the standby command
                                        csrLLInsertHead( &pMac->sme.smeCmdPendingList, &pNewCmd->Link,
                                                        LL_ACCESS_LOCK );
                                    }
                                    else
                                    {
                                        //Continue the command here
                                        fContinue = pmcProcessCommand( pMac, pCommand );
                                        if( fContinue )
                                        {
                                            //The command failed, remove it
                                            if( csrLLRemoveEntry( &pMac->sme.smeCmdActiveList,
                                                        &pCommand->Link, LL_ACCESS_LOCK ) )
                                            {
                                                pmcReleaseCommand( pMac, pCommand );
                                            }
                                        }
                                    }
                                }
                                else
                                {
                                    csrLLUnlock( &pMac->sme.smeCmdActiveList );
                                    smsLog( pMac, LOGE, FL(" failed to remove standby command\n") );
                                    VOS_ASSERT(0);
                                }
                            }
                            break;

                        case eSmeCommandAddTs:
                        case eSmeCommandDelTs:
                            csrLLUnlock( &pMac->sme.smeCmdActiveList );
#ifndef WLAN_MDM_CODE_REDUCTION_OPT
                            fContinue = qosProcessCommand( pMac, pCommand );
                            if( fContinue )
                            {
                                //The command failed, remove it
                                if( csrLLRemoveEntry( &pMac->sme.smeCmdActiveList,
                                            &pCommand->Link, LL_ACCESS_NOLOCK ) )
                                {
//#ifndef WLAN_MDM_CODE_REDUCTION_OPT
                                    qosReleaseCommand( pMac, pCommand );
//#endif /* WLAN_MDM_CODE_REDUCTION_OPT*/
                                }
                            }
#endif
                            break;

                        default:
                            //something is wrong
                            //remove it from the active list
                            smsLog(pMac, LOGE, " csrProcessCommand processes an unknown command %d\n", pCommand->command);
                            pEntry = csrLLRemoveHead( &pMac->sme.smeCmdActiveList, LL_ACCESS_NOLOCK );
                            csrLLUnlock( &pMac->sme.smeCmdActiveList );
                            pCommand = GET_BASE_ADDR( pEntry, tSmeCmd, Link );
                            smeReleaseCommand( pMac, pCommand );
                            status = eHAL_STATUS_FAILURE;
                            break;
                    }
                    if(!HAL_STATUS_SUCCESS(status))
                    {
                        fContinue = eANI_BOOLEAN_TRUE;
                    }
                }//if(pEntry)
                else
                {
                    //This is odd. Some one else pull off the command.
                    csrLLUnlock( &pMac->sme.smeCmdActiveList );
                }
            }
            else
            {
                csrLLUnlock( &pMac->sme.smeCmdActiveList );
            }
        }
        else
        {
            //No command waiting
            csrLLUnlock( &pMac->sme.smeCmdActiveList );
            //This is only used to restart an idle mode scan, it means at least one other idle scan has finished.
            if(pMac->scan.fRestartIdleScan && eANI_BOOLEAN_FALSE == pMac->scan.fCancelIdleScan)
            {
                tANI_U32 nTime = 0;

                pMac->scan.fRestartIdleScan = eANI_BOOLEAN_FALSE;
                if(!HAL_STATUS_SUCCESS(csrScanTriggerIdleScan(pMac, &nTime)))
                {
                    csrScanStartIdleScanTimer(pMac, nTime);
                }
            }
        }
    }
    else {
        csrLLUnlock( &pMac->sme.smeCmdActiveList );
    }

    return ( fContinue );
}

void smeProcessPendingQueue( tpAniSirGlobal pMac )
{
    while( smeProcessCommand( pMac ) );
}


tANI_BOOLEAN smeCommandPending(tpAniSirGlobal pMac)
{
    return ( !csrLLIsListEmpty( &pMac->sme.smeCmdActiveList, LL_ACCESS_NOLOCK ) ||
        !csrLLIsListEmpty(&pMac->sme.smeCmdPendingList, LL_ACCESS_NOLOCK) );
}



//Global APIs

/*--------------------------------------------------------------------------

  \brief sme_Open() - Initialze all SME modules and put them at idle state

  The function initializes each module inside SME, PMC, CCM, CSR, etc. . Upon
  successfully return, all modules are at idle state ready to start.

  smeOpen must be called before any other SME APIs can be involved.
  smeOpen must be called after macOpen.
  This is a synchronuous call
  \param hHal - The handle returned by macOpen.

  \return eHAL_STATUS_SUCCESS - SME is successfully initialized.

          Other status means SME is failed to be initialized
  \sa

  --------------------------------------------------------------------------*/
eHalStatus sme_Open(tHalHandle hHal)
{
   eHalStatus status = eHAL_STATUS_FAILURE;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   do {
      pMac->sme.state = SME_STATE_STOP;
      if( !VOS_IS_STATUS_SUCCESS( vos_lock_init( &pMac->sme.lkSmeGlobalLock ) ) )
      {
          smsLog( pMac, LOGE, "sme_Open failed init lock\n" );
          status = eHAL_STATUS_FAILURE;
          break;
      }

      status = ccmOpen(hHal);
      if ( ! HAL_STATUS_SUCCESS( status ) ) {
         smsLog( pMac, LOGE, "ccmOpen failed during initialization with \
              status=%d\n", status );
         break;
      }

      status = csrOpen(pMac);
      if ( ! HAL_STATUS_SUCCESS( status ) ) {
         smsLog( pMac, LOGE, "csrOpen failed during initialization with \
                status=%d\n", status );
         break;
      }

      status = pmcOpen(hHal);
      if ( ! HAL_STATUS_SUCCESS( status ) ) {
         smsLog( pMac, LOGE, "pmcOpen failed during initialization with \
              status=%d\n", status );
         break;
      }

#ifndef WLAN_MDM_CODE_REDUCTION_OPT
      status = sme_QosOpen(pMac);
      if ( ! HAL_STATUS_SUCCESS( status ) ) {
         smsLog( pMac, LOGE, "Qos open failed during initialization with \
              status=%d\n", status );
         break;
      }

      status = btcOpen(pMac);
      if ( ! HAL_STATUS_SUCCESS( status ) ) {
         smsLog( pMac, LOGE, "btcOpen open failed during initialization with \
              status=%d\n", status );
         break;
      }
#endif
#ifdef FEATURE_INNAV_SUPPORT
      status = measInNavOpen(pMac);
      if ( ! HAL_STATUS_SUCCESS( status ) ) {
         smsLog(pMac, LOGE, "measInNavOpen failed during initialization with \
              status=%d\n", status );
         break;
      }
#endif

      if(!HAL_STATUS_SUCCESS((status = initSmeCmdList(pMac))))
          break;

#ifdef WLAN_SOFTAP_FEATURE
//      if(VOS_STA_SAP_MODE == vos_get_conparam ( ))
      {
         v_PVOID_t pvosGCtx = vos_get_global_context(VOS_MODULE_ID_SAP, NULL);
		 if ( NULL == pvosGCtx ){
		  	smsLog( pMac, LOGE, "WLANSAP_Open open failed during initialization with \
             	 \n");
            status = eHAL_STATUS_FAILURE;
			break;
		 } 	
		  	
         status = WLANSAP_Open( pvosGCtx );
		 if ( ! HAL_STATUS_SUCCESS( status ) ) {
            smsLog( pMac, LOGE, "WLANSAP_Open open failed during initialization with \
             	 status=%d\n", status );
		    break;
    	 }
      }
#endif
#if defined WLAN_FEATURE_VOWIFI
      status = rrmOpen(pMac);
      if ( ! HAL_STATUS_SUCCESS( status ) ) {
         smsLog( pMac, LOGE, "rrmOpen open failed during initialization with \
              status=%d\n", status );
         break;
      }
#endif

#if defined WLAN_FEATURE_VOWIFI_11R
      sme_FTOpen(pMac);
#endif
#if defined WLAN_FEATURE_P2P
      sme_p2pOpen(pMac);
#endif

   }while (0);

   return status;
}

#ifdef WLAN_SOFTAP_FEATURE
/*--------------------------------------------------------------------------

  \brief sme_set11dinfo() - Set the 11d information about valid channels
   and there power using information from nvRAM
   This function is called only for AP.

  This is a synchronuous call

  \param hHal - The handle returned by macOpen.
  \Param pSmeConfigParams - a pointer to a caller allocated object of
  typedef struct _smeConfigParams.

  \return eHAL_STATUS_SUCCESS - SME update the config parameters successfully.

		Other status means SME is failed to update the config parameters.
  \sa
--------------------------------------------------------------------------*/

eHalStatus sme_set11dinfo(tHalHandle hHal,  tpSmeConfigParams pSmeConfigParams)
{
   eHalStatus status = eHAL_STATUS_FAILURE;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   if (NULL == pSmeConfigParams ) {
      smsLog( pMac, LOGE, "Empty config param structure for SME, nothing to \
            update\n");
      return status;
   }	

   status = csrSetChannels(hHal, &pSmeConfigParams->csrConfig );
   if ( ! HAL_STATUS_SUCCESS( status ) ) {
      smsLog( pMac, LOGE, "csrChangeDefaultConfigParam failed with status=%d\n",
              status );
   }
    return status;
}

eHalStatus sme_setRegInfo(tHalHandle hHal,  tANI_U8 *apCntryCode)
{
   eHalStatus status = eHAL_STATUS_FAILURE;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   if (NULL == apCntryCode ) {
      smsLog( pMac, LOGE, "Empty Country Code, nothing to \
            update\n");
      return status;
   }

   status = csrSetRegInfo(hHal, apCntryCode );
   if ( ! HAL_STATUS_SUCCESS( status ) ) {
      smsLog( pMac, LOGE, "csrSetRegInfo failed with status=%d\n",
              status );
   }
    return status;
}

#endif

/*--------------------------------------------------------------------------

  \brief sme_UpdateConfig() - Change configurations for all SME moduels

  The function updates some configuration for modules in SME, CCM, CSR, etc
  during SMEs close open sequence.

  Modules inside SME apply the new configuration at the next transaction.

  This is a synchronuous call

  \param hHal - The handle returned by macOpen.
  \Param pSmeConfigParams - a pointer to a caller allocated object of
  typedef struct _smeConfigParams.

  \return eHAL_STATUS_SUCCESS - SME update the config parameters successfully.

          Other status means SME is failed to update the config parameters.
  \sa

  --------------------------------------------------------------------------*/
eHalStatus sme_UpdateConfig(tHalHandle hHal, tpSmeConfigParams pSmeConfigParams)
{
   eHalStatus status = eHAL_STATUS_FAILURE;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   if (NULL == pSmeConfigParams ) {
      smsLog( pMac, LOGE, "Empty config param structure for SME, nothing to \
            update\n");
      return status;
   }

   status = csrChangeDefaultConfigParam(pMac, &pSmeConfigParams->csrConfig);

   if ( ! HAL_STATUS_SUCCESS( status ) ) {
      smsLog( pMac, LOGE, "csrChangeDefaultConfigParam failed with status=%d\n",
              status );
   }

#if defined WLAN_FEATURE_VOWIFI
   status = rrmChangeDefaultConfigParam(hHal, &pSmeConfigParams->rrmConfig);

   if ( ! HAL_STATUS_SUCCESS( status ) ) {
      smsLog( pMac, LOGE, "rrmChangeDefaultConfigParam failed with status=%d\n",
              status );
   }
#endif
   //Apply the global config if SME is in ready state
   //We don't want to apply global CFG in connect state because that may cause some side affect
   if( SME_IS_READY(pMac) && csrIsAllSessionDisconnected( pMac) )
   {
       csrSetGlobalCfgs(pMac);
   }

   return status;
}


/*--------------------------------------------------------------------------

  \brief sme_HoConfig() - Change handoff configurations for CSR during SMEs
  close -> open sequence.

  Modules inside SME apply the new configuration at the next transaction.


  \param hHal - The handle returned by macOpen.
  \Param pSmeHoConfigParams - a pointer to a caller allocated object of
  typedef struct _smeHoConfigParams.

  \return eHAL_STATUS_SUCCESS - SME update the config parameters successfully.

          Other status means SME is failed to update the config parameters.
  \sa

  --------------------------------------------------------------------------*/
#ifdef FEATURE_WLAN_GEN6_ROAMING
eHalStatus sme_HoConfig(tHalHandle hHal, tpSmeHoConfigParams pSmeHoConfigParams)
{
   eHalStatus status = eHAL_STATUS_FAILURE;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   if (NULL == pSmeHoConfigParams ) {
      smsLog( pMac, LOGE, "Empty config param structure for SME\n");
   }

   if(pSmeHoConfigParams)
   {
	   status = csrHoConfigParams(hHal, &pSmeHoConfigParams->csrHoConfig);
   }
   else
   {
	   status = csrHoConfigParams(hHal, NULL);
   }


   if ( ! HAL_STATUS_SUCCESS( status ) ) {
      smsLog( pMac, LOGE, "csrHoConfigParams failed with status=%d\n",
              status );
   }

   return status;

}
#endif
/* ---------------------------------------------------------------------------
    \fn sme_ChangeConfigParams
    \brief The SME API exposed for HDD to provide config params to SME during
    SMEs stop -> start sequence.

    If HDD changed the domain that will cause a reset. This function will
    provide the new set of 11d information for the new domain. Currrently this
    API provides info regarding 11d only at reset but we can extend this for
    other params (PMC, QoS) which needs to be initialized again at reset.

    This is a synchronuous call

    \param hHal - The handle returned by macOpen.

    \Param
    pUpdateConfigParam - a pointer to a structure (tCsrUpdateConfigParam) that
                currently provides 11d related information like Country code,
                Regulatory domain, valid channel list, Tx power per channel, a
                list with active/passive scan allowed per valid channel.

    \return eHalStatus
  ---------------------------------------------------------------------------*/
eHalStatus sme_ChangeConfigParams(tHalHandle hHal,
                                 tCsrUpdateConfigParam *pUpdateConfigParam)
{
   eHalStatus status = eHAL_STATUS_FAILURE;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   if (NULL == pUpdateConfigParam ) {
      smsLog( pMac, LOGE, "Empty config param structure for SME, nothing to \
            reset\n");
      return status;
   }

   status = csrChangeConfigParams(pMac, pUpdateConfigParam);

   if ( ! HAL_STATUS_SUCCESS( status ) ) {
      smsLog( pMac, LOGE, "csrUpdateConfigParam failed with status=%d\n",
              status );
   }

   return status;

}

/*--------------------------------------------------------------------------

  \brief sme_HDDReadyInd() - SME sends eWNI_SME_SYS_READY_IND to PE to inform
  that the NIC is ready tio run.

  The function is called by HDD at the end of initialization stage so PE/HAL can
  enable the NIC to running state.

  This is a synchronuous call
  \param hHal - The handle returned by macOpen.

  \return eHAL_STATUS_SUCCESS - eWNI_SME_SYS_READY_IND is sent to PE
                                successfully.

          Other status means SME failed to send the message to PE.
  \sa

  --------------------------------------------------------------------------*/
eHalStatus sme_HDDReadyInd(tHalHandle hHal)
{
   tSirSmeReadyReq Msg;
   eHalStatus status = eHAL_STATUS_FAILURE;
   tPmcPowerState powerState;
   tPmcSwitchState hwWlanSwitchState;
   tPmcSwitchState swWlanSwitchState;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   do
   {
      csrSetGlobalCfgs( pMac );
      status = csrInitChannelList( pMac );
      if ( ! HAL_STATUS_SUCCESS( status ) )
      {
         smsLog( pMac, LOGE, "csrInitChannelList failed during sme_HDDReadyInd with status=%d\n",
                 status );
         break;
      }

   Msg.messageType = eWNI_SME_SYS_READY_IND;
   Msg.length      = sizeof( tSirSmeReadyReq );

      if (eSIR_FAILURE != halMmhForwardMBmsg( hHal, (tSirMbMsg*)&Msg ))
      {
      status = eHAL_STATUS_SUCCESS;
      }
      else
      {
      smsLog( pMac, LOGE, "halMmhForwardMBmsg failed to send \
            eWNI_SME_SYS_READY_IND\n");
         break;
   }

   status = pmcQueryPowerState( hHal, &powerState,
                                &hwWlanSwitchState, &swWlanSwitchState );
      if ( ! HAL_STATUS_SUCCESS( status ) )
      {
      smsLog( pMac, LOGE, "pmcQueryPowerState failed with status=%d\n",
              status );
         break;
   }

   if ( (ePMC_SWITCH_OFF != hwWlanSwitchState) &&
        (ePMC_SWITCH_OFF != swWlanSwitchState) )
   {
      status = csrReady(pMac);
         if ( ! HAL_STATUS_SUCCESS( status ) )
         {
            smsLog( pMac, LOGE, "csrReady failed with status=%d\n", status );
            break;
         }
         status = pmcReady(hHal);
         if ( ! HAL_STATUS_SUCCESS( status ) )
         {
             smsLog( pMac, LOGE, "pmcReady failed with status=%d\n", status );
             break;
      }
#ifndef WLAN_MDM_CODE_REDUCTION_OPT
         if(VOS_STATUS_SUCCESS != btcReady(hHal))
         {
             status = eHAL_STATUS_FAILURE;
             smsLog( pMac, LOGE, "btcReady failed\n");
             break;
   }
#endif

#if defined WLAN_FEATURE_VOWIFI
         if(VOS_STATUS_SUCCESS != rrmReady(hHal))
         {
             status = eHAL_STATUS_FAILURE;
             smsLog( pMac, LOGE, "rrmReady failed\n");
             break;
   }
#endif
   }
      pMac->sme.state = SME_STATE_READY;
   } while( 0 );

   return status;
}

/*--------------------------------------------------------------------------

  \brief sme_Start() - Put all SME modules at ready state.

  The function starts each module in SME, PMC, CCM, CSR, etc. . Upon
  successfully return, all modules are ready to run.
  This is a synchronuous call
  \param hHal - The handle returned by macOpen.

  \return eHAL_STATUS_SUCCESS - SME is ready.

          Other status means SME is failed to start
  \sa

  --------------------------------------------------------------------------*/
eHalStatus sme_Start(tHalHandle hHal)
{
   eHalStatus status = eHAL_STATUS_FAILURE;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   do
   {
      ccmStart(hHal);

      status = csrStart(pMac);
      if ( ! HAL_STATUS_SUCCESS( status ) ) {
         smsLog( pMac, LOGP, "csrStart failed during smeStart with status=%d\n",
                 status );
         break;
      }

      status = pmcStart(hHal);
      if ( ! HAL_STATUS_SUCCESS( status ) ) {
         smsLog( pMac, LOGP, "pmcStart failed during smeStart with status=%d\n",
                 status );
         break;
      }

#ifdef WLAN_SOFTAP_FEATURE
//      if(VOS_STA_SAP_MODE == vos_get_conparam ( )){
         status = WLANSAP_Start(vos_get_global_context(VOS_MODULE_ID_SAP, NULL));
         if ( ! HAL_STATUS_SUCCESS( status ) ) {
           smsLog( pMac, LOGP, "WLANSAP_Start failed during smeStart with status=%d\n",
                 status );
		   break;
        }
//     }
#endif
      pMac->sme.state = SME_STATE_START;
   }while (0);

   return status;
}


/*--------------------------------------------------------------------------

  \brief sme_ProcessMsg() - The main message processor for SME.

  The function is called by a message dispatcher when to process a message
  targeted for SME.

  This is a synchronuous call
  \param hHal - The handle returned by macOpen.
  \param pMsg - A pointer to a caller allocated object of tSirMsgQ.

  \return eHAL_STATUS_SUCCESS - SME successfully process the message.

          Other status means SME failed to process the message to HAL.
  \sa

  --------------------------------------------------------------------------*/
eHalStatus sme_ProcessMsg(tHalHandle hHal, vos_msg_t* pMsg)
{
   eHalStatus status = eHAL_STATUS_FAILURE;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   if (pMsg == NULL) {
      smsLog( pMac, LOGE, "Empty message for SME, nothing to process\n");
      return status;
   }

   status = sme_AcquireGlobalLock( &pMac->sme );
   if ( HAL_STATUS_SUCCESS( status ) )
   {
      if( SME_IS_START(pMac) )
      {
          switch (pMsg->type) { // TODO: Will be modified to do a range check for msgs instead of having cases for each msgs
          case eWNI_PMC_ENTER_BMPS_RSP:
          case eWNI_PMC_EXIT_BMPS_RSP:
          case eWNI_PMC_EXIT_BMPS_IND:
          case eWNI_PMC_ENTER_IMPS_RSP:
          case eWNI_PMC_EXIT_IMPS_RSP:
          case eWNI_PMC_SMPS_STATE_IND:
          case eWNI_PMC_ENTER_UAPSD_RSP:
          case eWNI_PMC_EXIT_UAPSD_RSP:
	      case eWNI_PMC_ENTER_WOWL_RSP:
	      case eWNI_PMC_EXIT_WOWL_RSP:
             //PMC
             if (pMsg->bodyptr)
             {
                pmcMessageProcessor(hHal, pMsg->bodyptr);
                status = eHAL_STATUS_SUCCESS;
                vos_mem_free( pMsg->bodyptr );
             } else {
                smsLog( pMac, LOGE, "Empty rsp message for PMC, nothing to process\n");
             }
             break;

          case WNI_CFG_SET_CNF:
          case WNI_CFG_DNLD_CNF:
          case WNI_CFG_GET_RSP:
          case WNI_CFG_ADD_GRP_ADDR_CNF:
          case WNI_CFG_DEL_GRP_ADDR_CNF:
             //CCM
             if (pMsg->bodyptr)
             {
                ccmCfgCnfMsgHandler(hHal, pMsg->bodyptr);
                status = eHAL_STATUS_SUCCESS;
                vos_mem_free( pMsg->bodyptr );
             } else {
                smsLog( pMac, LOGE, "Empty rsp message for CCM, nothing to process\n");
             }
             break;

          case eWNI_SME_ADDTS_RSP:
          case eWNI_SME_DELTS_RSP:
          case eWNI_SME_DELTS_IND:
#ifdef WLAN_FEATURE_VOWIFI_11R
          case eWNI_SME_FT_AGGR_QOS_RSP:
#endif             
             //QoS
             if (pMsg->bodyptr)
             {
#ifndef WLAN_MDM_CODE_REDUCTION_OPT
                status = sme_QosMsgProcessor(pMac, pMsg->type, pMsg->bodyptr);
                vos_mem_free( pMsg->bodyptr );
#endif
             } else {
                smsLog( pMac, LOGE, "Empty rsp message for QoS, nothing to process\n");
             }
             break;
#if defined WLAN_FEATURE_VOWIFI
          case eWNI_SME_NEIGHBOR_REPORT_IND:
          case eWNI_SME_BEACON_REPORT_REQ_IND:
#if defined WLAN_VOWIFI_DEBUG
             smsLog( pMac, LOGE, "Recieved RRM message. Message Id = %d\n", pMsg->type );
#endif
             if ( pMsg->bodyptr )
             {
                status = sme_RrmMsgProcessor( pMac, pMsg->type, pMsg->bodyptr );
                vos_mem_free( pMsg->bodyptr );
             }
             else
             {
                smsLog( pMac, LOGE, "Empty message for RRM, nothing to process\n");
             }
             break;
#endif

#ifdef FEATURE_INNAV_SUPPORT
          //Handle the eWNI_SME_INNAV_MEAS_RSP:
          case eWNI_SME_INNAV_MEAS_RSP:
                if(pMsg->bodyptr)
                {
                        status = sme_MeasHandleInNavMeasRsp(pMac, pMsg->bodyptr);
                        vos_mem_free(pMsg->bodyptr);
                }
                else
                {
                        smsLog( pMac, LOGE, "Empty rsp message for meas (eWNI_SME_INNAV_MEAS_RSP), nothing to process\n");
                }
                smeProcessPendingQueue( pMac );
                break;
#endif

          case eWNI_SME_ADD_STA_SELF_RSP:
                if(pMsg->bodyptr)
                {
                   status = csrProcessAddStaSessionRsp(pMac, pMsg->bodyptr);
                   vos_mem_free(pMsg->bodyptr);
                }
                else
                {
                   smsLog( pMac, LOGE, "Empty rsp message for meas (eWNI_SME_ADD_STA_SELF_RSP), nothing to process\n");
                }
                break;
          case eWNI_SME_DEL_STA_SELF_RSP:
                if(pMsg->bodyptr)
                {
                   status = csrProcessDelStaSessionRsp(pMac, pMsg->bodyptr);
                   vos_mem_free(pMsg->bodyptr);
                }
                else
                {
                   smsLog( pMac, LOGE, "Empty rsp message for meas (eWNI_SME_DEL_STA_SELF_RSP), nothing to process\n");
                }
                break;
#ifdef WLAN_FEATURE_P2P
          //Handle the eWNI_SME_INNAV_MEAS_RSP:
          case eWNI_SME_REMAIN_ON_CHN_RSP:
                if(pMsg->bodyptr)
                {
                        status = sme_remainOnChnRsp(pMac, pMsg->bodyptr);
                        vos_mem_free(pMsg->bodyptr);
                }
                else
                {
                        smsLog( pMac, LOGE, "Empty rsp message for meas (eWNI_SME_REMAIN_ON_CHN_RSP), nothing to process\n");
                }
                break;
          case eWNI_SME_REMAIN_ON_CHN_RDY_IND:
                if(pMsg->bodyptr)
                {
                        status = sme_remainOnChnReady(pMac, pMsg->bodyptr);
                        vos_mem_free(pMsg->bodyptr);
                }
                else
                {
                        smsLog( pMac, LOGE, "Empty rsp message for meas (eWNI_SME_REMAIN_ON_CHN_RDY_IND), nothing to process\n");
                }
                break;
           case eWNI_SME_MGMT_FRM_IND:
                if(pMsg->bodyptr)
                {
                  sme_mgmtFrmInd(pMac, pMsg->bodyptr);
                  vos_mem_free(pMsg->bodyptr);
                }
                else
                { 
                     smsLog( pMac, LOGE, "Empty rsp message for meas (eWNI_SME_MGMT_FRM_IND), nothing to process\n");
                }
                break;
           case eWNI_SME_ACTION_FRAME_SEND_CNF:
                if(pMsg->bodyptr)
                {
                        status = sme_sendActionCnf(pMac, pMsg->bodyptr);
                        vos_mem_free(pMsg->bodyptr);
                }
                else
                {
                        smsLog( pMac, LOGE, "Empty rsp message for meas (eWNI_SME_ACTION_FRAME_SEND_CNF), nothing to process\n");
                }
                break;
#endif
					 
          default:

             if ( ( pMsg->type >= eWNI_SME_MSG_TYPES_BEGIN )
                  &&  ( pMsg->type <= eWNI_SME_MSG_TYPES_END ) )
             {
                //CSR
                if (pMsg->bodyptr)
                {
                   status = csrMsgProcessor(hHal, pMsg->bodyptr);
                   vos_mem_free( pMsg->bodyptr );
                }
                else
                {
                   smsLog( pMac, LOGE, "Empty rsp message for CSR, nothing to process\n");
                }
             }
             else
             {
                smsLog( pMac, LOGW, "Unknown message type %d, nothing to process\n",
                        pMsg->type);
                if (pMsg->bodyptr)
                {
                   vos_mem_free( pMsg->bodyptr );
             }
             }
          }//switch
      } //SME_IS_START
      else
      {
         smsLog( pMac, LOGW, "message type %d in stop state ignored\n", pMsg->type);
         if (pMsg->bodyptr)
         {
            vos_mem_free( pMsg->bodyptr );
         }
      }
      sme_ReleaseGlobalLock( &pMac->sme );
   }
   else
   {
      smsLog( pMac, LOGW, "Locking failed, bailing out\n");
      if (pMsg->bodyptr)
      {
          vos_mem_free( pMsg->bodyptr );
      }
   }

   return status;
}


//No need to hold the global lock here because this function can only be called
//after sme_Stop.
v_VOID_t sme_FreeMsg( tHalHandle hHal, vos_msg_t* pMsg )
{
   if( pMsg )
   {
      if (pMsg->bodyptr)
      {
         vos_mem_free( pMsg->bodyptr );
      }
   }

}


/*--------------------------------------------------------------------------

  \brief sme_Stop() - Stop all SME modules and put them at idle state

  The function stops each module in SME, PMC, CCM, CSR, etc. . Upon
  return, all modules are at idle state ready to start.

  This is a synchronuous call
  \param hHal - The handle returned by macOpen

  \return eHAL_STATUS_SUCCESS - SME is stopped.

          Other status means SME is failed to stop but caller should still
          consider SME is stopped.
  \sa

  --------------------------------------------------------------------------*/
eHalStatus sme_Stop(tHalHandle hHal, tANI_BOOLEAN pmcFlag)
{
   eHalStatus status = eHAL_STATUS_FAILURE;
   eHalStatus fail_status = eHAL_STATUS_SUCCESS;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

#ifdef WLAN_SOFTAP_FEATURE
   status = WLANSAP_Stop(vos_get_global_context(VOS_MODULE_ID_SAP, NULL));
   if ( ! HAL_STATUS_SUCCESS( status ) ) {
      smsLog( pMac, LOGE, "WLANSAP_Stop failed during smeStop with status=%d\n",
                          status );
      fail_status = status;
   }
#endif

   if(pmcFlag)
   {
      status = pmcStop(hHal);
      if ( ! HAL_STATUS_SUCCESS( status ) ) {
         smsLog( pMac, LOGE, "pmcStop failed during smeStop with status=%d\n",
                 status );
         fail_status = status;
      }
   }

   status = csrStop(pMac);
   if ( ! HAL_STATUS_SUCCESS( status ) ) {
      smsLog( pMac, LOGE, "csrStop failed during smeStop with status=%d\n",
              status );
      fail_status = status;
   }

   ccmStop(hHal);

   purgeSmeCmdList(pMac);

   if (!HAL_STATUS_SUCCESS( fail_status )) {
      status = fail_status;
   }

   pMac->sme.state = SME_STATE_STOP;

   return status;
}

/*--------------------------------------------------------------------------

  \brief sme_Close() - Release all SME modules and their resources.

  The function release each module in SME, PMC, CCM, CSR, etc. . Upon
  return, all modules are at closed state.

  No SME APIs can be involved after smeClose except smeOpen.
  smeClose must be called before macClose.
  This is a synchronuous call
  \param hHal - The handle returned by macOpen

  \return eHAL_STATUS_SUCCESS - SME is successfully close.

          Other status means SME is failed to be closed but caller still cannot
          call any other SME functions except smeOpen.
  \sa

  --------------------------------------------------------------------------*/
eHalStatus sme_Close(tHalHandle hHal)
{
   eHalStatus status = eHAL_STATUS_FAILURE;
   eHalStatus fail_status = eHAL_STATUS_SUCCESS;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   status = csrClose(pMac);
   if ( ! HAL_STATUS_SUCCESS( status ) ) {
      smsLog( pMac, LOGE, "csrClose failed during sme close with status=%d\n",
              status );
      fail_status = status;
   }

#ifdef WLAN_SOFTAP_FEATURE
//   if(VOS_STA_SAP_MODE == vos_get_conparam ( )){
         status = WLANSAP_Close(vos_get_global_context(VOS_MODULE_ID_SAP, NULL));
         if ( ! HAL_STATUS_SUCCESS( status ) ) {
             smsLog( pMac, LOGE, "WLANSAP_close failed during sme close with status=%d\n",
                 status );
             fail_status = status;
         }
//   }
#endif

#ifndef WLAN_MDM_CODE_REDUCTION_OPT
   status = btcClose(hHal);
   if ( ! HAL_STATUS_SUCCESS( status ) ) {
      smsLog( pMac, LOGE, "BTC close failed during sme close with status=%d\n",
              status );
      fail_status = status;
   }

   status = sme_QosClose(pMac);
   if ( ! HAL_STATUS_SUCCESS( status ) ) {
      smsLog( pMac, LOGE, "Qos close failed during sme close with status=%d\n",
              status );
      fail_status = status;
   }
#endif
#ifdef FEATURE_INNAV_SUPPORT
   status = measInNavClose(hHal);
   if ( ! HAL_STATUS_SUCCESS( status ) ) {
       smsLog( pMac, LOGE, "INNAV close failed during sme close with status=%d\n",
              status );
      fail_status = status;
   }
#endif

   status = ccmClose(hHal);
         if ( ! HAL_STATUS_SUCCESS( status ) ) {
      smsLog( pMac, LOGE, "ccmClose failed during sme close with status=%d\n",
                 status );
             fail_status = status;
         }

   status = pmcClose(hHal);
   if ( ! HAL_STATUS_SUCCESS( status ) ) {
      smsLog( pMac, LOGE, "pmcClose failed during sme close with status=%d\n",
              status );
      fail_status = status;
   }

#if defined WLAN_FEATURE_VOWIFI
   status = rrmClose(hHal);
   if ( ! HAL_STATUS_SUCCESS( status ) ) {
      smsLog( pMac, LOGE, "RRM close failed during sme close with status=%d\n",
              status );
      fail_status = status;
   }
#endif

#if defined WLAN_FEATURE_VOWIFI_11R
   sme_FTClose(hHal);
#endif
#if defined WLAN_FEATURE_P2P
   sme_p2pClose(hHal);
#endif

   freeSmeCmdList(pMac);

   if( !VOS_IS_STATUS_SUCCESS( vos_lock_destroy( &pMac->sme.lkSmeGlobalLock ) ) )
   {
       fail_status = eHAL_STATUS_FAILURE;
   }

   if (!HAL_STATUS_SUCCESS( fail_status )) {
      status = fail_status;
   }

   pMac->sme.state = SME_STATE_STOP;

   return status;
}

/* ---------------------------------------------------------------------------
    \fn sme_ScanRequest
    \brief a wrapper function to Request a 11d or full scan from CSR.
    This is an asynchronuous call
    \param pScanRequestID - pointer to an object to get back the request ID
    \param callback - a callback function that scan calls upon finish, will not
                      be called if csrScanRequest returns error
    \param pContext - a pointer passed in for the callback
    \return eHalStatus
  ---------------------------------------------------------------------------*/
eHalStatus sme_ScanRequest(tHalHandle hHal, tANI_U8 sessionId, tCsrScanRequest *pscanReq,
                           tANI_U32 *pScanRequestID, 
                           csrScanCompleteCallback callback, void *pContext)
{
    eHalStatus status = eHAL_STATUS_FAILURE;
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

    smsLog(pMac, LOGE, FL("   enter \n"));
    do
    {
    if(pMac->scan.fScanEnable)
    {
        status = sme_AcquireGlobalLock( &pMac->sme );
        if ( HAL_STATUS_SUCCESS( status ) )
        {
#ifdef FEATURE_WLAN_GEN6_ROAMING
            //Since HO only happens for infra mode, we only check for the
            //connect status of infra link.
            //This way is to save off channel time so AP has less chance of
            //deauth the Libra side when Libra doens't response to many TIM-ed beacons.
            if( csrIsConnStateConnectedInfra( pMac, sessionId ) &&
                (csrScanIsBgScanEnabled( pMac ) || csrScanGetChannelMask(pMac)) )
            {
                //background scan by HO is enable, no need to scan here
                if( callback )
                {
                    tANI_U32 lScanId = pMac->scan.nextScanID++; //let it wrap around
                    //Assign a scanID in case caller uses it during the callback.
                    if(pScanRequestID)
                    {
                        *pScanRequestID = lScanId;
                    }
                    sme_ReleaseGlobalLock( &pMac->sme );
                    callback( pMac, pContext, lScanId, eCSR_SCAN_ONGOING );
                    status = sme_AcquireGlobalLock( &pMac->sme );
                    if ( !HAL_STATUS_SUCCESS( status ) )
                    {
                        status = eHAL_STATUS_SUCCESS;
                        break;
                    }
	        }
                }
            else
#endif //#ifdef FEATURE_WLAN_GEN6_ROAMING
            {
#ifdef FEATURE_WLAN_GEN6_ROAMING
                    //create the channel mask if needed
                    if( csrIsConnStateConnectedInfra( pMac, sessionId ) &&
                       (CSR_IS_ROAM_SUBSTATE_HO_NRT(pMac) || CSR_IS_ROAM_SUBSTATE_HO_RT(pMac)))
                    {
                        csrScanSetChannelMask(pMac, &pscanReq->ChannelInfo);
                    }
#endif //#ifdef FEATURE_WLAN_GEN6_ROAMING
                    status = csrScanRequest( hHal, sessionId, pscanReq,
                                     pScanRequestID, callback, pContext );
                }
                  
                sme_ReleaseGlobalLock( &pMac->sme );
            } //sme_AcquireGlobalLock success
        } //if(pMac->scan.fScanEnable)
    } while( 0 );

    return (status);

}

/* ---------------------------------------------------------------------------
    \fn sme_ScanGetResult
    \brief a wrapper function to request scan results from CSR.
    This is a synchronuous call
    \param pFilter - If pFilter is NULL, all cached results are returned
    \param phResult - an object for the result.
    \return eHalStatus
  ---------------------------------------------------------------------------*/
eHalStatus sme_ScanGetResult(tHalHandle hHal, tANI_U8 sessionId, tCsrScanResultFilter *pFilter,
                            tScanResultHandle *phResult)
{
   eHalStatus status = eHAL_STATUS_FAILURE;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   smsLog(pMac, LOGE, FL("   enter \n"));
   status = sme_AcquireGlobalLock( &pMac->sme );
   if ( HAL_STATUS_SUCCESS( status ) )
   {
       status = csrScanGetResult( hHal, pFilter, phResult );
       sme_ReleaseGlobalLock( &pMac->sme );
   }
   smsLog(pMac, LOGE, FL("   exit status %d \n"), status);

   return (status);
}


/* ---------------------------------------------------------------------------
    \fn sme_ScanFlushResult
    \brief a wrapper function to request CSR to clear scan results.
    This is a synchronuous call
    \return eHalStatus
  ---------------------------------------------------------------------------*/
eHalStatus sme_ScanFlushResult(tHalHandle hHal, tANI_U8 sessionId)
{
   eHalStatus status = eHAL_STATUS_FAILURE;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   status = sme_AcquireGlobalLock( &pMac->sme );
   if ( HAL_STATUS_SUCCESS( status ) )
   {
       status = csrScanFlushResult( hHal );
       sme_ReleaseGlobalLock( &pMac->sme );
   }

   return (status);
}


/* ---------------------------------------------------------------------------
    \fn sme_ScanResultGetFirst
    \brief a wrapper function to request CSR to returns the first element of
           scan result.
    This is a synchronuous call
    \param hScanResult - returned from csrScanGetResult
    \return tCsrScanResultInfo * - NULL if no result
  ---------------------------------------------------------------------------*/
tCsrScanResultInfo *sme_ScanResultGetFirst(tHalHandle hHal,
                                          tScanResultHandle hScanResult)
{
   eHalStatus status = eHAL_STATUS_FAILURE;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );
   tCsrScanResultInfo *pRet = NULL;

   status = sme_AcquireGlobalLock( &pMac->sme );
   if ( HAL_STATUS_SUCCESS( status ) )
   {
       pRet = csrScanResultGetFirst( pMac, hScanResult );
       sme_ReleaseGlobalLock( &pMac->sme );
   }

   return (pRet);
}


/* ---------------------------------------------------------------------------
    \fn sme_ScanResultGetNext
    \brief a wrapper function to request CSR to returns the next element of
           scan result. It can be called without calling csrScanResultGetFirst
           first
    This is a synchronuous call
    \param hScanResult - returned from csrScanGetResult
    \return Null if no result or reach the end
  ---------------------------------------------------------------------------*/
tCsrScanResultInfo *sme_ScanResultGetNext(tHalHandle hHal,
                                          tScanResultHandle hScanResult)
{
    eHalStatus status = eHAL_STATUS_FAILURE;
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );
    tCsrScanResultInfo *pRet = NULL;

    status = sme_AcquireGlobalLock( &pMac->sme );
    if ( HAL_STATUS_SUCCESS( status ) )
    {
        pRet = csrScanResultGetNext( pMac, hScanResult );
        sme_ReleaseGlobalLock( &pMac->sme );
    }

    return (pRet);
}


/* ---------------------------------------------------------------------------
    \fn sme_ScanSetBGScanparams
    \brief a wrapper function to request CSR to set BG scan params in PE
    This is a synchronuous call
    \param pScanReq - BG scan request structure
    \return eHalStatus
  ---------------------------------------------------------------------------*/
eHalStatus sme_ScanSetBGScanparams(tHalHandle hHal, tANI_U8 sessionId, tCsrBGScanRequest *pScanReq)
{
    eHalStatus status = eHAL_STATUS_FAILURE;
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

    if( NULL != pScanReq )
    {
        status = sme_AcquireGlobalLock( &pMac->sme );
        if ( HAL_STATUS_SUCCESS( status ) )
        {
            status = csrScanSetBGScanparams( hHal, pScanReq );
            sme_ReleaseGlobalLock( &pMac->sme );
        }
    }

    return (status);
}


/* ---------------------------------------------------------------------------
    \fn sme_ScanResultPurge
    \brief a wrapper function to request CSR to remove all items(tCsrScanResult)
           in the list and free memory for each item
    This is a synchronuous call
    \param hScanResult - returned from csrScanGetResult. hScanResult is
                         considered gone by
    calling this function and even before this function reutrns.
    \return eHalStatus
  ---------------------------------------------------------------------------*/
eHalStatus sme_ScanResultPurge(tHalHandle hHal, tScanResultHandle hScanResult)
{
   eHalStatus status = eHAL_STATUS_FAILURE;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   status = sme_AcquireGlobalLock( &pMac->sme );
   if ( HAL_STATUS_SUCCESS( status ) )
   {
       status = csrScanResultPurge( hHal, hScanResult );
       sme_ReleaseGlobalLock( &pMac->sme );
   }

   return (status);
}

/* ---------------------------------------------------------------------------
    \fn sme_ScanGetPMKIDCandidateList
    \brief a wrapper function to return the PMKID candidate list
    This is a synchronuous call
    \param pPmkidList - caller allocated buffer point to an array of
                        tPmkidCandidateInfo
    \param pNumItems - pointer to a variable that has the number of
                       tPmkidCandidateInfo allocated when retruning, this is
                       either the number needed or number of items put into
                       pPmkidList
    \return eHalStatus - when fail, it usually means the buffer allocated is not
                         big enough and pNumItems
    has the number of tPmkidCandidateInfo.
    \Note: pNumItems is a number of tPmkidCandidateInfo,
           not sizeof(tPmkidCandidateInfo) * something
  ---------------------------------------------------------------------------*/
eHalStatus sme_ScanGetPMKIDCandidateList(tHalHandle hHal, tANI_U8 sessionId,
                                        tPmkidCandidateInfo *pPmkidList, 
                                        tANI_U32 *pNumItems )
{
   eHalStatus status = eHAL_STATUS_FAILURE;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   status = sme_AcquireGlobalLock( &pMac->sme );
   if ( HAL_STATUS_SUCCESS( status ) )
   {
       status = csrScanGetPMKIDCandidateList( pMac, sessionId, pPmkidList, pNumItems );
       sme_ReleaseGlobalLock( &pMac->sme );
   }

   return (status);
}

/*----------------------------------------------------------------------------
  \fn sme_RoamRegisterLinkQualityIndCallback

  \brief
  a wrapper function to allow HDD to register a callback handler with CSR for
  link quality indications.

  Only one callback may be registered at any time.
  In order to deregister the callback, a NULL cback may be provided.

  Registration happens in the task context of the caller.

  \param callback - Call back being registered
  \param pContext - user data

  DEPENDENCIES: After CSR open

  \return eHalStatus
-----------------------------------------------------------------------------*/
eHalStatus sme_RoamRegisterLinkQualityIndCallback(tHalHandle hHal, tANI_U8 sessionId,
                                                  csrRoamLinkQualityIndCallback   callback,
                                                  void                           *pContext)
{
   return(csrRoamRegisterLinkQualityIndCallback((tpAniSirGlobal)hHal, callback, pContext));
}

/* ---------------------------------------------------------------------------
    \fn sme_RoamRegisterCallback
    \brief a wrapper function to allow HDD to register a callback with CSR.
           Unlike scan, roam has one callback for all the roam requests
    \param callback - a callback function that roam calls upon when state changes
    \param pContext - a pointer passed in for the callback
    \return eHalStatus
  ---------------------------------------------------------------------------*/
eHalStatus sme_RoamRegisterCallback(tHalHandle hHal,
                                    csrRoamCompleteCallback callback,
                                    void *pContext)
{
   return(csrRoamRegisterCallback((tpAniSirGlobal)hHal, callback, pContext));
}

eCsrPhyMode sme_GetPhyMode(tHalHandle hHal)
{
    tpAniSirGlobal pMac = PMAC_STRUCT(hHal);
    return pMac->roam.configParam.phyMode;
}

/* ---------------------------------------------------------------------------
    \fn sme_RoamConnect
    \brief a wrapper function to request CSR to inititiate an association
    This is an asynchronuous call.
    \param sessionId - the sessionId returned by sme_OpenSession.
    \param pProfile - can be NULL to join to any open ones
    \param hBssListIn - a list of BSS descriptor to roam to. It is returned
                        from csrScanGetResult
    \param pRoamId - to get back the request ID
    \return eHalStatus
  ---------------------------------------------------------------------------*/
eHalStatus sme_RoamConnect(tHalHandle hHal, tANI_U8 sessionId, tCsrRoamProfile *pProfile,
                           tANI_U32 *pRoamId)
{
    eHalStatus status = eHAL_STATUS_FAILURE;
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

    smsLog(pMac, LOGE, FL("   enter \n"));
    status = sme_AcquireGlobalLock( &pMac->sme );
    if ( HAL_STATUS_SUCCESS( status ) )
    {
        if( CSR_IS_SESSION_VALID( pMac, sessionId ) )
        {
            status = csrRoamConnect( pMac, sessionId, pProfile, NULL, pRoamId );
        }
        else
        {
            status = eHAL_STATUS_INVALID_PARAMETER;
        }
        sme_ReleaseGlobalLock( &pMac->sme );
    }

    return (status);
}

/* ---------------------------------------------------------------------------
    \fn sme_RoamReassoc
    \brief a wrapper function to request CSR to inititiate a re-association
    \param pProfile - can be NULL to join the currently connected AP. In that
    case modProfileFields should carry the modified field(s) which could trigger
    reassoc
    \param modProfileFields - fields which are part of tCsrRoamConnectedProfile
    that might need modification dynamically once STA is up & running and this
    could trigger a reassoc
    \param pRoamId - to get back the request ID
    \return eHalStatus
  -------------------------------------------------------------------------------*/
eHalStatus sme_RoamReassoc(tHalHandle hHal, tANI_U8 sessionId, tCsrRoamProfile *pProfile,
                          tCsrRoamModifyProfileFields modProfileFields,
                          tANI_U32 *pRoamId)
{
    eHalStatus status = eHAL_STATUS_FAILURE;
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

    smsLog(pMac, LOGE, FL("   enter \n"));
    status = sme_AcquireGlobalLock( &pMac->sme );
    if ( HAL_STATUS_SUCCESS( status ) )
    {
        if( CSR_IS_SESSION_VALID( pMac, sessionId ) )
        {
            status = csrRoamReassoc( pMac, sessionId, pProfile, modProfileFields, pRoamId );
        }
        else
        {
            status = eHAL_STATUS_INVALID_PARAMETER;
        }
        sme_ReleaseGlobalLock( &pMac->sme );
    }

    return (status);
}

/* ---------------------------------------------------------------------------
    \fn sme_RoamConnectToLastProfile
    \brief a wrapper function to request CSR to disconnect and reconnect with
           the same profile
    This is an asynchronuous call.
    \return eHalStatus. It returns fail if currently connected
  ---------------------------------------------------------------------------*/
eHalStatus sme_RoamConnectToLastProfile(tHalHandle hHal, tANI_U8 sessionId)
{
   eHalStatus status = eHAL_STATUS_FAILURE;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   status = sme_AcquireGlobalLock( &pMac->sme );
   if ( HAL_STATUS_SUCCESS( status ) )
   {
      if( CSR_IS_SESSION_VALID( pMac, sessionId ) )
      {
         status = csrRoamConnectToLastProfile( pMac, sessionId );
      }
      else
      {
          status = eHAL_STATUS_INVALID_PARAMETER;
      }
      sme_ReleaseGlobalLock( &pMac->sme );
   }

   return (status);
}

/* ---------------------------------------------------------------------------
    \fn sme_RoamDisconnect
    \brief a wrapper function to request CSR to disconnect from a network
    This is an asynchronuous call.
    \param reason -- To indicate the reason for disconnecting. Currently, only
                     eCSR_DISCONNECT_REASON_MIC_ERROR is meanful.
    \return eHalStatus
  ---------------------------------------------------------------------------*/
eHalStatus sme_RoamDisconnect(tHalHandle hHal, tANI_U8 sessionId, eCsrRoamDisconnectReason reason)
{
   eHalStatus status = eHAL_STATUS_FAILURE;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   smsLog(pMac, LOGE, FL("   enter \n"));
   status = sme_AcquireGlobalLock( &pMac->sme );
   if ( HAL_STATUS_SUCCESS( status ) )
   {
      if( CSR_IS_SESSION_VALID( pMac, sessionId ) )
      {
         status = csrRoamDisconnect( pMac, sessionId, reason );
      }
      else
      {
          status = eHAL_STATUS_INVALID_PARAMETER;
      }
      sme_ReleaseGlobalLock( &pMac->sme );
   }

   return (status);
}

#ifdef WLAN_SOFTAP_FEATURE
/* ---------------------------------------------------------------------------
    \fn sme_RoamStopBss
    \brief To stop BSS for Soft AP. This is an asynchronous API.
    \param hHal - Global structure
    \param sessionId - sessionId of SoftAP
    \return eHalStatus  SUCCESS  Roam callback will be called to indicate actual results
  -------------------------------------------------------------------------------*/
eHalStatus sme_RoamStopBss(tHalHandle hHal, tANI_U8 sessionId)
{
   eHalStatus status = eHAL_STATUS_FAILURE;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   smsLog(pMac, LOGE, FL("   enter \n"));
   status = sme_AcquireGlobalLock( &pMac->sme );
   if ( HAL_STATUS_SUCCESS( status ) )
   {
      if( CSR_IS_SESSION_VALID( pMac, sessionId ) )
      {
         status = csrRoamIssueStopBssCmd( pMac, sessionId, eANI_BOOLEAN_TRUE );
      }
      else
      {
          status = eHAL_STATUS_INVALID_PARAMETER;
      }
      sme_ReleaseGlobalLock( &pMac->sme );
   }

   return (status);
}

/* ---------------------------------------------------------------------------
    \fn sme_RoamDisconnectSta
    \brief To disassociate a station. This is an asynchronous API.
    \param hHal - Global structure
    \param sessionId - sessionId of SoftAP
    \param pPeerMacAddr - Caller allocated memory filled with peer MAC address (6 bytes)
    \return eHalStatus  SUCCESS  Roam callback will be called to indicate actual results
  -------------------------------------------------------------------------------*/
eHalStatus sme_RoamDisconnectSta(tHalHandle hHal, tANI_U8 sessionId,
                                tANI_U8 *pPeerMacAddr)
{
   eHalStatus status = eHAL_STATUS_FAILURE;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   if ( NULL == pMac )
   {
     VOS_ASSERT(0);
     return status;
   }

   status = sme_AcquireGlobalLock( &pMac->sme );
   if ( HAL_STATUS_SUCCESS( status ) )
   {
      if( CSR_IS_SESSION_VALID( pMac, sessionId ) )
      {
         status = csrRoamIssueDisassociateSta( pMac, sessionId, pPeerMacAddr);
      }
      else
      {
         status = eHAL_STATUS_INVALID_PARAMETER;
      }
      sme_ReleaseGlobalLock( &pMac->sme );
   }

   return (status);
}

/* ---------------------------------------------------------------------------
    \fn sme_RoamDeauthSta
    \brief To disassociate a station. This is an asynchronous API.
    \param hHal - Global structure
    \param sessionId - sessionId of SoftAP
    \param pPeerMacAddr - Caller allocated memory filled with peer MAC address (6 bytes)
    \return eHalStatus  SUCCESS  Roam callback will be called to indicate actual results
  -------------------------------------------------------------------------------*/
eHalStatus sme_RoamDeauthSta(tHalHandle hHal, tANI_U8 sessionId,
                                tANI_U8 *pPeerMacAddr)
{
   eHalStatus status = eHAL_STATUS_FAILURE;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   if ( NULL == pMac )
   {
     VOS_ASSERT(0);
     return status;
   }

   status = sme_AcquireGlobalLock( &pMac->sme );
   if ( HAL_STATUS_SUCCESS( status ) )
   {
      if( CSR_IS_SESSION_VALID( pMac, sessionId ) )
      {
         status = csrRoamIssueDeauthSta( pMac, sessionId, pPeerMacAddr);
      }
      else
      {
         status = eHAL_STATUS_INVALID_PARAMETER;
      }
      sme_ReleaseGlobalLock( &pMac->sme );
   }

   return (status);
}

/* ---------------------------------------------------------------------------
    \fn sme_RoamTKIPCounterMeasures
    \brief To start or stop TKIP counter measures. This is an asynchronous API.
    \param sessionId - sessionId of SoftAP
    \param pPeerMacAddr - Caller allocated memory filled with peer MAC address (6 bytes)
    \return eHalStatus
  -------------------------------------------------------------------------------*/
eHalStatus sme_RoamTKIPCounterMeasures(tHalHandle hHal, tANI_U8 sessionId,
                                        tANI_BOOLEAN bEnable)
{
   eHalStatus status = eHAL_STATUS_FAILURE;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   if ( NULL == pMac )
   {
     VOS_ASSERT(0);
     return status;
   }

   status = sme_AcquireGlobalLock( &pMac->sme );
   if ( HAL_STATUS_SUCCESS( status ) )
   {
        if( CSR_IS_SESSION_VALID( pMac, sessionId ) )
        {
            status = csrRoamIssueTkipCounterMeasures( pMac, sessionId, bEnable);
        }
        else
        {
            status = eHAL_STATUS_INVALID_PARAMETER;
        }
        sme_ReleaseGlobalLock( &pMac->sme );
   }

   return (status);
}

/* ---------------------------------------------------------------------------
    \fn sme_RoamGetAssociatedStas
    \brief To probe the list of associated stations from various modules of CORE stack.
    \This is an asynchronous API.
    \param sessionId    - sessionId of SoftAP
    \param modId        - Module from whom list of associtated stations is to be probed.
                          If an invalid module is passed then by default VOS_MODULE_ID_PE will be probed
    \param pUsrContext  - Opaque HDD context
    \param pfnSapEventCallback  - Sap event callback in HDD
    \param pAssocBuf    - Caller allocated memory to be filled with associatd stations info
    \return eHalStatus
  -------------------------------------------------------------------------------*/
eHalStatus sme_RoamGetAssociatedStas(tHalHandle hHal, tANI_U8 sessionId,
                                        VOS_MODULE_ID modId, void *pUsrContext,
                                        void *pfnSapEventCallback, tANI_U8 *pAssocStasBuf)
{
   eHalStatus status = eHAL_STATUS_FAILURE;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   if ( NULL == pMac )
   {
     VOS_ASSERT(0);
     return status;
   }

   status = sme_AcquireGlobalLock( &pMac->sme );
   if ( HAL_STATUS_SUCCESS( status ) )
   {
        if( CSR_IS_SESSION_VALID( pMac, sessionId ) )
        {
            status = csrRoamGetAssociatedStas( pMac, sessionId, modId, pUsrContext, pfnSapEventCallback, pAssocStasBuf );
        }
        else
        {
            status = eHAL_STATUS_INVALID_PARAMETER;
        }
        sme_ReleaseGlobalLock( &pMac->sme );
   }

   return (status);
}

/* ---------------------------------------------------------------------------
    \fn sme_RoamGetWpsSessionOverlap
    \brief To get the WPS PBC session overlap information.
    \This is an asynchronous API.
    \param sessionId    - sessionId of SoftAP
    \param pUsrContext  - Opaque HDD context
    \param pfnSapEventCallback  - Sap event callback in HDD
    \pRemoveMac - pointer to Mac address which needs to be removed from session
    \return eHalStatus
  -------------------------------------------------------------------------------*/
eHalStatus sme_RoamGetWpsSessionOverlap(tHalHandle hHal, tANI_U8 sessionId,
                                        void *pUsrContext, void 
                                        *pfnSapEventCallback, v_MACADDR_t pRemoveMac)
{
   eHalStatus status = eHAL_STATUS_FAILURE;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   if ( NULL == pMac )
   {
     VOS_ASSERT(0);
     return status;
   }

   status = sme_AcquireGlobalLock( &pMac->sme );
   if ( HAL_STATUS_SUCCESS( status ) )
   {
        if( CSR_IS_SESSION_VALID( pMac, sessionId ) )
        {
            status = csrRoamGetWpsSessionOverlap( pMac, sessionId, pUsrContext, pfnSapEventCallback, pRemoveMac);
        }
        else
        {
            status = eHAL_STATUS_INVALID_PARAMETER;
        }
        sme_ReleaseGlobalLock( &pMac->sme );
   }

   return (status);
}

#endif

/* ---------------------------------------------------------------------------
    \fn sme_RoamGetConnectState
    \brief a wrapper function to request CSR to return the current connect state
           of Roaming
    This is a synchronuous call.
    \return eHalStatus
  ---------------------------------------------------------------------------*/
eHalStatus sme_RoamGetConnectState(tHalHandle hHal, tANI_U8 sessionId, eCsrConnectState *pState)
{
    eHalStatus status = eHAL_STATUS_FAILURE;
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

    status = sme_AcquireGlobalLock( &pMac->sme );
    if ( HAL_STATUS_SUCCESS( status ) )
    {
       if( CSR_IS_SESSION_VALID( pMac, sessionId ) )
       {
          status = csrRoamGetConnectState( pMac, sessionId, pState );
       }
       else
       {
           status = eHAL_STATUS_INVALID_PARAMETER;
       }
       sme_ReleaseGlobalLock( &pMac->sme );
    }

    return (status);
}

/* ---------------------------------------------------------------------------
    \fn sme_RoamGetConnectProfile
    \brief a wrapper function to request CSR to return the current connect
           profile. Caller must call csrRoamFreeConnectProfile after it is done
           and before reuse for another csrRoamGetConnectProfile call.
    This is a synchronuous call.
    \param pProfile - pointer to a caller allocated structure
                      tCsrRoamConnectedProfile
    \return eHalStatus. Failure if not connected
  ---------------------------------------------------------------------------*/
eHalStatus sme_RoamGetConnectProfile(tHalHandle hHal, tANI_U8 sessionId,
                                     tCsrRoamConnectedProfile *pProfile)
{
   eHalStatus status = eHAL_STATUS_FAILURE;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   status = sme_AcquireGlobalLock( &pMac->sme );
   if ( HAL_STATUS_SUCCESS( status ) )
   {
      if( CSR_IS_SESSION_VALID( pMac, sessionId ) )
      {
         status = csrRoamGetConnectProfile( pMac, sessionId, pProfile );
      }
      else
      {
          status = eHAL_STATUS_INVALID_PARAMETER;
      }
      sme_ReleaseGlobalLock( &pMac->sme );
   }

   return (status);
}

/* ---------------------------------------------------------------------------
    \fn sme_RoamFreeConnectProfile
    \brief a wrapper function to request CSR to free and reinitialize the
           profile returned previously by csrRoamGetConnectProfile.
    This is a synchronuous call.
    \param pProfile - pointer to a caller allocated structure
                      tCsrRoamConnectedProfile
    \return eHalStatus.
  ---------------------------------------------------------------------------*/
eHalStatus sme_RoamFreeConnectProfile(tHalHandle hHal,
                                      tCsrRoamConnectedProfile *pProfile)
{
   eHalStatus status = eHAL_STATUS_FAILURE;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   status = sme_AcquireGlobalLock( &pMac->sme );
   if ( HAL_STATUS_SUCCESS( status ) )
   {
       status = csrRoamFreeConnectProfile( pMac, pProfile );
       sme_ReleaseGlobalLock( &pMac->sme );
   }

   return (status);
}

/* ---------------------------------------------------------------------------
    \fn sme_RoamSetPMKIDCache
    \brief a wrapper function to request CSR to return the PMKID candidate list
    This is a synchronuous call.
    \param pPMKIDCache - caller allocated buffer point to an array of
                         tPmkidCacheInfo
    \param numItems - a variable that has the number of tPmkidCacheInfo
                      allocated when retruning, this is either the number needed
                      or number of items put into pPMKIDCache
    \return eHalStatus - when fail, it usually means the buffer allocated is not
                         big enough and pNumItems has the number of
                         tPmkidCacheInfo.
    \Note: pNumItems is a number of tPmkidCacheInfo,
           not sizeof(tPmkidCacheInfo) * something
  ---------------------------------------------------------------------------*/
eHalStatus sme_RoamSetPMKIDCache( tHalHandle hHal, tANI_U8 sessionId, tPmkidCacheInfo *pPMKIDCache,
                                  tANI_U32 numItems )
{
   eHalStatus status = eHAL_STATUS_FAILURE;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   status = sme_AcquireGlobalLock( &pMac->sme );
   if ( HAL_STATUS_SUCCESS( status ) )
   {
      if( CSR_IS_SESSION_VALID( pMac, sessionId ) )
      {
         status = csrRoamSetPMKIDCache( pMac, sessionId, pPMKIDCache, numItems );
      }
      else
      {
          status = eHAL_STATUS_INVALID_PARAMETER;
      }
      sme_ReleaseGlobalLock( &pMac->sme );
   }

   return (status);
}

/* ---------------------------------------------------------------------------
    \fn sme_RoamGetSecurityReqIE
    \brief a wrapper function to request CSR to return the WPA or RSN or WAPI IE CSR
           passes to PE to JOIN request or START_BSS request
    This is a synchronuous call.
    \param pLen - caller allocated memory that has the length of pBuf as input.
                  Upon returned, *pLen has the needed or IE length in pBuf.
    \param pBuf - Caller allocated memory that contain the IE field, if any,
                  upon return
    \param secType - Specifies whether looking for WPA/WPA2/WAPI IE
    \return eHalStatus - when fail, it usually means the buffer allocated is not
                         big enough
  ---------------------------------------------------------------------------*/
eHalStatus sme_RoamGetSecurityReqIE(tHalHandle hHal, tANI_U8 sessionId, tANI_U32 *pLen,
                                  tANI_U8 *pBuf, eCsrSecurityType secType)
{
    eHalStatus status = eHAL_STATUS_FAILURE;
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

    status = sme_AcquireGlobalLock( &pMac->sme );
    if ( HAL_STATUS_SUCCESS( status ) )
    {
        if( CSR_IS_SESSION_VALID( pMac, sessionId ) )
        {
           status = csrRoamGetWpaRsnReqIE( hHal, sessionId, pLen, pBuf );
        }
        else
        {
           status = eHAL_STATUS_INVALID_PARAMETER;
        }
        sme_ReleaseGlobalLock( &pMac->sme );
    }

    return (status);
}

/* ---------------------------------------------------------------------------
    \fn sme_RoamGetSecurityRspIE
    \brief a wrapper function to request CSR to return the WPA or RSN or WAPI IE from
           the beacon or probe rsp if connected
    This is a synchronuous call.
    \param pLen - caller allocated memory that has the length of pBuf as input.
                  Upon returned, *pLen has the needed or IE length in pBuf.
    \param pBuf - Caller allocated memory that contain the IE field, if any,
                  upon return
    \param secType - Specifies whether looking for WPA/WPA2/WAPI IE
    \return eHalStatus - when fail, it usually means the buffer allocated is not
                         big enough
  ---------------------------------------------------------------------------*/
eHalStatus sme_RoamGetSecurityRspIE(tHalHandle hHal, tANI_U8 sessionId, tANI_U32 *pLen,
                                  tANI_U8 *pBuf, eCsrSecurityType secType)
{
    eHalStatus status = eHAL_STATUS_FAILURE;
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

    status = sme_AcquireGlobalLock( &pMac->sme );
    if ( HAL_STATUS_SUCCESS( status ) )
    {
        if( CSR_IS_SESSION_VALID( pMac, sessionId ) )
        {
           status = csrRoamGetWpaRsnRspIE( pMac, sessionId, pLen, pBuf );
        }
        else
        {
           status = eHAL_STATUS_INVALID_PARAMETER;
        }
        sme_ReleaseGlobalLock( &pMac->sme );
    }

    return (status);

}


/* ---------------------------------------------------------------------------
    \fn sme_RoamGetNumPMKIDCache
    \brief a wrapper function to request CSR to return number of PMKID cache
           entries
    This is a synchronuous call.
    \return tANI_U32 - the number of PMKID cache entries
  ---------------------------------------------------------------------------*/
tANI_U32 sme_RoamGetNumPMKIDCache(tHalHandle hHal, tANI_U8 sessionId)
{
    eHalStatus status = eHAL_STATUS_FAILURE;
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );
    tANI_U32 numPmkidCache = 0;

    status = sme_AcquireGlobalLock( &pMac->sme );
    if ( HAL_STATUS_SUCCESS( status ) )
    {
        if( CSR_IS_SESSION_VALID( pMac, sessionId ) )
        {
           numPmkidCache = csrRoamGetNumPMKIDCache( pMac, sessionId );
           status = eHAL_STATUS_SUCCESS;
        }
        else
        {
           status = eHAL_STATUS_INVALID_PARAMETER;
        }
        sme_ReleaseGlobalLock( &pMac->sme );
    }

    return (numPmkidCache);
}

/* ---------------------------------------------------------------------------
    \fn sme_RoamGetPMKIDCache
    \brief a wrapper function to request CSR to return PMKID cache from CSR
    This is a synchronuous call.
    \param pNum - caller allocated memory that has the space of the number of
                  pBuf tPmkidCacheInfo as input. Upon returned, *pNum has the
                  needed or actually number in tPmkidCacheInfo.
    \param pPmkidCache - Caller allocated memory that contains PMKID cache, if
                         any, upon return
    \return eHalStatus - when fail, it usually means the buffer allocated is not
                         big enough
  ---------------------------------------------------------------------------*/
eHalStatus sme_RoamGetPMKIDCache(tHalHandle hHal, tANI_U8 sessionId, tANI_U32 *pNum,
                                 tPmkidCacheInfo *pPmkidCache)
{
   eHalStatus status = eHAL_STATUS_FAILURE;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   status = sme_AcquireGlobalLock( &pMac->sme );
   if ( HAL_STATUS_SUCCESS( status ) )
   {
       if( CSR_IS_SESSION_VALID( pMac, sessionId ) )
       {
          status = csrRoamGetPMKIDCache( pMac, sessionId, pNum, pPmkidCache );
       }
       else
       {
          status = eHAL_STATUS_INVALID_PARAMETER;
       }
       sme_ReleaseGlobalLock( &pMac->sme );
   }

   return (status);
}


/* ---------------------------------------------------------------------------
    \fn sme_GetConfigParam
    \brief a wrapper function that HDD calls to get the global settings
           currently maintained by CSR.
    This is a synchronuous call.
    \param pParam - caller allocated memory
    \return eHalStatus
  ---------------------------------------------------------------------------*/
eHalStatus sme_GetConfigParam(tHalHandle hHal, tCsrConfigParam *pParam)
{
   eHalStatus status = eHAL_STATUS_FAILURE;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   status = sme_AcquireGlobalLock( &pMac->sme );
   if ( HAL_STATUS_SUCCESS( status ) )
   {
       status = csrGetConfigParam(pMac, pParam);
       sme_ReleaseGlobalLock( &pMac->sme );
   }

   return (status);
}

/* ---------------------------------------------------------------------------
    \fn sme_CfgSetInt
    \brief a wrapper function that HDD calls to set parameters in CFG.
    This is a synchronuous call.
    \param cfgId - Configuration Parameter ID (type) for STA.
    \param ccmValue - The information related to Configuration Parameter ID
                      which needs to be saved in CFG
    \param callback - To be registered by CSR with CCM. Once the CFG done with
                      saving the information in the database, it notifies CCM &
                      then the callback will be invoked to notify.
    \param toBeSaved - To save the request for future reference
    \return eHalStatus
  ---------------------------------------------------------------------------*/
eHalStatus sme_CfgSetInt(tHalHandle hHal, tANI_U32 cfgId, tANI_U32 ccmValue,
                         tCcmCfgSetCallback callback, eAniBoolean toBeSaved)
{
   return(ccmCfgSetInt(hHal, cfgId, ccmValue, callback, toBeSaved));
}

/* ---------------------------------------------------------------------------
    \fn sme_CfgSetStr
    \brief a wrapper function that HDD calls to set parameters in CFG.
    This is a synchronuous call.
    \param cfgId - Configuration Parameter ID (type) for STA.
    \param pStr - Pointer to the byte array which carries the information needs
                  to be saved in CFG
    \param length - Length of the data to be saved
    \param callback - To be registered by CSR with CCM. Once the CFG done with
                      saving the information in the database, it notifies CCM &
                      then the callback will be invoked to notify.
    \param toBeSaved - To save the request for future reference
    \return eHalStatus
  ---------------------------------------------------------------------------*/
eHalStatus sme_CfgSetStr(tHalHandle hHal, tANI_U32 cfgId, tANI_U8 *pStr,
                         tANI_U32 length, tCcmCfgSetCallback callback,
                         eAniBoolean toBeSaved)
{
   return(ccmCfgSetStr(hHal, cfgId, pStr, length, callback, toBeSaved));
}

/* ---------------------------------------------------------------------------
    \fn sme_GetModifyProfileFields
    \brief HDD or SME - QOS calls this function to get the current values of
    connected profile fields, changing which can cause reassoc.
    This function must be called after CFG is downloaded and STA is in connected
    state. Also, make sure to call this function to get the current profile
    fields before calling the reassoc. So that pModifyProfileFields will have
    all the latest values plus the one(s) has been updated as part of reassoc
    request.
    \param pModifyProfileFields - pointer to the connected profile fields
    changing which can cause reassoc

    \return eHalStatus
  -------------------------------------------------------------------------------*/
eHalStatus sme_GetModifyProfileFields(tHalHandle hHal, tANI_U8 sessionId,
                                     tCsrRoamModifyProfileFields * pModifyProfileFields)
{
   eHalStatus status = eHAL_STATUS_FAILURE;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   status = sme_AcquireGlobalLock( &pMac->sme );
   if ( HAL_STATUS_SUCCESS( status ) )
   {
       if( CSR_IS_SESSION_VALID( pMac, sessionId ) )
       {
          status = csrGetModifyProfileFields(pMac, sessionId, pModifyProfileFields);
       }
       else
       {
          status = eHAL_STATUS_INVALID_PARAMETER;
       }
       sme_ReleaseGlobalLock( &pMac->sme );
   }

   return (status);
}

/*--------------------------------------------------------------------------
    \fn sme_SetConfigPowerSave
    \brief  Wrapper fn to change power save configuration in SME (PMC) module.
            For BMPS related configuration, this function also updates the CFG
            and sends a message to FW to pick up the new values. Note: Calling
            this function only updates the configuration and does not enable
            the specified power save mode.
    \param  hHal - The handle returned by macOpen.
    \param  psMode - Power Saving mode being modified
    \param  pConfigParams - a pointer to a caller allocated object of type
            tPmcSmpsConfigParams or tPmcBmpsConfigParams or tPmcImpsConfigParams
    \return eHalStatus
  --------------------------------------------------------------------------*/
eHalStatus sme_SetConfigPowerSave(tHalHandle hHal, tPmcPowerSavingMode psMode,
			                      void *pConfigParams)
{
   eHalStatus status = eHAL_STATUS_FAILURE;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   if (NULL == pConfigParams ) {
      smsLog( pMac, LOGE, "Empty config param structure for PMC, nothing to \
            update\n");
      return eHAL_STATUS_FAILURE;
   }

   status = sme_AcquireGlobalLock( &pMac->sme );
   if ( HAL_STATUS_SUCCESS( status ) )
   {
       status = pmcSetConfigPowerSave(hHal, psMode, pConfigParams);
       sme_ReleaseGlobalLock( &pMac->sme );
   }

   return (status);
}

/*--------------------------------------------------------------------------
    \fn sme_GetConfigPowerSave
    \brief  Wrapper fn to retireve power save configuration in SME (PMC) module
    \param  hHal - The handle returned by macOpen.
    \param  psMode - Power Saving mode
    \param  pConfigParams - a pointer to a caller allocated object of type
            tPmcSmpsConfigParams or tPmcBmpsConfigParams or tPmcImpsConfigParams
    \return eHalStatus
  --------------------------------------------------------------------------*/
eHalStatus sme_GetConfigPowerSave(tHalHandle hHal, tPmcPowerSavingMode psMode,
			                      void *pConfigParams)
{
   eHalStatus status = eHAL_STATUS_FAILURE;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   if (NULL == pConfigParams ) {
      smsLog( pMac, LOGE, "Empty config param structure for PMC, nothing to \
            update\n");
      return eHAL_STATUS_FAILURE;
   }

   status = sme_AcquireGlobalLock( &pMac->sme );
   if ( HAL_STATUS_SUCCESS( status ) )
   {
       status = pmcGetConfigPowerSave(hHal, psMode, pConfigParams);
       sme_ReleaseGlobalLock( &pMac->sme );
   }

   return (status);
}

/* ---------------------------------------------------------------------------
    \fn sme_SignalPowerEvent
    \brief  Signals to PMC that a power event has occurred. Used for putting
            the chip into deep sleep mode.
    \param  hHal - The handle returned by macOpen.
    \param  event - the event that has occurred
    \return eHalStatus
  ---------------------------------------------------------------------------*/
eHalStatus sme_SignalPowerEvent (tHalHandle hHal, tPmcPowerEvent event)
{
   eHalStatus status = eHAL_STATUS_FAILURE;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   status = sme_AcquireGlobalLock( &pMac->sme );
   if ( HAL_STATUS_SUCCESS( status ) )
   {
       status = pmcSignalPowerEvent(hHal, event);
       sme_ReleaseGlobalLock( &pMac->sme );
   }

   return (status);
}

/* ---------------------------------------------------------------------------
    \fn sme_EnablePowerSave
    \brief  Enables one of the power saving modes.
    \param  hHal - The handle returned by macOpen.
    \param  psMode - The power saving mode to enable. If BMPS mode is enabled
                     while the chip is operating in Full Power, PMC will start
                     a timer that will try to put the chip in BMPS mode after
                     expiry.
    \return eHalStatus
  ---------------------------------------------------------------------------*/
eHalStatus sme_EnablePowerSave (tHalHandle hHal, tPmcPowerSavingMode psMode)
{
   eHalStatus status = eHAL_STATUS_FAILURE;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   status = sme_AcquireGlobalLock( &pMac->sme );
   if ( HAL_STATUS_SUCCESS( status ) )
   {
       status =  pmcEnablePowerSave(hHal, psMode);
       sme_ReleaseGlobalLock( &pMac->sme );
   }

   return (status);
}

/* ---------------------------------------------------------------------------
    \fn sme_DisablePowerSave
    \brief   Disables one of the power saving modes.
    \param  hHal - The handle returned by macOpen.
    \param  psMode - The power saving mode to disable. Disabling does not imply
                     that device will be brought out of the current PS mode. This
                     is purely a configuration API.
    \return eHalStatus
  ---------------------------------------------------------------------------*/
eHalStatus sme_DisablePowerSave (tHalHandle hHal, tPmcPowerSavingMode psMode)
{
   eHalStatus status = eHAL_STATUS_FAILURE;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   status = sme_AcquireGlobalLock( &pMac->sme );
   if ( HAL_STATUS_SUCCESS( status ) )
   {
       status = pmcDisablePowerSave(hHal, psMode);
       sme_ReleaseGlobalLock( &pMac->sme );
   }

   return (status);
 }

/* ---------------------------------------------------------------------------
    \fn sme_StartAutoBmpsTimer
    \brief  Starts a timer that periodically polls all the registered
            module for entry into Bmps mode. This timer is started only if BMPS is
            enabled and whenever the device is in full power.
    \param  hHal - The handle returned by macOpen.
    \return eHalStatus
  ---------------------------------------------------------------------------*/
eHalStatus sme_StartAutoBmpsTimer ( tHalHandle hHal)
{
   eHalStatus status = eHAL_STATUS_FAILURE;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   status = sme_AcquireGlobalLock( &pMac->sme );
   if ( HAL_STATUS_SUCCESS( status ) )
   {
       status = pmcStartAutoBmpsTimer(hHal);
       sme_ReleaseGlobalLock( &pMac->sme );
   }

   return (status);
}
/* ---------------------------------------------------------------------------
    \fn sme_StopAutoBmpsTimer
    \brief  Stops the Auto BMPS Timer that was started using sme_startAutoBmpsTimer
            Stopping the timer does not cause a device state change. Only the timer
            is stopped. If "Full Power" is desired, use the sme_RequestFullPower API
    \param  hHal - The handle returned by macOpen.
    \return eHalStatus
  ---------------------------------------------------------------------------*/
eHalStatus sme_StopAutoBmpsTimer ( tHalHandle hHal)
{
   eHalStatus status = eHAL_STATUS_FAILURE;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   status = sme_AcquireGlobalLock( &pMac->sme );
   if ( HAL_STATUS_SUCCESS( status ) )
   {
       status = pmcStopAutoBmpsTimer(hHal);
       sme_ReleaseGlobalLock( &pMac->sme );
   }

   return (status);
}
/* ---------------------------------------------------------------------------
    \fn sme_QueryPowerState
    \brief  Returns the current power state of the device.
    \param  hHal - The handle returned by macOpen.
    \param pPowerState - pointer to location to return power state (LOW or HIGH)
    \param pSwWlanSwitchState - ptr to location to return SW WLAN Switch state
    \return eHalStatus
  ---------------------------------------------------------------------------*/
eHalStatus sme_QueryPowerState (
   tHalHandle hHal,
   tPmcPowerState *pPowerState,
   tPmcSwitchState *pSwWlanSwitchState)
{
   eHalStatus status = eHAL_STATUS_FAILURE;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   status = sme_AcquireGlobalLock( &pMac->sme );
   if ( HAL_STATUS_SUCCESS( status ) )
   {
       status = pmcQueryPowerState (hHal, pPowerState, NULL, pSwWlanSwitchState);
       sme_ReleaseGlobalLock( &pMac->sme );
   }

   return (status);
}

/* ---------------------------------------------------------------------------
    \fn sme_IsPowerSaveEnabled
    \brief  Checks if the device is able to enter a particular power save mode
            This does not imply that the device is in a particular PS mode
    \param  hHal - The handle returned by macOpen.
    \param psMode - the power saving mode
    \return eHalStatus
  ---------------------------------------------------------------------------*/
tANI_BOOLEAN sme_IsPowerSaveEnabled (tHalHandle hHal, tPmcPowerSavingMode psMode)
{
   eHalStatus status = eHAL_STATUS_FAILURE;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );
   tANI_BOOLEAN result = false;

   status = sme_AcquireGlobalLock( &pMac->sme );
   if ( HAL_STATUS_SUCCESS( status ) )
   {
       result = pmcIsPowerSaveEnabled(hHal, psMode);
       sme_ReleaseGlobalLock( &pMac->sme );
       return result;
   }

   return false;
}

/* ---------------------------------------------------------------------------
    \fn sme_RequestFullPower
    \brief  Request that the device be brought to full power state. When the
            device enters Full Power PMC will start a BMPS timer if BMPS PS mode
            is enabled. On timer expiry PMC will attempt to put the device in
            BMPS mode if following holds true:
            - BMPS mode is enabled
            - Polling of all modules through the Power Save Check routine passes
            - STA is associated to an access point
    \param  hHal - The handle returned by macOpen.
    \param  - callbackRoutine Callback routine invoked in case of success/failure
    \return eHalStatus - status
     eHAL_STATUS_SUCCESS - device brought to full power state
     eHAL_STATUS_FAILURE - device cannot be brought to full power state
     eHAL_STATUS_PMC_PENDING - device is being brought to full power state,
  ---------------------------------------------------------------------------*/
eHalStatus sme_RequestFullPower (
   tHalHandle hHal,
   void (*callbackRoutine) (void *callbackContext, eHalStatus status),
   void *callbackContext,
   tRequestFullPowerReason fullPowerReason)
{
   eHalStatus status = eHAL_STATUS_FAILURE;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   status = sme_AcquireGlobalLock( &pMac->sme );
   if ( HAL_STATUS_SUCCESS( status ) )
   {
       status = pmcRequestFullPower(hHal, callbackRoutine, callbackContext, fullPowerReason);
       sme_ReleaseGlobalLock( &pMac->sme );
   }

   return (status);
}

/* ---------------------------------------------------------------------------
    \fn sme_RequestBmps
    \brief  Request that the device be put in BMPS state. Request will be
            accepted only if BMPS mode is enabled and power save check routine
            passes.
    \param  hHal - The handle returned by macOpen.
    \param  - callbackRoutine Callback routine invoked in case of success/failure
    \return eHalStatus
      eHAL_STATUS_SUCCESS - device is in BMPS state
      eHAL_STATUS_FAILURE - device cannot be brought to BMPS state
      eHAL_STATUS_PMC_PENDING - device is being brought to BMPS state
  ---------------------------------------------------------------------------*/
eHalStatus sme_RequestBmps (
   tHalHandle hHal,
   void (*callbackRoutine) (void *callbackContext, eHalStatus status),
   void *callbackContext)
{
   eHalStatus status = eHAL_STATUS_FAILURE;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   status = sme_AcquireGlobalLock( &pMac->sme );
   if ( HAL_STATUS_SUCCESS( status ) )
   {
       status = pmcRequestBmps(hHal, callbackRoutine, callbackContext);
       sme_ReleaseGlobalLock( &pMac->sme );
   }

   return (status);
}


/* ---------------------------------------------------------------------------
    \fn  sme_SetDHCPTillPowerActiveFlag
    \brief  Sets/Clears DHCP related flag in PMC to disable/enable auto BMPS
            entry by PMC
    \param  hHal - The handle returned by macOpen.
  ---------------------------------------------------------------------------*/
void  sme_SetDHCPTillPowerActiveFlag(tHalHandle hHal, tANI_U8 flag)
{
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   // Set/Clear the DHCP flag which will disable/enable auto BMPS entery by PMC
   pMac->pmc.remainInPowerActiveTillDHCP = flag;
}


/* ---------------------------------------------------------------------------
    \fn sme_StartUapsd
    \brief  Request that the device be put in UAPSD state. If the device is in
            Full Power it will be put in BMPS mode first and then into UAPSD
            mode.
    \param  hHal - The handle returned by macOpen.
    \param  - callbackRoutine Callback routine invoked in case of success/failure
      eHAL_STATUS_SUCCESS - device is in UAPSD state
      eHAL_STATUS_FAILURE - device cannot be brought to UAPSD state
      eHAL_STATUS_PMC_PENDING - device is being brought to UAPSD state
      eHAL_STATUS_PMC_DISABLED - UAPSD is disabled or BMPS mode is disabled
    \return eHalStatus
  ---------------------------------------------------------------------------*/
eHalStatus sme_StartUapsd (
   tHalHandle hHal,
   void (*callbackRoutine) (void *callbackContext, eHalStatus status),
   void *callbackContext)
{
   eHalStatus status = eHAL_STATUS_FAILURE;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   status = sme_AcquireGlobalLock( &pMac->sme );
   if ( HAL_STATUS_SUCCESS( status ) )
   {
       status = pmcStartUapsd(hHal, callbackRoutine, callbackContext);
       sme_ReleaseGlobalLock( &pMac->sme );
   }

   return (status);
 }

/* ---------------------------------------------------------------------------
    \fn sme_StopUapsd
    \brief  Request that the device be put out of UAPSD state. Device will be
            put in in BMPS state after stop UAPSD completes.
    \param  hHal - The handle returned by macOpen.
    \return eHalStatus
      eHAL_STATUS_SUCCESS - device is put out of UAPSD and back in BMPS state
      eHAL_STATUS_FAILURE - device cannot be brought out of UAPSD state
  ---------------------------------------------------------------------------*/
eHalStatus sme_StopUapsd (tHalHandle hHal)
{
   eHalStatus status = eHAL_STATUS_FAILURE;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   status = sme_AcquireGlobalLock( &pMac->sme );
   if ( HAL_STATUS_SUCCESS( status ) )
   {
       status = pmcStopUapsd(hHal);
       sme_ReleaseGlobalLock( &pMac->sme );
   }

   return (status);
}

/* ---------------------------------------------------------------------------
    \fn sme_RequestStandby
    \brief  Request that the device be put in standby. It is HDD's responsibility
            to bring the chip to full power and do a disassoc before calling
            this API.
    \param  hHal - The handle returned by macOpen.
    \param  - callbackRoutine Callback routine invoked in case of success/failure
    \return eHalStatus
      eHAL_STATUS_SUCCESS - device is in Standby mode
      eHAL_STATUS_FAILURE - device cannot be put in standby mode
      eHAL_STATUS_PMC_PENDING - device is being put in standby mode
  ---------------------------------------------------------------------------*/
eHalStatus sme_RequestStandby (
   tHalHandle hHal,
   void (*callbackRoutine) (void *callbackContext, eHalStatus status),
   void *callbackContext)
{
   eHalStatus status = eHAL_STATUS_FAILURE;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   smsLog( pMac, LOGE, FL(" \n") );
   status = sme_AcquireGlobalLock( &pMac->sme );
   if ( HAL_STATUS_SUCCESS( status ) )
   {
       status = pmcRequestStandby(hHal, callbackRoutine, callbackContext);
       sme_ReleaseGlobalLock( &pMac->sme );
   }

   return (status);
}

/* ---------------------------------------------------------------------------
    \fn sme_RegisterPowerSaveCheck
    \brief  Register a power save check routine that is called whenever
            the device is about to enter one of the power save modes.
    \param  hHal - The handle returned by macOpen.
    \param  checkRoutine -  Power save check routine to be registered
    \return eHalStatus
            eHAL_STATUS_SUCCESS - successfully registered
            eHAL_STATUS_FAILURE - not successfully registered
  ---------------------------------------------------------------------------*/
eHalStatus sme_RegisterPowerSaveCheck (
   tHalHandle hHal,
   tANI_BOOLEAN (*checkRoutine) (void *checkContext), void *checkContext)
{
   eHalStatus status = eHAL_STATUS_FAILURE;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   status = sme_AcquireGlobalLock( &pMac->sme );
   if ( HAL_STATUS_SUCCESS( status ) )
   {
       status = pmcRegisterPowerSaveCheck (hHal, checkRoutine, checkContext);
       sme_ReleaseGlobalLock( &pMac->sme );
   }

   return (status);
}

/* ---------------------------------------------------------------------------
    \fn sme_DeregisterPowerSaveCheck
    \brief  Deregister a power save check routine
    \param  hHal - The handle returned by macOpen.
    \param  checkRoutine -  Power save check routine to be deregistered
    \return eHalStatus
            eHAL_STATUS_SUCCESS - successfully deregistered
            eHAL_STATUS_FAILURE - not successfully deregistered
  ---------------------------------------------------------------------------*/
eHalStatus sme_DeregisterPowerSaveCheck (
   tHalHandle hHal,
   tANI_BOOLEAN (*checkRoutine) (void *checkContext))
{
   eHalStatus status = eHAL_STATUS_FAILURE;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   status = sme_AcquireGlobalLock( &pMac->sme );
   if ( HAL_STATUS_SUCCESS( status ) )
   {
       status = pmcDeregisterPowerSaveCheck (hHal, checkRoutine);
       sme_ReleaseGlobalLock( &pMac->sme );
   }

   return (status);
}

/* ---------------------------------------------------------------------------
    \fn sme_RegisterDeviceStateUpdateInd
    \brief  Register a callback routine that is called whenever
            the device enters a new device state (Full Power, BMPS, UAPSD)
    \param  hHal - The handle returned by macOpen.
    \param  callbackRoutine -  Callback routine to be registered
    \param  callbackContext -  Cookie to be passed back during callback
    \return eHalStatus
            eHAL_STATUS_SUCCESS - successfully registered
            eHAL_STATUS_FAILURE - not successfully registered
  ---------------------------------------------------------------------------*/
eHalStatus sme_RegisterDeviceStateUpdateInd (
   tHalHandle hHal,
   void (*callbackRoutine) (void *callbackContext, tPmcState pmcState),
   void *callbackContext)
{
   eHalStatus status = eHAL_STATUS_FAILURE;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   status = sme_AcquireGlobalLock( &pMac->sme );
   if ( HAL_STATUS_SUCCESS( status ) )
   {
       status = pmcRegisterDeviceStateUpdateInd (hHal, callbackRoutine, callbackContext);
       sme_ReleaseGlobalLock( &pMac->sme );
   }

   return (status);
}

/* ---------------------------------------------------------------------------
    \fn sme_DeregisterDeviceStateUpdateInd
    \brief  Deregister a routine that was registered for device state changes
    \param  hHal - The handle returned by macOpen.
    \param  callbackRoutine -  Callback routine to be deregistered
    \return eHalStatus
            eHAL_STATUS_SUCCESS - successfully deregistered
            eHAL_STATUS_FAILURE - not successfully deregistered
  ---------------------------------------------------------------------------*/
eHalStatus sme_DeregisterDeviceStateUpdateInd (
   tHalHandle hHal,
   void (*callbackRoutine) (void *callbackContext, tPmcState pmcState))
{
   eHalStatus status = eHAL_STATUS_FAILURE;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   status = sme_AcquireGlobalLock( &pMac->sme );
   if ( HAL_STATUS_SUCCESS( status ) )
   {
       status = pmcDeregisterDeviceStateUpdateInd (hHal, callbackRoutine);
       sme_ReleaseGlobalLock( &pMac->sme );
   }

   return (status);
}

/* ---------------------------------------------------------------------------
    \fn sme_WowlAddBcastPattern
    \brief  Add a pattern for Pattern Byte Matching in Wowl mode. Firmware will
            do a pattern match on these patterns when Wowl is enabled during BMPS
            mode. Note that Firmware performs the pattern matching only on
            broadcast frames and while Libra is in BMPS mode.
    \param  hHal - The handle returned by macOpen.
    \param  pattern -  Pattern to be added
    \return eHalStatus
            eHAL_STATUS_FAILURE  Cannot add pattern
            eHAL_STATUS_SUCCESS  Request accepted.
  ---------------------------------------------------------------------------*/
eHalStatus sme_WowlAddBcastPattern (
   tHalHandle hHal,
   tpSirWowlAddBcastPtrn pattern)
{
    eHalStatus status = eHAL_STATUS_FAILURE;
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );
    status = sme_AcquireGlobalLock( &pMac->sme );
    if ( HAL_STATUS_SUCCESS( status ) )
    {
       status = pmcWowlAddBcastPattern (hHal, pattern);
       sme_ReleaseGlobalLock( &pMac->sme );
    }

    return (status);
}

/* ---------------------------------------------------------------------------
    \fn sme_WowlDelBcastPattern
    \brief  Delete a pattern that was added for Pattern Byte Matching.
    \param  hHal - The handle returned by macOpen.
    \param  pattern -  Pattern to be deleted
    \return eHalStatus
            eHAL_STATUS_FAILURE  Cannot delete pattern
            eHAL_STATUS_SUCCESS  Request accepted.
  ---------------------------------------------------------------------------*/
eHalStatus sme_WowlDelBcastPattern (
   tHalHandle hHal,
   tpSirWowlDelBcastPtrn pattern)
{
    eHalStatus status = eHAL_STATUS_FAILURE;
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );
    status = sme_AcquireGlobalLock( &pMac->sme );
    if ( HAL_STATUS_SUCCESS( status ) )
    {
       status = pmcWowlDelBcastPattern (hHal, pattern);
       sme_ReleaseGlobalLock( &pMac->sme );
    }

    return (status);
}

/* ---------------------------------------------------------------------------
    \fn sme_EnterWowl
    \brief  This is the SME API exposed to HDD to request enabling of WOWL mode.
            WoWLAN works on top of BMPS mode. If the device is not in BMPS mode,
            SME will will cache the information that WOWL has been enabled and
            attempt to put the device in BMPS. On entry into BMPS, SME will
            enable the WOWL mode.
            Note 1: If we exit BMPS mode (someone requests full power), we
            will NOT resume WOWL when we go back to BMPS again. Request for full
            power (while in WOWL mode) means disable WOWL and go to full power.
            Note 2: Both UAPSD and WOWL work on top of BMPS. On entry into BMPS, SME
            will give priority to UAPSD and enable only UAPSD if both UAPSD and WOWL
            are required. Currently there is no requirement or use case to support
            UAPSD and WOWL at the same time.

    \param  hHal - The handle returned by macOpen.
    \param  callbackRoutine -  Callback routine provided by HDD.
                               Used for success/failure notification by SME
    \param  callbackContext - A cookie passed by HDD, that is passed back to HDD
                              at the time of callback.
    \return eHalStatus
            eHAL_STATUS_SUCCESS  Device is already in WoWLAN mode
            eHAL_STATUS_FAILURE  Device cannot enter WoWLAN mode.
            eHAL_STATUS_PMC_PENDING  Request accepted. SME will enable WOWL after
                                      BMPS mode is entered.
  ---------------------------------------------------------------------------*/
eHalStatus sme_EnterWowl (
    tHalHandle hHal,
    void (*callbackRoutine) (void *callbackContext, eHalStatus status),
    void *callbackContext, tpSirSmeWowlEnterParams wowlEnterParams)
{
    eHalStatus status = eHAL_STATUS_FAILURE;
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );
    status = sme_AcquireGlobalLock( &pMac->sme );
    if ( HAL_STATUS_SUCCESS( status ) )
    {
       status = pmcEnterWowl (hHal, callbackRoutine, callbackContext, wowlEnterParams);
       sme_ReleaseGlobalLock( &pMac->sme );
    }

    return (status);
}
/* ---------------------------------------------------------------------------
    \fn sme_ExitWowl
    \brief  This is the SME API exposed to HDD to request exit from WoWLAN mode.
            SME will initiate exit from WoWLAN mode and device will be put in BMPS
            mode.
    \param  hHal - The handle returned by macOpen.
    \return eHalStatus
            eHAL_STATUS_FAILURE  Device cannot exit WoWLAN mode.
            eHAL_STATUS_SUCCESS  Request accepted to exit WoWLAN mode.
  ---------------------------------------------------------------------------*/
eHalStatus sme_ExitWowl (tHalHandle hHal)
{
    eHalStatus status = eHAL_STATUS_FAILURE;
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );
    status = sme_AcquireGlobalLock( &pMac->sme );
    if ( HAL_STATUS_SUCCESS( status ) )
    {
       status = pmcExitWowl (hHal);
       sme_ReleaseGlobalLock( &pMac->sme );
    }

    return (status);
}

/* ---------------------------------------------------------------------------

    \fn sme_RoamSetKey

    \brief To set encryption key. This function should be called only when connected
    This is an asynchronous API.

    \param pSetKeyInfo - pointer to a caller allocated object of tCsrSetContextInfo

    \param pRoamId  Upon success return, this is the id caller can use to identify the request in roamcallback

    \return eHalStatus  SUCCESS  Roam callback will be called indicate actually results

                         FAILURE or RESOURCES  The API finished and failed.

  -------------------------------------------------------------------------------*/
eHalStatus sme_RoamSetKey(tHalHandle hHal, tANI_U8 sessionId, tCsrRoamSetKey *pSetKey, tANI_U32 *pRoamId)
{
   eHalStatus status = eHAL_STATUS_FAILURE;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );
   tANI_U32 roamId;
#ifdef WLAN_SOFTAP_FEATURE
   tANI_U32 i;
   tCsrRoamSession *pSession = NULL;
#endif

   status = sme_AcquireGlobalLock( &pMac->sme );
   if ( HAL_STATUS_SUCCESS( status ) )
   {
      roamId = GET_NEXT_ROAM_ID(&pMac->roam);
      if(pRoamId)
      {
         *pRoamId = roamId;
      }

#ifdef WLAN_SOFTAP_FEATURE
      smsLog(pMac, LOGE, FL("keyLength\n"), pSetKey->keyLength);

      for(i=0; i<pSetKey->keyLength; i++)
          smsLog(pMac, LOGE, FL("%02x"), pSetKey->Key[i]);

      smsLog(pMac, LOGE, "\n sessionId=%d roamId=%d\n", sessionId, roamId);

      pSession = CSR_GET_SESSION(pMac, sessionId);
      if(NULL == pSession)
      {
          VOS_ASSERT(0);
          return eHAL_STATUS_FAILURE;
      }
      if(CSR_IS_INFRA_AP(&pSession->connectedProfile))
      {

          if ( ( eCSR_ENCRYPT_TYPE_WEP40 == pSetKey->encType ) ||
             ( eCSR_ENCRYPT_TYPE_WEP40_STATICKEY == pSetKey->encType ))
              pSession->pCurRoamProfile->negotiatedUCEncryptionType = eCSR_ENCRYPT_TYPE_WEP40_STATICKEY;

          if ( ( eCSR_ENCRYPT_TYPE_WEP104 == pSetKey->encType ) ||
             ( eCSR_ENCRYPT_TYPE_WEP104_STATICKEY == pSetKey->encType ))
          {
	      pSession->pCurRoamProfile->negotiatedUCEncryptionType = eCSR_ENCRYPT_TYPE_WEP104_STATICKEY;
          }

      }
#endif

      status = csrRoamSetKey ( pMac, sessionId, pSetKey, roamId );
      sme_ReleaseGlobalLock( &pMac->sme );
   }

   return (status);
}


/* ---------------------------------------------------------------------------

    \fn sme_RoamRemoveKey

    \brief To set encryption key. This is an asynchronous API.

    \param pRemoveKey - pointer to a caller allocated object of tCsrRoamRemoveKey

    \param pRoamId  Upon success return, this is the id caller can use to identify the request in roamcallback

    \return eHalStatus  SUCCESS  Roam callback will be called indicate actually results

                         FAILURE or RESOURCES  The API finished and failed.

  -------------------------------------------------------------------------------*/
eHalStatus sme_RoamRemoveKey(tHalHandle hHal, tANI_U8 sessionId,
                             tCsrRoamRemoveKey *pRemoveKey, tANI_U32 *pRoamId)
{
   eHalStatus status = eHAL_STATUS_FAILURE;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );
   tANI_U32 roamId;

   status = sme_AcquireGlobalLock( &pMac->sme );
   if ( HAL_STATUS_SUCCESS( status ) )
   {
      roamId = GET_NEXT_ROAM_ID(&pMac->roam);
      if(pRoamId)
      {
         *pRoamId = roamId;
      }
      status = csrRoamIssueRemoveKeyCommand( pMac, sessionId, pRemoveKey, roamId );
      sme_ReleaseGlobalLock( &pMac->sme );
   }

   return (status);
}

/* ---------------------------------------------------------------------------
    \fn sme_GetRssi
    \brief a wrapper function that client calls to register a callback to get RSSI

    \param callback - SME sends back the requested stats using the callback
    \param staId - The station ID for which the stats is requested for
    \param pContext - user context to be passed back along with the callback
    \param pVosContext - vos context
    \return eHalStatus
  ---------------------------------------------------------------------------*/
eHalStatus sme_GetRssi(tHalHandle hHal,
                             tCsrRssiCallback callback,
                             tANI_U8 staId, void *pContext, void* pVosContext)
{
   eHalStatus status = eHAL_STATUS_FAILURE;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   status = sme_AcquireGlobalLock( &pMac->sme );
   if ( HAL_STATUS_SUCCESS( status ) )
   {
      status = csrGetRssi( pMac, callback,
                                 staId, pContext, pVosContext);
      sme_ReleaseGlobalLock( &pMac->sme );
   }
   return (status);
}

/* ---------------------------------------------------------------------------
    \fn sme_GetStatistics
    \brief a wrapper function that client calls to register a callback to get
    different PHY level statistics from CSR.

    \param requesterId - different client requesting for statistics, HDD, UMA/GAN etc
    \param statsMask - The different category/categories of stats requester is looking for
    \param callback - SME sends back the requested stats using the callback
    \param periodicity - If requester needs periodic update in millisec, 0 means
                         it's an one time request
    \param cache - If requester is happy with cached stats
    \param staId - The station ID for which the stats is requested for
    \param pContext - user context to be passed back along with the callback
    \return eHalStatus
  ---------------------------------------------------------------------------*/
eHalStatus sme_GetStatistics(tHalHandle hHal, eCsrStatsRequesterType requesterId,
                             tANI_U32 statsMask,
                             tCsrStatsCallback callback,
                             tANI_U32 periodicity, tANI_BOOLEAN cache,
                             tANI_U8 staId, void *pContext)
{
   eHalStatus status = eHAL_STATUS_FAILURE;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   status = sme_AcquireGlobalLock( &pMac->sme );
   if ( HAL_STATUS_SUCCESS( status ) )
   {
      status = csrGetStatistics( pMac, requesterId , statsMask, callback,
                                 periodicity, cache, staId, pContext);
      sme_ReleaseGlobalLock( &pMac->sme );
   }

   return (status);

}

/* ---------------------------------------------------------------------------

    \fn sme_GetCountryCode

    \brief To return the current country code. If no country code is applied, default country code is
    used to fill the buffer.
    If 11d supported is turned off, an error is return and the last applied/default country code is used.
    This is a synchronous API.

    \param pBuf - pointer to a caller allocated buffer for returned country code.

    \param pbLen  For input, this parameter indicates how big is the buffer.
                   Upon return, this parameter has the number of bytes for country. If pBuf
                   doesn't have enough space, this function returns
                   fail status and this parameter contains the number that is needed.

    \return eHalStatus  SUCCESS.

                         FAILURE or RESOURCES  The API finished and failed.

  -------------------------------------------------------------------------------*/
eHalStatus sme_GetCountryCode(tHalHandle hHal, tANI_U8 *pBuf, tANI_U8 *pbLen)
{
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

    return ( csrGetCountryCode( pMac, pBuf, pbLen ) );
}


/* ---------------------------------------------------------------------------

    \fn sme_SetCountryCode

    \brief To change the current/default country code.
    If 11d supported is turned off, an error is return.
    This is a synchronous API.

    \param pCountry - pointer to a caller allocated buffer for the country code.

    \param pfRestartNeeded  A pointer to caller allocated memory, upon successful return, it indicates
    whether a reset is required.

    \return eHalStatus  SUCCESS.

                         FAILURE or RESOURCES  The API finished and failed.

  -------------------------------------------------------------------------------*/
eHalStatus sme_SetCountryCode(tHalHandle hHal, tANI_U8 *pCountry, tANI_BOOLEAN *pfRestartNeeded)
{
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

    return ( csrSetCountryCode( pMac, pCountry, pfRestartNeeded ) );
}


/* ---------------------------------------------------------------------------
    \fn sme_ResetCountryCodeInformation
    \brief this function is to reset the country code current being used back to EEPROM default
    this includes channel list and power setting. This is a synchronous API.
    \param pfRestartNeeded - pointer to a caller allocated space. Upon successful return, it indicates whether
    a restart is needed to apply the change
    \return eHalStatus
  -------------------------------------------------------------------------------*/
eHalStatus sme_ResetCountryCodeInformation(tHalHandle hHal, tANI_BOOLEAN *pfRestartNeeded)
{
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

    return ( csrResetCountryCodeInformation( pMac, pfRestartNeeded ) );
}


/* ---------------------------------------------------------------------------
    \fn sme_GetSupportedCountryCode
    \brief this function is to get a list of the country code current being supported
    \param pBuf - Caller allocated buffer with at least 3 bytes, upon success return,
    this has the country code list. 3 bytes for each country code. This may be NULL if
    caller wants to know the needed byte count.
    \param pbLen - Caller allocated, as input, it indicates the length of pBuf. Upon success return,
    this contains the length of the data in pBuf. If pbuf is NULL, as input, *pbLen should be 0.
    \return eHalStatus
  -------------------------------------------------------------------------------*/
eHalStatus sme_GetSupportedCountryCode(tHalHandle hHal, tANI_U8 *pBuf, tANI_U32 *pbLen)
{
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

    return ( csrGetSupportedCountryCode( pMac, pBuf, pbLen ) );
}


/* ---------------------------------------------------------------------------
    \fn sme_GetCurrentRegulatoryDomain
    \brief this function is to get the current regulatory domain. This is a synchronous API.
    This function must be called after CFG is downloaded and all the band/mode setting already passed into
    SME. The function fails if 11d support is turned off.
    \param pDomain - Caller allocated buffer to return the current domain.
    \return eHalStatus  SUCCESS.

                         FAILURE or RESOURCES  The API finished and failed.
  -------------------------------------------------------------------------------*/
eHalStatus sme_GetCurrentRegulatoryDomain(tHalHandle hHal, v_REGDOMAIN_t *pDomain)
{
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );
    eHalStatus status = eHAL_STATUS_INVALID_PARAMETER;

    if( pDomain )
    {
        if( csrIs11dSupported( pMac ) )
        {
            *pDomain = csrGetCurrentRegulatoryDomain( pMac );
            status = eHAL_STATUS_SUCCESS;
        }
        else
        {
            status = eHAL_STATUS_FAILURE;
        }
    }

    return ( status );
}


/* ---------------------------------------------------------------------------
    \fn sme_SetRegulatoryDomain
    \brief this function is to set the current regulatory domain.
    This function must be called after CFG is downloaded and all the band/mode setting already passed into
    SME. This is a synchronous API.
    \param domainId - indicate the domain (defined in the driver) needs to set to.
    See v_REGDOMAIN_t for definition
    \param pfRestartNeeded - pointer to a caller allocated space. Upon successful return, it indicates whether
    a restart is needed to apply the change
    \return eHalStatus
  -------------------------------------------------------------------------------*/
eHalStatus sme_SetRegulatoryDomain(tHalHandle hHal, v_REGDOMAIN_t domainId, tANI_BOOLEAN *pfRestartNeeded)
{
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

    return ( csrSetRegulatoryDomain( pMac, domainId, pfRestartNeeded ) );
}


/* ---------------------------------------------------------------------------

    \fn sme_GetRegulatoryDomainForCountry

    \brief To return a regulatory domain base on a country code. This is a synchronous API.

    \param pCountry - pointer to a caller allocated buffer for input country code.

    \param pDomainId  Upon successful return, it is the domain that country belongs to.
    If it is NULL, returning success means that the country code is known.

    \return eHalStatus  SUCCESS.

                         FAILURE or RESOURCES  The API finished and failed.

  -------------------------------------------------------------------------------*/
eHalStatus sme_GetRegulatoryDomainForCountry(tHalHandle hHal, tANI_U8 *pCountry, v_REGDOMAIN_t *pDomainId)
{
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

    return ( csrGetRegulatoryDomainForCountry( pMac, pCountry, pDomainId ) );
}




/* ---------------------------------------------------------------------------

    \fn sme_GetSupportedRegulatoryDomains

    \brief To return a list of supported regulatory domains. This is a synchronous API.

    \param pDomains - pointer to a caller allocated buffer for returned regulatory domains.

    \param pNumDomains  For input, this parameter indicates howm many domains pDomains can hold.
                         Upon return, this parameter has the number for supported domains. If pDomains
                         doesn't have enough space for all the supported domains, this function returns
                         fail status and this parameter contains the number that is needed.

    \return eHalStatus  SUCCESS.

                         FAILURE or RESOURCES  The API finished and failed.

  -------------------------------------------------------------------------------*/
eHalStatus sme_GetSupportedRegulatoryDomains(tHalHandle hHal, v_REGDOMAIN_t *pDomains, tANI_U32 *pNumDomains)
{
    eHalStatus status = eHAL_STATUS_INVALID_PARAMETER;

    //We support all domains for now
    if( pNumDomains )
    {
        if( NUM_REG_DOMAINS <= *pNumDomains )
        {
            status = eHAL_STATUS_SUCCESS;
        }
        *pNumDomains = NUM_REG_DOMAINS;
    }
    if( HAL_STATUS_SUCCESS( status ) )
    {
        if( pDomains )
        {
            pDomains[0] = REGDOMAIN_FCC;
            pDomains[1] = REGDOMAIN_ETSI;
            pDomains[2] = REGDOMAIN_JAPAN;
            pDomains[3] = REGDOMAIN_WORLD;
            pDomains[4] = REGDOMAIN_N_AMER_EXC_FCC;
            pDomains[5] = REGDOMAIN_APAC;
            pDomains[6] = REGDOMAIN_KOREA;
            pDomains[7] = REGDOMAIN_HI_5GHZ;
            pDomains[8] = REGDOMAIN_NO_5GHZ;
        }
        else
        {
            status = eHAL_STATUS_INVALID_PARAMETER;
        }
    }

    return ( status );
}


//some support functions
tANI_BOOLEAN sme_Is11dSupported(tHalHandle hHal)
{
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

    return ( csrIs11dSupported( pMac ) );
}


tANI_BOOLEAN sme_Is11hSupported(tHalHandle hHal)
{
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

    return ( csrIs11hSupported( pMac ) );
}


tANI_BOOLEAN sme_IsWmmSupported(tHalHandle hHal)
{
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

    return ( csrIsWmmSupported( pMac ) );
}

//Upper layer to get the list of the base channels to scan for passively 11d info from csr
eHalStatus sme_ScanGetBaseChannels( tHalHandle hHal, tCsrChannelInfo * pChannelInfo )
{
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   return(csrScanGetBaseChannels(pMac,pChannelInfo) );
}

/* ---------------------------------------------------------------------------
    \fn sme_BtcSignalBtEvent
    \brief  API to signal Bluetooth (BT) event to the WLAN driver. Based on the
            BT event type and the current operating mode of Libra (full power,
            BMPS, UAPSD etc), appropriate Bluetooth Coexistence (BTC) strategy
            would be employed.
    \param  hHal - The handle returned by macOpen.
    \param  pBtEvent -  Pointer to a caller allocated object of type tSmeBtEvent
                        Caller owns the memory and is responsible for freeing it.
    \return VOS_STATUS
            VOS_STATUS_E_FAILURE  BT Event not passed to HAL. This can happen
                                   if BTC execution mode is set to BTC_WLAN_ONLY
                                   or BTC_PTA_ONLY.
            VOS_STATUS_SUCCESS    BT Event passed to HAL
  ---------------------------------------------------------------------------*/
VOS_STATUS sme_BtcSignalBtEvent (tHalHandle hHal, tpSmeBtEvent pBtEvent)
{
    VOS_STATUS status = VOS_STATUS_E_FAILURE;
#ifndef WLAN_MDM_CODE_REDUCTION_OPT
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

    if ( eHAL_STATUS_SUCCESS == sme_AcquireGlobalLock( &pMac->sme ) )
    {
        status = btcSignalBTEvent (hHal, pBtEvent);
        sme_ReleaseGlobalLock( &pMac->sme );
    }
#endif
    return (status);
}

/* ---------------------------------------------------------------------------
    \fn sme_BtcSetConfig
    \brief  API to change the current Bluetooth Coexistence (BTC) configuration
            This function should be invoked only after CFG download has completed.
            Calling it after sme_HDDReadyInd is recommended.
    \param  hHal - The handle returned by macOpen.
    \param  pSmeBtcConfig - Pointer to a caller allocated object of type tSmeBtcConfig.
                            Caller owns the memory and is responsible for freeing it.
    \return VOS_STATUS
            VOS_STATUS_E_FAILURE  Config not passed to HAL.
            VOS_STATUS_SUCCESS  Config passed to HAL
  ---------------------------------------------------------------------------*/
VOS_STATUS sme_BtcSetConfig (tHalHandle hHal, tpSmeBtcConfig pSmeBtcConfig)
{
    VOS_STATUS status = VOS_STATUS_E_FAILURE;
#ifndef WLAN_MDM_CODE_REDUCTION_OPT
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );
    if ( eHAL_STATUS_SUCCESS == sme_AcquireGlobalLock( &pMac->sme ) )
    {
        status = btcSetConfig (hHal, pSmeBtcConfig);
        sme_ReleaseGlobalLock( &pMac->sme );
    }
#endif
    return (status);
}

/* ---------------------------------------------------------------------------
    \fn sme_BtcGetConfig
    \brief  API to retrieve the current Bluetooth Coexistence (BTC) configuration
    \param  hHal - The handle returned by macOpen.
    \param  pSmeBtcConfig - Pointer to a caller allocated object of type
                            tSmeBtcConfig. Caller owns the memory and is responsible
                            for freeing it.
    \return VOS_STATUS
            VOS_STATUS_E_FAILURE - failure
            VOS_STATUS_SUCCESS  success
  ---------------------------------------------------------------------------*/
VOS_STATUS sme_BtcGetConfig (tHalHandle hHal, tpSmeBtcConfig pSmeBtcConfig)
{
    VOS_STATUS status = VOS_STATUS_E_FAILURE;
#ifndef WLAN_MDM_CODE_REDUCTION_OPT
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

    if ( eHAL_STATUS_SUCCESS == sme_AcquireGlobalLock( &pMac->sme ) )
    {
        status = btcGetConfig (hHal, pSmeBtcConfig);
        sme_ReleaseGlobalLock( &pMac->sme );
    }
#endif
    return (status);
}
/* ---------------------------------------------------------------------------
    \fn sme_SetCfgPrivacy
    \brief  API to set configure privacy parameters
    \param  hHal - The handle returned by macOpen.
    \param  pProfile - Pointer CSR Roam profile.
    \param  fPrivacy - This parameter indicates status of privacy 

    \return void
  ---------------------------------------------------------------------------*/
void sme_SetCfgPrivacy( tHalHandle hHal,
                        tCsrRoamProfile *pProfile,
                        tANI_BOOLEAN fPrivacy
                        )
{
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );
    if ( eHAL_STATUS_SUCCESS == sme_AcquireGlobalLock( &pMac->sme ) )
    {
        csrSetCfgPrivacy(pMac, pProfile, fPrivacy);
        sme_ReleaseGlobalLock( &pMac->sme );
    }
}

#if defined WLAN_FEATURE_VOWIFI
/* ---------------------------------------------------------------------------
    \fn sme_NeighborReportRequest
    \brief  API to request neighbor report.
    \param  hHal - The handle returned by macOpen.
    \param  pRrmNeighborReq - Pointer to a caller allocated object of type
                            tRrmNeighborReq. Caller owns the memory and is responsible
                            for freeing it.
    \return VOS_STATUS
            VOS_STATUS_E_FAILURE - failure
            VOS_STATUS_SUCCESS  success
  ---------------------------------------------------------------------------*/
VOS_STATUS sme_NeighborReportRequest (tHalHandle hHal, tANI_U8 sessionId,
                                    tpRrmNeighborReq pRrmNeighborReq, tpRrmNeighborRspCallbackInfo callbackInfo)
{
    VOS_STATUS status = VOS_STATUS_E_FAILURE;
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

    if ( eHAL_STATUS_SUCCESS == sme_AcquireGlobalLock( &pMac->sme ) )
    {
        status = sme_RrmNeighborReportRequest (hHal, sessionId, pRrmNeighborReq, callbackInfo);
        sme_ReleaseGlobalLock( &pMac->sme );
    }

    return (status);
}
#endif

//The following are debug APIs to support direct read/write register/memory
//They are placed in SME because HW cannot be access when in LOW_POWER state
//AND not connected. The knowledge and synchronization is done in SME

//sme_DbgReadRegister
//Caller needs to validate the input values
VOS_STATUS sme_DbgReadRegister(tHalHandle hHal, v_U32_t regAddr, v_U32_t *pRegValue)
{
    v_PVOID_t pvosGCTx = vos_get_global_context(VOS_MODULE_ID_PE, (v_VOID_t *)hHal);
    VOS_STATUS status = VOS_STATUS_E_FAILURE;
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );
    // To make Quarky work in FTM mode
    if(pMac->gDriverType == eDRIVER_TYPE_MFG)
    {
        status = WLANBAL_ReadRegister( pvosGCTx, regAddr, pRegValue );
    }
    else
    {
    tPmcPowerState PowerState;
    tANI_U32 sessionId = 0;

    if ( eHAL_STATUS_SUCCESS == sme_AcquireGlobalLock( &pMac->sme ) )
    {
        if( HAL_STATUS_SUCCESS( pmcQueryPowerState( pMac, &PowerState, NULL, NULL ) ) )
        {
            if( !csrIsConnStateDisconnected(pMac,sessionId) || ( ePMC_LOW_POWER != PowerState ) )
            {
                status = WLANBAL_ReadRegister( pvosGCTx, regAddr, pRegValue );
            }
            else
            {
                //This is a hack for Qualky/pttWniSocket
                //Current implementation doesn't allow pttWniSocket to inform Qualky an error
                *pRegValue = 0xDEADBEEF;
                status = VOS_STATUS_SUCCESS;
            }
        }
        sme_ReleaseGlobalLock( &pMac->sme );
    }
    }
    return ( status );
}


//sme_DbgWriteRegister
//Caller needs to validate the input values
VOS_STATUS sme_DbgWriteRegister(tHalHandle hHal, v_U32_t regAddr, v_U32_t regValue)
{
    v_PVOID_t pvosGCTx = vos_get_global_context(VOS_MODULE_ID_PE, (v_VOID_t *)hHal);
    VOS_STATUS status = VOS_STATUS_E_FAILURE;
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );
    // To make Quarky work in FTM mode
    if(pMac->gDriverType == eDRIVER_TYPE_MFG)
    {
        status = WLANBAL_WriteRegister( pvosGCTx, regAddr, regValue );
    }
    else
    {
    tPmcPowerState PowerState;
    tANI_U32 sessionId = 0;

    if ( eHAL_STATUS_SUCCESS == sme_AcquireGlobalLock( &pMac->sme ) )
    {
        if( HAL_STATUS_SUCCESS( pmcQueryPowerState( pMac, &PowerState, NULL, NULL ) ) )
        {
            if( !csrIsConnStateDisconnected(pMac,sessionId) || ( ePMC_LOW_POWER != PowerState ) )
            {
                status = WLANBAL_WriteRegister( pvosGCTx, regAddr, regValue );
            }
        }
        sme_ReleaseGlobalLock( &pMac->sme );
    }
    }
    return ( status );
}



//sme_DbgReadMemory
//Caller needs to validate the input values
//pBuf caller allocated buffer has the length of nLen
VOS_STATUS sme_DbgReadMemory(tHalHandle hHal, v_U32_t memAddr, v_U8_t *pBuf, v_U32_t nLen)
{
    v_PVOID_t pvosGCTx = vos_get_global_context(VOS_MODULE_ID_PE, (v_VOID_t *)hHal);
    VOS_STATUS status = VOS_STATUS_E_FAILURE;
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );
    // To make Quarky work in FTM mode
    if(pMac->gDriverType == eDRIVER_TYPE_MFG)
    {
        status = WLANBAL_ReadMemory( pvosGCTx, memAddr, ( void * )( pBuf ), nLen );
    }
    else
    {
    tPmcPowerState PowerState;
    tANI_U32 sessionId = 0;

    if ( eHAL_STATUS_SUCCESS == sme_AcquireGlobalLock( &pMac->sme ) )
    {
        if( HAL_STATUS_SUCCESS( pmcQueryPowerState( pMac, &PowerState, NULL, NULL ) ) )
        {
            if( !csrIsConnStateDisconnected(pMac,sessionId) || ( ePMC_LOW_POWER != PowerState ) )
            {
                status = WLANBAL_ReadMemory( pvosGCTx, memAddr, ( void * )( pBuf ), nLen );
            }
			else
			{
                //This is a hack for Qualky/pttWniSocket
                //Current implementation doesn't allow pttWniSocket to inform Qualky an error
				vos_mem_set(pBuf, nLen, 0xCD);
				status = eHAL_STATUS_SUCCESS;
				smsLog(pMac, LOGE, FL(" filled with 0xCD because it cannot access the hardware\n"));
			}
        }
        sme_ReleaseGlobalLock( &pMac->sme );
    }
    }
    return ( status );
}


//sme_DbgWriteMemory
//Caller needs to validate the input values
VOS_STATUS sme_DbgWriteMemory(tHalHandle hHal, v_U32_t memAddr, v_U8_t *pBuf, v_U32_t nLen)
{
    v_PVOID_t pvosGCTx = vos_get_global_context(VOS_MODULE_ID_PE, (v_VOID_t *)hHal);
    VOS_STATUS status = VOS_STATUS_E_FAILURE;
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );
    // To make Quarky work in FTM mode
    if(pMac->gDriverType == eDRIVER_TYPE_MFG)
    {
        status = WLANBAL_WriteMemory( pvosGCTx, memAddr, ( void * )( pBuf ), nLen );
    }
    else
    {
    tPmcPowerState PowerState;
    tANI_U32 sessionId = 0;

    if ( eHAL_STATUS_SUCCESS == sme_AcquireGlobalLock( &pMac->sme ) )
    {
        if( HAL_STATUS_SUCCESS( pmcQueryPowerState( pMac, &PowerState, NULL, NULL ) ) )
        {
            if( !csrIsConnStateDisconnected(pMac,sessionId) || ( ePMC_LOW_POWER != PowerState ) )
            {
                status = WLANBAL_WriteMemory( pvosGCTx, memAddr, ( void * )( pBuf ), nLen );
            }
        }
        sme_ReleaseGlobalLock( &pMac->sme );
    }
    }
    return ( status );
}


void smsLog(tpAniSirGlobal pMac, tANI_U32 loglevel, const char *pString,...) 
{	
#ifdef WLAN_DEBUG
    // Verify against current log level
    if ( loglevel > pMac->utils.gLogDbgLevel[LOG_INDEX_FOR_MODULE( SIR_SMS_MODULE_ID )] )
        return;
    else
    {
        va_list marker;

        va_start( marker, pString );     /* Initialize variable arguments. */

        logDebug(pMac, SIR_SMS_MODULE_ID, loglevel, pString, marker);

        va_end( marker );              /* Reset variable arguments.      */
    }
#endif
}
/* ---------------------------------------------------------------------------
    \fn sme_GetFwVersion
    \brief  This API returns the firmware version.
    \param  hHal - The handle returned by macOpen.
    \param  version - Points to the FwVersionInfo structure.
    \return VOS_STATUS
            VOS_STATUS_E_INVAL - failure
            VOS_STATUS_SUCCESS  success
  ---------------------------------------------------------------------------*/

VOS_STATUS sme_GetFwVersion (tHalHandle hHal,FwVersionInfo *pVersion)
{
    VOS_STATUS status = VOS_STATUS_SUCCESS;
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

    if ( eHAL_STATUS_SUCCESS == sme_AcquireGlobalLock( &pMac->sme ) )
    {
        if( pVersion != NULL ) {
            vos_mem_copy((v_VOID_t*)pVersion,(v_VOID_t*)&pMac->hal.FwParam.fwVersion, sizeof(FwVersionInfo));
        }
        else {
            status = VOS_STATUS_E_INVAL;
        }
        sme_ReleaseGlobalLock( &pMac->sme );
    }

    return (status);
}

#ifdef FEATURE_WLAN_WAPI
/* ---------------------------------------------------------------------------
    \fn sme_RoamSetBKIDCache
    \brief The SME API exposed to HDD to allow HDD to provde SME the BKID
    candidate list.
    \param hHal - Handle to the HAL. The HAL handle is returned by the HAL after
    it is opened (by calling halOpen).
    \param pBKIDCache - caller allocated buffer point to an array of tBkidCacheInfo
    \param numItems - a variable that has the number of tBkidCacheInfo allocated
    when retruning, this is the number of items put into pBKIDCache
    \return eHalStatus - when fail, it usually means the buffer allocated is not
    big enough and pNumItems has the number of tBkidCacheInfo.
  ---------------------------------------------------------------------------*/
eHalStatus sme_RoamSetBKIDCache( tHalHandle hHal, tANI_U32 sessionId, tBkidCacheInfo *pBKIDCache,
                                 tANI_U32 numItems )
{
   eHalStatus status = eHAL_STATUS_FAILURE;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   status = sme_AcquireGlobalLock( &pMac->sme );
   if ( HAL_STATUS_SUCCESS( status ) )
   {
       status = csrRoamSetBKIDCache( pMac, sessionId, pBKIDCache, numItems );
       sme_ReleaseGlobalLock( &pMac->sme );
   }

   return (status);
}

/* ---------------------------------------------------------------------------
    \fn sme_RoamGetBKIDCache
    \brief The SME API exposed to HDD to allow HDD to request SME to return its
    BKID cache.
    \param hHal - Handle to the HAL. The HAL handle is returned by the HAL after
    it is opened (by calling halOpen).
    \param pNum - caller allocated memory that has the space of the number of
    tBkidCacheInfo as input. Upon returned, *pNum has the needed number of entries
    in SME cache.
    \param pBkidCache - Caller allocated memory that contains BKID cache, if any,
    upon return
    \return eHalStatus - when fail, it usually means the buffer allocated is not
    big enough.
  ---------------------------------------------------------------------------*/
eHalStatus sme_RoamGetBKIDCache(tHalHandle hHal, tANI_U32 *pNum,
                                tBkidCacheInfo *pBkidCache)
{
   eHalStatus status = eHAL_STATUS_FAILURE;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   status = sme_AcquireGlobalLock( &pMac->sme );
   if ( HAL_STATUS_SUCCESS( status ) )
   {
       smsLog(pMac, LOGE, FL(" !!!!!!!!!!!!!!!!!!SessionId is hardcoded\n"));
       status = csrRoamGetBKIDCache( pMac, 0, pNum, pBkidCache );
       sme_ReleaseGlobalLock( &pMac->sme );
   }

   return (status);
}

/* ---------------------------------------------------------------------------
    \fn sme_RoamGetNumBKIDCache
    \brief The SME API exposed to HDD to allow HDD to request SME to return the
    number of BKID cache entries.
    \param hHal - Handle to the HAL. The HAL handle is returned by the HAL after
    it is opened (by calling halOpen).
    \return tANI_U32 - the number of BKID cache entries.
  ---------------------------------------------------------------------------*/
tANI_U32 sme_RoamGetNumBKIDCache(tHalHandle hHal, tANI_U32 sessionId)
{
   eHalStatus status = eHAL_STATUS_FAILURE;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );
   tANI_U32 numBkidCache = 0;

   status = sme_AcquireGlobalLock( &pMac->sme );
   if ( HAL_STATUS_SUCCESS( status ) )
   {
       numBkidCache = csrRoamGetNumBKIDCache( pMac, sessionId );
       sme_ReleaseGlobalLock( &pMac->sme );
   }

   return (numBkidCache);
}

/* ---------------------------------------------------------------------------
    \fn sme_ScanGetBKIDCandidateList
    \brief a wrapper function to return the BKID candidate list
    \param pBkidList - caller allocated buffer point to an array of
                        tBkidCandidateInfo
    \param pNumItems - pointer to a variable that has the number of
                       tBkidCandidateInfo allocated when retruning, this is
                       either the number needed or number of items put into
                       pPmkidList
    \return eHalStatus - when fail, it usually means the buffer allocated is not
                         big enough and pNumItems
    has the number of tBkidCandidateInfo.
    \Note: pNumItems is a number of tBkidCandidateInfo,
           not sizeof(tBkidCandidateInfo) * something
  ---------------------------------------------------------------------------*/
eHalStatus sme_ScanGetBKIDCandidateList(tHalHandle hHal, tANI_U32 sessionId,
                                        tBkidCandidateInfo *pBkidList,
                                        tANI_U32 *pNumItems )
{
    eHalStatus status = eHAL_STATUS_FAILURE;
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

    status = sme_AcquireGlobalLock( &pMac->sme );
    if ( HAL_STATUS_SUCCESS( status ) )
    {
        status = csrScanGetBKIDCandidateList( pMac, sessionId, pBkidList, pNumItems );
        sme_ReleaseGlobalLock( &pMac->sme );
    }

    return (status);
}
#endif /* FEATURE_WLAN_WAPI */

#ifdef FEATURE_INNAV_SUPPORT

/*****************************************************************************
 INNAV related modifications and function additions
 *****************************************************************************/

/* ---------------------------------------------------------------------------
    \fn sme_getInNavMeasurementResult
    \brief a wrapper function to obtain the RSSI/RTT measurement results
    \param pInNavMeasRsp - A pointer to the response object
    \param pContext - a pointer passed in for the callback
    \return eHalStatus
  ---------------------------------------------------------------------------*/
eHalStatus sme_getInNavMeasurementResult(tHalHandle hHal,
        tInNavMeasurementResponse **pInNavMeasRsp)
{
    eHalStatus status = eHAL_STATUS_SUCCESS;
    tpAniSirGlobal pMac = PMAC_STRUCT(hHal);

    do
    {
        //acquire the lock for the sme object
        status = sme_AcquireGlobalLock(&pMac->sme);

        if(!HAL_STATUS_SUCCESS(status))
        {
            break;
        }

        if(pMac->innavMeas.pMeasurementResult != NULL)
        {
            *pInNavMeasRsp = pMac->innavMeas.pMeasurementResult;
        }
        else
        {
            status = eHAL_STATUS_FAILURE;
        }

        //release the lock for the sme object
        sme_ReleaseGlobalLock( &pMac->sme );

    } while(0);

    return status;
}

/* ---------------------------------------------------------------------------
    \fn sme_InNavMeasurementRequest
    \brief a wrapper function to Request RSSI/RTT measurements
    \param sessionId - session id to be used for measurement.
    \param pMeasurementRequestID - pointer to an object to get back the request ID
    \param callback - a callback function that meas calls upon finish, will not
                      be called if measMeasurementRequest returns error
    \param pContext - a pointer passed in for the callback
    \return eHalStatus
  ---------------------------------------------------------------------------*/
eHalStatus sme_InNavMeasurementRequest(tHalHandle hHal, 
        tANI_U8 sessionId,
        tInNavMeasurementConfig *pInNavMeasConfig, 
        tANI_U32 *pMeasurementRequestID, 
        measMeasurementCompleteCallback callback, 
        void *pContext)
{
    eHalStatus status = eHAL_STATUS_SUCCESS;
    tpAniSirGlobal pMac = PMAC_STRUCT(hHal);

    smsLog(pMac, LOG1, "%s new innav measurement request\n", __FUNCTION__);
    smsLog(pMac, LOG1, "%s sessionId               = %d\n", __FUNCTION__, sessionId);
    smsLog(pMac, LOG1, "%s #bssids                 = %u\n", __FUNCTION__, pInNavMeasConfig->numBSSIDs);
    smsLog(pMac, LOG1, "%s #measurements           = %u\n", __FUNCTION__, pInNavMeasConfig->numInNavMeasurements);
    smsLog(pMac, LOG1, "%s #repetetions            = %u\n", __FUNCTION__, pInNavMeasConfig->numSetRepetitions);
    smsLog(pMac, LOG1, "%s time-interval (ms)      = %u\n", __FUNCTION__, pInNavMeasConfig->measurementTimeInterval);

    do
    {
        //acquire the lock for the sme object
        status = sme_AcquireGlobalLock(&pMac->sme);
        if(HAL_STATUS_SUCCESS(status))
        {
            tANI_U32 lMeasId = pMac->innavMeas.nextMeasurementId++; //let it wrap around

	    if(pMeasurementRequestID)
	    {
		    *pMeasurementRequestID = lMeasId;
	    }
	    else
	    {
	        return eHAL_STATUS_FAILURE;
	    }
            status = measInNavMeasurementRequest(hHal, sessionId, pInNavMeasConfig, pMeasurementRequestID, callback, pContext);

            //release the lock for the sme object
            sme_ReleaseGlobalLock( &pMac->sme );
        }
    } while(0);

    smsLog(pMac, LOGW, "exiting function %s\n", __FUNCTION__);

    return(status);
}

#endif /*FEATURE_INNAV_SUPPORT*/

/*--------------------------------------------------------------------------

  \brief sme_OpenSession() - Open a session for scan/roam operation.

  This is a synchronous API.


  \param hHal - The handle returned by macOpen.
  \param callback - A pointer to the function caller specifies for roam/connect status indication
  \param pContext - The context passed with callback
  \param pSelfMacAddr - Caller allocated memory filled with self MAC address (6 bytes)
  \param pbSessionId - pointer to a caller allocated buffer for returned session ID

  \return eHAL_STATUS_SUCCESS - session is opened. sessionId returned.

          Other status means SME is failed to open the session.
          eHAL_STATUS_RESOURCES - no more session available.
  \sa

  --------------------------------------------------------------------------*/
eHalStatus sme_OpenSession(tHalHandle hHal, csrRoamCompleteCallback callback, void *pContext,
                           tANI_U8 *pSelfMacAddr, tANI_U8 *pbSessionId)
{
   eHalStatus status;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   if( NULL == pbSessionId )
   {
      status = eHAL_STATUS_INVALID_PARAMETER;
   }
   else
   {
      status = sme_AcquireGlobalLock( &pMac->sme );
      if ( HAL_STATUS_SUCCESS( status ) )
      {
         status = csrRoamOpenSession( pMac, callback, pContext, pSelfMacAddr, pbSessionId );

         sme_ReleaseGlobalLock( &pMac->sme );
      }
   }

   return ( status );
}


/*--------------------------------------------------------------------------

  \brief sme_CloseSession() - Open a session for scan/roam operation.

  This is a synchronous API.


  \param hHal - The handle returned by macOpen.

  \param sessionId - A previous opened session's ID.

  \return eHAL_STATUS_SUCCESS - session is closed.

          Other status means SME is failed to open the session.
          eHAL_STATUS_INVALID_PARAMETER - session is not opened.
  \sa

  --------------------------------------------------------------------------*/
eHalStatus sme_CloseSession(tHalHandle hHal, tANI_U8 sessionId,
                          csrRoamSessionCloseCallback callback, void *pContext)
{
   eHalStatus status;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   status = sme_AcquireGlobalLock( &pMac->sme );
   if ( HAL_STATUS_SUCCESS( status ) )
   {
      status = csrRoamCloseSession( pMac, sessionId, FALSE, 
                                    callback, pContext );

      sme_ReleaseGlobalLock( &pMac->sme );
   }

   return ( status );
}

#ifdef WLAN_SOFTAP_FEATURE
/* ---------------------------------------------------------------------------

    \fn sme_RoamUpdateAPWPSIE

    \brief To update AP's WPS IE. This function should be called after SME AP session is created
    This is an asynchronous API.

    \param pAPWPSIES - pointer to a caller allocated object of tSirAPWPSIEs

    \return eHalStatus  SUCCESS 

                         FAILURE or RESOURCES  The API finished and failed.

  -------------------------------------------------------------------------------*/
eHalStatus sme_RoamUpdateAPWPSIE(tHalHandle hHal, tANI_U8 sessionId, tSirAPWPSIEs *pAPWPSIES)
{

   eHalStatus status = eHAL_STATUS_FAILURE;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   status = sme_AcquireGlobalLock( &pMac->sme );
   if ( HAL_STATUS_SUCCESS( status ) )
   {

      status = csrRoamUpdateAPWPSIE( pMac, sessionId, pAPWPSIES );

      sme_ReleaseGlobalLock( &pMac->sme );
   }

   return (status);
}
/* ---------------------------------------------------------------------------

    \fn sme_RoamUpdateAPWPARSNIEs

    \brief To update AP's WPA/RSN IEs. This function should be called after SME AP session is created
    This is an asynchronous API.

    \param pAPSirRSNie - pointer to a caller allocated object of tSirRSNie with WPS/RSN IEs

    \return eHalStatus  SUCCESS 

                         FAILURE or RESOURCES  The API finished and failed.

  -------------------------------------------------------------------------------*/
eHalStatus sme_RoamUpdateAPWPARSNIEs(tHalHandle hHal, tANI_U8 sessionId, tSirRSNie * pAPSirRSNie)
{

   eHalStatus status = eHAL_STATUS_FAILURE;
   tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

   status = sme_AcquireGlobalLock( &pMac->sme );
   if ( HAL_STATUS_SUCCESS( status ) )
   {

      status = csrRoamUpdateWPARSNIEs( pMac, sessionId, pAPSirRSNie);

      sme_ReleaseGlobalLock( &pMac->sme );
   }

   return (status);
}
#endif


/*-------------------------------------------------------------------------------*

  \fn sme_sendBTAmpEvent

  \brief to receive the coex priorty request from BT-AMP PAL
  and send the BT_AMP link state to HAL

  \param btAmpEvent - btAmpEvent

  \return eHalStatus: SUCESS : BTAmp event successfully sent to HAL

                      FAILURE: API failed

-------------------------------------------------------------------------------*/

eHalStatus	sme_sendBTAmpEvent(tHalHandle hHal, tSmeBtAmpEvent btAmpEvent)
{
  vos_msg_t msg;
  tpSmeBtAmpEvent ptrSmeBtAmpEvent = NULL;
  eHalStatus status = eHAL_STATUS_FAILURE;

  ptrSmeBtAmpEvent = vos_mem_malloc(sizeof(tpSmeBtAmpEvent));
  if (NULL == ptrSmeBtAmpEvent)
     {
        VOS_TRACE(VOS_MODULE_ID_SME, VOS_TRACE_LEVEL_ERROR, "%s: "
           "Not able to allocate memory for BTAmp event", __FUNCTION__);
        return status;
   }

  vos_mem_copy(ptrSmeBtAmpEvent, (void*)&btAmpEvent, sizeof(tSmeBtAmpEvent));
  msg.type = SIR_HAL_SIGNAL_BTAMP_EVENT;
  msg.reserved = 0;
  msg.bodyptr = ptrSmeBtAmpEvent;

  //status = halFW_SendBTAmpEventMesg(pMac, event);

  if(VOS_STATUS_SUCCESS != vos_mq_post_message(VOS_MODULE_ID_HAL, &msg))
  {
    VOS_TRACE(VOS_MODULE_ID_SME, VOS_TRACE_LEVEL_ERROR, "%s: "
           "Not able to post SIR_HAL_SIGNAL_BTAMP_EVENT message to HAL", __FUNCTION__);
    vos_mem_free(ptrSmeBtAmpEvent);
    return status;
  }

  return eHAL_STATUS_SUCCESS;

}


/* ---------------------------------------------------------------------------
    \fn sme_SetHostOffload
    \brief  API to set the host offload feature.
    \param  hHal - The handle returned by macOpen.
    \param  pRequest -  Pointer to the offload request.
    \return eHalStatus
  ---------------------------------------------------------------------------*/
eHalStatus sme_SetHostOffload (tHalHandle hHal, tpSirHostOffloadReq pRequest)
{
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );
    eHalStatus status;

    if ( eHAL_STATUS_SUCCESS == ( status = sme_AcquireGlobalLock( &pMac->sme ) ) )
    {
        status = pmcSetHostOffload (hHal, pRequest);
        sme_ReleaseGlobalLock( &pMac->sme );
    }

    return (status);
}

/* ---------------------------------------------------------------------------
    \fn sme_AbortMacScan
    \brief  API to cancel MAC scan.
    \param  hHal - The handle returned by macOpen.
    \return VOS_STATUS
            VOS_STATUS_E_FAILURE - failure
            VOS_STATUS_SUCCESS  success
  ---------------------------------------------------------------------------*/
eHalStatus sme_AbortMacScan(tHalHandle hHal)
{
    eHalStatus status;
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

    status = sme_AcquireGlobalLock( &pMac->sme );
    if ( HAL_STATUS_SUCCESS( status ) )
    {
       status = csrScanAbortMacScan(pMac);
    
       sme_ReleaseGlobalLock( &pMac->sme );
    }
    
    return ( status );
}

/* ----------------------------------------------------------------------------
        \fn sme_GetOperationChannel
        \brief API to get current channel on which STA is parked
	this function gives channel information only of infra station or IBSS station
        \param hHal and poiter to memory location
        \returns eHAL_STATUS_SUCCESS
                eHAL_STATUS_FAILURE
-------------------------------------------------------------------------------*/
eHalStatus sme_GetOperationChannel(tHalHandle hHal, tANI_U32 *pChannel)
{
    tANI_U32 sessionId;
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );
    tCsrRoamSession *pSession;

    for (sessionId = 0; sessionId < CSR_ROAM_SESSION_MAX ; sessionId++)
    {
       if (CSR_IS_SESSION_VALID( pMac, sessionId ))
       {
          pSession = CSR_GET_SESSION( pMac, sessionId );

          if(( pSession->connectedProfile.BSSType == eCSR_BSS_TYPE_INFRASTRUCTURE ) || 
             ( pSession->connectedProfile.BSSType == eCSR_BSS_TYPE_IBSS ) ||
             ( pSession->connectedProfile.BSSType == eCSR_BSS_TYPE_START_IBSS ))
          {
              *pChannel =pSession->connectedProfile.operationChannel;
              return eHAL_STATUS_SUCCESS;
          }
       }
    }
    return eHAL_STATUS_FAILURE;
}// sme_GetOperationChannel ends here

#ifdef WLAN_FEATURE_P2P
/* ---------------------------------------------------------------------------

    \fn sme_RegisterMgtFrame

    \brief To register managment frame of specified type and subtype. 
    \param frameType - type of the frame that needs to be passed to HDD.
    \param matchData - data which needs to be matched before passing frame 
                       to HDD. 
    \param matchDataLen - Length of matched data.
    \return eHalStatus 
  -------------------------------------------------------------------------------*/
eHalStatus sme_RegisterMgmtFrame(tHalHandle hHal, tANI_U8 sessionId, 
                     tANI_U16 frameType, tANI_U8* matchData, tANI_U16 matchLen)
{
    eHalStatus status = eHAL_STATUS_SUCCESS;
    tpAniSirGlobal pMac = PMAC_STRUCT(hHal);
	
    if ( eHAL_STATUS_SUCCESS == ( status = sme_AcquireGlobalLock( &pMac->sme ) ) )
    {
        tSirRegisterMgmtFrame *pMsg;
        tANI_U16 len;
        tCsrRoamSession *pSession = CSR_GET_SESSION( pMac, sessionId );

        if( NULL != pSession )
        {
            if( !pSession->sessionActive )
                VOS_ASSERT(0);
        }
        else
        {
            VOS_ASSERT(0);
            return eHAL_STATUS_FAILURE;
        }

        len = sizeof(tSirRegisterMgmtFrame) + matchLen;
        
        status = palAllocateMemory(pMac->hHdd, (void**)&pMsg, len );
        if(HAL_STATUS_SUCCESS(status))
        {
            palZeroMemory(pMac->hHdd, pMsg, len);
            pMsg->messageType     = eWNI_SME_REGISTER_MGMT_FRAME_REQ;
            pMsg->length          = len;
            pMsg->sessionId       = sessionId;
            pMsg->registerFrame	  = VOS_TRUE;
            pMsg->frameType       = frameType;
            pMsg->matchLen	  = matchLen;
            palCopyMemory( pMac, pMsg->matchData, matchData, matchLen); 
            status = palSendMBMessage(pMac->hHdd, pMsg);
        }
        sme_ReleaseGlobalLock( &pMac->sme );
    }
    return status;
}

/* ---------------------------------------------------------------------------

    \fn sme_DeregisterMgtFrame

    \brief To De-register managment frame of specified type and subtype. 
    \param frameType - type of the frame that needs to be passed to HDD.
    \param matchData - data which needs to be matched before passing frame 
                       to HDD. 
    \param matchDataLen - Length of matched data.
    \return eHalStatus 
  -------------------------------------------------------------------------------*/
eHalStatus sme_DeregisterMgmtFrame(tHalHandle hHal, tANI_U8 sessionId, 
                     tANI_U16 frameType, tANI_U8* matchData, tANI_U16 matchLen)
{
    eHalStatus status = eHAL_STATUS_SUCCESS;
    tpAniSirGlobal pMac = PMAC_STRUCT(hHal);
	
    if ( eHAL_STATUS_SUCCESS == ( status = sme_AcquireGlobalLock( &pMac->sme ) ) )
    {
        tSirRegisterMgmtFrame *pMsg;
        tANI_U16 len;
        tCsrRoamSession *pSession = CSR_GET_SESSION( pMac, sessionId );

        if( NULL == pSession )
        {
            VOS_ASSERT(0);
            return eHAL_STATUS_FAILURE;
        }
        if( !pSession->sessionActive ) 
            VOS_ASSERT(0);
        
        len = sizeof(tSirRegisterMgmtFrame) + matchLen;
        
        status = palAllocateMemory(pMac->hHdd, (void**)&pMsg, len );
        if(HAL_STATUS_SUCCESS(status))
        {
            palZeroMemory(pMac->hHdd, pMsg, len);
            pMsg->messageType     = eWNI_SME_REGISTER_MGMT_FRAME_REQ;
            pMsg->length          = len; 
            pMsg->registerFrame	  = VOS_FALSE;
            pMsg->frameType       = frameType;
            pMsg->matchLen	  = matchLen;
            palCopyMemory( pMac, pMsg->matchData, matchData, matchLen); 
            status = palSendMBMessage(pMac->hHdd, pMsg);
        }
        sme_ReleaseGlobalLock( &pMac->sme );
    }
    return status;
}

/* ---------------------------------------------------------------------------
    \fn sme_RemainOnChannel
    \brief  API to request remain on channel for 'x' duration. used in p2p in listen state
    \param  hHal - The handle returned by macOpen.
    \param  pRequest -  channel
    \param  duration - duration in ms
    \param callback - HDD registered callback to process reaminOnChannelRsp
    \param context - HDD Callback param
    \return eHalStatus
  ---------------------------------------------------------------------------*/

eHalStatus sme_RemainOnChannel(tHalHandle hHal, tANI_U8 sessionId,
	     tANI_U8 channel, tANI_U32 duration,
        remainOnChanCallback callback, 
        void *pContext)
{
  eHalStatus status = eHAL_STATUS_SUCCESS;
  tpAniSirGlobal pMac = PMAC_STRUCT(hHal);

  if ( eHAL_STATUS_SUCCESS == ( status = sme_AcquireGlobalLock( &pMac->sme ) ) )
  {
    status = p2pRemainOnChannel (hHal, sessionId, channel, duration, callback, pContext);
    sme_ReleaseGlobalLock( &pMac->sme );
  }
  return(status);
}

/* ---------------------------------------------------------------------------
    \fn sme_ReportProbeReq
    \brief  API to enable/disable forwarding of probeReq to apps in p2p.
    \param  hHal - The handle returned by macOpen.
    \param  falg: to set the Probe request forarding to wpa_supplicant in listen state in p2p
    \return eHalStatus
  ---------------------------------------------------------------------------*/

eHalStatus sme_ReportProbeReq(tHalHandle hHal,
	     tANI_U8 flag)
{
    eHalStatus status = eHAL_STATUS_SUCCESS;
    tpAniSirGlobal pMac = PMAC_STRUCT(hHal);

    do
    {
        //acquire the lock for the sme object
        status = sme_AcquireGlobalLock(&pMac->sme);
        if(HAL_STATUS_SUCCESS(status))
        {
            /* call set in context */
				pMac->p2pContext.probeReqForwarding = flag;
            //release the lock for the sme object
            sme_ReleaseGlobalLock( &pMac->sme );
        }
    } while(0);

    smsLog(pMac, LOGW, "exiting function %s\n", __FUNCTION__);

    return(status);
}

/* ---------------------------------------------------------------------------
    \fn sme_updateP2pIe
    \brief  API to set the P2p Ie in p2p context
    \param  hHal - The handle returned by macOpen.
    \param  p2pIe -  Ptr to p2pIe from HDD.
    \param p2pIeLength: length of p2pIe
    \return eHalStatus
  ---------------------------------------------------------------------------*/

eHalStatus sme_updateP2pIe(tHalHandle hHal,
	     void *p2pIe, tANI_U32 p2pIeLength)
{
    eHalStatus status = eHAL_STATUS_SUCCESS;
    tpAniSirGlobal pMac = PMAC_STRUCT(hHal);
	 
	  //acquire the lock for the sme object
	  status = sme_AcquireGlobalLock(&pMac->sme);
	  if(HAL_STATUS_SUCCESS(status))
	  {
	     if(NULL != pMac->p2pContext.probeRspIe){
		  	  vos_mem_free(pMac->p2pContext.probeRspIe);
          pMac->p2pContext.probeRspIeLength = 0;
	     }
	     pMac->p2pContext.probeRspIe = vos_mem_malloc(p2pIeLength);
         if (NULL == pMac->p2pContext.probeRspIe)
         {
             VOS_TRACE(VOS_MODULE_ID_SME, VOS_TRACE_LEVEL_ERROR, "%s: "
                            "Not able to allocate memory for probeRspIe", __FUNCTION__);
             return eHAL_STATUS_FAILURE;
         }
         pMac->p2pContext.probeRspIeLength = p2pIeLength;

         sirDumpBuf( pMac, SIR_LIM_MODULE_ID, LOGE, pMac->p2pContext.probeRspIe, pMac->p2pContext.probeRspIeLength ); 
		 vos_mem_copy((tANI_U8 *)pMac->p2pContext.probeRspIe , p2pIe,
                p2pIeLength);

		 //release the lock for the sme object
	     sme_ReleaseGlobalLock( &pMac->sme );
	  }
   
    smsLog(pMac, LOGW, "exiting function %s\n", __FUNCTION__);

    return(status);
}

/* ---------------------------------------------------------------------------
    \fn sme_sendAction
    \brief  API to send action frame from supplicant.
    \param  hHal - The handle returned by macOpen.
    \return eHalStatus
  ---------------------------------------------------------------------------*/

eHalStatus sme_sendAction(tHalHandle hHal, tANI_U8 sessionId,
                          const tANI_U8 *pBuf, tANI_U32 len,
                          tANI_BOOLEAN noack)
{
    eHalStatus status = eHAL_STATUS_SUCCESS;
    tpAniSirGlobal pMac = PMAC_STRUCT(hHal);

    //acquire the lock for the sme object
    status = sme_AcquireGlobalLock(&pMac->sme);
    if(HAL_STATUS_SUCCESS(status))
    {
        p2pSendAction(hHal, sessionId, pBuf, len, noack);
        //release the lock for the sme object
        sme_ReleaseGlobalLock( &pMac->sme );
    }

    smsLog(pMac, LOGW, "exiting function %s\n", __FUNCTION__);

    return(status);
}

eHalStatus sme_CancelRemainOnChannel(tHalHandle hHal, tANI_U8 sessionId )
{
  eHalStatus status = eHAL_STATUS_SUCCESS;
  tpAniSirGlobal pMac = PMAC_STRUCT(hHal);

  if ( eHAL_STATUS_SUCCESS == ( status = sme_AcquireGlobalLock( &pMac->sme ) ) )
  {
    status = p2pCancelRemainOnChannel (hHal, sessionId);
    sme_ReleaseGlobalLock( &pMac->sme );
  }
  return(status);
}

//Power Save Related
eHalStatus sme_p2pSetPs(tHalHandle hHal, tP2pPsConfig * data)
{
  eHalStatus status = eHAL_STATUS_SUCCESS;
  tpAniSirGlobal pMac = PMAC_STRUCT(hHal);

  if ( eHAL_STATUS_SUCCESS == ( status = sme_AcquireGlobalLock( &pMac->sme ) ) )
  {
    status = p2pSetPs (hHal, data);
    sme_ReleaseGlobalLock( &pMac->sme );
  }
  return(status);
}

#endif

