/**
 * @file   gpio_test.c
 * @author Derek Molloy
 * @date   19 April 2015
 * @brief  A kernel module for controlling a GPIO LED/button pair. The device mounts devices via
 * sysfs /sys/class/gpio/gpio115 and gpio49. Therefore, this test LKM circuit assumes that an LED
 * is attached to GPIO 49 which is on P9_23 and the button is attached to GPIO 115 on P9_27. There
 * is no requirement for a custom overlay, as the pins are in their default mux mode states.
 * @see http://www.derekmolloy.ie/
 */
 
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>                 // Required for the GPIO functions
#include <linux/interrupt.h>            // Required for the IRQ code

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Derek Molloy");
MODULE_DESCRIPTION("A Button/LED test driver for the BBB");
MODULE_VERSION("0.1");

static unsigned int gpioLED = 49;
static unsigned int gpioButton = 115;
static unsigned int irqNumber;
static unsigned int numberPresses = 0;
static bool ledOn = false;

// Function prototype for the custom IRQ handler function -- see below for the implementation
static irq_handler_t ebbgpio_irq_handler(unsigned int irq, void *dev_id,
                                         struct pt_regs *regs);

/** @brief The LKM initialization function
 *  The static keyword restricts the visibility of the function to within this C file. The __init
 *  macro means that for a built-in driver (not a LKM) the function is only used at initialization
 *  time and that it can be discarded and its memory freed up after that point. In this example this
 *  function sets up the GPIOs and the IRQ
 *  @return returns 0 if successful
 */
static int __init ebbgpio_init(void) {
  int result = 0;
  printk(KERN_INFO "GPIO_TEST: Initializing the GPIO_TEST LKM\n");
  // Is the GPIO a valid GPIO number?
  if (!gpio_is_valid(gpioLED)) {
    printk(KERN_INFO "GPIO_TEST: invalid LED GPIO\n");
    return -ENODEV;
  }

  // Going to set up the LED. It is a GPIO in output mode and will be on by default.
  ledOn = true;
  gpio_request(gpioLED, "sysfs");  // request the GPIO
  gpio_direction_output(gpioLED, ledOn);  // set the GPIO in OUTPUT mode and on
  gpio_export(gpioLED, false);  // Causes gpio49 to appear in /sys/class/gpio. The bool argument prevents the direction from being changed
  gpio_request(gpioButton, "sysfs");  // request the GPIO
  gpio_direction_input(gpioButton);  // set the GPIO in INPUT mode
  gpio_export(gpioLED, false);  // Causes gpio49 to appear in /sys/class/gpio. The bool argument prevents the direction from being changed

  // Perform a quick test to see that the button is working as expected on LKM load
  printk(KERN_INFO "GPIO_TEST: The button state is currently: %d\n", gpio_get_value(gpioButton));

  // GPIO numbers and IRQ numbers are not the same! This function performs the mapping for us
  irqNumber = gpio_to_irq(gpioButton);
  printk(KERN_INFO "GPIO_TEST: The button is mapped to IRQ: %d\n", irqNumber);

  // This next call requests an interrupt line
  result = request_irq(irqNumber,             // The interrupt number requested
                       (irq_handler_t) ebbgpio_irq_handler, // The pointer to the handler function below
                       IRQF_TRIGGER_RISING,   // Interrupt on rising edge (button press, not release)
                       "ebb_gpio_handler",    // Used in /proc/interrupts to identify the owner
                       NULL);                 // The *dev_id for shared interrupt lines, NULL is okay

  printk(KERN_INFO "GPIO_TEST: The interrupt request result is: %d\n", result);
  return result;
}

/** @brief The LKM cleanup function
 *  *  Similar to the initialization function, it is static. The __exit macro notifies that if this
 *   *  code is used for a built-in driver (not a LKM) that this function is not required. Used to release the
 *    *  GPIOs and display cleanup messages.
 *     */
static void __exit ebbgpio_exit(void){
  printk(KERN_INFO "GPIO_TEST: The button state is currently: %d\n", gpio_get_value(gpioButton));
  printk(KERN_INFO "GPIO_TEST: The button was pressed %d times\n", numberPresses);
  gpio_set_value(gpioLED, 0);              // Turn the LED off, makes it clear the device was unloaded
  gpio_unexport(gpioLED);                  // Unexport the LED GPIO
  free_irq(irqNumber, NULL);               // Free the IRQ number, no *dev_id required in this case
  gpio_unexport(gpioButton);               // Unexport the Button GPIO
  gpio_free(gpioLED);                      // Free the LED GPIO
  gpio_free(gpioButton);                   // Free the Button GPIO
  printk(KERN_INFO "GPIO_TEST: Goodbye from the LKM!\n");
}

static irq_handler_t ebbgpio_irq_handler(unsigned int irq, void *dev_id,
                                         struct pt_regs *regs) {
  ledOn = != ledOn;
  gpio_set_value(gpioLED, ledOn);
  printk(KERN_INFO "GPIO_TEST: Interrupt! (button state is %d)\n", gpio_get_value(gpioButton));
  numberPresses++;                         // Global counter, will be outputted when the module is unloaded
  return (irq_handler_t) IRQ_HANDLED;      // Announce that the IRQ has been handled correctly
}

// This next calls are  mandatory -- they identify the initialization function
// and the cleanup function (as above).
module_init(ebbgpio_init);
module_exit(ebbgpio_exit);
