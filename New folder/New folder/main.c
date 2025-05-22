#include <lpc21xx.h>

#define LEFT_LED     (1 << 17)  // as per my microcontroller
#define RIGHT_LED    (1 << 19)
#define LEFT_SW      (1 << 14)
#define RIGHT_SW     (1 << 16)

int left_active = 0;
int right_active = 0;
int hazard_active = 0;

void timer0_delay_ms(unsigned int ms) {
    T0TCR = 0x02;             // Reset Timer
    T0PR = 1500 - 1;         // Prescaler for 1ms (PCLK = 15MHz)
    T0TCR = 0x01;             // Enable Timer

    while (T0TC < ms);        // Wait until delay ends

    T0TCR = 0x00;             // Stop Timer
    T0TC = 0;                 // Reset counter
}

void UART0_Init() {
    PINSEL0 |= (1 << 0) | (1 << 2);  // P0.0 = TXD0, P0.1 = RXD0
    U0LCR = 0x83;
    U0DLM = 0;
    U0DLL = 97;                      // 9600 baud @ 15MHz PCLK
    U0LCR = 0x03;
}

void UART0_SendChar(char c) {
    while (!(U0LSR & 0x20));
    U0THR = c;
}

void UART0_SendString(const char *str) {
    while (*str) {
        UART0_SendChar(*str++);
    }
}

void turn_off_all_leds() {
    IOSET0 = LEFT_LED | RIGHT_LED;  // Active-low: set = OFF
}

int main() {
    IODIR0 |= LEFT_LED | RIGHT_LED;
    IODIR0 &= ~(LEFT_SW | RIGHT_SW);  // Switches = input
    UART0_Init();

    while (1) {
        int left_pressed = !(IOPIN0 & LEFT_SW);
        int right_pressed = !(IOPIN0 & RIGHT_SW);

        // --- Hazard Mode ---
        if (left_pressed && right_pressed) {
            timer0_delay_ms(1000);  // Wait for 1 sec hold
            if (!(IOPIN0 & LEFT_SW) && !(IOPIN0 & RIGHT_SW)) {
                if (!hazard_active) {
                    UART0_SendString("Hazard Mode ON\n");
                    hazard_active = 1;
                    left_active = right_active = 0;
                }

                IOCLR0 = LEFT_LED | RIGHT_LED;
                timer0_delay_ms(300);
                IOSET0 = LEFT_LED | RIGHT_LED;
                timer0_delay_ms(300);
                continue;
            }
        }

        // --- Left Indicator ---
        else if (left_pressed && !right_pressed) {
            timer0_delay_ms(1000);  // Wait to confirm hold
            if (!(IOPIN0 & LEFT_SW) && (IOPIN0 & RIGHT_SW)) {
                if (!left_active) {
                    UART0_SendString("Left Indicator ON\n");
                    left_active = 1;
                    right_active = hazard_active = 0;
                }

                IOCLR0 = LEFT_LED;
                IOSET0 = RIGHT_LED;
                timer0_delay_ms(300);
                IOSET0 = LEFT_LED;
                timer0_delay_ms(300);
                continue;
            }
        }

        // --- Right Indicator ---
        else if (right_pressed && !left_pressed) {
            timer0_delay_ms(1000);  // Wait to confirm hold
            if (!(IOPIN0 & RIGHT_SW) && (IOPIN0 & LEFT_SW)) {
                if (!right_active) {
                    UART0_SendString("Right Indicator ON\n");
                    right_active = 1;
                    left_active = hazard_active = 0;
                }

                IOCLR0 = RIGHT_LED;
                IOSET0 = LEFT_LED;
                timer0_delay_ms(300);
                IOSET0 = RIGHT_LED;
                timer0_delay_ms(300);
                continue;
            }
        }

        // --- Nothing Pressed: Turn OFF if any mode active ---
        if (left_active) {
            UART0_SendString("Left Indicator OFF\n");
            left_active = 0;
        }
        if (right_active) {
            UART0_SendString("Right Indicator OFF\n");
            right_active = 0;
        }
        if (hazard_active) {
            UART0_SendString("Hazard Mode OFF\n");
            hazard_active = 0;
        }

        turn_off_all_leds();
        timer0_delay_ms(100);  // Idle delay
    }
}
