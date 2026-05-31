#include <string>
#include <cstring>
#include <math.h>

#include "pico/stdlib.h"
#include "hardware/i2c.h"

#include "ssd1306_font.h"

// Display dimension definitions
#define SSD1306_HEIGHT  32
#define SSD1306_WIDTH   128

#define SSD1306_I2C_ADDR 0x3C
#define SSD1306_I2C_CLK 400

// Command definitions
#define SSD1306_SET_MEM_MODE        0x20
#define SSD1306_SET_COL_ADDR        0x21
#define SSD1306_SET_PAGE_ADDR       0x22
#define SSD1306_SET_HORIZ_SCROLL    0x26
#define SSD1306_SET_SCROLL          0x2E

#define SSD1306_SET_DISP_START_LINE 0x40

#define SSD1306_SET_CONTRAST        0x81
#define SSD1306_SET_CHARGE_PUMP     0x8D

#define SSD1306_SET_SEG_REMAP           0xA0
#define SSD1306_SET_ENTIRE_ON           0xA4
#define SSD1306_SET_ALL_ON              0xA5
#define SSD1306_SET_NORM_DISP           0xA6
#define SSD1306_SET_INV_DISP            0xA7
#define SSD1306_SET_MUX_RATIO           0xA8
#define SSD1306_SET_DISP                0xAE
#define SSD1306_SET_COM_OUT_DIR         0xC0
#define SSD1306_SET_COM_OUT_DIR_FLIP    0xC0

#define SSD1306_SET_DISP_OFFSET     0xD3
#define SSD1306_SET_DISP_CLK_DIV    0xD5
#define SSD1306_SET_PRECHARGE       0xD9
#define SSD1306_SET_COM_PIN_CFG     0xDA
#define SSD1306_SET_VCOM_DESEL      0xDB

#define SSD1306_PAGE_HEIGHT 8
#define SSD1306_NUM_PAGES   (SSD1306_HEIGHT / SSD1306_PAGE_HEIGHT)
#define SSD1306_BUF_LEN     (SSD1306_NUM_PAGES * SSD1306_WIDTH)

#define SSD1306_WRITE_MODE  0xFE
#define SSD1306_READ_MODE   0xFF

// I2C port definitions
#define I2C_PORT i2c1
#define I2C_SDA 26
#define I2C_SCL 27

struct render_area {
    uint8_t start_col;
    uint8_t end_col;
    uint8_t start_page;
    uint8_t end_page;

    int buflen;
}frame_area;

uint8_t buffer[SSD1306_BUF_LEN];

void calc_render_area_buflen(struct render_area *area) {
    area->buflen = (area->end_col - area->start_col + 1) * (area->end_page - area->start_page + 1);
}

void SSD1306_send_cmd(uint8_t cmd) {
    uint8_t buf[2] = {0x80, cmd};
    i2c_write_blocking(I2C_PORT, SSD1306_I2C_ADDR, buf, 2, false);
}

void SSD1306_send_cmd_list(uint8_t *buf, int num) {
    for (int i=0;i<num;i++)
        SSD1306_send_cmd(buf[i]);
}

void SSD1306_send_buf(uint8_t buf[], int buflen) {
    uint8_t temp_buf[buflen + 1];

    temp_buf[0] = 0x40;
    memcpy(temp_buf+1, buf, buflen);

    i2c_write_blocking(I2C_PORT, SSD1306_I2C_ADDR, temp_buf, buflen + 1, false);
}

void render(uint8_t *buf, struct render_area *area) {
    uint8_t cmds[] = {
        SSD1306_SET_COL_ADDR,
        area->start_col,
        area->end_col,
        SSD1306_SET_PAGE_ADDR,
        area->start_page,
        area->end_page
    };

    SSD1306_send_cmd_list(cmds, count_of(cmds));
    SSD1306_send_buf(buf, area->buflen);
}

void SSD1306_init() {
    i2c_init(I2C_PORT, SSD1306_I2C_CLK * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);

    uint8_t cmds[] = {
        SSD1306_SET_DISP,               // set display off
        /* memory mapping */
        SSD1306_SET_MEM_MODE,           // set memory address mode 0 = horizontal, 1 = vertical, 2 = page
        0x00,                           // horizontal addressing mode
        /* resolution and layout */
        SSD1306_SET_DISP_START_LINE,    // set display start line to 0
        SSD1306_SET_SEG_REMAP | 0x01,   // set segment re-map, column address 127 is mapped to SEG0
        SSD1306_SET_MUX_RATIO,          // set multiplex ratio
        SSD1306_HEIGHT - 1,             // Display height - 1
        SSD1306_SET_COM_OUT_DIR | 0x08, // set COM output scan direction. Scan from bottom up, COM[N-1] to COM0
        SSD1306_SET_DISP_OFFSET,        // set display offset
        0x00,                           // no offset
        SSD1306_SET_COM_PIN_CFG,        // set COM pins hardware configuration. Board specific magic number.
                                        // 0x02 Works for 128x32, 0x12 Possibly works for 128x64. Other options 0x22, 0x32
#if ((SSD1306_WIDTH == 128) && (SSD1306_HEIGHT == 32))
        0x02,
#elif ((SSD1306_WIDTH == 128) && (SSD1306_HEIGHT == 64))
        0x12,
#else
        0x02,
#endif
        /* timing and driving scheme */
        SSD1306_SET_DISP_CLK_DIV,       // set display clock divide ratio
        0x80,                           // div ratio of 1, standard freq
        SSD1306_SET_PRECHARGE,          // set pre-charge period
        0xF1,                           // Vcc internally generated on our board
        SSD1306_SET_VCOM_DESEL,         // set VCOMH deselect level
        0x30,                           // 0.83xVcc
        /* display */
        SSD1306_SET_CONTRAST,           // set contrast control
        0xFF,
        SSD1306_SET_ENTIRE_ON,          // set entire display on to follow RAM content
        SSD1306_SET_NORM_DISP,           // set normal (not inverted) display
        SSD1306_SET_CHARGE_PUMP,        // set charge pump
        0x14,                           // Vcc internally generated on our board
        SSD1306_SET_SCROLL | 0x00,      // deactivate horizontal scrolling if set. This is necessary as memory writes will corrupt if scrolling was enabled
        SSD1306_SET_DISP | 0x01, // turn display on
    };

    SSD1306_send_cmd_list(cmds, count_of(cmds));

    frame_area = {
        start_col: 0,
        end_col : SSD1306_WIDTH - 1,
        start_page : 0,
        end_page : SSD1306_NUM_PAGES - 1
    };

    calc_render_area_buflen(&frame_area);

    memset(buffer, 0, SSD1306_BUF_LEN);
    render(buffer, &frame_area);
}

void SSD1306_scroll(bool on) {
    uint8_t scroll = SSD1306_SET_SCROLL | (on ? uint8_t{1} : uint8_t{0});

    uint8_t cmds[] = {
        SSD1306_SET_HORIZ_SCROLL | 0x00,
        0x00, // dummy byte
        0x00, // start page 0
        0x00, // time interval
        SSD1306_NUM_PAGES - 1, // end page
        0x00, // dummy byte
        0xFF, // dummy byte
        scroll // Start/stop scrolling
    };

    SSD1306_send_cmd_list(cmds, count_of(cmds));
}

static void SetPixel(uint8_t *buf, int x,int y, bool on) {
    assert(x >= 0 && x < SSD1306_WIDTH && y >=0 && y < SSD1306_HEIGHT);

    const int BytesPerRow = SSD1306_WIDTH ; // x pixels, 1bpp, but each row is 8 pixel high, so (x / 8) * 8

    int byte_idx = (y / 8) * BytesPerRow + x;
    uint8_t byte = buf[byte_idx];

    if (on)
        byte |=  1 << (y % 8);
    else
        byte &= ~(1 << (y % 8));

    buf[byte_idx] = byte;
}

static void DrawLine(uint8_t *buf, int x0, int y0, int x1, int y1, bool on) {

    int dx =  abs(x1-x0);
    int sx = x0<x1 ? 1 : -1;
    int dy = -abs(y1-y0);
    int sy = y0<y1 ? 1 : -1;
    int err = dx+dy;
    int e2;

    while (true) {
        SetPixel(buf, x0, y0, on);
        if (x0 == x1 && y0 == y1)
            break;
        e2 = 2*err;

        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

static inline int GetFontIndex(uint8_t ch) {
    if (ch >= 'A' && ch <='Z') {
        return  ch - 'A' + 1;
    }
    else if (ch >= '0' && ch <='9') {
        return  ch - '0' + 27;
    }
    else return  0; // Not got that char so space.
}

static void WriteChar(uint8_t *buf, int16_t x, int16_t y, uint8_t ch) {
    if (x > SSD1306_WIDTH - 8 || y > SSD1306_HEIGHT - 8)
        return;

    y = y/8;

    ch = toupper(ch);
    int idx = GetFontIndex(ch);
    int fb_idx = y * 128 + x;

    for (int i=0;i<8;i++) {
        buf[fb_idx++] = font[idx * 8 + i];
    }
}

static void WriteString(uint8_t *buf, int16_t x, int16_t y, const char *str) {
    if (x > SSD1306_WIDTH - 8 || y > SSD1306_HEIGHT - 8)
        return;

    while (*str) {
        WriteChar(buf, x, y, *str++);
        x+=8;
    }
}

// Stepper driver pin definitions
#define STP_PIN 14
#define DIR_PIN 15
#define MS1_PIN 11
#define MS2_PIN 12
#define MS3_PIN 13

// Steps per rotation for the NEMA17
#define STEPS_PER_REV 200.0f
#define MAX_RPM       600.0f

typedef enum {
    MICROSTEP_FULL      = 1,
    MICROSTEP_HALF      = 2,
    MICROSTEP_QUARTER   = 4,
    MICROSTEP_EIGHTH    = 8,
    MICROSTEP_SIXTEENTH = 16
} microstep_mode_t;

static uint32_t step_delay_us = 0;
static microstep_mode_t current_microstep = MICROSTEP_FULL;

static float current_speed_rpm = 0.0f;
static float target_speed_rpm  = 0.0f;

static float accel_rpm_per_s = 120.0f;

static absolute_time_t last_ramp_update;

static uint32_t rpm_to_step_delay_us(float rpm, microstep_mode_t microstep) {
    if (rpm <= 0.0f) {
        return 0;
    }

    if (rpm > MAX_RPM) {
        rpm = MAX_RPM;
    }

    float steps_per_rev = STEPS_PER_REV * (float)microstep;
    float steps_per_sec = (rpm * steps_per_rev) / 60.0f;

    if (steps_per_sec < 1.0f) {
        steps_per_sec = 1.0f;
    }

    return (uint32_t)(1000000.0f / steps_per_sec);
}

static void stepper_apply_speed(void) {
    step_delay_us = rpm_to_step_delay_us(current_speed_rpm, current_microstep);
}

static void stepper_set_microstep(microstep_mode_t mode) {
    current_microstep = mode;

    switch (mode) {
        case MICROSTEP_FULL:
            gpio_put(MS1_PIN, 0);
            gpio_put(MS2_PIN, 0);
            gpio_put(MS3_PIN, 0);
            break;

        case MICROSTEP_HALF:
            gpio_put(MS1_PIN, 1);
            gpio_put(MS2_PIN, 0);
            gpio_put(MS3_PIN, 0);
            break;

        case MICROSTEP_QUARTER:
            gpio_put(MS1_PIN, 0);
            gpio_put(MS2_PIN, 1);
            gpio_put(MS3_PIN, 0);
            break;

        case MICROSTEP_EIGHTH:
            gpio_put(MS1_PIN, 1);
            gpio_put(MS2_PIN, 1);
            gpio_put(MS3_PIN, 0);
            break;

        case MICROSTEP_SIXTEENTH:
            gpio_put(MS1_PIN, 1);
            gpio_put(MS2_PIN, 1);
            gpio_put(MS3_PIN, 1);
            break;
    }

    /* Same shaft RPM, new pulse/rev => recompute delay */
    stepper_apply_speed();
}

static void stepper_init(void) {
    gpio_init(STP_PIN);
    gpio_set_dir(STP_PIN, GPIO_OUT);
    gpio_put(STP_PIN, 0);

    gpio_init(DIR_PIN);
    gpio_set_dir(DIR_PIN, GPIO_OUT);
    gpio_put(DIR_PIN, 0);

    gpio_init(MS1_PIN);
    gpio_set_dir(MS1_PIN, GPIO_OUT);

    gpio_init(MS2_PIN);
    gpio_set_dir(MS2_PIN, GPIO_OUT);

    gpio_init(MS3_PIN);
    gpio_set_dir(MS3_PIN, GPIO_OUT);

    current_speed_rpm   = 0.0f;
    target_speed_rpm    = 0.0f;
    step_delay_us       = 0;

    stepper_set_microstep(MICROSTEP_FULL);
    last_ramp_update = get_absolute_time();
}

static void stepper_set_direction(bool clockwise) {
    gpio_put(DIR_PIN, clockwise ? 1 : 0);
}

static void stepper_set_speed_rpm(float rpm) {
    if (rpm < 0.0f) {
        rpm = 0.0f;
    }

    if (rpm > MAX_RPM) {
        rpm = MAX_RPM;
    }

    target_speed_rpm = rpm;
}

static void stepper_set_acceleration_rpm(float rpm_per_s) {
    if (rpm_per_s < 1.0f) {
        rpm_per_s = 1.0f;
    }

    accel_rpm_per_s = rpm_per_s;
}

static void stepper_update_speed_ramp(void) {
    absolute_time_t now = get_absolute_time();
    int64_t dt_us = absolute_time_diff_us(last_ramp_update, now);
    last_ramp_update = now;

    if (dt_us <= 0) {
        return;
    }

    float dt_s = (float)dt_us / 1000000.0f;
    float max_delta_rpm = accel_rpm_per_s * dt_s;
    float error = target_speed_rpm - current_speed_rpm;

    if (fabsf(error) <= max_delta_rpm) {
        current_speed_rpm = target_speed_rpm;
    } else if (error > 0.0f) {
        current_speed_rpm += max_delta_rpm;
    } else {
        current_speed_rpm -= max_delta_rpm;
    }

    stepper_apply_speed();
}

static void stepper_run(void) {
    stepper_update_speed_ramp();

    if (step_delay_us == 0) {
        sleep_us(1000);
        return;
    }

    gpio_put(STP_PIN, 1);
    sleep_us(2);
    gpio_put(STP_PIN, 0);

    if (step_delay_us > 2) {
        sleep_us(step_delay_us - 2);
    }
}

// Define the type of edge trigger
#define GPIO_IRQ_EDGE GPIO_IRQ_EDGE_RISE

// Encoder pin definitions
#define ENC_CC 9
#define ENC_CW 10
#define ENC_SW 8

void update_display() {
    std::string step_mode;

    switch (current_microstep) {
        case MICROSTEP_FULL:
            step_mode = "Full";
            break;
        case MICROSTEP_HALF:
            step_mode = "Half";
        break;
        case MICROSTEP_QUARTER:
            step_mode = "Quarter";
        break;
        case MICROSTEP_EIGHTH:
            step_mode = "Eighth";
        break;
        case MICROSTEP_SIXTEENTH:
            step_mode = "Sixteenth";
        break;
        default:
            step_mode = "Error";
            break;
    }

    std::string text[] = { "STEPPER DRIVER", "Mode: " + step_mode, "Speed: " + std::to_string((int)target_speed_rpm) };

    memset(buffer, 0, SSD1306_BUF_LEN);
    render(buffer, &frame_area);

    int y = 0;
    for (uint i = 0 ; i < count_of(text); i++) {
        WriteString(buffer, 5, y, text[i].c_str());
        y+=8;
    }
    render(buffer, &frame_area);

    printf("Update display!\n");
}

void gpio_callback(uint gpio, uint32_t events) {
    // Check for clockwise operartion
    if (gpio == ENC_CW) {
        if (!gpio_get(ENC_CC)){
            stepper_set_speed_rpm(target_speed_rpm - 10.0f);

            update_display();
        }

        return;
    }

    // Check for counterclockwise operartion
    if (gpio == ENC_CC) {
        if (!gpio_get(ENC_CW)){
            stepper_set_speed_rpm(target_speed_rpm + 10.0f);

            update_display();
        }

        return;
    }

    // Check for button operation
    if (gpio == ENC_SW) {
        switch (current_microstep) {
            case MICROSTEP_FULL:
                stepper_set_microstep(MICROSTEP_HALF);
                break;
            case MICROSTEP_HALF:
                stepper_set_microstep(MICROSTEP_QUARTER);
            break;
            case MICROSTEP_QUARTER:
                stepper_set_microstep(MICROSTEP_EIGHTH);
            break;
            case MICROSTEP_EIGHTH:
                stepper_set_microstep(MICROSTEP_SIXTEENTH);
            break;
            case MICROSTEP_SIXTEENTH:
                stepper_set_microstep(MICROSTEP_FULL);
            break;
            default:
                stepper_set_microstep(MICROSTEP_FULL);
                break;
        }

        stepper_set_speed_rpm(target_speed_rpm);

        update_display();

        return;
    }
}

static void set_gpio_handlers() {
    gpio_init(ENC_CC);
    gpio_set_dir(ENC_CC, GPIO_IN);

    gpio_init(ENC_CW);
    gpio_set_dir(ENC_CW, GPIO_IN);

    gpio_init(ENC_SW);
    gpio_set_dir(ENC_SW, GPIO_IN);

    gpio_set_irq_enabled_with_callback(ENC_CC, GPIO_IRQ_EDGE, true, gpio_callback);
    gpio_set_irq_enabled_with_callback(ENC_CW, GPIO_IRQ_EDGE, true, gpio_callback);
    gpio_set_irq_enabled_with_callback(ENC_SW, GPIO_IRQ_EDGE, true, gpio_callback);
}

int main() {
    stdio_init_all();

    SSD1306_init();

    set_gpio_handlers();

    stepper_init();

    stepper_set_direction(true);
    stepper_set_microstep(MICROSTEP_HALF);
    stepper_set_speed_rpm(60.0f);

    update_display();

    while (1) {
        stepper_run();
    }
}
