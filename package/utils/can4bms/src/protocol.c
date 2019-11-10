#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "protocol.h"
#include "network.h"

static int can_bms_global_info(int fd)
{
	int i;
	unsigned char msg[8];
	for(i=0;i<BMS_BATTERY_GROUP_MEMBERS;i++)
	{
		memset(msg,0x0,8);
//[1] 电池总电压 BYTE0 BYTE 1
		msg[0] = 3;
		msg[1] = 7;
//[2] 电池组电流 BYTE 2 BYTE 3
		msg[2] = 1;
		msg[3] = 2;
//[3] 环境温度 BYTE 5
		msg[4]=37;
		msg[5]=2;
//[4] SOC BYTE 6
		msg[6]=0xfa;
//[5] SOH BYTE 7
		msg[7]=0x1f;
		network_send_data(fd,BMS_BATTERY_GROUP_OVERVIEW+i,msg,8,EXT_FRAME);

//[1] 充电截止电压 BYTE 0 BYTE 1
		msg[0] = 3;
		msg[1] = 8;
//[2] 放电截止电压 BYTE2 BYTE 3
		msg[2] = 3;
		msg[3] = 5;
//[3] 最大充电电流 BYTE 4 BYTE 5
		msg[4] = 1;
		msg[5] = 2;
//[4] 最大放电电流 BYTE 6 BYTE 7
		msg[6] = 1;
		msg[7] = 11;
		network_send_data(fd,BMS_BATTERY_GROUP_CHARGE_DISCHARGE+i,msg,8,EXT_FRAME);

//[1] 最高单体电池电压 byte 0 byte 1
		msg[0] = 1;
		msg[1] = 5;

//[2] 最低单体电池电压 byte 2 byte 3
		msg[2] = 1;
		msg[3] = 2;

//[3] 最高单体电池电压编号 byte 4 byte 5
		msg[4] = 11;
		msg[5] = 22;

//[4] 最低单体电池电压编号 byte 6 byte 7
		msg[6] = 22;
		msg[7] = 33;
		network_send_data(fd,BMS_BATTERY_SINGLE_VOLT+i,msg,8,EXT_FRAME);

//[1] 最高单体电池温度 byte 0 byte 1
		msg[0] = 33;
		msg[1] = 1;

//[2] 最低单体电池温度 byte 2-3
		msg[2] = 33;
		msg[3] = 2;
//[3] 最高单体电池编号byte 4-5
		msg[4] = 2;
		msg[5] = 1;
//[4] 最低单体电池温度编号 6-7
		msg[6] = 0x11;
		msg[7] = 0x11;
		network_send_data(fd,BMS_BATTERY_TEMP+i,msg,8,EXT_FRAME);

//[1]  基本状态 byte 0
// bit0 0-未充电 1-充电
// bit1 0-未放电 1-放电
// bit2 0-休眠 1-唤醒
// bit3 xx
// bit4 xx
// bit5 0-接触器闭合 1-接触器断开
// bit6 0-未充满 1-充满
// bit7 0-未放空 1-防空
		msg[0] = 0xff;
//[2] 循环周期 byte 1 byte 2
		msg[1] = 12;
		msg[2] = 34;
//[3] 故障 byte 3
// bit0 VOLT ERR /电压传感器故障
// bit1 TMPR ERR /温度传感器故障
// bit2 IN COMM ERR/内部通信故障
// bit3 dcov_err /输入过压故障
// bit4 RVERR 输入反接故障
// bit5 继电器检测故障
// bit6-7 reserved
		msg[3] = 0x0;
//[4] 告警 byte 4-5
//bit 0 BLV/电池单体低压告警
//bit1BHV 电池单体高压告警
//bit2 plv 电池组放电低压告警
//bit3 phv 电池组充电高压告警
//bit4 clt 电池组充电低温告警
//bit5 CHT 电池组充电高温告警
//bit6 dlt 电池组放电低温告警
//bit7 dht 电池组放电高温告警
//
//bit 0 电池组充电过流告警
//bit 1 DOCA 电池组放电过流告警
		msg[4] = 0x0;
		msg[5] = 0x0;
//[5] 保护 byte 6-7
//bit 0 BUV 电池单体欠压保护
//bit1 BOV电池单体过压保护
//bit2 puv 电池组放电欠压保护
//bit3 pov 电池组过压保护
//bit4 cut 充电低温保护
//bit5 cot 充电高温保护
//bit6 dut 放电低温保护
//bit7 dot 放电高温保护
//
//bit0 coc 电池组充电过流保护
//bit1 doc电池放电过流保护
		msg[6] = 0x0;
		msg[7]=0x0;
		network_send_data(fd,BMS_BATTERY_BASIC_INFORMATION+i,msg,8,EXT_FRAME);

//[1] 最高电池组电压 BYTE0 1
		msg[0] = 3;
		msg[1] = 2;
//[2] 最低电池组电压 byte 2-3
		msg[2] = 3;
		msg[3] = 1;
//[3] 最高电池组电压编号 byte 4-5
		msg[4] = 0;
		msg[5] = 12;
//[4] 最低电池组电压编号 6-7
		msg[6] = 0;
		msg[7]=13;
		network_send_data(fd,BMS_BATTERY_GROUP_VOLT+i,msg,8,EXT_FRAME);

//[1] 最高电池组温度 byte0-1
		msg[0] = 33;
		msg[1] = 3;
//[2] 最低电池组温度 2-3
		msg[2] = 33;
		msg[3] = 1;
//[3] 最高电池组温度编号 byte 4-5
		msg[4] = 0;
		msg[5] = 1;
//[4] 最低电池组温度编号 byte 6-7
		msg[6] = 0;
		msg[7] = 2;
		network_send_data(fd,BMS_BATTERY_GROUP_TEMP+i,msg,8,EXT_FRAME);

		memset(msg,0x0,8);
//[1] 禁止充电标志 0xAA 有效，其它值无效 byte 0
		msg[0] = 0xaa;
//[1] 禁止放电标志 0xAA 有效，其它值无效 byte 1
		msg[1] = 0xaa;
		network_send_data(fd,BMS_BATTERY_DISABLE_CHARGE+i,msg,8,EXT_FRAME);

	}

	return 0;
}

static int can_bms_global_device_info(int fd)
{
	int i;
	unsigned char msg[8];

	for(i=0;i<BMS_BATTERY_GROUP_MEMBERS;i++)
	{
//[1] 硬件版本	0:无效;	1:A 版本;	2:B 版本 ;	其他:预留 byte 0
		msg[0] = 1;
//硬件版本-SP	0x10 byte 1
		msg[1] = 0x10;
//硬件版本-V
		msg[2] = 0x20;
//硬件版本-R
		msg[3] = 0x30;
//软件版本-V
		msg[4] = 0x15;
//软件版本-B
		msg[5] = 0x50;
//a软件版本-S
		msg[6] = 0x16;
		msg[7] = 0x0;//reserved

		network_send_data(fd,BMS_SYSTEM_VERSION+i,msg,8,EXT_FRAME);

//[1] 电池总数 byte 0-1
		msg[0] = 0x1;
		msg[1] = 0x11;
//[2] 串联 module 个数 byte 2
		msg[2] = 0x3;
//[3] Module 中电池个数 byte 3
		msg[3] = 127;
//[4] 电压平台 byte 4-5
		msg[4] = 0xff;
		msg[5] = 0xfa;
//[5] 安时（Ah）数 byte 6
		msg[6] = 10;
		msg[7] = 0xff;//reserved
		network_send_data(fd,BMS_SYSTEM_INFO+i,msg,8,EXT_FRAME);
	}

	return 0;

}

static int can_bms_wakeup(unsigned char data)
{
	if(data == 0x55){
//进入睡眠状态
		printf("Sleep .... \n");
	}else if(data == 0xaa){
//唤醒设备，退出休眠状态
		printf("Wake up !!!\n");
	}
	return 0;
}

static int can_bms_cell_module_data(int fd)
{
	int j,i;unsigned char msg[8];
	unsigned int range = BMS_SINGLE_INFO_RANGE;

	for(i=0;i<0xf;i++)
	{
//00~7F 为 128 帧数据，对应 0~511 串电池，每帧 4 个 cell volt，依次排列
//Cell volt byte 0-1 2-3 4-5 6-7
		for(j=0;j<range;j++){
			msg[0] = 0x3;
			msg[1] = 7;
			msg[2] = 0x3;
			msg[3] = 6;
			msg[4] = 0x3;
			msg[5] = 5;
			msg[6] = 3;
			msg[7] = 6;
			network_send_data(fd,BMS_SINGLE_INFO+i+(j*0x10),msg,8,EXT_FRAME);
		}


//00~7F 为 128 帧数据，对应 0~511 串电池，每帧 4 个 cell tempreture，依次排列
////Cell temp byte 0-1 2-3 4-5 6-7
		for(j=0;j<range;j++){
			msg[0] = 0x3;
			msg[1] = 7;
			msg[2] = 0x3;
			msg[3] = 6;
			msg[4] = 0x3;
			msg[5] = 5;
			msg[6] = 3;
			msg[7] = 6;
			network_send_data(fd,BMS_SINGLE_TEMP+i+(j*0x10),msg,8,EXT_FRAME);
		}


//00~0F 为 16 帧数据，对应 64 个 module，每帧 4 个 module，依次排列
//module volt byte 0-1 2-3 4-5 6-7
		for(j=0;j<BMS_MODULE_INFO_RANGE;j++){
			msg[0] = 0x3;
			msg[1] = 7;
			msg[2] = 0x3;
			msg[3] = 6;
			msg[4] = 0x3;
			msg[5] = 5;
			msg[6] = 3;
			msg[7] = 6;
			network_send_data(fd,BMS_MODULE_INFO+i+(j*0x10),msg,8,EXT_FRAME);
		}
//20~2F 为 16 帧数据，对应 64 个 module，每帧 4 个 module，依次排列
//Module temp byte 0-1 2-3 4-5 6-7
		for(j=0x20;j<(BMS_MODULE_INFO_RANGE * 2);j++)
		{
			msg[0] = 0x3;
			msg[1] = 7;
			msg[2] = 0x3;
			msg[3] = 6;
			msg[4] = 0x3;
			msg[5] = 5;
			msg[6] = 3;
			msg[7] = 6;
			network_send_data(fd,BMS_MODULE_TEMP+i+(j*0x10),msg,8,EXT_FRAME);
		
		}
	
	}

	return 0;
}

static int can_bms_break(unsigned short int id,unsigned char data,int fd)
{
	unsigned char msg[8];
//0xAA: 强制电池组断开主继电器
//0x55：电池组闭合主继电器其他：无效

	if(data == 0xaa)
	{
		printf("Break cell \n");
	}else if(data == 0x55){
		printf("Close cell \n"); 
	}

	network_send_data(fd,BMS_BREAK_RESP + (id - BMS_BREAK),msg,8,EXT_FRAME);

	return 0;
}
int can_decompose(int fd,unsigned short int id,unsigned char *msg,unsigned char len)
{
	int i;
	printf("DECOMPOSE MESSAGE CANID:%x, MSG LEN=%d,MSG=:",id,len);
	for(i=0;i<len;i++)
	{
		printf("%02x,",msg[i]);
	}
	printf("\n");

	if(id == BMS_CAN_BROCAST){
		switch(msg[0]){
			case 0: //GLOBAL INFORMATION
				can_bms_global_info(fd);
			break;
			case 1: //CELL and Modules data
				can_bms_cell_module_data(fd);
			break;
			case 2: //GLOBAL DEVICE INFORMATION 
				can_bms_global_device_info(fd);
			break;
		}
	
	}else if((id >= BMS_SYSTEM_WAKEUP) || (id < (BMS_SYSTEM_WAKEUP + 0xf))){
		can_bms_wakeup(msg[0]);
	}else if((id >= BMS_BREAK ) || (id < BMS_BREAK + 0xf)){
		can_bms_break(id,msg,fd);
	}else{
		printf("Unknown message, return \n");
		return 0;
	}

	return 0;
}
