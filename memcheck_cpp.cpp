/*******************************************************************
*
*    DESCRIPTION:
*
*    AUTHOR: 
*
*    HISTORY:
*
*    DATE:2011-1-12
*
*******************************************************************/
#include  <stdio.h>
#include  <stdlib.h>
#include  <stddef.h>
#include  <stdarg.h>
#include  <syslog.h>
#include  <inttypes.h>

#if defined(DBG_MEM_CHECK)

#define DBG_MEM_CHECK_IMPL__
#include  "memcheck.h"
/************************************************************************/
/*                                                                      */
/************************************************************************/
/*extension for c++,in memcheck.c*/
extern unsigned long mc_defcpp_new;
extern unsigned long mc_defcpp_delete;

/************************************************************************/
/*                                                                      */
/************************************************************************/

#ifdef __cplusplus
extern "C"{
#endif 

    void *MEM_CHECK_LOCALIZE(mc_get)(size_t size,const char * file,int lineno,unsigned long  marker);
    void  MEM_CHECK_LOCALIZE(mc_put)(void* p,const char * file,int lineno,unsigned long  marker);

#ifdef __cplusplus
}
#endif

/************************************************************************/
/*                                                                      */
/************************************************************************/

using namespace std;

/************************************************************************/
/*                                                                      */
/************************************************************************/
#if MEM_CHECK_CPP_INIT
class MCInit
{
public:
    MCInit()
    {
        memcheck_init(MEM_CHECK_SIZE_MAX,MEM_CHECK_CPP_INFO);
    }
    ~MCInit()
    {
        memcheck_exit(0);
    }
};
/*single instance*/
MCInit MEM_CHECK_LOCALIZE(static_mc_init);
#endif

/************************************************************************/
/*                                                                      */
/************************************************************************/
static const char *marker_delete_0 = "by delete";
static const char *marker_delete_1 = "by delete[]";

/************************************************************************/
/*  default operator new(...)  delete(...)                              */
/************************************************************************/
void* operator new(size_t size) throw()
{
    mc_defcpp_new++;
    return MEM_CHECK_LOCALIZE(mc_get)(size,NULL,0,(unsigned long)marker_delete_0);
}
void* operator new[](size_t size) throw()
{
    mc_defcpp_new++;
    return MEM_CHECK_LOCALIZE(mc_get)(size,NULL,0,(unsigned long)marker_delete_1);
}

void operator delete(void* p) throw()
{
    mc_defcpp_delete++;
    MEM_CHECK_LOCALIZE(mc_put)(p,marker_delete_0,0,(unsigned long)marker_delete_0);
}
void operator delete[](void* p) throw()
{
    mc_defcpp_delete++;
    MEM_CHECK_LOCALIZE(mc_put)(p,marker_delete_1,0,(unsigned long)marker_delete_1);
}

/************************************************************************/
/*                                                                      */
/************************************************************************/
#endif /*DBG_MEM_CHECK*/

