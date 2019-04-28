/*
 * Датчик кислорода и влажности
 * version 0.0.0
 * написана с использованием libopencm3
 * добавлен фреймбуфер
 * портировано на stm32f030
 * 10 feb 2019
 * v1.99
 * 28 apr 2019
 */

/**********************************************************************
 * Секция include и defines
**********************************************************************/
#define INCLUDE_FLOAT 1

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/usart.h>
#include "mini-printf.h"
#include "nokia5110frame.h"
#include "sht20.h"




/**********************************************************************
 * Секция глобальных переменных
 *********************************************************************/
 
uint8_t channel_array[] = {7,4,5,6, ADC_CHANNEL_TEMP};
	
/**********************************************************************
 * Секция настройки переферии
**********************************************************************/

static void gpio_setup(){
	
	//pa5 - Просто отладочный светодиод
	rcc_periph_clock_enable(RCC_GPIOA);
	gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT,
					GPIO_PUPD_NONE, GPIO5);
	gpio_set_output_options(GPIOA, GPIO_OTYPE_PP,
                    GPIO_OSPEED_2MHZ, GPIO5);
    gpio_clear(GPIOA,GPIO5);
    
}

	
void spi_setup(void){
	//Enable SPI1 Periph and gpio clocks
	rcc_periph_clock_enable(RCC_GPIOB);
	rcc_periph_clock_enable(RCC_SPI1);
	/* Configure GPIOs:
	 * SCK PB3 
	 * MOSI PB5
	 * CS PB4
	 * RST PB7
	 * DC PB6
	 */
	 //CS,RST,DC
	gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT,
					GPIO_PUPD_NONE, GPIO4|GPIO7|GPIO6);
	gpio_set_output_options(GPIOB, GPIO_OTYPE_PP,
                    GPIO_OSPEED_2MHZ, GPIO4|GPIO7|GPIO6);
    gpio_set(GPIOB,GPIO4|GPIO7|GPIO6);
	//MOSI,SCK
	gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO3|GPIO5);
	gpio_set_af(GPIOB,GPIO_AF0,GPIO3|GPIO5);
	//Настройка SPI1
	spi_disable(SPI1);
	spi_reset(SPI1);
	spi_init_master(SPI1, SPI_CR1_BAUDRATE_FPCLK_DIV_128,
					SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
					SPI_CR1_CPHA_CLK_TRANSITION_1, SPI_CR1_MSBFIRST);
	//spi_set_crcl_8bit(SPI1); Закоментировато тк один хрен не работает
	spi_enable_software_slave_management(SPI1);
	spi_set_nss_high(SPI1);
	spi_enable(SPI1);
	}

static void i2c_setup(void){
	/* Enable clocks for I2C2 and AFIO. */
	rcc_periph_clock_enable(RCC_I2C1);
	/* Set alternate functions for the SCL and SDA pins of I2C2.
	 * SDA PB7
	 * SCL PB6
	 *  */
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO9|GPIO10);
	gpio_set_output_options(GPIOA, GPIO_OTYPE_OD,
                    GPIO_OSPEED_2MHZ, GPIO9|GPIO10);
    gpio_set_af(GPIOA,GPIO_AF4,GPIO9|GPIO10);
	
	/* Disable the I2C before changing any configuration. */
	i2c_peripheral_disable(I2C1);
	i2c_set_speed(I2C1,i2c_speed_fm_400k,48);
	i2c_peripheral_enable(I2C1);
}	
	
	
static void adc_setup(void){
	//На PA6 висит lm358 с усилением 100 подкюченая к датчику
	rcc_periph_clock_enable(RCC_ADC);
	rcc_periph_clock_enable(RCC_GPIOA);

	gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE,
					GPIO7);
	
	adc_power_off(ADC1);
	adc_set_clk_source(ADC1, ADC_CLKSOURCE_ADC);
	adc_calibrate(ADC1);
	adc_set_operation_mode(ADC1, ADC_MODE_SEQUENTIAL);
	adc_disable_external_trigger_regular(ADC1);
	adc_set_right_aligned(ADC1);
	adc_enable_temperature_sensor();
	 adc_enable_vbat_sensor();
	adc_set_sample_time_on_all_channels(ADC1, ADC_SMPTIME_239DOT5);
	adc_set_regular_sequence(ADC1, 1, channel_array);
	adc_set_resolution(ADC1, ADC_RESOLUTION_12BIT);
	adc_disable_analog_watchdog(ADC1);
	adc_power_on(ADC1);

	/* Wait for ADC starting up. */
	int i;
	for (i = 0; i < 800000; i++) {    /* Wait a bit. */
		__asm__("nop");
	}

}




/**********************************************************************
 * Секция основных функций
 **********************************************************************/
 
//Очень маленький print to uart для отладки
//void usart_print(char *data){
//	while (*data) usart_send_blocking(USART1, *data++);
	//usart_send_blocking(USART1,(int)'\n');
//}



static void lcd_adc(uint16_t data){
	char bufer[70];
	uint16_t temp,temp2,input,percent,raw=data;
	bufer_clear();
	//if(data<915) data=0; else data-=915;
	temp=(3300*data)>>12;
	//temp2=(3300*raw)>>12;
	input=temp/14;
	//snprintf(bufer, 70,
	//			"adc value:%d\nUadcc=%d mv\nUadcr=%d mv\nUin=%d.%d mv",
	//				data,temp,temp2,input/10,input%10);
	snprintf(bufer, 70,
				"Oxygen sensor:\nUin=%d.%d mv", input/10,input%10);
	lcdstr_at(bufer,0,0);
	percent=10*temp/77;//Подгонка под московский воздух
	snprintf(bufer,50, "%d.%d%%", percent/10,percent%10);
	lcdstrx2_at(bufer,3,3);
	draw_rectangle(0,16,83,31);
	//drawLine(0, 31, 83,31);
	bufer_send();
	
}
uint16_t mesure(void){
	unsigned int temp=0;
	for(unsigned int i=0; i<128;i++){
		adc_start_conversion_regular(ADC1);
		while (!(adc_eoc(ADC1)));
		temp += adc_read_regular(ADC1);	
		}
	return temp>>7;
	//return temp;
	}

static void test_lcd(){
	char bufer[256];
	uint16_t temp,mv,rh,t,data,ro,o;
	float V,RH,T,RO;
	data=mesure();
	bufer_clear();
	mv=(3300*data)>>12;
	mv-=356;
	
	o=mv/19;
	
	
	rh=sht_read_rh();
	t=sht_read_t();
	ro=sht_ro(rh,t);
	
	
	rh/=10;
	t/=10;
	ro/=10;
	snprintf(bufer, 256,
	"info:\nUadc=%d mV\nO2 %d.%d%%\nRH=%d.%d%\nT=%d.%d C\nro=%d.%d mg/l",
		mv,o/10,o%10,rh/10,rh%10,t/10,t%10,ro/10,ro%10);
	lcdstr_at(bufer,0,0);
	bufer_send();
}


static void logo(void){
	bufer_clear();
	lcdstr_at("The best",3,0);
	lcdstr_at("Oxygen sensoR!",0,1);
	lcdstr_at("v. 1.9",3,2);
	lcdstrx2_at("SHAMAN",2,3);
	lcdstr_at("28 apr. 2019",0,5);
	//drawLine(0, 7, 83,7);
	//draw_circle(42,24,23);
	//draw_rectangle(0,0,83,47);
	bufer_send();
}

int main(void){
	rcc_clock_setup_in_hsi_out_48mhz();

	gpio_setup();
	spi_setup();
	i2c_setup();
	
	lcdinit();
	adc_setup();
	lcdclear();
	//init command
	logo();
	for(int i=0;i<0xffffff;i++)__asm__("nop");
	
	
	uint16_t temp=0;
	
	while (1){
		for(int i=0;i<0xffff;i++)__asm__("nop");
		//temp = mesure();
		//lcd_adc(temp);
		test_lcd();	
		};
		
return 0;
}
