# cps-trace

### 사용 목적

커널 코드에 존재하는 함수들을 실 시간으로 모니터링 하여 흐름 파악을 용의하게 함

### 사용 준비

cps-tracer.h을 include해야 함

lightnvm 모듈에선 pblk.h에 포함시켜 사용

**c파일 중 한 곳에서  extern 변수 초기화를 진행해야함**

lightnvm 모듈에선 pblk-init.c 상단에서 초기화 하여 사용
```
//CPS Tracer Init
int CPS_TARGET_ONLY     = 0;
int CPS_TGT_FILE_NUM    = 0;
CPS_tgt_file CPS_tgt_files[CPS_MAX_TGT_FILE];
void *CPS_FUNC_ADDR[CPS_MAX_CALLSTACK];
int CPS_CALL_DEEP       = 0;
int CPS_FUNC_COUNT      = 0;
char * current_filename = "";
char kstr[1024]         = "";
//CPS Tracer Init End
```



### 컴파일 및 실행
* 컴파일
```
gcc pblk-cps-tracer.c -o cps_tracer
```
* 실행

./실행파일 {field:value}

field = file, type

file = 특정 파일 or 조건에 맞는 파일(wild card지원)중 *.c형태의 파일

type = 삽입모드:in(insert), 삭제모드:rm(remove)

value 생략시 자동으로 file:파일명으로 간주 *단 rm만 입력한 경우 type:rm으로 간주

*.c 파일이 최소한 한개는 포함되어 있어야 함
예시
```
# 모든 파일에 추적코드 삽입
./cps_tracer *
# 모든 파일에서 추적코드 삭제(type 생략)
./cps_tracer * rm
# 모든 파일에서 추적코드 삭제
./cps_tracer * type:rm
# pblk로 시작하고 .c로 끝나는 모든 파일에 추적코드 삽입
./cps_tracer pblk*.c
# 특정 파일들에게만 삽입
./cps_tracer pblk-init.c pblk-read.c 
```
### 사용 방법
