#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <hd44780.h>
#include <esp_idf_lib_helpers.h>
#include "driver/gpio.h"
#include <stdio.h>
#include <stdlib.h>  // for random cat positions

// ---------------- LCD ----------------
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

// ---------------- KEYPAD ----------------
#define NROWS 4
#define NCOLS 4
#define NOPRESS '\0'

int row_pins[] = {GPIO_NUM_4, GPIO_NUM_5, GPIO_NUM_6, GPIO_NUM_7};
int col_pins[] = {GPIO_NUM_15, GPIO_NUM_16, GPIO_NUM_17, GPIO_NUM_18};

char keypad_array[NROWS][NCOLS] =
{
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
};

// ---------------- HEART & CAT GRAPHICS ----------------
uint8_t heart_char[] = {0x1b, 0x1f, 0x0e, 0x04, 0x00, 0x00, 0x00, 0x00};
uint8_t broken_heart_char[] = {0x1b, 0x15, 0x0a, 0x04, 0x00, 0x00, 0x00, 0x00};
uint8_t cat_icon[] = {0x00, 0x00, 0x05, 0x17, 0x0f, 0x0a, 0x00, 0x00};

// ---------------- GAME VARIABLES ----------------
int state = 0;
int tasks_total = 0;
int tasks_done = 0;
int hearts = 0;
int max_hearts = 0;
int prev_state = -1;

// ---------------- INIT KEYPAD ----------------
void init_keypad()
{
    for(int i=0;i<NROWS;i++)
    {
        gpio_reset_pin(row_pins[i]);
        gpio_set_direction(row_pins[i],GPIO_MODE_OUTPUT);
        gpio_set_level(row_pins[i],1);
    }

    for(int i=0;i<NCOLS;i++)
    {
        gpio_reset_pin(col_pins[i]);
        gpio_set_direction(col_pins[i],GPIO_MODE_INPUT);
        gpio_pullup_en(col_pins[i]);
    }
}

// ---------------- SCAN KEYPAD ----------------
char scan_keypad()
{
    for(int r=0;r<NROWS;r++)
    {
        for(int i=0;i<NROWS;i++)
            gpio_set_level(row_pins[i],1);

        gpio_set_level(row_pins[r],0);

        for(int c=0;c<NCOLS;c++)
        {
            if(gpio_get_level(col_pins[c]) == 0)
                return keypad_array[r][c];
        }
    }

    return NOPRESS;
}

// ---------------- DRAW HEARTS ----------------
void draw_hearts()
{
    for(int i=0;i<max_hearts;i++)
    {
        if(i < hearts)
            hd44780_putc(&lcd,0);  // full heart
        else
            hd44780_putc(&lcd,1);  // broken heart
    }
}

// ---------------- DRAW CAT ----------------
void draw_cat(int col, int row)
{
    hd44780_upload_character(&lcd, 2, cat_icon);
    hd44780_gotoxy(&lcd, col, row);
    hd44780_putc(&lcd, 2);
}

// ---------------- LCD GAME TASK ----------------
void lcd_game_task(void *pv)
{
    vTaskDelay(pdMS_TO_TICKS(500));
    hd44780_init(&lcd);

    // load custom characters
    hd44780_upload_character(&lcd,0,heart_char);
    hd44780_upload_character(&lcd,1,broken_heart_char);

    hd44780_clear(&lcd);

    char buffer[20];

    int cat_col = 0;
    int cat_row = 3; // default row
    int cat_dir = 1; // direction

    while(1)
    {
        char key = scan_keypad();

        if(hearts <= 0 && tasks_total > 0)
            state = 6;

        if(state != prev_state)
        {
            hd44780_clear(&lcd);
            prev_state = state;
        }

        // -------- WELCOME --------
        if(state == 0)
        {
            char text[] = "Welcome!";
            int start_col = 4;

            for(int i=0;text[i]!='\0';i++)
            {
                hd44780_gotoxy(&lcd,start_col+i,1);
                hd44780_putc(&lcd,text[i]);
                vTaskDelay(pdMS_TO_TICKS(200));
            }

            vTaskDelay(pdMS_TO_TICKS(500));
            state = 1;
        }
        //-------- TASK COUNT ----------

        else if(state == 1)
        {
            hd44780_gotoxy(&lcd,0,0);
            hd44780_puts(&lcd,"How many tasks?");

            hd44780_gotoxy(&lcd,0,1);
            hd44780_puts(&lcd,"Press 1-9");

            if(key >= '1' && key <= '9')
            {
                tasks_total = key - '0';
                hearts = tasks_total;
                max_hearts = tasks_total;
                tasks_done = 0;

                state = 2;
            }
        }

        // -------- INSTRUCTIONS --------
        else if(state == 2)
        {
            hd44780_gotoxy(&lcd,0,0);
            hd44780_puts(&lcd,"Set Timer Now");

            hd44780_gotoxy(&lcd,0,1);
            hd44780_puts(&lcd,"Press A");

            hd44780_gotoxy(&lcd,0,2);
            hd44780_puts(&lcd,"To Continue");

            if(key == 'A')
                state = 3;
        }


        // -------- TASK MENU --------
        else if(state == 3)
        {
            sprintf(buffer,"%d/%d tasks",tasks_done,tasks_total);
            hd44780_gotoxy(&lcd,0,0);
            hd44780_puts(&lcd,buffer);

            hd44780_gotoxy(&lcd,0,1);
            hd44780_puts(&lcd,"Hearts:");

            hd44780_gotoxy(&lcd,0,2);
            draw_hearts();

            hd44780_gotoxy(&lcd,0,3);
            hd44780_puts(&lcd,"Press B");

            // Animate cat on bottom row
            draw_cat(cat_col, cat_row);
            cat_col += cat_dir;
            if(cat_col >= 15 || cat_col <= 0) cat_dir *= -1;

            if(key == 'B')
                state = 4;
        }

        // -------- TASK CHECK --------
        else if(state == 4)
        {
            hd44780_gotoxy(&lcd,0,0);
            hd44780_puts(&lcd,"Finished task?");

            hd44780_gotoxy(&lcd,0,1);
            hd44780_puts(&lcd,"C=Yes  D=No");

            // Animate cat
            draw_cat(cat_col, cat_row);
            cat_col += cat_dir;
            if(cat_col >= 15 || cat_col <= 0) cat_dir *= -1;

            if(key == 'C')
            {
                tasks_done++;
                state = 3;
            }
            else if(key == 'D')
            {
                hearts--;
                state = 3;
            }

            if(tasks_done >= tasks_total)
            {
                if(hearts > 0)
                    state = 5;
                else
                    state = 6;
            }
        }

        // -------- CONGRATS --------
        else if(state == 5)
        {
            int pos_index = 0;
            while(1)
            {
                hd44780_clear(&lcd);

                hd44780_gotoxy(&lcd,1,0);
                hd44780_puts(&lcd,"* CONGRATS! *");

                hd44780_gotoxy(&lcd,1,1);
                hd44780_puts(&lcd,"All Tasks Done");

                if(pos_index % 2 == 0)
                    hd44780_gotoxy(&lcd,0,2), hd44780_puts(&lcd,"*  *  *  *  *");
                else
                    hd44780_gotoxy(&lcd,0,2), hd44780_puts(&lcd,"  *  *  *  * ");

                // Animate cat randomly
                cat_col = rand() % 16;
                cat_row = rand() % 4;
                draw_cat(cat_col, cat_row);

                pos_index++;
                vTaskDelay(pdMS_TO_TICKS(400));
            }
        }

        // -------- GAME OVER --------
        else if(state == 6)
        {
            int pos_index = 0;
            while(1)
            {
                hd44780_clear(&lcd);

                hd44780_gotoxy(&lcd,3,0);
                hd44780_puts(&lcd,"GAME OVER");

                hd44780_gotoxy(&lcd,1,1);
                hd44780_puts(&lcd,"X   X   X");

                hd44780_gotoxy(&lcd,0,2);
                hd44780_puts(&lcd," X   X   X");

                hd44780_gotoxy(&lcd,2,3);
                hd44780_puts(&lcd,"X   X");

                // Animate cat on bottom row
                draw_cat(cat_col, cat_row);
                cat_col += cat_dir;
                if(cat_col >= 15 || cat_col <= 0) cat_dir *= -1;

                vTaskDelay(pdMS_TO_TICKS(400));

                hd44780_clear(&lcd);

                hd44780_gotoxy(&lcd,3,0);
                hd44780_puts(&lcd,"GAME OVER");

                hd44780_gotoxy(&lcd,0,1);
                hd44780_puts(&lcd," X   X   X");

                hd44780_gotoxy(&lcd,2,2);
                hd44780_puts(&lcd,"X   X");

                hd44780_gotoxy(&lcd,1,3);
                hd44780_puts(&lcd,"X   X   X");

                draw_cat(cat_col, cat_row);
                cat_col += cat_dir;
                if(cat_col >= 15 || cat_col <= 0) cat_dir *= -1;

                vTaskDelay(pdMS_TO_TICKS(400));
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ---------------- MAIN ----------------
void app_main()
{
    init_keypad();
    xTaskCreate(lcd_game_task,"lcd_game_task",4096,NULL,5,NULL);
}