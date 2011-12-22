/*
 * Airgo Networks, Inc proprietary. All rights reserved.
 * This file limSerDesUtils.h contains the utility definitions
 * LIM uses while processing messages from upper layer software
 * modules
 * Author:        Chandra Modumudi
 * Date:          10/20/02
 * History:-
 * Date           Modified by    Modification Information
 * --------------------------------------------------------------------
 */
#ifndef __LIM_SERDES_UTILS_H
#define __LIM_SERDES_UTILS_H

#include "sirApi.h"
#include "aniSystemDefs.h"
#include "sirMacProtDef.h"
#include "utilsApi.h"
#include "limTypes.h"
#include "limPropExtsUtils.h"

tSirRetStatus   limStartBssReqSerDes(tpAniSirGlobal, tpSirSmeStartBssReq, tANI_U8 *);
tSirRetStatus   limStopBssReqSerDes(tpAniSirGlobal, tpSirSmeStopBssReq, tANI_U8 *);
tSirRetStatus   limJoinReqSerDes(tpAniSirGlobal, tpSirSmeJoinReq, tANI_U8 *);
void            limAssocIndSerDes(tpAniSirGlobal, tpLimMlmAssocInd, tANI_U8 *, tpPESession);
void            limReassocIndSerDes(tpAniSirGlobal, tpLimMlmReassocInd, tANI_U8 *, tpPESession psessionEntry);
tSirRetStatus   limAssocCnfSerDes(tpAniSirGlobal, tpSirSmeAssocCnf, tANI_U8 *);
tSirRetStatus   limDisassocCnfSerDes(tpAniSirGlobal, tpSirSmeDisassocCnf, tANI_U8 *);
tSirRetStatus   limSetContextReqSerDes(tpAniSirGlobal, tpSirSmeSetContextReq, tANI_U8 *);
tSirRetStatus   limDisassocReqSerDes(tpAniSirGlobal, tSirSmeDisassocReq *, tANI_U8 *);
tSirRetStatus   limDeauthReqSerDes(tpAniSirGlobal, tSirSmeDeauthReq *, tANI_U8 *);
void            limAuthIndSerDes(tpAniSirGlobal, tpLimMlmAuthInd, tANI_U8 *);
void            limStatSerDes(tpAniSirGlobal, tpAniStaStatStruct, tANI_U8 *);
void            limGetSessionInfo(tpAniSirGlobal pMac, tANI_U8 *, tANI_U8 *, tANI_U16 *);

#if (WNI_POLARIS_FW_PACKAGE == ADVANCED) && (WNI_POLARIS_FW_PRODUCT == AP)
tSirRetStatus   limMeasurementReqSerDes(tpAniSirGlobal, tpSirSmeMeasurementReq, tANI_U8 *);
void            limMeasurementIndSerDes(tpAniSirGlobal, tANI_U8 *);
void            limCopyNeighborInfoToCfg(tpAniSirGlobal, tSirNeighborBssInfo);
tSirRetStatus   limWdsReqSerDes(tpAniSirGlobal, tpSirSmeSetWdsInfoReq, tANI_U8 *); 
tSirRetStatus   limRadioInfoSerDes(tpAniSirGlobal, tpSirRadarInfo, tANI_U8 *, tANI_U16 *, tANI_U32);
tSirRetStatus   limSmeWmStatusChangeHeaderSerDes(tpAniSirGlobal, tSirSmeStatusChangeCode, tANI_U8 *, tANI_U16 *, tANI_U32);
tSirRetStatus   nonTitanBssFoundSerDes( tpAniSirGlobal, tpSirNeighborBssWdsInfo, tANI_U8 *, tANI_U16 * );
#endif

void            limPackBkgndScanFailNotify(tpAniSirGlobal, tSirSmeStatusChangeCode, 
                                           tpSirBackgroundScanInfo, tSirSmeWmStatusChangeNtf *);

#if (WNI_POLARIS_FW_PACKAGE == ADVANCED)
tANI_U32 limCopyNeighborBssInfo(tpAniSirGlobal, tANI_U8 *, tpSirNeighborBssInfo);
tANI_U32 limGetNeighborBssInfo(tpAniSirGlobal, tpSirNeighborBssInfo, tANI_U8 *);
#endif

tSirRetStatus limRemoveKeyReqSerDes(tpAniSirGlobal pMac, tpSirSmeRemoveKeyReq pRemoveKeyReq, tANI_U8 * pBuf);

#ifdef WLAN_SOFTAP_FEATURE
tANI_BOOLEAN    limIsSmeGetAssocSTAsReqValid(tpAniSirGlobal pMac, tpSirSmeGetAssocSTAsReq pGetAssocSTAsReq, tANI_U8 *pBuf);
tSirRetStatus   limTkipCntrMeasReqSerDes(tpAniSirGlobal pMac, tpSirSmeTkipCntrMeasReq  ptkipCntrMeasReq, tANI_U8 *pBuf);

tSirRetStatus limUpdateAPWPSIEsReqSerDes(tpAniSirGlobal pMac, tpSirUpdateAPWPSIEsReq pUpdateAPWPSIEsReq, tANI_U8 *pBuf);
tSirRetStatus limUpdateAPWPARSNIEsReqSerDes(tpAniSirGlobal pMac, tpSirUpdateAPWPARSNIEsReq pUpdateAPWPARSNIEsReq, tANI_U8 *pBuf);
#endif

#if (WNI_POLARIS_FW_PACKAGE == ADVANCED) && (WNI_POLARIS_FW_PRODUCT == AP)
tANI_BOOLEAN limIsSmeSwitchChannelReqValid(tpAniSirGlobal, tANI_U8 *, tpSirSmeSwitchChannelReq);
#endif

// Byte String <--> tANI_U16/tANI_U32 copy functions
static inline void limCopyU16(tANI_U8 *ptr, tANI_U16 u16Val)
{
#if defined(ANI_PRODUCT_TYPE_AP)
    *ptr++ = (tANI_U8) ((u16Val >> 8) & 0xff);    
    *ptr   = (tANI_U8) (u16Val & 0xff);
#elif ((defined(ANI_OS_TYPE_OSX) && defined(ANI_LITTLE_BYTE_ENDIAN)) || \
       (defined(ANI_OS_TYPE_WINDOWS) && defined(ANI_LITTLE_BYTE_ENDIAN)) || \
       (defined(ANI_OS_TYPE_AMSS) && defined(ANI_LITTLE_BYTE_ENDIAN)) || \
       (defined(ANI_OS_TYPE_ANDROID) && defined(ANI_LITTLE_BYTE_ENDIAN)))
    *ptr++ = (tANI_U8) (u16Val & 0xff);
    *ptr   = (tANI_U8) ((u16Val >> 8) & 0xff);
#else
#error "Unknown combination of OS Type and endianess"
#endif
}
        
static inline tANI_U16 limGetU16(tANI_U8 *ptr)
{
#if defined(ANI_PRODUCT_TYPE_AP)
    return (((tANI_U16) (*ptr << 8)) |
            ((tANI_U16) (*(ptr+1))));
#elif ((defined(ANI_OS_TYPE_OSX) && defined(ANI_LITTLE_BYTE_ENDIAN)) || \
       (defined(ANI_OS_TYPE_WINDOWS) && defined(ANI_LITTLE_BYTE_ENDIAN)) || \
       (defined(ANI_OS_TYPE_AMSS) && defined(ANI_LITTLE_BYTE_ENDIAN)) || \
       (defined(ANI_OS_TYPE_ANDROID) && defined(ANI_LITTLE_BYTE_ENDIAN)))
    return (((tANI_U16) (*(ptr+1) << 8)) |
            ((tANI_U16) (*ptr)));
#else
#error "Unknown combination of OS Type and endianess"
#endif
}

static inline void limCopyU32(tANI_U8 *ptr, tANI_U32 u32Val)
{
#if defined(ANI_PRODUCT_TYPE_AP)
    *ptr++ = (tANI_U8) ((u32Val >> 24) & 0xff);
    *ptr++ = (tANI_U8) ((u32Val >> 16) & 0xff);
    *ptr++ = (tANI_U8) ((u32Val >> 8) & 0xff);
    *ptr   = (tANI_U8) (u32Val & 0xff);
#elif ((defined(ANI_OS_TYPE_OSX) && defined(ANI_LITTLE_BYTE_ENDIAN)) || \
       (defined(ANI_OS_TYPE_WINDOWS) && defined(ANI_LITTLE_BYTE_ENDIAN)) || \
       (defined(ANI_OS_TYPE_AMSS) && defined(ANI_LITTLE_BYTE_ENDIAN)) || \
       (defined(ANI_OS_TYPE_ANDROID) && defined(ANI_LITTLE_BYTE_ENDIAN)))
    *ptr++ = (tANI_U8) (u32Val & 0xff);
    *ptr++ = (tANI_U8) ((u32Val >> 8) & 0xff);
    *ptr++ = (tANI_U8) ((u32Val >> 16) & 0xff);
    *ptr   = (tANI_U8) ((u32Val >> 24) & 0xff);
#else
#error "Unknown combination of OS Type and endianess"
#endif
}

static inline tANI_U32 limGetU32(tANI_U8 *ptr)
{
#if defined(ANI_PRODUCT_TYPE_AP)
    return ((*(ptr) << 24) |
            (*(ptr+1) << 16) |
            (*(ptr+2) << 8) |
            (*(ptr+3)));
#elif ((defined(ANI_OS_TYPE_OSX) && defined(ANI_LITTLE_BYTE_ENDIAN)) || \
       (defined(ANI_OS_TYPE_WINDOWS) && defined(ANI_LITTLE_BYTE_ENDIAN)) || \
       (defined(ANI_OS_TYPE_AMSS) && defined(ANI_LITTLE_BYTE_ENDIAN)) || \
       (defined(ANI_OS_TYPE_ANDROID) && defined(ANI_LITTLE_BYTE_ENDIAN)))
    return ((*(ptr+3) << 24) |
            (*(ptr+2) << 16) |
            (*(ptr+1) << 8) |
            (*(ptr)));
#else
#error "Unknown combination of OS Type and endianess"
#endif
}

#endif /* __LIM_SERDES_UTILS_H */

