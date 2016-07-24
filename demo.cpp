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
/*for C*/
#include <signal.h>
#include <fcntl.h>
#include <error.h>
#include <stdio.h>
#include <getopt.h>
#include <pthread.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <new>

/*for memcheck*/
#include "memcheck.h"

extern int demo_cc(void);
extern int demo_cpp(void);

/************************************************************************/
/*                     main                                             */
/************************************************************************/
int main(int argc, const char **argv)
{
    /*(((((MC-API)))))*/
    /*
    *--------------------------------------------- 
    *	call before init
    *---------------------------------------------
    */
#if 0
    {/*zero alloc*/
        char * p_alloc_before_init = (char*)malloc(100);
        /*do some other*/
    }
#endif

#if 0
    {/*zero new*/
        char * p_new_before_init = new char;
        /*do some other*/
    }
#endif

#if 0
    {/*zero free*/
        char * p_free_before_init = (char*)0xbadbad;
        free(p_free_before_init);
        /*do some other*/
    }
#endif

#if 0
    {/*zero delete*/
        char * p_delete_before_init = (char*)0xbadbad;
        delete p_delete_before_init;
        /*do some other*/
    }
#endif

    /*
    *--------------------------------------------- 
    * global init
    *---------------------------------------------
    */
#if !MEM_CHECK_CPP_INIT
    /*must init memory check module*/
    memcheck_init(
        2*1024*1024,/*total limit ,for 2M bytes*/
        1           /*1==show detailed,0==show error only*/
        );
#endif

    /*
    *--------------------------------------------- 
    * test for c
    *---------------------------------------------
    */
#if 0
    {/*simple usage*/
        char * p_not_initialised; 
        free(p_not_initialised);
    }
#endif

#if 0
    {/*simple usage*/
        char * p_null = NULL; 
        free(p_null);
    }
#endif

#if 0
    {/*double free*/
        char * p_something = (char*)malloc(512); 
        /*free once*/
        free(p_something);
        /*do some other*/
        /*.....*/
        /*free again*/
        free(p_something);
    }
#endif

#if 0
    {/*out of limit*/
        char * p_out_of_limit = (char*)malloc(3*1024*1024); 
        /*do some other*/
    }
#endif

#if 0
    {/*zero alloc*/
        char * p_zero_alloc = (char*)malloc(0); 
        /*do some other*/
    }
#endif

#if 0
    {/*over run,access out of range*/
        char * p_over_run = (char*)malloc(16); 
        /*write access out,and read access out can NOT be detected*/
        p_over_run[17] = 0x55;
        /*do some other*/
        /*...*/
        /*when you try to free,then it will check invalid access*/
        free(p_over_run);
    }
#endif

#if 0
    {
        /*
        *	over run,system allocation will align to sizeof(int)=4byte ,
        *	so,if you access out and align to sizeof(int),the mechanism will be failed!!
        *	there are some rules:
        *   1)memory allocation size MUST be align to 4byte,those,17,15,23...,are invalid!
        *	2)you must adjust you detected range to adative your application!(MEM_CHECK_MAGIC_RANGE)
        */
        char * p_over_run = (char*)malloc(17); 
        /*write access out,and read access out can NOT be detected*/
        p_over_run[18] = 0x55;
        /*do some other*/
        /*...*/
        /*!!!!!can NOT be dected!!!!!*/
        free(p_over_run);
    }
#endif

#if 0
    {/*down run,access back of range*/
        char * p_down_run = (char*)malloc(16); 
        char * p_ref;
        /*write access out,and read access out can NOT be detected*/
        p_ref = p_down_run;

        p_ref--;
        *p_ref = 0x55;
        /*do some other*/
        /*...*/
        /*when you try to free,then it will check invalid access*/
        free(p_down_run);
    }
#endif

#if 1
    {/*far end over run,can NOT be detected*/
        char * p_far_end = (char*)malloc(16); 
        /*write access out,mostly will crash*/
        p_far_end[524] = 0x55;
        /*do some other*/
        /*...*/
        /*mostly can NOT come here*/
        free(p_far_end);
    }
#endif
    /*
    *--------------------------------------------- 
    * test for c++
    *---------------------------------------------
    */
#if 0
    {/*double delete*/
        char * p_double = new char;
        /*delete once*/
        delete[] p_double;
        /*do some other*/
        /*...*/
        /*delete again*/
        delete[] p_double;
    }
#endif



    /*
    *--------------------------------------------- 
    * console
    *---------------------------------------------
    */
#if 0
    {/*mass alloc*/
        char * p_mass[1024];

        for(int i=0;i<256;i++)
        {
            p_mass[i] = (char*)malloc(64+i*1);
        }
        /*(((((MC-API)))))*/
        /*after alloc or system init done,then we dump all info*/
        memcheck_dump(
            0, /*0==just dump, but NOT try to free,1==try to free*/
            0  /*0==dump info only ,1==dump data too*/
            );
        /*(((((MC-API)))))*/
        /*
        * check for valid,p=NULL,means check all allocation,
        * it will show info if some error
        */
        memcheck_check(NULL);
        memcheck_check(p_mass[5]);
        /*(((((MC-API)))))*/
        /*
        *	show allocation info
        */
        memcheck_info(0); 
        memcheck_info(1);
    }
#endif


    /*(((((MC-API)))))*/
    /*cleanup and check for leaks,if we can come here!*/
    memcheck_exit(0);

    return 0;
}
/************************************************************************/
/*                                                                      */
/************************************************************************/
