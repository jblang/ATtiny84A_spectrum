#define F_CPU 8000000

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <math.h>
#include <stdlib.h>

// UART pins
#define TX_PORT     PORTB
#define TX_PIN      PB0
#define TX_DDR      DDRB
#define TX_DDR_PIN  DDB0

volatile uint16_t txbyte = 0;

void uart_putchar(char c) {
    while (txbyte)
        ;
    txbyte = (c << 1) | (1 << 9); // char + stop bit
    TCCR0B = (1 << CS01);
}

void uart_putstr(char* string) {
    while (*string)
        uart_putchar(*string++);
}

void uart_init() {
    TX_DDR |= (1 << TX_DDR_PIN);
    TX_PORT |= (1 << TX_PIN);
    TCCR0A = (1 << WGM01); // CTC mode
    TIMSK0 |= (1 << OCF0A);
    OCR0A = 103; // 104 us (9600 baud ) @ 1 MHz timer clock
    sei();
}

ISR(TIM0_COMPA_vect) {
    uint16_t local_txbyte = txbyte;
    if (local_txbyte & 1)
        TX_PORT |= (1 << TX_PIN);
    else
        TX_PORT &= ~(1 << TX_PIN);
    local_txbyte >>= 1;
    txbyte = local_txbyte;
    if (!local_txbyte) {
        TCCR0B = 0;
        TCNT0 = 0;
    }
}

#define N           32
#define PI2         6.2832
#define TFS         6
#define NUM_LEDS    8
#define BANK_MAX    16
#define THRESHOLD   0U
#define OFFSET      0

static int8_t samples[N]; // discrete-time signal
static int16_t W[N]; // Twiddle Factors
static uint16_t power[NUM_LEDS + 1]; // power spectrum of x
static uint8_t bank[NUM_LEDS]; // flash accumulation
static const uint8_t pins[NUM_LEDS] = {0x11, 0x12, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02};

volatile uint8_t counter = 0; // samples counter

void
dft(void) {
    int32_t Xre, Xim;
    uint8_t sin_idx, cos_idx;
    
    power[NUM_LEDS] = 0;

    for (uint8_t k = 0; k < N / 2 + 1; ++k) {
        Xre = Xim = 0;
        cos_idx = 0;
        sin_idx = 3 * N / 4;
        for (uint8_t n = 0; n < N; ++n) {
            Xre += ((int) samples[n] * (int) W[cos_idx % N]) >> TFS;
            Xim -= ((int) samples[n] * (int) W[sin_idx % N]) >> TFS;
            sin_idx += k;
            cos_idx += k;
        }
        uint16_t p = (Xre * Xre + Xim * Xim) / (N * N);
        if (k > NUM_LEDS)
            power[NUM_LEDS] = p;
        else
            power[k] = p;
    }
}

void
kolorofon(void) {
    for (uint8_t i = 0; i < NUM_LEDS; ++i) {
        if (power[i + 1] > THRESHOLD) {
            if (bank[i] < BANK_MAX) {
                bank[i]++;
            }
            if (pins[i] & 0xf0)
                PORTB |= _BV(pins[i] & 0xf);
            else
                PORTA |= _BV(pins[i] & 0xf);
        } else {
            if (bank[i]) {
                bank[i]--;
            } else {
                if (pins[i] & 0xf0)
                    PORTB &= ~_BV(pins[i] & 0xf);
                else
                    PORTA &= ~_BV(pins[i] & 0xf);
            }
        }
    }
}

ISR(ADC_vect) {

    if (counter < N) {
        samples[counter++] = ADCH; // read raw sample <-128; 127>

        ADCSRA |= _BV(ADSC); // trigger next signal acquisition
    }
}

void adc_init(void) {
    ADCSRA |= _BV(ADPS2) | _BV(ADPS1); // 8 MHz / 64 = 125 khz ADC clock
    ADCSRA |= _BV(ADEN) | _BV(ADIE); // enable ADC interrupt
    ADCSRB |= _BV(BIN) | _BV(ADLAR); // left adjust, bipolar mode
    ADMUX =  _BV(MUX3) | _BV(MUX0); // differential ADC0/1 20x gain
}

void log_power(void)
{
    for (uint8_t i = 0; i < N >> 1; i++) {
        char buf[16];
        utoa(power[i], buf, 10);
        uart_putstr(buf);
        uart_putchar('\t');
    }
    uart_putchar('\n');
    uart_putchar('\r');
}

void log_samples(void)
{
    for (uint8_t i = 0; i < N; i++) {
        char buf[16];
        itoa(samples[i], buf, 10);
        uart_putstr(buf);
        uart_putchar('\t');
    }
    uart_putchar('\n');
    uart_putchar('\r');
}

int main(void) {
    DDRB |= _BV(PB2) | _BV(PB1);
    DDRA |= _BV(PA7) | _BV(PA6) | _BV(PA5) | _BV(PA4) | _BV(PA3) | _BV(PA2);

    adc_init();
    uart_init();
    sei();

    ADCSRA |= _BV(ADSC);

    /* generate Twiddle Factors */
    for (uint8_t n = 0; n < N; ++n) {
        W[n] = (int16_t) ((float) (1 << TFS) *
                cos((float) n * PI2 / (float) N));
    }

    while (1) {
        if (counter == N) {
            dft();
            //log_power();
            //log_samples();
            kolorofon();
            counter = 0;
            ADCSRA |= _BV(ADSC);
        }
    }
}

