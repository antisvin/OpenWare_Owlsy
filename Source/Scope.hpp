#ifndef __SCOPE_HPP__
#define __SCOPE_HPP__

#include "ApplicationSettings.h"
#include "SerialBuffer.hpp"
#include "device.h"
#include "ProgramVector.h"


template <uint8_t input_channels = 2, uint8_t output_channels = 2>
class Scope {
public:
    /*
     * Both input and output channels can be selected
     */
    void setChannel(uint8_t new_channel){
        reset();
        if (new_channel >= total_channels) {
            new_channel = total_channels - 1;
        }
        channel = new_channel;
    }

    void update(){
        readChannelData();        
    }

    int8_t getBufferData(){
        return buffer.pull();
    }

private:
    SerialBuffer<CODEC_BLOCKSIZE, int8_t> buffer;
    uint8_t channel;
    static constexpr uint8_t total_channels = input_channels + output_channels;

    void readChannelData(){
        ProgramVector *pv = getProgramVector();
        // We could skip some element in loops below to scale horizontal resolution
        int32_t* stream = channel < input_channels ? pv->audio_input : pv->audio_output;
        uint8_t rel_channel = channel < input_channels ? channel : (channel - input_channels);
        for (int i = 0; i < 32; i++){
            int32_t sample = stream[input_channels * i + rel_channel];
            buffer.push(int8_t(sample >> 16));
        }
    }

    void reset(){
        buffer.reset();
        buffer.setAll(0U);
    }
};

#endif
