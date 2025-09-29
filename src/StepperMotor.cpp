#include "StepperMotor.h"

StepperMotor::StepperMotor(uint8_t pin_ena, uint8_t pin_dir, uint8_t pin_pul, 
    bool reverse, float steps_per_mm, float steps_per_degre) {
    _pin_ena = pin_ena;
    _pin_dir = pin_dir;
    _pin_pul = pin_pul;
    _reverse = reverse;
    _steps_per_mm = steps_per_mm;
    _steps_per_degre = steps_per_degre;
    _current_pos = 0;
    _max_pos = 0;
    _target_pos = 0;
    _steps_remaining = 0;
    _calibrated = false;
    _state = IDLE;
    _pulse_state = false;
    _last_step_time = 0;
    _pause_start_time = 0;
    setSpeed(1000); // Скорость по умолчанию 1000 Гц
}

StepperMotor::StepperMotor(uint8_t pin_ena, uint8_t pin_dir, uint8_t pin_pul, uint8_t pin_endstop, 
    bool reverse, float steps_per_mm, float steps_per_degre) {
    _pin_ena = pin_ena;
    _pin_dir = pin_dir;
    _pin_pul = pin_pul;
    _pin_endstop_start = pin_endstop;
    _reverse = reverse;
    _steps_per_mm = steps_per_mm;
    _steps_per_degre = steps_per_degre;
    _current_pos = 0;
    _max_pos = 0;
    _target_pos = 0;
    _steps_remaining = 0;
    _calibrated = false;
    _state = IDLE;
    _pulse_state = false;
    _last_step_time = 0;
    _pause_start_time = 0;
    setSpeed(1000); // Скорость по умолчанию 1000 Гц
}

void StepperMotor::begin() {
    pinMode(_pin_ena, OUTPUT);
    pinMode(_pin_dir, OUTPUT);
    pinMode(_pin_pul, OUTPUT);
    disable(); // Изначально драйвер выключен
    digitalWrite(_pin_pul, LOW);  // Изначально пин импульса в LOW
}

void StepperMotor::setSpeed(long speed_hz) {
    if (speed_hz > 0) {
        _delay_micros = 1000000 / speed_hz / 2;
    }
}

void StepperMotor::enable() {
    digitalWrite(_pin_ena, LOW);
}

void StepperMotor::disable() {
    digitalWrite(_pin_ena, HIGH);
}

void StepperMotor::stepHigh() {
    digitalWrite(_pin_pul, HIGH);
    _pulse_state = true;
    _last_step_time = micros();
}

void StepperMotor::stepLow() {
    digitalWrite(_pin_pul, LOW);
    _pulse_state = false;
    _last_step_time = micros();
}

bool StepperMotor::isBusy() {
    return _state != IDLE;
}

bool StepperMotor::moveTo(long absolute_pos) {
    if (!_calibrated) {
        Serial.println(F("Ошибка: Двигатель не откалиброван. Выполните калибровку."));
        return false;
    }
    
    if (_state != IDLE) {
        Serial.println(F("Ошибка: Двигатель занят."));
        return false;
    }
    
    // Ограничиваем движение пределами рейки
    if (absolute_pos < 0) absolute_pos = 0;
    if (absolute_pos > _max_pos) absolute_pos = _max_pos;
    
    return move(absolute_pos - _current_pos);
}

bool StepperMotor::move(long relative_pos) {
    if (_state != IDLE && _state != CALIBRATING_RETURN) {
        return false;
    }
    
    if (relative_pos == 0) {
        return true;
    }
    
    _target_pos = _current_pos + relative_pos;
    _steps_remaining = abs(relative_pos);
    
    enable(); // Включаем драйвер
    
    // Определяем направление
    if (relative_pos > 0) {
        digitalWrite(_pin_dir, HIGH);
    } else {
        digitalWrite(_pin_dir, LOW);
    }
    
    // Сохраняем информацию о том, что это движение в рамках калибровки
    if (_state == CALIBRATING_RETURN) {
        // Остаемся в состоянии CALIBRATING_RETURN
    } else {
        _state = MOVING;
    }
    
    _last_step_time = micros();
    return true;
}

void StepperMotor::startCalibration(uint8_t pin_endstop_start, long max_distance_steps) {
    if (_state != IDLE) {
        Serial.println(F("Ошибка: Двигатель занят."));
        return;
    }
    
    _pin_endstop_start = pin_endstop_start;
    _max_pos = max_distance_steps;  // Устанавливаем максимальную позицию
    
    pinMode(_pin_endstop_start, INPUT_PULLUP);

    Serial.println(F("Начало калибровки..."));
    Serial.println(F("Движение к начальному концевику..."));
    Serial.print(F("Максимальное расстояние установлено: "));
    Serial.print(_max_pos);
    Serial.println(F(" шагов."));
    
    enable();
    digitalWrite(_pin_dir, LOW); // Направление к началу
    
    _state = CALIBRATING_HOME;
    _last_step_time = micros();
    _calibrated = false;
}

void StepperMotor::update() {
    static bool was_calibrating_return = false;
    
    // Запоминаем, была ли калибровка
    if (_state == CALIBRATING_RETURN) {
        was_calibrating_return = true;
    }
    
    switch (_state) {
        case IDLE:
            // Если только что завершилась калибровка
            if (was_calibrating_return) {
                Serial.println(F("Калибровка завершена."));
                was_calibrating_return = false;
            }
            break;
            
        case MOVING:
            updateMovement();
            break;
            
        case CALIBRATING_HOME:
        case CALIBRATING_PAUSE:
            updateCalibration();
            break;
            
        case CALIBRATING_RETURN:
            updateMovement(); // Используем тот же механизм для движения
            break;
            
        case STEP_HIGH:
        case STEP_LOW:
            updateMovement();
            break;
    }
}

void StepperMotor::updateMovement() {
    unsigned long current_time = micros();
    
    if (_state == MOVING || _state == STEP_HIGH || _state == STEP_LOW || _state == CALIBRATING_RETURN) {
        if (current_time - _last_step_time >= _delay_micros) {
            if (_state == MOVING || _state == STEP_LOW || _state == CALIBRATING_RETURN) {
                // Время для HIGH импульса
                stepHigh();
                _state = STEP_HIGH;
            } else if (_state == STEP_HIGH) {
                // Время для LOW импульса
                stepLow();
                _steps_remaining--;
                
                // Обновляем позицию
                if (_target_pos > _current_pos) {
                    _current_pos++;
                } else {
                    _current_pos--;
                }
                
                if (_steps_remaining > 0) {
                    _state = STEP_LOW;
                } else {
                    // Движение завершено
                    _state = IDLE;
                    disable(); // Выключаем драйвер
                }
            }
        }
    }
}

void StepperMotor::updateCalibration() {
    unsigned long current_time = micros();
    
    switch (_state) {
        case CALIBRATING_HOME:
            if (digitalRead(_pin_endstop_start) == LOW) {
                // Концевик сработал
                _current_pos = 0;
                Serial.println(F("Начальная точка найдена. Установлена позиция 0."));
                Serial.print(F("Длина рейки установлена: "));
                Serial.print(_max_pos);
                Serial.println(F(" шагов."));
                
                _calibrated = true;
                disable();
                _state = CALIBRATING_PAUSE;
                _pause_start_time = millis();
            } else {
                // Продолжаем движение
                if (current_time - _last_step_time >= _delay_micros) {
                    if (!_pulse_state) {
                        stepHigh();
                    } else {
                        stepLow();
                    }
                }
            }
            break;
            
        case CALIBRATING_PAUSE:
            if (millis() - _pause_start_time >= 500) {
                // Пауза закончилась, калибровка завершена
                Serial.println(F("Калибровка завершена."));
                _state = IDLE;
            }
            break;
            
        case CALIBRATING_RETURN:
            // Состояние изменится на IDLE автоматически в updateMovement
            // когда движение будет завершено
            break;
    }
}

long StepperMotor::getCurrentPosition() {
    return _current_pos;
}

bool StepperMotor::isCalibrated() {
    return _calibrated;
}