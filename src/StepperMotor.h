#ifndef STEPPER_MOTOR_H
#define STEPPER_MOTOR_H

#include <Arduino.h>

enum MotorState {
    IDLE,
    MOVING,
    CALIBRATING_HOME,
    CALIBRATING_END,
    CALIBRATING_RETURN,
    CALIBRATING_PAUSE,
    STEP_HIGH,
    STEP_LOW
};

class StepperMotor {
public:
    // Конструктор: принимает номера пинов для ENA, DIR, PUL
    StepperMotor(uint8_t pin_ena, uint8_t pin_dir, uint8_t pin_pul);

    // Инициализация пинов
    void begin();

    // Обновление состояния двигателя - должно вызываться в loop()
    void update();

    // Калибровка: поиск начального и конечного положения
    // Принимает пины для концевых выключателей
    void startCalibration(uint8_t pin_endstop_start, uint8_t pin_endstop_end);

    // Движение к абсолютной позиции (в шагах)
    bool moveTo(long absolute_pos);

    // Движение на относительное количество шагов
    bool move(long relative_pos);

    // Проверка, выполняется ли операция
    bool isBusy();

    // Установка скорости вращения в Гц (шагов в секунду)
    void setSpeed(long speed_hz);

    // Включение драйвера (подача питания на мотор)
    void enable();

    // Отключение драйвера (снятие питания)
    void disable();

    // Получить текущую позицию
    long getCurrentPosition();

    // Проверка, откалиброван ли двигатель
    bool isCalibrated();

private:
    void stepHigh(); // Установить пин PUL в HIGH
    void stepLow();  // Установить пин PUL в LOW
    void updateMovement(); // Обновление движения
    void updateCalibration(); // Обновление калибровки
    
    // Пины
    uint8_t _pin_ena;
    uint8_t _pin_dir;
    uint8_t _pin_pul;

    // Пины концевиков
    uint8_t _pin_endstop_start;
    uint8_t _pin_endstop_end;

    // Состояние
    MotorState _state;
    int32_t _current_pos; // Текущая позиция в шагах
    int32_t _max_pos;     // Максимальная позиция (длина рейки в шагах)
    int32_t _target_pos;  // Целевая позиция
    int32_t _steps_remaining; // Количество оставшихся шагов
    
    // Тайминги
    uint16_t _delay_micros; // Задержка между шагами для контроля скорости
    uint32_t _last_step_time; // Время последнего шага
    uint32_t _pause_start_time; // Время начала паузы
    
    // Флаги
    bool _calibrated;  // Флаг, что калибровка пройдена
    bool _pulse_state; // Состояние импульса (HIGH/LOW)
};

#endif