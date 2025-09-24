#include "StepperMotor.h"

StepperMotor::StepperMotor(int pin_ena, int pin_dir, int pin_pul) {
    _pin_ena = pin_ena;
    _pin_dir = pin_dir;
    _pin_pul = pin_pul;
    _current_pos = 0;
    _max_pos = 0;
    _calibrated = false;
    setSpeed(1000); // Скорость по умолчанию 1000 Гц
}

void StepperMotor::begin() {
    pinMode(_pin_ena, OUTPUT);
    pinMode(_pin_dir, OUTPUT);
    pinMode(_pin_pul, OUTPUT);
    digitalWrite(_pin_ena, HIGH); // Изначально драйвер выключен
}

void StepperMotor::setSpeed(long speed_hz) {
    if (speed_hz > 0) {
        _delay_micros = 1000000 / speed_hz / 2;
    }
}

void StepperMotor::step() {
    digitalWrite(_pin_pul, HIGH);
    delayMicroseconds(_delay_micros);
    digitalWrite(_pin_pul, LOW);
    delayMicroseconds(_delay_micros);
}

void StepperMotor::moveTo(long absolute_pos) {
    if (!_calibrated) {
        Serial.println("Ошибка: Двигатель не откалиброван. Выполните калибровку.");
        return;
    }
    // Ограничиваем движение пределами рейки
    if (absolute_pos < 0) absolute_pos = 0;
    if (absolute_pos > _max_pos) absolute_pos = _max_pos;
    
    move(absolute_pos - _current_pos);
}

void StepperMotor::move(long relative_pos) {
    digitalWrite(_pin_ena, LOW); // Включаем драйвер
    // TODO Убрать delay
    delayMicroseconds(10);       // Небольшая задержка

    // Определяем направление
    if (relative_pos > 0) {
        digitalWrite(_pin_dir, HIGH); // Установите свое направление (HIGH или LOW)
    } else {
        digitalWrite(_pin_dir, LOW);
    }

    // Выполняем шаги
    for (long i = 0; i < abs(relative_pos); i++) {
        step();
    }
    _current_pos += relative_pos; // Обновляем позицию

    digitalWrite(_pin_ena, HIGH); // Выключаем драйвер для экономии энергии
}

void StepperMotor::calibrate(int pin_endstop_start, int pin_endstop_end) {
    pinMode(pin_endstop_start, INPUT_PULLUP);
    pinMode(pin_endstop_end, INPUT_PULLUP);

    Serial.println("Начало калибровки...");

    // --- Поиск начальной точки (нуля) ---
    Serial.println("Движение к начальному концевику...");
    digitalWrite(_pin_ena, LOW);
    digitalWrite(_pin_dir, LOW); // Направление к началу

    // Двигаемся, пока не сработает концевик
    // digitalRead вернет LOW при срабатывании, если используется INPUT_PULLUP
    while (digitalRead(pin_endstop_start) == HIGH) {
        step();
    }
    _current_pos = 0;
    Serial.println("Начальная точка найдена. Установлена позиция 0.");
    
    // TODO Убрать delay
    delay(500); // Пауза

    // --- Поиск конечной точки ---
    Serial.println("Движение к конечному концевику...");
    digitalWrite(_pin_dir, HIGH); // Направление к концу

    // Двигаемся, пока не сработает второй концевик
    while (digitalRead(pin_endstop_end) == HIGH) {
        step();
        _current_pos++; // Считаем шаги
    }
    _max_pos = _current_pos;
    Serial.print("Конечная точка найдена. Длина рейки: ");
    Serial.print(_max_pos);
    Serial.println(" шагов.");

    _calibrated = true;
    digitalWrite(_pin_ena, HIGH); // Отключаем драйвер
    
    // TODO Убрать delay    
    delay(500);
    
    // Возвращаемся в начало
    Serial.println("Возвращение в начальную позицию.");
    moveTo(0);
    Serial.println("Калибровка завершена.");
}

long StepperMotor::getCurrentPosition() {
    return _current_pos;
}

bool StepperMotor::isCalibrated() {
    return _calibrated;
}