/**=========================================================================
  
  \file  pmmDebug.c
  
  \brief implementation for log Debug related APIs

  \author Sunit Bhatia
  
   Copyright 2008 (c) Qualcomm, Incorporated.  All Rights Reserved.
   
   Qualcomm Confidential and Proprietary.
  
  ========================================================================*/

#include "pmmDebug.h"

void pmmLog(tpAniSirGlobal pMac, tANI_U32 loglevel, const char *pString,...) 
{
#ifdef WLAN_DEBUG
    // Verify against current log level
    if ( loglevel > pMac->utils.gLogDbgLevel[LOG_INDEX_FOR_MODULE( SIR_PMM_MODULE_ID )] )
        return;
    else
    {
        va_list marker;

        va_start( marker, pString );     /* Initialize variable arguments. */

        logDebug(pMac, SIR_PMM_MODULE_ID, loglevel, pString, marker);
        
        va_end( marker );              /* Reset variable arguments.      */
    }
#endif
}
