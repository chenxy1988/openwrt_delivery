#ifndef __CAN_PROTOCOL_H__
#define __CAN_PROTOCOL_H__
#define BMS_CAN_BROCAST				0x4200
#define BMS_BATTERY_GROUP_OVERVIEW		0x4210
#define BMS_BATTERY_GROUP_MEMBERS		0xf
#define BMS_BATTERY_GROUP_CHARGE_DISCHARGE	0x4220
#define BMS_BATTERY_SINGLE_VOLT			0x4230
#define BMS_BATTERY_TEMP			0X4240
#define BMS_BATTERY_BASIC_INFORMATION		0X4250
#define BMS_BATTERY_GROUP_VOLT			0x4260
#define BMS_BATTERY_GROUP_TEMP			0x4270
#define BMS_BATTERY_DISABLE_CHARGE		0x4280
#define BMS_SYSTEM_VERSION			0x7310
#define BMS_SYSTEM_INFO				0x7320
#define BMS_SYSTEM_WAKEUP			0x8200
#define BMS_SINGLE_INFO				0x5000
#define BMS_SINGLE_INFO_RANGE			0x80
#define BMS_SINGLE_TEMP				0x6000
#define BMS_MODULE_INFO				0x7000
#define BMS_MODULE_INFO_RANGE			0xf
#define BMS_MODULE_TEMP				0x7200
#define BMS_BREAK				0x8220
#define BMS_BREAK_RESP				0x8230


int can_decompose(int fd,unsigned short int id,unsigned char *msg,unsigned char len);

#endif

