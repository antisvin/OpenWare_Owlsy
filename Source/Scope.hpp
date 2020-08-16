#ifndef __SCOPE_HPP__
#define __SCOPE_HPP__

#include "ApplicationSettings.h"
#include "SerialBuffer.hpp"
#include "device.h"
#include "ProgramVector.h"


// This class uses a ring buffer of size twice as large as buffer_size parameter.
template <uint16_t buffer_size, uint8_t input_channels = 2, uint8_t output_channels = 2>
class Scope {
public:
    /*
     * Both input and output channels can be selected
     */
    void setChannel(uint8_t channel){
        reset();
        if (channel >= total_channels) {
            channel = total_channels - 1;
        }
    }
    /*
     * Update channel
     * 
     * Returns true if enough data is stored for next screen update. This happens when buffer is fully
     * written.
     */
    void update(){
        readChannelData();        
    }

    /*
    bool hasData(uint16_t desired) {
        return buffer.getReadCapacity() >= desired;
    }

    void resync(){
        size_t pos = buffer.getWriteIndex() + buffer_size;
        pos %= buffer_size * 2;
        buffer.setReadIndex(pos);
    }
    */

    int8_t getBufferData(){
        return buffer.pull();
    }

    static constexpr uint16_t getSize(){
        return buffer_size;
    }
private:
    SerialBuffer<buffer_size, int8_t> buffer;
    uint8_t channel;
    static constexpr uint8_t total_channels = input_channels + output_channels;

    void readChannelData(){
        ProgramVector *pv = getProgramVector();
        // We could skip some element in loops below to scale horizontal resolution
        if (channel < input_channels) {
            for (int i = 0; i < 32; i++){
                int32_t sample = (int32_t)(pv->audio_input[input_channels * i + channel]);
                //int32_t sample = pv->audio_input[input_channels * i + channel];
                buffer.push(int8_t(sample >> 16));
            }
        }
        else {
            for (int i = 0; i < 32; i++){
                int32_t* sample = pv->audio_output + (channel - input_channels) * i + (channel - input_channels);
                buffer.push(int8_t(*sample >> 16));
            }
        }
    }

    void reset(){
        buffer.reset();
        buffer.setAll(0U);
    }
};

#endif