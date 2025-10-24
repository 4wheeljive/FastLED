#ifdef __AVR__

#include "io_avr.h"



#ifdef ARDUINO_H
#include "Arduino.h"
#endif

namespace fl {

// Helper functions for AVR UART I/O
#ifdef UART_UDR
static void avr_uart_putchar(char c) {
    // Wait for empty transmit buffer
    while (!(UART_UCSRA & (1 << UART_UDRE)));
    // Put data into buffer, sends the data
    UART_UDR = c;
}

static int avr_uart_available() {
    // Check if data is available in receive buffer
    return (UART_UCSRA & (1 << UART_RXC)) ? 1 : 0;
}

static int avr_uart_read() {
    // Check if data is available
    if (UART_UCSRA & (1 << UART_RXC)) {
        return UART_UDR;
    }
    return -1;
}
#endif

// Print functions
void print_avr(const char* str) {
    if (!str) return;

#ifdef UART_UDR
    // AVR: Use native UART registers first (if UART is initialized)
    const char* p = str;

    // Check if UART seems to be initialized (basic check)
    if (UART_UCSRA != 0xFF) {  // 0xFF usually indicates uninitialized
        while (*p) {
            avr_uart_putchar(*p++);
        }
    } else {
        // Fallback to Arduino Serial if available
        #ifdef ARDUINO_H
        if (Serial) {
            Serial.print(str);
        }
        #endif
    }
#else
    // Fallback to Arduino Serial if UART registers not available
    #ifdef ARDUINO_H
    if (Serial) {
        Serial.print(str);
    }
    #endif
#endif
}

void println_avr(const char* str) {
    if (!str) return;
    print_avr(str);
    print_avr("\n");
}

// Input functions
int available_avr() {
#ifdef UART_UDR
    // AVR: Use native UART registers first (if UART is initialized)
    // Check if UART seems to be initialized (basic check)
    if (UART_UCSRA != 0xFF) {  // 0xFF usually indicates uninitialized
        return avr_uart_available();
    } else {
        // Fallback to Arduino Serial if available
        #ifdef ARDUINO_H
        if (Serial) {
            return Serial.available();
        }
        #endif
    }
#else
    // Fallback to Arduino Serial if UART registers not available
    #ifdef ARDUINO_H
    if (Serial) {
        return Serial.available();
    }
    #endif
#endif
    return 0;
}

int read_avr() {
#ifdef UART_UDR
    // AVR: Use native UART registers first (if UART is initialized)
    // Check if UART seems to be initialized (basic check)
    if (UART_UCSRA != 0xFF) {  // 0xFF usually indicates uninitialized
        return avr_uart_read();
    } else {
        // Fallback to Arduino Serial if available
        #ifdef ARDUINO_H
        if (Serial && Serial.available()) {
            return Serial.read();
        }
        #endif
    }
#else
    // Fallback to Arduino Serial if UART registers not available
    #ifdef ARDUINO_H
    if (Serial && Serial.available()) {
        return Serial.read();
    }
    #endif
#endif
    return -1;
}

} // namespace fl

#endif // __AVR__
