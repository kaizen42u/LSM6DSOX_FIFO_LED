#include <stdarg.h>

#include "BuiltinColourLED.h"
#include "LSM6DSOXFIFOWrapper.h"

#define UART_CLOCK_RATE 921600 // Does not matter here since RP2040 is using USB Serial Port. (Virtual UART)
#define IIC_BUS_SPEED 400e3    // I2C bus speed in Hz. Options are: 100 kHz, 400 kHz, and 1.0 Mhz.
#define PRINT_BUFFER_SIZE 128  // Increase this number if you see the output gets truncated

static LSM6DSOXFIFO IMU = LSM6DSOXFIFO(Wire, LSM6DSOX_I2C_ADD_L); // IMU on the I2C bus
static BuiltinColourLED ColourLED;

static int log(const char *format, ...);

static int log(const char *format, ...)
{
    static char print_buffer[PRINT_BUFFER_SIZE] = {'\0'};
    int ret_val;
    va_list va;
    va_start(va, format);
    ret_val = vsnprintf(print_buffer, PRINT_BUFFER_SIZE, format, va);
    va_end(va);
    Serial.write(print_buffer);
    return ret_val;
}

static int IMULoggingCB(const char *str)
{
    return log("%s", str);
}

static void IMUDataReadyCB([[maybe_unused]] LSM6DSOXFIFO::imu_data_t *data)
{
    IMU.print(data);
}

void setup()
{
    ColourLED.enable();
    ColourLED.setRGB(100, 0, 0);
    
    Serial.begin(UART_CLOCK_RATE);

    // Comment out this line to skip waiting for serial:
    while (!Serial)
    {
        static uint32_t last_millis = millis();
        const static uint16_t delta = 600;
        static bool state = true;
        if (millis() - last_millis >= delta) {
          ColourLED.setRGB(0, state ? 100 : 0, 0);
          state = !state;
          last_millis += delta;
        }
    }

    ColourLED.setRGB(0, 0, 100);

    // I2C, fast mode
    Wire.begin();
    Wire.setClock(IIC_BUS_SPEED);

    IMU.registerLoggingCallback(IMULoggingCB);
    IMU.registerDataReadyCallback(IMUDataReadyCB);
    // Initialize sensors
    if (!IMU.initialize())
    {
        log("Failed to initialize IMU\n");
        while (1)
            ; // Halt execution
    }

    ColourLED.setRGB(100, 100, 100);
    log("Starting...\n");
}

void loop()
{
    IMU.update();

    // Show a rainbow LED effect
    static uint32_t last_led_millis = millis();
    const static uint8_t delta = 10;
    if (millis() - last_led_millis >= delta) {
      static uint16_t hue = 0;
      ColourLED.setHSV(hue, 100, 50);
      hue = (hue + 1) % 360;
      last_led_millis += delta;
    }

    delay(1); // FIFO continues batching while the MCU "sleeps"
}
