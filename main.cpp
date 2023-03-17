#include "mbed.h"
#include "platform/mbed_thread.h"
#include "zigbee.h"

#define MAXIMUM_BUFFER_SIZE                                                 300
#define START_DELIMITER                                                     0x7E

static BufferedSerial xbee(PA_2, PA_3);
DigitalOut rst(PA_0);

DigitalOut myled(LED1);

Thread thread;

void rx_callback(char *buffer){
    int length = 0;
    char c;
    parsedFrame result;
    while(1){
        if(xbee.readable()>0){
            c = 0;
            while(c != START_DELIMITER){
                xbee.read(&c, 1);
            }
            length = 0;
            while(length <= 0){
                result = readFrame(buffer, xbee);
                length = result.length;
                debug("Payload Size: %i\n", length);
            }
            for (int i = 0 ; i<length; i++){
                debug("%02x", buffer[i]);
            }debug("\n");
            if(result.frameID == 0x90){
                debug("%.*s\n", length - 11, buffer + 11);
            }
        }
    }
}

int main() {
 
    char tx_buf[MAXIMUM_BUFFER_SIZE] = {0};
    char rx_buf[MAXIMUM_BUFFER_SIZE] = {0};
    char payload[] = "test~}{~test";

    rst = 0;
    myled = 0;
    thread_sleep_for(100);
    rst = 1;
    thread_sleep_for(100);
    int x = writeFrame(tx_buf, 0xFFFE, 0x0013a20041f217cc, payload, sizeof(payload));
    debug("Arrived 0\n");
    thread.start(callback(rx_callback, rx_buf));
    debug("Setup finished\n");

    while (1) {
        debug("Send message\n");
        myled = 1; 
        xbee.write(tx_buf, x);
        /*for (int i = 0 ; i<x; i++)
            {
                debug("%02x ", tx_buf[i]);
            }debug("\n");*/
        myled = 0; 
        //debug("\n");
        thread_sleep_for(2000);
}
    // 83C9
    // 0013A20041F223B8
}