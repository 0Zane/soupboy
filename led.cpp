#include <Arduino.h>
#include <include/pins.h>


void updatered(bool state){
    digitalWrite(RED,state);
}

void updategreen(bool state){
    digitalWrite(GREEN, state);
}

void updateblue(bool state){
    digitalWrite(BLUE, state);
}