// GPIOHS Protocol Implementation

#include "gpiohs.h"
#include "fpioa.h"
#include "utils.h"
#include "mm.h"

#define GPIOHS_MAX_PINNO 32

volatile gpiohs_t *const gpiohs = (volatile gpiohs_t *)GPIOHS_V;

void gpiohs_set_drive_mode(uchar pin, gpio_drive_mode_t mode)
{
    // configASSERT(pin < GPIOHS_MAX_PINNO);
    int io_number = fpioa_get_io_by_function(FUNC_GPIOHS0 + pin);
    // configASSERT(io_number >= 0);

    fpioa_pull_t pull = FPIOA_PULL_NONE;
    uint32 dir = 0;

    switch(mode)
    {
        case GPIO_DM_INPUT:
            pull = FPIOA_PULL_NONE;
            dir = 0;
            break;
        case GPIO_DM_INPUT_PULL_DOWN:
            pull = FPIOA_PULL_DOWN;
            dir = 0;
            break;
        case GPIO_DM_INPUT_PULL_UP:
            pull = FPIOA_PULL_UP;
            dir = 0;
            break;
        case GPIO_DM_OUTPUT:
            pull = FPIOA_PULL_DOWN;
            dir = 1;
            break;
        default:
            // configASSERT(!"GPIO drive mode is not supported.") 
            break;
    }

    fpioa_set_io_pull(io_number, pull);
    set_bit(dir ? gpiohs->output_en.u32 : gpiohs->input_en.u32, io_number);
    clear_bit((!dir) ? gpiohs->output_en.u32 : gpiohs->input_en.u32, io_number);
}

void gpiohs_set_pin(uchar pin, gpio_pin_value_t value)
{
    // configASSERT(pin < GPIOHS_MAX_PINNO);
    switch (value)
    {
        case GPIO_PV_LOW:
            clear_bit(gpiohs->output_val.u32, pin);
            break;
        case GPIO_PV_HIGH:
            set_bit(gpiohs->output_val.u32, pin);
            break;
        default:
            break;
    }
}
