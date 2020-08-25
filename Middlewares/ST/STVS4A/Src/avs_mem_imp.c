
#include "avs_private.h"

#define SIZE_ALIGN      4
#define SIGNATURE       ((uint32_t)0x53564120) /* "AVS " */
#define CHECK_MALLOC


#if defined(AVS_USE_DEBUG)
void avs_mem_assert(uint32_t condition,char_t *pCondition, char_t *pFile, int32_t line);
void avs_mem_check(avs_mem_pool *pHandle, uint32_t force);
#define AVS_MEM_ASSERT(a)  avs_mem_assert((a),#a,__FILE__,__LINE__)
/* Check if we are thread safe */

#define AVS_TRACE_MALLOC
#define CHECK_CORRUPTION(a,b) avs_mem_check((a),(b))
#define AVS_MEM_PROLOG(a)      avs_mem_call_check((a),1)
#define AVS_MEM_EPILOG(a)      avs_mem_call_check((a),0)


#else
#define AVS_MEM_ASSERT(...)  ((void)0)
#define AVS_MEM_PROLOG(...)  ((void)0)
#define AVS_MEM_EPILOG(...)  ((void)0)
#define CHECK_CORRUPTION(...) ((void)0)
#endif

#ifdef CHECK_MALLOC
#define AVS_MEM_CHECK_SIGNATURE(p)     AVS_MEM_ASSERT( (p)->m_signature == SIGNATURE )
#else
#define AVS_MEM_CHECK_SIGNATURE(p)
#endif /*CHECK_MALLOC*/


#define REASONABLE_SIZE(a) ((a)->m_iBaseSize)

typedef struct t_avs_core_mem_Info
{
  int32_t   sizeOccuped;
  int32_t   sizeFree;
  int32_t   AllocMax;
  int32_t   NumFrag;

} avs_core_mem_Info;


#define AVS_MEM_MALLOC(p,a) (p)->m_globalAlloc+=(a)
#define AVS_MEM_FREE(p,a)   (p)->m_globalAlloc-=(a)
#define AVS_MEM_OFFSET(a) ((uint32_t)(a))
#define AVS_MEM_CHK_SIZE(h,size) avs_mem_check_size((h),(size))





typedef struct _avs_mem_header
{
#ifdef CHECK_MALLOC
  uint32_t          m_signature;
#endif /* CHECK_MALLOC */
  struct _avs_mem_header     *m_pNext;
  uint32_t          m_dwFree;
  uint32_t            m_dwOccuped;
#ifdef AVS_TRACE_MALLOC
  char_t  *                                 m_pstring;
  uint32_t                                m_line;
#endif
} avs_mem_header;

void avs_mem_call_check(avs_mem_pool *pHandle,uint32_t flag);
void avs_mem_call_check(avs_mem_pool *pHandle,uint32_t flag)
{
  if(flag !=0)
  {
    AVS_MEM_ASSERT((pHandle->m_flags & 1U)==0);
    pHandle->m_flags  |= 1U;
  }
  else
  {
    AVS_MEM_ASSERT((pHandle->m_flags & 1U)!=0);
    pHandle->m_flags  &= ~1U;
  }
}
void avs_mem_check_size(avs_mem_pool *pHandle,int32_t size);
void avs_mem_check_size(avs_mem_pool *pHandle,int32_t size)
{
  AVS_MEM_ASSERT((size >= 0) && (size < pHandle->m_iBaseSize));
}


static avs_mem_header  *avs_mem_find_best_candidate(avs_mem_pool *pHandle, uint32_t  size);
static void          *avs_mem_get_malloc_pointer(avs_mem_header *pBlock);

static void          *avs_mem_get_malloc_pointer(avs_mem_header *pBlock)
{
  return (void *)(&pBlock[1]);
}
static avs_mem_header  *avs_mem_get_header_pointer(void  *pBlock);
static avs_mem_header  *avs_mem_get_header_pointer(void  *pBlock)
{
  return ((avs_mem_header*)pBlock) - 1;
}
static avs_mem_header    *avs_mem_get_prev_header(avs_mem_pool *pHandle, avs_mem_header *pblock);

static int32_t Align(int32_t size, int32_t radix);
static int32_t Align(int32_t size, int32_t radix)
{
  int32_t newsize = (size + radix - 1) / radix;
  newsize *= radix;
  return newsize;
}

/* ------------------------------------------------------------------------------------- */
/*! Initialize the memory pool
*
* \param *pBlock
* \param size
*
* \return memBool
*/
int32_t  avs_mem_init(avs_mem_pool *pHandle, void *pBlock, uint32_t  size)
{
  if(size < sizeof(avs_mem_header))
  {
    return 0;
  }

  memset(pHandle, 0, sizeof(*pHandle));
  pHandle->m_iBaseSize = size;
  pHandle->m_checkFreq = 0;
  pHandle->m_pBaseMalloc = (avs_mem_header*)pBlock;
  if(pBlock != 0)
  {
    memset(pBlock, 0, size);
    avs_mem_header *pFirst = ((avs_mem_header *)pHandle->m_pBaseMalloc);

    pFirst->m_pNext = 0;
    pFirst->m_dwFree = size - sizeof(avs_mem_header);
    pFirst->m_dwOccuped = 0;
  #ifdef CHECK_MALLOC
    pFirst->m_signature = SIGNATURE;
  #endif
  }
  return 1;

}

/* ------------------------------------------------------------------------------------- */
/*!
*
*
* \return void
*/
void avs_mem_term(avs_mem_pool *pHandle)
{
}



/* ------------------------------------------------------------------------------------- */
/*!
*
* \param size
*
* \return memavs_mem_header *
*/
static avs_mem_header * avs_mem_find_best_candidate(avs_mem_pool *pHandle, uint32_t  size)
{
  avs_mem_header *pFirst = ((avs_mem_header *)pHandle->m_pBaseMalloc);
  avs_mem_header *pBestOne = 0;
  avs_mem_header *pCur = pFirst;

  uint32_t  dwRequestedSize = size + sizeof(avs_mem_header);

  while(pCur)
  {
    if(pCur->m_dwFree >= dwRequestedSize )
    {

      if(pBestOne)
      {
        /* Always prefer the smaller block */
        if(pBestOne->m_dwFree > pCur->m_dwFree)
        {
          pBestOne = pCur;
        }

      }
      else
      {
        pBestOne = pCur;
      }

    }
    pCur = pCur->m_pNext;
  }



  return pBestOne;
}



/* ------------------------------------------------------------------------------------- */
/*!
*
* \param *pblock
*
* \return memavs_mem_header
*/
static avs_mem_header *avs_mem_get_prev_header(avs_mem_pool *pHandle, avs_mem_header *pblock)
{
  avs_mem_header *pFirst = ((avs_mem_header *)pHandle->m_pBaseMalloc);
  avs_mem_header *pCur = pFirst;
  /* Can't free first block */
  if(pblock == pFirst )
  {
    return 0;
  }
  while(pCur)
  {
    if(pCur->m_pNext == pblock)
    {
      break;
    }
    pCur = pCur->m_pNext;
  }
  return pCur;

}




/* ------------------------------------------------------------------------------------- */
/* pooled calloc
*/

void * avs_mem_calloc(avs_mem_pool *pHandle, uint32_t  size, uint32_t  elem)
{
  if(pHandle->m_iBaseSize ==0) 
  {
    return 0;
  }
  void *pMalloc = avs_mem_alloc(pHandle, size * elem);
  if(pMalloc == 0)
  {
    return 0;
  }
  memset(pMalloc, 0, size * elem);
  return pMalloc;
}






/* ------------------------------------------------------------------------------------- */
/* pooled alloc
*
*/
void *     avs_mem_alloc(avs_mem_pool *pHandle, uint32_t size)
{

  if(pHandle->m_iBaseSize ==0) 
  {
    return 0;
  }
  
  AVS_MEM_PROLOG(pHandle);
  uint8_t *pMallocPtr = 0;
  avs_mem_header *pBlockNext = 0;
  int32_t dwLastFree;

  CHECK_CORRUPTION(pHandle, FALSE);




  uint32_t  sizeAligned = Align(size, SIZE_ALIGN);

  avs_mem_header *pCandidate = avs_mem_find_best_candidate(pHandle, sizeAligned);
  if(!pCandidate)
  {
    AVS_MEM_EPILOG(pHandle);
    return 0;
  }

  dwLastFree   = pCandidate->m_dwFree;

  /* Compute the next entry at the end of the coppered block size */

  pMallocPtr = (  uint8_t *)avs_mem_get_malloc_pointer(pCandidate);
  pBlockNext = (avs_mem_header *)(uint32_t)(pMallocPtr + pCandidate->m_dwOccuped);
  memset(pBlockNext ,0,sizeof(avs_mem_header ));
#ifdef CHECK_MALLOC
  pBlockNext->m_signature = SIGNATURE;
#endif
  pHandle->m_nbFrags++;
  /*  link the candidate to the next block */

  pBlockNext->m_pNext = pCandidate->m_pNext;
  pCandidate->m_pNext   =   pBlockNext;
  /* We have heated all memory of the candidate */
  pCandidate->m_dwFree  =  0;
  /* The next block gets the block size */
  pBlockNext->m_dwOccuped = sizeAligned;
  /* The we recompute the free space */
  pBlockNext->m_dwFree =  dwLastFree - (sizeAligned + sizeof(avs_mem_header));

  AVS_MEM_CHECK_SIGNATURE(pBlockNext);
  AVS_MEM_CHK_SIZE(pHandle, pBlockNext->m_dwOccuped);
  AVS_MEM_CHK_SIZE(pHandle, pBlockNext->m_dwFree);
  AVS_MEM_MALLOC(pHandle, size);

  pMallocPtr = avs_mem_get_malloc_pointer(pBlockNext);
  AVS_MEM_EPILOG(pHandle);
  return pMallocPtr ;
}


/* ------------------------------------------------------------------------------------- */
/* pooled realloc
*
*/
void * avs_mem_realloc(avs_mem_pool *pHandle, void *pBlock, uint32_t  sizeMalloc)
{
  if(pHandle->m_iBaseSize ==0) 
  {
    return 0;
  }
  
  uint32_t  dwSizeBlock;
  uint32_t  sizeAligned ;
  avs_mem_header *pCandidate ;
  CHECK_CORRUPTION(pHandle, FALSE);


  if(pBlock == 0)
  {
    return avs_mem_alloc(pHandle, sizeMalloc);
  }

  sizeAligned = Align(sizeMalloc, SIZE_ALIGN);
  pCandidate = avs_mem_get_header_pointer(pBlock);

  dwSizeBlock = pCandidate->m_dwOccuped + pCandidate->m_dwFree ;

  if(dwSizeBlock > sizeAligned)
  {
    AVS_MEM_PROLOG(pHandle);
    /* Retrieve  the block   */
    AVS_MEM_FREE(pHandle, pCandidate->m_dwOccuped);

    pCandidate->m_dwOccuped = sizeAligned;
    pCandidate->m_dwFree = dwSizeBlock - sizeAligned;

    AVS_MEM_CHECK_SIGNATURE(pCandidate);
    AVS_MEM_CHK_SIZE(pHandle, pCandidate->m_dwOccuped);
    AVS_MEM_CHK_SIZE(pHandle, pCandidate->m_dwFree);
    AVS_MEM_MALLOC(pHandle, sizeMalloc);
    AVS_MEM_EPILOG(pHandle);
    return avs_mem_get_malloc_pointer(pCandidate);
  }
  else
  {
    /* Else free the block & realloc */
    /* We assume that the free is done first */
    void *pNewBlk = avs_mem_alloc(pHandle, sizeMalloc);
    if(pNewBlk )
    {
      AVS_MEM_CHECK_SIGNATURE(avs_mem_get_header_pointer(pNewBlk));
      AVS_MEM_CHK_SIZE(pHandle, avs_mem_get_header_pointer(pNewBlk)->m_dwOccuped);
      AVS_MEM_CHK_SIZE(pHandle, avs_mem_get_header_pointer(pNewBlk)->m_dwFree);
      memcpy(pNewBlk, avs_mem_get_malloc_pointer(pCandidate), pCandidate->m_dwOccuped);
    }
    avs_mem_free(pHandle, pBlock);
    return pNewBlk ;
  }



}



/* ------------------------------------------------------------------------------------- */
/* pooled free
*
*/
void avs_mem_free(avs_mem_pool *pHandle, void *pBlk)
{
  if(pHandle->m_iBaseSize ==0) 
  {
    return;
  }
  
  AVS_MEM_PROLOG(pHandle);
  avs_mem_header *pNext = avs_mem_get_header_pointer(pBlk);
  CHECK_CORRUPTION(pHandle, FALSE);
  AVS_MEM_ASSERT(pHandle->m_nbFrags);
  pHandle->m_nbFrags--;

  AVS_MEM_CHECK_SIGNATURE(pNext);
  AVS_MEM_FREE(pHandle, pNext->m_dwOccuped);
  avs_mem_header *pPrev = avs_mem_get_prev_header(pHandle, pNext);
  if(pPrev == 0)
  {
    AVS_MEM_EPILOG(pHandle);

    AVS_TRACE_ERROR("Free Corrupted");
    return;
  }
  /* Unlink block */
  pPrev->m_pNext = pNext->m_pNext;
  pPrev->m_dwFree += pNext->m_dwOccuped + pNext->m_dwFree + sizeof(avs_mem_header);
  AVS_MEM_CHECK_SIGNATURE(pPrev);
  AVS_MEM_CHK_SIZE(pHandle, pPrev->m_dwOccuped);
  AVS_MEM_CHK_SIZE(pHandle, pPrev->m_dwFree);
  AVS_MEM_EPILOG(pHandle);
}


uint32_t avs_mem_check_ptr(avs_mem_pool *pHandle, void *ptr)
{
  if(pHandle == 0)
  {
    return 0;
  }
  if(ptr == 0)
  {
    return 0;
  }
  if(AVS_MEM_OFFSET(ptr) < AVS_MEM_OFFSET(pHandle->m_pBaseMalloc))
  {
    return 0;
  }
  if(AVS_MEM_OFFSET(ptr) > AVS_MEM_OFFSET(pHandle->m_pBaseMalloc) + pHandle->m_iBaseSize)
  {
    return 0;
  }
#ifdef CHECK_MALLOC
  if(avs_mem_get_header_pointer(ptr)->m_signature != SIGNATURE)
  {
    return 0;
  }
#endif

  return 1;
}


void  avs_mem_get_info(avs_mem_pool *pHandle, avs_core_mem_Info * pInfo );
void  avs_mem_get_info(avs_mem_pool *pHandle, avs_core_mem_Info * pInfo )
{
  avs_mem_header *pCur = 0;
  pInfo->sizeOccuped = 0;
  pInfo->sizeFree = 0;
  pInfo->AllocMax = 0;
  pInfo->NumFrag = 0;
  avs_mem_header *pFirst = ((avs_mem_header *)pHandle->m_pBaseMalloc);

  pCur = pFirst;
  while(pCur)
  {
    pInfo->sizeOccuped += pCur->m_dwOccuped;
    pInfo->sizeFree += pCur->m_dwFree;
    if(pInfo->AllocMax < pCur->m_dwFree)
    {
      pInfo->AllocMax = pCur->m_dwFree;
    }

    pInfo->NumFrag++;
    pCur = pCur->m_pNext;
  }

  /* Don't forget the size of the header */
  pInfo->AllocMax =   pInfo->AllocMax  > sizeof(avs_mem_header) ? pInfo->AllocMax  - sizeof(avs_mem_header) : 0;

}

void  avs_mem_check(avs_mem_pool *pHandle, uint32_t force);
void  avs_mem_check(avs_mem_pool *pHandle, uint32_t force)
{

  pHandle->m_checkCount++;
  if(!force)
  {
    if(pHandle->m_checkFreq == 0)
    {
      return;
    }
    if(pHandle->m_checkCount % pHandle->m_checkFreq != 0)
    {
      return;
    }
  }

  avs_mem_header *pCur = 0;
  avs_mem_header *pFirst = ((avs_mem_header *)pHandle->m_pBaseMalloc);
  pCur = pFirst;
  int32_t nbFrag = 0;
  while((pCur != 0) && (nbFrag < pHandle->m_nbFrags + 30))
  {
    AVS_MEM_CHECK_SIGNATURE(pCur);
    AVS_MEM_CHK_SIZE(pHandle, pCur->m_dwOccuped);
    AVS_MEM_CHK_SIZE(pHandle, pCur->m_dwFree);
    if(pCur->m_pNext)
    {
      AVS_MEM_CHECK_SIGNATURE(pCur->m_pNext);
      AVS_MEM_CHK_SIZE(pHandle, pCur->m_pNext->m_dwOccuped);
      AVS_MEM_CHK_SIZE(pHandle, pCur->m_pNext->m_dwFree);
    }
    nbFrag++;
    pCur = pCur->m_pNext;
  }
  AVS_MEM_ASSERT(nbFrag <= pHandle->m_nbFrags + 1);

}






void avs_mem_check_curruption(avs_mem_pool *pHandle)
{
  avs_mem_check(pHandle, TRUE);
}


uint32_t  avs_mem_check_curruption_blk(avs_mem_pool *pHandle, void *pBlk, int32_t size)
{

  pHandle->m_checkCount++;
  if(pHandle->m_checkFreq == 0)
  {
    return 0;
  }
  if(pHandle->m_checkCount % pHandle->m_checkFreq != 0)
  {
    return 0;
  }
  uint32_t maxFrags = pHandle->m_nbFrags;
  avs_mem_header *pCur = 0;
  avs_mem_header *pFirst = ((avs_mem_header *)pHandle->m_pBaseMalloc);
  pCur = pFirst;
  int32_t nbFrag = 0;
  while((pCur!=0) && (nbFrag < pHandle->m_nbFrags + 30))
  {
    AVS_MEM_CHECK_SIGNATURE(pCur);
    AVS_MEM_CHK_SIZE(pHandle, pCur->m_dwOccuped);
    AVS_MEM_CHK_SIZE(pHandle, pCur->m_dwFree);
    if(pCur->m_pNext)
    {
      AVS_MEM_CHECK_SIGNATURE(pCur->m_pNext);
      AVS_MEM_CHK_SIZE(pHandle, pCur->m_pNext->m_dwOccuped);
      AVS_MEM_CHK_SIZE(pHandle, pCur->m_pNext->m_dwFree);
    }
    /* New check the limits */
    uint8_t *pCurEnd = (uint8_t*)(uint32_t)pCur->m_pNext;
    if(pCurEnd  == 0)
    {
      pCurEnd = (uint8_t *)pHandle->m_pBaseMalloc + pHandle->m_iBaseSize;
    }

    if((pBlk >= (void*)pCur) && (pBlk <  (void*)pCurEnd))
    {
      uint8_t *pEnd = (uint8_t *)pBlk + size ;

      if(pBlk <   (void*)&pCur[0])
      {
        /* The block will corrupt the header */
        AVS_MEM_ASSERT(0);

        return 1;
      }


      /* Ok the block is in this check */
      /* Compute end of block to concider */
      if((pCurEnd != 0) && (pEnd  > pCurEnd))
      {
        /* The block will corrupt the next chucnk */
        AVS_MEM_ASSERT(0);
        return 1;
      }
    }
    nbFrag++;
    pCur = pCur->m_pNext;
  }
  AVS_MEM_ASSERT(nbFrag <= maxFrags + 1);
  AVS_MEM_ASSERT(nbFrag != maxFrags);

  return 0;
}



void avs_mem_assert(uint32_t condition,char_t *pCondition, char_t *pFile, int32_t line);
void avs_mem_assert(uint32_t condition,char_t *pCondition, char_t *pFile, int32_t line)
{
  if(condition==0)
  {
    /* Don't use avs trace message  due to the mutex */
    printf("Mem curruption: condition %s : at %s:%ld\r\n", pCondition, pFile, line);
    AVS_Signal_Exeception(NULL, AVS_SIGNAL_EXCEPTION_MEM_CURRUPTION);
  }
}



#if defined(AVS_TRACE_MALLOC)  && defined(CHECK_MALLOC)
void  avs_mem_print_frags(avs_mem_pool *pHandle);
void  avs_mem_print_frags(avs_mem_pool *pHandle)
{

  avs_mem_header *pCur = 0;
  avs_mem_header *pFirst = ((avs_mem_header *)pHandle->m_pBaseMalloc);
  pCur = pFirst;
  int32_t nbFrag = 0;
  while((pCur!=0) && (nbFrag < pHandle->m_nbFrags + 30))
  {
    int32_t err = 1;
    if(pCur->m_signature !=  SIGNATURE)
    {
      err = 1;
    }
    if(((int32_t)pCur->m_dwOccuped >= 0) && ((int32_t)pCur->m_dwOccuped < pHandle-> m_iBaseSize))
    {
      err = 1;
    }
    if(((int32_t)pCur->m_dwFree >= 0) && ((int32_t)pCur->m_dwFree < pHandle-> m_iBaseSize))
    {
      err = 1;
    }

    printf("%c%04ld : %p  Ocp:%04lx Free:%04lx %s:%lu\r",
           err == 0 ? '*' : ' ',
           nbFrag,
           pCur,
           pCur->m_dwOccuped,
           pCur->m_dwFree,
           pCur->m_pstring == 0 ? "None" : pCur->m_pstring,
           pCur->m_line);
    nbFrag++;
    pCur = pCur->m_pNext;
  }
}


void avs_mem_free_trace(avs_mem_pool *pHandle, void *pBlock, char_t *pString, int32_t line);
void avs_mem_free_trace(avs_mem_pool *pHandle, void *pBlock, char_t *pString, int32_t line)
{
  avs_mem_free(pHandle, pBlock);

}

void * avs_mem_realloc_trace(avs_mem_pool *pHandle, void *pBlock, uint32_t  sizeMalloc, char_t *pString, int32_t line);
void * avs_mem_realloc_trace(avs_mem_pool *pHandle, void *pBlock, uint32_t  sizeMalloc, char_t *pString, int32_t line)
{
  void *pMalloc = avs_mem_realloc(pHandle, pBlock, sizeMalloc);
  if(pMalloc)
  {
    avs_mem_get_header_pointer(pMalloc)->m_pstring = pString;
    avs_mem_get_header_pointer(pMalloc)->m_line    = line;

  }
  return pMalloc;
}


void * avs_mem_alloc_trace(avs_mem_pool *pHandle, uint32_t  sizeMalloc, char_t *pString, int32_t line);
void * avs_mem_alloc_trace(avs_mem_pool *pHandle, uint32_t  sizeMalloc, char_t *pString, int32_t line)
{
  void *pMalloc = avs_mem_alloc(pHandle, sizeMalloc);
  if(pMalloc)
  {
    avs_mem_get_header_pointer(pMalloc)->m_pstring = pString;
    avs_mem_get_header_pointer(pMalloc)->m_line    = line;

  }
  return pMalloc;
}

#ifdef AVS_USE_LEAK_DETECTOR

#define MAX_SNAP        300
static uint32_t         gISnap = 0;
static avs_mem_header *gSnap[MAX_SNAP];

static avs_mem_header * avs_mem_leak_find(avs_mem_pool *pHandle, avs_mem_header *pFind)
{

  for(int32_t a = 0; a < gISnap ; a++)
  {
    if(gSnap[a] == pFind)  return gSnap[a];
  }
  return NULL;
}

void print_leak_frags(avs_mem_pool *pHandle,char *pTitle)
{
  AVS_TRACE_INFO("%s :  Current %d frags, last %d frags total : %d bytes", pTitle,pHandle->m_nbFrags, gISnap, pHandle->m_globalAlloc);
  avs_mem_header *pBlk =  ((avs_mem_header *)pHandle->m_pBaseMalloc)->m_pNext;
  int index = 0;
  while(pBlk)
  {

    avs_mem_header *pCur = avs_mem_leak_find(pHandle, pBlk);
    if(pCur == 0)
    {
      AVS_TRACE_INFO("%03d:New Block %p:%05d", index++, avs_mem_get_malloc_pointer(pBlk), pBlk->m_dwOccuped);
    }
    pBlk = pBlk->m_pNext;
  }
}




void avs_mem_leak_detector(avs_mem_pool *pHandle)
{
  if(pHandle->m_pBaseMalloc ==0)
  {
    return;
  }
  uint32_t index = 0;
  AVS_TRACE_INFO("Leak detector :  Current %d frags, last %d frags total : %d bytes", pHandle->m_nbFrags, gISnap, pHandle->m_globalAlloc);
  print_leak_frags(pHandle,"Leak detector");

  /* The first block is a dummy block */
  avs_mem_header * pBlk = ((avs_mem_header *)pHandle->m_pBaseMalloc)->m_pNext;
  gISnap  = 0;
  while(pBlk && gISnap < MAX_SNAP)
  {
    AVS_MEM_ASSERT(gISnap < MAX_SNAP);
    gSnap[gISnap] = pBlk;
    gISnap++;
    pBlk = pBlk->m_pNext;
  }

}
#endif


#endif

