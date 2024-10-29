//Portions from https://github.com/dbdexter-dev/sondedump/tree/master/sonde/dfm09
//under MIT license
#ifndef __M10_H__
#define __M10_H__

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "m20.h"
#include "main.h"

#define M10_MAX_DATA_LEN 99
#define M10_PACKET_LENGTH 202U

extern const Sonde __code m10;

typedef struct {
	uint8_t sync_mark[3];
	uint8_t len;
	uint8_t type;
	uint8_t data[M10_MAX_DATA_LEN];
} M10Frame;

typedef struct {
	uint8_t sync_mark[3];
	uint8_t len;
	uint8_t type;                       /* 0x9f */

	uint8_t small_values[2];
	uint8_t dlat[2];    /* x velocity */
	uint8_t dlon[2];    /* y velocity */
	uint8_t dalt[2];    /* z velocity */
	uint8_t time[4];    /* GPS time */
	uint8_t lat[4];
	uint8_t lon[4];
	uint8_t alt[4];
	uint8_t _pad0[4];
	uint8_t sat_count;   /* Number of satellites used for fix */
	uint8_t _pad3;
	uint8_t week[2];    /* GPS week */

	uint8_t prn[12];            /* PRNs of satellites used for fix */
	uint8_t _pad1[4];
	uint8_t rh_ref[3];          /* RH reading @ 55% */
	uint8_t rh_counts[3];       /* RH reading */
	uint8_t _pad2[6];
	uint8_t adc_temp_range;     /* Temperature range index */
	uint8_t adc_temp_val[2];    /* Temperature ADC value */
	uint8_t unk0[4];            /* Probably related to temp range */
	uint8_t adc_batt_val[2];
	uint8_t unk3[2];    /* Correlated to adc_battery_val, also very linear */
	uint8_t _pad4[12];
	uint8_t unk4[2];    /* Fairly constant */
	uint8_t unk5[2];    /* Fairly constant */
	uint8_t unk6[2];    /* Correlated to unk0 */
	uint8_t unk7[2];

	uint8_t serial[5];
	uint8_t seq;
} M10Frame_9f;

uint16_t m10CrcStep(uint16_t c, uint8_t b);
#endif