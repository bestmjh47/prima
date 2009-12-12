/*
 * Airgo Networks, Inc proprietary. All rights reserved.
 * limStaHashApi.c: Provides access functions to get/set values of station hash entry fields.
 * Author:    Sunit Bhatia
 * Date:       09/19/2006
 * History:-
 * Date        Modified by            Modification Information
 *
 * --------------------------------------------------------------------------
 *
 */

#include "limStaHashApi.h"


























/**
 * limGetStaHashBssidx()
 *
 *FUNCTION:
 * This function is called to Get the Bss Index of the currently associated Station.
 *
 *LOGIC:
 *
 *ASSUMPTIONS:
 * NA
 *
 *NOTE:
 * NA
 *
 * @param pMac  pointer to Global Mac structure.
 * @param assocId AssocID of the Station.
 * @param bssidx pointer to the bss index, which will be returned by the function.
 *
 * @return success if GET operation is ok, else Failure.
 */

tSirRetStatus limGetStaHashBssidx(tpAniSirGlobal pMac, tANI_U16 assocId, tANI_U8 *bssidx)
{
    tpDphHashNode pSta = dphGetHashEntry(pMac, assocId);

    if (pSta == NULL)
    {
        PELOGE(limLog(pMac, LOGE, FL("invalid STA %d\n"),  assocId);)
        return eSIR_LIM_INVALID_STA;
    }

    *bssidx = (tANI_U8)pSta->bssId;
    return eSIR_SUCCESS;
}



