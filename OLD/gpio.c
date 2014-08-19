#include "gpio.h"

int open_gpio (void) {

 if (!bcm2835_init()) {
  return (1);
 }

 // Set the pin to be an input
 bcm2835_gpio_fsel(BUTTON1, BCM2835_GPIO_FSEL_INPT);
 bcm2835_gpio_fsel(BUTTON2, BCM2835_GPIO_FSEL_INPT);
 bcm2835_gpio_fsel(BUTTON3, BCM2835_GPIO_FSEL_INPT);
 bcm2835_gpio_fsel(BUTTON4, BCM2835_GPIO_FSEL_INPT);

 // Set the pin to be an output
 bcm2835_gpio_fsel(LED1, BCM2835_GPIO_FSEL_OUTP);
 bcm2835_gpio_fsel(LED2, BCM2835_GPIO_FSEL_OUTP);

 return (0);
}

void close_gpio (void) {
 bcm2835_close();
}

uint8_t read_gpio_pin (uint8_t pin) {

}

void write_gpio_pin (uint8_t pin, uint8_t level) {
 // Turn it on/off
// bcm2835_gpio_write(PIN, HIGH);
// bcm2835_gpio_write(PIN, LOW);
 bcm2835_gpio_write(pin, level);
}


