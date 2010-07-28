/*
 * Airgo Networks, Inc proprietary. All rights reserved.
 * This file limSmeReqUtils.h contains the utility definitions
 * LIM uses while processing SME request messsages.
 * Author:        Chandra Modumudi
 * Date:          02/13/02
 * History:-
 * Date           Modified by    Modification Information
 * --------------------------------------------------------------------
 */
#ifndef __LIM_SME_REQ_UTILS_H
#define __LIM_SME_REQ_UTILS_H

#include "sirApi.h"
#include "limTypes.h"


// LIM SME requst messages related utilility functions
tANI_U8 limIsSmeStartReqValid(tpAniSirGlobal, tANI_U32 *);
tANI_U8 limIsSmeStartBssReqValid(tpAniSirGlobal, tpSirSmeStartBssReq);
tANI_U8 limIsSmeScanReqValid(tpAniSirGlobal, tpSirSmeScanReq);
tANI_U8 limIsSmeJoinReqValid(tpAniSirGlobal, tpSirSmeJoinReq);
tANI_U8 limIsSmeAuthReqValid(tpSirSmeAuthReq);
tANI_U8 limIsSmeDisassocReqValid(tpAniSirGlobal, tpSirSmeDisassocReq);
tANI_U8 limIsSmeDeauthReqValid(tpAniSirGlobal, tpSirSmeDeauthReq);
tANI_U8 limIsSmeSetContextReqValid(tpAniSirGlobal, tpSirSmeSetContextReq);
tANI_U8 limIsSmeStopBssReqValid(tANI_U32 *);
tANI_U8*  limGetBssIdFromSmeJoinReqMsg(tANI_U8 *);
tANI_U8 limIsSmeDisassocCnfValid(tpAniSirGlobal, tpSirSmeDisassocCnf);

#endif /* __LIM_SME_REQ_UTILS_H */
