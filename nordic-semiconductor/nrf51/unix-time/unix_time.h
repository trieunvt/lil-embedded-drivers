/**
 * @file    unix_time.h
 * @author  trieunvt
 * @brief   Header file of Unix time module.
 **/

#ifndef _UNIX_TIME_H_
#define _UNIX_TIME_H_

#include "stdint.h"
#include "stdbool.h"

/* Developer-defined macro(s) */
#define UT_LEAP_YEAR(year)  ((((year) % 4 == 0) && ((year) % 100 != 0)) || ((year) % 400 == 0))
#define UT_OFFSET_YEAR      (1970)

/* Struct type definition(s) */
typedef struct UnixTime_Class   UnixTime_ClassTypeDef;
typedef struct UnixTime_API     UnixTime_APITypeDef;
typedef struct UnixTime_Date    UnixTime_DateTypeDef;

struct UnixTime_Class {
    const UnixTime_APITypeDef *api;
    UnixTime_DateTypeDef *date;
    uint32_t unix;
};

struct UnixTime_API {
    void (*init)(UnixTime_ClassTypeDef *self);
    bool (*convertToDate)(UnixTime_ClassTypeDef *self, uint32_t unix);
    bool (*convertToUnix)(UnixTime_ClassTypeDef *self,
                          UnixTime_DateTypeDef *date);
};

struct UnixTime_Date {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
};

/* For everywhere usage */
extern UnixTime_ClassTypeDef unix_time;

#endif /* _UNIX_TIME_H_ */
