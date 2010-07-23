#ifndef __SYS_GLOBAL_H__
#define __SYS_GLOBAL_H__

typedef struct sAniSirSys
{
    tANI_U32 abort; /* system is aborting and will be unloaded, only MMH thread is running */

    // Radio ID
    tANI_U32 gSirRadioId;

    tANI_U32 gSysFrameCount[4][16];
    tANI_U32 gSysBbtReceived;
    tANI_U32 gSysBbtPostedToLim;
    tANI_U32 gSysBbtPostedToSch;
    tANI_U32 gSysBbtPostedToPmm;
    tANI_U32 gSysBbtPostedToHal;
    tANI_U32 gSysBbtDropped;
    tANI_U32 gSysBbtNonLearnFrameInv;
    tANI_U32 gSysBbtLearnFrameInv;
    tANI_U32 gSysBbtCrcFail;
    tANI_U32 gSysBbtDuplicates;
    tANI_U32 gSysReleaseCount;
    tANI_U32 probeError, probeBadSsid, probeIgnore, probeRespond;

    tANI_U32 gSysFramesSent[4][16];

    tANI_U32 gSysEnableLearnMode;
    tANI_U32 gSysEnableScanMode;
    tANI_U32 gSysEnableLinkMonitorMode;

    tANI_U32 gSysResetCounts;

    TX_THREAD gSirSchThread;
    TX_THREAD gSirPmmThread;
    TX_THREAD gSirLimThread;
    TX_THREAD gSirHalThread;
    TX_THREAD gSirMntThread;
    TX_THREAD gSirMmhThread;

    TX_QUEUE gSirHalMsgQ;            // Message Queue variables
    TX_QUEUE gSirMntMsgQ;
    TX_QUEUE gSirLimMsgQ;
    TX_QUEUE gSirLimDeferredMsgQ;
    TX_QUEUE gSirSchMsgQ;
    TX_QUEUE gSirPmmMsgQ;

#if defined(ANI_MANF_DIAG) || defined(ANI_PHY_DEBUG)
    TX_THREAD gSirNimPttThread;
    TX_QUEUE gSirNimRDMsgQ;
#endif

    TX_QUEUE gSirHalEvtQ;
    TX_QUEUE gSirTxMsgQ;
    TX_QUEUE gSirRxMsgQ;

    // The system is based on the HAL states. The following static definition
    // of the HAL state.
    tANI_U16 gSirThreadCount;

    tANI_U8 debugOnReset;

    tANI_BOOLEAN fTestRadar;
    tANI_U8   detRadarChIds[20];
    tANI_U32  radarDetectCount;
    tANI_U8   radarDetected;
    tANI_U8   gSysdropLimPkts;
} tAniSirSys, *tpAniSirSys;

#endif
