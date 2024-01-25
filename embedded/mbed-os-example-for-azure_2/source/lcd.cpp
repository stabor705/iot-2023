#include "lcd.h"


int TouchScreen::init(){
    BSP_LCD_Init();

    /* Touchscreen initialization */
    if (BSP_TS_Init(BSP_LCD_GetXSize(), BSP_LCD_GetYSize()) == TS_ERROR) {
        return -1;
    }

    BSP_LCD_SetFont(&Font16);
    return 0;
}

//no padding ofc 
void TouchScreen::updateScreen(const char * messege){
    BSP_LCD_Clear(LCD_COLOR_GREEN);
    BSP_LCD_SetBackColor(LCD_COLOR_GREEN);
    BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
    BSP_LCD_DisplayStringAt(0, LINE(8), (uint8_t *)messege, CENTER_MODE);
}


void TouchScreen::blockUntillTouch(){
        while (!touched) {
        BSP_TS_GetState(&TS_State);
        if(TS_State.touchDetected) {
            touched = 1;
        }
    }

}