#ifndef MaplinCtrl_h
#define MaplinCtrl_h

#include "Arduino.h"

class MaplinCtrl {
    public:
        MaplinCtrl(unsigned int tx_pin, unsigned int led_pin);

        void deviceOn(unsigned int channel_id, unsigned int button_id);
        void deviceOff(unsigned int channel_id, unsigned int button_id);
        void simulateButton(int channel, int button, int on);

    private:
        int tx_pin_;
        int led_pin_;

        const static int kPayloadSize;
        const static int kPulseWidthSmall;
        const static long buttons[16];

        void sendData(long payload1, long payload2);
};

#endif /* MaplinCtrl_h */