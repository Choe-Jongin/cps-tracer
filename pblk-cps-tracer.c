#include "pblk-cps-tracer.h"

int CPS_TARGET_ONLY     = 0;
int CPS_TGT_FILE_NUM    = 0;
CPS_tgt_file CPS_tgt_files[CPS_MAX_TGT_FILE];
void *CPS_FUNC_ADDR[CPS_MAX_CALLSTACK];
char *CPS_FUNC_NAME[CPS_MAX_CALLSTACK];

int CPS_CALL_DEEP       = 0;
int CPS_FUNC_COUNT      = 0;
char * current_filename = "";
char kstr[1024]         = "";