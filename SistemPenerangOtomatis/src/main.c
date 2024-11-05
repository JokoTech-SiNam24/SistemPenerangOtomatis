/* 	
	Sistem Penerang Otomatis Menggunakan LDR

	Kelompok JokoTech:
	- Muhammad Daffa Grahito Triharsanto 		2206820075
	- Muhammad Yusuf Haikal 					2206081490
	- Tegar Wahyu Khisbulloh 					2206082032
	- Arju Naja Muhammad 						2206082045
*/
#include <asf.h>
#include <stdio.h>
#include <ioport.h>
#include <adc.h>
#include <sysclk.h>
#include <gfx_mono.h>
#include <gfx_mono_text.h>
#include <delay.h>
#include <tc.h>

static char strbuf[128];
#define LIGHT_THRESHOLD 4000  	// Adjust threshold
void timer_callback(void);		// Deklarasi fungsi timer callback
volatile uint16_t ldr_value = 0;

// Konfigurasi ADC
static void configure_adc(void) {
	struct adc_config adc_conf;
	struct adc_channel_config adcch_conf;

	adc_read_configuration(&ADCA, &adc_conf);
	adcch_read_configuration(&ADCA, ADC_CH0, &adcch_conf);

	adc_set_conversion_parameters(&adc_conf, ADC_SIGN_OFF, ADC_RES_12, ADC_REF_VCC);
	adc_set_clock_rate(&adc_conf, 200000UL);
	adc_set_conversion_trigger(&adc_conf, ADC_TRIG_MANUAL, 1, 0);
	adc_write_configuration(&ADCA, &adc_conf);

	adcch_set_input(&adcch_conf, ADCCH_POS_PIN0, ADCCH_NEG_NONE, 1);
	adcch_write_configuration(&ADCA, ADC_CH0, &adcch_conf);

	adc_enable(&ADCA);
}

// Baca nilai ADC
uint16_t read_adc(void) {
	adc_start_conversion(&ADCA, ADC_CH0);
	adc_wait_for_interrupt_flag(&ADCA, ADC_CH0);
	return adc_get_result(&ADCA, ADC_CH0);
}

// Fungsi untuk mengkonfigurasi pin lampu
void configure_light_pin(void) {
	PORTC.DIR |= PIN0_bm;     
	PORTC.OUTCLR = PIN0_bm;   
}

// Setup timer interrupt
void setup_timer(void) {
	tc_enable(&TCC0);
	tc_set_overflow_interrupt_callback(&TCC0, timer_callback);
	tc_set_wgm(&TCC0, TC_WG_NORMAL);
	tc_write_period(&TCC0, 10000); // Adjustable tc period untuk screen
	tc_set_overflow_interrupt_level(&TCC0, TC_INT_LVL_HI);
	tc_write_clock_source(&TCC0, TC_CLKSEL_DIV1_gc); // No prescale
}

// Timer callback untuk logic interrupt kontrol lampu
void timer_callback(void) {
	ldr_value = read_adc();
	
	// Saat LDR terbuka (terang) = nilai ADC rendah = LOW
	// Saat LDR tertutup (gelap) = nilai ADC tinggi = HIGH
	if (ldr_value > LIGHT_THRESHOLD) {
		// Kondisi terang (HIGH)
		PORTC.OUTSET = PIN0_bm; // Turn on LED
		} else {
		// Kondisi gelap (LOW)
		PORTC.OUTCLR = PIN0_bm; // Turn off LED
	}
}

int main(void) {
	sysclk_init();
	board_init();
	pmic_init();
	
	configure_adc();
	configure_light_pin();
	setup_timer();

	// Enable global interrupts
	cpu_irq_enable();
	
	gfx_mono_init();
	gpio_set_pin_high(LCD_BACKLIGHT_ENABLE_PIN);
	
	gfx_mono_draw_string("LDR Lamp Control", 0, 0, &sysfont);
	
	while (1) {
		// Update nilai ADC di LCD
		snprintf(strbuf, sizeof(strbuf), "ADC: %4d", ldr_value);
		gfx_mono_draw_string(strbuf, 0, 8, &sysfont);
		
		if (ldr_value > LIGHT_THRESHOLD) {
			gfx_mono_draw_string("LDR: HIGH", 0, 16, &sysfont);
			gfx_mono_draw_string("Lamp: ON ", 0, 24, &sysfont);
		} else {
			gfx_mono_draw_string("LDR: LOW ", 0, 16, &sysfont);
			gfx_mono_draw_string("Lamp: OFF", 0, 24, &sysfont);
		}
		
		delay_ms(100);
	}
}