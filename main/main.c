#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <hd44780.h>
#include <esp_idf_lib_helpers.h>
#include "driver/gpio.h"

#define BUZZER GPIO_NUM_10
#define BUTTON GPIO_NUM_17

void lcd_task(void *pv) 
{
    vTaskDelay(pdMS_TO_TICKS(500));   

    hd44780_t lcd =
    {
        .write_cb = NULL,
        .font = HD44780_FONT_5X8,
        .lines = 4,
        .pins = {
            .rs = GPIO_NUM_38,
            .e  = GPIO_NUM_37,
            .d4 = GPIO_NUM_36,
            .d5 = GPIO_NUM_35,
            .d6 = GPIO_NUM_48,
            .d7 = GPIO_NUM_47,
            .bl = HD44780_NOT_USED
        }
    };

    hd44780_init(&lcd);
    vTaskDelay(pdMS_TO_TICKS(200));
    hd44780_clear(&lcd);

    // Custom characters
    uint8_t customCharRight[8] = {0x00,0x1B,0x1F,0x0E,0x04,0x00,0x00,0x00};
    uint8_t customCharLeft[8]  = {0x00,0x1B,0x1F,0x1F,0x0E,0x00,0x00,0x00};

    hd44780_upload_character(&lcd, 0, customCharRight);  // prints as 0x08
    hd44780_upload_character(&lcd, 1, customCharLeft);   // prints as 0x09

    // Left icon
    hd44780_gotoxy(&lcd, 0, 0);
    hd44780_putc(&lcd, 0x09);

    // Right icons (5)
    int start_col = 16 - 5;
    hd44780_gotoxy(&lcd, start_col, 0);
    for (int i = 0; i < 5; i++) {
        hd44780_putc(&lcd, 0x08);
    }

    // Row 2 text
    hd44780_gotoxy(&lcd, 0, 1);
    hd44780_puts(&lcd, "In Progress");

    while (1) {
        // Later you can update the LCD here if needed
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void buzzer_task(void *pv)
{
    //button configuration 
    gpio_config_t btn_conf = {
        .pin_bit_mask = (1ULL << BUTTON),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&btn_conf);

    //buzzer configuration
    gpio_reset_pin(BUZZER);
    gpio_set_direction(BUZZER, GPIO_MODE_OUTPUT);
    gpio_set_level(BUZZER, 0);

    int prev = 1;       // button not pressed
    int curr = 1;
    int buzzer_on = 0;  // buzzer starts OFF

    while (1) {
        curr = gpio_get_level(BUTTON);

        //detects release 
        if (prev == 0 && curr == 1) {
            buzzer_on = !buzzer_on;        // toggle buzzer
            gpio_set_level(BUZZER, buzzer_on);
        }

        prev = curr;
        vTaskDelay(pdMS_TO_TICKS(20));     // debounce
    }
}

void app_main()
{
    xTaskCreate(lcd_task, "lcd_task", 4096, NULL, 5, NULL);
    xTaskCreate(buzzer_task, "buzzer_task", 2048, NULL, 5, NULL);
}