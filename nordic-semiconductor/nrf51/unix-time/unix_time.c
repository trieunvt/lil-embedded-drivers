/**
 * @file    unix_time.c
 * @author  trieunvt
 * @brief   Unix time module driver.
 **/

#include "unix_time.h"

static const uint8_t DaysInAMonthMap[2][12] = {
	{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},	/* Not leap year */
	{31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}	/* Leap year */
};

/* Static function declaration(s) */
static void UnixTime_Init(UnixTime_ClassTypeDef *self);
static bool UnixTime_ConvertToDate(UnixTime_ClassTypeDef *self, uint32_t unix);
static bool UnixTime_ConvertToUnix(UnixTime_ClassTypeDef *self,
								   UnixTime_DateTypeDef *date);

/* API declaration(s) */
static const UnixTime_APITypeDef unix_time_api = {
    .init           = UnixTime_Init,
	.convertToDate  = UnixTime_ConvertToDate,
    .convertToUnix  = UnixTime_ConvertToUnix,
};

/* Class declaration(s) */
UnixTime_ClassTypeDef unix_time = {
    .api = &unix_time_api,
};

/* Initialize Unix time */
static void UnixTime_Init(UnixTime_ClassTypeDef *self) {
	self->unix          = 0;
	self->date->year    = 0;
	self->date->month   = 0;
	self->date->day     = 0;
	self->date->hour    = 0;
	self->date->minute  = 0;
	self->date->second  = 0;
}

static bool UnixTime_ConvertToDate(UnixTime_ClassTypeDef *self, uint32_t unix) {
	if (unix < 1) {
		return false;
	}

	self->date->second = unix % 60;
	unix /= 60;

	self->date->minute = unix % 60;
	unix /= 60;

	self->date->hour = unix % 24;
	unix /= 24;

	uint32_t a, b, c, d, e, f;

	a = (uint32_t)((4 * unix + 102032) / 146097 + 15);
	b = (uint32_t)(unix + 2442113 + a - (a / 4));
	c = (20 * b - 2442) / 7305;
	d = b - 365 * c - (c / 4);
	e = d * 1000 / 30601;
	f = d - e * 30 - e * 601 / 1000;

	/* January and February are counted as months 13 and 14 of the previous year */
	if (e <= 13) {
		c -= 4716;
		e -= 1;
	} else {
		c -= 4715;
		e -= 13;
	}

	self->date->year = c;
	self->date->month = e;
	self->date->day = f;

	return true;
}

static bool UnixTime_ConvertToUnix(UnixTime_ClassTypeDef *self,
                                   UnixTime_DateTypeDef *date) {
	uint32_t days = 0, seconds = 0;

	if (date->year < UT_OFFSET_YEAR) {
		return false;
	}

	for (uint16_t i = UT_OFFSET_YEAR; i < date->year; i++) {
		if (UT_LEAP_YEAR(i)) {
			days += 366;
		} else {
			days += 365;
		}
	}

	for (uint16_t i = 1; i < date->month; i++) {
		days += DaysInAMonthMap[UT_LEAP_YEAR(date->year)][i - 1];
	}

	days += date->day - 1;
	seconds = days * 86400;
	seconds += date->hour * 3600;
	seconds += date->minute * 60;
	seconds += date->second;
	self->unix = seconds;

	return true;
}
