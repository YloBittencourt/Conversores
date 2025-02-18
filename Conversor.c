#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h" 
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "ssd1306.h"

// Definições de pinos
#define LED_BLUE 12
#define LED_RED 13
#define LED_GREEN 11
#define BUTTON_A 5
#define I2C_SDA 14
#define I2C_SCL 15
#define JOYSTICK_X_PIN 26  
#define JOYSTICK_Y_PIN 27  
#define JOYSTICK_PB 22 
#define I2C_PORT i2c1
#define endereco 0x3C

// Definições de display
volatile bool alternar_leds = true;
volatile bool alternar_led_verde = false;
volatile int border_style = 0;
volatile uint32_t last_time = 0;

// Função de interrupção para os botões
void gpio_irq_handler(uint gpio, uint32_t events) {
    // Debounce
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    if(current_time - last_time > 200){
        // Verifica qual botão foi pressionado
        if (gpio == BUTTON_A){
            alternar_leds = !alternar_leds; // Alterna o estado dos LEDs
            if (!alternar_leds) {
                    pwm_set_gpio_level(LED_RED, 0);
                    pwm_set_gpio_level(LED_BLUE, 0);
            }
        } else if (gpio == JOYSTICK_PB){ // Alterna o estado do LED verde e o estilo da borda
            alternar_led_verde = !alternar_led_verde;
            gpio_put(LED_GREEN, alternar_led_verde);
            border_style = (border_style + 1) % 3;
        }
        last_time = current_time;
    }
}

// Função para configurar o PWM
void setup_pwm(uint pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(pin);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_wrap(&config, 4095);
    pwm_init(slice, &config, true);
}


int main(){
    // Inicialização de periféricos
    stdio_init_all();

    // Inicialização de pinos
    gpio_init(LED_RED);
    gpio_set_dir(LED_RED, GPIO_OUT);
    gpio_init(LED_BLUE);
    gpio_set_dir(LED_BLUE, GPIO_OUT);
    gpio_init(LED_GREEN);
    gpio_set_dir(LED_GREEN, GPIO_OUT);

    gpio_init(JOYSTICK_PB);
    gpio_set_dir(JOYSTICK_PB, GPIO_IN);
    gpio_pull_up(JOYSTICK_PB);
    gpio_set_irq_enabled_with_callback(JOYSTICK_PB, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    // Inicialização do display
    setup_pwm(LED_RED);
    setup_pwm(LED_BLUE);

    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    
    
    ssd1306_t ssd;
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT);
    ssd1306_config(&ssd); 
    ssd1306_send_data(&ssd);

    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    adc_init();
    adc_gpio_init(JOYSTICK_X_PIN);
    adc_gpio_init(JOYSTICK_Y_PIN);

    
    uint16_t adc_x, adc_y; // Valores lidos do ADC
    int eixo_x = WIDTH / 2 - 4, eixo_y = HEIGHT / 2 - 4; // Posição inicial do quadrado

    while (true){

        // Leitura dos valores do ADC
        adc_select_input(1);
        adc_x = adc_read();
        adc_select_input(0);
        adc_y = adc_read();
    
        // Cálculo da margem de zona morta
        int delta_x = abs(2048 - adc_x);
        int delta_y = abs(2048 - adc_y);
        int limite = 200; 
    
        // Atualização dos LEDs
        if (alternar_leds) {
            if (delta_x > limite) {
                pwm_set_gpio_level(LED_RED, (delta_x - limite) * 2);
            } else {
                pwm_set_gpio_level(LED_RED, 0);
            }
    
            if (delta_y > limite) {
                pwm_set_gpio_level(LED_BLUE, (delta_y - limite) * 2);
            } else {
                pwm_set_gpio_level(LED_BLUE, 0);
            }
        } else {
            pwm_set_gpio_level(LED_RED, 0);
            pwm_set_gpio_level(LED_BLUE, 0);
        }
        
        eixo_x = (adc_x * (WIDTH - 8)) / 4095;
        eixo_y = HEIGHT - 8 - (adc_y * (HEIGHT - 8)) / 4095;
    
        ssd1306_fill(&ssd, false);

            // Desenha a borda conforme o estado de border_style
        if (border_style == 1) {
            ssd1306_rect(&ssd, 0, 0, WIDTH, HEIGHT, true, false); // Borda simples
        } else if (border_style == 2) {
            ssd1306_rect(&ssd, 2, 2, WIDTH - 4, HEIGHT - 4, true, false); // Borda menor
        }

        ssd1306_rect(&ssd, eixo_y, eixo_x, 8, 8, true, false);
        ssd1306_send_data(&ssd);
    
        sleep_ms(100);
    }
    
    
}
