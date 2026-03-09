#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <hd44780.h>
#include <esp_idf_lib_helpers.h>

void app_main()
{
    vTaskDelay(pdMS_TO_TICKS(500));   // allow LCD to power up

    hd44780_t lcd =
    {
        .write_cb = NULL,
        .font = HD44780_FONT_5X8,
        .lines = 4,
        .pins = {
            .rs = GPIO_NUM_4,
            .e  = GPIO_NUM_5,
            .d4 = GPIO_NUM_6,
            .d5 = GPIO_NUM_7,
            .d6 = GPIO_NUM_9,
            .d7 = GPIO_NUM_10,
            .bl = HD44780_NOT_USED
        }
    };

    hd44780_init(&lcd);
    vTaskDelay(pdMS_TO_TICKS(200));
    hd44780_clear(&lcd);

    // --- Custom character for the RIGHT side (slot 0) ---
    uint8_t customCharRight[8] = {
        0x00,
        0x1B,
        0x1F,
        0x0E,
        0x04,
        0x00,
        0x00,
        0x00
    };

    // --- Custom character for the LEFT side (slot 1) ---
    uint8_t customCharLeft[8] = {
        0x00,
        0x1B,
        0x1F,
        0x1F,
        0x0E,
        0x00,
        0x00,
        0x00
    };

    // Upload both characters
    hd44780_upload_character(&lcd, 0, customCharRight);  // prints as 0x08
    hd44780_upload_character(&lcd, 1, customCharLeft);   // prints as 0x09

    // --- Print LEFT character at column 0 ---
    hd44780_gotoxy(&lcd, 0, 0);
    hd44780_putc(&lcd, 0x09);   // slot 1

    // --- Print RIGHT character 5 times, flush right ---
    int start_col = 16 - 5;     // 16 columns wide → start at column 11
    hd44780_gotoxy(&lcd, start_col, 0);

    for (int i = 0; i < 5; i++) {
        hd44780_putc(&lcd, 0x08);   // slot 0
    }

    //Row 2
    hd44780_gotoxy(&lcd, 0, 1);     // row index 1 = second row
    hd44780_puts(&lcd, "In Progress");

}