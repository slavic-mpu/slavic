#include <Arduino.h>
#include "StepperMotor.h"

// --- НАСТРОЙКИ ---
// Пины для первого двигателя
const int PIN_X_ENA = 2;
const int PIN_X_DIR = 3;
const int PIN_X_PUL = 4;
const int PIN_X_ENDSTOP = 8;

// Пины для второго двигателя
const int PIN_Y_ENA = 5;
const int PIN_Y_DIR = 6;
const int PIN_Y_PUL = 7;
const int PIN_Y_ENDSTOP = 9;

// Максимальные расстояния в шагах
const long MAX_X_STEPS = 10000;
const long MAX_Y_STEPS = 10000;

// Создаем два двигателя
StepperMotor motorX(PIN_X_ENA, PIN_X_DIR, PIN_X_PUL, false, 80.0, 1.8);
StepperMotor motorY(PIN_Y_ENA, PIN_Y_DIR, PIN_Y_PUL, false, 80.0, 1.8);

// Структура для хранения текущих координат
struct Position {
    long x = 0;
    long y = 0;
} currentPos, targetPos;

void printHelp() {
    Serial.println(F("\n--- 2-осевая система управления (G-code) ---"));
    Serial.println(F("Поддерживаемые G-code команды:"));
    Serial.println(F("  G28        - Калибровка (Home) всех осей"));
    Serial.println(F("  G28 X      - Калибровка только оси X"));
    Serial.println(F("  G28 Y      - Калибровка только оси Y"));
    Serial.println(F("  G1 X100 Y200 - Линейное перемещение в позицию X=100, Y=200"));
    Serial.println(F("  G1 X500    - Перемещение только по оси X"));
    Serial.println(F("  M114       - Показать текущие координаты"));
    Serial.println(F("  M119       - Показать статус концевиков"));
    Serial.println(F("--------------------------------------------"));
}

bool parseGCode(String command) {
    command.toUpperCase();
    command.trim();
    
    // G28 - Home (калибровка)
    if (command.startsWith("G28")) {
        if (command.indexOf("X") >= 0 && command.indexOf("Y") >= 0) {
            // Калибровка обеих осей
            Serial.println(F("Начинаем калибровку обеих осей..."));
            if (!motorX.isBusy() && !motorY.isBusy()) {
                motorX.startCalibration(PIN_X_ENDSTOP, MAX_X_STEPS);
                motorY.startCalibration(PIN_Y_ENDSTOP, MAX_Y_STEPS);
                return true;
            }
        } else if (command.indexOf("X") >= 0) {
            // Калибровка только X
            Serial.println(F("Калибровка оси X..."));
            if (!motorX.isBusy()) {
                motorX.startCalibration(PIN_X_ENDSTOP, MAX_X_STEPS);
                return true;
            }
        } else if (command.indexOf("Y") >= 0) {
            // Калибровка только Y
            Serial.println(F("Калибровка оси Y..."));
            if (!motorY.isBusy()) {
                motorY.startCalibration(PIN_Y_ENDSTOP, MAX_Y_STEPS);
                return true;
            }
        } else {
            // G28 без параметров - калибровка обеих осей
            Serial.println(F("Калибровка всех осей..."));
            if (!motorX.isBusy() && !motorY.isBusy()) {
                motorX.startCalibration(PIN_X_ENDSTOP, MAX_X_STEPS);
                motorY.startCalibration(PIN_Y_ENDSTOP, MAX_Y_STEPS);
                return true;
            }
        }
        Serial.println(F("Ошибка: Один или оба двигателя заняты"));
        return false;
    }
    
    // G1 - Линейное перемещение
    else if (command.startsWith("G1")) {
        if (motorX.isBusy() || motorY.isBusy()) {
            Serial.println(F("Ошибка: Двигатели заняты"));
            return false;
        }
        
        bool hasX = false, hasY = false;
        long newX = currentPos.x, newY = currentPos.y;
        
        // Парсим X координату
        int xIndex = command.indexOf("X");
        if (xIndex >= 0) {
            String xStr = "";
            for (int i = xIndex + 1; i < command.length(); i++) {
                char c = command.charAt(i);
                if (c == ' ' || c == 'Y' || c == 'Z') break;
                if (isdigit(c) || c == '-' || c == '.') xStr += c;
            }
            if (xStr.length() > 0) {
                newX = xStr.toInt();
                hasX = true;
            }
        }
        
        // Парсим Y координату
        int yIndex = command.indexOf("Y");
        if (yIndex >= 0) {
            String yStr = "";
            for (int i = yIndex + 1; i < command.length(); i++) {
                char c = command.charAt(i);
                if (c == ' ' || c == 'X' || c == 'Z') break;
                if (isdigit(c) || c == '-' || c == '.') yStr += c;
            }
            if (yStr.length() > 0) {
                newY = yStr.toInt();
                hasY = true;
            }
        }
        
        if (hasX || hasY) {
            Serial.print(F("Перемещение в: X="));
            Serial.print(newX);
            Serial.print(F(" Y="));
            Serial.println(newY);
            
            bool success = true;
            if (hasX) success &= motorX.moveTo(newX);
            if (hasY) success &= motorY.moveTo(newY);
            
            if (success) {
                targetPos.x = newX;
                targetPos.y = newY;
                Serial.println(F("OK"));
                return true;
            } else {
                Serial.println(F("Ошибка выполнения команды"));
                return false;
            }
        }
    }
    
    // M114 - Показать позицию
    else if (command.startsWith("M114")) {
        Serial.print(F("X:"));
        Serial.print(motorX.getCurrentPosition());
        Serial.print(F(" Y:"));
        Serial.print(motorY.getCurrentPosition());
        
        if (motorX.isBusy() || motorY.isBusy()) {
            Serial.println(F(" (движется)"));
        } else {
            Serial.println();
        }
        return true;
    }
    
    // M119 - Статус концевиков
    else if (command.startsWith("M119")) {
        Serial.print(F("X: "));
        Serial.print(motorX.isCalibrated() ? F("Calibrated") : F("Not calibrated"));
        Serial.print(F(" Y: "));
        Serial.println(motorY.isCalibrated() ? F("Calibrated") : F("Not calibrated"));
        return true;
    }
    
    Serial.println(F("Неизвестная команда"));
    return false;
}

void setup() {
    Serial.begin(115200);
    while (!Serial) { ; } // Ожидание подключения к порту

    motorX.begin();
    motorY.begin();
    
    motorX.setSpeed(1600);
    motorY.setSpeed(1600);

    Serial.println(F("2-осевая система управления инициализирована"));
    printHelp();
}

void loop() {
    motorX.update();
    motorY.update();
    
    currentPos.x = motorX.getCurrentPosition();
    currentPos.y = motorY.getCurrentPosition();
    
    // Обработка команд из Serial
    if (Serial.available() > 0) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        
        if (command.length() > 0) {
            Serial.print(F("Получена команда: "));
            Serial.println(command);
            parseGCode(command);
        }
    }
}