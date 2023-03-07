#ifndef __SOFTWARE_LED_HPP__
#define __SOFTWARE_LED_HPP__

#include "Pin.h"

static constexpr float sr = 1000.f;
static constexpr float resolution = 65535.f;
static constexpr float step = 120.f / sr;

class LedChannel {
public:
    LedChannel(Pin pin, bool inverted = false)
        : pin(pin)
        , on(!inverted)
        , off(inverted) {
    }
    void set(uint8_t value) {
        // this->threshold = threshold;
        brightness = float(value) / 255;
        // brightness = brightness * brightness * brightness;
        threshold = brightness * resolution;
    }
    void update() {
        pwm += step;
        if (pwm > 1.f)
            pwm -= 1.f;
        pin.set(brightness > pwm ? on : off);
    }

private:
    Pin pin;
    bool on, off;
    float brightness = 0.f, threshold = 0.f, counter = 0.f, pwm = 0.f;
};

class RGBLed {
public:
    RGBLed(LedChannel r, LedChannel g, LedChannel b)
        : r(r)
        , g(g)
        , b(b) {
    }
    void set(uint32_t rgb) {
        value = rgb;
        value_dirty = true;
    }
    static uint32_t fromHSB(float hue, float sat, float brightness) {
        float red = 0.0;
        float green = 0.0;
        float blue = 0.0;

        if (sat == 0.0) {
            red = brightness;
            green = brightness;
            blue = brightness;
        }
        else {
            while (hue >= 1.0) {
                hue -= 1.0;
            }
            while (hue < 0.0) {
                hue += 1.0;
            }

            int slice = hue * 6.0;
            float hue_frac = (hue * 6.0) - slice;

            float aa = brightness * (1.0 - sat);
            float bb = brightness * (1.0 - sat * hue_frac);
            float cc = brightness * (1.0 - sat * (1.0 - hue_frac));

            switch (slice) {
            case 0:
                red = brightness;
                green = cc;
                blue = aa;
                break;
            case 1:
                red = bb;
                green = brightness;
                blue = aa;
                break;
            case 2:
                red = aa;
                green = brightness;
                blue = cc;
                break;
            case 3:
                red = aa;
                green = bb;
                blue = brightness;
                break;
            case 4:
                red = cc;
                green = aa;
                blue = brightness;
                break;
            case 5:
                red = brightness;
                green = aa;
                blue = bb;
                break;
            default:
                red = 0.0;
                green = 0.0;
                blue = 0.0;
                break;
            }
        }

        uint32_t ired = red * 255.0;
        uint32_t igreen = green * 255.0;
        uint32_t iblue = blue * 255.0;

        return uint32_t((ired << 16) | (igreen << 8) | (iblue));
    }
    static uint32_t from30Bit(uint32_t rgb) {
        return (((rgb >> 22) & 0xff) << 16) | (((rgb >> 12) & 0xff) << 8) |
            ((rgb >> 2) & 0xff);
    }
    uint32_t get() const {
        return value;
    }
    void update() {
        if (value_dirty) {
            value_dirty = false;
            r.set(value >> 16);
            g.set((value & 0xffff) >> 8);
            b.set(value & 0xff);
        }

        r.update();
        g.update();
        b.update();
    }

private:
    LedChannel r, g, b;
    bool value_dirty = false;
    uint32_t value = 0;
};

#endif
