#ifndef __SOFTWARE_ENCODER_HPP__
#define __SOFTWARE_ENCODER_HPP__

#include "DebouncedButton.hpp"


// Durations should be up to bit length of click_state - 1
template<uint8_t debounce_duration = 8, uint8_t long_press_duration = 31>
class SoftwareEncoder : public DebouncedButton<debounce_duration, long_press_duration> {
public:
    SoftwareEncoder(GPIO_TypeDef *port_a, uint32_t pin_a, GPIO_TypeDef *port_b,
              uint32_t pin_b, GPIO_TypeDef *port_click, uint32_t pin_click)
        : port_a(port_a), pin_a(pin_a), port_b(port_b), pin_b(pin_b),
          DebouncedButton<debounce_duration, long_press_duration>(port_click, pin_click) {};
    ~SoftwareEncoder() = default;

    /*
     * Update should be performed frequently to avoid missing rotations. I think
     * SysTick handler would be perfect for this, as we will keep counting consistently with any
     * audio buffer size.
     */
    void updateCounter() {
        a_state <<= 1;
        a_state |= getPin(port_a, pin_a);
        b_state <<= 1;
        b_state |= getPin(port_b, pin_b);
        if ((a_state & 0x03) == 0x02 && (b_state & 0x03) == 0x00 ) {
            counter++;
        }
        else if ((b_state & 0x03) == 0x02 && (a_state & 0x03) == 0x00 ) {
            counter--;
        }
    }

    inline uint16_t getValue() const {
        return counter;
    }

private:
    GPIO_TypeDef *port_a;
    uint32_t pin_a;
    GPIO_TypeDef *port_b;
    uint32_t pin_b;
    uint32_t a_state, b_state;
    uint16_t counter;
};

#endif