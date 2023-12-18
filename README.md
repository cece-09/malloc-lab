# [SWJungle] Week 05 malloc lab
SW사관학교 정글 5주차 과제 동적할당기 구현입니다. 아래는 과제 제출 시 Pull Request 전문입니다.

## 구현 방법
* 명시적 방법을 사용한 분리 맞춤 segregated fit 할당기 구현입니다.
* 전역 `char*` type 배열을 사용해 각 가용리스트의 head를 저장합니다.
* 최대 클래스는 $2^{14}$까지로, 이를 초과하는 블록은 배열 마지막 원소로 저장됩니다.
* 할당받고자 하는 크기에 맞는 클래스부터 가용 블록을 검색하고, 하나의 클래스 안에서의 검색은 **first-fit**으로 진행됩니다.
* realloc은 다음과 같은 로직을 추가했습니다.
1. 현재보다 작은 사이즈를 요청하면 재할당하지 않습니다.
2. 오른쪽에 가용블록이 있고, 나와 병합하여 재할당이 가능하다면, 병합합니다.

## 구현 결과
```shell
Perf index = 51 (util) + 40 (thru) = 91/100
```

## 기타 구현방법의 결과
Implicit (+next fit)
```
Perf index = 44 (util) + 22 (thru) = 66/100
```

Explicit (+realloc 개선)
```
Perf index = 45 (util) + 40 (thru) = 85/100
```


---
```
#####################################################################
# CS:APP Malloc Lab
# Handout files for students
#
# Copyright (c) 2002, R. Bryant and D. O'Hallaron, All rights reserved.
# May not be used, modified, or copied without permission.
#
######################################################################

***********
Main Files:
***********

mm.{c,h}	
	Your solution malloc package. mm.c is the file that you
	will be handing in, and is the only file you should modify.

mdriver.c	
	The malloc driver that tests your mm.c file

short{1,2}-bal.rep
	Two tiny tracefiles to help you get started. 

Makefile	
	Builds the driver

**********************************
Other support files for the driver
**********************************

config.h	Configures the malloc lab driver
fsecs.{c,h}	Wrapper function for the different timer packages
clock.{c,h}	Routines for accessing the Pentium and Alpha cycle counters
fcyc.{c,h}	Timer functions based on cycle counters
ftimer.{c,h}	Timer functions based on interval timers and gettimeofday()
memlib.{c,h}	Models the heap and sbrk function

*******************************
Building and running the driver
*******************************
To build the driver, type "make" to the shell.

To run the driver on a tiny test trace:

	unix> mdriver -V -f short1-bal.rep

The -V option prints out helpful tracing and summary information.

To get a list of the driver flags:

	unix> mdriver -h

```
