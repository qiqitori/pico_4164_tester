#include <stdio.h>
#include "pico/stdlib.h"

#define D           0
#define WRITE       1
#define RAS         2
#define A0          3
#define A2          4
#define A1          5

#define A7          16
#define A5          17
#define A4          18
#define A3          19
#define A6          20
#define Q           21
#define CAS         22

#define HIGH        1
#define LOW         0

#ifndef PICO_DEFAULT_LED_PIN
#error blink requires a board with a regular LED
#endif

#define STATUS_LED PICO_DEFAULT_LED_PIN
// #define STATUS_LED 15

#define BUS_SIZE 8
#define MAX_ERRORS 20
#define REFRESH_EVERY_N_WRITES 4
#define REFRESH_EVERY_N_READS 256 // 256 is the maximum for this implementation
#define N_REFRESHES 256

const unsigned int a_bus[BUS_SIZE] = {
  A0, A1, A2, A3, A4, A5, A6, A7
};

// #define debug_print printf
#define debug_print(...) ;

void nop(void) {
    __asm__ __volatile__( "nop\t\n");
}

void set_bus(unsigned int a) {
    int i;
    /* Write lowest bit into lowest address line first, then next-lowest bit, etc. */
    for (i = 0; i < BUS_SIZE; i++) {
        gpio_put(a_bus[i], a & 1);
        a >>= 1;
    }
}

void refresh() {
    int row;
    for (row = 0; row < 256; row++) {
        set_bus(row);
        gpio_put(RAS, LOW);
        nop();
        nop();
        nop();
        nop();
        nop();
        nop();
        nop();
        nop();
        nop();
        nop();
        nop();
        nop();
        nop();
        nop();
        nop();
        nop();
        nop();
        nop();
        nop();
        nop();
        gpio_put(RAS, HIGH);
    }
}

void write_address(int row, int col, bool val) {
    // Pull RAS and CAS HIGH
    gpio_put(RAS, HIGH);
    gpio_put(CAS, HIGH);

    // Set row address
    set_bus(row);

    // Pull RAS LOW
    gpio_put(RAS, LOW);
    nop(); // need to wait 15 ns before setting column address
    nop(); // need to wait 15 ns before setting column address
    nop(); // need to wait 15 ns before setting column address
    nop(); // need to wait 15 ns before setting column address

    // Set column address
    set_bus(col);

    // Pull Write LOW (Enables write)
    gpio_put(WRITE, LOW);

    // Pull CAS LOW
    gpio_put(CAS, LOW);

    // Set Data in pin to HIGH (write a one)
    gpio_put(D, val);

    sleep_us(1);
    gpio_put(WRITE, HIGH);
    gpio_put(CAS, HIGH);
    gpio_put(RAS, HIGH);
}

void read_address(int row, int col, bool *val) {
    // Pull RAS and CAS and WRITE HIGH
    gpio_put(RAS, HIGH);
    gpio_put(CAS, HIGH);
    gpio_put(WRITE, HIGH);

    // Set row address
    set_bus(row);

    // Pull RAS LOW
    gpio_put(RAS, LOW);
    nop(); // need to wait 15 ns before setting column address
    nop(); // need to wait 15 ns before setting column address
    nop(); // need to wait 15 ns before setting column address
    nop(); // need to wait 15 ns before setting column address

    // Set column address
    set_bus(col);

    // Pull CAS LOW
    gpio_put(CAS, LOW);

    sleep_us(1);

    *val = gpio_get(Q);

    gpio_put(WRITE, HIGH);
    gpio_put(CAS, HIGH);
    gpio_put(RAS, HIGH);
}

void setup() {
    bool dummy = false;
    int i;

    gpio_init(D);
    gpio_init(WRITE);
    gpio_init(RAS);
    gpio_init(A0);
    gpio_init(A2);
    gpio_init(A1);
    
    gpio_init(A7);
    gpio_init(A5);
    gpio_init(A4);
    gpio_init(A3);
    gpio_init(A6);
    gpio_init(Q);
    gpio_init(CAS);

    gpio_init(STATUS_LED);

    gpio_set_dir(D, GPIO_OUT);
    gpio_set_dir(WRITE, GPIO_OUT);
    gpio_set_dir(RAS, GPIO_OUT);
    gpio_set_dir(A0, GPIO_OUT);
    gpio_set_dir(A2, GPIO_OUT);
    gpio_set_dir(A1, GPIO_OUT);

    gpio_set_dir(A7, GPIO_OUT);
    gpio_set_dir(A5, GPIO_OUT);
    gpio_set_dir(A4, GPIO_OUT);
    gpio_set_dir(A3, GPIO_OUT);
    gpio_set_dir(A6, GPIO_OUT);
    gpio_set_dir(Q, GPIO_IN);
    gpio_set_dir(CAS, GPIO_OUT);

    gpio_set_dir(STATUS_LED, GPIO_OUT);

    gpio_put(RAS, HIGH);
    gpio_put(CAS, HIGH);
    gpio_put(WRITE, HIGH);
    gpio_put(D, LOW);
    gpio_put(A0, LOW);
    gpio_put(A1, LOW);
    gpio_put(A2, LOW);
    gpio_put(A3, LOW);
    gpio_put(A4, LOW);
    gpio_put(A5, LOW);
    gpio_put(A6, LOW);
    gpio_put(A7, LOW);
    
    sleep_us(1110);
    for (i = 0; i < 8; i++) {
        read_address(0, 0, &dummy);
        sleep_us(1);
    }
}

void test(bool start_val) {
    int row, col, refresh_count;
    int errors = 0;
    bool read_val = false;
    bool val = start_val;

    for (row = 0; row < 256; row++) {
        debug_print("Testing row: %d\n", row);
        for (col = 0; col < 256; col++) {
            gpio_put(STATUS_LED, val);
            write_address(row, col, val);
            read_address(row, col, &read_val);
            if (val != read_val) {
                printf("ERROR: row %d col %d read %d but expected %d\n", row, col, read_val, val);
                if (++errors > MAX_ERRORS) {
                    while (true) {
                        gpio_put(STATUS_LED, HIGH);
                        sleep_ms(50);
                        gpio_put(STATUS_LED, LOW);
                        sleep_ms(50);
                    }
                }
            }
            val = !val;
            gpio_put(STATUS_LED, val);
            if (col % REFRESH_EVERY_N_WRITES == 0) {
                refresh();
            }
        }
    }
    for (refresh_count = 0; refresh_count < N_REFRESHES; refresh_count++) {
        printf("Refresh test %d\n", refresh_count);
        val = start_val; // start from start_val (which determines whether we're testing 10101010... or 01010101...)
        for (row = 0; row < 256; row++) {
            for (col = 0; col < 256; col++) {
                gpio_put(STATUS_LED, val);
                read_address(row, col, &read_val);
                if (val != read_val) {
                    printf("ERROR: row %d col %d read %d but expected %d\n", row, col, read_val, val);
                    if (++errors > MAX_ERRORS) {
                        while (true) {
                            gpio_put(STATUS_LED, HIGH);
                            sleep_ms(50);
                            gpio_put(STATUS_LED, LOW);
                            sleep_ms(50);
                        }
                    }
                }
                val = !val;
                gpio_put(STATUS_LED, val);
                if (col % REFRESH_EVERY_N_READS == 0) {
                    refresh();
                }
            }
        }
    }
}

int main() {
    stdio_init_all();
    setup();

    sleep_ms(10000); // wait until /dev/ttyACM0 device is ready on host
    printf("Starting 10101010... test\n");
    test(true);
    printf("Starting 01010101... test\n");
    test(false);
    printf("Test done. All OK!\n");
    while (true) {
        gpio_put(STATUS_LED, HIGH);
        sleep_ms(1000);
        gpio_put(STATUS_LED, LOW);
        sleep_ms(1000);
    }
}
