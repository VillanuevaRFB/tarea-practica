#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

volatile unsigned int fuente = 0; // 0 = LM35 (PC1), 1 = Potenciómetro (PC0)

ISR(INT0_vect) {
    fuente ^= 1; // Cambia entre 0 y 1 al presionar el botón
}

void configurar_interrupcion() {
    EIMSK |= (1 << INT0);        // Habilita INT0
    EICRA |= (1 << ISC01);       // Interrupción en flanco de bajada
    DDRB &= ~0x01;               // PB0 como entrada
    PORTB |= 0x01;               // Pull-up activado
    sei();                       // Habilita interrupciones globales
}

void configurar_adc() {
    ADMUX = (1 << REFS0) | (1 << ADLAR);          // AVcc como referencia, resultado a la izquierda
    ADCSRA = (1 << ADEN) | (1 << ADPS2);          // Habilita ADC, prescaler 16
}

unsigned int leer_adc(unsigned int canal) {
    ADMUX = (ADMUX & 0xF0) | (canal & 0x0F);       
    ADCSRA |= (1 << ADSC);                         // Inicia conversión
    while (!(ADCSRA & (1 << ADIF)));               // Espera a que termine
    ADCSRA |= (1 << ADIF);                         // Limpia bandera
    return ADCH;                                   // Devuelve 8 bits (ajustado a la izquierda)
}

void mostrar_digito(unsigned int valor, unsigned int control) {
    PORTB = control;            // PB3, PB4, PB5 controlan el display activo
    PORTD &= 0x0F;              // Limpia PD4–PD7
    PORTD |= (valor << 4);      // Coloca BCD en PD4–PD7
    _delay_ms(5);
}

int main(void) {
    DDRD = 0xFC;    // PD2-PD7 como salida (motor y displays)
    DDRB = 0x38;    // PB3, PB4, PB5 como salida (control displays)
    PORTD &= ~(0x0C); // Apaga motor inicialmente

    configurar_interrupcion();
    configurar_adc();

    while (1) {
        unsigned int lectura_pot = leer_adc(0);      // Canal 0 = PC0 = Potenciómetro
        unsigned int lectura_lm35 = leer_adc(1);     // Canal 1 = PC1 = LM35

        double voltaje_lm35 = (lectura_lm35 * 5.0) / 255.0;
        double temperatura_lm35 = voltaje_lm35 * 100.0;

        double voltaje_pot = (lectura_pot * 5.0) / 255.0;
        double temperatura_referencia = voltaje_pot * 100.0;

        double temperatura_actual = (fuente == 0) ? temperatura_lm35 : temperatura_referencia;

        if (temperatura_lm35 < temperatura_referencia) {
            PORTD |= 0x0C;   // Enciende motor en PD2 y PD3
        } else {
            PORTD &= ~0x0C;  // Apaga motor
        }

        // Convertir temperatura a BCD
        uint16_t temp = (uint16_t)(temperatura_actual * 10); // Ej: 23.4 → 234
        unsigned int decenas = temp / 100;
        unsigned int unidades = (temp / 10) % 10;
        unsigned int decimas = temp % 10;

        // Multiplexado: decimales -> unidades -> decenas
        mostrar_digito(decimas, 0x08);  // PB3
        mostrar_digito(unidades, 0x10); // PB4
        mostrar_digito(decenas, 0x20);  // PB5
    }
}