#include "motor.h"


Motor::Motor(uint8_t motor_pin)
    : motor_pin_(motor_pin)
    , power_(0)
    , motor_delay_(0)
    , mode_(mComfort)
    {
    }



void Motor::begin() {
    motor_.attach(motor_pin_);
    motor_.writeMicroseconds(800);
}



void Motor::update() {

}



void Motor::setPower(uint8_t value) {
    if (power_ == value)
        return;
    
    if (int(value - power_) > int(motor_spec_[mode_][0])) {
        if (millis() - motor_delay_ > motor_spec_[mode_][1]) {
            motor_delay_ = millis();
            power_ += motor_spec_[mode_][0];
        }
    }
    else {
        power_ = value;
    }

    Serial.print(mode_); Serial.print("    "); Serial.println(power_);
    motor_.writeMicroseconds(map(power_, 0, 255, 800, motor_spec_[mode_][2]));
}



void Motor::setMode(Mode mode) {
    if (power_ == 0)
        mode_ = mode;       
}



void Motor::switchMode(bool clockwise) { // Переключение режимов
    if (power_ == 0) { // Если курок газа отпущен
        int n = static_cast<int>(mode_);

        n += clockwise ? 1 : -1; // Если по часовой стрелке, то ставим следующий

        if ( n > 3) {
            n = 1;
        }
        if ( n < 1 ) {
            n = 1;
        }

        mode_ = static_cast<Mode>(n);        
    }
}