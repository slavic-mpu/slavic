#ifndef STEPPER_MOTOR_H
#define STEPPER_MOTOR_H

#include <Arduino.h>

class StepperMotor {
public:
    // Конструктор: принимает номера пинов для ENA, DIR, PUL
    StepperMotor(int pin_ena, int pin_dir, int pin_pul);

    // Инициализация пинов
    void begin();

    // Калибровка: поиск начального и конечного положения
    // Принимает пины для концевых выключателей
    void calibrate(int pin_endstop_start, int pin_endstop_end);

    // Движение к абсолютной позиции (в шагах)
    void moveTo(long absolute_pos);

    // Движение на относительное количество шагов
    void move(long relative_pos);

    // Установка скорости вращения в Гц (шагов в секунду)
    void setSpeed(long speed_hz);

    // Получить текущую позицию
    long getCurrentPosition();

    // Проверка, откалиброван ли двигатель
    bool isCalibrated();

private:
    void step(); // Внутренний метод для совершения одного шага
    
    // Пины
    int _pin_ena;
    int _pin_dir;
    int _pin_pul;

    // Состояние
    long _current_pos; // Текущая позиция в шагах
    long _max_pos;     // Максимальная позиция (длина рейки в шагах)
    unsigned long _delay_micros; // Задержка между шагами для контроля скорости
    bool _calibrated;  // Флаг, что калибровка пройдена
};

#endif // STEPPER_MOTOR_H