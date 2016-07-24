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
#ifndef _DBG_MEM_CHECK_H
#define _DBG_MEM_CHECK_H

/* 
* ---------------------------------------------------------------------------------------- 
* for config
*  
* \# the config setting 
* ---------------------------------------------------------------------------------------- 
*/ 
#define MEM_CHECK_LOCALIZE(fn)      _dbg_ ##fn              /*localize name,should be changed*/
#define MEM_CHECK_MULTI_THREADS     1                       /*enable multi-thread*/
#define MEM_CHECK_SIZE_HASH         512                     /*hash table size,more bigger more faster,should be 16-1024*/
#define MEM_CHECK_SIZE_ALIGN        sizeof(unsigned long)   /*alloc align size ,should NOT change for normal using*/
#define MEM_CHECK_SIZE_MAX          (1024*1024*2)           /*max limit,2M default,depends on app*/
#define MEM_CHECK_SIZE_MIN          (0)                     /*0K,never mind!*/

#define MEM_CHECK_CPP_INIT          1                       /*enable c++ static init as default*/
#define MEM_CHECK_CPP_INFO          1                       /*enable alloc/free info*/
#define MEM_CHECK_CPP_TRACK         1                       /*enable alloc/free pos track*/

/* 
* ---------------------------------------------------------------------------------------- 
* for code
*  
* \# the code setting 
* ---------------------------------------------------------------------------------------- 
*/ 
#include  <stdio.h>
#include  <stdlib.h>
#include  <stddef.h>
#include  <inttypes.h>
#include  <unistd.h>
#include  <string.h>
#include  <sys/time.h>

/* 
* ---------------------------------------------------------------------------------------- 
* for config
*  
* \# the config setting 
* ---------------------------------------------------------------------------------------- 
*/  
#define MEM_CHECK_MAGIC_HEAD    0xD5D3853A
#define MEM_CHECK_MAGIC_TIAL    0xD5D3853B
#define MEM_CHECK_MAGIC_NONE    0x00000000
#define MEM_CHECK_MAGIC_RANGE   2
#define MEM_CHECK_MAGIC_SIZE    (MEM_CHECK_MAGIC_RANGE*sizeof(unsigned long))

/* 
* ---------------------------------------------------------------------------------------- 
* for c++ 
*  
* \# the real interface 
* ---------------------------------------------------------------------------------------- 
*/ 
#ifdef __cplusplus
extern "C++" {

    void* operator new(size_t size,const char * file,int lineno) throw();
    void* operator new[](size_t size,const char * file,int lineno) throw();
    void  operator delete(void* p) throw();
    void  operator delete[](void* p) throw();

} /* extern "C++"*/
#endif /*__cplusplus*/

/* 
* ---------------------------------------------------------------------------------------- 
* for c++ and c 
*  
* \# the real interface 
* ---------------------------------------------------------------------------------------- 
*/ 
#ifdef __cplusplus
extern "C"{
#endif 
    void MEM_CHECK_LOCALIZE(mc_init)(int max,int detailed);
    void MEM_CHECK_LOCALIZE(mc_exit)(int bFree);
    void MEM_CHECK_LOCALIZE(mc_dir)(int (*fn_puts)(const char *str));

    void*MEM_CHECK_LOCALIZE(mc_get)(size_t size,const char * file,int lineno,unsigned long  marker);
    void MEM_CHECK_LOCALIZE(mc_put)(void* p,const char * file,int lineno,unsigned long  marker);

    void *MEM_CHECK_LOCALIZE(mc_malloc) (size_t size,const char * file,int lineno);
    void *MEM_CHECK_LOCALIZE(mc_calloc) (size_t size,const char * file,int lineno);
    void *MEM_CHECK_LOCALIZE(mc_realloc)(void* p,size_t size,const char * file,int lineno);
    char *MEM_CHECK_LOCALIZE(mc_strdup) (const char* str,const char * file,int lineno);
    char *MEM_CHECK_LOCALIZE(mc_strndup)(const char* str,int n,const char * file,int lineno);
    void  MEM_CHECK_LOCALIZE(mc_free)   (void* p,const char * file,int lineno);

    /*memory check control*/    
    void MEM_CHECK_LOCALIZE(mc_trace)(const char * file,int lineno);
    void MEM_CHECK_LOCALIZE(mc_dump )(int bFree,int bData,const char * file,int lineno);
    void MEM_CHECK_LOCALIZE(mc_check)(void * ptr,const char * name,const char * file,int lineno);
    void MEM_CHECK_LOCALIZE(mc_info )(int show_nodes,const char * file,int lineno);

#ifdef __cplusplus
}  /* extern "C++"*/
#endif 
/* 
* ---------------------------------------------------------------------------------------- 
* for c++ and c  
*  
* the user interface 
* ---------------------------------------------------------------------------------------- 
*/
#if (defined(DBG_MEM_CHECK)  && !defined(DBG_MEM_CHECK_IMPL__))
/************************************************************************/
/*                  APIs redirection                                    */
/************************************************************************/
/*for c++ and c*/
#undef  malloc
#undef  calloc
#undef  realloc
#undef  strdup
#undef  strndup
#undef  free

#define malloc(size)            MEM_CHECK_LOCALIZE(mc_malloc) ((size),__FILE__,__LINE__)
#define calloc(nr,size)         MEM_CHECK_LOCALIZE(mc_calloc) ((nr)*(size),__FILE__,__LINE__)
#define realloc(p,size)         MEM_CHECK_LOCALIZE(mc_realloc)((p),(size),__FILE__,__LINE__)
#define strdup(str)             MEM_CHECK_LOCALIZE(mc_strdup )((str),__FILE__,__LINE__)
#define strndup(str,n)          MEM_CHECK_LOCALIZE(mc_strndup)((str),(n),__FILE__,__LINE__)
#define free(p)                 MEM_CHECK_LOCALIZE(mc_free)   ((p),__FILE__,__LINE__)

/*for c++*/
#if (defined(__cplusplus))
#define new     new(__FILE__,__LINE__)
#define delete  MEM_CHECK_LOCALIZE(mc_trace)(__FILE__,__LINE__),delete
#endif /*__cplusplus*/

#endif /*(defined(DBG_MEM_CHECK)  && !defined(DBG_MEM_CHECK_IMPL__))*/


/************************************************************************/
/*                  enable  memory check                                */
/************************************************************************/
#if (defined(DBG_MEM_CHECK))

/*for control*/
#define memcheck_init(max,detailed) MEM_CHECK_LOCALIZE(mc_init) (max,detailed)
#define memcheck_exit(bFree)        MEM_CHECK_LOCALIZE(mc_exit) (bFree)
#define memcheck_puts(fn)           MEM_CHECK_LOCALIZE(mc_dir)  (fn)
#define memcheck_trace(file,lineno) MEM_CHECK_LOCALIZE(mc_trace)(file,lineno)
#define memcheck_dump(bFree,bData)  MEM_CHECK_LOCALIZE(mc_dump) (bFree,bData,__FILE__,__LINE__)
#define memcheck_check(ptr)         MEM_CHECK_LOCALIZE(mc_check)(ptr,#ptr,__FILE__,__LINE__)
#define memcheck_info(show_all)     MEM_CHECK_LOCALIZE(mc_info) (show_all,__FILE__,__LINE__)

/************************************************************************/
/*                  disable memory check                                */
/************************************************************************/
#else 

/*for control*/
#define memcheck_init(max,detailed)
#define memcheck_exit(bFree)
#define memcheck_puts(fn)
#define memcheck_trace(file,lineno)
#define memcheck_dump(bFree,bData)
#define memcheck_check(ptr)
#define memcheck_info(show_all)

#endif /*(defined(DBG_MEM_CHECK))*/

/************************************************************************/
/*                                                                      */
/************************************************************************/
#endif /*_DBG_MEM_CHECK_H*/

