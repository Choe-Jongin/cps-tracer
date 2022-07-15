
/*****************   CPS  ****************/

#ifndef CPS_TRACER_HBLK_H_
#define CPS_TRACER_HBLK_H_

#include <linux/kernel.h>
#include <linux/string.h>

// for call stack
#define CPS_MAX_CALLSTACK 16
#define CPS_CALLER __builtin_frame_address(1)
#define CPS_CALLEE __builtin_frame_address(0)
extern int CPS_CALL_DEEP;
extern void *CPS_FUNC_ADDR[CPS_MAX_CALLSTACK];

// for print
extern int CPS_FUNC_COUNT;
extern char * current_filename;
extern char kstr[1024];

static inline void CPS_OPEN_FUNC(void *caller, void *callee)
{
	if( caller <= 0x01 )
	{
		CPS_FUNC_ADDR[0] = callee;
        CPS_CALL_DEEP = 0;
        return;
	}

    int i;
    for (i = CPS_CALL_DEEP; i >= 0; i--)
    {
        if (CPS_FUNC_ADDR[i] == caller)
        {
            CPS_CALL_DEEP = i;
            break;
        }
    }

    // fail to find caller(first call)
    if (i < 0)
    {
        CPS_FUNC_ADDR[0] = callee;
        CPS_CALL_DEEP = 0;
        return;
    }

    if (++CPS_CALL_DEEP < CPS_MAX_CALLSTACK)
    {
        CPS_FUNC_ADDR[CPS_CALL_DEEP] = callee;
    }
}

static inline void CPS_FUNCTION(void *caller, void *callee, const char * filename, const char * funcname, int argc, ...)
{
	char tab_str[CPS_MAX_CALLSTACK*2] = "";
	int filestart_i = strlen(filename);
	int cps_i;
	int tab_i;
	va_list ap;
	++CPS_FUNC_COUNT;

	//filename without path
	while(filestart_i > 0 && filename[--filestart_i] != '/')
		;

	//if diffent file, show file name
	if( current_filename != filename )
	{
		sprintf(kstr,"%s\033[0;31mCPS -\e[1m in %s\e[m\033[0m", KERN_ALERT, &(filename[filestart_i+1]));
		printk(kstr);
	}

	// check call stack
	if( caller > 0x1 && callee > 0x1)
		printk("%psCaller:%ps Callee:%ps\n", KERN_INFO, caller, callee);

	CPS_OPEN_FUNC(caller, callee);
	for (tab_i= 0; tab_i < CPS_CALL_DEEP*2; tab_i++)
	{
		if( tab_i%2 == 0 )
        	tab_str[tab_i] = '|';
		else if( tab_i%2 == 1 )
        	tab_str[tab_i] = ' ';
	}
	
	//_builtin_return_address(0);
	sprintf(kstr,"%s\033[0;31mCPS[%d] - %s\e[1m%s\e[m\033[0m", KERN_ALERT, CPS_FUNC_COUNT, tab_str, funcname);
	
	//print args
	va_start(ap, argc);
	for( cps_i = 0 ; cps_i < argc ; cps_i++ )
	{
		int currentlen = strlen(kstr);
		char * typestr  = va_arg(ap, char*);
		char * varname  = va_arg(ap, char*);

		//type and name
		sprintf(&(kstr[currentlen]), " [%s %s", (const char *)typestr, (const char *)varname);

		//value by type
		currentlen = strlen(kstr);
		if(		 strcmp(typestr,"char *")==0)		sprintf(&(kstr[currentlen]), ":%s]",va_arg(ap, char*));
		else if( strcmp(typestr,"const char *")==0)	sprintf(&(kstr[currentlen]), ":%s]", va_arg(ap, char*));
		else if( typestr[strlen(typestr)-1] == '*')	sprintf(&(kstr[currentlen]), ":%ps]", va_arg(ap, void*));
		else if( strcmp(typestr,"int")==0)			sprintf(&(kstr[currentlen]), ":%d]", va_arg(ap, int));
		else if( strcmp(typestr,"long")==0)			sprintf(&(kstr[currentlen]), ":%ld]", va_arg(ap, long));
		else if( strcmp(typestr,"float")==0)		sprintf(&(kstr[currentlen]), ":%f]", va_arg(ap, float));
		else if( strcmp(typestr,"double")==0)		sprintf(&(kstr[currentlen]), ":%f]", va_arg(ap, float));
		else if( strcmp(typestr,"char")==0)			sprintf(&(kstr[currentlen]), ":%c]", va_arg(ap, char));
		else										sprintf(&(kstr[currentlen]), ":%d]", va_arg(ap, int));
	}

	printk(kstr);
	va_end(ap);

	current_filename = (char *)filename;
}
static inline void CPS_MSG(const char *str)
{
	char kstr[256];
	
	sprintf(kstr,"%s\033[0;31mCPSMSG -\e[1m %s\e[m\033[0m",
		KERN_ALERT , str);
	printk(kstr);
}
#endif /* CPS_TRACER_HBLK_H_ */