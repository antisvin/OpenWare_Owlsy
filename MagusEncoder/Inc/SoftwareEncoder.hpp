#ifndef __SOFTWARE_ENCODER_HPP__
#define __SOFTWARE_ENCODER_HPP__

#include "DebouncedButton.hpp"

#define ENCODER_SPEED 9
// Something around 8-10 works fine

#define min(a, b) (a < b ? a : b)
#define max(a, b) (a > b ? a : b)


// Durations should be up to bit length of click_state - 1
class SoftwareEncoder : public DebouncedButton {
public:
    SoftwareEncoder(GPIO_TypeDef *port_a, uint32_t pin_a, GPIO_TypeDef *port_b,
              uint32_t pin_b, GPIO_TypeDef *port_click, uint32_t pin_click)
        : DebouncedButton(port_click, pin_click), port_a(port_a)
        , pin_a(pin_a), port_b(port_b), pin_b(pin_b) {
        last_counter_update = HAL_GetTick();
    };
    ~SoftwareEncoder() = default;

    /**
     * Update should be performed frequently to avoid missing rotations. I think
     * SysTick handler would be perfect for this, as we will keep counting consistently with any
     * audio buffer size.
     * 
     * @return: true if rotation detected
     */
    bool updateCounter(uint32_t max_value) {
        a_state <<= 1;
        a_state |= getPin(port_a, pin_a);
        b_state <<= 1;
        b_state |= getPin(port_b, pin_b);
        if ((a_state & 0x03) == 0x00 && (b_state & 0x03) == 0x02 ) {
            updateTicks();
            counter += max_value / getTicks();
            return true;
        }
        else if ((b_state & 0x03) == 0x00 && (a_state & 0x03) == 0x02 ) {
            updateTicks();
            counter -= max_value / getTicks();
            return true;
        }
        return false;
    }

    inline uint16_t getValue() const {
        return counter;
    }

    uint16_t getTicks() const {
        uint16_t ticks = min(last_counter_update - prev_counter_update, 0xFFFFU);
        return max(1, (ticks * ticks) >> ENCODER_SPEED);
    }

private:
    GPIO_TypeDef *port_a;
    uint32_t pin_a;
    GPIO_TypeDef *port_b;
    uint32_t pin_b;
    uint32_t a_state, b_state;
    uint16_t counter = 0x3FFF;
    uint32_t last_counter_update, prev_counter_update;

    inline void updateTicks(){
        prev_counter_update = last_counter_update;
        last_counter_update = HAL_GetTick();
    }
};

#endif
