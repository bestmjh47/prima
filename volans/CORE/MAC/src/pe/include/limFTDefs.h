#if defined WLAN_FEATURE_VOWIFI_11R
/**=========================================================================
  
   Macros and Function prototypes FT and 802.11R purposes 

   Copyright 2010 (c) Qualcomm, Incorporated.  All Rights Reserved.
   
   Qualcomm Confidential and Proprietary.
  
  ========================================================================*/

#ifndef __LIMFTDEFS_H__
#define __LIMFTDEFS_H__


#include <palTypes.h>

/*-------------------------------------------------------------------------- 
  Preprocessor definitions and constants
  ------------------------------------------------------------------------*/
#define SIR_MDIE_SIZE               3 // MD ID(2 bytes), Capability(1 byte)
#define MAX_TIDS                    8
#define MAX_FTIE_SIZE             256 // Max size limited to 256, on acct. of IW custom events


/*-------------------------------------------------------------------------- 
  Type declarations
  ------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------- 
  FT Pre Auth Req SME<->PE 
  ------------------------------------------------------------------------*/
typedef struct sSirFTPreAuthReq
{
   tANI_U16    messageType;      // eWNI_SME_FT_PRE_AUTH_REQ
   tANI_U16    length;
   tANI_U8     preAuthchannelNum;
   tSirMacAddr currbssId;        // BSSID currently associated to suspend the link
   tSirMacAddr preAuthbssId;     // BSSID to preauth to
   tANI_U16    ft_ies_length;
   tANI_U8     ft_ies[MAX_FTIE_SIZE];
   tpSirBssDescription  pbssDescription;
} tSirFTPreAuthReq, *tpSirFTPreAuthReq;

/*-------------------------------------------------------------------------
  FT Pre Auth Rsp PE<->SME 
  ------------------------------------------------------------------------*/
typedef struct sSirFTPreAuthRsp
{
   tANI_U16    messageType;      // eWNI_SME_FT_PRE_AUTH_RSP
   tANI_U16    length;
   tSirMacAddr preAuthbssId;     // BSSID to preauth to
   tANI_U8     status;
   tANI_U8     smeSessionId;
   tANI_U16    ft_ies_length;
   tANI_U8     ft_ies[MAX_FTIE_SIZE];
   tANI_U16    ric_ies_length;
   tANI_U8     ric_ies[MAX_FTIE_SIZE];
} tSirFTPreAuthRsp, *tpSirFTPreAuthRsp;

/*-------------------------------------------------------------------------
  Global FT Information 
  ------------------------------------------------------------------------*/
typedef struct sFTPEContext
{
    tpSirFTPreAuthReq pFTPreAuthReq;                      // Saved FT Pre Auth Req
    void              *psavedsessionEntry;                
    tANI_U8           ftPreAuthStatus;
    tANI_U16          saved_auth_rsp_length;
    tANI_U8           saved_auth_rsp[MAX_FTIE_SIZE];

    // Items created for the new FT, session
    void              *pftSessionEntry;                   // Saved session created for pre-auth 
    void              *pAddBssReq;                        // Save add bss req.
    void              *pAddStaReq;                        // Save add sta req.

} tftPEContext, *tpftPEContext; 


#endif /* __LIMFTDEFS_H__ */ 

#endif /* WLAN_FEATURE_VOWIFI_11R */
