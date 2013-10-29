/*
 * This file limDebug.h contains log function called by LIM module.
 *
 * Author:      Chandra Modumudi
 * Date:        02/11/02
 * History:-
 * Date         Modified by    Modification Information
 * --------------------------------------------------------------------
 * 
 */

#ifndef __LIM_DEBUG_H__
#define __LIM_DEBUG_H__

#include "utilsApi.h"
#include "sirDebug.h"


void limLog(tpAniSirGlobal pMac, tANI_U32 loglevel, const char *pString,...) ;

/* define this to show more message in the LIM during TDLS development */
#define LIM_DEBUG_TDLS

#ifdef LIM_DEBUG_TDLS
#define LIM_LOG_TDLS(x0) x0
#else
#define LIM_LOG_TDLS(x0) 
#endif

#endif
