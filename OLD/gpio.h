#include <stdint.h>
#include <bcm2835.h>

#ifndef _GPIO_H_
#define _GPIO_H_

//output pins
#define LED1 RPI_GPIO_P1_07
#define LED2 RPI_GPIO_P1_11

//input pins
#define BUTTON1 RPI_GPIO_P1_15
#define BUTTON2 RPI_GPIO_P1_19
#define BUTTON3 RPI_GPIO_P1_21
#define BUTTON4 RPI_GPIO_P1_23

int  open_gpio (void);
void close_gpio (void);

void write_gpio_pin (uint8_t pin, uint8_t level);
uint8_t read_gpio_pin (uint8_t pin);

#endif

