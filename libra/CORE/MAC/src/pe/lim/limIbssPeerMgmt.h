/*
 * Airgo Networks, Inc proprietary. All rights reserved.
 * This file limIbssPeerMgmt.h contains prototypes for
 * the utility functions LIM uses to maintain peers in IBSS.
 * Author:        Chandra Modumudi
 * Date:          03/12/04
 * History:-
 * Date           Modified by    Modification Information
 * --------------------------------------------------------------------
 */

#include "sirCommon.h"
#include "limUtils.h"

void limIbssInit(tpAniSirGlobal);
void limIbssDelete(tpAniSirGlobal,tpPESession psessionEntry);
tSirRetStatus limIbssCoalesce(tpAniSirGlobal, tpSirMacMgmtHdr, tpSchBeaconStruct, tANI_U8*,tANI_U32, tANI_U16,tpPESession);
tSirRetStatus limIbssStaAdd(tpAniSirGlobal, void *);
tSirRetStatus limIbssAddStaRsp( tpAniSirGlobal, void *,tpPESession);
void limIbssDelBssRsp( tpAniSirGlobal, void *,tpPESession);
void limIbssDelBssRspWhenCoalescing(tpAniSirGlobal,  void *);
void limIbssAddBssRspWhenCoalescing(tpAniSirGlobal  pMac, void * msg);

void limIbssDecideProtectionOnDelete(tpAniSirGlobal pMac, tpDphHashNode pStaDs, tpUpdateBeaconParams pBeaconParams);
void limIbssHeartBeatHandle(tpAniSirGlobal pMac,tpPESession psessionEntry);

