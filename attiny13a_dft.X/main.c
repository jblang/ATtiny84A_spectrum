#define F_CPU 9600000

/**
 * Copyright (c) 2019, Lukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 * ATtiny13/026
 * Disco lights using DFT (Discrete Fourier Transformation)
 */

#include <avr/io.h>
#include <avr/interrupt.h>

#define LED_LOW              PB1
#define LED_MID              PB2
#define LED_HIGH             PB0

#define LOW_THRESHOLD        (65500UL)
#define MID_THRESHOLD        (500)
#define HIGH_THRESHOLD       (5000)

#define N                    (7) // N-points (DFT)
#define B                    (3 * N / 4) // b-value (DFT)

const int8_t W[N] = {10, 6, -2, -9, -9, -2, 6}; // twiddle factors (DFT)
int8_t samples[N]; // raw samples (ADC)
uint16_t power[N >> 1]; // power spectrum (DFT)
volatile uint8_t counter = 0;

static void dft(void);

int
main(void) {

    /* setup */
    DDRB |= _BV(LED_LOW) | _BV(LED_MID) | _BV(LED_HIGH); // set LED pins as OUTPUT
    ADCSRA |= _BV(ADPS2) | _BV(ADPS1); // set ADC division factor to 256
    ADCSRA |= _BV(ADEN) | _BV(ADIE); // enable ADC interrupt
    ADMUX = _BV(MUX1); // set ADC4 (PB4) as audio input
    ADMUX |= _BV(ADLAR); // left adjust of ADC result
    sei(); // enable global interrupts

    ADCSRA |= _BV(ADSC); // start first signal acquisition

    /* loop */
    while (1) {
        if (counter == N) {
            dft(); // do some DSP

            /* LOW freqency band */
            if (~power[0] > LOW_THRESHOLD) {
                PORTB |= _BV(LED_LOW);
            } else {
                PORTB &= ~_BV(LED_LOW);
            }

            /* MID freqency band */
            if (power[1] > MID_THRESHOLD) {
                PORTB |= _BV(LED_MID);
            } else {
                PORTB &= ~_BV(LED_MID);
            }

            /* HIGH frequency band */
            if (power[2] > HIGH_THRESHOLD) {
                PORTB |= _BV(LED_HIGH);
            } else {
                PORTB &= ~_BV(LED_HIGH);
            }

            counter = 0; // reset samples counter
            ADCSRA |= _BV(ADSC); // trigger next signal acqusistion
        }
    }
}

ISR(ADC_vect) {

    if (counter < N) {
        samples[counter++] = ADCH - 128; // read raw sample <-128; 127>
        ADCSRA |= _BV(ADSC); // trigger next signal acquisition
    }
}

/**
 * Twiddle-factor-based DFT algorithm with reduced memory access.
 */
void
dft(void) {
    uint8_t a, b, i, j;
    int16_t re[N];
    int16_t im[N];

    for (i = 0; i < N; ++i) {
        re[i] = 0;
        im[i] = 0;
    }

    for (i = 0; i < (N >> 1); ++i) {
        a = 0;
        b = B;
        for (j = 0; j < N; ++j) {
            re[i] += W[a % N] * samples[j];
            im[i] -= W[b % N] * samples[j];
            a += i;
            b += i;
        }
        power[i] = (re[i] * re[i] + im[i] * im[i]) >> 4;
    }
}