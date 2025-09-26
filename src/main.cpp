#include <Arduino.h>
#include "StepperMotor.h"

// --- НАСТРОЙКИ ---
// Пины для драйвера шагового двигателя
const int PIN_ENA = 2;
const int PIN_DIR = 3;
const int PIN_PUL = 4;

// Пины для концевых выключателей
const int PIN_ENDSTOP_START = 8;
const int PIN_ENDSTOP_END = 9;

StepperMotor motor(PIN_ENA, PIN_DIR, PIN_PUL);

void printHelp() {
    Serial.println(F("\n--- Система управления двигателем ---"));
    Serial.println(F("Введите команду в Serial Monitor:"));
    Serial.println(F("  'C' - запустить калибровку."));
    Serial.println(F("  'H' - вернуться в начальную позицию (Home)."));
    Serial.println(F("  'S' - показать текущий статус (позицию)."));
    Serial.println(F("  <число> - переместиться в указанную координату (например, '1500')."));
    Serial.println(F("------------------------------------"));
}

void setup() {
    Serial.begin(115200);
    while (!Serial) { ; } // Ожидание подключения к порту

    motor.begin();
    motor.setSpeed(1600);

    printHelp();
}

void loop() {
    motor.update(); //Постоянно обновляем состояние двигателя
    
    if (Serial.available() > 0) {
        String input = Serial.readStringUntil('\n');
        input.trim(); // Удаляем пробелы по краям

        if (input.equalsIgnoreCase("C")) {
            if (motor.isBusy()) {
                Serial.println(F("Двигатель занят. Дождитесь завершения текущей операции."));
            } else {
                motor.startCalibration(PIN_ENDSTOP_START, PIN_ENDSTOP_END);
            }
        } 
        else if (input.equalsIgnoreCase("H")) {
            if (motor.isBusy()) {
                Serial.println(F("Двигатель занят. Дождитесь завершения текущей операции."));
            } else {
                Serial.println(F("Возвращение в начальную точку..."));
                if (motor.moveTo(0)) {
                    Serial.println(F("Команда принята."));
                }
            }
        }
        else if (input.equalsIgnoreCase("S")) {
            if (motor.isCalibrated()) {
                Serial.print(F("Статус: Откалиброван. Текущая позиция: "));
                Serial.print(motor.getCurrentPosition());
                if (motor.isBusy()) {
                    Serial.println(F(" (движется)"));
                } else {
                    Serial.println(F(" (готов)"));
                }
            } else {
                Serial.print(F("Статус: Не откалиброван"));
                if (motor.isBusy()) {
                    Serial.println(F(" (выполняется операция)"));
                } else {
                    Serial.println();
                }
            }
        }
        else {
            // Пробуем преобразовать ввод в число
            long target_pos = input.toInt();
            if (motor.isBusy()) {
                Serial.println(F("Двигатель занят. Дождитесь завершения текущей операции."));
            } else {
                Serial.print(F("Получена команда на перемещение в точку: "));
                Serial.println(target_pos);
                if (motor.moveTo(target_pos)) {
                    Serial.println(F("Команда принята."));
                } else {
                    Serial.println(F("Ошибка выполнения команды."));
                }
            }
        }
    }
}