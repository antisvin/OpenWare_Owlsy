#ifndef __DEBOUNCED_BUTTON_HPP__
#define __DEBOUNCED_BUTTON_HPP__

#include "gpio.h"
#include "ApplicationSettings.h"

// Durations should be up to bit length of click_state - 2
template<uint8_t debounce_duration = 8, uint8_t long_press_duration = 31>
class DebouncedButton {
public:
    DebouncedButton(GPIO_TypeDef *port_click, uint32_t pin_click)
        : port_click(port_click), pin_click(pin_click) {};
    ~DebouncedButton() = default;

    /*
     * Debouncing should probably be performed at screen refresh rate
     */
    void debounce() {
        click_state <<= 1;
        // This assumes that button is pulled up, so pin input gets inverted. We
        // could have an option to remove inverting, or better a template parameter
        click_state |= !getPin(port_click, pin_click);
    }        

    inline bool isPressed() const {
        return (click_state & short_click_mask) == short_click_mask;
    }

    inline bool isRisingEdge() const {
        return (click_state & short_click_mask) == click_mask_rising; 
    }

    inline bool isFallingEdge() const {
        return (click_state & short_click_mask) == click_mask_falling; 
    }

    inline bool isLongPress() const {
        return (click_state & long_click_mask) == long_click_mask;
    }

    inline bool isLongRisingEdge() const {
        return (click_state & long_click_mask) == click_mask_long_rising; 
    }

    inline bool isLongFallingEdge() const {
        return (click_state & long_click_mask) == click_mask_long_falling;
    }

private:
    GPIO_TypeDef *port_click;
    uint32_t pin_click;
    uint32_t click_state;

    static constexpr uint32_t click_mask_rising       = 0x7;
    static constexpr uint32_t click_mask_falling      = 0xe;
    static constexpr uint32_t short_click_mask        = 0xf;
    static constexpr uint32_t click_mask_long_rising  = 0x7fffffff;
    static constexpr uint32_t click_mask_long_falling = 0xfffffffe;
    static constexpr uint32_t long_click_mask         = 0xffffffff;
};

#endif