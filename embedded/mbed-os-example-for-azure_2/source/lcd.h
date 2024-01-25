#include "stm32f413h_discovery.h"
#include "stm32f413h_discovery_ts.h"
#include "stm32f413h_discovery_lcd.h"




class TouchScreen {
    public:
    TS_StateTypeDef TS_State = {0};
    void updateScreen(const char *messege);
    void blockUntillTouch();
    int init();
    private:
    bool touched = 0;
};



