/*
 * Airgo Networks, Inc proprietary. All rights reserved.
 * This file parserApi.cc contains the code for parsing
 * 802.11 messages.
 * Author:        Pierre Vandwalle
 * Date:          03/18/02
 * History:-
 * Date           Modified by    Modification Information
 * --------------------------------------------------------------------
 *
 */

#include "sirApi.h"
#include "aniGlobal.h"
#include "parserApi.h"
#include "cfgApi.h"
#include "limUtils.h"
#include "utilsParser.h"
#include "limSerDesUtils.h"
#include "schApi.h"
#include "palApi.h"
#include "wmmApsd.h"
#if defined WLAN_FEATURE_VOWIFI
#include "rrmApi.h"
#endif



////////////////////////////////////////////////////////////////////////
void dot11fLog(tpAniSirGlobal pMac, int loglevel, const char *pString,...) 
{
#ifdef WLAN_DEBUG
	if( (tANI_U32)loglevel > pMac->utils.gLogDbgLevel[LOG_INDEX_FOR_MODULE( SIR_DBG_MODULE_ID )] )
	{
		return;
	}
	else
	{
        va_list         marker;

        va_start( marker, pString );     /* Initialize variable arguments. */

        logDebug(pMac, SIR_DBG_MODULE_ID, loglevel, pString, marker);

        va_end( marker );              /* Reset variable arguments.      */
    }
#endif	
}

void
swapBitField16(tANI_U16 in, tANI_U16 *out)
{
#   ifdef ANI_LITTLE_BIT_ENDIAN
    *out = in;
#   else // Big-Endian...
    *out = ( ( in & 0x8000 ) >> 15 ) |
           ( ( in & 0x4000 ) >> 13 ) |
           ( ( in & 0x2000 ) >> 11 ) |
           ( ( in & 0x1000 ) >>  9 ) |
           ( ( in & 0x0800 ) >>  7 ) |
           ( ( in & 0x0400 ) >>  5 ) |
           ( ( in & 0x0200 ) >>  3 ) |
           ( ( in & 0x0100 ) >>  1 ) |
           ( ( in & 0x0080 ) <<  1 ) |
           ( ( in & 0x0040 ) <<  3 ) |
           ( ( in & 0x0020 ) <<  5 ) |
           ( ( in & 0x0010 ) <<  7 ) |
           ( ( in & 0x0008 ) <<  9 ) |
           ( ( in & 0x0004 ) << 11 ) |
           ( ( in & 0x0002 ) << 13 ) |
           ( ( in & 0x0001 ) << 15 );
#   endif // ANI_LITTLE_BIT_ENDIAN
}

void
swapBitField32(tANI_U32 in, tANI_U32 *out)
{
#   ifdef ANI_LITTLE_BIT_ENDIAN
    *out = in;
#   else // Big-Endian...
    *out = ( ( in & 0x80000000 ) >> 31 ) |
           ( ( in & 0x40000000 ) >> 29 ) |
           ( ( in & 0x20000000 ) >> 27 ) |
           ( ( in & 0x10000000 ) >> 25 ) |
           ( ( in & 0x08000000 ) >> 23 ) |
           ( ( in & 0x04000000 ) >> 21 ) |
           ( ( in & 0x02000000 ) >> 19 ) |
           ( ( in & 0x01000000 ) >> 17 ) |
           ( ( in & 0x00800000 ) >> 15 ) |
           ( ( in & 0x00400000 ) >> 13 ) |
           ( ( in & 0x00200000 ) >> 11 ) |
           ( ( in & 0x00100000 ) >>  9 ) |
           ( ( in & 0x00080000 ) >>  7 ) |
           ( ( in & 0x00040000 ) >>  5 ) |
           ( ( in & 0x00020000 ) >>  3 ) |
           ( ( in & 0x00010000 ) >>  1 ) |
           ( ( in & 0x00008000 ) <<  1 ) |
           ( ( in & 0x00004000 ) <<  3 ) |
           ( ( in & 0x00002000 ) <<  5 ) |
           ( ( in & 0x00001000 ) <<  7 ) |
           ( ( in & 0x00000800 ) <<  9 ) |
           ( ( in & 0x00000400 ) << 11 ) |
           ( ( in & 0x00000200 ) << 13 ) |
           ( ( in & 0x00000100 ) << 15 ) |
           ( ( in & 0x00000080 ) << 17 ) |
           ( ( in & 0x00000040 ) << 19 ) |
           ( ( in & 0x00000020 ) << 21 ) |
           ( ( in & 0x00000010 ) << 23 ) |
           ( ( in & 0x00000008 ) << 25 ) |
           ( ( in & 0x00000004 ) << 27 ) |
           ( ( in & 0x00000002 ) << 29 ) |
           ( ( in & 0x00000001 ) << 31 );
#   endif // ANI_LITTLE_BIT_ENDIAN
}

inline static void __printWMMParams(tpAniSirGlobal  pMac, tDot11fIEWMMParams *pWmm)
{
    limLog(pMac, LOG1, FL("WMM Parameters Received: \n"));
    limLog(pMac, LOG1, FL("BE: aifsn %d, acm %d, aci %d, cwmin %d, cwmax %d, txop %d \n"),
           pWmm->acbe_aifsn, pWmm->acbe_acm, pWmm->acbe_aci, pWmm->acbe_acwmin, pWmm->acbe_acwmax, pWmm->acbe_txoplimit);
	
    limLog(pMac, LOG1, FL("BK: aifsn %d, acm %d, aci %d, cwmin %d, cwmax %d, txop %d \n"),
           pWmm->acbk_aifsn, pWmm->acbk_acm, pWmm->acbk_aci, pWmm->acbk_acwmin, pWmm->acbk_acwmax, pWmm->acbk_txoplimit);
	
    limLog(pMac, LOG1, FL("VI: aifsn %d, acm %d, aci %d, cwmin %d, cwmax %d, txop %d \n"),
           pWmm->acvi_aifsn, pWmm->acvi_acm, pWmm->acvi_aci, pWmm->acvi_acwmin, pWmm->acvi_acwmax, pWmm->acvi_txoplimit);
	
    limLog(pMac, LOG1, FL("VO: aifsn %d, acm %d, aci %d, cwmin %d, cwmax %d, txop %d \n"),
           pWmm->acvo_aifsn, pWmm->acvo_acm, pWmm->acvo_aci, pWmm->acvo_acwmin, pWmm->acvo_acwmax, pWmm->acvo_txoplimit);

    return;
}

////////////////////////////////////////////////////////////////////////
// Functions for populating "dot11f" style IEs


// return: >= 0, the starting location of the IE in rsnIEdata inside tSirRSNie
//         < 0, cannot find
static int FindIELocation( tpAniSirGlobal pMac,
                           tpSirRSNie pRsnIe,
                           tANI_U8 EID)
{
    int idx, ieLen, bytesLeft;

    // Here's what's going on: 'rsnIe' looks like this:

    //     typedef struct sSirRSNie
    //     {
    //         tANI_U16       length;
    //         tANI_U8        rsnIEdata[SIR_MAC_MAX_IE_LENGTH+2];
    //     } tSirRSNie, *tpSirRSNie;

    // other code records both the WPA & RSN IEs (including their EIDs &
    // lengths) into the array 'rsnIEdata'.  We may have:

    //     With WAPI support, there may be 3 IEs here
    //     It can be only WPA IE, or only RSN IE or only WAPI IE
    //     Or two or all three of them with no particular ordering

    // The if/then/else statements that follow are here to figure out
    // whether we have the WPA IE, and where it is if we *do* have it.

    //Save the first IE length 
    ieLen = pRsnIe->rsnIEdata[ 1 ] + 2;
    idx = 0;
    bytesLeft = pRsnIe->length;

    while( 1 )
    {
        if ( EID == pRsnIe->rsnIEdata[ idx ] )
        {
            //Found it
            return (idx);
        }
        else if ( EID != pRsnIe->rsnIEdata[ idx ] &&
             // & if no more IE, 
             bytesLeft <= (tANI_U16)( ieLen ) )
        {
            dot11fLog( pMac, LOG3, FL("No IE (%d) in FindIELocation.\n"), EID );
            return (-1);
        }
        bytesLeft -= ieLen;
        ieLen = pRsnIe->rsnIEdata[ idx + 1 ] + 2;
        idx += ieLen;
    }

    return (-1);
}


tSirRetStatus
PopulateDot11fCapabilities(tpAniSirGlobal         pMac,
                           tDot11fFfCapabilities *pDot11f,
                           tpPESession            psessionEntry)
{
    tANI_U16           cfg;
    tSirRetStatus nSirStatus;

    nSirStatus = cfgGetCapabilityInfo( pMac, &cfg,psessionEntry );
    if ( eSIR_SUCCESS != nSirStatus )
    {
        dot11fLog( pMac, LOGP, FL("Failed to retrieve the Capabilities b"
                               "itfield from CFG (%d).\n"), nSirStatus );
        return nSirStatus;
    }

#if 0
    if ( sirIsPropCapabilityEnabled( pMac, SIR_MAC_PROP_CAPABILITY_11EQOS ) )
    {
        SIR_MAC_CLEAR_CAPABILITY( cfg, QOS );
    }
#endif
    swapBitField16( cfg, ( tANI_U16* )pDot11f );

    return eSIR_SUCCESS;
} // End PopulateDot11fCapabilities.

tSirRetStatus
PopulateDot11fCapabilities2(tpAniSirGlobal         pMac,
                            tDot11fFfCapabilities *pDot11f,
                            tpDphHashNode          pSta, 
                            tpPESession            psessionEntry)
{
    tANI_U16           cfg;
    tSirRetStatus nSirStatus;
    nSirStatus = cfgGetCapabilityInfo( pMac, &cfg ,psessionEntry);
    if ( eSIR_SUCCESS != nSirStatus )
    {
        dot11fLog( pMac, LOGP, FL("Failed to retrieve the Capabilities b"
                               "itfield from CFG (%d).\n"), nSirStatus );
        return nSirStatus;
    }

    if ( ( NULL != pSta ) && pSta->aniPeer &&
         PROP_CAPABILITY_GET( 11EQOS, pSta->propCapability ) )
    {
        SIR_MAC_CLEAR_CAPABILITY( cfg, QOS );
    }

    swapBitField16( cfg, ( tANI_U16* )pDot11f );

    return eSIR_SUCCESS;

} // End PopulateDot11fCapabilities2.

void
PopulateDot11fChanSwitchAnn(tpAniSirGlobal          pMac,
                            tDot11fIEChanSwitchAnn *pDot11f)
{
    pDot11f->switchMode = pMac->lim.gLimChannelSwitch.switchMode;
    pDot11f->newChannel = pMac->lim.gLimChannelSwitch.primaryChannel;
    pDot11f->switchCount = ( tANI_U8 ) pMac->lim.gLimChannelSwitch.switchCount;

    pDot11f->present = 1;
} // End PopulateDot11fChanSwitchAnn.

void
PopulateDot11fExtChanSwitchAnn(tpAniSirGlobal		   pMac,
                            tDot11fIEExtChanSwitchAnn *pDot11f)
{
    //Has to be updated on the cb state basis
    pDot11f->secondaryChannelOffset = 
             limGetHTCBState(pMac->lim.gLimChannelSwitch.secondarySubBand);

    pDot11f->present = 1;
}

tSirRetStatus
PopulateDot11fCountry(tpAniSirGlobal    pMac,
                      tDot11fIECountry *pDot11f,
		      tpPESession psessionEntry)
{
    tANI_U32           len, maxlen, codelen;
    tANI_U16           item;
    tSirRetStatus nSirStatus;
    tANI_U8            temp[CFG_MAX_STR_LEN], code[3];

    if (psessionEntry->lim11dEnabled )
    {
        if (pMac->lim.gLimPhyMode == WNI_CFG_PHY_MODE_11A)
        {
            item   = WNI_CFG_MAX_TX_POWER_5;
            maxlen = WNI_CFG_MAX_TX_POWER_5_LEN;
        }
        else
        {
            item   = WNI_CFG_MAX_TX_POWER_2_4;
            maxlen = WNI_CFG_MAX_TX_POWER_2_4_LEN;
        }

        CFG_GET_STR( nSirStatus, pMac, item, temp, len, maxlen );

        if ( 3 > len )
        {
            // no limit on tx power, cannot include the IE because at least
            // one (channel,num,tx power) must be present
            return eSIR_SUCCESS;
        }

        CFG_GET_STR( nSirStatus, pMac, WNI_CFG_COUNTRY_CODE,
                     code, codelen, 3 );

        palCopyMemory( pMac->hHdd, pDot11f->country, code, codelen );

        pDot11f->num_triplets = ( tANI_U8 ) ( len / 3 );
        palCopyMemory( pMac->hHdd, ( tANI_U8* )pDot11f->triplets, temp, len );

        pDot11f->present = 1;
    }

    return eSIR_SUCCESS;
} // End PopulateDot11fCountry.

tSirRetStatus
PopulateDot11fDSParams(tpAniSirGlobal     pMac,
                       tDot11fIEDSParams *pDot11f, tANI_U8 channel)
{
    tSirRetStatus       nSirStatus;
    tANI_U32            nPhyMode;

    // Get PHY mode and based on that add DS Parameter Set IE
    CFG_GET_INT( nSirStatus, pMac, WNI_CFG_PHY_MODE, nPhyMode );

    if ( WNI_CFG_PHY_MODE_11A != nPhyMode )
    {
        // .11b/g mode PHY => Include the DS Parameter Set IE:
        #if 0
        CFG_GET_INT( nSirStatus, pMac, WNI_CFG_CURRENT_CHANNEL, cfg );
        #endif //TO SUPPORT BT-AMP
        pDot11f->curr_channel = channel;
        pDot11f->present = 1;
    }

    return eSIR_SUCCESS;
} // End PopulateDot11fDSParams.

#define SET_AIFSN(aifsn) (((aifsn) < 2) ? 2 : (aifsn))


void
PopulateDot11fEDCAParamSet(tpAniSirGlobal         pMac,
                           tDot11fIEEDCAParamSet *pDot11f, 
				tpPESession psessionEntry)
{

    if (  psessionEntry->limQosEnabled )
    {
        //change to bitwise operation, after this is fixed in frames.
        pDot11f->qos = (tANI_U8)(0xf0 & (pMac->sch.schObject.gSchEdcaParamSetCount << 4) );

        // Fill each EDCA parameter set in order: be, bk, vi, vo
        pDot11f->acbe_aifsn     = ( 0xf & SET_AIFSN(pMac->sch.schObject.gSchEdcaParamsBC[0].aci.aifsn) );
        pDot11f->acbe_acm       = ( 0x1 & pMac->sch.schObject.gSchEdcaParamsBC[0].aci.acm );
        pDot11f->acbe_aci       = ( 0x3 & SIR_MAC_EDCAACI_BESTEFFORT );
        pDot11f->acbe_acwmin    = ( 0xf & pMac->sch.schObject.gSchEdcaParamsBC[0].cw.min );
        pDot11f->acbe_acwmax    = ( 0xf & pMac->sch.schObject.gSchEdcaParamsBC[0].cw.max );
        pDot11f->acbe_txoplimit = pMac->sch.schObject.gSchEdcaParamsBC[0].txoplimit;

        pDot11f->acbk_aifsn     = ( 0xf & SET_AIFSN(pMac->sch.schObject.gSchEdcaParamsBC[1].aci.aifsn) );
        pDot11f->acbk_acm       = ( 0x1 & pMac->sch.schObject.gSchEdcaParamsBC[1].aci.acm );
        pDot11f->acbk_aci       = ( 0x3 & SIR_MAC_EDCAACI_BACKGROUND );
        pDot11f->acbk_acwmin    = ( 0xf & pMac->sch.schObject.gSchEdcaParamsBC[1].cw.min );
        pDot11f->acbk_acwmax    = ( 0xf & pMac->sch.schObject.gSchEdcaParamsBC[1].cw.max );
        pDot11f->acbk_txoplimit = pMac->sch.schObject.gSchEdcaParamsBC[1].txoplimit;

        pDot11f->acvi_aifsn     = ( 0xf & SET_AIFSN(pMac->sch.schObject.gSchEdcaParamsBC[2].aci.aifsn) );
        pDot11f->acvi_acm       = ( 0x1 & pMac->sch.schObject.gSchEdcaParamsBC[2].aci.acm );
        pDot11f->acvi_aci       = ( 0x3 & SIR_MAC_EDCAACI_VIDEO );
        pDot11f->acvi_acwmin    = ( 0xf & pMac->sch.schObject.gSchEdcaParamsBC[2].cw.min );
        pDot11f->acvi_acwmax    = ( 0xf & pMac->sch.schObject.gSchEdcaParamsBC[2].cw.max );
        pDot11f->acvi_txoplimit = pMac->sch.schObject.gSchEdcaParamsBC[2].txoplimit;

        pDot11f->acvo_aifsn     = ( 0xf & SET_AIFSN(pMac->sch.schObject.gSchEdcaParamsBC[3].aci.aifsn) );
        pDot11f->acvo_acm       = ( 0x1 & pMac->sch.schObject.gSchEdcaParamsBC[3].aci.acm );
        pDot11f->acvo_aci       = ( 0x3 & SIR_MAC_EDCAACI_VOICE );
        pDot11f->acvo_acwmin    = ( 0xf & pMac->sch.schObject.gSchEdcaParamsBC[3].cw.min );
        pDot11f->acvo_acwmax    = ( 0xf & pMac->sch.schObject.gSchEdcaParamsBC[3].cw.max );
        pDot11f->acvo_txoplimit = pMac->sch.schObject.gSchEdcaParamsBC[3].txoplimit;

        pDot11f->present = 1;
    }

} // End PopluateDot11fEDCAParamSet.

#ifdef WLAN_SOFTAP_FEATURE
tSirRetStatus
PopulateDot11fERPInfo(tpAniSirGlobal    pMac,
                      tDot11fIEERPInfo *pDot11f,
                      tpPESession    psessionEntry)
#else
tSirRetStatus
PopulateDot11fERPInfo(tpAniSirGlobal    pMac,
                      tDot11fIEERPInfo *pDot11f)
#endif
{
    tSirRetStatus nSirStatus;
    tANI_U32            val;
    tSirRFBand          rfBand = SIR_BAND_UNKNOWN;

    limGetRfBand(pMac, &rfBand);
    if(SIR_BAND_2_4_GHZ == rfBand)			
    {
        pDot11f->present = 1;

#ifdef WLAN_SOFTAP_FEATURE
        if(psessionEntry->limSystemRole == eLIM_AP_ROLE ){
	        val  = psessionEntry->cfgProtection.fromllb;
            if(!val ){
               dot11fLog( pMac, LOGE, FL("11B protection not enabled. Not populating ERP IE %d\n" ),val );
               return eSIR_SUCCESS;
            }
       	}else{
#endif       	
            CFG_GET_INT( nSirStatus, pMac, WNI_CFG_PROTECTION_ENABLED, val );

            if ( ! ((val >> WNI_CFG_PROTECTION_ENABLED_FROM_llB) & 1))
            {
                dot11fLog( pMac, LOG4, FL("11B protection not enabled. Not populating ERP IE\n") );
                return eSIR_SUCCESS;
            }
#ifdef WLAN_SOFTAP_FEATURE
        }
#endif

#ifdef WLAN_SOFTAP_FEATURE
       if ( (psessionEntry->limSystemRole == eLIM_AP_ROLE ) && psessionEntry->gLim11bParams.protectionEnabled)
       {
            pDot11f->non_erp_present = 1;
            pDot11f->use_prot        = 1;
       }
       else
#endif
       {
           if ( pMac->lim.gLim11bParams.protectionEnabled)
           {
               pDot11f->non_erp_present = 1;
               pDot11f->use_prot        = 1;
           }
       }

#ifdef WLAN_SOFTAP_FEATURE
        if ( (psessionEntry->limSystemRole == eLIM_AP_ROLE ) && psessionEntry->gLimOlbcParams.protectionEnabled )
        {
            //FIXME_PROTECTION: we should be setting non_erp present also.
            //check the test plan first.
            pDot11f->use_prot = 1;
        }
        else 
#endif
       {
            if ( pMac->lim.gLimOlbcParams.protectionEnabled )
            {
               //FIXME_PROTECTION: we should be setting non_erp present also.
               //check the test plan first.
               pDot11f->use_prot = 1;
            }
        }	


#ifdef WLAN_SOFTAP_FEATURE
        if((psessionEntry->limSystemRole == eLIM_AP_ROLE ) && 
            ((psessionEntry->gLimNoShortParams.numNonShortPreambleSta) 
               || !psessionEntry->fShortPreamble)){ 
                pDot11f->barker_preamble = 1;
         
        }
        
        else if (psessionEntry->limSystemRole != eLIM_AP_ROLE){ 
#endif
            CFG_GET_INT( nSirStatus, pMac, WNI_CFG_SHORT_PREAMBLE, val );

            if ( pMac->lim.gLimNoShortParams.numNonShortPreambleSta || ! val )
            {
                pDot11f->barker_preamble = 1;
            }

#ifdef WLAN_SOFTAP_FEATURE
      }
#endif
        // if protection always flag is set, advertise protection enabled
        // regardless of legacy stations presence
        CFG_GET_INT( nSirStatus, pMac, WNI_CFG_11G_PROTECTION_ALWAYS, val );

        if ( val )
        {
            pDot11f->use_prot = 1;
        }
    }

    return eSIR_SUCCESS;
} // End PopulateDot11fERPInfo.

tSirRetStatus
PopulateDot11fExtSuppRates(tpAniSirGlobal         pMac,
                           tDot11fIEExtSuppRates *pDot11f)
{
    tANI_U32           nRates;
    tSirRetStatus nSirStatus;
    tANI_U8            rates[WNI_CFG_EXTENDED_OPERATIONAL_RATE_SET_LEN];

    if ( WNI_CFG_PHY_MODE_11G == pMac->lim.gLimPhyMode )
    {
        CFG_GET_STR( nSirStatus, pMac, WNI_CFG_EXTENDED_OPERATIONAL_RATE_SET,
                     rates, nRates, WNI_CFG_EXTENDED_OPERATIONAL_RATE_SET_LEN );
        if ( 0 != nRates )
        {
            palCopyMemory( pMac->hHdd, pDot11f->rates, rates, nRates );
            pDot11f->num_rates = ( tANI_U8 ) nRates;
            pDot11f->present = 1;
        }
    }

    return eSIR_SUCCESS;
} // End PopulateDot11fExtSuppRates.

tSirRetStatus
PopulateDot11fExtSuppRates1(tpAniSirGlobal         pMac,
                            tANI_U8                     nChannelNum,
                            tDot11fIEExtSuppRates *pDot11f)
{
    tANI_U32           nRates;
    tSirRetStatus nSirStatus;
    tANI_U8            rates[SIR_MAC_MAX_NUMBER_OF_RATES];

    if ( 14 < nChannelNum )
    {
        pDot11f->present = 0;
        return eSIR_SUCCESS;
    }

    // N.B. I have *no* idea why we're calling 'wlan_cfgGetStr' with an argument
    // of WNI_CFG_SUPPORTED_RATES_11A here, but that's what was done
    // previously & I'm afraid to change it!
    CFG_GET_STR( nSirStatus, pMac, WNI_CFG_SUPPORTED_RATES_11A,
                 rates, nRates, SIR_MAC_MAX_NUMBER_OF_RATES );

    if ( 0 != nRates )
    {
        pDot11f->num_rates = ( tANI_U8 ) nRates;
        palCopyMemory( pMac->hHdd, pDot11f->rates, rates, nRates );
        pDot11f->present   = 1;
    }

    return eSIR_SUCCESS;
} // PopulateDot11fExtSuppRates1.

tSirRetStatus
PopulateDot11fHCF(tpAniSirGlobal  pMac,
                  tANI_U32             capEnable,
                  tDot11fIEHCF   *pDot11f)
{
#if 0
    tANI_U32           cfg;
    tSirRetStatus nSirStatus;

    if ( PROP_CAPABILITY_GET( HCF, capEnable ) )
    {
        limLog( pMac, LOG2, FL("HCF not filled in PopulateDot11fHCF ("
                               "cap 0x%08x).\n"), capEnable);
        return eSIR_SUCCESS;
    }

    CFG_GET_INT( nSirStatus, pMac, WNI_CFG_HCF_ENABLED, cfg );
    if ( cfg )
    {
        pDot11f->enabled = 1;
        pDot11f->present = 1;
    }
#endif
    return eSIR_SUCCESS;
} // End PopulateDot11fHCF.

tSirRetStatus
PopulateDot11fHTCaps(tpAniSirGlobal           pMac,
                             tDot11fIEHTCaps *pDot11f)
{
    tANI_U32                         nCfgValue, nCfgLen;
    tANI_U8                          nCfgValue8;
    tSirRetStatus                    nSirStatus;
    tSirMacHTParametersInfo         *pHTParametersInfo;
#ifdef WLAN_SOFTAP_FEATURE // this is added for fixing CRs on MDM9K platform - 257951, 259577
    union {
        tANI_U16                        nCfgValue16;
        tSirMacHTCapabilityInfo         htCapInfo;
        tSirMacExtendedHTCapabilityInfo extHtCapInfo;
    } uHTCapabilityInfo;
#else
    tANI_U16                         nCfgValue16;
    tSirMacHTCapabilityInfo         *pHTCapabilityInfo;
    tSirMacExtendedHTCapabilityInfo *pExtendedHTCapabilityInfo;
#endif

    tSirMacTxBFCapabilityInfo       *pTxBFCapabilityInfo;
    tSirMacASCapabilityInfo         *pASCapabilityInfo;

    CFG_GET_INT( nSirStatus, pMac, WNI_CFG_HT_CAP_INFO, nCfgValue );

#ifdef WLAN_SOFTAP_FEATURE  // this is added for fixing CRs on MDM9K platform - 257951, 259577
    uHTCapabilityInfo.nCfgValue16 = nCfgValue & 0xFFFF;
#else
    nCfgValue16 = ( tANI_U16 ) nCfgValue;
    pHTCapabilityInfo = ( tSirMacHTCapabilityInfo* ) &nCfgValue16;
#endif

    dot11fLog( pMac, LOG1, FL( "HT Caps: %x\n" ), nCfgValue);


#ifdef WLAN_SOFTAP_FEATURE  // this is added for fixing CRs on MDM9K platform - 257951, 259577
    pDot11f->advCodingCap             = uHTCapabilityInfo.htCapInfo.advCodingCap;
    pDot11f->supportedChannelWidthSet = uHTCapabilityInfo.htCapInfo.supportedChannelWidthSet;
    pDot11f->mimoPowerSave            = uHTCapabilityInfo.htCapInfo.mimoPowerSave;
    pDot11f->greenField               = uHTCapabilityInfo.htCapInfo.greenField;
    pDot11f->shortGI20MHz             = uHTCapabilityInfo.htCapInfo.shortGI20MHz;
    pDot11f->shortGI40MHz             = uHTCapabilityInfo.htCapInfo.shortGI40MHz;
    pDot11f->txSTBC                   = uHTCapabilityInfo.htCapInfo.txSTBC;
    pDot11f->rxSTBC                   = uHTCapabilityInfo.htCapInfo.rxSTBC;
    pDot11f->delayedBA                = uHTCapabilityInfo.htCapInfo.delayedBA;
    pDot11f->maximalAMSDUsize         = uHTCapabilityInfo.htCapInfo.maximalAMSDUsize;
    pDot11f->dsssCckMode40MHz         = uHTCapabilityInfo.htCapInfo.dsssCckMode40MHz;
    pDot11f->psmp                     = uHTCapabilityInfo.htCapInfo.psmp;
    pDot11f->stbcControlFrame         = uHTCapabilityInfo.htCapInfo.stbcControlFrame;
    pDot11f->lsigTXOPProtection       = uHTCapabilityInfo.htCapInfo.lsigTXOPProtection;
#else
    pDot11f->advCodingCap             = pHTCapabilityInfo->advCodingCap;
    pDot11f->supportedChannelWidthSet = pHTCapabilityInfo->supportedChannelWidthSet;
    pDot11f->mimoPowerSave            = pHTCapabilityInfo->mimoPowerSave;
    pDot11f->greenField               = pHTCapabilityInfo->greenField;
    pDot11f->shortGI20MHz             = pHTCapabilityInfo->shortGI20MHz;
    pDot11f->shortGI40MHz             = pHTCapabilityInfo->shortGI40MHz;
    pDot11f->txSTBC                   = pHTCapabilityInfo->txSTBC;
    pDot11f->rxSTBC                   = pHTCapabilityInfo->rxSTBC;
    pDot11f->delayedBA                = pHTCapabilityInfo->delayedBA;
    pDot11f->maximalAMSDUsize         = pHTCapabilityInfo->maximalAMSDUsize;
    pDot11f->dsssCckMode40MHz         = pHTCapabilityInfo->dsssCckMode40MHz;
    pDot11f->psmp                     = pHTCapabilityInfo->psmp;
    pDot11f->stbcControlFrame         = pHTCapabilityInfo->stbcControlFrame;
    pDot11f->lsigTXOPProtection       = pHTCapabilityInfo->lsigTXOPProtection;
#endif

    dot11fLog(pMac, LOG1, FL("SupportedChnlWidth: %d, mimoPS: %d, GF: %d, shortGI20:%d, shortGI40: %d, dsssCck: %d\n"),
                                            pDot11f->supportedChannelWidthSet, pDot11f->mimoPowerSave,  pDot11f->greenField,
                                            pDot11f->shortGI20MHz, pDot11f->shortGI40MHz, pDot11f->dsssCckMode40MHz);


    CFG_GET_INT( nSirStatus, pMac, WNI_CFG_HT_AMPDU_PARAMS, nCfgValue );

    nCfgValue8 = ( tANI_U8 ) nCfgValue;
    pHTParametersInfo = ( tSirMacHTParametersInfo* ) &nCfgValue8;

    pDot11f->maxRxAMPDUFactor = pHTParametersInfo->maxRxAMPDUFactor;
    pDot11f->mpduDensity      = pHTParametersInfo->mpduDensity;
    pDot11f->reserved1        = pHTParametersInfo->reserved;

    dot11fLog( pMac, LOG1, FL( "AMPDU Param: %x\n" ), nCfgValue);


    CFG_GET_STR( nSirStatus, pMac, WNI_CFG_SUPPORTED_MCS_SET,
                 pDot11f->supportedMCSSet, nCfgLen,
                 SIZE_OF_SUPPORTED_MCS_SET );


    CFG_GET_INT( nSirStatus, pMac, WNI_CFG_EXT_HT_CAP_INFO, nCfgValue );

#ifdef WLAN_SOFTAP_FEATURE  // this is added for fixing CRs on MDM9K platform - 257951, 259577
    uHTCapabilityInfo.nCfgValue16 = nCfgValue & 0xFFFF;

    pDot11f->pco            = uHTCapabilityInfo.extHtCapInfo.pco;
    pDot11f->transitionTime = uHTCapabilityInfo.extHtCapInfo.transitionTime;
    pDot11f->mcsFeedback    = uHTCapabilityInfo.extHtCapInfo.mcsFeedback;

#else
    nCfgValue16 = ( tANI_U16 ) nCfgValue ;
    pExtendedHTCapabilityInfo = ( tSirMacExtendedHTCapabilityInfo* ) &nCfgValue16;
    pDot11f->pco            = pExtendedHTCapabilityInfo->pco;
    pDot11f->transitionTime = pExtendedHTCapabilityInfo->transitionTime;
    pDot11f->mcsFeedback    = pExtendedHTCapabilityInfo->mcsFeedback;
#endif

    CFG_GET_INT( nSirStatus, pMac, WNI_CFG_TX_BF_CAP, nCfgValue );

    pTxBFCapabilityInfo = ( tSirMacTxBFCapabilityInfo* ) &nCfgValue;
    pDot11f->txBF                                       = pTxBFCapabilityInfo->txBF;
    pDot11f->rxStaggeredSounding                        = pTxBFCapabilityInfo->rxStaggeredSounding;
    pDot11f->txStaggeredSounding                        = pTxBFCapabilityInfo->txStaggeredSounding;
    pDot11f->rxZLF                                      = pTxBFCapabilityInfo->rxZLF;
    pDot11f->txZLF                                      = pTxBFCapabilityInfo->txZLF;
    pDot11f->implicitTxBF                               = pTxBFCapabilityInfo->implicitTxBF;
    pDot11f->calibration                                = pTxBFCapabilityInfo->calibration;
    pDot11f->explicitCSITxBF                            = pTxBFCapabilityInfo->explicitCSITxBF;
    pDot11f->explicitUncompressedSteeringMatrix         = pTxBFCapabilityInfo->explicitUncompressedSteeringMatrix;
    pDot11f->explicitBFCSIFeedback                      = pTxBFCapabilityInfo->explicitBFCSIFeedback;
    pDot11f->explicitUncompressedSteeringMatrixFeedback = pTxBFCapabilityInfo->explicitUncompressedSteeringMatrixFeedback;
    pDot11f->explicitCompressedSteeringMatrixFeedback   = pTxBFCapabilityInfo->explicitCompressedSteeringMatrixFeedback;
    pDot11f->csiNumBFAntennae                           = pTxBFCapabilityInfo->csiNumBFAntennae;
    pDot11f->uncompressedSteeringMatrixBFAntennae       = pTxBFCapabilityInfo->uncompressedSteeringMatrixBFAntennae;
    pDot11f->compressedSteeringMatrixBFAntennae         = pTxBFCapabilityInfo->compressedSteeringMatrixBFAntennae;

    CFG_GET_INT( nSirStatus, pMac, WNI_CFG_AS_CAP, nCfgValue );

    nCfgValue8 = ( tANI_U8 ) nCfgValue;

    pASCapabilityInfo = ( tSirMacASCapabilityInfo* ) &nCfgValue8;
    pDot11f->antennaSelection         = pASCapabilityInfo->antennaSelection;
    pDot11f->explicitCSIFeedbackTx    = pASCapabilityInfo->explicitCSIFeedbackTx;
    pDot11f->antennaIndicesFeedbackTx = pASCapabilityInfo->antennaIndicesFeedbackTx;
    pDot11f->explicitCSIFeedback      = pASCapabilityInfo->explicitCSIFeedback;
    pDot11f->antennaIndicesFeedback   = pASCapabilityInfo->antennaIndicesFeedback;
    pDot11f->rxAS                     = pASCapabilityInfo->rxAS;
    pDot11f->txSoundingPPDUs          = pASCapabilityInfo->txSoundingPPDUs;

    pDot11f->present = 1;

    return eSIR_SUCCESS;

} // End PopulateDot11fHTCaps.

#ifdef WLAN_SOFTAP_FEATURE
tSirRetStatus
PopulateDot11fHTInfo(tpAniSirGlobal   pMac,
                     tDot11fIEHTInfo *pDot11f,
                     tpPESession      psessionEntry )
#else
tSirRetStatus
PopulateDot11fHTInfo(tpAniSirGlobal   pMac,
		     tDot11fIEHTInfo *pDot11f)
#endif
{
    tANI_U32             nCfgValue, nCfgLen;
    tANI_U8              htInfoField1;
    tANI_U16            htInfoField2;
    tSirRetStatus        nSirStatus;
    tSirMacHTInfoField1 *pHTInfoField1;
    tSirMacHTInfoField2 *pHTInfoField2;
#ifdef WLAN_SOFTAP_FEATURE  // this is added for fixing CRs on MDM9K platform - 257951, 259577
    union {
        tANI_U16         nCfgValue16;
        tSirMacHTInfoField3 infoField3;
    }uHTInfoField;
    union {
        tANI_U16         nCfgValue16;
        tSirMacHTInfoField2 infoField2;
    }uHTInfoField2;
#else
    tANI_U16            htInfoField3;
    tSirMacHTInfoField3 *pHTInfoField3;
#endif

#ifndef WLAN_SOFTAP_FEATURE
    tpPESession         psessionEntry = &pMac->lim.gpSession[0];  //TBD-RAJESH HOW TO GET sessionEntry?????
#endif

    #if 0
    CFG_GET_INT( nSirStatus, pMac, WNI_CFG_CURRENT_CHANNEL, nCfgValue );
    #endif // TO SUPPORT BT-AMP
    
    pDot11f->primaryChannel = psessionEntry->currentOperChannel;

    CFG_GET_INT( nSirStatus, pMac, WNI_CFG_HT_INFO_FIELD1, nCfgValue );

    htInfoField1 = ( tANI_U8 ) nCfgValue;

    pHTInfoField1 = ( tSirMacHTInfoField1* ) &htInfoField1;
    pHTInfoField1->secondaryChannelOffset     = pMac->lim.gHTSecondaryChannelOffset;
    pHTInfoField1->recommendedTxWidthSet      = pMac->lim.gHTRecommendedTxWidthSet;
    pHTInfoField1->rifsMode                   = pMac->lim.gHTRifsMode;
    pHTInfoField1->serviceIntervalGranularity = pMac->lim.gHTServiceIntervalGranularity;


#ifdef WLAN_SOFTAP_FEATURE
    if(psessionEntry->limSystemRole == eLIM_AP_ROLE ){
    CFG_GET_INT( nSirStatus, pMac, WNI_CFG_HT_INFO_FIELD2, nCfgValue );

    uHTInfoField2.nCfgValue16 = nCfgValue & 0xFFFF; // this is added for fixing CRs on MDM9K platform - 257951, 259577

    uHTInfoField2.infoField2.opMode   =  psessionEntry->htOperMode;
    uHTInfoField2.infoField2.nonGFDevicesPresent = psessionEntry->gHTNonGFDevicesPresent;
    uHTInfoField2.infoField2.obssNonHTStaPresent = psessionEntry->gHTObssMode;	/*added for Obss  */

    uHTInfoField2.infoField2.reserved = 0;

   }else{
#endif
        CFG_GET_INT( nSirStatus, pMac, WNI_CFG_HT_INFO_FIELD2, nCfgValue );

        htInfoField2 = ( tANI_U16 ) nCfgValue;

        pHTInfoField2 = ( tSirMacHTInfoField2* ) &htInfoField2;
        pHTInfoField2->opMode   = pMac->lim.gHTOperMode;
        pHTInfoField2->nonGFDevicesPresent = pMac->lim.gHTNonGFDevicesPresent;
        pHTInfoField2->obssNonHTStaPresent = pMac->lim.gHTObssMode;	/*added for Obss  */

        pHTInfoField2->reserved = 0;
#ifdef WLAN_SOFTAP_FEATURE
    }
#endif

    CFG_GET_INT( nSirStatus, pMac, WNI_CFG_HT_INFO_FIELD3, nCfgValue );


#ifdef WLAN_SOFTAP_FEATURE  // this is added for fixing CRs on MDM9K platform - 257951, 259577
    uHTInfoField.nCfgValue16 = nCfgValue & 0xFFFF;


    uHTInfoField.infoField3.basicSTBCMCS                  = pMac->lim.gHTSTBCBasicMCS;
    uHTInfoField.infoField3.dualCTSProtection             = pMac->lim.gHTDualCTSProtection;
    uHTInfoField.infoField3.secondaryBeacon               = pMac->lim.gHTSecondaryBeacon;
    uHTInfoField.infoField3.lsigTXOPProtectionFullSupport = pMac->lim.gHTLSigTXOPFullSupport;
    uHTInfoField.infoField3.pcoActive                     = pMac->lim.gHTPCOActive;
    uHTInfoField.infoField3.pcoPhase                      = pMac->lim.gHTPCOPhase;
    uHTInfoField.infoField3.reserved                      = 0;

#else
    htInfoField3 = (tANI_U16) nCfgValue;

    pHTInfoField3 = ( tSirMacHTInfoField3* ) &htInfoField3;
    pHTInfoField3->basicSTBCMCS                  = pMac->lim.gHTSTBCBasicMCS;
    pHTInfoField3->dualCTSProtection             = pMac->lim.gHTDualCTSProtection;
    pHTInfoField3->secondaryBeacon               = pMac->lim.gHTSecondaryBeacon;
    pHTInfoField3->lsigTXOPProtectionFullSupport = pMac->lim.gHTLSigTXOPFullSupport;
    pHTInfoField3->pcoActive                     = pMac->lim.gHTPCOActive;
    pHTInfoField3->pcoPhase                      = pMac->lim.gHTPCOPhase;
    pHTInfoField3->reserved                      = 0;
#endif

    pDot11f->secondaryChannelOffset        = pHTInfoField1->secondaryChannelOffset;
    pDot11f->recommendedTxWidthSet         = pHTInfoField1->recommendedTxWidthSet;
    pDot11f->rifsMode                      = pHTInfoField1->rifsMode;
    pDot11f->controlledAccessOnly          = pHTInfoField1->controlledAccessOnly;
    pDot11f->serviceIntervalGranularity    = pHTInfoField1->serviceIntervalGranularity;

#ifdef WLAN_SOFTAP_FEATURE  // this is added for fixing CRs on MDM9K platform - 257951, 259577
    pDot11f->opMode                        = uHTInfoField2.infoField2.opMode;
    pDot11f->nonGFDevicesPresent           = uHTInfoField2.infoField2.nonGFDevicesPresent;
    pDot11f->obssNonHTStaPresent           = uHTInfoField2.infoField2.obssNonHTStaPresent;
    pDot11f->reserved                      = uHTInfoField2.infoField2.reserved;

#else
    pDot11f->opMode                        = pHTInfoField2->opMode;
    pDot11f->nonGFDevicesPresent           = pHTInfoField2->nonGFDevicesPresent;
    pDot11f->obssNonHTStaPresent           = pHTInfoField2->obssNonHTStaPresent;
    pDot11f->reserved                      = pHTInfoField2->reserved;
#endif

#ifdef WLAN_SOFTAP_FEATURE  // this is added for fixing CRs on MDM9K platform - 257951, 259577
    pDot11f->basicSTBCMCS                  = uHTInfoField.infoField3.basicSTBCMCS;
    pDot11f->dualCTSProtection             = uHTInfoField.infoField3.dualCTSProtection;
    pDot11f->secondaryBeacon               = uHTInfoField.infoField3.secondaryBeacon;
    pDot11f->lsigTXOPProtectionFullSupport = uHTInfoField.infoField3.lsigTXOPProtectionFullSupport;
    pDot11f->pcoActive                     = uHTInfoField.infoField3.pcoActive;
    pDot11f->pcoPhase                      = uHTInfoField.infoField3.pcoPhase;
    pDot11f->reserved2                     = uHTInfoField.infoField3.reserved;
#else
    pDot11f->basicSTBCMCS                  = pHTInfoField3->basicSTBCMCS;
    pDot11f->dualCTSProtection             = pHTInfoField3->dualCTSProtection;
    pDot11f->secondaryBeacon               = pHTInfoField3->secondaryBeacon;
    pDot11f->lsigTXOPProtectionFullSupport = pHTInfoField3->lsigTXOPProtectionFullSupport;
    pDot11f->pcoActive                     = pHTInfoField3->pcoActive;
    pDot11f->pcoPhase                      = pHTInfoField3->pcoPhase;
    pDot11f->reserved2                     = pHTInfoField3->reserved;
#endif
    CFG_GET_STR( nSirStatus, pMac, WNI_CFG_BASIC_MCS_SET,
                 pDot11f->basicMCSSet, nCfgLen,
                 SIZE_OF_BASIC_MCS_SET );

    pDot11f->present = 1;

    return eSIR_SUCCESS;

} // End PopulateDot11fHTInfo.

void
PopulateDot11fIBSSParams(tpAniSirGlobal       pMac,
       tDot11fIEIBSSParams *pDot11f, tpPESession psessionEntry)
{
    if ( eLIM_STA_IN_IBSS_ROLE == psessionEntry->limSystemRole )
    {
        pDot11f->present = 1;
        // ATIM duration is always set to 0
        pDot11f->atim = 0;
    }

} // End PopulateDot11fIBSSParams.


#ifdef ANI_SUPPORT_11H
tSirRetStatus
PopulateDot11fMeasurementReport0(tpAniSirGlobal              pMac,
                                 tpSirMacMeasReqActionFrame  pReq,
                                 tDot11fIEMeasurementReport *pDot11f)
{
    pDot11f->token     = pReq->measReqIE.measToken;
    pDot11f->late      = 0;
    pDot11f->incapable = 0;
    pDot11f->refused   = 1;
    pDot11f->type      = SIR_MAC_BASIC_MEASUREMENT_TYPE;

    pDot11f->present   = 1;

    return eSIR_SUCCESS;

} // End PopulatedDot11fMeasurementReport0.

tSirRetStatus
PopulateDot11fMeasurementReport1(tpAniSirGlobal              pMac,
                                  tpSirMacMeasReqActionFrame  pReq,
                                  tDot11fIEMeasurementReport *pDot11f)
{
    pDot11f->token     = pReq->measReqIE.measToken;
    pDot11f->late      = 0;
    pDot11f->incapable = 0;
    pDot11f->refused   = 1;
    pDot11f->type      = SIR_MAC_CCA_MEASUREMENT_TYPE;

    pDot11f->present   = 1;

    return eSIR_SUCCESS;

} // End PopulatedDot11fMeasurementReport1.

tSirRetStatus
PopulateDot11fMeasurementReport2(tpAniSirGlobal              pMac,
                                 tpSirMacMeasReqActionFrame  pReq,
                                 tDot11fIEMeasurementReport *pDot11f)
{
    pDot11f->token     = pReq->measReqIE.measToken;
    pDot11f->late      = 0;
    pDot11f->incapable = 0;
    pDot11f->refused   = 1;
    pDot11f->type      = SIR_MAC_RPI_MEASUREMENT_TYPE;

    pDot11f->present   = 1;

    return eSIR_SUCCESS;

} // End PopulatedDot11fMeasurementReport2.
#endif

void
PopulateDot11fPowerCaps(tpAniSirGlobal      pMac,
                        tDot11fIEPowerCaps *pCaps,
                        tANI_U8 nAssocType,
                        tpPESession psessionEntry)
{
    if (nAssocType == LIM_REASSOC)
    {
        pCaps->minTxPower = psessionEntry->pLimReAssocReq->powerCap.minTxPower;
        pCaps->maxTxPower = psessionEntry->pLimReAssocReq->powerCap.maxTxPower;
    }else
    {
        pCaps->minTxPower = psessionEntry->pLimJoinReq->powerCap.minTxPower;
        pCaps->maxTxPower = psessionEntry->pLimJoinReq->powerCap.maxTxPower;

    }
    
    pCaps->present    = 1;
} // End PopulateDot11fPowerCaps.

tSirRetStatus
PopulateDot11fPowerConstraints(tpAniSirGlobal             pMac,
                               tDot11fIEPowerConstraints *pDot11f)
{
    tANI_U32           cfg;
    tSirRetStatus nSirStatus;

    CFG_GET_INT( nSirStatus, pMac, WNI_CFG_LOCAL_POWER_CONSTRAINT, cfg );

    pDot11f->localPowerConstraints = ( tANI_U8 )cfg;
    pDot11f->present = 1;

    return eSIR_SUCCESS;
} // End PopulateDot11fPowerConstraints.

tSirRetStatus
PopulateDot11fPropCapability(tpAniSirGlobal           pMac,
                             tANI_U32                      capEnable,
                             tDot11fIEPropCapability *pDot11f)
{
#if 0
    tANI_U16           pcaps;
    tSirRetStatus nSirStatus;
    tANI_U32           cfg, phyMode, nRates;
    tANI_U8            rates[WNI_CFG_PROPRIETARY_OPERATIONAL_RATE_SET_LEN];

    pcaps = 0U;

#   if ( WNI_POLARIS_FW_PACKAGE == ADVANCED )

    CFG_GET_INT( nSirStatus, pMac, WNI_CFG_HCF_ENABLED, cfg );
    if ( cfg && PROP_CAPABILITY_GET( HCF, capEnable ))
        PROP_CAPABILITY_SET( HCF, pcaps );

#   endif

    if ( pMac->lim.gLimQosEnabled  && PROP_CAPABILITY_GET( 11EQOS, capEnable ) )
        PROP_CAPABILITY_SET( 11EQOS, pcaps );

    // Populate extended rateset if needed
    limGetPhyMode( pMac, &phyMode );
    if ( ( phyMode == WNI_CFG_PHY_MODE_11G ) ||
         ( phyMode == WNI_CFG_PHY_MODE_11A ) )
    {
        CFG_GET_STR( nSirStatus, pMac, WNI_CFG_PROPRIETARY_OPERATIONAL_RATE_SET,
                     rates, nRates, WNI_CFG_PROPRIETARY_OPERATIONAL_RATE_SET_LEN );
        if ( nRates && PROP_CAPABILITY_GET( EXTRATES, capEnable ) )
            PROP_CAPABILITY_SET( EXTRATES, pcaps );
    }


    if ( pMac->lim.gLimWmeEnabled  && PROP_CAPABILITY_GET( WME, capEnable ) )
        PROP_CAPABILITY_SET( WME, pcaps );

    if ( ( pMac->lim.gLimWsmEnabled  ) && ( pMac->lim.gLimWmeEnabled  ) && PROP_CAPABILITY_GET( WSM, capEnable ) )
        PROP_CAPABILITY_SET( WSM, pcaps );
    if ( PROP_CAPABILITY_GET( EDCAPARAMS, capEnable ) )
        PROP_CAPABILITY_SET( EDCAPARAMS, pcaps );
    if ( PROP_CAPABILITY_GET( LOADINFO, capEnable ) )
        PROP_CAPABILITY_SET( LOADINFO, pcaps );
    if ( PROP_CAPABILITY_GET( VERSION, capEnable ) )
        PROP_CAPABILITY_SET( VERSION, pcaps );

    if ( PROP_CAPABILITY_GET( TITAN, capEnable ) )
        PROP_CAPABILITY_SET( TITAN, pcaps );

    if ( PROP_CAPABILITY_GET( TAURUS, capEnable ) )
        PROP_CAPABILITY_SET( TAURUS, pcaps );

    if ( 0 == pcaps)
    {
        dot11fLog( pMac, LOG1, FL("PopulateDot11fPropCapability: no cap's configured (0x%x)\n"),
                capEnable );
        return eSIR_SUCCESS;
    }

    pDot11f->capability = pcaps;
    pDot11f->present = 1;
#endif
    return eSIR_SUCCESS;
} // End PopulateDot11fPropCapability.

void
PopulateDot11fPropChannSwitchAnn(tpAniSirGlobal               pMac,
                                 tANI_U32                          capEnable,
                                 tDot11fIEPropChannSwitchAnn *pDot11f)
{
#if 0
    // The AP shall populate the Proprietary Channel Switch IE in its
    // Beacon/Probe Rsp if AP is going to change/disable its secondary
    // subband.
    if ( ( pMac->lim.gLimChannelSwitch.state == eLIM_CHANNEL_SWITCH_SECONDARY_ONLY ) ||
         ( ( pMac->lim.gLimChannelSwitch.state != eLIM_CHANNEL_SWITCH_IDLE ) &&
            !pMac->lim.gLim11hEnable ) )
    {
        pDot11f->mode                 = ( tANI_U8 ) pMac->lim.gLimChannelSwitch.switchMode;
        pDot11f->primary_channel      = ( tANI_U8 ) pMac->lim.gLimChannelSwitch.primaryChannel;
        pDot11f->sub_band             = ( tANI_U8 ) pMac->lim.gLimChannelSwitch.secondarySubBand;
        pDot11f->channel_switch_count = ( tANI_U8 ) pMac->lim.gLimChannelSwitch.switchCount;
        pDot11f->present              = 1;

        limLog( pMac, LOG1, FL("mode %d, chnl %d, band %d, count %d\n"),
                pDot11f->mode,
                pMac->lim.gLimChannelSwitch.primaryChannel,
                pMac->lim.gLimChannelSwitch.secondarySubBand,
                pMac->lim.gLimChannelSwitch.switchCount );
    }
#endif

} // End PopulateDot11fPropChannSwitchAnn.

void
PopulateDot11fPropEDCAParams(tpAniSirGlobal           pMac,
                             tANI_U16                      caps,
                             tDot11fIEPropEDCAParams *pDot11f)
{

#if 0
    // We include the EDCA param IE only if needed, i.e. QOS or WME enabled
    if ( PROP_CAPABILITY_GET( EDCAPARAMS, caps ) &&
         ( ( PROP_CAPABILITY_GET( 11EQOS, caps ) && ( pMac->lim.gLimQosEnabled ) ) ||
           ( PROP_CAPABILITY_GET( WME, caps )    && ( pMac->lim.gLimWmeEnabled ) ) ) )
    {
        pDot11f->qos = (tANI_U8)(0xf0 & (pMac->sch.schObject.gSchEdcaParamSetCount << 4) );

        // Fill each EDCA parameter set in order: be, bk, vi, vo
        pDot11f->acbe_aifsn     = ( 0xf & SET_AIFSN(pMac->sch.schObject.gSchEdcaParamsBC[0].aci.aifsn) );
        pDot11f->acbe_acm       = ( 0x1 & pMac->sch.schObject.gSchEdcaParamsBC[0].aci.acm );
        pDot11f->acbe_aci       = ( 0x3 & SIR_MAC_EDCAACI_BESTEFFORT );
        pDot11f->acbe_min       = ( 0xf & pMac->sch.schObject.gSchEdcaParamsBC[0].cw.min );
        pDot11f->acbe_max       = ( 0xf & pMac->sch.schObject.gSchEdcaParamsBC[0].cw.max);
        pDot11f->acbe_txoplimit = pMac->sch.schObject.gSchEdcaParamsBC[0].txoplimit;

        pDot11f->acbk_aifsn     = ( 0xf & SET_AIFSN(pMac->sch.schObject.gSchEdcaParamsBC[1].aci.aifsn) );
        pDot11f->acbk_acm       = ( 0x1 & pMac->sch.schObject.gSchEdcaParamsBC[1].aci.acm );
        pDot11f->acbk_aci       = ( 0x3 & SIR_MAC_EDCAACI_BACKGROUND );
        pDot11f->acbk_min    = ( 0xf & pMac->sch.schObject.gSchEdcaParamsBC[1].cw.min );
        pDot11f->acbk_max    = ( 0xf & pMac->sch.schObject.gSchEdcaParamsBC[1].cw.max );
        pDot11f->acbk_txoplimit = pMac->sch.schObject.gSchEdcaParamsBC[1].txoplimit;

        pDot11f->acvi_aifsn     = ( 0xf & SET_AIFSN(pMac->sch.schObject.gSchEdcaParamsBC[2].aci.aifsn) );
        pDot11f->acvi_acm       = ( 0x1 & pMac->sch.schObject.gSchEdcaParamsBC[2].aci.acm );
        pDot11f->acvi_aci       = ( 0x3 & SIR_MAC_EDCAACI_VIDEO );
        pDot11f->acvi_min    = ( 0xf & pMac->sch.schObject.gSchEdcaParamsBC[2].cw.min );
        pDot11f->acvi_max    = ( 0xf & pMac->sch.schObject.gSchEdcaParamsBC[2].cw.max );
        pDot11f->acvi_txoplimit = pMac->sch.schObject.gSchEdcaParamsBC[2].txoplimit;

        pDot11f->acvo_aifsn     = ( 0xf & SET_AIFSN(pMac->sch.schObject.gSchEdcaParamsBC[3].aci.aifsn) );
        pDot11f->acvo_acm       = ( 0x1 & pMac->sch.schObject.gSchEdcaParamsBC[3].aci.acm );
        pDot11f->acvo_aci       = ( 0x3 & SIR_MAC_EDCAACI_VOICE );
        pDot11f->acvo_min    = ( 0xf & pMac->sch.schObject.gSchEdcaParamsBC[3].cw.min );
        pDot11f->acvo_max    = ( 0xf & pMac->sch.schObject.gSchEdcaParamsBC[3].cw.max );
        pDot11f->acvo_txoplimit = pMac->sch.schObject.gSchEdcaParamsBC[3].txoplimit;

        pDot11f->present = 1;
    }
#endif
} // End PopulateDot11fPropEDCAParams.

tSirRetStatus
PopulateDot11fPropSuppRates(tpAniSirGlobal          pMac,
                            tANI_U32                     capEnable,
                            tDot11fIEPropSuppRates *pDot11f)
{
#if 0
    tANI_U32           phyMode, nRates;
    tSirRetStatus nSirStatus;
    tANI_U8            rates[WNI_CFG_PROPRIETARY_OPERATIONAL_RATE_SET_LEN];

    limGetPhyMode( pMac, &phyMode );
    if ( ( phyMode == WNI_CFG_PHY_MODE_11G )||
         ( phyMode == WNI_CFG_PHY_MODE_11A ) )
    {
        if ( PROP_CAPABILITY_GET( EXTRATES, capEnable ) )
        {
            limLog( pMac, LOG2, FL("ExtRates not filled (cap 0x%x)\n"),
                    capEnable);
            return eSIR_SUCCESS;
        }

        CFG_GET_STR( nSirStatus, pMac, WNI_CFG_PROPRIETARY_OPERATIONAL_RATE_SET,
                     rates, nRates, WNI_CFG_PROPRIETARY_OPERATIONAL_RATE_SET_LEN );

        if ( 0 != nRates )
        {
            pDot11f->num_rates = ( tANI_U8 ) nRates;
            palCopyMemory( pMac->hHdd, pDot11f->rates, rates, nRates );
            pDot11f->present = 1;
        }

    }
#endif 
    return eSIR_SUCCESS;
} // End PopulateDot11fPropSuppRates.

void
PopulateDot11fQOSCapsAp(tpAniSirGlobal      pMac,
                        tDot11fIEQOSCapsAp *pDot11f)
{
    pDot11f->count    = pMac->sch.schObject.gSchEdcaParamSetCount;
	pDot11f->reserved = 0;
    pDot11f->txopreq  = 0;
    pDot11f->qreq     = 0;
    pDot11f->qack     = 0;
    pDot11f->present  = 1;
} // End PopulatedDot11fQOSCaps.

void
PopulateDot11fQOSCapsStation(tpAniSirGlobal    pMac,
                      tDot11fIEQOSCapsStation *pDot11f)
{
    tANI_U32  val = 0;

    if(wlan_cfgGetInt(pMac, WNI_CFG_MAX_SP_LENGTH, &val) != eSIR_SUCCESS) 
        PELOGE(limLog(pMac, LOGE, FL("could not retrieve Max SP Length \n"));)
   
    pDot11f->more_data_ack = 0;
    pDot11f->max_sp_length = (tANI_U8)val;
    pDot11f->qack    = 0;

    if (pMac->lim.gUapsdEnable)
    {
        pDot11f->acbe_uapsd = LIM_UAPSD_GET(ACBE, pMac->lim.gUapsdPerAcBitmask);
        pDot11f->acbk_uapsd = LIM_UAPSD_GET(ACBK, pMac->lim.gUapsdPerAcBitmask);
        pDot11f->acvi_uapsd = LIM_UAPSD_GET(ACVI, pMac->lim.gUapsdPerAcBitmask);
        pDot11f->acvo_uapsd = LIM_UAPSD_GET(ACVO, pMac->lim.gUapsdPerAcBitmask);
    }   
    pDot11f->present = 1;
} // End PopulatedDot11fQOSCaps.

tSirRetStatus
PopulateDot11fRSN(tpAniSirGlobal  pMac,
                  tpSirRSNie      pRsnIe,
                  tDot11fIERSN   *pDot11f)
{
    tANI_U32        status;
    int  idx;

    if ( pRsnIe->length )
    {
        if( 0 <= ( idx = FindIELocation( pMac, pRsnIe, DOT11F_EID_RSN ) ) )
            {
        status = dot11fUnpackIeRSN( pMac,
                                    pRsnIe->rsnIEdata + idx + 2, //EID, length
                                    pRsnIe->rsnIEdata[ idx + 1 ],
                                    pDot11f );
        if ( DOT11F_FAILED( status ) )
        {
            dot11fLog( pMac, LOGE, FL("Parse failure in PopulateDot11fRS"
                                   "N (0x%08x).\n"),
                    status );
			return eSIR_FAILURE;
        }
        dot11fLog( pMac, LOG2, FL("dot11fUnpackIeRSN returned 0x%08x in "
                               "PopulateDot11fRSN.\n"), status );
        }

    }

    return eSIR_SUCCESS;
} // End PopulateDot11fRSN.

tSirRetStatus PopulateDot11fRSNOpaque( tpAniSirGlobal      pMac,
                                       tpSirRSNie          pRsnIe,
                                       tDot11fIERSNOpaque *pDot11f )
{
    int idx;

    if ( pRsnIe->length )
    {
        if( 0 <= ( idx = FindIELocation( pMac, pRsnIe, DOT11F_EID_RSN ) ) )
        {
            pDot11f->present  = 1;
            pDot11f->num_data = pRsnIe->rsnIEdata[ idx + 1 ];
            palCopyMemory( pMac->hHdd, pDot11f->data,
                           pRsnIe->rsnIEdata + idx + 2,    // EID, len
                           pRsnIe->rsnIEdata[ idx + 1 ] );
        }
    }

    return eSIR_SUCCESS;

} // End PopulateDot11fRSNOpaque.



#if defined(FEATURE_WLAN_WAPI)

tSirRetStatus
PopulateDot11fWAPI(tpAniSirGlobal  pMac,
                  tpSirRSNie      pRsnIe,
                  tDot11fIEWAPI   *pDot11f)
        {
    tANI_U32        status;
    int  idx;

    if ( pRsnIe->length )
        {
        if( 0 <= ( idx = FindIELocation( pMac, pRsnIe, DOT11F_EID_WAPI ) ) )
        {
            status = dot11fUnpackIeWAPI( pMac,
                                        pRsnIe->rsnIEdata + idx + 2, //EID, length
                                        pRsnIe->rsnIEdata[ idx + 1 ],
                                        pDot11f );
            if ( DOT11F_FAILED( status ) )
            {
                dot11fLog( pMac, LOGE, FL("Parse failure in PopulateDot11fWAPI (0x%08x).\n"),
                        status );
                return eSIR_FAILURE;
            }
    	    dot11fLog( pMac, LOG2, FL("dot11fUnpackIeRSN returned 0x%08x in "
                               "PopulateDot11fWAPI.\n"), status );
        }
    }

    return eSIR_SUCCESS;
} // End PopulateDot11fWAPI.

tSirRetStatus PopulateDot11fWAPIOpaque( tpAniSirGlobal      pMac,
                                       tpSirRSNie          pRsnIe,
                                       tDot11fIEWAPIOpaque *pDot11f )
{
    int idx;

    if ( pRsnIe->length )
    {
        if( 0 <= ( idx = FindIELocation( pMac, pRsnIe, DOT11F_EID_WAPI ) ) )
        {
        pDot11f->present  = 1;
        pDot11f->num_data = pRsnIe->rsnIEdata[ idx + 1 ];
        palCopyMemory( pMac->hHdd, pDot11f->data,
                       pRsnIe->rsnIEdata + idx + 2,    // EID, len
                       pRsnIe->rsnIEdata[ idx + 1 ] );
    }
    }

    return eSIR_SUCCESS;

} // End PopulateDot11fWAPIOpaque.


#endif //defined(FEATURE_WLAN_WAPI)

void
PopulateDot11fSSID(tpAniSirGlobal pMac,
                   tSirMacSSid   *pInternal,
                   tDot11fIESSID *pDot11f)
{
    pDot11f->present = 1;
    pDot11f->num_ssid = pInternal->length;
    if ( pInternal->length )
    {
        palCopyMemory( pMac->hHdd, ( tANI_U8* )pDot11f->ssid, ( tANI_U8* )&pInternal->ssId,
                       pInternal->length );
    }
} // End PopulateDot11fSSID.

tSirRetStatus
PopulateDot11fSSID2(tpAniSirGlobal pMac,
                    tDot11fIESSID *pDot11f)
{
    tANI_U32           nCfg;
    tSirRetStatus nSirStatus;

    CFG_GET_STR( nSirStatus, pMac, WNI_CFG_SSID, pDot11f->ssid, nCfg, 32 );
    pDot11f->num_ssid = ( tANI_U8 )nCfg;
    pDot11f->present  = 1;
    return eSIR_SUCCESS;
} // End PopulateDot11fSSID2.

void
PopulateDot11fSchedule(tSirMacScheduleIE *pSchedule,
                       tDot11fIESchedule *pDot11f)
{
    pDot11f->aggregation        = pSchedule->info.aggregation;
    pDot11f->tsid               = pSchedule->info.tsid;
    pDot11f->direction          = pSchedule->info.direction;
    pDot11f->reserved           = pSchedule->info.rsvd;
    pDot11f->service_start_time = pSchedule->svcStartTime;
    pDot11f->service_interval   = pSchedule->svcInterval;
    pDot11f->max_service_dur    = pSchedule->maxSvcDuration;
    pDot11f->spec_interval      = pSchedule->specInterval;

    pDot11f->present = 1;
} // End PopulateDot11fSchedule.

void
PopulateDot11fSuppChannels(tpAniSirGlobal         pMac,
                           tDot11fIESuppChannels *pDot11f,
                           tANI_U8 nAssocType,
                           tpPESession psessionEntry)
{
    tANI_U8  i;
    tANI_U8 *p;

    if (nAssocType == LIM_REASSOC)
    {
        p = ( tANI_U8* )psessionEntry->pLimReAssocReq->supportedChannels.channelList;
        pDot11f->num_bands = psessionEntry->pLimReAssocReq->supportedChannels.numChnl;
    }else
    {
        p = ( tANI_U8* )psessionEntry->pLimJoinReq->supportedChannels.channelList;
        pDot11f->num_bands = psessionEntry->pLimJoinReq->supportedChannels.numChnl;
    }
    for ( i = 0U; i < pDot11f->num_bands; ++i, ++p)
    {
        pDot11f->bands[i][0] = *p;
        pDot11f->bands[i][1] = 1;
    }

    pDot11f->present = 1;

} // End PopulateDot11fSuppChannels.

tSirRetStatus
PopulateDot11fSuppRates(tpAniSirGlobal      pMac,
                        tANI_U8                  nChannelNum,
                        tDot11fIESuppRates *pDot11f,tpPESession psessionEntry)
{
    tSirRetStatus nSirStatus;
    tANI_U32           nRates;
    tANI_U8            rates[SIR_MAC_MAX_NUMBER_OF_RATES];

   /* Use the operational rates present in session entry whenever nChannelNum is set to OPERATIONAL
       else use the supported rate set from CFG, which is fixed and does not change dynamically and is used for
       sending mgmt frames (lile probe req) which need to go out before any session is present.
   */
    if(POPULATE_DOT11F_RATES_OPERATIONAL == nChannelNum )
    {
        #if 0
        CFG_GET_STR( nSirStatus, pMac, WNI_CFG_OPERATIONAL_RATE_SET,
                     rates, nRates, SIR_MAC_MAX_NUMBER_OF_RATES );
        #endif //TO SUPPORT BT-AMP
        if(psessionEntry != NULL)
        {
	 	nRates = psessionEntry->rateSet.numRates;
        	palCopyMemory(pMac->hHdd, rates, psessionEntry->rateSet.rate, 
              	                     nRates);
        }
	else
        	dot11fLog( pMac, LOGE, FL("no session context exists while populating Operational Rate Set\n"));
        
    }
    else if ( 14 >= nChannelNum )
    {
        CFG_GET_STR( nSirStatus, pMac, WNI_CFG_SUPPORTED_RATES_11B,
                     rates, nRates, SIR_MAC_MAX_NUMBER_OF_RATES );
    }
    else
    {
        CFG_GET_STR( nSirStatus, pMac, WNI_CFG_SUPPORTED_RATES_11A,
                     rates, nRates, SIR_MAC_MAX_NUMBER_OF_RATES );
    }

    if ( 0 != nRates )
    {
        pDot11f->num_rates = ( tANI_U8 )nRates;
        palCopyMemory( pMac->hHdd, pDot11f->rates, rates, nRates );
        pDot11f->present   = 1;
    }

    return eSIR_SUCCESS;

} // End PopulateDot11fSuppRates.

tSirRetStatus
PopulateDot11fTPCReport(tpAniSirGlobal      pMac,
                        tDot11fIETPCReport *pDot11f,
                        tpPESession psessionEntry)
{
    tANI_U16 staid, txPower;
    tSirRetStatus nSirStatus;

    nSirStatus = limGetMgmtStaid( pMac, &staid, psessionEntry);
    if ( eSIR_SUCCESS != nSirStatus )
    {
        dot11fLog( pMac, LOG1, FL("Failed to get the STAID in Populat"
                                  "eDot11fTPCReport; limGetMgmtStaid "
                                  "returned status %d.\n"),
                   nSirStatus );
        return eSIR_FAILURE;
    }

    // FramesToDo: This function was "misplaced" in the move to Gen4_TVM...
    // txPower = halGetRateToPwrValue( pMac, staid, pMac->lim.gLimCurrentChannelId, isBeacon );
    txPower = 0;
    pDot11f->tx_power    = ( tANI_U8 )txPower;
    pDot11f->link_margin = 0;
    pDot11f->present     = 1;

    return eSIR_SUCCESS;
} // End PopulateDot11fTPCReport.


void PopulateDot11fTSInfo(tSirMacTSInfo   *pInfo,
                          tDot11fFfTSInfo *pDot11f)
{
    pDot11f->traffic_type   = pInfo->traffic.trafficType;
    pDot11f->tsid           = pInfo->traffic.tsid;
    pDot11f->direction      = pInfo->traffic.direction;
    pDot11f->access_policy  = pInfo->traffic.accessPolicy;
    pDot11f->aggregation    = pInfo->traffic.aggregation;
    pDot11f->psb            = pInfo->traffic.psb;
    pDot11f->user_priority  = pInfo->traffic.userPrio;
    pDot11f->tsinfo_ack_pol = pInfo->traffic.ackPolicy;
    pDot11f->schedule       = pInfo->schedule.schedule;
} // End PopulatedDot11fTSInfo.

void PopulateDot11fWMM(tpAniSirGlobal      pMac,
                       tDot11fIEWMMInfoAp  *pInfo,
                       tDot11fIEWMMParams *pParams,
                       tDot11fIEWMMCaps   *pCaps,
                       tpPESession        psessionEntry)
{
    if ( psessionEntry->limWmeEnabled )
    {
        if ( eLIM_STA_IN_IBSS_ROLE == psessionEntry->limSystemRole )
        {
            //if ( ! sirIsPropCapabilityEnabled( pMac, SIR_MAC_PROP_CAPABILITY_WME ) )
            {
                PopulateDot11fWMMInfoAp( pMac, pInfo, psessionEntry );
            }
        }
        else
        {
            {
                PopulateDot11fWMMParams( pMac, pParams, psessionEntry);
            }

           if ( psessionEntry->limWsmEnabled )
            {
                PopulateDot11fWMMCaps( pCaps );
            }
        }
    }
} // End PopulateDot11fWMM.

void PopulateDot11fWMMCaps(tDot11fIEWMMCaps *pCaps)
{
    pCaps->version       = SIR_MAC_OUI_VERSION_1;
    pCaps->qack          = 0;
    pCaps->queue_request = 1;
    pCaps->txop_request  = 0;
    pCaps->more_ack      = 0;
    pCaps->present       = 1;
} // End PopulateDot11fWmmCaps.

void PopulateDot11fWMMInfoAp(tpAniSirGlobal pMac, tDot11fIEWMMInfoAp *pInfo, 
                             tpPESession psessionEntry)
{
    pInfo->version = SIR_MAC_OUI_VERSION_1;

    /* WMM Specification 3.1.3, 3.2.3
     * An IBSS staion shall always use its default WMM parameters.
     */
    if ( eLIM_STA_IN_IBSS_ROLE == psessionEntry->limSystemRole )
    {
        pInfo->param_set_count = 0;
        pInfo->uapsd = 0;
    }
    else
    {
        pInfo->param_set_count = ( 0xf & pMac->sch.schObject.gSchEdcaParamSetCount );
#ifdef WLAN_SOFTAP_FEATURE
        if(psessionEntry->limSystemRole == eLIM_AP_ROLE ){
            pInfo->uapsd = ( 0x1 & psessionEntry->apUapsdEnable );
        }
        else
#endif
            pInfo->uapsd = ( 0x1 & pMac->lim.gUapsdEnable );
    }
    pInfo->present = 1;
}

void PopulateDot11fWMMInfoStation(tpAniSirGlobal pMac, tDot11fIEWMMInfoStation *pInfo)
{
    tANI_U32  val = 0;

    pInfo->version = SIR_MAC_OUI_VERSION_1;
    pInfo->acvo_uapsd = LIM_UAPSD_GET(ACVO, pMac->lim.gUapsdPerAcBitmask);
    pInfo->acvi_uapsd = LIM_UAPSD_GET(ACVI, pMac->lim.gUapsdPerAcBitmask);
    pInfo->acbk_uapsd = LIM_UAPSD_GET(ACBK, pMac->lim.gUapsdPerAcBitmask);
    pInfo->acbe_uapsd = LIM_UAPSD_GET(ACBE, pMac->lim.gUapsdPerAcBitmask);

    if(wlan_cfgGetInt(pMac, WNI_CFG_MAX_SP_LENGTH, &val) != eSIR_SUCCESS) 
        PELOGE(limLog(pMac, LOGE, FL("could not retrieve Max SP Length \n"));)

    pInfo->max_sp_length = (tANI_U8)val;
    pInfo->present = 1;
}

#ifdef WLAN_SOFTAP_FEATURE
void PopulateDot11fWMMParams(tpAniSirGlobal      pMac,
                             tDot11fIEWMMParams *pParams,
                             tpPESession        psessionEntry)
#else
void PopulateDot11fWMMParams(tpAniSirGlobal      pMac,
                             tDot11fIEWMMParams *pParams)
#endif
{
    pParams->version = SIR_MAC_OUI_VERSION_1;

#ifdef WLAN_SOFTAP_FEATURE
    if(psessionEntry->limSystemRole == eLIM_AP_ROLE)
       pParams->qosInfo =
           (psessionEntry->apUapsdEnable << 7) | ((tANI_U8)(0x0f & pMac->sch.schObject.gSchEdcaParamSetCount));
    else 
#endif
       pParams->qosInfo =
           (pMac->lim.gUapsdEnable << 7) | ((tANI_U8)(0x0f & pMac->sch.schObject.gSchEdcaParamSetCount));

    // Fill each EDCA parameter set in order: be, bk, vi, vo
    pParams->acbe_aifsn     = ( 0xf & SET_AIFSN(pMac->sch.schObject.gSchEdcaParamsBC[0].aci.aifsn) );
    pParams->acbe_acm       = ( 0x1 & pMac->sch.schObject.gSchEdcaParamsBC[0].aci.acm );
    pParams->acbe_aci       = ( 0x3 & SIR_MAC_EDCAACI_BESTEFFORT );
    pParams->acbe_acwmin    = ( 0xf & pMac->sch.schObject.gSchEdcaParamsBC[0].cw.min );
    pParams->acbe_acwmax    = ( 0xf & pMac->sch.schObject.gSchEdcaParamsBC[0].cw.max );
    pParams->acbe_txoplimit = pMac->sch.schObject.gSchEdcaParamsBC[0].txoplimit;

    pParams->acbk_aifsn     = ( 0xf & SET_AIFSN(pMac->sch.schObject.gSchEdcaParamsBC[1].aci.aifsn) );
    pParams->acbk_acm       = ( 0x1 & pMac->sch.schObject.gSchEdcaParamsBC[1].aci.acm );
    pParams->acbk_aci       = ( 0x3 & SIR_MAC_EDCAACI_BACKGROUND );
    pParams->acbk_acwmin    = ( 0xf & pMac->sch.schObject.gSchEdcaParamsBC[1].cw.min );
    pParams->acbk_acwmax    = ( 0xf & pMac->sch.schObject.gSchEdcaParamsBC[1].cw.max );
    pParams->acbk_txoplimit = pMac->sch.schObject.gSchEdcaParamsBC[1].txoplimit;

#ifdef WLAN_SOFTAP_FEATURE
    if(psessionEntry->limSystemRole == eLIM_AP_ROLE )
        pParams->acvi_aifsn     = ( 0xf & pMac->sch.schObject.gSchEdcaParamsBC[2].aci.aifsn );
    else
#endif 
        pParams->acvi_aifsn     = ( 0xf & SET_AIFSN(pMac->sch.schObject.gSchEdcaParamsBC[2].aci.aifsn) );



    pParams->acvi_acm       = ( 0x1 & pMac->sch.schObject.gSchEdcaParamsBC[2].aci.acm );
    pParams->acvi_aci       = ( 0x3 & SIR_MAC_EDCAACI_VIDEO );
    pParams->acvi_acwmin    = ( 0xf & pMac->sch.schObject.gSchEdcaParamsBC[2].cw.min );
    pParams->acvi_acwmax    = ( 0xf & pMac->sch.schObject.gSchEdcaParamsBC[2].cw.max );
    pParams->acvi_txoplimit = pMac->sch.schObject.gSchEdcaParamsBC[2].txoplimit;

#ifdef WLAN_SOFTAP_FEATURE
    if(psessionEntry->limSystemRole == eLIM_AP_ROLE )
        pParams->acvo_aifsn     = ( 0xf & pMac->sch.schObject.gSchEdcaParamsBC[3].aci.aifsn );
    else
#endif
        pParams->acvo_aifsn     = ( 0xf & SET_AIFSN(pMac->sch.schObject.gSchEdcaParamsBC[3].aci.aifsn) );

    pParams->acvo_acm       = ( 0x1 & pMac->sch.schObject.gSchEdcaParamsBC[3].aci.acm );
    pParams->acvo_aci       = ( 0x3 & SIR_MAC_EDCAACI_VOICE );
    pParams->acvo_acwmin    = ( 0xf & pMac->sch.schObject.gSchEdcaParamsBC[3].cw.min );
    pParams->acvo_acwmax    = ( 0xf & pMac->sch.schObject.gSchEdcaParamsBC[3].cw.max );
    pParams->acvo_txoplimit = pMac->sch.schObject.gSchEdcaParamsBC[3].txoplimit;

    pParams->present = 1;

} // End PopulateDot11fWMMParams.

void PopulateDot11fWMMSchedule(tSirMacScheduleIE    *pSchedule,
                               tDot11fIEWMMSchedule *pDot11f)
{
    pDot11f->version            = 1;
    pDot11f->aggregation        = pSchedule->info.aggregation;
    pDot11f->tsid               = pSchedule->info.tsid;
    pDot11f->direction          = pSchedule->info.direction;
    pDot11f->reserved           = pSchedule->info.rsvd;
    pDot11f->service_start_time = pSchedule->svcStartTime;
    pDot11f->service_interval   = pSchedule->svcInterval;
    pDot11f->max_service_dur    = pSchedule->maxSvcDuration;
    pDot11f->spec_interval      = pSchedule->specInterval;

    pDot11f->present = 1;
} // End PopulateDot11fWMMSchedule.

tSirRetStatus
PopulateDot11fWPA(tpAniSirGlobal  pMac,
                  tpSirRSNie      pRsnIe,
                  tDot11fIEWPA   *pDot11f)
{
    tANI_U32        status;
    int idx;

    if ( pRsnIe->length )
    {
        if( 0 <= ( idx = FindIELocation( pMac, pRsnIe, DOT11F_EID_WPA ) ) )
        {
        status = dot11fUnpackIeWPA( pMac,
                                    pRsnIe->rsnIEdata + idx + 2 + 4,  // EID, length, OUI
                                    pRsnIe->rsnIEdata[ idx + 1 ] - 4, // OUI
                                    pDot11f );
        if ( DOT11F_FAILED( status ) )
        {
            dot11fLog( pMac, LOGE, FL("Parse failure in PopulateDot11fWP"
                                   "A (0x%08x).\n"),
                    status );
            return eSIR_FAILURE;
        }
    }
    }

    return eSIR_SUCCESS;
} // End PopulateDot11fWPA.



tSirRetStatus PopulateDot11fWPAOpaque( tpAniSirGlobal      pMac,
                                       tpSirRSNie          pRsnIe,
                                       tDot11fIEWPAOpaque *pDot11f )
{
    int idx;

    if ( pRsnIe->length )
    {
        if( 0 <= ( idx = FindIELocation( pMac, pRsnIe, DOT11F_EID_WPA ) ) )
        {
        pDot11f->present  = 1;
        pDot11f->num_data = pRsnIe->rsnIEdata[ idx + 1 ] - 4;
        palCopyMemory( pMac->hHdd, pDot11f->data,
                       pRsnIe->rsnIEdata + idx + 2 + 4,    // EID, len, OUI
                       pRsnIe->rsnIEdata[ idx + 1 ] - 4 ); // OUI
    }
    }

    return eSIR_SUCCESS;

} // End PopulateDot11fWPAOpaque.

////////////////////////////////////////////////////////////////////////

tSirRetStatus
sirGetCfgPropCaps(tpAniSirGlobal pMac, tANI_U16 *caps)
{
#if 0
    tANI_U32 val;

    *caps = 0;
    if (wlan_cfgGetInt(pMac, WNI_CFG_PROPRIETARY_ANI_FEATURES_ENABLED, &val)
        != eSIR_SUCCESS)
    {
        limLog(pMac, LOGP, FL("could not retrieve PropFeature enabled flag\n"));
        return eSIR_FAILURE;
    }
    if (wlan_cfgGetInt(pMac, WNI_CFG_PROP_CAPABILITY, &val) != eSIR_SUCCESS)
    {
        limLog(pMac, LOGP, FL("could not retrieve PROP_CAPABLITY flag\n"));
        return eSIR_FAILURE;
    }

    *caps = (tANI_U16) val;
#endif
    return eSIR_SUCCESS;
}

tANI_U8
sirIsPropCapabilityEnabled(tpAniSirGlobal pMac, tANI_U32 bitnum)
{
#if 0
    tANI_U16 val;

    if (bitnum > SIR_MAC_PROP_CAPABILITY_MAXBITOFFSET)
    {
        limLog(pMac, LOGE, FL("isPropCapEnabled: invalid offset %d\n"),
                  bitnum);
        return false;
    }
    if (sirGetCfgPropCaps(pMac, &val) != eSIR_SUCCESS)
    {
        PELOGE(limLog(pMac, LOGE, FL("Can't get PropCaps\n"));)
        return false;
    }

    return ((val & (1 << bitnum)) ? true : false);
#endif
   return false;
}


tSirRetStatus
sirConvertProbeReqFrame2Struct(tpAniSirGlobal  pMac,
                               tANI_U8             *pFrame,
                               tANI_U32             nFrame,
                               tpSirProbeReq   pProbeReq)
{
    tANI_U32 status;
    tDot11fProbeRequest pr;

    // Ok, zero-init our [out] parameter,
    palZeroMemory( pMac->hHdd, (tANI_U8*)pProbeReq, sizeof(tSirProbeReq));

    // delegate to the framesc-generated code,
    status = dot11fUnpackProbeRequest(pMac, pFrame, nFrame, &pr);
    if ( DOT11F_FAILED( status ) )
    {
        limLog(pMac, LOGE, FL("Failed to parse a Probe Request (0x%08x, %d bytes):\n"),
                  status, nFrame);
        PELOG2(sirDumpBuf(pMac, SIR_DBG_MODULE_ID, LOG2, pFrame, nFrame);)
        return eSIR_FAILURE;
    }
    else if ( DOT11F_WARNED( status ) )
    {
      limLog( pMac, LOGW, FL("There were warnings while unpacking a Probe Request (0x%08x, %d bytes):\n"),
                 status, nFrame );
        PELOG2(sirDumpBuf(pMac, SIR_DBG_MODULE_ID, LOG2, pFrame, nFrame);)
    }

    // & "transliterate" from a 'tDot11fProbeRequestto' a 'tSirProbeReq'...
    if ( ! pr.SSID.present )
    {
        PELOGW(limLog(pMac, LOGW, FL("Mandatory IE SSID not present!\n"));)
    }
    else
    {
        pProbeReq->ssidPresent = 1;
        ConvertSSID( pMac, &pProbeReq->ssId, &pr.SSID );
    }

    if ( ! pr.SuppRates.present )
    {
        PELOGW(limLog(pMac, LOGW, FL("Mandatory IE Supported Rates not present!\n"));)
        return eSIR_FAILURE;
    }
    else
    {
        pProbeReq->suppRatesPresent = 1;
        ConvertSuppRates( pMac, &pProbeReq->supportedRates, &pr.SuppRates );
    }

    if ( pr.ExtSuppRates.present )
    {
        pProbeReq->extendedRatesPresent = 1;
        ConvertExtSuppRates( pMac, &pProbeReq->extendedRates, &pr.ExtSuppRates );
    }

    if ( pr.HTCaps.present )
    {
        palCopyMemory( pMac, &pProbeReq->HTCaps, &pr.HTCaps, sizeof( tDot11fIEHTCaps ) );
    }

    if ( pr.WscProbeReq.present )
    {
        pProbeReq->wscIePresent = 1;
        memcpy(&pProbeReq->probeReqWscIeInfo, &pr.WscProbeReq, sizeof(tDot11fIEWscProbeReq));
    }

    return eSIR_SUCCESS;

} // End sirConvertProbeReqFrame2Struct.

tSirRetStatus sirConvertProbeFrame2Struct(tpAniSirGlobal       pMac,
                                          tANI_U8             *pFrame,
                                          tANI_U32             nFrame,
                                          tpSirProbeRespBeacon pProbeResp)
{
    tANI_U32             status;
    tDot11fProbeResponse pr;

    // Ok, zero-init our [out] parameter,
    palZeroMemory( pMac->hHdd, ( tANI_U8* )pProbeResp, sizeof(tSirProbeRespBeacon) );
    palZeroMemory( pMac->hHdd, ( tANI_U8* )&pr, sizeof(tDot11fProbeResponse) );

    
    // delegate to the framesc-generated code,
    status = dot11fUnpackProbeResponse( pMac, pFrame, nFrame, &pr );
    if ( DOT11F_FAILED( status ) )
    {
        limLog(pMac, LOGE, FL("Failed to parse a Probe Response (0x%08x, %d bytes):\n"),
                  status, nFrame);
        PELOG2(sirDumpBuf(pMac, SIR_DBG_MODULE_ID, LOG2, pFrame, nFrame);)
        return eSIR_FAILURE;
    }
    else if ( DOT11F_WARNED( status ) )
    {
      limLog( pMac, LOGW, FL("There were warnings while unpacking a Probe Response (0x%08x, %d bytes):\n"),
                 status, nFrame );
        PELOG2(sirDumpBuf(pMac, SIR_DBG_MODULE_ID, LOG2, pFrame, nFrame);)
    }

    // & "transliterate" from a 'tDot11fProbeResponse' to a 'tSirProbeRespBeacon'...

    // Timestamp
    palCopyMemory( pMac->hHdd, ( tANI_U8* )pProbeResp->timeStamp, ( tANI_U8* )&pr.TimeStamp, sizeof(tSirMacTimeStamp) );

    // Beacon Interval
    pProbeResp->beaconInterval = pr.BeaconInterval.interval;

    // Capabilities
    pProbeResp->capabilityInfo.ess            = pr.Capabilities.ess;
    pProbeResp->capabilityInfo.ibss           = pr.Capabilities.ibss;
    pProbeResp->capabilityInfo.cfPollable     = pr.Capabilities.cfPollable;
    pProbeResp->capabilityInfo.cfPollReq      = pr.Capabilities.cfPollReq;
    pProbeResp->capabilityInfo.privacy        = pr.Capabilities.privacy;
    pProbeResp->capabilityInfo.shortPreamble  = pr.Capabilities.shortPreamble;
    pProbeResp->capabilityInfo.pbcc           = pr.Capabilities.pbcc;
    pProbeResp->capabilityInfo.channelAgility = pr.Capabilities.channelAgility;
    pProbeResp->capabilityInfo.spectrumMgt    = pr.Capabilities.spectrumMgt;
    pProbeResp->capabilityInfo.qos            = pr.Capabilities.qos;
    pProbeResp->capabilityInfo.shortSlotTime  = pr.Capabilities.shortSlotTime;
    pProbeResp->capabilityInfo.apsd           = pr.Capabilities.apsd;
    pProbeResp->capabilityInfo.rrm            = pr.Capabilities.rrm;
    pProbeResp->capabilityInfo.dsssOfdm       = pr.Capabilities.dsssOfdm;
    pProbeResp->capabilityInfo.delayedBA       = pr.Capabilities.delayedBA;
    pProbeResp->capabilityInfo.immediateBA    = pr.Capabilities.immediateBA;

    if ( ! pr.SSID.present )
    {
        PELOGW(limLog(pMac, LOGW, FL("Mandatory IE SSID not present!\n"));)
    }
    else
    {
        pProbeResp->ssidPresent = 1;
        ConvertSSID( pMac, &pProbeResp->ssId, &pr.SSID );
    }

    if ( ! pr.SuppRates.present )
    {
        PELOGW(limLog(pMac, LOGW, FL("Mandatory IE Supported Rates not present!\n"));)
    }
    else
    {
        pProbeResp->suppRatesPresent = 1;
        ConvertSuppRates( pMac, &pProbeResp->supportedRates, &pr.SuppRates );
    }

    if ( pr.ExtSuppRates.present )
    {
        pProbeResp->extendedRatesPresent = 1;
        ConvertExtSuppRates( pMac, &pProbeResp->extendedRates, &pr.ExtSuppRates );
    }

    if ( pr.DSParams.present )
    {
        pProbeResp->dsParamsPresent = 1;
        pProbeResp->channelNumber = pr.DSParams.curr_channel;
    }

    if ( pr.CFParams.present )
    {
        pProbeResp->cfPresent = 1;
        ConvertCFParams( pMac, &pProbeResp->cfParamSet, &pr.CFParams );
    }

    if ( pr.Country.present )
    {
        pProbeResp->countryInfoPresent = 1;
        ConvertCountry( pMac, &pProbeResp->countryInfoParam, &pr.Country );
    }

    if ( pr.EDCAParamSet.present )
    {
        pProbeResp->edcaPresent = 1;
        ConvertEDCAParam( pMac, &pProbeResp->edcaParams, &pr.EDCAParamSet );
    }

    if ( pr.ChanSwitchAnn.present )
    {
        pProbeResp->channelSwitchPresent = 1;
        palCopyMemory( pMac, &pProbeResp->channelSwitchIE, &pr.ChanSwitchAnn,
			                                    sizeof(tDot11fIEExtChanSwitchAnn) );
    }

       if ( pr.ExtChanSwitchAnn.present )
    {
        pProbeResp->extChannelSwitchPresent = 1;
        palCopyMemory( pMac, &pProbeResp->extChannelSwitchIE, &pr.ExtChanSwitchAnn,
			                                    sizeof(tDot11fIEExtChanSwitchAnn) );
    }

    if( pr.TPCReport.present)
    {
        pProbeResp->tpcReportPresent = 1;
        palCopyMemory(pMac->hHdd, &pProbeResp->tpcReport, &pr.TPCReport, sizeof(tDot11fIETPCReport));
    }

    if( pr.PowerConstraints.present)
    {
        pProbeResp->powerConstraintPresent = 1;
        palCopyMemory(pMac->hHdd, &pProbeResp->localPowerConstraint, &pr.PowerConstraints, sizeof(tDot11fIEPowerConstraints));
    }

    if ( pr.Quiet.present )
    {
        pProbeResp->quietIEPresent = 1;
        palCopyMemory( pMac, &pProbeResp->quietIE, &pr.Quiet, sizeof(tDot11fIEQuiet) );
    }

    if ( pr.HTCaps.present )
    {
        palCopyMemory( pMac, &pProbeResp->HTCaps, &pr.HTCaps, sizeof( tDot11fIEHTCaps ) );
    }

    if ( pr.HTInfo.present )
    {
        palCopyMemory( pMac, &pProbeResp->HTInfo, &pr.HTInfo, sizeof( tDot11fIEHTInfo ) );
    }

    if ( pr.RSN.present )
    {
        pProbeResp->rsnPresent = 1;
        ConvertRSN( pMac, &pProbeResp->rsn, &pr.RSN );
    }

    if ( pr.WPA.present )
    {
        pProbeResp->wpaPresent = 1;
        ConvertWPA( pMac, &pProbeResp->wpa, &pr.WPA );
    }

    if ( pr.WMMParams.present )
    {
        pProbeResp->wmeEdcaPresent = 1;
        ConvertWMMParams( pMac, &pProbeResp->edcaParams, &pr.WMMParams );
        PELOG1(limLog(pMac, LOG1, FL("WMM Parameter present in Probe Response Frame!\n"));
                                __printWMMParams(pMac, &pr.WMMParams);)
    }

    if ( pr.WMMInfoAp.present )
    {
        pProbeResp->wmeInfoPresent = 1;
        PELOG1(limLog(pMac, LOG1, FL("WMM Information Element present in Probe Response Frame!\n"));)
    }

    if ( pr.WMMCaps.present )
    {
        pProbeResp->wsmCapablePresent = 1;
    }


    if ( pr.ERPInfo.present )
    {
        pProbeResp->erpPresent = 1;
        ConvertERPInfo( pMac, &pProbeResp->erpIEInfo, &pr.ERPInfo );
    }

#ifdef WLAN_FEATURE_VOWIFI_11R
    if (pr.MobilityDomain.present)
    {
        // MobilityDomain
        pProbeResp->mdiePresent = 1;
        palCopyMemory( pMac->hHdd, (tANI_U8 *)&(pProbeResp->mdie[0]), (tANI_U8 *)&(pr.MobilityDomain.MDID), sizeof(tANI_U16) );
        pProbeResp->mdie[2] = ((pr.MobilityDomain.overDSCap << 0) | (pr.MobilityDomain.resourceReqCap << 1));
#ifdef WLAN_FEATURE_VOWIFI_11R_DEBUG
        limLog(pMac, LOGE, FL("mdie=%02x%02x%02x\n"), (unsigned int)pProbeResp->mdie[0],
			(unsigned int)pProbeResp->mdie[1], (unsigned int)pProbeResp->mdie[2]);
#endif
    }
#endif
    return eSIR_SUCCESS;

} // End sirConvertProbeFrame2Struct.

tSirRetStatus
sirConvertAssocReqFrame2Struct(tpAniSirGlobal pMac,
                               tANI_U8            *pFrame,
                               tANI_U32            nFrame,
                               tpSirAssocReq  pAssocReq)
{
    tDot11fAssocRequest ar;
    tANI_U32                 status;

    // Zero-init our [out] parameter,
    palZeroMemory( pMac->hHdd, ( tANI_U8* )pAssocReq, sizeof(tSirAssocReq) );
    palZeroMemory( pMac->hHdd, ( tANI_U8* )&ar, sizeof( tDot11fAssocRequest ) );

    // delegate to the framesc-generated code,
    status = dot11fUnpackAssocRequest( pMac, pFrame, nFrame, &ar );
    if ( DOT11F_FAILED( status ) )
    {
        limLog(pMac, LOGE, FL("Failed to parse an Association Request (0x%08x, %d bytes):\n"),
                  status, nFrame);
        PELOG2(sirDumpBuf(pMac, SIR_DBG_MODULE_ID, LOG2, pFrame, nFrame);)
        return eSIR_FAILURE;
    }
    else if ( DOT11F_WARNED( status ) )
    {
      limLog( pMac, LOGW, FL("There were warnings while unpacking an Assoication Request (0x%08x, %d bytes):\n"),
                 status, nFrame );
        PELOG2(sirDumpBuf(pMac, SIR_DBG_MODULE_ID, LOG2, pFrame, nFrame);)
    }

    // & "transliterate" from a 'tDot11fAssocRequest' to a 'tSirAssocReq'...

    // make sure this is seen as an assoc request
    pAssocReq->reassocRequest = 0;

    // Capabilities
    pAssocReq->capabilityInfo.ess            = ar.Capabilities.ess;
    pAssocReq->capabilityInfo.ibss           = ar.Capabilities.ibss;
    pAssocReq->capabilityInfo.cfPollable     = ar.Capabilities.cfPollable;
    pAssocReq->capabilityInfo.cfPollReq      = ar.Capabilities.cfPollReq;
    pAssocReq->capabilityInfo.privacy        = ar.Capabilities.privacy;
    pAssocReq->capabilityInfo.shortPreamble  = ar.Capabilities.shortPreamble;
    pAssocReq->capabilityInfo.pbcc           = ar.Capabilities.pbcc;
    pAssocReq->capabilityInfo.channelAgility = ar.Capabilities.channelAgility;
    pAssocReq->capabilityInfo.spectrumMgt    = ar.Capabilities.spectrumMgt;
    pAssocReq->capabilityInfo.qos            = ar.Capabilities.qos;
    pAssocReq->capabilityInfo.shortSlotTime  = ar.Capabilities.shortSlotTime;
    pAssocReq->capabilityInfo.apsd           = ar.Capabilities.apsd;
    pAssocReq->capabilityInfo.rrm            = ar.Capabilities.rrm;
    pAssocReq->capabilityInfo.dsssOfdm       = ar.Capabilities.dsssOfdm;
    pAssocReq->capabilityInfo.delayedBA       = ar.Capabilities.delayedBA;
    pAssocReq->capabilityInfo.immediateBA    = ar.Capabilities.immediateBA;

    // Listen Interval
    pAssocReq->listenInterval = ar.ListenInterval.interval;

    // SSID
    if ( ar.SSID.present )
    {
        pAssocReq->ssidPresent = 1;
        ConvertSSID( pMac, &pAssocReq->ssId, &ar.SSID );
    }

    // Supported Rates
    if ( ar.SuppRates.present )
    {
        pAssocReq->suppRatesPresent = 1;
        ConvertSuppRates( pMac, &pAssocReq->supportedRates, &ar.SuppRates );
    }

    // Extended Supported Rates
    if ( ar.ExtSuppRates.present )
    {
        pAssocReq->extendedRatesPresent = 1;
        ConvertExtSuppRates( pMac, &pAssocReq->extendedRates, &ar.ExtSuppRates );
    }

    // QOS Capabilities:
    if ( ar.QOSCapsStation.present )
    {
        pAssocReq->qosCapabilityPresent = 1;
        ConvertQOSCapsStation( pMac, &pAssocReq->qosCapability, &ar.QOSCapsStation );
    }

    // WPA
    if ( ar.WPAOpaque.present )
    {
        pAssocReq->wpaPresent = 1;
        ConvertWPAOpaque( pMac, &pAssocReq->wpa, &ar.WPAOpaque );
    }

    // RSN
    if ( ar.RSNOpaque.present )
    {
        pAssocReq->rsnPresent = 1;
        ConvertRSNOpaque( pMac, &pAssocReq->rsn, &ar.RSNOpaque );
    }


    // Power Capabilities
    if ( ar.PowerCaps.present )
    {
        pAssocReq->powerCapabilityPresent     = 1;
        ConvertPowerCaps( pMac, &pAssocReq->powerCapability, &ar.PowerCaps );
    }

    // Supported Channels
    if ( ar.SuppChannels.present )
    {
        pAssocReq->supportedChannelsPresent = 1;
        ConvertSuppChannels( pMac, &pAssocReq->supportedChannels, &ar.SuppChannels );
    }

    if ( ar.HTCaps.present )
    {
        palCopyMemory( pMac, &pAssocReq->HTCaps, &ar.HTCaps, sizeof( tDot11fIEHTCaps ) );
    }

    if ( ar.WMMInfoStation.present )
    {
        pAssocReq->wmeInfoPresent = 1;
#ifdef WLAN_SOFTAP_FEATURE
        palCopyMemory( pMac, &pAssocReq->WMMInfoStation, &ar.WMMInfoStation, sizeof( tDot11fIEWMMInfoStation ) );
#endif

    }


    if ( ar.WMMCaps.present ) pAssocReq->wsmCapablePresent = 1;

    if ( ! pAssocReq->ssidPresent )
    {
        PELOG2(limLog(pMac, LOG2, FL("Received Assoc without SSID IE.\n"));)
        return eSIR_FAILURE;
    }

    if (!pAssocReq->suppRatesPresent)
    {
        PELOG2(limLog(pMac, LOG2, FL("Received Assoc without supp rate IE.\n"));)
        return eSIR_FAILURE;
    }

    /* Initialize wscInfo. */
    memset(&pAssocReq->wscInfo, 0, sizeof(tSirMacWscInfo));

    if ( ar.WscAssocReq.present )
    {
        pAssocReq->wscInfo.present = 1;

        /* Copy the element info from tDot11fAssocRequest(ar) to
					 tSirAssocReq(pAssocReq) structure, if present */

        if (ar.WscAssocReq.Version.present){
                /* Get the version from Minor and Major; Also consider endianness. */
					pAssocReq->wscInfo.wpsVersion   = ( (ar.WscAssocReq.Version.major << 4)
																							| ar.WscAssocReq.Version.minor );
        }

        if (ar.WscAssocReq.RequestType.present){
            pAssocReq->wscInfo.wpsRequestType   = ar.WscAssocReq.RequestType.reqType;
    }
	}

    return eSIR_SUCCESS;

} // End sirConvertAssocReqFrame2Struct.

tSirRetStatus
sirConvertAssocRespFrame2Struct(tpAniSirGlobal pMac,
                                tANI_U8            *pFrame,
                                tANI_U32            nFrame,
                                tpSirAssocRsp  pAssocRsp)
{
    tDot11fAssocResponse ar;
    tANI_U32                  status;

    // Zero-init our [out] parameter,
    palZeroMemory( pMac->hHdd, ( tANI_U8* )pAssocRsp, sizeof(tSirAssocRsp) );

    // delegate to the framesc-generated code,
    status = dot11fUnpackAssocResponse( pMac, pFrame, nFrame, &ar);
    if ( DOT11F_FAILED( status ) )
    {
        limLog(pMac, LOGE, FL("Failed to parse an Association Response (0x%08x, %d bytes):\n"),
                  status, nFrame);
        PELOG2(sirDumpBuf(pMac, SIR_DBG_MODULE_ID, LOG2, pFrame, nFrame);)
        return eSIR_FAILURE;
    }
    else if ( DOT11F_WARNED( status ) )
    {
        limLog( pMac, LOGW, FL("There were warnings while unpacking an Association Response (0x%08x, %d bytes):\n"),
                   status, nFrame );
        PELOG2(sirDumpBuf(pMac, SIR_DBG_MODULE_ID, LOG2, pFrame, nFrame);)
    }

    // & "transliterate" from a 'tDot11fAssocResponse' a 'tSirAssocRsp'...

    // Capabilities
    pAssocRsp->capabilityInfo.ess            = ar.Capabilities.ess;
    pAssocRsp->capabilityInfo.ibss           = ar.Capabilities.ibss;
    pAssocRsp->capabilityInfo.cfPollable     = ar.Capabilities.cfPollable;
    pAssocRsp->capabilityInfo.cfPollReq      = ar.Capabilities.cfPollReq;
    pAssocRsp->capabilityInfo.privacy        = ar.Capabilities.privacy;
    pAssocRsp->capabilityInfo.shortPreamble  = ar.Capabilities.shortPreamble;
    pAssocRsp->capabilityInfo.pbcc           = ar.Capabilities.pbcc;
    pAssocRsp->capabilityInfo.channelAgility = ar.Capabilities.channelAgility;
    pAssocRsp->capabilityInfo.spectrumMgt    = ar.Capabilities.spectrumMgt;
    pAssocRsp->capabilityInfo.qos            = ar.Capabilities.qos;
    pAssocRsp->capabilityInfo.shortSlotTime  = ar.Capabilities.shortSlotTime;
    pAssocRsp->capabilityInfo.apsd           = ar.Capabilities.apsd;
    pAssocRsp->capabilityInfo.rrm            = ar.Capabilities.rrm;
    pAssocRsp->capabilityInfo.dsssOfdm       = ar.Capabilities.dsssOfdm;
    pAssocRsp->capabilityInfo.delayedBA       = ar.Capabilities.delayedBA;
    pAssocRsp->capabilityInfo.immediateBA    = ar.Capabilities.immediateBA;

    pAssocRsp->statusCode = ar.Status.status;
    pAssocRsp->aid        = ar.AID.associd;

    if ( ! ar.SuppRates.present )
    {
        PELOGW(limLog(pMac, LOGW, FL("Mandatory IE Supported Rates not present!\n"));)
        return eSIR_FAILURE;
    }
    else
    {
        pAssocRsp->suppRatesPresent = 1;
        ConvertSuppRates( pMac, &pAssocRsp->supportedRates, &ar.SuppRates );
    }

    if ( ar.ExtSuppRates.present )
    {
        pAssocRsp->extendedRatesPresent = 1;
        ConvertExtSuppRates( pMac, &pAssocRsp->extendedRates, &ar.ExtSuppRates );
    }

    if ( ar.EDCAParamSet.present )
    {
        pAssocRsp->edcaPresent = 1;
        ConvertEDCAParam( pMac, &pAssocRsp->edca, &ar.EDCAParamSet );
    }


    if ( ar.WMMParams.present )
    {
        pAssocRsp->wmeEdcaPresent = 1;
        ConvertWMMParams( pMac, &pAssocRsp->edca, &ar.WMMParams);
        limLog(pMac, LOGE, FL("WMM Parameter Element present in Association Response Frame!\n"));
        __printWMMParams(pMac, &ar.WMMParams);
    }

    if ( ar.HTCaps.present )
    {
        palCopyMemory( pMac, &pAssocRsp->HTCaps, &ar.HTCaps, sizeof( tDot11fIEHTCaps ) );
    }

    if ( ar.HTInfo.present )
    {
        palCopyMemory( pMac, &pAssocRsp->HTInfo, &ar.HTInfo, sizeof( tDot11fIEHTInfo ) );
    }

#ifdef WLAN_FEATURE_VOWIFI_11R
    if (ar.MobilityDomain.present)
    {
        // MobilityDomain
        pAssocRsp->mdiePresent = 1;
        palCopyMemory( pMac->hHdd, (tANI_U8 *)&(pAssocRsp->mdie[0]), (tANI_U8 *)&(ar.MobilityDomain.MDID), sizeof(tANI_U16) );
        pAssocRsp->mdie[2] = ((ar.MobilityDomain.overDSCap << 0) | (ar.MobilityDomain.resourceReqCap << 1));
#ifdef WLAN_FEATURE_VOWIFI_11R_DEBUG
        limLog(pMac, LOGE, FL("new mdie=%02x%02x%02x\n"), (unsigned int)pAssocRsp->mdie[0],
			(unsigned int)pAssocRsp->mdie[1], (unsigned int)pAssocRsp->mdie[2]);
#endif
    }

    if ( ar.FTInfo.present )
    {
#ifdef WLAN_FEATURE_VOWIFI_11R_DEBUG
        limLog(pMac, LOGE, FL("FT Info present %d %d %d\n"), ar.FTInfo.R0KH_ID.num_PMK_R0_ID,
                ar.FTInfo.R0KH_ID.present,
                ar.FTInfo.R1KH_ID.present);
#endif
        pAssocRsp->ftinfoPresent = 1;
        palCopyMemory( pMac, &pAssocRsp->FTInfo, &ar.FTInfo, sizeof(tDot11fIEFTInfo) );
    }
#endif

    return eSIR_SUCCESS;

} // End sirConvertAssocRespFrame2Struct.

tSirRetStatus
sirConvertReassocReqFrame2Struct(tpAniSirGlobal pMac,
                                 tANI_U8            *pFrame,
                                 tANI_U32            nFrame,
                                 tpSirAssocReq  pAssocReq)
{
    tDot11fReAssocRequest ar;
    tANI_U32                   status;

    // Zero-init our [out] parameter,
    palZeroMemory( pMac->hHdd, ( tANI_U8* )pAssocReq, sizeof(tSirAssocReq) );

    // delegate to the framesc-generated code,
    status = dot11fUnpackReAssocRequest( pMac, pFrame, nFrame, &ar );
    if ( DOT11F_FAILED( status ) )
    {
        limLog(pMac, LOGE, FL("Failed to parse a Re-association Request (0x%08x, %d bytes):\n"),
                  status, nFrame);
        PELOG2(sirDumpBuf(pMac, SIR_DBG_MODULE_ID, LOG2, pFrame, nFrame);)
        return eSIR_FAILURE;
    }
    else if ( DOT11F_WARNED( status ) )
    {
      limLog( pMac, LOGW, FL("There were warnings while unpacking a Re-association Request (0x%08x, %d bytes):\n"),
                 status, nFrame );
        PELOG2(sirDumpBuf(pMac, SIR_DBG_MODULE_ID, LOG2, pFrame, nFrame);)
    }

    // & "transliterate" from a 'tDot11fReAssocRequest' to a 'tSirAssocReq'...

    // make sure this is seen as a re-assoc request
    pAssocReq->reassocRequest = 1;

    // Capabilities
    pAssocReq->capabilityInfo.ess            = ar.Capabilities.ess;
    pAssocReq->capabilityInfo.ibss           = ar.Capabilities.ibss;
    pAssocReq->capabilityInfo.cfPollable     = ar.Capabilities.cfPollable;
    pAssocReq->capabilityInfo.cfPollReq      = ar.Capabilities.cfPollReq;
    pAssocReq->capabilityInfo.privacy        = ar.Capabilities.privacy;
    pAssocReq->capabilityInfo.shortPreamble  = ar.Capabilities.shortPreamble;
    pAssocReq->capabilityInfo.pbcc           = ar.Capabilities.pbcc;
    pAssocReq->capabilityInfo.channelAgility = ar.Capabilities.channelAgility;
    pAssocReq->capabilityInfo.spectrumMgt    = ar.Capabilities.spectrumMgt;
    pAssocReq->capabilityInfo.qos            = ar.Capabilities.qos;
    pAssocReq->capabilityInfo.shortSlotTime  = ar.Capabilities.shortSlotTime;
    pAssocReq->capabilityInfo.apsd           = ar.Capabilities.apsd;
    pAssocReq->capabilityInfo.rrm            = ar.Capabilities.rrm;
    pAssocReq->capabilityInfo.dsssOfdm      = ar.Capabilities.dsssOfdm;
    pAssocReq->capabilityInfo.delayedBA       = ar.Capabilities.delayedBA;
    pAssocReq->capabilityInfo.immediateBA    = ar.Capabilities.immediateBA;

    // Listen Interval
    pAssocReq->listenInterval = ar.ListenInterval.interval;

    // SSID
    if ( ar.SSID.present )
    {
        pAssocReq->ssidPresent = 1;
        ConvertSSID( pMac, &pAssocReq->ssId, &ar.SSID );
    }

    // Supported Rates
    if ( ar.SuppRates.present )
    {
        pAssocReq->suppRatesPresent = 1;
        ConvertSuppRates( pMac, &pAssocReq->supportedRates, &ar.SuppRates );
    }

    // Extended Supported Rates
    if ( ar.ExtSuppRates.present )
    {
        pAssocReq->extendedRatesPresent = 1;
        ConvertExtSuppRates( pMac, &pAssocReq->extendedRates,
                             &ar.ExtSuppRates );
    }

    // QOS Capabilities:
    if ( ar.QOSCapsStation.present )
    {
        pAssocReq->qosCapabilityPresent = 1;
        ConvertQOSCapsStation( pMac, &pAssocReq->qosCapability, &ar.QOSCapsStation );
    }

    // WPA
    if ( ar.WPAOpaque.present )
    {
        pAssocReq->wpaPresent = 1;
        ConvertWPAOpaque( pMac, &pAssocReq->wpa, &ar.WPAOpaque );
    }

    // RSN
    if ( ar.RSNOpaque.present )
    {
        pAssocReq->rsnPresent = 1;
        ConvertRSNOpaque( pMac, &pAssocReq->rsn, &ar.RSNOpaque );
    }


    // Power Capabilities
    if ( ar.PowerCaps.present )
    {
        pAssocReq->powerCapabilityPresent     = 1;
        ConvertPowerCaps( pMac, &pAssocReq->powerCapability, &ar.PowerCaps );
    }

    // Supported Channels
    if ( ar.SuppChannels.present )
    {
        pAssocReq->supportedChannelsPresent = 1;
        ConvertSuppChannels( pMac, &pAssocReq->supportedChannels, &ar.SuppChannels );
    }

    if ( ar.HTCaps.present )
    {
        palCopyMemory( pMac, &pAssocReq->HTCaps, &ar.HTCaps, sizeof( tDot11fIEHTCaps ) );
    }

    if ( ar.WMMInfoStation.present )
    {
        pAssocReq->wmeInfoPresent = 1;
#ifdef WLAN_SOFTAP_FEATURE
        palCopyMemory( pMac, &pAssocReq->WMMInfoStation, &ar.WMMInfoStation, sizeof( tDot11fIEWMMInfoStation ) );
#endif

    }

    if ( ar.WMMCaps.present ) pAssocReq->wsmCapablePresent = 1;

    if ( ! pAssocReq->ssidPresent )
    {
        PELOG2(limLog(pMac, LOG2, FL("Received Assoc without SSID IE.\n"));)
        return eSIR_FAILURE;
    }

    if ( ! pAssocReq->suppRatesPresent )
    {
        PELOG2(limLog(pMac, LOG2, FL("Received Assoc without supp rate IE.\n"));)
        return eSIR_FAILURE;
    }

    // Why no call to 'updateAssocReqFromPropCapability' here, like
    // there is in 'sirConvertAssocReqFrame2Struct'?

		memset(&pAssocReq->wscInfo, 0, sizeof(tSirMacWscInfo));

    if ( ar.WscAssocReq.present )
    {
        /* Copy the element info from tDot11fAssocRequest(ar) to tSirAssocReq(pAssocReq)
           structure, if present */
        if (ar.WscAssocReq.Version.present){
                /* Get the version from Minor and Major; Also consider endianness. */
                pAssocReq->wscInfo.wpsVersion   = (ar.WscAssocReq.Version.major << 4) | ar.WscAssocReq.Version.minor;
        }

        if (ar.WscAssocReq.RequestType.present){
            pAssocReq->wscInfo.wpsRequestType   = ar.WscAssocReq.RequestType.reqType;
        }
    }

    return eSIR_SUCCESS;

} // End sirConvertReassocReqFrame2Struct.

tSirRetStatus
sirParseBeaconIE(tpAniSirGlobal        pMac,
                 tpSirProbeRespBeacon  pBeaconStruct,
                 tANI_U8                   *pPayload,
                 tANI_U32                   nPayload)
{
    tDot11fBeaconIEs bies;
    tANI_U32              status;

    // Zero-init our [out] parameter,
    palZeroMemory( pMac->hHdd, ( tANI_U8* )pBeaconStruct, sizeof(tSirProbeRespBeacon) );

    // delegate to the framesc-generated code,
    status = dot11fUnpackBeaconIEs( pMac, pPayload, nPayload, &bies );
    if ( DOT11F_FAILED( status ) )
    {
        limLog(pMac, LOGE, FL("Failed to parse Beacon IEs (0x%08x, %d bytes):\n"),
                  status, nPayload);
        PELOG2(sirDumpBuf(pMac, SIR_DBG_MODULE_ID, LOG2, pPayload, nPayload);)
        return eSIR_FAILURE;
    }
    else if ( DOT11F_WARNED( status ) )
    {
      limLog( pMac, LOGW, FL("There were warnings while unpacking Beacon IEs (0x%08x, %d bytes):\n"),
                 status, nPayload );
        PELOG2(sirDumpBuf(pMac, SIR_DBG_MODULE_ID, LOG2, pPayload, nPayload);)
    }

    // & "transliterate" from a 'tDot11fBeaconIEs' to a 'tSirProbeRespBeacon'...
    if ( ! bies.SSID.present )
    {
        PELOGW(limLog(pMac, LOGW, FL("Mandatory IE SSID not present!\n"));)
    }
    else
    {
        pBeaconStruct->ssidPresent = 1;
        ConvertSSID( pMac, &pBeaconStruct->ssId, &bies.SSID );
    }

    if ( ! bies.SuppRates.present )
    {
        PELOGW(limLog(pMac, LOGW, FL("Mandatory IE Supported Rates not present!\n"));)
    }
    else
    {
        pBeaconStruct->suppRatesPresent = 1;
        ConvertSuppRates( pMac, &pBeaconStruct->supportedRates, &bies.SuppRates );
    }

    if ( bies.ExtSuppRates.present )
    {
        pBeaconStruct->extendedRatesPresent = 1;
        ConvertExtSuppRates( pMac, &pBeaconStruct->extendedRates, &bies.ExtSuppRates );
    }

    if ( bies.DSParams.present )
    {
        pBeaconStruct->dsParamsPresent = 1;
        pBeaconStruct->channelNumber = bies.DSParams.curr_channel;
    }

    if ( bies.CFParams.present )
    {
        pBeaconStruct->cfPresent = 1;
        ConvertCFParams( pMac, &pBeaconStruct->cfParamSet, &bies.CFParams );
    }

    if ( bies.TIM.present )
    {
        pBeaconStruct->timPresent = 1;
        ConvertTIM( pMac, &pBeaconStruct->tim, &bies.TIM );
    }

    if ( bies.Country.present )
    {
        pBeaconStruct->countryInfoPresent = 1;
        ConvertCountry( pMac, &pBeaconStruct->countryInfoParam, &bies.Country );
    }

    // 11h IEs
    if(bies.TPCReport.present)
    {
        pBeaconStruct->tpcReportPresent = 1;
        palCopyMemory(pMac,
                      &pBeaconStruct->tpcReport,
                      &bies.TPCReport,
                      sizeof( tDot11fIETPCReport));
    }

    if(bies.PowerConstraints.present)
    {
        pBeaconStruct->powerConstraintPresent = 1;
        palCopyMemory(pMac,
                      &pBeaconStruct->localPowerConstraint,
                      &bies.PowerConstraints,
                      sizeof(tDot11fIEPowerConstraints));
    }

    if ( bies.EDCAParamSet.present )
    {
        pBeaconStruct->edcaPresent = 1;
        ConvertEDCAParam( pMac, &pBeaconStruct->edcaParams, &bies.EDCAParamSet );
    }

    // QOS Capabilities:
    if ( bies.QOSCapsAp.present )
    {
        pBeaconStruct->qosCapabilityPresent = 1;
        ConvertQOSCaps( pMac, &pBeaconStruct->qosCapability, &bies.QOSCapsAp );
    }



    if ( bies.ChanSwitchAnn.present )
    {
        pBeaconStruct->channelSwitchPresent = 1;
        palCopyMemory(pMac->hHdd, &pBeaconStruct->channelSwitchIE, &bies.ChanSwitchAnn,
			                                              sizeof(tDot11fIEChanSwitchAnn));
    }

    if ( bies.ExtChanSwitchAnn.present)
    {
        pBeaconStruct->extChannelSwitchPresent= 1;
        palCopyMemory(pMac->hHdd, &pBeaconStruct->extChannelSwitchIE, &bies.ExtChanSwitchAnn,
			                                              sizeof(tDot11fIEExtChanSwitchAnn));
    }

    if ( bies.Quiet.present )
    {
        pBeaconStruct->quietIEPresent = 1;
        palCopyMemory( pMac, &pBeaconStruct->quietIE, &bies.Quiet, sizeof(tDot11fIEQuiet) );
    }

    if ( bies.HTCaps.present )
    {
        palCopyMemory( pMac, &pBeaconStruct->HTCaps, &bies.HTCaps, sizeof( tDot11fIEHTCaps ) );
    }

    if ( bies.HTInfo.present )
    {
        palCopyMemory( pMac, &pBeaconStruct->HTInfo, &bies.HTInfo, sizeof( tDot11fIEHTInfo ) );
    }

    if ( bies.RSN.present )
    {
        pBeaconStruct->rsnPresent = 1;
        ConvertRSN( pMac, &pBeaconStruct->rsn, &bies.RSN );
    }

    if ( bies.WPA.present )
    {
        pBeaconStruct->wpaPresent = 1;
        ConvertWPA( pMac, &pBeaconStruct->wpa, &bies.WPA );
    }

    if ( bies.WMMParams.present )
    {
        pBeaconStruct->wmeEdcaPresent = 1;
        ConvertWMMParams( pMac, &pBeaconStruct->edcaParams, &bies.WMMParams );
    }

    if ( bies.WMMInfoAp.present )
    {
        pBeaconStruct->wmeInfoPresent = 1;
    }

    if ( bies.WMMCaps.present )
    {
        pBeaconStruct->wsmCapablePresent = 1;
    }


    if ( bies.ERPInfo.present )
    {
        pBeaconStruct->erpPresent = 1;
        ConvertERPInfo( pMac, &pBeaconStruct->erpIEInfo, &bies.ERPInfo );
    }

    return eSIR_SUCCESS;

} // End sirParseBeaconIE.

tSirRetStatus
sirConvertBeaconFrame2Struct(tpAniSirGlobal       pMac,
                             tANI_U8             *pFrame,
                             tpSirProbeRespBeacon pBeaconStruct)
{
    tDot11fBeacon   beacon;
    tANI_U32        status, nPayload;
    tANI_U8        *pPayload;
    tpSirMacMgmtHdr pHdr;

    pPayload = WDA_GET_RX_MPDU_DATA( pFrame );
    nPayload = WDA_GET_RX_PAYLOAD_LEN( pFrame );
    pHdr     = WDA_GET_RX_MAC_HEADER( pFrame );
    
    // Zero-init our [out] parameter,
    palZeroMemory( pMac->hHdd, ( tANI_U8* )pBeaconStruct, sizeof(tSirProbeRespBeacon) );
    palZeroMemory( pMac->hHdd, ( tANI_U8* )&beacon, sizeof(tDot11fBeacon) );

    // get the MAC address out of the BD,
    palCopyMemory( pMac->hHdd, pBeaconStruct->bssid, pHdr->sa, 6 );

    // delegate to the framesc-generated code,
    status = dot11fUnpackBeacon( pMac, pPayload, nPayload, &beacon );
    if ( DOT11F_FAILED( status ) )
    {
        limLog(pMac, LOGE, FL("Failed to parse Beacon IEs (0x%08x, %d bytes):\n"),
                  status, nPayload);
        PELOG2(sirDumpBuf(pMac, SIR_DBG_MODULE_ID, LOG2, pPayload, nPayload);)
        return eSIR_FAILURE;
    }
    else if ( DOT11F_WARNED( status ) )
    {
      limLog( pMac, LOGW, FL("There were warnings while unpacking Beacon IEs (0x%08x, %d bytes):\n"),
                 status, nPayload );
        PELOG2(sirDumpBuf(pMac, SIR_DBG_MODULE_ID, LOG2, pPayload, nPayload);)
    }

    // & "transliterate" from a 'tDot11fBeacon' to a 'tSirProbeRespBeacon'...
    // Timestamp
    palCopyMemory( pMac->hHdd, ( tANI_U8* )pBeaconStruct->timeStamp, ( tANI_U8* )&beacon.TimeStamp, sizeof(tSirMacTimeStamp) );

    // Beacon Interval
    pBeaconStruct->beaconInterval = beacon.BeaconInterval.interval;

    // Capabilities
    pBeaconStruct->capabilityInfo.ess            = beacon.Capabilities.ess;
    pBeaconStruct->capabilityInfo.ibss           = beacon.Capabilities.ibss;
    pBeaconStruct->capabilityInfo.cfPollable     = beacon.Capabilities.cfPollable;
    pBeaconStruct->capabilityInfo.cfPollReq      = beacon.Capabilities.cfPollReq;
    pBeaconStruct->capabilityInfo.privacy        = beacon.Capabilities.privacy;
    pBeaconStruct->capabilityInfo.shortPreamble  = beacon.Capabilities.shortPreamble;
    pBeaconStruct->capabilityInfo.pbcc           = beacon.Capabilities.pbcc;
    pBeaconStruct->capabilityInfo.channelAgility = beacon.Capabilities.channelAgility;
    pBeaconStruct->capabilityInfo.spectrumMgt    = beacon.Capabilities.spectrumMgt;
    pBeaconStruct->capabilityInfo.qos            = beacon.Capabilities.qos;
    pBeaconStruct->capabilityInfo.shortSlotTime  = beacon.Capabilities.shortSlotTime;
    pBeaconStruct->capabilityInfo.apsd           = beacon.Capabilities.apsd;
    pBeaconStruct->capabilityInfo.rrm            = beacon.Capabilities.rrm;
    pBeaconStruct->capabilityInfo.dsssOfdm      = beacon.Capabilities.dsssOfdm;
    pBeaconStruct->capabilityInfo.delayedBA     = beacon.Capabilities.delayedBA;
    pBeaconStruct->capabilityInfo.immediateBA    = beacon.Capabilities.immediateBA;
 
    if ( ! beacon.SSID.present )
    {
        PELOGW(limLog(pMac, LOGW, FL("Mandatory IE SSID not present!\n"));)
    }
    else
    {
        pBeaconStruct->ssidPresent = 1;
        ConvertSSID( pMac, &pBeaconStruct->ssId, &beacon.SSID );
    }

    if ( ! beacon.SuppRates.present )
    {
        PELOGW(limLog(pMac, LOGW, FL("Mandatory IE Supported Rates not present!\n"));)
    }
    else
    {
        pBeaconStruct->suppRatesPresent = 1;
        ConvertSuppRates( pMac, &pBeaconStruct->supportedRates, &beacon.SuppRates );
    }

    if ( beacon.ExtSuppRates.present )
    {
        pBeaconStruct->extendedRatesPresent = 1;
        ConvertExtSuppRates( pMac, &pBeaconStruct->extendedRates, &beacon.ExtSuppRates );
    }

    if ( beacon.DSParams.present )
    {
        pBeaconStruct->dsParamsPresent = 1;
        pBeaconStruct->channelNumber = beacon.DSParams.curr_channel;
    }

    if ( beacon.CFParams.present )
    {
        pBeaconStruct->cfPresent = 1;
        ConvertCFParams( pMac, &pBeaconStruct->cfParamSet, &beacon.CFParams );
    }

    if ( beacon.TIM.present )
    {
        pBeaconStruct->timPresent = 1;
        ConvertTIM( pMac, &pBeaconStruct->tim, &beacon.TIM );
    }

    if ( beacon.Country.present )
    {
        pBeaconStruct->countryInfoPresent = 1;
        ConvertCountry( pMac, &pBeaconStruct->countryInfoParam, &beacon.Country );
    }

    // QOS Capabilities:
    if ( beacon.QOSCapsAp.present )
    {
        pBeaconStruct->qosCapabilityPresent = 1;
        ConvertQOSCaps( pMac, &pBeaconStruct->qosCapability, &beacon.QOSCapsAp );
    }

    if ( beacon.EDCAParamSet.present )
    {
        pBeaconStruct->edcaPresent = 1;
        ConvertEDCAParam( pMac, &pBeaconStruct->edcaParams, &beacon.EDCAParamSet );
    }

    if ( beacon.ChanSwitchAnn.present )
    {
        pBeaconStruct->channelSwitchPresent = 1;
        palCopyMemory( pMac, &pBeaconStruct->channelSwitchIE, &beacon.ChanSwitchAnn,
                                                       sizeof(tDot11fIEChanSwitchAnn) );
    }

    if ( beacon.ExtChanSwitchAnn.present )
    {
        pBeaconStruct->extChannelSwitchPresent = 1;
        palCopyMemory( pMac, &pBeaconStruct->extChannelSwitchIE, &beacon.ExtChanSwitchAnn,
                                                       sizeof(tDot11fIEExtChanSwitchAnn) );
    }

    if( beacon.TPCReport.present)
    {
        pBeaconStruct->tpcReportPresent = 1;
        palCopyMemory(pMac->hHdd, &pBeaconStruct->tpcReport, &beacon.TPCReport,
                                                     sizeof(tDot11fIETPCReport));
    }

    if( beacon.PowerConstraints.present)
    {
        pBeaconStruct->powerConstraintPresent = 1;
        palCopyMemory(pMac->hHdd, &pBeaconStruct->localPowerConstraint, &beacon.PowerConstraints,
                                                               sizeof(tDot11fIEPowerConstraints));
    }

    if ( beacon.Quiet.present )
    {
        pBeaconStruct->quietIEPresent = 1;
        palCopyMemory( pMac, &pBeaconStruct->quietIE, &beacon.Quiet, sizeof(tDot11fIEQuiet));
    }

    if ( beacon.HTCaps.present )
    {
        palCopyMemory( pMac, &pBeaconStruct->HTCaps, &beacon.HTCaps, sizeof( tDot11fIEHTCaps ) );
    }

    if ( beacon.HTInfo.present )
    {
        palCopyMemory( pMac, &pBeaconStruct->HTInfo, &beacon.HTInfo, sizeof( tDot11fIEHTInfo) );

    }

    if ( beacon.RSN.present )
    {
        pBeaconStruct->rsnPresent = 1;
        ConvertRSN( pMac, &pBeaconStruct->rsn, &beacon.RSN );
    }

    if ( beacon.WPA.present )
    {
        pBeaconStruct->wpaPresent = 1;
        ConvertWPA( pMac, &pBeaconStruct->wpa, &beacon.WPA );
    }

    if ( beacon.WMMParams.present )
    {
        pBeaconStruct->wmeEdcaPresent = 1;
        ConvertWMMParams( pMac, &pBeaconStruct->edcaParams, &beacon.WMMParams );
        PELOG1(limLog(pMac, LOG1, FL("WMM Parameter present in Beacon Frame!\n"));
        __printWMMParams(pMac, &beacon.WMMParams); )
    }

    if ( beacon.WMMInfoAp.present )
    {
        pBeaconStruct->wmeInfoPresent = 1;
        PELOG1(limLog(pMac, LOG1, FL("WMM Info present in Beacon Frame!\n"));)
    }

    if ( beacon.WMMCaps.present )
    {
        pBeaconStruct->wsmCapablePresent = 1;
    }

    if ( beacon.ERPInfo.present )
    {
        pBeaconStruct->erpPresent = 1;
        ConvertERPInfo( pMac, &pBeaconStruct->erpIEInfo, &beacon.ERPInfo );
    }

#ifdef WLAN_FEATURE_VOWIFI_11R
    if (beacon.MobilityDomain.present)
    {
        // MobilityDomain
        pBeaconStruct->mdiePresent = 1;
        palCopyMemory( pMac->hHdd, (tANI_U8 *)&(pBeaconStruct->mdie[0]), (tANI_U8 *)&(beacon.MobilityDomain.MDID), sizeof(tANI_U16) );
        pBeaconStruct->mdie[2] = ((beacon.MobilityDomain.overDSCap << 0) | (beacon.MobilityDomain.resourceReqCap << 1));

    }
#endif

    return eSIR_SUCCESS;

} // End sirConvertBeaconFrame2Struct.

tSirRetStatus
sirConvertAuthFrame2Struct(tpAniSirGlobal        pMac,
                           tANI_U8                   *pFrame,
                           tANI_U32                   nFrame,
                           tpSirMacAuthFrameBody pAuth)
{
    tDot11fAuthentication auth;
    tANI_U32                   status;

    // Zero-init our [out] parameter,
    palZeroMemory( pMac->hHdd, ( tANI_U8* )pAuth, sizeof(tSirMacAuthFrameBody) );

    // delegate to the framesc-generated code,
    status = dot11fUnpackAuthentication( pMac, pFrame, nFrame, &auth );
    if ( DOT11F_FAILED( status ) )
    {
        limLog(pMac, LOGE, FL("Failed to parse an Authentication frame (0x%08x, %d bytes):\n"),
                  status, nFrame);
        PELOG2(sirDumpBuf(pMac, SIR_DBG_MODULE_ID, LOG2, pFrame, nFrame);)
        return eSIR_FAILURE;
    }
    else if ( DOT11F_WARNED( status ) )
    {
      limLog( pMac, LOGW, FL("There were warnings while unpacking an Authentication frame (0x%08x, %d bytes):\n"),
                 status, nFrame );
        PELOG2(sirDumpBuf(pMac, SIR_DBG_MODULE_ID, LOG2, pFrame, nFrame);)
    }

    // & "transliterate" from a 'tDot11fAuthentication' to a 'tSirMacAuthFrameBody'...
    pAuth->authAlgoNumber           = auth.AuthAlgo.algo;
    pAuth->authTransactionSeqNumber = auth.AuthSeqNo.no;
    pAuth->authStatusCode           = auth.Status.status;

    if ( auth.ChallengeText.present )
    {
        pAuth->type   = SIR_MAC_CHALLENGE_TEXT_EID;
        pAuth->length = auth.ChallengeText.num_text;
        palCopyMemory( pMac->hHdd, pAuth->challengeText, auth.ChallengeText.text, auth.ChallengeText.num_text );
    }

    return eSIR_SUCCESS;

} // End sirConvertAuthFrame2Struct.

tSirRetStatus
sirConvertAddtsReq2Struct(tpAniSirGlobal    pMac,
                          tANI_U8               *pFrame,
                          tANI_U32               nFrame,
                          tSirAddtsReqInfo *pAddTs)
{
	tDot11fAddTSRequest    addts = {{0}};
	tDot11fWMMAddTSRequest wmmaddts = {{0}};
    tANI_U8                     j;
    tANI_U16                    i;
    tANI_U32                    status;

    if ( SIR_MAC_QOS_ADD_TS_REQ != *( pFrame + 1 ) )
    {
        limLog( pMac, LOGE, FL("sirConvertAddtsReq2Struct invoked "
                                  "with an Action of %d; this is not "
                                  "supported & is probably an error."),
                   *( pFrame + 1 ) );
        return eSIR_FAILURE;
    }

    // Zero-init our [out] parameter,
    palZeroMemory( pMac->hHdd, ( tANI_U8* )pAddTs, sizeof(tSirAddtsReqInfo) );

    // delegate to the framesc-generated code,
    switch ( *pFrame )
    {
    case SIR_MAC_ACTION_QOS_MGMT:
        status = dot11fUnpackAddTSRequest( pMac, pFrame, nFrame, &addts );
        break;
    case SIR_MAC_ACTION_WME:
        status = dot11fUnpackWMMAddTSRequest( pMac, pFrame, nFrame, &wmmaddts );
        break;
    default:
        limLog( pMac, LOGE, FL("sirConvertAddtsReq2Struct invoked "
                                  "with a Category of %d; this is not"
                                  " supported & is probably an error."),
                   *pFrame );
        return eSIR_FAILURE;
    }

    if ( DOT11F_FAILED( status ) )
    {
        limLog(pMac, LOGE, FL("Failed to parse an Add TS Request f"
                                 "rame (0x%08x, %d bytes):\n"),
                  status, nFrame);
        PELOG2(sirDumpBuf(pMac, SIR_DBG_MODULE_ID, LOG2, pFrame, nFrame);)
        return eSIR_FAILURE;
    }
    else if ( DOT11F_WARNED( status ) )
    {
        limLog( pMac, LOGW, FL("There were warnings while unpackin"
                                  "g an Add TS Request frame (0x%08x,"
                                  "%d bytes):\n"),
                   status, nFrame );
        PELOG2(sirDumpBuf(pMac, SIR_DBG_MODULE_ID, LOG2, pFrame, nFrame);)
    }

    // & "transliterate" from a 'tDot11fAddTSRequest' or a
    // 'tDot11WMMAddTSRequest' to a 'tSirMacAddtsReqInfo'...
    if ( SIR_MAC_ACTION_QOS_MGMT == *pFrame )
    {
        pAddTs->dialogToken = addts.DialogToken.token;

        if ( addts.TSPEC.present )
        {
            ConvertTSPEC( pMac, &pAddTs->tspec, &addts.TSPEC );
        }
        else
        {
            limLog( pMac, LOGE, FL("Mandatory TSPEC element missing in Add TS Request.\n") );
            return eSIR_FAILURE;
        }

        if ( addts.num_TCLAS )
        {
            pAddTs->numTclas = (tANI_U8)addts.num_TCLAS;

            for ( i = 0U; i < addts.num_TCLAS; ++i )
            {
                if ( eSIR_SUCCESS != ConvertTCLAS( pMac, &( pAddTs->tclasInfo[i] ), &( addts.TCLAS[i] ) ) )
                {
                    limLog( pMac, LOGE, FL("Failed to convert a TCLAS IE.\n") );
                    return eSIR_FAILURE;
                }
            }
        }

        if ( addts.TCLASSPROC.present )
        {
            pAddTs->tclasProcPresent = 1;
            pAddTs->tclasProc = addts.TCLASSPROC.processing;
        }

        if ( addts.WMMTSPEC.present )
        {
            pAddTs->wsmTspecPresent = 1;
            ConvertWMMTSPEC( pMac, &pAddTs->tspec, &addts.WMMTSPEC );
        }

        if ( addts.num_WMMTCLAS )
        {
            j = pAddTs->numTclas + addts.num_WMMTCLAS;
            if ( SIR_MAC_TCLASIE_MAXNUM > j ) j = SIR_MAC_TCLASIE_MAXNUM;

            for ( i = pAddTs->numTclas; i < j; ++i )
            {
                if ( eSIR_SUCCESS != ConvertWMMTCLAS( pMac, &( pAddTs->tclasInfo[i] ), &( addts.WMMTCLAS[i] ) ) )
                {
                    limLog( pMac, LOGE, FL("Failed to convert a TCLAS IE.\n") );
                    return eSIR_FAILURE;
                }
            }
        }

        if ( addts.WMMTCLASPROC.present )
        {
            pAddTs->tclasProcPresent = 1;
            pAddTs->tclasProc = addts.WMMTCLASPROC.processing;
        }

        if ( 1 < pAddTs->numTclas && ( ! pAddTs->tclasProcPresent ) )
        {
            limLog( pMac, LOGE, FL("%d TCLAS IE but not TCLASPROC IE.\n"),
                       pAddTs->numTclas );
            return eSIR_FAILURE;
        }
    }
    else
    {
        pAddTs->dialogToken = wmmaddts.DialogToken.token;

        if ( wmmaddts.WMMTSPEC.present )
        {
            pAddTs->wmeTspecPresent = 1;
            ConvertWMMTSPEC( pMac, &pAddTs->tspec, &wmmaddts.WMMTSPEC );
        }
        else
        {
            limLog( pMac, LOGE, FL("Mandatory WME TSPEC element missing!\n") );
            return eSIR_FAILURE;
        }
    }

    return eSIR_SUCCESS;

} // End sirConvertAddtsReq2Struct.

tSirRetStatus
sirConvertAddtsRsp2Struct(tpAniSirGlobal    pMac,
                          tANI_U8               *pFrame,
                          tANI_U32               nFrame,
                          tSirAddtsRspInfo *pAddTs)
{
	tDot11fAddTSResponse    addts = {{0}};
	tDot11fWMMAddTSResponse wmmaddts = {{0}};
    tANI_U8                      j;
    tANI_U16                     i;
    tANI_U32                     status;

    if ( SIR_MAC_QOS_ADD_TS_RSP != *( pFrame + 1 ) )
    {
        limLog( pMac, LOGE, FL("sirConvertAddtsRsp2Struct invoked "
                                  "with an Action of %d; this is not "
                                  "supported & is probably an error."),
                   *( pFrame + 1 ) );
        return eSIR_FAILURE;
    }

    // Zero-init our [out] parameter,
    palZeroMemory( pMac->hHdd, ( tANI_U8* )pAddTs, sizeof(tSirAddtsRspInfo) );
    palZeroMemory( pMac->hHdd, ( tANI_U8* )&addts, sizeof(tDot11fAddTSResponse) );
    palZeroMemory( pMac->hHdd, ( tANI_U8* )&wmmaddts, sizeof(tDot11fWMMAddTSResponse) );


    // delegate to the framesc-generated code,
    switch ( *pFrame )
    {
    case SIR_MAC_ACTION_QOS_MGMT:
        status = dot11fUnpackAddTSResponse( pMac, pFrame, nFrame, &addts );
        break;
    case SIR_MAC_ACTION_WME:
        status = dot11fUnpackWMMAddTSResponse( pMac, pFrame, nFrame, &wmmaddts );
        break;
    default:
        limLog( pMac, LOGE, FL("sirConvertAddtsRsp2Struct invoked "
                                  "with a Category of %d; this is not"
                                  " supported & is probably an error."),
                   *pFrame );
        return eSIR_FAILURE;
    }

    if ( DOT11F_FAILED( status ) )
    {
        limLog(pMac, LOGE, FL("Failed to parse an Add TS Response f"
                                 "rame (0x%08x, %d bytes):\n"),
                  status, nFrame);
        PELOG2(sirDumpBuf(pMac, SIR_DBG_MODULE_ID, LOG2, pFrame, nFrame);)
        return eSIR_FAILURE;
    }
    else if ( DOT11F_WARNED( status ) )
    {
        limLog( pMac, LOGW, FL("There were warnings while unpackin"
                                  "g an Add TS Response frame (0x%08x,"
                                  "%d bytes):\n"),
                   status, nFrame );
        PELOG2(sirDumpBuf(pMac, SIR_DBG_MODULE_ID, LOG2, pFrame, nFrame);)
    }

    // & "transliterate" from a 'tDot11fAddTSResponse' or a
    // 'tDot11WMMAddTSResponse' to a 'tSirMacAddtsRspInfo'...
    if ( SIR_MAC_ACTION_QOS_MGMT == *pFrame )
    {
        pAddTs->dialogToken = addts.DialogToken.token;
        pAddTs->status      = ( tSirMacStatusCodes )addts.Status.status;

        if ( addts.TSDelay.present )
        {
            ConvertTSDelay( pMac, &pAddTs->delay, &addts.TSDelay );
        }

        // TS Delay is present iff status indicates its presence
        if ( eSIR_MAC_TS_NOT_CREATED_STATUS == pAddTs->status && ! addts.TSDelay.present )
        {
            limLog( pMac, LOGW, FL("Missing TSDelay IE.\n") );
        }

        if ( addts.TSPEC.present )
        {
            ConvertTSPEC( pMac, &pAddTs->tspec, &addts.TSPEC );
        }
        else
        {
            limLog( pMac, LOGE, FL("Mandatory TSPEC element missing in Add TS Response.\n") );
            return eSIR_FAILURE;
        }

        if ( addts.num_TCLAS )
        {
            pAddTs->numTclas = (tANI_U8)addts.num_TCLAS;

            for ( i = 0U; i < addts.num_TCLAS; ++i )
            {
                if ( eSIR_SUCCESS != ConvertTCLAS( pMac, &( pAddTs->tclasInfo[i] ), &( addts.TCLAS[i] ) ) )
                {
                    limLog( pMac, LOGE, FL("Failed to convert a TCLAS IE.\n") );
                    return eSIR_FAILURE;
                }
            }
        }

        if ( addts.TCLASSPROC.present )
        {
            pAddTs->tclasProcPresent = 1;
            pAddTs->tclasProc = addts.TCLASSPROC.processing;
        }

        if ( addts.Schedule.present )
        {
            pAddTs->schedulePresent = 1;
            ConvertSchedule( pMac, &pAddTs->schedule, &addts.Schedule );
        }

        if ( addts.WMMSchedule.present )
        {
            pAddTs->schedulePresent = 1;
            ConvertWMMSchedule( pMac, &pAddTs->schedule, &addts.WMMSchedule );
        }

        if ( addts.WMMTSPEC.present )
        {
            pAddTs->wsmTspecPresent = 1;
            ConvertWMMTSPEC( pMac, &pAddTs->tspec, &addts.WMMTSPEC );
        }

        if ( addts.num_WMMTCLAS )
        {
            j = pAddTs->numTclas + addts.num_WMMTCLAS;
            if ( SIR_MAC_TCLASIE_MAXNUM > j ) j = SIR_MAC_TCLASIE_MAXNUM;

            for ( i = pAddTs->numTclas; i < j; ++i )
            {
                if ( eSIR_SUCCESS != ConvertWMMTCLAS( pMac, &( pAddTs->tclasInfo[i] ), &( addts.WMMTCLAS[i] ) ) )
                {
                    limLog( pMac, LOGE, FL("Failed to convert a TCLAS IE.\n") );
                    return eSIR_FAILURE;
                }
            }
        }

        if ( addts.WMMTCLASPROC.present )
        {
            pAddTs->tclasProcPresent = 1;
            pAddTs->tclasProc = addts.WMMTCLASPROC.processing;
        }

        if ( 1 < pAddTs->numTclas && ( ! pAddTs->tclasProcPresent ) )
        {
            limLog( pMac, LOGE, FL("%d TCLAS IE but not TCLASPROC IE.\n"),
                       pAddTs->numTclas );
            return eSIR_FAILURE;
        }
    }
    else
    {
        pAddTs->dialogToken = wmmaddts.DialogToken.token;
        pAddTs->status      = ( tSirMacStatusCodes )wmmaddts.StatusCode.statusCode;

        if ( wmmaddts.WMMTSPEC.present )
        {
            pAddTs->wmeTspecPresent = 1;
            ConvertWMMTSPEC( pMac, &pAddTs->tspec, &wmmaddts.WMMTSPEC );
        }
        else
        {
            limLog( pMac, LOGE, FL("Mandatory WME TSPEC element missing!\n") );
            return eSIR_FAILURE;
        }

    }

    return eSIR_SUCCESS;

} // End sirConvertAddtsRsp2Struct.

tSirRetStatus
sirConvertDeltsReq2Struct(tpAniSirGlobal    pMac,
                          tANI_U8               *pFrame,
                          tANI_U32               nFrame,
                          tSirDeltsReqInfo *pDelTs)
{
	tDot11fDelTS    delts = {{0}};
	tDot11fWMMDelTS wmmdelts = {{0}};
    tANI_U32             status;

    if ( SIR_MAC_QOS_DEL_TS_REQ != *( pFrame + 1 ) )
    {
        limLog( pMac, LOGE, FL("sirConvertDeltsRsp2Struct invoked "
                                  "with an Action of %d; this is not "
                                  "supported & is probably an error."),
                   *( pFrame + 1 ) );
        return eSIR_FAILURE;
    }

    // Zero-init our [out] parameter,
    palZeroMemory( pMac->hHdd, ( tANI_U8* )pDelTs, sizeof(tSirDeltsReqInfo) );

    // delegate to the framesc-generated code,
    switch ( *pFrame )
    {
    case SIR_MAC_ACTION_QOS_MGMT:
        status = dot11fUnpackDelTS( pMac, pFrame, nFrame, &delts );
        break;
    case SIR_MAC_ACTION_WME:
        status = dot11fUnpackWMMDelTS( pMac, pFrame, nFrame, &wmmdelts );
        break;
    default:
        limLog( pMac, LOGE, FL("sirConvertDeltsRsp2Struct invoked "
                                  "with a Category of %d; this is not"
                                  " supported & is probably an error."),
                   *pFrame );
        return eSIR_FAILURE;
    }

    if ( DOT11F_FAILED( status ) )
    {
        limLog(pMac, LOGE, FL("Failed to parse an Del TS Request f"
                                 "rame (0x%08x, %d bytes):\n"),
                  status, nFrame);
        PELOG2(sirDumpBuf(pMac, SIR_DBG_MODULE_ID, LOG2, pFrame, nFrame);)
        return eSIR_FAILURE;
    }
    else if ( DOT11F_WARNED( status ) )
    {
        dot11fLog( pMac, LOGW, FL("There were warnings while unpackin"
                                  "g an Del TS Request frame (0x%08x,"
                                  "%d bytes):\n"),
                   status, nFrame );
        PELOG2(sirDumpBuf(pMac, SIR_DBG_MODULE_ID, LOG2, pFrame, nFrame);)
    }

    // & "transliterate" from a 'tDot11fDelTSResponse' or a
    // 'tDot11WMMDelTSResponse' to a 'tSirMacDeltsReqInfo'...
    if ( SIR_MAC_ACTION_QOS_MGMT == *pFrame )
    {
        pDelTs->tsinfo.traffic.trafficType  = (tANI_U16)delts.TSInfo.traffic_type;
        pDelTs->tsinfo.traffic.tsid         = (tANI_U16)delts.TSInfo.tsid;
        pDelTs->tsinfo.traffic.direction    = (tANI_U16)delts.TSInfo.direction;
        pDelTs->tsinfo.traffic.accessPolicy = (tANI_U16)delts.TSInfo.access_policy;
        pDelTs->tsinfo.traffic.aggregation  = (tANI_U16)delts.TSInfo.aggregation;
        pDelTs->tsinfo.traffic.psb          = (tANI_U16)delts.TSInfo.psb;
        pDelTs->tsinfo.traffic.userPrio     = (tANI_U16)delts.TSInfo.user_priority;
        pDelTs->tsinfo.traffic.ackPolicy    = (tANI_U16)delts.TSInfo.tsinfo_ack_pol;

        pDelTs->tsinfo.schedule.schedule    = (tANI_U8)delts.TSInfo.schedule;
    }
    else
    {
        if ( wmmdelts.WMMTSPEC.present )
        {
            pDelTs->wmeTspecPresent = 1;
            ConvertWMMTSPEC( pMac, &pDelTs->tspec, &wmmdelts.WMMTSPEC );
        }
        else
        {
            dot11fLog( pMac, LOGE, FL("Mandatory WME TSPEC element missing!\n") );
            return eSIR_FAILURE;
        }
    }

    return eSIR_SUCCESS;

} // End sirConvertDeltsReq2Struct.


#ifdef ANI_SUPPORT_11H
tSirRetStatus
sirConvertTpcReqFrame2Struct(tpAniSirGlobal            pMac,
                             tANI_U8                       *pFrame,
                             tpSirMacTpcReqActionFrame pTpcReqFrame,
                             tANI_U32                       nFrame)
{
    tDot11fTPCRequest     req;
    tANI_U32                   status;

    // Zero-init our [out] parameter,
    palZeroMemory( pMac->hHdd, ( tANI_U8* )pTpcReqFrame, sizeof(tSirMacTpcReqActionFrame) );

    // delegate to the framesc-generated code,
    status = dot11fUnpackTPCRequest( pMac, pFrame, nFrame, &req );
    if ( DOT11F_FAILED( status ) )
    {
        dot11fLog(pMac, LOGE, FL("Failed to parse a TPC Request frame (0x%08x, %d bytes):\n"),
                  status, nFrame);
        PELOG2(sirDumpBuf(pMac, SIR_DBG_MODULE_ID, LOG2, pFrame, nFrame);)
        return eSIR_FAILURE;
    }
    else if ( DOT11F_WARNED( status ) )
    {
      dot11fLog( pMac, LOGW, FL("There were warnings while unpacking a TPC Request frame (0x%08x, %d bytes):\n"),
                 status, nFrame );
        PELOG2(sirDumpBuf(pMac, SIR_DBG_MODULE_ID, LOG2, pFrame, nFrame);)
    }

    // & "transliterate" from a 'tDot11fTPCRequest' to a
    // 'tSirMacTpcReqActionFrame'...
    pTpcReqFrame->actionHeader.category    = req.Category.category;
    pTpcReqFrame->actionHeader.actionID    = req.Action.action;
    pTpcReqFrame->actionHeader.dialogToken = req.DialogToken.token;
    if ( req.TPCRequest.present )
    {
        pTpcReqFrame->type   = DOT11F_EID_TPCREQUEST;
        pTpcReqFrame->length = 0;
    }
    else
    {
        dot11fLog( pMac, LOGW, FL("!!!Rcv TPC Req of inalid type!\n") );
        return eSIR_FAILURE;
    }

    return eSIR_SUCCESS;

} // End sirConvertTpcReqFrame2Struct.


tSirRetStatus
sirConvertMeasReqFrame2Struct(tpAniSirGlobal             pMac,
                              tANI_U8                        *pFrame,
                              tpSirMacMeasReqActionFrame pMeasReqFrame,
                              tANI_U32                        nFrame)
{
    tDot11fMeasurementRequest mr;
    tANI_U32                       status;

    // Zero-init our [out] parameter,
    palZeroMemory( pMac->hHdd, ( tANI_U8* )pMeasReqFrame, sizeof(tpSirMacMeasReqActionFrame) );

    // delegate to the framesc-generated code,
    status = dot11fUnpackMeasurementRequest( pMac, pFrame, nFrame, &mr );
    if ( DOT11F_FAILED( status ) )
    {
        dot11fLog(pMac, LOGE, FL("Failed to parse a Measurement Request frame (0x%08x, %d bytes):\n"),
                  status, nFrame);
        PELOG2(sirDumpBuf(pMac, SIR_DBG_MODULE_ID, LOG2, pFrame, nFrame);)
        return eSIR_FAILURE;
    }
    else if ( DOT11F_WARNED( status ) )
    {
      dot11fLog( pMac, LOGW, FL("There were warnings while unpacking a Measurement Request frame (0x%08x, %d bytes):\n"),
                 status, nFrame );
        PELOG2(sirDumpBuf(pMac, SIR_DBG_MODULE_ID, LOG2, pFrame, nFrame);)
    }

    // & "transliterate" from a 'tDot11fMeasurementRequest' to a
    // 'tpSirMacMeasReqActionFrame'...
    pMeasReqFrame->actionHeader.category    = mr.Category.category;
    pMeasReqFrame->actionHeader.actionID    = mr.Action.action;
    pMeasReqFrame->actionHeader.dialogToken = mr.DialogToken.token;

    if ( 0 == mr.num_MeasurementRequest )
    {
        dot11fLog( pMac, LOGE, FL("Missing mandatory IE in Measurement Request Frame.\n") );
        return eSIR_FAILURE;
    }
    else if ( 1 < mr.num_MeasurementRequest )
    {
        limLog( pMac, LOGW, FL("Warning: dropping extra Measurement Request IEs!") );
    }

    pMeasReqFrame->measReqIE.type        = DOT11F_EID_MEASUREMENTREQUEST;
    pMeasReqFrame->measReqIE.length      = DOT11F_IE_MEASUREMENTREQUEST_MIN_LEN;
    pMeasReqFrame->measReqIE.measToken   = mr.MeasurementRequest[0].measurement_token;
    pMeasReqFrame->measReqIE.measReqMode = ( mr.MeasurementRequest[0].reserved << 3 ) |
                                           ( mr.MeasurementRequest[0].enable   << 2 ) |
                                           ( mr.MeasurementRequest[0].request  << 1 ) |
                                           ( mr.MeasurementRequest[0].report /*<< 0*/ );
    pMeasReqFrame->measReqIE.measType    = mr.MeasurementRequest[0].measurement_type;

    pMeasReqFrame->measReqIE.measReqField.channelNumber = mr.MeasurementRequest[0].channel_no;

    palCopyMemory( pMac->hHdd, pMeasReqFrame->measReqIE.measReqField.measStartTime,
                   mr.MeasurementRequest[0].meas_start_time, 8 );

    pMeasReqFrame->measReqIE.measReqField.measDuration = mr.MeasurementRequest[0].meas_duration;

    return eSIR_SUCCESS;

} // End sirConvertMeasReqFrame2Struct.
#endif


void
PopulateDot11fTSPEC(tSirMacTspecIE  *pOld,
                    tDot11fIETSPEC  *pDot11f)
{
    pDot11f->traffic_type         = pOld->tsinfo.traffic.trafficType;
    pDot11f->tsid                 = pOld->tsinfo.traffic.tsid;
    pDot11f->direction            = pOld->tsinfo.traffic.direction;
    pDot11f->access_policy        = pOld->tsinfo.traffic.accessPolicy;
    pDot11f->aggregation          = pOld->tsinfo.traffic.aggregation;
    pDot11f->psb                  = pOld->tsinfo.traffic.psb;
    pDot11f->user_priority        = pOld->tsinfo.traffic.userPrio;
    pDot11f->tsinfo_ack_pol       = pOld->tsinfo.traffic.ackPolicy;
    pDot11f->schedule             = pOld->tsinfo.schedule.schedule;
    /* As defined in IEEE 802.11-2007, section 7.3.2.30
     * Nominal MSDU size: Bit[0:14]=Size, Bit[15]=Fixed
     */
    pDot11f->size                 = ( pOld->nomMsduSz & 0x7fff );
    pDot11f->fixed                = ( pOld->nomMsduSz & 0x8000 ) ? 1 : 0;
    pDot11f->max_msdu_size        = pOld->maxMsduSz;
    pDot11f->min_service_int      = pOld->minSvcInterval;
    pDot11f->max_service_int      = pOld->maxSvcInterval;
    pDot11f->inactivity_int       = pOld->inactInterval;
    pDot11f->suspension_int       = pOld->suspendInterval;
    pDot11f->service_start_time   = pOld->svcStartTime;
    pDot11f->min_data_rate        = pOld->minDataRate;
    pDot11f->mean_data_rate       = pOld->meanDataRate;
    pDot11f->peak_data_rate       = pOld->peakDataRate;
    pDot11f->burst_size           = pOld->maxBurstSz;
    pDot11f->delay_bound          = pOld->delayBound;
    pDot11f->min_phy_rate         = pOld->minPhyRate;
    pDot11f->surplus_bw_allowance = pOld->surplusBw;
    pDot11f->medium_time          = pOld->mediumTime;

    pDot11f->present = 1;

} // End PopulateDot11fTSPEC.

void
PopulateDot11fWMMTSPEC(tSirMacTspecIE     *pOld,
                       tDot11fIEWMMTSPEC  *pDot11f)
{
    pDot11f->traffic_type         = pOld->tsinfo.traffic.trafficType;
    pDot11f->tsid                 = pOld->tsinfo.traffic.tsid;
    pDot11f->direction            = pOld->tsinfo.traffic.direction;
    pDot11f->access_policy        = pOld->tsinfo.traffic.accessPolicy;
    pDot11f->aggregation          = pOld->tsinfo.traffic.aggregation;
    pDot11f->psb                  = pOld->tsinfo.traffic.psb;
    pDot11f->user_priority        = pOld->tsinfo.traffic.userPrio;
    pDot11f->tsinfo_ack_pol       = pOld->tsinfo.traffic.ackPolicy;
    pDot11f->burst_size_defn      = pOld->tsinfo.traffic.burstSizeDefn;
    /* As defined in IEEE 802.11-2007, section 7.3.2.30
     * Nominal MSDU size: Bit[0:14]=Size, Bit[15]=Fixed
     */
    pDot11f->size                 = ( pOld->nomMsduSz & 0x7fff );
    pDot11f->fixed                = ( pOld->nomMsduSz & 0x8000 ) ? 1 : 0;
    pDot11f->max_msdu_size        = pOld->maxMsduSz;
    pDot11f->min_service_int      = pOld->minSvcInterval;
    pDot11f->max_service_int      = pOld->maxSvcInterval;
    pDot11f->inactivity_int       = pOld->inactInterval;
    pDot11f->suspension_int       = pOld->suspendInterval;
    pDot11f->service_start_time   = pOld->svcStartTime;
    pDot11f->min_data_rate        = pOld->minDataRate;
    pDot11f->mean_data_rate       = pOld->meanDataRate;
    pDot11f->peak_data_rate       = pOld->peakDataRate;
    pDot11f->burst_size           = pOld->maxBurstSz;
    pDot11f->delay_bound          = pOld->delayBound;
    pDot11f->min_phy_rate         = pOld->minPhyRate;
    pDot11f->surplus_bw_allowance = pOld->surplusBw;
    pDot11f->medium_time          = pOld->mediumTime;

    pDot11f->version = 1;
    pDot11f->present = 1;

} // End PopulateDot11fWMMTSPEC.

tSirRetStatus
PopulateDot11fTCLAS(tpAniSirGlobal  pMac,
                    tSirTclasInfo  *pOld,
                    tDot11fIETCLAS *pDot11f)
{
    pDot11f->user_priority   = pOld->tclas.userPrio;
    pDot11f->classifier_type = pOld->tclas.classifierType;
    pDot11f->classifier_mask = pOld->tclas.classifierMask;

    switch ( pDot11f->classifier_type )
    {
    case SIR_MAC_TCLASTYPE_ETHERNET:
        palCopyMemory( pMac->hHdd, ( tANI_U8* )&pDot11f->info.EthParams.source,
                       ( tANI_U8* )&pOld->tclasParams.eth.srcAddr, 6 );
        palCopyMemory( pMac->hHdd, ( tANI_U8* )&pDot11f->info.EthParams.dest,
                       ( tANI_U8* )&pOld->tclasParams.eth.dstAddr, 6 );
        pDot11f->info.EthParams.type = pOld->tclasParams.eth.type;
        break;
    case SIR_MAC_TCLASTYPE_TCPUDPIP:
        pDot11f->info.IpParams.version = pOld->version;
        if ( SIR_MAC_TCLAS_IPV4 == pDot11f->info.IpParams.version )
        {
            palCopyMemory( pMac->hHdd, ( tANI_U8* )&pDot11f->info.IpParams.params.
                           IpV4Params.source,
                           ( tANI_U8* )pOld->tclasParams.ipv4.srcIpAddr, 4 );
            palCopyMemory( pMac->hHdd, ( tANI_U8* )&pDot11f->info.IpParams.params.
                           IpV4Params.dest,
                           ( tANI_U8* )pOld->tclasParams.ipv4.dstIpAddr, 4 );
            pDot11f->info.IpParams.params.IpV4Params.src_port  =
              pOld->tclasParams.ipv4.srcPort;
            pDot11f->info.IpParams.params.IpV4Params.dest_port =
              pOld->tclasParams.ipv4.dstPort;
            pDot11f->info.IpParams.params.IpV4Params.DSCP      =
              pOld->tclasParams.ipv4.dscp;
            pDot11f->info.IpParams.params.IpV4Params.proto     =
              pOld->tclasParams.ipv4.protocol;
            pDot11f->info.IpParams.params.IpV4Params.reserved  =
              pOld->tclasParams.ipv4.rsvd;
        }
        else
        {
            palCopyMemory( pMac->hHdd, ( tANI_U8* )&pDot11f->info.IpParams.params.
                           IpV6Params.source,
                           ( tANI_U8* )pOld->tclasParams.ipv6.srcIpAddr, 16 );
            palCopyMemory( pMac->hHdd, ( tANI_U8* )&pDot11f->info.IpParams.params.
                           IpV6Params.dest,
                           ( tANI_U8* )pOld->tclasParams.ipv6.dstIpAddr, 16 );
            pDot11f->info.IpParams.params.IpV6Params.src_port  =
              pOld->tclasParams.ipv6.srcPort;
            pDot11f->info.IpParams.params.IpV6Params.dest_port =
              pOld->tclasParams.ipv6.dstPort;
            palCopyMemory( pMac->hHdd, ( tANI_U8* )&pDot11f->info.IpParams.params.
                           IpV6Params.flow_label,
                           ( tANI_U8* )pOld->tclasParams.ipv6.flowLabel, 3 );
        }
        break;
    case SIR_MAC_TCLASTYPE_8021DQ:
        pDot11f->info.Params8021dq.tag_type = pOld->tclasParams.t8021dq.tag;
        break;
    default:
        limLog( pMac, LOGE, FL("Bad TCLAS type %d in PopulateDot11fTCLAS.\n"),
                pDot11f->classifier_type );
        return eSIR_FAILURE;
    }

    pDot11f->present = 1;

    return eSIR_SUCCESS;

} // End PopulateDot11fTCLAS.

tSirRetStatus
PopulateDot11fWMMTCLAS(tpAniSirGlobal     pMac,
                       tSirTclasInfo     *pOld,
                       tDot11fIEWMMTCLAS *pDot11f)
{
    pDot11f->version         = 1;
    pDot11f->user_priority   = pOld->tclas.userPrio;
    pDot11f->classifier_type = pOld->tclas.classifierType;
    pDot11f->classifier_mask = pOld->tclas.classifierMask;

    switch ( pDot11f->classifier_type )
    {
    case SIR_MAC_TCLASTYPE_ETHERNET:
        palCopyMemory( pMac->hHdd, ( tANI_U8* )&pDot11f->info.EthParams.source,
                       ( tANI_U8* )&pOld->tclasParams.eth.srcAddr, 6 );
        palCopyMemory( pMac->hHdd, ( tANI_U8* )&pDot11f->info.EthParams.dest,
                       ( tANI_U8* )&pOld->tclasParams.eth.dstAddr, 6 );
        pDot11f->info.EthParams.type = pOld->tclasParams.eth.type;
        break;
    case SIR_MAC_TCLASTYPE_TCPUDPIP:
        pDot11f->info.IpParams.version = pOld->version;
        if ( SIR_MAC_TCLAS_IPV4 == pDot11f->info.IpParams.version )
        {
            palCopyMemory( pMac->hHdd, ( tANI_U8* )&pDot11f->info.IpParams.params.
                           IpV4Params.source,
                           ( tANI_U8* )pOld->tclasParams.ipv4.srcIpAddr, 4 );
            palCopyMemory( pMac->hHdd, ( tANI_U8* )&pDot11f->info.IpParams.params.
                           IpV4Params.dest,
                           ( tANI_U8* )pOld->tclasParams.ipv4.dstIpAddr, 4 );
            pDot11f->info.IpParams.params.IpV4Params.src_port  =
              pOld->tclasParams.ipv4.srcPort;
            pDot11f->info.IpParams.params.IpV4Params.dest_port =
              pOld->tclasParams.ipv4.dstPort;
            pDot11f->info.IpParams.params.IpV4Params.DSCP      =
              pOld->tclasParams.ipv4.dscp;
            pDot11f->info.IpParams.params.IpV4Params.proto     =
              pOld->tclasParams.ipv4.protocol;
            pDot11f->info.IpParams.params.IpV4Params.reserved  =
              pOld->tclasParams.ipv4.rsvd;
        }
        else
        {
            palCopyMemory( pMac->hHdd, ( tANI_U8* )&pDot11f->info.IpParams.params.
                           IpV6Params.source,
                           ( tANI_U8* )pOld->tclasParams.ipv6.srcIpAddr, 16 );
            palCopyMemory( pMac->hHdd, ( tANI_U8* )&pDot11f->info.IpParams.params.
                           IpV6Params.dest,
                           ( tANI_U8* )pOld->tclasParams.ipv6.dstIpAddr, 16 );
            pDot11f->info.IpParams.params.IpV6Params.src_port  =
              pOld->tclasParams.ipv6.srcPort;
            pDot11f->info.IpParams.params.IpV6Params.dest_port =
              pOld->tclasParams.ipv6.dstPort;
            palCopyMemory( pMac->hHdd, ( tANI_U8* )&pDot11f->info.IpParams.params.
                           IpV6Params.flow_label,
                           ( tANI_U8* )pOld->tclasParams.ipv6.flowLabel, 3 );
        }
        break;
    case SIR_MAC_TCLASTYPE_8021DQ:
        pDot11f->info.Params8021dq.tag_type = pOld->tclasParams.t8021dq.tag;
        break;
    default:
        limLog( pMac, LOGE, FL("Bad TCLAS type %d in PopulateDot11fTCLAS.\n"),
                pDot11f->classifier_type );
        return eSIR_FAILURE;
    }

    pDot11f->present = 1;

    return eSIR_SUCCESS;

} // End PopulateDot11fWMMTCLAS.

#if ( WNI_POLARIS_FW_PRODUCT == AP )

tSirRetStatus
PopulateDot11fCFParams(tpAniSirGlobal         pMac,
                       tDot11fFfCapabilities *pCaps,
                       tDot11fIECFParams     *pDot11f)
{
    tANI_U32           cfg;
    tSirRetStatus nSirStatus;

    if ( pCaps->cfPollable || pCaps->cfPollReq )
    {
        pDot11f->cfp_count = schGetCFPCount( pMac );

        CFG_GET_INT( nSirStatus, pMac, WNI_CFG_CFP_PERIOD, cfg );
        pDot11f->cfp_period = ( cfg & 0x000000ff );

        CFG_GET_INT( nSirStatus, pMac, WNI_CFG_CFP_MAX_DURATION, cfg );
        pDot11f->cfp_maxduration = ( cfg & 0x0000ffff );

        pDot11f->cfp_durremaining = schGetCFPDurRemaining( pMac );

        pDot11f->present = 1;
    }

    return eSIR_SUCCESS;
} // End PopulateDot11fCFParams.

void
PopulateDot11fQuiet(tpAniSirGlobal  pMac,
                    tDot11fIEQuiet *pDot11f)
{
    // Quiet Count - Num TBTTs until start of quiet interval
    pDot11f->count = (tANI_U8) pMac->lim.gLimSpecMgmt.quietCount;
    // Quiet Period
    pDot11f->period = 0;
    // Quiet Duration (in TU's)
    pDot11f->duration = ( tANI_U16 ) pMac->lim.gLimSpecMgmt.quietDuration_TU;
    // Quiet Offset
    pDot11f->offset = 0;

    dot11fLog( pMac, LOG2,
               FL("Filling QuietBSS IE: %d, Len %d, QC %1d, QP 0, QD "
                  "%2d(TU)/%2d(TICKS), QO 0\n"),
               SIR_MAC_QUIET_EID, DOT11F_IE_QUIET_MIN_LEN,
               pMac->lim.gLimSpecMgmt.quietCount,
               pMac->lim.gLimSpecMgmt.quietDuration_TU,
               pMac->lim.gLimSpecMgmt.quietDuration );

    pDot11f->present = 1;

} // End PopulateDot11fQuiet.

tSirRetStatus
PopulateDot11fAPName(tpAniSirGlobal   pMac,
                     tANI_U32              capEnable,
                     tDot11fIEAPName *pDot11f)
{
    tANI_U32           nName;
    tSirRetStatus nSirStatus;

    CFG_GET_STR( nSirStatus, pMac, WNI_CFG_AP_NODE_NAME, pDot11f->name,
                 nName, SIR_MAC_MAX_SSID_LENGTH );
    pDot11f->num_name = ( tANI_U8 ) nName;
    pDot11f->present = 1;

    return eSIR_SUCCESS;
} // End PopulateDot11fAPName.

void
PopulateDot11fPropQuietBSS(tpAniSirGlobal         pMac,
                           tANI_U32                    capsEnable,
                           tDot11fIEPropQuietBSS *pDot11f)
{
    // Populate Proprietary Quiet BSS IE, ONLY IF 11H is NOT enabled!  If 11H
    // IS enabled, then the Quiet IE will be appropriately sent as per the
    // 11H spec and hence need NOT be sent in the Proprietary IE as well
    if ( ( eLIM_QUIET_RUNNING == pMac->lim.gLimSpecMgmt.quietState ) &&
         !pMac->lim.gLim11hEnable  &&
         PROP_CAPABILITY_GET( TITAN, capsEnable ) )
    {
        // Quiet Count - Num TBTTs until start of quiet interval
        pDot11f->quiet_count = pMac->lim.gLimSpecMgmt.quietCount;
        // Quiet Period
        pDot11f->quiet_period = 0;
        // Quiet Duration (in TU's)
        pDot11f->quiet_duration = ( tANI_U16 ) pMac->lim.gLimSpecMgmt.quietDuration_TU;
        // Quiet Offset
        pDot11f->quiet_offset = 0;

        pDot11f->present = 1;
    }
} // End PopulateDot11fPropQuietBSS.

void
PopulateDot11fTrigStaBkScan(tpAniSirGlobal             pMac,
                            tANI_U32                        capsEnable,
                            tDot11fIETriggerStaBgScan *pDot11f)
{
    if ( PROP_CAPABILITY_GET( TITAN, capsEnable ) )
    {
        pDot11f->enable = pMac->lim.gLimTriggerBackgroundScanDuringQuietBss;
        pDot11f->present = 1;
    }
} // End PopulateDot11fTrigStaBkScan.

#if (WNI_POLARIS_FW_PACKAGE == ADVANCED)

tSirRetStatus
PopulateDot11fWDS(tpAniSirGlobal  pMac,
                  tANI_U32             capEnable,
                  tDot11fIEWDS   *pDot11f )
{
    tpPESession psessionEntry = &pMac->lim.gpSession[0];  //TBD-RAJESH HOW TO GET sessionEntry?????

    if ( psessionEntry->pLimStartBssReq->wdsInfo.wdsLength )
    {
        if ( psessionEntry->pLimStartBssReq->wdsInfo.wdsLength > 64)
        {
            limLog( pMac, LOG3, FL("WDS Length overflow in PopulateDot11fWDS.") );
            return eSIR_FAILURE;
        }

        pDot11f->num_wdsData = psessionEntry->pLimStartBssReq->wdsInfo.wdsLength;
        palCopyMemory( pMac->hHdd, pDot11f->wdsData,
                       psessionEntry->pLimStartBssReq->wdsInfo.wdsBytes,
                       pDot11f->num_wdsData );
        pDot11f->present = 1;
    }

    return eSIR_SUCCESS;
} // End PopulateDot11fWDS.

#endif // WNI_POLARIS_FW_PACKAGE == ADVANCED

#endif // WNI_POLARIS_FW_PRODUCT == AP

tSirRetStatus PopulateDot11fWsc(tpAniSirGlobal pMac,
                                tDot11fIEWscBeacon *pDot11f)
{

    tANI_U32 wpsState;

    pDot11f->Version.present = 1;
    pDot11f->Version.major = 0x01;
    pDot11f->Version.minor = 0x00;

    if (wlan_cfgGetInt(pMac, (tANI_U16) WNI_CFG_WPS_STATE, &wpsState) != eSIR_SUCCESS)
        limLog(pMac, LOGP,"Failed to cfg get id %d\n", WNI_CFG_WPS_STATE );

    pDot11f->WPSState.present = 1;
    pDot11f->WPSState.state = (tANI_U8) wpsState;

    pDot11f->APSetupLocked.present = 0;

    pDot11f->SelectedRegistrar.present = 0;

    pDot11f->DevicePasswordID.present = 0;

    pDot11f->SelectedRegistrarConfigMethods.present = 0;

    pDot11f->UUID_E.present = 0;

    pDot11f->RFBands.present = 0;

    pDot11f->present = 1;
    return eSIR_SUCCESS;
}

tSirRetStatus PopulateDot11fWscRegistrarInfo(tpAniSirGlobal pMac,
                                             tDot11fIEWscBeacon *pDot11f)
{
    const struct sLimWscIeInfo *const pWscIeInfo = &(pMac->lim.wscIeInfo);
    tANI_U32 devicepasswdId;


    pDot11f->APSetupLocked.present = 1;
    pDot11f->APSetupLocked.fLocked = pWscIeInfo->apSetupLocked;

    pDot11f->SelectedRegistrar.present = 1;
    pDot11f->SelectedRegistrar.selected = pWscIeInfo->selectedRegistrar;

    if (wlan_cfgGetInt(pMac, (tANI_U16) WNI_CFG_WPS_DEVICE_PASSWORD_ID, &devicepasswdId) != eSIR_SUCCESS)
        limLog(pMac, LOGP,"Failed to cfg get id %d\n", WNI_CFG_WPS_DEVICE_PASSWORD_ID );

    pDot11f->DevicePasswordID.present = 1;
    pDot11f->DevicePasswordID.id = (tANI_U16) devicepasswdId;

    pDot11f->SelectedRegistrarConfigMethods.present = 1;
    pDot11f->SelectedRegistrarConfigMethods.methods = pWscIeInfo->selectedRegistrarConfigMethods;

    // UUID_E and RF Bands are applicable only for dual band AP

    return eSIR_SUCCESS;
}

tSirRetStatus DePopulateDot11fWscRegistrarInfo(tpAniSirGlobal pMac,
                                               tDot11fIEWscBeacon *pDot11f)
{
    pDot11f->APSetupLocked.present = 0;
    pDot11f->SelectedRegistrar.present = 0;
    pDot11f->DevicePasswordID.present = 0;
    pDot11f->SelectedRegistrarConfigMethods.present = 0;

    return eSIR_SUCCESS;
}
#ifdef WLAN_SOFTAP_FEATURE
tSirRetStatus PopulateDot11fProbeResWPSIEs(tpAniSirGlobal pMac, tDot11fIEWscProbeRes *pDot11f, tpPESession psessionEntry)
{
 
   tSirWPSProbeRspIE *pSirWPSProbeRspIE;

   pSirWPSProbeRspIE = &psessionEntry->APWPSIEs.SirWPSProbeRspIE;
   
    
    if(pSirWPSProbeRspIE->FieldPresent & SIR_WPS_PROBRSP_VER_PRESENT)
    {
        pDot11f->present = 1;
        pDot11f->Version.present = 1;
        pDot11f->Version.major = (tANI_U8) ((pSirWPSProbeRspIE->Version & 0xF0)>>4);
        pDot11f->Version.minor = (tANI_U8) (pSirWPSProbeRspIE->Version & 0x0F);
    }
    else
    {
        pDot11f->present = 0;
        pDot11f->Version.present = 0;
    }

    if(pSirWPSProbeRspIE->FieldPresent & SIR_WPS_PROBRSP_STATE_PRESENT)
    {
        
        pDot11f->WPSState.present = 1;
        pDot11f->WPSState.state = pSirWPSProbeRspIE->wpsState;
    }
    else
        pDot11f->WPSState.present = 0;
        
    if(pSirWPSProbeRspIE->FieldPresent & SIR_WPS_PROBRSP_APSETUPLOCK_PRESENT)
    {
        pDot11f->APSetupLocked.present = 1;
        pDot11f->APSetupLocked.fLocked = pSirWPSProbeRspIE->APSetupLocked;
    }
    else
        pDot11f->APSetupLocked.present = 0;
        
    if(pSirWPSProbeRspIE->FieldPresent & SIR_WPS_PROBRSP_SELECTEDREGISTRA_PRESENT)
    {
        pDot11f->SelectedRegistrar.present = 1;
        pDot11f->SelectedRegistrar.selected = pSirWPSProbeRspIE->SelectedRegistra;
    }
    else
         pDot11f->SelectedRegistrar.present = 0;
    
    if(pSirWPSProbeRspIE->FieldPresent & SIR_WPS_PROBRSP_DEVICEPASSWORDID_PRESENT)
    {
        pDot11f->DevicePasswordID.present = 1;
        pDot11f->DevicePasswordID.id = pSirWPSProbeRspIE->DevicePasswordID;
    }
    else
        pDot11f->DevicePasswordID.present = 0;
        
    if(pSirWPSProbeRspIE->FieldPresent & SIR_WPS_PROBRSP_SELECTEDREGISTRACFGMETHOD_PRESENT)
    {
        pDot11f->SelectedRegistrarConfigMethods.present = 1;
        pDot11f->SelectedRegistrarConfigMethods.methods = pSirWPSProbeRspIE->SelectedRegistraCfgMethod;
    }
    else
        pDot11f->SelectedRegistrarConfigMethods.present = 0;
        
    if(pSirWPSProbeRspIE->FieldPresent & SIR_WPS_PROBRSP_RESPONSETYPE_PRESENT)
    {
        pDot11f->ResponseType.present = 1;
        pDot11f->ResponseType.resType = pSirWPSProbeRspIE->ResponseType;
    }
    else
        pDot11f->ResponseType.present = 0;
        
    if(pSirWPSProbeRspIE->FieldPresent & SIR_WPS_PROBRSP_UUIDE_PRESENT)
    {
        pDot11f->UUID_E.present = 1;
        palCopyMemory(pMac->hHdd, pDot11f->UUID_E.uuid, pSirWPSProbeRspIE->UUID_E, WNI_CFG_WPS_UUID_LEN);
    }
    else
        pDot11f->UUID_E.present = 0;
    
    if(pSirWPSProbeRspIE->FieldPresent & SIR_WPS_PROBRSP_MANUFACTURE_PRESENT)
    {
        pDot11f->Manufacturer.present = 1;
        pDot11f->Manufacturer.num_name = pSirWPSProbeRspIE->Manufacture.num_name;
        palCopyMemory(pMac->hHdd, pDot11f->Manufacturer.name, pSirWPSProbeRspIE->Manufacture.name, pSirWPSProbeRspIE->Manufacture.num_name);
    } 
    else
        pDot11f->Manufacturer.present = 0;
        
    if(pSirWPSProbeRspIE->FieldPresent & SIR_WPS_PROBRSP_MODELNUMBER_PRESENT)
    {
        pDot11f->ModelName.present = 1;
        pDot11f->ModelName.num_text = pSirWPSProbeRspIE->ModelName.num_text;
        palCopyMemory(pMac->hHdd, pDot11f->ModelName.text, pSirWPSProbeRspIE->ModelName.text, pDot11f->ModelName.num_text);
    }
    else
      pDot11f->ModelName.present = 0;
    
    if(pSirWPSProbeRspIE->FieldPresent & SIR_WPS_PROBRSP_MODELNUMBER_PRESENT)
    {
        pDot11f->ModelNumber.present = 1;
        pDot11f->ModelNumber.num_text = pSirWPSProbeRspIE->ModelNumber.num_text;
        palCopyMemory(pMac->hHdd, pDot11f->ModelNumber.text, pSirWPSProbeRspIE->ModelNumber.text, pDot11f->ModelNumber.num_text);
    }
    else
        pDot11f->ModelNumber.present = 0;
    
    if(pSirWPSProbeRspIE->FieldPresent & SIR_WPS_PROBRSP_SERIALNUMBER_PRESENT)
    {
        pDot11f->SerialNumber.present = 1;
        pDot11f->SerialNumber.num_text = pSirWPSProbeRspIE->SerialNumber.num_text;
        palCopyMemory(pMac->hHdd, pDot11f->SerialNumber.text, pSirWPSProbeRspIE->SerialNumber.text, pDot11f->SerialNumber.num_text);
    }
    else
        pDot11f->SerialNumber.present = 0;
    
    if(pSirWPSProbeRspIE->FieldPresent & SIR_WPS_PROBRSP_PRIMARYDEVICETYPE_PRESENT)
    {
        pDot11f->PrimaryDeviceType.present = 1;
        palCopyMemory(pMac->hHdd, pDot11f->PrimaryDeviceType.oui, pSirWPSProbeRspIE->PrimaryDeviceOUI, sizeof(pSirWPSProbeRspIE->PrimaryDeviceOUI)); 
        pDot11f->PrimaryDeviceType.primary_category = pSirWPSProbeRspIE->PrimaryDeviceCategory;
        pDot11f->PrimaryDeviceType.sub_category = pSirWPSProbeRspIE->DeviceSubCategory;
    }
    else
        pDot11f->PrimaryDeviceType.present = 0;
    
    if(pSirWPSProbeRspIE->FieldPresent & SIR_WPS_PROBRSP_DEVICENAME_PRESENT)
    {
        pDot11f->DeviceName.present = 1;
        pDot11f->DeviceName.num_text = pSirWPSProbeRspIE->DeviceName.num_text;
        palCopyMemory(pMac->hHdd, pDot11f->DeviceName.text, pSirWPSProbeRspIE->DeviceName.text, pDot11f->DeviceName.num_text);
    }
    else
        pDot11f->DeviceName.present = 0;
    
    if(pSirWPSProbeRspIE->FieldPresent & SIR_WPS_PROBRSP_CONFIGMETHODS_PRESENT)
    {
        pDot11f->ConfigMethods.present = 1;
        pDot11f->ConfigMethods.methods = pSirWPSProbeRspIE->ConfigMethod;
    }
    else
        pDot11f->ConfigMethods.present = 0;
    
    
    if(pSirWPSProbeRspIE->FieldPresent & SIR_WPS_PROBRSP_RF_BANDS_PRESENT)
    {
        pDot11f->RFBands.present = 1;
        pDot11f->RFBands.bands = pSirWPSProbeRspIE->RFBand;
    }
    else
       pDot11f->RFBands.present = 0;      
       
    return eSIR_SUCCESS;
}
tSirRetStatus PopulateDot11fAssocResWPSIEs(tpAniSirGlobal pMac, tDot11fIEWscAssocRes *pDot11f, tpPESession psessionEntry)
{
   tSirWPSProbeRspIE *pSirWPSProbeRspIE;

   pSirWPSProbeRspIE = &psessionEntry->APWPSIEs.SirWPSProbeRspIE;
   
    if(pSirWPSProbeRspIE->FieldPresent & SIR_WPS_PROBRSP_VER_PRESENT)
    {
        pDot11f->present = 1;
        pDot11f->Version.present = 1;
        pDot11f->Version.major = (tANI_U8) ((pSirWPSProbeRspIE->Version & 0xF0)>>4);
        pDot11f->Version.minor = (tANI_U8) (pSirWPSProbeRspIE->Version & 0x0F);
    }
    else
    {
        pDot11f->present = 0;
        pDot11f->Version.present = 0;
    }
    
    if(pSirWPSProbeRspIE->FieldPresent & SIR_WPS_PROBRSP_RESPONSETYPE_PRESENT)
    {
        pDot11f->ResponseType.present = 1;
        pDot11f->ResponseType.resType = pSirWPSProbeRspIE->ResponseType;
    }
    else
        pDot11f->ResponseType.present = 0;    

    return eSIR_SUCCESS;
}

tSirRetStatus PopulateDot11fBeaconWPSIEs(tpAniSirGlobal pMac, tDot11fIEWscBeacon *pDot11f, tpPESession psessionEntry)
{
 
   tSirWPSBeaconIE *pSirWPSBeaconIE;

   pSirWPSBeaconIE = &psessionEntry->APWPSIEs.SirWPSBeaconIE;
   
    
    if(pSirWPSBeaconIE->FieldPresent & SIR_WPS_PROBRSP_VER_PRESENT)
    {
        pDot11f->present = 1;
        pDot11f->Version.present = 1;
        pDot11f->Version.major = (tANI_U8) ((pSirWPSBeaconIE->Version & 0xF0)>>4);
        pDot11f->Version.minor = (tANI_U8) (pSirWPSBeaconIE->Version & 0x0F);
    }
    else
    {
        pDot11f->present = 0;
        pDot11f->Version.present = 0;
    }
    
    if(pSirWPSBeaconIE->FieldPresent & SIR_WPS_BEACON_STATE_PRESENT)
    {
        
        pDot11f->WPSState.present = 1;
        pDot11f->WPSState.state = pSirWPSBeaconIE->wpsState;
    }
    else
        pDot11f->WPSState.present = 0;
        
    if(pSirWPSBeaconIE->FieldPresent & SIR_WPS_BEACON_APSETUPLOCK_PRESENT)
    {
        pDot11f->APSetupLocked.present = 1;
        pDot11f->APSetupLocked.fLocked = pSirWPSBeaconIE->APSetupLocked;
    }
    else
        pDot11f->APSetupLocked.present = 0;
        
    if(pSirWPSBeaconIE->FieldPresent & SIR_WPS_BEACON_SELECTEDREGISTRA_PRESENT)
    {
        pDot11f->SelectedRegistrar.present = 1;
        pDot11f->SelectedRegistrar.selected = pSirWPSBeaconIE->SelectedRegistra;
    }
    else
         pDot11f->SelectedRegistrar.present = 0;
    
    if(pSirWPSBeaconIE->FieldPresent & SIR_WPS_BEACON_DEVICEPASSWORDID_PRESENT)
    {
        pDot11f->DevicePasswordID.present = 1;
        pDot11f->DevicePasswordID.id = pSirWPSBeaconIE->DevicePasswordID;
    }
    else
        pDot11f->DevicePasswordID.present = 0;
        
    if(pSirWPSBeaconIE->FieldPresent & SIR_WPS_BEACON_SELECTEDREGISTRACFGMETHOD_PRESENT)
    {
        pDot11f->SelectedRegistrarConfigMethods.present = 1;
        pDot11f->SelectedRegistrarConfigMethods.methods = pSirWPSBeaconIE->SelectedRegistraCfgMethod;
    }
    else
        pDot11f->SelectedRegistrarConfigMethods.present = 0;
        
    if(pSirWPSBeaconIE->FieldPresent & SIR_WPS_BEACON_UUIDE_PRESENT)
    {
        pDot11f->UUID_E.present = 1;
        palCopyMemory(pMac->hHdd, pDot11f->UUID_E.uuid, pSirWPSBeaconIE->UUID_E, WNI_CFG_WPS_UUID_LEN);
    }
    else
        pDot11f->UUID_E.present = 0;
    
       
    if(pSirWPSBeaconIE->FieldPresent & SIR_WPS_BEACON_RF_BANDS_PRESENT)
    {
        pDot11f->RFBands.present = 1;
        pDot11f->RFBands.bands = pSirWPSBeaconIE->RFBand;
    }
    else
       pDot11f->RFBands.present = 0;      
       
    return eSIR_SUCCESS;
}
#endif
tSirRetStatus PopulateDot11fWscInProbeRes(tpAniSirGlobal pMac,
                                          tDot11fIEWscProbeRes *pDot11f)
{
    tANI_U32 cfgMethods;
    tANI_U32 cfgStrLen;
    tANI_U32 val;
    tANI_U32 wpsVersion, wpsState;


    if (wlan_cfgGetInt(pMac, (tANI_U16) WNI_CFG_WPS_VERSION, &wpsVersion) != eSIR_SUCCESS)
        limLog(pMac, LOGP,"Failed to cfg get id %d\n", WNI_CFG_WPS_VERSION );

    pDot11f->Version.present = 1;
    pDot11f->Version.major = (tANI_U8) ((wpsVersion & 0xF0)>>4);
    pDot11f->Version.minor = (tANI_U8) (wpsVersion & 0x0F);

    if (wlan_cfgGetInt(pMac, (tANI_U16) WNI_CFG_WPS_STATE, &wpsState) != eSIR_SUCCESS)
        limLog(pMac, LOGP,"Failed to cfg get id %d\n", WNI_CFG_WPS_STATE );

    pDot11f->WPSState.present = 1;
    pDot11f->WPSState.state = (tANI_U8) wpsState;

    pDot11f->APSetupLocked.present = 0;

    pDot11f->SelectedRegistrar.present = 0;

    pDot11f->DevicePasswordID.present = 0;

    pDot11f->SelectedRegistrarConfigMethods.present = 0;

    pDot11f->ResponseType.present = 1;
    if ((pMac->lim.wscIeInfo.reqType == REQ_TYPE_REGISTRAR) ||
        (pMac->lim.wscIeInfo.reqType == REQ_TYPE_WLAN_MANAGER_REGISTRAR)){
        pDot11f->ResponseType.resType = RESP_TYPE_ENROLLEE_OPEN_8021X;
    }
    else{
         pDot11f->ResponseType.resType = RESP_TYPE_AP;
    }

    /* UUID is a 16 byte long binary. Still use wlan_cfgGetStr to get it. */
    pDot11f->UUID_E.present = 1;
    cfgStrLen = WNI_CFG_WPS_UUID_LEN;
    if (wlan_cfgGetStr(pMac,
                  WNI_CFG_WPS_UUID,
                  pDot11f->UUID_E.uuid,
                  &cfgStrLen) != eSIR_SUCCESS)
    {
        *(pDot11f->UUID_E.uuid) = '\0';
    }

    pDot11f->Manufacturer.present = 1;
    cfgStrLen = WNI_CFG_MANUFACTURER_NAME_LEN - 1;
    if (wlan_cfgGetStr(pMac,
                  WNI_CFG_MANUFACTURER_NAME,
                  pDot11f->Manufacturer.name,
                  &cfgStrLen) != eSIR_SUCCESS)
    {
        pDot11f->Manufacturer.num_name = 0;
        *(pDot11f->Manufacturer.name) = '\0';
    }
    else
    {
        pDot11f->Manufacturer.num_name = (tANI_U8) (cfgStrLen & 0x000000FF);
        pDot11f->Manufacturer.name[cfgStrLen] = '\0';
    }

    pDot11f->ModelName.present = 1;
    cfgStrLen = WNI_CFG_MODEL_NAME_LEN - 1;
    if (wlan_cfgGetStr(pMac,
                  WNI_CFG_MODEL_NAME,
                  pDot11f->ModelName.text,
                  &cfgStrLen) != eSIR_SUCCESS)
    {
        pDot11f->ModelName.num_text = 0;
        *(pDot11f->ModelName.text) = '\0';
    }
    else
    {
        pDot11f->ModelName.num_text = (tANI_U8) (cfgStrLen & 0x000000FF);
        pDot11f->ModelName.text[cfgStrLen] = '\0';
    }

    pDot11f->ModelNumber.present = 1;
    cfgStrLen = WNI_CFG_MODEL_NUMBER_LEN - 1;
    if (wlan_cfgGetStr(pMac,
                  WNI_CFG_MODEL_NUMBER,
                  pDot11f->ModelNumber.text,
                  &cfgStrLen) != eSIR_SUCCESS)
    {
        pDot11f->ModelNumber.num_text = 0;
        *(pDot11f->ModelNumber.text) = '\0';
    }
    else
    {
        pDot11f->ModelNumber.num_text = (tANI_U8) (cfgStrLen & 0x000000FF);
        pDot11f->ModelNumber.text[cfgStrLen] = '\0';
    }

    pDot11f->SerialNumber.present = 1;
    cfgStrLen = WNI_CFG_MANUFACTURER_PRODUCT_VERSION_LEN - 1;
    if (wlan_cfgGetStr(pMac,
                  WNI_CFG_MANUFACTURER_PRODUCT_VERSION,
                  pDot11f->SerialNumber.text,
                  &cfgStrLen) != eSIR_SUCCESS)
    {
        pDot11f->SerialNumber.num_text = 0;
        *(pDot11f->SerialNumber.text) = '\0';
    }
    else
    {
        pDot11f->SerialNumber.num_text = (tANI_U8) (cfgStrLen & 0x000000FF);
        pDot11f->SerialNumber.text[cfgStrLen] = '\0';
    }

    pDot11f->PrimaryDeviceType.present = 1;

    if (wlan_cfgGetInt(pMac, WNI_CFG_WPS_PRIMARY_DEVICE_CATEGORY, &val) != eSIR_SUCCESS)
    {
       limLog(pMac, LOGP, FL("cfg get prim device category failed\n"));
    }
    else
       pDot11f->PrimaryDeviceType.primary_category = (tANI_U16) val;

    if (wlan_cfgGetInt(pMac, WNI_CFG_WPS_PIMARY_DEVICE_OUI, &val) != eSIR_SUCCESS)
    {
       limLog(pMac, LOGP, FL("cfg get prim device OUI failed\n"));
    }
    else
    {
       *(pDot11f->PrimaryDeviceType.oui) = (tANI_U8)((val >> 24)& 0xff);
       *(pDot11f->PrimaryDeviceType.oui+1) = (tANI_U8)((val >> 16)& 0xff);
       *(pDot11f->PrimaryDeviceType.oui+2) = (tANI_U8)((val >> 8)& 0xff);
       *(pDot11f->PrimaryDeviceType.oui+3) = (tANI_U8)((val & 0xff));
    }

    if (wlan_cfgGetInt(pMac, WNI_CFG_WPS_DEVICE_SUB_CATEGORY, &val) != eSIR_SUCCESS)
    {
       limLog(pMac, LOGP, FL("cfg get prim device sub category failed\n"));
    }
    else
       pDot11f->PrimaryDeviceType.sub_category = (tANI_U16) val;

    pDot11f->DeviceName.present = 1;
    cfgStrLen = WNI_CFG_MANUFACTURER_PRODUCT_NAME_LEN - 1;
    if (wlan_cfgGetStr(pMac,
                  WNI_CFG_MANUFACTURER_PRODUCT_NAME,
                  pDot11f->DeviceName.text,
                  &cfgStrLen) != eSIR_SUCCESS)
    {
        pDot11f->DeviceName.num_text = 0;
        *(pDot11f->DeviceName.text) = '\0';
    }
    else
    {
        pDot11f->DeviceName.num_text = (tANI_U8) (cfgStrLen & 0x000000FF);
        pDot11f->DeviceName.text[cfgStrLen] = '\0';
    }

    if (wlan_cfgGetInt(pMac,
                  WNI_CFG_WPS_CFG_METHOD,
                  &cfgMethods) != eSIR_SUCCESS)
    {
        pDot11f->ConfigMethods.present = 0;
        pDot11f->ConfigMethods.methods = 0;
    }
    else
    {
        pDot11f->ConfigMethods.present = 1;
        pDot11f->ConfigMethods.methods = (tANI_U16) (cfgMethods & 0x0000FFFF);
    }

    pDot11f->RFBands.present = 0;

    pDot11f->present = 1;
    return eSIR_SUCCESS;
}

tSirRetStatus PopulateDot11fWscRegistrarInfoInProbeRes(tpAniSirGlobal pMac,
                                                       tDot11fIEWscProbeRes *pDot11f)
{
    const struct sLimWscIeInfo *const pWscIeInfo = &(pMac->lim.wscIeInfo);
    tANI_U32 devicepasswdId;

    pDot11f->APSetupLocked.present = 1;
    pDot11f->APSetupLocked.fLocked = pWscIeInfo->apSetupLocked;

    pDot11f->SelectedRegistrar.present = 1;
    pDot11f->SelectedRegistrar.selected = pWscIeInfo->selectedRegistrar;

    if (wlan_cfgGetInt(pMac, (tANI_U16) WNI_CFG_WPS_DEVICE_PASSWORD_ID, &devicepasswdId) != eSIR_SUCCESS)
       limLog(pMac, LOGP,"Failed to cfg get id %d\n", WNI_CFG_WPS_DEVICE_PASSWORD_ID );

    pDot11f->DevicePasswordID.present = 1;
    pDot11f->DevicePasswordID.id = (tANI_U16) devicepasswdId;

    pDot11f->SelectedRegistrarConfigMethods.present = 1;
    pDot11f->SelectedRegistrarConfigMethods.methods = pWscIeInfo->selectedRegistrarConfigMethods;

    // UUID_E and RF Bands are applicable only for dual band AP

    return eSIR_SUCCESS;
}

tSirRetStatus DePopulateDot11fWscRegistrarInfoInProbeRes(tpAniSirGlobal pMac,
                                                         tDot11fIEWscProbeRes *pDot11f)
{
    pDot11f->APSetupLocked.present = 0;
    pDot11f->SelectedRegistrar.present = 0;
    pDot11f->DevicePasswordID.present = 0;
    pDot11f->SelectedRegistrarConfigMethods.present = 0;

    return eSIR_SUCCESS;
}

tSirRetStatus PopulateDot11fWscInAssocRes(tpAniSirGlobal pMac,
                                          tDot11fIEWscAssocRes *pDot11f)
{
    tANI_U32  wpsVersion=0;

    if (wlan_cfgGetInt(pMac, (tANI_U16) WNI_CFG_WPS_VERSION, &wpsVersion) != eSIR_SUCCESS)
       limLog(pMac, LOGP,"Failed to cfg get id %d\n", WNI_CFG_WPS_VERSION );

    pDot11f->Version.present = 1;
    pDot11f->Version.major = (tANI_U8) ((wpsVersion & 0xF0)>>4);
    pDot11f->Version.minor = (tANI_U8) (wpsVersion & 0x0F);

    pDot11f->ResponseType.present = 1;
    if ((pMac->lim.wscIeInfo.reqType == REQ_TYPE_REGISTRAR) ||
        (pMac->lim.wscIeInfo.reqType == REQ_TYPE_WLAN_MANAGER_REGISTRAR))
    {
        pDot11f->ResponseType.resType = RESP_TYPE_ENROLLEE_OPEN_8021X;
    }
    else
    {
         pDot11f->ResponseType.resType = RESP_TYPE_AP;
    }
    return eSIR_SUCCESS;
}

tSirRetStatus PopulateDot11fWscAssocReq( tpAniSirGlobal        pMac,
                                         tDot11fIEWscAssocReq *pDot11f )
{
    tANI_U32           cfg;
    tSirRetStatus nSirStatus;

    // Read the WPS protocol version from cfg.  This  one-byte field is
    // broken into a four-bit major part using the top MSBs and four-bit
    // minor part using the LSBs. As an example, version 3.2 would be
    // 0x32.
    CFG_GET_INT( nSirStatus, pMac, WNI_CFG_WPS_VERSION, cfg );

    pDot11f->Version.minor   = ( tANI_U8 ) ( cfg & 0x000f );
    pDot11f->Version.major   = ( tANI_U8 ) ( ( cfg & 0x00f0 ) >> 4 );
    pDot11f->Version.present = 1;

    // The Response Type component specifies the operational mode of the
    // device for this setup exchange. The Response Type IE is carried
    // throughout the 802.1X data channel setup process:

    //    Response Type Value       Description
    //    0x00                      Enrollee, Info only
    //    0x01                      Enrollee, open 802.1X
    //    0x02                      Registrar
    //    0x03                      AP

    pDot11f->RequestType.reqType = 1;
    pDot11f->RequestType.present = 1;

    pDot11f->present = 1;

    return eSIR_SUCCESS;

} // End PopulateDot11fWscAssocReq.

tSirRetStatus PopulateDot11fWscProbeReq( tpAniSirGlobal        pMac,
                                         tDot11fIEWscProbeReq *pDot11f )
{
    tANI_U32      cfg, ncfg;
    tSirRetStatus nSirStatus;
    tANI_U8       uuid[ 16 ];

    // Read the WPS protocol version from cfg.  This  one-byte field is
    // broken into a four-bit major part using the top MSBs and four-bit
    // minor part using the LSBs. As an example, version 3.2 would be
    // 0x32.
    CFG_GET_INT( nSirStatus, pMac, WNI_CFG_WPS_VERSION, cfg );

    pDot11f->Version.minor   = ( tANI_U8 ) ( cfg & 0x000f );
    pDot11f->Version.major   = ( tANI_U8 ) ( ( cfg & 0x00f0 ) >> 4 );
    pDot11f->Version.present = 1;

    // The Response Type component specifies the operational mode of the
    // device for this setup exchange. The Response Type IE is carried
    // throughout the 802.1X data channel setup process:

    //    Response Type Value       Description
    //    0x00                      Enrollee, Info only
    //    0x01                      Enrollee, open 802.1X
    //    0x02                      Registrar
    //    0x03                      AP

    CFG_GET_INT( nSirStatus, pMac, WNI_CFG_WPS_REQUEST_TYPE, cfg );

    pDot11f->RequestType.reqType = ( tANI_U8 ) cfg;
    pDot11f->RequestType.present = 1;

    CFG_GET_INT( nSirStatus, pMac, WNI_CFG_WPS_CFG_METHOD, cfg );
    pDot11f->ConfigMethods.methods = ( tANI_U16 ) cfg;
    pDot11f->ConfigMethods.present = 1;

    CFG_GET_STR( nSirStatus, pMac, WNI_CFG_WPS_UUID, uuid, ncfg, 16 );
    palCopyMemory( pMac->hHdd, pDot11f->UUID_E.uuid, uuid, 16 );
    pDot11f->UUID_E.present = 1;

    CFG_GET_INT( nSirStatus, pMac, WNI_CFG_WPS_PRIMARY_DEVICE_CATEGORY, cfg );
    pDot11f->PrimaryDeviceType.primary_category = ( tANI_U16 ) cfg;

    CFG_GET_INT( nSirStatus, pMac, WNI_CFG_WPS_PIMARY_DEVICE_OUI, cfg );
    pDot11f->PrimaryDeviceType.oui[ 0 ] =
        ( tANI_U8 ) ( ( cfg & 0xff000000 ) >> 24 );
    pDot11f->PrimaryDeviceType.oui[ 1 ] =
        ( tANI_U8 ) ( ( cfg & 0x00ff0000 ) >> 16 );
    pDot11f->PrimaryDeviceType.oui[ 2 ] =
        ( tANI_U8 ) ( ( cfg & 0x0000ff00 ) >>  8 );
    pDot11f->PrimaryDeviceType.oui[ 3 ] =
        ( tANI_U8 ) ( cfg & 0x000000ff );

    CFG_GET_INT( nSirStatus, pMac, WNI_CFG_WPS_DEVICE_SUB_CATEGORY, cfg );
    pDot11f->PrimaryDeviceType.sub_category = ( tANI_U16 ) cfg;

    pDot11f->PrimaryDeviceType.present = 1;

    pDot11f->RFBands.present = 0;

    CFG_GET_INT( nSirStatus, pMac, WNI_CFG_WPS_ASSOCIATION_STATE, cfg );
    pDot11f->AssociationState.state   = ( tANI_U16 ) cfg;
    pDot11f->AssociationState.present = 1;

    CFG_GET_INT( nSirStatus, pMac, WNI_CFG_WPS_CONFIGURATION_ERROR, cfg );
    pDot11f->ConfigurationError.error   = ( tANI_U16 ) cfg;
    pDot11f->ConfigurationError.present = 1;

    CFG_GET_INT( nSirStatus, pMac, WNI_CFG_WPS_DEVICE_PASSWORD_ID, cfg );
    pDot11f->DevicePasswordID.id      = ( tANI_U16 ) cfg;
    pDot11f->DevicePasswordID.present = 1;

    pDot11f->present = 1;

    return eSIR_SUCCESS;

} // End PopulateDot11fWscProbeReq.

#if defined WLAN_FEATURE_VOWIFI

tSirRetStatus PopulateDot11fWFATPC( tpAniSirGlobal        pMac,
                                    tDot11fIEWFATPC *pDot11f, tANI_U8 txPower, tANI_U8 linkMargin )
{
     pDot11f->txPower = txPower;
     pDot11f->linkMargin = linkMargin;
     pDot11f->present = 1;

     return eSIR_SUCCESS;
}

tSirRetStatus PopulateDot11fBeaconReport( tpAniSirGlobal pMac, tDot11fIEMeasurementReport *pDot11f, tSirMacBeaconReport *pBeaconReport )
{

     pDot11f->report.Beacon.regClass = pBeaconReport->regClass;
     pDot11f->report.Beacon.channel = pBeaconReport->channel;
     palCopyMemory( pMac->hHdd, pDot11f->report.Beacon.meas_start_time, pBeaconReport->measStartTime, sizeof(pDot11f->report.Beacon.meas_start_time) );
     pDot11f->report.Beacon.meas_duration = pBeaconReport->measDuration;
     pDot11f->report.Beacon.condensed_PHY = pBeaconReport->phyType;
     pDot11f->report.Beacon.reported_frame_type = !pBeaconReport->bcnProbeRsp;
     pDot11f->report.Beacon.RCPI = pBeaconReport->rcpi;
     pDot11f->report.Beacon.RSNI = pBeaconReport->rsni;
     palCopyMemory( pMac->hHdd, pDot11f->report.Beacon.BSSID, pBeaconReport->bssid, sizeof(tSirMacAddr));
     pDot11f->report.Beacon.antenna_id = pBeaconReport->antennaId;
     pDot11f->report.Beacon.parent_TSF = pBeaconReport->parentTSF;

     if( pBeaconReport->numIes )
     {
          pDot11f->report.Beacon.BeaconReportFrmBody.present = 1;
          palCopyMemory( pMac->hHdd, pDot11f->report.Beacon.BeaconReportFrmBody.reportedFields, pBeaconReport->Ies, pBeaconReport->numIes );
          pDot11f->report.Beacon.BeaconReportFrmBody.num_reportedFields = pBeaconReport->numIes;
     }

     return eSIR_SUCCESS;

}

tSirRetStatus PopulateDot11fRRMIe( tpAniSirGlobal pMac, tDot11fIERRMEnabledCap *pDot11f, tpPESession    psessionEntry )
{  
   tpRRMCaps pRrmCaps;

   pRrmCaps = rrmGetCapabilities( pMac, psessionEntry );

   pDot11f->LinkMeasurement         = pRrmCaps->LinkMeasurement ;
   pDot11f->NeighborRpt             = pRrmCaps->NeighborRpt ;
   pDot11f->parallel                = pRrmCaps->parallel ;
   pDot11f->repeated                = pRrmCaps->repeated ;
   pDot11f->BeaconPassive           = pRrmCaps->BeaconPassive ;
   pDot11f->BeaconActive            = pRrmCaps->BeaconActive ;
   pDot11f->BeaconTable             = pRrmCaps->BeaconTable ;
   pDot11f->BeaconRepCond           = pRrmCaps->BeaconRepCond ;
   pDot11f->FrameMeasurement        = pRrmCaps->FrameMeasurement ;
   pDot11f->ChannelLoad             = pRrmCaps->ChannelLoad ;
   pDot11f->NoiseHistogram          = pRrmCaps->NoiseHistogram ;
   pDot11f->statistics              = pRrmCaps->statistics ;
   pDot11f->LCIMeasurement          = pRrmCaps->LCIMeasurement ;
   pDot11f->LCIAzimuth              = pRrmCaps->LCIAzimuth ;
   pDot11f->TCMCapability           = pRrmCaps->TCMCapability ;
   pDot11f->triggeredTCM            = pRrmCaps->triggeredTCM ;
   pDot11f->APChanReport            = pRrmCaps->APChanReport ;
   pDot11f->RRMMIBEnabled           = pRrmCaps->RRMMIBEnabled ;
   pDot11f->operatingChanMax        = pRrmCaps->operatingChanMax ;
   pDot11f->nonOperatinChanMax      = pRrmCaps->nonOperatingChanMax ;
   pDot11f->MeasurementPilot        = pRrmCaps->MeasurementPilot ;
   pDot11f->MeasurementPilotEnabled = pRrmCaps->MeasurementPilotEnabled ;
   pDot11f->NeighborTSFOffset       = pRrmCaps->NeighborTSFOffset ;
   pDot11f->RCPIMeasurement         = pRrmCaps->RCPIMeasurement ;
   pDot11f->RSNIMeasurement         = pRrmCaps->RSNIMeasurement ;
   pDot11f->BssAvgAccessDelay       = pRrmCaps->BssAvgAccessDelay ;
   pDot11f->BSSAvailAdmission       = pRrmCaps->BSSAvailAdmission ;
   pDot11f->AntennaInformation      = pRrmCaps->AntennaInformation ;

   pDot11f->present = 1;
   return eSIR_SUCCESS;
}
#endif

#if defined WLAN_FEATURE_VOWIFI_11R
void PopulateMDIE( tpAniSirGlobal        pMac,
                   tDot11fIEMobilityDomain *pDot11f, tANI_U8 mdie[SIR_MDIE_SIZE] )
{
   pDot11f->present = 1;
   pDot11f->MDID = (tANI_U16)((mdie[1] << 8) | (mdie[0]));

   // Plugfest fix
   pDot11f->overDSCap =   (mdie[2] & 0x01);
   pDot11f->resourceReqCap = ((mdie[2] >> 1) & 0x01);
   
}

void PopulateFTInfo( tpAniSirGlobal      pMac,
                     tDot11fIEFTInfo     *pDot11f )
{
   pDot11f->present = 1;
   pDot11f->IECount = 0; //TODO: put valid data during reassoc.
   //All other info is zero.

}
#endif

void PopulateDot11fAssocRspRates ( tpAniSirGlobal pMac, tDot11fIESuppRates *pSupp, 
      tDot11fIEExtSuppRates *pExt, tANI_U16 *_11bRates, tANI_U16 *_11aRates )
{
  tANI_U8 num_supp = 0, num_ext = 0;

  /* HAL_NUM_11B_RATES is defined in FW for Libra. Prima code can no longer
  access it.  Do we need equavilent? */
#ifdef FEATURE_WLAN_NON_INTEGRATED_SOC
  tANI_U8 i,j;
  for( i = 0 ; (i < HAL_NUM_11B_RATES && _11bRates[i]) ; i++, num_supp++ )
  {
      pSupp->rates[num_supp] = _11bRates[i];
  }  
  for( j = 0 ; (j < HAL_NUM_11A_RATES && _11aRates[j]) ; j++ )
  {
     if( num_supp < 8 )
         pSupp->rates[num_supp++] = _11aRates[j];
     else
         pExt->rates[num_ext++] =  _11aRates[j]; 
  }  
#endif
  if( num_supp )
  {
      pSupp->num_rates = num_supp;
      pSupp->present = 1;
  }
  if( num_ext )
  {
     pExt->num_rates = num_ext;
     pExt->present = 1;
  }
} 
// parserApi.c ends here.
