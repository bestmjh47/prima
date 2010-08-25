/*
 * Airgo Networks, Inc proprietary. All rights reserved.
 *
 * Author:      Sandesh Goel
 * Date:        02/25/02
 * History:-
 * Date            Modified by    Modification Information
 * --------------------------------------------------------------------
 *
 */

#ifndef __POL_DEBUG_H__
#define __POL_DEBUG_H__

#define LOGOFF  0
#define LOGP    1
#define LOGE    2
#define LOGW    3
#define LOG1    4
#define LOG2    5
#define LOG3    6
#define LOG4    7

#ifdef ANI_DEBUG
#define PMM_LOG_LEVEL LOG4
#define SCH_LOG_LEVEL LOG4
#define ARQ_LOG_LEVEL LOG4
#define LIM_LOG_LEVEL LOG4
#define HAL_LOG_LEVEL LOG4
#define SYS_LOG_LEVEL LOG4
#define CFG_LOG_LEVEL LOG4
#define DPH_LOG_LEVEL LOG4
#else
#define PMM_LOG_LEVEL LOGW
#define SCH_LOG_LEVEL LOGW
#define ARQ_LOG_LEVEL LOGW
#define LIM_LOG_LEVEL LOGW
#define HAL_LOG_LEVEL LOGW
#define SYS_LOG_LEVEL LOGW
#define CFG_LOG_LEVEL LOGW
#define DPH_LOG_LEVEL LOGW
#endif



#ifdef PE_DEBUG_LOGE
#define PELOGE(p) { p }
#else
#define PELOGE(p) { }
#endif

#ifdef PE_DEBUG_LOGW
#define PELOGW(p) { p }
#else
#define PELOGW(p) { }
#endif

#ifdef PE_DEBUG_LOG1
#define PELOG1(p) { p }
#else
#define PELOG1(p) { }
#endif

#ifdef PE_DEBUG_LOG2
#define PELOG2(p) { p }
#else
#define PELOG2(p) { }
#endif

#ifdef PE_DEBUG_LOG3
#define PELOG3(p) { p }
#else
#define PELOG3(p) { }
#endif

#ifdef PE_DEBUG_LOG4
#define PELOG4(p) { p }
#else
#define PELOG4(p) { }
#endif



#define FL(x)    "%s: %d: "\
                 x, __FUNCTION__, __LINE__

#endif
