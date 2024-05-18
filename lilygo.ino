#ifndef BOARD_HAS_PSRAM
#error "Please enable PSRAM, Arduino IDE -> tools -> PSRAM -> OPI !!!"
#endif

/* *** Includes ********************************************* */
#include <Arduino.h>
#include "epd_driver.h"
#include "firasans.h"
#include "Button2.h"

/* *** Hardware ********************************************* */
Button2 btn1(BUTTON_1);

/* *** Classes ********************************************** */
struct Cursor
{
    int x;
    int y;
};

class StringBuffer
{
public:
    char *head;
    char *curr;
    size_t size;

    StringBuffer(size_t size)
    {
        this->size = size;
        this->head = (char *)ps_calloc(sizeof(*this->head), size);
        this->curr = this->head;
    }

    const inline size_t curr_off(void)
    {
        return this->curr - this->head;
    }

    inline void insert_char(char c)
    {
        *(this->curr++) = c;
    }
};

/* *** Contsants ******************************************** */
const Rect_t text_area = {
    .x = 10,
    .y = 20,
    .width = EPD_WIDTH - 20,
    .height = EPD_HEIGHT - 10};

const char *text[] = {
    "CHAPTER I. Down the Rabbit-Hole Alice was beginning to get very tired of sitting by her sister on the bank, and of having nothing to do:",
    "once or twice she had peeped into the book her sister was reading, but it had no pictures or conversations in it,",
    "“and what is the use of a book,” thought Alice “without pictures or conversations?”"};

const int chars_in_line = 47;

/* *** Globals ********************************************** */
uint8_t *framebuffer;
Cursor g_cursor = {
    .x = 20,
    .y = 60};
int state = 0;
int vref = 1100;

/* *** Functions ******************************************** */
void reset_global_curser(void)
{
    g_cursor.x = 20;
    g_cursor.y = 60;
}

inline bool is_next_char_line_break(const size_t i)
{
    return i % chars_in_line == chars_in_line - 1;
}

inline bool is_char_new_line(const char c)
{
    return NULL != strchr(" \n", c);
}

inline bool is_char_space(const char c)
{
    return NULL != strchr(" \t", c);
}

inline bool is_char_null_terminator(const char c)
{
    return c == '\0';
}

inline bool is_char_word_separator(const char c)
{
    return NULL != strchr(" .,;:'", c) ||
           is_char_new_line(c) ||
           is_char_null_terminator(c) ||
           is_char_space(c);
}

void render_text(char *text)
{
    StringBuffer string_buffer = StringBuffer(300);
    reset_global_curser();
    int length = strlen(text);

    for (int i = 0; i < length; i++)
    {
        if (is_next_char_line_break(i))
        {
            if (is_char_word_separator(text[i + 1]))
            {
                string_buffer.insert_char(text[i]);
                string_buffer.insert_char(text[++i]);
                string_buffer.insert_char('\n');
            }
            else
            {
                string_buffer.insert_char('-');
                string_buffer.insert_char('\n');
                string_buffer.insert_char(text[i]);
            }
        }
        else
        {
            string_buffer.insert_char(text[i]);
        }
    }
    string_buffer.insert_char('\0');

    write_string((GFXfont *)&FiraSans, string_buffer.head, &g_cursor.x, &g_cursor.y, NULL);
}

void displayInfo(void)
{
    epd_poweron();
    epd_clear_area(text_area);
    render_text((char *)text[0]);
    epd_poweroff();
}

void enter_deep_sleep(void)
{
    delay(1000);
    epd_clear_area(text_area);
    write_string((GFXfont *)&FiraSans, "DeepSleep", &g_cursor.x, &g_cursor.y, NULL);
    epd_poweroff_all();
#if defined(CONFIG_IDF_TARGET_ESP32)
    // Set to wake up by GPIO39
    esp_sleep_enable_ext1_wakeup(GPIO_SEL_39, ESP_EXT1_WAKEUP_ANY_LOW);
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
    esp_sleep_enable_ext1_wakeup(GPIO_SEL_21, ESP_EXT1_WAKEUP_ANY_LOW);
#endif
    esp_deep_sleep_start();
}

/* *** Events *********************************************** */
void buttonPressed(Button2 &b)
{
    state++;
    displayInfo();
}

/* *** Setup ************************************************ */
uint8_t *get_new_frame_buffer(void)
{
    uint8_t *local_buffer = NULL;

    local_buffer = (uint8_t *)ps_calloc(sizeof(uint8_t), EPD_WIDTH * EPD_HEIGHT / 2);
    if (!local_buffer)
    {
        Serial.println("alloc memory failed !!!");
        while (1)
            ;
    }
    memset(local_buffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);

    return local_buffer;
}

void setup()
{
    Serial.begin(115200);
    epd_init();
    framebuffer = get_new_frame_buffer();

    btn1.setPressedHandler(buttonPressed);

    // epd_clear();
    displayInfo();
    // epd_draw_grayscale_image(epd_full_screen(), framebuffer);
    // epd_poweroff();
}

void loop()
{
    btn1.loop();
    delay(2);
}