#include "gpio_irq.h"
#include "klog.h"
#include "../common/interrupt_codes.h"

//macros to declare associations between GPIO and interrupt code in a readable list
#define _gpio(code, gpio) {#code, (uint8_t)code, gpio, -EINVAL}
#define _gpio_list_end {NULL, -EINVAL, -EINVAL, -EINVAL}

//associations between GPIO and interrupt codes
static gpio_irq_t list[] = {
	_gpio(INTERRUPT_ACQUISITION_FULL,		480),
	_gpio(INTERRUPT_ACQUISITION_HALF_FULL,	481),
	_gpio(INTERRUPT_ACQUISITION_TEST,		482),
	_gpio_list_end
};

//--

static gpio_irq_handler_f gpio_irq_handler = NULL;

static gpio_irq_t* find_gpio_irq(int irq) {
	gpio_irq_t *gpioirq;

	for (gpioirq = list; gpioirq->name != NULL; gpioirq++) {
		if (gpioirq->irq == irq) {
			return gpioirq;
		}
	}

	return NULL;
}

static irqreturn_t forward_irq_to_handler(int irq, void *dev_id) {
	klog_debug("Interrupt happened at irq=%d\n", irq);

	if (gpio_irq_handler == NULL) {
		klog_warning("No gpio_irq_handler handler! Ignoring...\n");
		return IRQ_HANDLED;
	}

	gpio_irq_t* gpioirq = find_gpio_irq(irq);
	if (gpioirq == NULL) {
		klog_warning("Unknown irq=%d! Ignoring...\n", irq);
		return IRQ_HANDLED;
	}
	
	gpio_irq_handler(gpioirq);
	return IRQ_HANDLED;
}

//--

void set_gpio_irq_handler(gpio_irq_handler_f handler) {
	gpio_irq_handler = handler;
}

int enable_gpio_irqs() {
	gpio_irq_t *gpioirq;
	for (gpioirq = list; gpioirq->name != NULL; gpioirq++) {
		gpioirq->irq = gpio_to_irq(gpioirq->gpio);
		klog_info("gpio_to_irq: %d -> %d (%s)\n", gpioirq->gpio, gpioirq->irq, gpioirq->name);

		int error = request_irq(gpioirq->irq, forward_irq_to_handler, 0, gpioirq->name, NULL);
		if (error) {
			klog_error("failure requesting irq %d (gpio=%d, %s) error=%d\n", gpioirq->irq, gpioirq->gpio, gpioirq->name, error);
			gpioirq->irq = -EINVAL;
			return error;
		}
		else {
			klog_info("requested irq %d (gpio=%d, %s)\n", gpioirq->irq, gpioirq->gpio, gpioirq->name);
		}
	}

	return 0;
}

void disable_gpio_irqs(void) {
	gpio_irq_t *gpioirq;
	for (gpioirq = list; gpioirq->name != NULL; gpioirq++) {
		if (gpioirq->irq != -EINVAL) {
			klog_info("freeing irq: irq=%d, gpio=%d (%s)\n", gpioirq->irq, gpioirq->gpio, gpioirq->name);
			free_irq(gpioirq->irq, NULL);
			gpioirq->irq = -EINVAL;
		}
	}
}