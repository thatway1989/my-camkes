/*************************************************************************
    > File Name: ComStack_Types.h
    > SPDX-License-Identifier: BSD-2-Clause
    > Author: XXX
    > Created Time: 2021年11月17日 星期三 10时43分08秒
 ************************************************************************/
#ifndef __COMSTACK_TYPES_H_
#define __COMSTACK_TYPES_H_

typedef int uint16;

typedef uint16 PduIdType;
typedef uint16 PduLengthType;

// IMPROVEMENT: remove all non-E_prefixed error enum values
typedef enum {
    BUFREQ_OK=0,
    BUFREQ_NOT_OK=1,
    BUFREQ_E_NOT_OK=BUFREQ_NOT_OK,
    BUFREQ_BUSY=2,
    BUFREQ_E_BUSY=BUFREQ_BUSY,
    BUFREQ_OVFL=3,
    BUFREQ_E_OVFL=BUFREQ_OVFL,
} BufReq_ReturnType;
#endif
