/* Keep absolute vectors in one object: separate _CABS areas concatenate. */
#include <gb/isr.h>
void serial_isr(void) __critical __interrupt;
void lcd_split(void) __critical __interrupt;
ISR_VECTOR(VECTOR_SERIAL,serial_isr)
ISR_VECTOR(VECTOR_STAT,lcd_split)
