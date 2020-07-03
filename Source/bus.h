#ifndef __BUS_H
#define __BUS_H

#include <stdint.h>

#ifdef __cplusplus
#include "ApplicationSettings.h"
 extern "C" {
#endif

#define BUS_STATUS_IDLE        0x00
#define BUS_STATUS_DISCOVER    0x01
#define BUS_STATUS_CONNECTED   0x02
#define BUS_STATUS_ERROR       0xff

   /* discovery, metadata, etc */
   void bus_setup();
   uint8_t bus_status();
   const char* bus_status_string();
   uint8_t bus_peer_count();
   uint8_t* bus_deviceid();
   void bus_discover();
   void bus_set_input_channel(uint8_t ch);
   /* outgoing: send message over digital bus */
   void bus_tx_parameter(uint8_t pid, int16_t value);
   void bus_tx_button(uint8_t bid, int16_t value);
   void bus_tx_command(uint8_t cmd, int16_t data);
   void bus_tx_message(const char* msg);
   void bus_tx_data(const uint8_t* data, uint16_t size);
   void bus_tx_error(const char* reason);
   /* incoming: callback when message received on digital bus */
   void bus_rx_parameter(uint8_t pid, int16_t value);
   void bus_rx_button(uint8_t bid, int16_t value);
   void bus_rx_command(uint8_t cmd, int16_t data);
   void bus_rx_message(const char* msg); 
   void bus_rx_data(const uint8_t* data, uint16_t size); 
   void bus_rx_error(const char* reason);
   /* low level IO */
   void bus_tx_frame(uint8_t* data);
   void serial_write(uint8_t* data, uint16_t len);
   
#ifdef __cplusplus
}
#endif

#endif /* __BUS_H */
