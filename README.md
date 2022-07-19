# cps-trace

커널 코드에 추적 코드를 자동으로 삽입/삭제 하여 커널 코드 추적 및 흐름 파악을 용의하게 함

기본적으론 lightnvm 모니터링에 초점을 맞추어 제작하였기 때문에 다른 모듈에서 사용시 제약 사항이 발생 할 수 있음

## 사용 준비

추적할 함수가 정의된 파일에 cps-tracer.h을 include해야 함

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

## 컴파일 및 추적코드 삽입 삭제
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

코드 삽입/삭제 후 커널 컴파일 수행

## 사용 방법

추적코드 삽입 후 커널컴파일이 완료되었으면 자동으로 사용 됨

**특정 파일 및 함수만 추적하는 Target mode**

원하는 파일에 추적코드 삽입 후 매번 재 컴파일 하는 번거로움을 줄이기 위해 도입 된 기능

추적 코드를 삽입하고 나면 디렉토리에 cps_target.txt 파일이 생성됨. 원하는 추적 수준밑에 파일과 해당 파일 내 함수 목록 작성
```
$ cat /usr/src/OpenChannelSSD/drivers/lightnvm/cps_target.txt 
LEVEL 0
pblk-core.c *
...
pblk-write.c *
pblk-recovery.c pblk_end_io_recov
LEVEL 1
LEVEL 2
pblk-recovery.c *
pblk-init.c *
```

LEVEL 0 추적하지 않음

LEVEL 1 추적은 하지만 실시간으로 화면에 표시하지 않음(KERN_INFO)

LEVEL 2 추적을 하고 결과를 실시간으로 화면에 출력(KERN_ALERT)

같은 파일명이 있으면 먼저(위 쪽에) 쓰여진 파일 우선

함수명에 *은 파일 내 모든 함수 추적을 의미함

LEVEL 2에 '파일명 *'이 있어도 LEVEL 0에 '파일명 특정함수'가 적혀 있으면 우선순위에 의해서 해당 함수만 추적 제외 가능

**Target mode 모드 사용**

타겟모드를 사용하기 시작 할 시점에  CPS_LEAD_TARGET_FILE("/usr/src/OpenChannelSSD/drivers/lightnvm/cps_target.txt") 함수 삽입

제대로 타겟 리스트 파일이 불러와 지 않을것을 대비한 예외처리 가능(파일 읽기 성공시 0반환)
```
if (CPS_LEAD_TARGET_FILE("/usr/src/OpenChannelSSD/drivers/lightnvm/cps_target.txt") != 0)
	if (CPS_LEAD_TARGET_FILE("drivers/lightnvm/cps_target.txt") != 0)
		if (CPS_LEAD_TARGET_FILE("cps_target.txt") != 0)
			if (CPS_LEAD_TARGET_FILE("/home/femu/cps_target.txt") != 0)
				CPS_MSG("fail target mode start");
```
static void *pblk_init(...) 상단에 해당 코드 삽입 후 사용 가능

pblk_module_init() 단계에선 파일 입출력이 정상적으로 시행되지 않아서 사용 불가능

타겟모드를 시작하는 CPS_LEAD_TARGET_FILE 함수를 불러오기 전까진 모든 추적코드가 화면에 즉시 출력됨


## 개선 예정 사항

* 콜스택 정상표시

	현재 tail call optimization에 의해서 콜스택이 정상적으로 indent되지 않음
	모듈만 분리해서 컴파일할때 -O2 옵션을 제거해야 함

* 출력 대상 다양화

	현재 printk를 이용하여 출력을 하고 있지만, 향후 LEVEL을 추가하여
	sysfs나 파일 출력등으로 출력 스트림을 다양화 해야 함
	특시 실시간 출력은 오버헤드가 크기 때문에 메모리에 쌓아두는 방식 등을 우선 고려

