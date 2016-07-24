/*******************************************************************
*
*    DESCRIPTION:
*
*    AUTHOR:
*
*    HISTORY:
*
*    DATE:
*
*******************************************************************/
#include  <stdio.h>
#include  <stdlib.h>
#include  <stddef.h>
#include  <stdarg.h>
#include  <errno.h>
#include  <stdbool.h>
#include  <string.h>
#include  <unistd.h>
#include  <pthread.h>

#if defined(DBG_MEM_CHECK)

#define DBG_MEM_CHECK_IMPL__
#include "memcheck.h"

/************************************************************************/
/*                                                                      */
/************************************************************************/


/************************************************************************/
/*                                                                      */
/************************************************************************/
#define MC_DEBUG(fmt,args...)   \
    do {  \
    if(mc_detailed) \
    mc_printf("MC[%4d:0x%08x]+"fmt"\n"  ,getpid(),pthread_self(),##args);  \
    }while(0)

#define MC_INFO(fmt,args...)    \
    do {  \
    if(mc_detailed) \
    mc_printf("MC[%4d:0x%08x]+"fmt"\n"  ,getpid(),pthread_self(),##args);  \
    }while(0)

#define MC_ERR(fmt,args...)    \
    do {  \
    mc_printf("MC[%4d:0x%08x]+"fmt"\n"  ,getpid(),pthread_self(),##args);  \
    }while(0)

#define MC_ASSERT(exp) \
    do {  \
    if(mc_detailed && !(exp)){ \
    mc_printf("MC[%4d:0x%08x]+[%s+%d]("#exp")==Falure!\nAbort...\n" ,getpid(),pthread_self(), __FILE__,    __LINE__);    \
    }   \
    }while(0)

#define  MC_HASH(val)          (((val)))
#define  MC_MCI2NODE(mci)      (&mc_nodes[((unsigned long)(mci)->payload)%MEM_CHECK_SIZE_HASH])
#define  MC_PTR2NODE(ptr)      (&mc_nodes[(MC_HASH((unsigned long)(ptr)))%MEM_CHECK_SIZE_HASH])
#define  MC_IDX2NODE(idx)      (&mc_nodes[((unsigned long)(idx))%MEM_CHECK_SIZE_HASH])
#define  MC_PTR2MCI(ptr)       ((struct mc_info *)((unsigned long)(ptr)-(sizeof(struct mc_info))))

#define  MC_MCI2NODE_IDX(mci)  ((int)((unsigned long)(mci)->payload)%MEM_CHECK_SIZE_HASH)
#define  MC_MCI2NODE_CNT(mci)  ((int)(MC_MCI2NODE(mci)->nCount))
#define  MC_PTR2NODE_IDX(ptr)  ((int)((unsigned long)ptr)%MEM_CHECK_SIZE_HASH)
#define  MC_PTR2NODE_CNT(ptr)  ((int)(MC_PTR2NODE(ptr)->nCount))

#define  MC_MCI2HMAGIC(mci)    ((unsigned long*)((mci)->magic))
#define  MC_MCI2TMAGIC(mci)    ((unsigned long*)((mci)->payload+(MC_ALIGN(mci->size)>>2)))

#define  MC_MCI2HCHECK(mci)    magic_valid(MC_MCI2HMAGIC(mci),MEM_CHECK_MAGIC_HEAD)
#define  MC_MCI2TCHECK(mci)    magic_valid(MC_MCI2TMAGIC(mci),MEM_CHECK_MAGIC_TIAL)

#define  MC_ALIGN(siz)         ((((unsigned long)siz)+(MEM_CHECK_SIZE_ALIGN-1))&~(MEM_CHECK_SIZE_ALIGN-1))

#define  MC_TO_MB(siz)         (((siz)>>(10+10))&(1024-1))
#define  MC_TO_KB(siz)         (((siz)>>(10+0))&(1024-1))
#define  MC_TO_NB(siz)         (((siz)>>(0+0))&(1024-1))

/************************************************************************/
/*                                                                      */
/************************************************************************/
struct mc_info 
{
    unsigned long  size;
    const char *   file;
    unsigned long  lineno;
    struct mc_info*next;
    unsigned long  marker;
    unsigned long  magic[MEM_CHECK_MAGIC_RANGE]; 
    unsigned long  payload[0];  
};
/*local setting*/
struct mc_node 
{
#if MEM_CHECK_MULTI_THREADS
    pthread_mutex_t lock;
#endif
    struct mc_info *pHead;
    struct mc_info *pTial;
    unsigned long   nCount;
};
typedef int (*mc_puts)(const char *str);

/************************************************************************/
/*                                                                      */
/************************************************************************/
/*node info*/
static struct mc_node *mc_nodes   = NULL;
/*info output*/
static mc_puts         mc_puts_fn = NULL;
/*debug info*/
static int             mc_detailed= 0;
/*statistic*/
static unsigned long   mc_total_limits  = 0;
static unsigned long   mc_total_malloc  = 0;
static unsigned long   mc_total_calloc  = 0;
static unsigned long   mc_total_realloc = 0;
static unsigned long   mc_total_strdup  = 0;
static unsigned long   mc_total_strndup = 0;
static unsigned long   mc_total_free    = 0;
static unsigned long   mc_alloc_count   = 0;
static unsigned long   mc_alloc_peak    = 0;

/*extension for c++*/
unsigned long          mc_defcpp_new    = 0;
unsigned long          mc_defcpp_delete = 0;

/************************************************************************/
/*                                                                      */
/************************************************************************/
#if MEM_CHECK_MULTI_THREADS
/**/
#define INIT_LOCK(node) \
    do \
{ \
    pthread_mutexattr_t mutex_attr; \
    pthread_mutexattr_init(&mutex_attr);  \
    pthread_mutex_init(&(node)->lock,&mutex_attr);  \
    pthread_mutexattr_destroy(&mutex_attr); \
}while (0)

#define DESTROY_LOCK(node) \
    do \
{ \
    pthread_mutex_destroy(&(node)->lock); \
}while (0)

#define LOCK(node)  \
    do \
{ \
    pthread_mutex_lock(&(node)->lock); \
}while (0)

#define UNLOCK(node)  \
    do \
{ \
    pthread_mutex_unlock(&(node)->lock); \
}while (0)
#else
#define INIT_LOCK(node)
#define DESTROY_LOCK(node)
#define LOCK(node) 
#define UNLOCK(node) 
#endif  
/************************************************************************/
/*                                                                      */
/************************************************************************/
/*
*Function Name:static functions
*
*Parameters:
*
*Description:
*
*Returns:
*
*/
static int mc_printf(const char *fmt, ...)
{
    va_list args;
    char    buf[512];

    va_start(args, fmt);
    vsnprintf(buf,sizeof(buf), fmt, args);
    va_end(args);

    if(mc_puts_fn)
        mc_puts_fn(buf);
    else
    {
        fputs(buf,stderr);
        fflush(stderr);
    }

    return 0;
}
/************************************************************************/
/*                                                                      */
/************************************************************************/
/*
*Function Name:static functions
*
*Parameters:
*
*Description:
*
*Returns:
*
*/
static void magic_set(unsigned long *magic,unsigned long value)
{
    int i;
    for(i=0;i<MEM_CHECK_MAGIC_RANGE;i++)
        magic[i] = value;
}
static int  magic_check(unsigned long *magic,unsigned long value)
{
    int i;
    for(i=0;i<MEM_CHECK_MAGIC_RANGE;i++)
    {
        if(magic[i] != value)
            return -EFAULT;
    }
    return 0;
}
static const char* magic_valid(unsigned long *magic,unsigned long value)
{
    if(magic_check(magic,value)==0)
        return "GOOD";
    else
        return "BAD";
}
static void magic_dump(struct mc_info *mci)
{
    unsigned long *magic;
    int i;

    MC_INFO("[dump]H-Magic:\n");

    for(i=0,magic=MC_MCI2HMAGIC(mci);i<MEM_CHECK_MAGIC_RANGE;i++)
    {
        if (i!=0 && i%0x10==0) 
            mc_printf("\n%04x: ",i);
        mc_printf("%08x  ",(int)magic[i]);
    }
    mc_printf("\n");

    MC_INFO("[dump]T-Magic:\n");
    for(i=0,magic=MC_MCI2TMAGIC(mci);i<MEM_CHECK_MAGIC_RANGE;i++)
    {
        if (i!=0 && i%0x10==0) 
            mc_printf("\n%04x: ",i);
        mc_printf("%08x  ",(int)magic[i]);
    }
    mc_printf("\n");
}
/*
*Function Name:static functions
*
*Parameters:
*
*Description:
*
*Returns:
*
*/

static void add_item(struct mc_info *mci)
{
    struct mc_node * node=MC_MCI2NODE(mci);

    LOCK(node);

    if (!node->pHead) 
    {
        node->pHead=mci;
        node->pTial=mci;
        node->pTial->next=NULL;
    } 
    else 
    {
        node->pTial->next=mci;
        node->pTial      =mci;
        node->pTial->next=NULL;
    }

    node->nCount++;

    UNLOCK(node);
}
static struct mc_info * del_item(void *p) 
{
    struct mc_node * node=MC_PTR2NODE(p);
    struct mc_info * cur;
    struct mc_info * prev;

    LOCK(node);

    cur = prev =node->pHead;
    while (cur) 
    {
        if (cur->payload==p)
        {
            if (cur==node->pHead)
            {
                if (cur==node->pTial)
                    node->pHead=node->pTial=NULL;
                else
                    node->pHead=cur->next;
            }
            else if (cur==node->pTial)
            {
                node->pTial      =prev;
                node->pTial->next=NULL;
            }
            else
            {
                prev->next=cur->next;
            }

            node->nCount--;

            UNLOCK(node);

            cur->next = NULL;

            return cur;/*return*/
        }
        prev=cur;cur=cur->next;
    }

    UNLOCK(node);
    return NULL;/*return*/
}
static void dump_item(bool bfree,bool bData)
{
    size_t count=0;
    size_t size =0;
    size_t nr;

    mc_printf("Memory Trace Dump:\n");

    for(nr=0;nr<MEM_CHECK_SIZE_HASH;nr++)
    {
        struct mc_node * node=MC_IDX2NODE(nr);
        struct mc_info * tmp,*cur;

        size_t i=0;
        size_t k=0;

        LOCK(node);

        cur=node->pHead;

        while (cur)
        {
            if (bfree) 
            {
                node->nCount--;
            }
            mc_printf("MC: [Leak at][%-4d+%-4d][%s+%d][%d+%p],H-Magic=%s,T-Magic=%s"
                ,(int)nr,(int)node->nCount,cur->file,(int)cur->lineno,(int)cur->size,cur->payload
                ,MC_MCI2HCHECK(cur),MC_MCI2TCHECK(cur));
            /*count*/
            count++;
            size+=cur->size;
            /*dump data*/
            if(bData)
            {
                mc_printf(",Data=");
                k = cur->size>256?256:MC_ALIGN(cur->size)+4;
                for (i=0;i<k;++i)
                {
                    if (i%0x10==0) 
                        mc_printf("\n%04x: ",i);
                    mc_printf("%02x  ",*((unsigned char*)cur->payload+i));
                }
            }
            mc_printf("\n");

            tmp=cur;cur=cur->next;
            if (bfree) 
            {
                free(tmp);
            }
        }
        if(bfree)
        {
            node->pHead=node->pTial=NULL;
        }
        UNLOCK(node);
    }
    /*report*/
    mc_printf("Memory Trace Report:\n");
    mc_printf("Count:%-10d Points\nSize :%-10d Bytes\n",count,size);
}
/*
*Function Name:_dbg_mc_get(...) _dbg_mc_put(...) _dbg_mc_adjust(...)
*
*Parameters:
*
*Description:
*
*Returns:
*
*/
void * MEM_CHECK_LOCALIZE(mc_get)(size_t size,const char * file,int lineno,unsigned long  marker)
{ 
    if(mc_nodes)
    {
        struct mc_info *mci;

        if(size<=MEM_CHECK_SIZE_MIN || size>=MEM_CHECK_SIZE_MAX)
        {
            MC_ERR("[err] Where=[%s+%d], Invalid Size[%d-%d]=%d",file,lineno,MEM_CHECK_SIZE_MIN,MEM_CHECK_SIZE_MAX,size);
            MC_ERR("Abort");
            abort();
        }

        if(mc_alloc_count+size>=mc_total_limits)
        {
            MC_ERR("[err] Where=[%s+%d], Explosion Size=%d",file,lineno,size);
            MC_ERR("Abort");
            abort();
        }

        /*it not our option*/
        if ((file==NULL && lineno==0))
        {
            MC_ERR("[get][0000+0000] Where=[%s+%d], Call outof check!!",file,lineno);
            return malloc(size);
        }

        mci = (struct mc_info *)malloc(sizeof(struct mc_info)+MC_ALIGN(size)+MEM_CHECK_MAGIC_SIZE);
        if (!mci) 
        {
            MC_ERR("[get][0000+0000] Where=[%s+%d], Attr=[%d+NO Memory]",file,lineno,size);
            return NULL;
        }
        /*store it */
        mci->size  =size;
        mci->file  =file;
        mci->lineno=lineno;
        mci->next  =NULL;
        mci->marker=marker;

        /*setup magic*/
        magic_set(MC_MCI2HMAGIC(mci),MEM_CHECK_MAGIC_HEAD);
        magic_set(MC_MCI2TMAGIC(mci),MEM_CHECK_MAGIC_TIAL);
        /*add to list*/
        add_item(mci);

        /*update*/
        mc_alloc_count+=mci->size;
        if(mc_alloc_count>mc_alloc_peak)
            mc_alloc_peak = mc_alloc_count;

        MC_DEBUG("[get][%-4d+%-4d] Where=[%s+%d], Attr=[%d+%p]"
            ,MC_MCI2NODE_IDX(mci),MC_MCI2NODE_CNT(mci),mci->file,(int)mci->lineno,(int)mci->size,mci->payload);

        return mci->payload;
    }
    else
    {
        /*to avoid call before init*/
        MC_ERR("[get][0000+0000] Where=[%s+%d], Call before init!!",file,lineno);
        return malloc(size);
    }
}

/*put memory*/
void MEM_CHECK_LOCALIZE(mc_put)(void* p,const char * file,int lineno,unsigned long  marker)
{
    /*to avoid call before init*/
    if(mc_nodes)
    {
        struct mc_info *mci =del_item(p);

        if (mci)
        {
            if(mci->marker!=marker)
            {
                MC_ERR("[err][%-4d+%-4d] Marker Mismatch, Where=[%s+%d], At=[%s+%d], Attr=[%d+%p]"
                    ,MC_MCI2NODE_IDX(mci),MC_MCI2NODE_CNT(mci),file,lineno,mci->file,(int)mci->lineno,(int)mci->size,mci->payload);
                MC_ERR("Abort");
                abort();
            }
            if (!file && !lineno) 
            {
                /*call by operator delete*/
                MC_DEBUG("    }[%-4d+%-4d] At=[%s+%d], Attr=[%d+%p]"
                    ,MC_MCI2NODE_IDX(mci),MC_MCI2NODE_CNT(mci),mci->file,(int)mci->lineno,(int)mci->size,mci->payload);
            }
            else if(magic_check(MC_MCI2HMAGIC(mci),MEM_CHECK_MAGIC_HEAD)!=0 
                || magic_check(MC_MCI2TMAGIC(mci),MEM_CHECK_MAGIC_TIAL)!=0
                || MC_PTR2MCI(p)!=mci
                || mci->file  ==NULL
                || mci->lineno==0
                || mci->size  <=MEM_CHECK_SIZE_MIN 
                || mci->size  >=MEM_CHECK_SIZE_MAX )
            {
                MC_ERR("[err][%-4d+%-4d] Memory Corrupted, Where=[%s+%d], At=[%s+%d], Attr=[%d+%p]"
                    ,MC_MCI2NODE_IDX(mci),MC_MCI2NODE_CNT(mci),file,lineno,mci->file,(int)mci->lineno,(int)mci->size,mci->payload);
                MC_ERR("[err]H-Magic=%s,T-Magic=%s",MC_MCI2HCHECK(mci),MC_MCI2TCHECK(mci));
                magic_dump(mci);
                MC_ERR("Abort");
                abort();
            }
            else
            {
                MC_DEBUG("[put][%-4d+%-4d] Where=[%s+%d], At=[%s+%d], Attr=[%d+%p]"
                    ,MC_MCI2NODE_IDX(mci),MC_MCI2NODE_CNT(mci),file,lineno,mci->file,(int)mci->lineno,(int)mci->size,mci->payload);
            }
            /*update*/
            mc_alloc_count-=mci->size;

            /*clear magic*/
            magic_set(MC_MCI2HMAGIC(mci),MEM_CHECK_MAGIC_NONE);
            magic_set(MC_MCI2TMAGIC(mci),MEM_CHECK_MAGIC_NONE);
            /*free mci*/
            free(mci);
            /*done*/
            return ;
        }

        /*it not our option*/
        if ((file==NULL && lineno==0))
        {
            MC_ERR("[get][0000+0000] Where=[%s+%d], Call outof check!!",file,lineno);
            free(p);
        }
        else 
        {
            MC_ERR("[put][0000+0000] Where=[%s+%d],Invalid Ptr=%p",file,lineno,p); 
            MC_ERR("Abort()"); 
            abort();
        }
    }
    else
    {
        MC_ERR("[get][0000+0000] Where=[%s+%d], Call before init",file,lineno);
        free(p);
    }
}

void * MEM_CHECK_LOCALIZE(mc_adjust)(void* p,size_t size,const char * file,int lineno,unsigned long  marker)
{
    /*to avoid call before init*/
    if(mc_nodes)
    {
        struct mc_info * mci=del_item(p);

        if (!mci) 
        {
            MC_ERR("[adj] Where=[%s+%d],Invalid Ptr=%p",file,lineno,p); 
            return NULL;
        }
        /*update*/
        mc_alloc_count-=mci->size;

        if(mc_alloc_count+size>=mc_total_limits)
        {
            MC_ERR("[err] Where=[%s+%d], Explosion Size=%d",file,lineno,size);
            MC_ERR("Abort");
            abort();
        }

        /*clear magic*/
        magic_set(MC_MCI2HMAGIC(mci),MEM_CHECK_MAGIC_NONE);
        magic_set(MC_MCI2TMAGIC(mci),MEM_CHECK_MAGIC_NONE);

        mci=(struct mc_info *)realloc(mci,sizeof(struct mc_info)+MC_ALIGN(size)+MEM_CHECK_MAGIC_SIZE);

        if (!mci) 
        {
            MC_ERR("NO Memory!!!");
            return NULL;
        }

        MC_DEBUG("[adj][%-4d+%-4d] Where=[%s+%d], At=[%s+%d], NewAttr=[%d+%p]"
            ,MC_MCI2NODE_IDX(mci),MC_MCI2NODE_CNT(mci),file,lineno,mci->file,(int)mci->lineno,size,mci->payload);

        /*change the info*/
        mci->size  =size;
        mci->file  =file;
        mci->lineno=lineno;
        mci->next  =NULL;
        mci->marker=marker;

        /*setup magic*/
        magic_set(MC_MCI2HMAGIC(mci),MEM_CHECK_MAGIC_HEAD);
        magic_set(MC_MCI2TMAGIC(mci),MEM_CHECK_MAGIC_TIAL);

        add_item(mci);

        /*update*/
        mc_alloc_count+=mci->size;
        if(mc_alloc_count>mc_alloc_peak)
            mc_alloc_peak = mc_alloc_count;

        return mci->payload;
    }
    else
    {
        MC_ERR("[adj][0000+0000] Where=[%s+%d], Call before init",file,lineno);
        return realloc(p,size);
    }
}


/*======================================================================*/
/*
*----------------------------------------------------------------------
*   MEM_CHECK_LOCALIZE(mc_init (...)    MEM_CHECK_LOCALIZE(mc_exit (...) 
*   
*   func:     
*   in  :    
*   out :   
*----------------------------------------------------------------------
*/
void MEM_CHECK_LOCALIZE(mc_init)(int limit,int detailed)
{
    int i;

    /*avoid re-init*/
    if(!mc_nodes)
    {
        mc_nodes = (struct mc_node*)malloc(sizeof(struct mc_node)*MEM_CHECK_SIZE_HASH);

        if(mc_nodes)
        {
            for(i=0;i<MEM_CHECK_SIZE_HASH;i++)
            {
                /*init mutex lock for mc list*/
                INIT_LOCK(&mc_nodes[i]);
                mc_nodes[i].pHead = NULL;
                mc_nodes[i].pTial = NULL;
                mc_nodes[i].nCount= 0;
            }
            /*tell to start*/
            MC_ERR("Start...");
        }
        else
        {
            MC_ERR("Start failed...");
        }
    }

    mc_total_limits = limit; 
    mc_detailed     = detailed;
}

void MEM_CHECK_LOCALIZE(mc_exit)(int bFree)
{
    int i;

    /*tell to stop*/
    MC_ERR("Stop...");

    if(mc_nodes)
    {
        dump_item(bFree,true); 

        for(i=0;i<MEM_CHECK_SIZE_HASH;i++)
        {
            /*init mutex lock for mc list*/
            DESTROY_LOCK(&mc_nodes[i]);
            mc_nodes[i].pHead = NULL;
            mc_nodes[i].pTial = NULL;
        }

        free(mc_nodes);
        mc_nodes = NULL;
    }
}
void MEM_CHECK_LOCALIZE(mc_dir)(int (*fn_puts)(const char *str))
{
    mc_puts_fn = fn_puts;
}
/*
*----------------------------------------------------------------------
*   MEM_CHECK_LOCALIZE(mc_trace (...)    
*   
*   func:     
*   in  :    
*   out :   
*----------------------------------------------------------------------
*/
void MEM_CHECK_LOCALIZE(mc_trace)(const char * file,int lineno)
{
    MC_DEBUG("[put] Where=[%10s][%4d] {",file,lineno);
}
/*
*----------------------------------------------------------------------
*   MEM_CHECK_LOCALIZE(mc_dump (...)    
*   
*   func:     
*   in  :    
*   out :   
*----------------------------------------------------------------------
*/
void MEM_CHECK_LOCALIZE(mc_dump)(int bFree,int bData,const char * file,int lineno)
{
    dump_item(bFree,bData) ;
}
/*
*----------------------------------------------------------------------
*   MEM_CHECK_LOCALIZE(mc_check (...)   
*   
*   func:     
*   in  :    
*   out :   
*----------------------------------------------------------------------
*/
void MEM_CHECK_LOCALIZE(mc_check)(void*ptr,const char * name,const char * file,int lineno)
{
    if(ptr)
    {
        struct mc_info * mci =MC_PTR2MCI(ptr);
        struct mc_node * node=MC_PTR2NODE(ptr);
        if(mci)
        {
            LOCK(node);

            MC_INFO("[chk][%-4d+%-4d]] Where=[%s+%d],%s=%p"
                ,(int)MC_PTR2NODE_IDX(ptr),(int)node->nCount,mci->file,(int)mci->lineno,name,ptr); 

            if(    magic_check(MC_MCI2HMAGIC(mci),MEM_CHECK_MAGIC_HEAD)!=0 
                || magic_check(MC_MCI2TMAGIC(mci),MEM_CHECK_MAGIC_TIAL)!=0
                || mci->file  ==NULL
                || mci->lineno==0
                || mci->size  <=MEM_CHECK_SIZE_MIN 
                || mci->size  >=MEM_CHECK_SIZE_MAX )
            {
                MC_ERR("[err][%-4d+%-4d] Memory Corrupted, At=[%s+%d], Attr=[%d+%p]"
                    ,(int)MC_PTR2NODE_IDX(ptr),(int)node->nCount,mci->file,(int)mci->lineno,(int)mci->size,mci->payload);
                MC_ERR("[err]H-Magic=%s,T-Magic=%s",MC_MCI2HCHECK(mci),MC_MCI2TCHECK(mci));
                magic_dump(mci);
                MC_ERR("Abort");
                abort();
            }
            UNLOCK(node);
        }
        else 
        {
            MC_INFO("[chk][0000+0000] Where=[%s+%d],Invalid %s=%p",file,lineno,name,ptr); 
        }
    }
    else
    {
        size_t count=0;
        int nr;

        if(file)
            mc_printf("MC  Memory Trace Check: [%s+%d]\n",file,lineno);

        for(nr=0;nr<MEM_CHECK_SIZE_HASH;nr++)
        {
            struct mc_node * node=MC_IDX2NODE(nr);
            struct mc_info * mci;

            LOCK(node);

            count+=node->nCount;

            /*
            * check node
            */

            if(node->pTial!=NULL && node->pTial->next!=NULL)
            {
                MC_ERR("[err][%-4d+%-4d] Memory Corrupted + node->pTial==%p,node->pTial->next==%p\n"
                    ,(int)nr,(int)node->nCount,node->pTial,node->pTial?node->pTial->next:NULL);
                MC_ERR("Abort");
                abort();
            }

            if(node->nCount==0 && (node->pHead!=NULL || node->pTial!=NULL))
            {
                MC_ERR("[err][%-4d+%-4d] Memory Corrupted + node->pHead==%p,node->pTial==%p\n"
                    ,(int)nr,(int)node->nCount,node->pHead,node->pTial);
                MC_ERR("Abort");
                abort();
            }
            /*
            * check item
            */
            mci=node->pHead;

            while (mci)
            {
                if(    magic_check(MC_MCI2HMAGIC(mci),MEM_CHECK_MAGIC_HEAD)!=0 
                    || magic_check(MC_MCI2TMAGIC(mci),MEM_CHECK_MAGIC_TIAL)!=0
                    || mci->file  ==NULL
                    || mci->lineno==0
                    || mci->size  <=MEM_CHECK_SIZE_MIN 
                    || mci->size  >=MEM_CHECK_SIZE_MAX )
                {
                    MC_ERR("[err][%-4d+%-4d] Memory Corrupted, At=[%s+%d], Attr=[%d+%p]"
                        ,(int)nr,(int)node->nCount,mci->file,(int)mci->lineno,(int)mci->size,mci->payload);
                    MC_ERR("[err]H-Magic=%s,T-Magic=%s",MC_MCI2HCHECK(mci),MC_MCI2TCHECK(mci));
                    MC_ERR("Abort");
                    abort();
                }

                mci=mci->next;
            }

            UNLOCK(node);
        }

        if(file)
        {
            mc_printf("MC  Memory Trace Check:%-10d Points\n",count);
        }
    }
}
/*
*----------------------------------------------------------------------
*   MEM_CHECK_LOCALIZE(mc_info(...)
*   
*   func:     
*   in  :    
*   out :   
*----------------------------------------------------------------------
*/
void MEM_CHECK_LOCALIZE(mc_info)(int show_nodes,const char * file,int lineno)
{
    int left;
    int nr;

    mc_printf("MC-Info: [%s+%d]\n",file,lineno);
    mc_printf("%-16s:%d\n","MC-node_size",  MEM_CHECK_SIZE_HASH);
    mc_printf("%-16s:%d\n","MC-align_size", MEM_CHECK_SIZE_ALIGN);
    mc_printf("%-16s:%d\n","MC-alloc_max",  MEM_CHECK_SIZE_MAX);
    mc_printf("%-16s:%d\n","MC-magic_size", MEM_CHECK_MAGIC_SIZE);

    /*show stats*/
    mc_printf("MC-Statistics:\n");
    mc_printf("%-16s:%u\n","MC-total_malloc", mc_total_malloc);
    mc_printf("%-16s:%u\n","MC-total_calloc", mc_total_calloc);
    mc_printf("%-16s:%u\n","MC-total_realloc",mc_total_realloc);
    mc_printf("%-16s:%u\n","MC-total_strdup", mc_total_strdup);
    mc_printf("%-16s:%u\n","MC-total_strndup",mc_total_strndup);
    mc_printf("%-16s:%u\n","MC-total_free",   mc_total_free);

    mc_printf("%-16s:%u,%u Mb,%u Kb,%u bytes\n","MC-total_limits",
        mc_total_limits,MC_TO_MB(mc_total_limits),MC_TO_KB(mc_total_limits),MC_TO_NB(mc_total_limits));
    mc_printf("%-16s:%u,%u Mb,%u Kb,%u bytes\n","MC-alloc_count",
        mc_alloc_count,MC_TO_MB(mc_alloc_count),MC_TO_KB(mc_alloc_count),MC_TO_NB(mc_alloc_count));
    mc_printf("%-16s:%u,%u Mb,%u Kb,%u bytes\n","MC-alloc_peak",
        mc_alloc_peak,MC_TO_MB(mc_alloc_peak),MC_TO_KB(mc_alloc_peak),MC_TO_NB(mc_alloc_peak));
    mc_printf("%-16s:new=%u delete=%u)\n","MC-default-cpp",
        mc_defcpp_new,mc_defcpp_delete);

    left = mc_total_limits-mc_alloc_count;
    mc_printf("%-16s:%u,%u Mb,%u Kb,%u bytes\n","MC-alloc_left",
        left,MC_TO_MB(left),MC_TO_KB(left),MC_TO_NB(left));

    /*show each nodes*/
    if(show_nodes)
    {
        for(nr=0;nr<MEM_CHECK_SIZE_HASH;nr++)
        {
            struct mc_node * node=MC_IDX2NODE(nr);

            LOCK(node);

            mc_printf("MC-Nodes<%-4d>:C=%d\n",nr,node->nCount);

            UNLOCK(node);
        }
    }

    mc_printf("MC-Info:End\n");
}
/*
*----------------------------------------------------------------------
*   MEM_CHECK_LOCALIZE(mc_malloc(...)  MEM_CHECK_LOCALIZE(mc_calloc(...)  MEM_CHECK_LOCALIZE(mc_realloc(...)  MEM_CHECK_LOCALIZE(mc_free(...)  
*   
*   func:     
*   in  :    
*   out :   
*----------------------------------------------------------------------
*/
void *MEM_CHECK_LOCALIZE(mc_malloc)(size_t size,const char * file,int lineno)
{
    mc_total_malloc++;

    return MEM_CHECK_LOCALIZE(mc_get)(size,file,lineno,0);
}
void *MEM_CHECK_LOCALIZE(mc_calloc)(size_t size,const char * file,int lineno)
{
    mc_total_calloc++;

    void* p= MEM_CHECK_LOCALIZE(mc_get)(size,file,lineno,0);
    if (p)
        memset(p,0,size);
    return p;
}
void *MEM_CHECK_LOCALIZE(mc_realloc)(void* p,size_t size,const char * file,int lineno)
{
    mc_total_realloc++;

    if (!p)
        return  MEM_CHECK_LOCALIZE(mc_get)(size,file,lineno,0);
    return MEM_CHECK_LOCALIZE(mc_adjust)(p,size,file,lineno,0);
}
char *MEM_CHECK_LOCALIZE(mc_strdup)(const char* str,const char * file,int lineno)
{
    size_t len;
    char* newStr;

    mc_total_strdup++;

    if (str==NULL)
        return NULL;

    len    = strlen(str);
    newStr = (char*)MEM_CHECK_LOCALIZE(mc_get)(len+1,file,lineno,0);

    strcpy(newStr,str);

    return newStr;
}
char *MEM_CHECK_LOCALIZE(mc_strndup)(const char* str,int n,const char * file,int lineno)
{
    int   len;
    char* newStr;

    mc_total_strndup++;

    if (str==NULL || n<=0)
        return NULL;

    len    = strlen(str);
    newStr = (char*)MEM_CHECK_LOCALIZE(mc_get)(n+1,file,lineno,0);

    n=n<len?n:len;
    strncpy(newStr,str,n);
    /*cut it off*/
    *(newStr+n)='\0';

    return newStr;
}
void MEM_CHECK_LOCALIZE(mc_free)(void* p,const char * file,int lineno)
{
    mc_total_free++;

    MEM_CHECK_LOCALIZE(mc_put)(p,file,lineno,0);
}
/************************************************************************/
/*                                                                      */
/************************************************************************/

#endif /*DBG_MEM_CHECK*/

