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
    Serial.println("\n--- Система управления двигателем ---");
    Serial.println("Введите команду в Serial Monitor:");
    Serial.println("  'C' - запустить калибровку.");
    Serial.println("  'H' - вернуться в начальную позицию (Home).");
    Serial.println("  'S' - показать текущий статус (позицию).");
    Serial.println("  <число> - переместиться в указанную координату (например, '1500').");
    Serial.println("------------------------------------");
}

void setup() {
    Serial.begin(115200);
    while (!Serial) { ; } // Ожидание подключения к порту

    motor.begin();
    motor.setSpeed(1600);

    printHelp();
}

void loop() {
    if (Serial.available() > 0) {
        String input = Serial.readStringUntil('\n');
        input.trim(); // Удаляем пробелы по краям

        if (input.equalsIgnoreCase("C")) {
            motor.calibrate(PIN_ENDSTOP_START, PIN_ENDSTOP_END);
        } 
        else if (input.equalsIgnoreCase("H")) {
            Serial.println("Возвращение в начальную точку...");
            motor.moveTo(0);
            Serial.println("Готово.");
        }
        else if (input.equalsIgnoreCase("S")) {
            if (motor.isCalibrated()) {
                Serial.print("Статус: Откалиброван. Текущая позиция: ");
                Serial.println(motor.getCurrentPosition());
            } else {
                Serial.println("Статус: Не откалиброван.");
            }
        }
        else {
            // Пробуем преобразовать ввод в число
            long target_pos = input.toInt();
            Serial.print("Получена команда на перемещение в точку: ");
            Serial.println(target_pos);
            motor.moveTo(target_pos);
            Serial.println("Движение завершено.");
        }
    }
}