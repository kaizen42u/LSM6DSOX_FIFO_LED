#include <stdarg.h> // Include standard library for variable argument lists

#include "BuiltinColourLED.h"    // Include header for built-in RGB LED control
#include "LSM6DSOXFIFOWrapper.h" // Include header for LSM6DSOX IMU FIFO wrapper

#define UART_CLOCK_RATE 921600 // Baud rate for UART communication, irrelevant for USB Serial Port (Virtual UART)
#define IIC_BUS_SPEED 400e3    // I2C bus speed in Hz (100 kHz, 400 kHz, or 1.0 MHz)
#define PRINT_BUFFER_SIZE 128  // Size of the buffer for printing messages; adjust if output gets truncated

static LSM6DSOXFIFO IMU = LSM6DSOXFIFO(Wire, LSM6DSOX_I2C_ADD_L); // Instantiate the IMU on the I2C bus
static BuiltinColourLED ColourLED;                                // Instantiate the built-in RGB LED

// Function to log messages
static int log(const char *format, ...);

// Definition of the log function
static int log(const char *format, ...)
{
    static char print_buffer[PRINT_BUFFER_SIZE] = {'\0'}; // Buffer for formatted messages
    int ret_val;
    va_list va;
    va_start(va, format);
    ret_val = vsnprintf(print_buffer, PRINT_BUFFER_SIZE, format, va); // Format the message into the buffer
    va_end(va);
    Serial.write(print_buffer); // Format the message into the buffer
    return ret_val;             // Returns the number of characters printed
}

// Callback function for IMU logging
static int IMULoggingCB(const char *str)
{
    return log("%s", str);
}

// Callback function for when IMU data is ready
static void IMUDataReadyCB([[maybe_unused]] LSM6DSOXFIFO::imu_data_t *data)
{
    IMU.print(data); // Print the IMU data
}

// Setup function, runs once at startup
void setup()
{
    ColourLED.enable();          // Enable the built-in RGB LED
    ColourLED.setRGB(100, 0, 0); // Set the LED color to red

    Serial.begin(UART_CLOCK_RATE); // Initialize serial communication at the specified baud rate

    // Comment out this line to skip waiting for serial:
    while (!Serial)
    {
        static uint32_t last_millis = millis();
        const static uint16_t delta = 600; // Time interval for LED blinking
        static bool state = true;
        if (millis() - last_millis >= delta)
        {
            ColourLED.setRGB(0, state ? 100 : 0, 0); // Blink the LED between off and green
            state = !state;
            last_millis += delta;
        }
    }

    ColourLED.setRGB(0, 0, 100); // Set the LED color to blue

    Wire.begin();                 // Initialize I2C communication
    Wire.setClock(IIC_BUS_SPEED); // Set the I2C bus speed

    IMU.registerLoggingCallback(IMULoggingCB);     // Register the logging callback for the IMU
    IMU.registerDataReadyCallback(IMUDataReadyCB); // Register the data-ready callback for the IMU

    // Initialize the IMU sensor
    if (!IMU.initialize())
    {
        log("Failed to initialize IMU\n");
        while (1)
            ; // Halt execution if initialization fails
    }

    ColourLED.setRGB(100, 100, 100); // Set the LED color to white
    log("Starting...\n");
}

// Loop function, runs continuously
void loop()
{
    IMU.update(); // Update the IMU data

    // Create a rainbow LED effect
    static uint32_t last_led_millis = millis();
    const static uint8_t delta = 10; // Time interval for LED color change
    if (millis() - last_led_millis >= delta)
    {
        static uint16_t hue = 0;
        ColourLED.setHSV(hue, 100, 50); // Set the LED color based on hue
        hue = (hue + 1) % 360;          // Increment hue and wrap around at 360
        last_led_millis += delta;
    }

    delay(1); // Short delay to demo FIFO batching while MCU "sleeps" or busy doing other things
}
