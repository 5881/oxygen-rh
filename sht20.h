//Библиотека для работы с датчиком влажности

/*********************************************************************
 * ver 0.3
 * A Belyy 21 apr 2019
 * 0.2 исправлен переход через 0 для влажности
 * 0.3 добвлен рассчёт абсолютной влажности
 *********************************************************************/



#define SHT20_ADDRESS  0X40
#define SHT20_Write_Add 0x80
#define SHT20_Read_Add	0x81
#define SHT20_Measurement_RH_HM  0XE5
#define SHT20_Measurement_T_HM  0XE3
#define SHT20_Measurement_RH_NHM  0XF5  
#define SHT20_Measurement_T_NHM  0XF3 
#define SHT20_READ_REG  0XE7
#define SHT20_WRITE_REG  0XE6
#define SHT20_SOFT_RESET  0XFE



//плотность насыщенного пара
static const uint16_t ro_saturated[21]={49,68,94,128,173,230,304,
	396,512,654,828,1040,1295,1601,1964,2393,2897,3487,4173,4968,5885};

uint16_t sht_read_rh(){
	for(uint32_t i=0;i<0xffff;i++)__asm__("nop");
	unsigned char temp[2];
	temp[0]=SHT20_Measurement_RH_HM;
	uint16_t data;
	i2c_transfer7(I2C1, SHT20_ADDRESS, temp,1,temp,2);
	data=temp[0]<<8|temp[1];
	temp[0]=SHT20_SOFT_RESET;
	i2c_transfer7(I2C1, SHT20_ADDRESS, temp,1,0,0);
	
	//вычислим влажность
	//-- calculate relative humidity [%RH] --
    //humidityRH = -6.0 + 125.0/65536 * (float)u16sRH; // RH= -6 + 125 * SRH/2^16
    data&=~0x0003; //сброс статус битов
    data=12500*data/65536;
    if(data>600) data-=600; else data=0; //чтоб не глючила в нуле
    return data;
}

uint16_t sht_read_t(){
	for(uint32_t i=0;i<0xffff;i++)__asm__("nop");
	unsigned char temp[2];
	temp[0]=SHT20_Measurement_T_HM;
	uint16_t data;
	i2c_transfer7(I2C1, SHT20_ADDRESS, temp,1,temp,2);
	data=temp[0]<<8|temp[1];
	temp[0]=SHT20_SOFT_RESET;
	i2c_transfer7(I2C1, SHT20_ADDRESS, temp,1,0,0);
	
	//вычислим температуру
	//-- calculate temperature [℃] --
	//T= -46.85 + 175.72 * ST/2^16
    data&=~0x0003; //сброс статус битов
    data=17572*data/65536 - 4685;
    return data;
}

uint16_t sht_ro(uint16_t RH, uint16_t t){//рассчёт абсолютной влажности
	uint8_t n;							// методом интерполяции
	uint16_t ro_sat_t,a,b,t_rel;
	n=t/500; //t домножено на 1000, в массиве шаг 5℃ 
	a=ro_saturated[n];
	b=ro_saturated[n+1];
	t_rel=t%500;
	ro_sat_t=a+(b-a)*t_rel/500;
	return RH*ro_sat_t/1000;
}
	
	
	

