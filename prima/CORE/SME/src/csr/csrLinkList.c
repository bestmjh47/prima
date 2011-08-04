/** ------------------------------------------------------------------------- * 
    ------------------------------------------------------------------------- *  

  
    \file csrLinkList.c
  
    Implementation for the Common link list interfaces.
  
    Copyright (C) 2006 Airgo Networks, Incorporated
  
 
   ========================================================================== */

#include "palApi.h"
#include "csrLinkList.h"
#include "vos_lock.h"
#include "vos_memory.h"
#include "vos_trace.h"

ANI_INLINE_FUNCTION void csrListInit(tListElem *pList)
{
    pList->last = pList->next = pList;
}


ANI_INLINE_FUNCTION void csrListRemoveEntry(tListElem *pEntry)
{
    tListElem *pLast;
    tListElem *pNext;
    
    pLast = pEntry->last;
    pNext = pEntry->next;
    pLast->next = pNext;
    pNext->last = pLast;
}


ANI_INLINE_FUNCTION tListElem * csrListRemoveHead(tListElem *pHead)
{
    tListElem *pEntry;
    tListElem *pNext;
    
    pEntry = pHead->next;
    pNext = pEntry->next;
    pHead->next = pNext;
    pNext->last = pHead;
    
    return (pEntry);
}



ANI_INLINE_FUNCTION tListElem * csrListRemoveTail(tListElem *pHead)
{
    tListElem *pEntry;
    tListElem *pLast;
    
    pEntry = pHead->last;
    pLast = pEntry->last;
    pHead->last = pLast;
    pLast->next = pHead;
    
    return (pEntry);
}


ANI_INLINE_FUNCTION void csrListInsertTail(tListElem *pHead, tListElem *pEntry)
{
    tListElem *pLast;
    
    pLast = pHead->last;
    pEntry->last = pLast;
    pEntry->next = pHead;
    pLast->next = pEntry;
    pHead->last = pEntry;
}


ANI_INLINE_FUNCTION void csrListInsertHead(tListElem *pHead, tListElem *pEntry)
{
    tListElem *pNext;
    
    pNext = pHead->next;
    pEntry->next = pNext;
    pEntry->last = pHead;
    pNext->last = pEntry;
    pHead->next = pEntry;
}


//Insert pNewEntry before pEntry
void csrListInsertEntry(tListElem *pEntry, tListElem *pNewEntry)
{
    tListElem *pLast;
    if( !pEntry) 
    {
        VOS_TRACE(VOS_MODULE_ID_SME, VOS_TRACE_LEVEL_FATAL,"%s: Error!! pEntry is Null", __FUNCTION__);
        return; 
    }
       
    pLast = pEntry->last;
    pLast->next = pNewEntry;
    pEntry->last = pNewEntry;
    pNewEntry->next = pEntry;
    pNewEntry->last = pLast;
}

tANI_U32 csrLLCount( tDblLinkList *pList ) 
{
    tANI_U32 c = 0; 
    

    if( !pList) 
    {
        VOS_TRACE(VOS_MODULE_ID_SME, VOS_TRACE_LEVEL_FATAL,"%s: Error!! pList is Null", __FUNCTION__);
        return c; 
    }

    if ( pList && ( LIST_FLAG_OPEN == pList->Flag ) ) 
    {
        c = pList->Count;
    }

    return( c ); 
}


void csrLLLock( tDblLinkList *pList ) 
{
    

    if( !pList) 
    {
        VOS_TRACE(VOS_MODULE_ID_SME, VOS_TRACE_LEVEL_FATAL,"%s: Error!! pList is Null", __FUNCTION__);
        return ; 
    }

    if ( LIST_FLAG_OPEN == pList->Flag )
    {
        vos_spin_lock_acquire(pList->Lock);
    }
}


void csrLLUnlock( tDblLinkList *pList )
{
    
    if( !pList) 
    {
        VOS_TRACE(VOS_MODULE_ID_SME, VOS_TRACE_LEVEL_FATAL,"%s: Error!! pList is Null", __FUNCTION__);
        return ; 
    }

    if ( LIST_FLAG_OPEN == pList->Flag ) 
    {
        vos_spin_lock_release(pList->Lock);
    }
}


tANI_BOOLEAN csrLLIsListEmpty( tDblLinkList *pList, tANI_BOOLEAN fInterlocked )
{
    tANI_BOOLEAN fEmpty = eANI_BOOLEAN_TRUE;

    
    if( !pList) 
    {
        VOS_TRACE(VOS_MODULE_ID_SME, VOS_TRACE_LEVEL_FATAL,"%s: Error!! pList is Null", __FUNCTION__);
        return fEmpty ; 
    }

    if ( LIST_FLAG_OPEN == pList->Flag ) 
    {
        if(fInterlocked)
        {
            csrLLLock(pList);
        }

        fEmpty = csrIsListEmpty( &pList->ListHead );
         
        if(fInterlocked)
        {
            csrLLUnlock(pList);
        }
    }
    return( fEmpty );
}



tANI_BOOLEAN csrLLFindEntry( tDblLinkList *pList, tListElem *pEntryToFind )
{
    tANI_BOOLEAN fFound = eANI_BOOLEAN_FALSE;
    tListElem *pEntry;

    
    if( !pList) 
    {
        VOS_TRACE(VOS_MODULE_ID_SME, VOS_TRACE_LEVEL_FATAL,"%s: Error!! pList is Null", __FUNCTION__);
        return fFound ; 
    }

    if ( LIST_FLAG_OPEN == pList->Flag ) 
    {
        pEntry = csrLLPeekHead( pList, LL_ACCESS_NOLOCK);

        // Have to make sure we don't loop back to the head of the list, which will
        // happen if the entry is NOT on the list...
    
        while( pEntry && ( pEntry != &pList->ListHead ) ) 
        {
            if ( pEntry == pEntryToFind ) 
            {
                fFound = eANI_BOOLEAN_TRUE;
                break;
            }
            pEntry = pEntry->next;
        }
        
    }
    return( fFound );
}


eHalStatus csrLLOpen( tHddHandle hHdd, tDblLinkList *pList )
{
    eHalStatus status = eHAL_STATUS_SUCCESS;

    
    if( !pList) 
    {
        VOS_TRACE(VOS_MODULE_ID_SME, VOS_TRACE_LEVEL_FATAL,"%s: Error!! pList is Null", __FUNCTION__);
        return eHAL_STATUS_FAILURE ; 
    }
    
    if ( LIST_FLAG_OPEN != pList->Flag ) 
    {
        pList->Count = 0;
        pList->Lock = vos_mem_malloc(sizeof(pList->Lock));
        if (pList->Lock == NULL)
        {
            VOS_TRACE(VOS_MODULE_ID_SME, VOS_TRACE_LEVEL_FATAL,"%s: Error!! Memory allocation for pList->Lock failed", __FUNCTION__);
            return eHAL_STATUS_RESOURCES;
        }

        status = vos_spin_lock_init(pList->Lock);

        if(HAL_STATUS_SUCCESS(status))
        {
            csrListInit( &pList->ListHead );
            pList->Flag = LIST_FLAG_OPEN;
            pList->hHdd = hHdd;
        }
    }
    return (status);
}

void csrLLClose( tDblLinkList *pList )
{
    if( !pList) 
    {
        VOS_TRACE(VOS_MODULE_ID_SME, VOS_TRACE_LEVEL_FATAL,"%s: Error!! pList is Null", __FUNCTION__);
        return ; 
    }

    if ( LIST_FLAG_OPEN == pList->Flag ) 
    {
        // Make sure the list is empty...
        csrLLPurge( pList, LL_ACCESS_LOCK );
        vos_mem_free( pList->Lock );
        pList->Flag = LIST_FLAG_CLOSE;
    }
}

void csrLLInsertTail( tDblLinkList *pList, tListElem *pEntry, tANI_BOOLEAN fInterlocked )
{    
    if( !pList) 
    {
        VOS_TRACE(VOS_MODULE_ID_SME, VOS_TRACE_LEVEL_FATAL,"%s: Error!! pList is Null", __FUNCTION__);
        return; 
    }

    if ( LIST_FLAG_OPEN == pList->Flag ) 
    {
        if(fInterlocked)
        {  
            csrLLLock(pList);
        }
        csrListInsertTail( &pList->ListHead, pEntry );
        pList->Count++;
        if(fInterlocked)
        {  
            csrLLUnlock(pList);
        }
    }
}



void csrLLInsertHead( tDblLinkList *pList, tListElem *pEntry, tANI_BOOLEAN fInterlocked )
{
    
    if( !pList) 
    {
        VOS_TRACE(VOS_MODULE_ID_SME, VOS_TRACE_LEVEL_FATAL,"%s: Error!! pList is Null", __FUNCTION__);
        return; 
    }

    if ( LIST_FLAG_OPEN == pList->Flag ) 
    {
        if(fInterlocked)
        {
            csrLLLock(pList);
        }
        csrListInsertHead( &pList->ListHead, pEntry );
        pList->Count++;
        if(fInterlocked)
        {
            csrLLUnlock(pList);
        }
    }
}


void csrLLInsertEntry( tDblLinkList *pList, tListElem *pEntry, tListElem *pNewEntry, tANI_BOOLEAN fInterlocked )
{    
    if( !pList) 
    {
        VOS_TRACE(VOS_MODULE_ID_SME, VOS_TRACE_LEVEL_FATAL,"%s: Error!! pList is Null", __FUNCTION__);
        return ; 
    }

    if ( LIST_FLAG_OPEN == pList->Flag ) 
    {
        if(fInterlocked)
        {
            csrLLLock(pList);
        }
        csrListInsertEntry( pEntry, pNewEntry );
        pList->Count++;
        if(fInterlocked)
        {
            csrLLUnlock(pList);
        }
    }
}



tListElem *csrLLRemoveTail( tDblLinkList *pList, tANI_BOOLEAN fInterlocked )
{
    tListElem *pEntry = NULL;

    if( !pList) 
    {
        VOS_TRACE(VOS_MODULE_ID_SME, VOS_TRACE_LEVEL_FATAL,"%s: Error!! pList is Null", __FUNCTION__);
        return pEntry ; 
    }

    if ( LIST_FLAG_OPEN == pList->Flag ) 
    {
        if ( fInterlocked ) 
        {
            csrLLLock( pList );
        }

        if ( !csrIsListEmpty(&pList->ListHead) ) 
        {

            pEntry = csrListRemoveTail( &pList->ListHead );
            pList->Count--;					
        }
        if ( fInterlocked ) 
        {
            csrLLUnlock( pList );
        }
    }

    return( pEntry );
}


tListElem *csrLLPeekTail( tDblLinkList *pList, tANI_BOOLEAN fInterlocked )
{
    tListElem *pEntry = NULL;

    
    if( !pList) 
    {
        VOS_TRACE(VOS_MODULE_ID_SME, VOS_TRACE_LEVEL_FATAL,"%s: Error!! pList is Null", __FUNCTION__);
        return pEntry ; 
    }

    if ( LIST_FLAG_OPEN == pList->Flag ) 
    {
        if ( fInterlocked ) 
        {  
            csrLLLock( pList );
        }

        if ( !csrIsListEmpty(&pList->ListHead) ) 
        {
            pEntry = pList->ListHead.last; 
        }
        if ( fInterlocked ) 
        {
            csrLLUnlock( pList );
        }
    }

    return( pEntry );
}



tListElem *csrLLRemoveHead( tDblLinkList *pList, tANI_BOOLEAN fInterlocked )
{
    tListElem *pEntry = NULL;
    

    if( !pList) 
    {
        VOS_TRACE(VOS_MODULE_ID_SME, VOS_TRACE_LEVEL_FATAL,"%s: Error!! pList is Null", __FUNCTION__);
        return pEntry ; 
    }

    if ( LIST_FLAG_OPEN == pList->Flag ) 
    {
        if ( fInterlocked ) 
        {  
            csrLLLock( pList );
        }

        if ( !csrIsListEmpty(&pList->ListHead) ) 
        {
            pEntry = csrListRemoveHead( &pList->ListHead );
            pList->Count--;
        }

        if ( fInterlocked ) 
        {
            csrLLUnlock( pList );
        }
    }

    return( pEntry );
}


tListElem *csrLLPeekHead( tDblLinkList *pList, tANI_BOOLEAN fInterlocked )
{
    tListElem *pEntry = NULL;

    if( !pList) 
    {
        VOS_TRACE(VOS_MODULE_ID_SME, VOS_TRACE_LEVEL_FATAL,"%s: Error!! pList is Null", __FUNCTION__);
        return pEntry ; 
    }
     
    if ( LIST_FLAG_OPEN == pList->Flag ) 
    {
        if ( fInterlocked ) 
        {  
            csrLLLock( pList );
        }

        if ( !csrIsListEmpty(&pList->ListHead) ) 
        {
            pEntry = pList->ListHead.next; 
        }
        if ( fInterlocked ) 
        {
            csrLLUnlock( pList );
        }
    }

    return( pEntry );
}



void csrLLPurge( tDblLinkList *pList, tANI_BOOLEAN fInterlocked )
{
    tListElem *pEntry;

    if( !pList) 
    {
        VOS_TRACE(VOS_MODULE_ID_SME, VOS_TRACE_LEVEL_FATAL,"%s: Error!! pList is Null", __FUNCTION__);
        return ; 
    }

    if ( LIST_FLAG_OPEN == pList->Flag ) 
    {
        if ( fInterlocked ) 
        {  
            csrLLLock( pList );
        }
        while( (pEntry = csrLLRemoveHead( pList, LL_ACCESS_NOLOCK )) ) 
        {
            // just remove everything from the list until 
            // nothing left on the list.
        }
        if ( fInterlocked ) 
        {  
            csrLLUnlock( pList );
        }
    }
}


tANI_BOOLEAN csrLLRemoveEntry( tDblLinkList *pList, tListElem *pEntryToRemove, tANI_BOOLEAN fInterlocked )
{
    tANI_BOOLEAN fFound = eANI_BOOLEAN_FALSE;
    tListElem *pEntry;

    if( !pList) 
    {
        VOS_TRACE(VOS_MODULE_ID_SME, VOS_TRACE_LEVEL_FATAL,"%s: Error!! pList is Null", __FUNCTION__);
        return fFound; 
    }

    if ( LIST_FLAG_OPEN == pList->Flag ) 
    {
        if ( fInterlocked ) 
        {
            csrLLLock( pList );
        }

        pEntry = csrLLPeekHead( pList, LL_ACCESS_NOLOCK );

        // Have to make sure we don't loop back to the head of the list, which will
        // happen if the entry is NOT on the list...
        while( pEntry && ( pEntry != &pList->ListHead ) ) 
        {
            if ( pEntry == pEntryToRemove )
            {
                csrListRemoveEntry( pEntry );
                pList->Count--;

                fFound = eANI_BOOLEAN_TRUE;
                break;
            }

            pEntry = pEntry->next; 
        }
        if ( fInterlocked ) 
        {
            csrLLUnlock( pList );
        }
    }

    return( fFound );
}



tListElem *csrLLNext( tDblLinkList *pList, tListElem *pEntry, tANI_BOOLEAN fInterlocked )
{
    tListElem *pNextEntry = NULL;

    if( !pList) 
    {
        VOS_TRACE(VOS_MODULE_ID_SME, VOS_TRACE_LEVEL_FATAL,"%s: Error!! pList is Null", __FUNCTION__);
        return pNextEntry ; 
    }

    if ( LIST_FLAG_OPEN == pList->Flag ) 
    {
        if ( fInterlocked ) 
        {
            csrLLLock( pList );
        }

        if ( !csrIsListEmpty(&pList->ListHead) && csrLLFindEntry( pList, pEntry ) ) 
        {
            pNextEntry = pEntry->next;
            //Make sure we don't walk past the head
            if ( pNextEntry == &pList->ListHead ) 
            {
                pNextEntry = NULL;
            }
        }

        if ( fInterlocked ) 
        {
            csrLLUnlock( pList );
        }
    }

    return( pNextEntry );
}


tListElem *csrLLPrevious( tDblLinkList *pList, tListElem *pEntry, tANI_BOOLEAN fInterlocked )
{
    tListElem *pNextEntry = NULL;

    if( !pList) 
    {
        VOS_TRACE(VOS_MODULE_ID_SME, VOS_TRACE_LEVEL_FATAL,"%s: Error!! pList is Null", __FUNCTION__);
        return pNextEntry ; 
    }

    if ( LIST_FLAG_OPEN == pList->Flag ) 
    {
        if ( fInterlocked ) 
        {
            csrLLLock( pList );
        }

        if ( !csrIsListEmpty(&pList->ListHead) && csrLLFindEntry( pList, pEntry ) ) 
        {
            pNextEntry = pEntry->last; 
            //Make sure we don't walk past the head
            if ( pNextEntry == &pList->ListHead ) 
            {
                pNextEntry = NULL;
            }
        }  

        if ( fInterlocked ) 
        {
            csrLLUnlock( pList );
        }
    }

    return( pNextEntry );
}




