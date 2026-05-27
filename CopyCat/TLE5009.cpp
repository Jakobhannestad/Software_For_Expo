
#include "arduino.h"
#include <SimpleFOC.h>
#include <TLE5009.h>

#define RW_mode false
#define RO_mode true

TLE5009::TLE5009(uint8_t init_pin_VGMR, uint8_t init_pin_SINN, uint8_t init_pin_SINP, uint8_t init_pin_COSN, uint8_t init_pin_COSP)
{
    _pin_VGMR = init_pin_VGMR; 
    _pin_SINN = init_pin_SINN; 
    _pin_SINP = init_pin_SINP; 
    _pin_COSN = init_pin_COSN; 
    _pin_COSP = init_pin_COSP;
}

void TLE5009::setDefaultCalibrationValues() 
{
    _offset_corrX = 0.0f;
    _offset_corrY = 0.0f;
    _amplitude_corrX = 1.0f;
    _amplitude_corrY = 1.0f;
    _angle_corrX = 0.0f;
    _angle_corrY = 0.0f;
    _angle_corr = 0.0f;
}

void TLE5009::setCalibrationEnabled(bool enabled)
{
    _calibrationEnabled = enabled;

    if (!_calibrationEnabled)
    {
        setDefaultCalibrationValues();
    }
}

void TLE5009::attachMotor(BLDCMotor* motor)
{
    _motor = motor;
}

bool TLE5009::isCalibrationEnabled() const
{ 
    return _calibrationEnabled;
}

bool TLE5009::hasCalibrationInFlash() const
{ 
    return _hasStoredCalibration;
}

void TLE5009::init()
{ 
    analogReadResolution(analogResolution);

    if (_calibrationEnabled)
    {
        readFromFlash();
    }
    else
    {
        setDefaultCalibrationValues();
        _hasStoredCalibration = false;

        if (Serial)
        {
            Serial.println("ENK: Calibration disabled, using default correction values.");
        }
    }
}

void TLE5009::readADC()
{
    const int samples = 8;

    float vgmr = 0.0f;
    float sinn = 0.0f;
    float sinp = 0.0f;
    float cosn = 0.0f;
    float cosp = 0.0f;

    for (int i = 0; i < samples; i++)
    {
        vgmr += analogReadMilliVolts(_pin_VGMR);
        sinn += analogReadMilliVolts(_pin_SINN);
        sinp += analogReadMilliVolts(_pin_SINP);
        cosn += analogReadMilliVolts(_pin_COSN);
        cosp += analogReadMilliVolts(_pin_COSP);
    }

    _VGMR = vgmr / samples;
    _SINN = sinn / samples;
    _SINP = sinp / samples;
    _COSN = cosn / samples;
    _COSP = cosp / samples;
}
/**
 * @brief returns angle of the motor
 * 
 * The method that simpleFOC library calls when it needs the angle of the motor, the angles must be between [0, 2*PI]
 * 
 * @returns angle of motor in radians
*/

void TLE5009::sampleRawDiff(float& cosDiff, float& sinDiff)
{
    readADC();
    cosDiff = _COSP - _COSN;
    sinDiff = _SINP - _SINN;
}

float TLE5009::getSensorAngle()
{
    readADC();

    _COS = _COSP - _COSN;
    _SIN = _SINP - _SINN;

    if (_calibrationEnabled)
    {
        _COS = _COS - _offset_corrX;
        _SIN = _SIN - _offset_corrY;

        if (fabsf(_amplitude_corrX) > 1e-6f) _COS = _COS / _amplitude_corrX;
        if (fabsf(_amplitude_corrY) > 1e-6f) _SIN = _SIN / _amplitude_corrY;

        _angle_corr = _angle_corrX - _angle_corrY;

        float c = cosf(-_angle_corr);
        if (fabsf(c) > 1e-6f)
        {
            _SIN = (_SIN - _COS * sinf(-_angle_corr)) / c;
        }
    }

    float angle = atan2f(_SIN, _COS) - _angle_corrX;

    while (angle < 0.0f) angle = angle + _2PI;
    while (angle >= _2PI) angle = angle - _2PI;

    if (tellerEnkoder == 10000)
    {
        Serial.print("ENK: Angle from encoder:");
        Serial.println(angle);
        tellerEnkoder = 0;
    }
    else
    {
        tellerEnkoder++;
    }
    return angle; 
}

void TLE5009::update()
{
    Sensor::update();
}

float TLE5009::angleDifference(float a, float b)
{ 
    float diff = a - b;

    while (diff > PI) diff -= _2PI;
    while (diff < -PI) diff += _2PI;

    return diff;
}

int TLE5009::calibrate()
{
    return calibrate(360, 5, true);
}

int TLE5009::calibrate(uint16_t steps, uint16_t settle_ms_per_step, bool saveToFlashAfter)
{
    if (!_calibrationEnabled)
    {
        setDefaultCalibrationValues();

        if (Serial)
        {
            Serial.println("ENK: Calibration is disabled. Using default correction values.");
        }
        return 1;
    }

    if (_motor == nullptr)
    {
        if (Serial)
        {
            Serial.println("ENK: Calibration failed: no motor attached.");
        }
        return 0;
    }

    if (steps < 32)
    {
        steps = 32;
    }

    if (Serial)
    {
        Serial.println("ENK: Starting TLE5009 calibration...");
    }

    setDefaultCalibrationValues();

    _maxCos = -1e9f;
    _minCos =  1e9f;
    _maxSin = -1e9f;
    _minSin =  1e9f;

    float voltage_align = _motor->voltage_sensor_align;

    if (voltage_align <= 0.0f)
    {
        voltage_align = _motor->voltage_limit*0.2f;
    }

    float cosRaw, sinRaw;
    sampleRawDiff(cosRaw, sinRaw);

    float startAngle = atan2f(sinRaw, cosRaw);
    float maxMovement = 0.0f;

    for (uint16_t i = 0; i < steps; i++)
    {
        float mechanical_angle = (_2PI * i) / (float)steps;
        float electrical_angle = _3PI_2 + _motor->pole_pairs * mechanical_angle;
    
        _motor->setPhaseVoltage(voltage_align, 0.0f, electrical_angle);
        delay(settle_ms_per_step);

        sampleRawDiff(cosRaw, sinRaw);

        float currentAngle = atan2f(sinRaw, cosRaw);

        float moved = fabsf(angleDifference(currentAngle, startAngle));

        if (moved > maxMovement)
        {
            maxMovement = moved;
        }

        if (cosRaw > _maxCos) _maxCos = cosRaw;
        if (cosRaw < _minCos) _minCos = cosRaw;
        if (sinRaw > _maxSin) _maxSin = sinRaw;
        if (sinRaw < _minSin) _minSin = sinRaw;
    }

    if (maxMovement < 0.2f)
    {
        Serial.println("ENK: Calibration failed: motor did not move enough.");
        Serial.print("ENK: Max movement: ");
        Serial.println(maxMovement);

        _motor->setPhaseVoltage(0, 0, 0);
        return 0;
    }

    _offset_corrX = (_maxCos + _minCos) * 0.5f;
    _offset_corrY = (_maxSin + _minSin) * 0.5f;

    float ampX = (_maxCos - _minCos) * 0.5f;
    float ampY = (_maxSin - _minSin) * 0.5f;

    if (ampX < 1e-6f || ampY < 1e-6f)
    {
        _motor->setPhaseVoltage(0, 0, 0);

        if (Serial)
        {
            Serial.println("ENK: Calibration failed: invalid signal amplitudes.");
        }
        return 0;
    }

    float avgAmp = (ampX + ampY) * 0.5f;
    _amplitude_corrX = ampX / avgAmp;
    _amplitude_corrY = ampY / avgAmp;

    for (uint16_t i = 0; i < steps; i++)
    {
        //-
        float mechanical_angle = (_PI/2 * i) / (float)steps;
        float electrical_angle = _3PI_2 + _motor->pole_pairs * mechanical_angle;

        _motor->setPhaseVoltage(voltage_align, 0.0f, electrical_angle);
        delay(settle_ms_per_step);
    }

        for (uint16_t i = 0; i < steps; i++)
    {
        //-
        float mechanical_angle = -(_PI/2 * i) / (float)steps;
        float electrical_angle = _3PI_2 + _motor->pole_pairs * mechanical_angle;

        _motor->setPhaseVoltage(voltage_align, 0.0f, electrical_angle);
        delay(settle_ms_per_step);
    }

    float xySum = 0.0f;

    sampleRawDiff(cosRaw, sinRaw);

    for (uint16_t i = 0; i < steps; i++)
    {
        //-
        float mechanical_angle = -(_2PI * i) / (float)steps;
        float electrical_angle = _3PI_2 + _motor->pole_pairs * mechanical_angle;

        _motor->setPhaseVoltage(voltage_align, 0.0f, electrical_angle);
        delay(settle_ms_per_step);

        sampleRawDiff(cosRaw, sinRaw);

        float x = (cosRaw - _offset_corrX) / _amplitude_corrX;
        float y = (sinRaw - _offset_corrY) / _amplitude_corrY;

        xySum += x * y;
    }

    float meanXY = xySum / (float)steps;
    float phaseError = asinf(constrain(2.0f * meanXY, -1.0f, 1.0f));

    _angle_corrX = 0.0f;
    _angle_corrY = -phaseError;
    _angle_corr = _angle_corrX - _angle_corrY;

    _motor->setPhaseVoltage(0, 0, 0);
    delay(200);

    if (saveToFlashAfter) 
    {
        saveToFlash(_offset_corrX, _offset_corrY,
                    _amplitude_corrX, _amplitude_corrY,
                    _angle_corrX, _angle_corrY);
        _hasStoredCalibration = true;
    }

    if (Serial)
    {
        Serial.println("ENK: Calibration finished.");
        Serial.print("ENK: offset_corrX: "); Serial.println(_offset_corrX);
        Serial.print("ENK: offset_corrY: "); Serial.println(_offset_corrY);
        Serial.print("ENK: amplitude_corrX: "); Serial.println(_amplitude_corrX);
        Serial.print("ENK: amplitude_corrY: "); Serial.println(_amplitude_corrY);
        Serial.print("ENK: angle_corrX: "); Serial.println(_angle_corrX);
        Serial.print("ENK: angle_corrY: "); Serial.println(_angle_corrY);

    }

    return 1;
    }

void TLE5009::saveToFlash(float offset_corrX, float offset_corrY, float amplitude_corrX, float amplitude_corrY, float angle_corrX, float angle_corrY)
{
    flashMemory.begin("calibrationValues", RW_mode);

    flashMemory.putFloat("offset_corrX", offset_corrX);
    flashMemory.putFloat("offset_corrY", offset_corrY);
    flashMemory.putFloat("amplitude_corrX",  amplitude_corrX);
    flashMemory.putFloat("amplitude_corrY", amplitude_corrY);
    flashMemory.putFloat("angle_corrX", angle_corrX);
    flashMemory.putFloat("angle_corrY", angle_corrY);

    flashMemory.end();
}

void TLE5009::readFromFlash()
{
    flashMemory.begin("calibrationValues", RO_mode);

    bool flashMemoryExsist = flashMemory.isKey("offset_corrX");

    if (flashMemoryExsist)
    {
        _offset_corrX = flashMemory.getFloat("offset_corrX", 0.0f);
        _offset_corrY = flashMemory.getFloat("offset_corrY", 0.0f);
        _amplitude_corrX = flashMemory.getFloat("amplitude_corrX", 1.0f);
        _amplitude_corrY = flashMemory.getFloat("amplitude_corrY", 1.0f);
        _angle_corrX = flashMemory.getFloat("angle_corrX", 0.0f);
        _angle_corrY = flashMemory.getFloat("angle_corrY", 0.0f);
        _angle_corr = _angle_corrX - _angle_corrY;
        _hasStoredCalibration = true;

        flashMemory.end();

        if (Serial)
        {
            Serial.println("Successfully loaded calibration values from flash.");
        }
        
    }   
    else
    {
        flashMemory.end();

        flashMemory.begin("calibrationValues", RW_mode);
        flashMemory.putFloat("offset_corrX", 0.0f);
        flashMemory.putFloat("offset_corrY", 0.0f);
        flashMemory.putFloat("amplitude_corrX", 1.0f);
        flashMemory.putFloat("amplitude_corrY", 1.0f);
        flashMemory.putFloat("angle_corrX", 0.0f);
        flashMemory.putFloat("angle_corrY", 0.0f);
        flashMemory.end();

        setDefaultCalibrationValues();
        _hasStoredCalibration = false;

        if (Serial)
        {
            Serial.println("No saved calibration found. Default calibration values written to flash.");
        }
    }
}
    
void TLE5009::resetFlash()
{
    flashMemory.begin("calibrationValues", RW_mode);

    flashMemory.putFloat("offset_corrX", 0.0f);
    flashMemory.putFloat("offset_corrY", 0.0f);
    flashMemory.putFloat("amplitude_corrX", 1.0f);
    flashMemory.putFloat("amplitude_corrY", 1.0f);
    flashMemory.putFloat("angle_corrX", 0.0f);
    flashMemory.putFloat("angle_corrY", 0.0f);

    flashMemory.end();

    setDefaultCalibrationValues();
    _hasStoredCalibration = false;

    if (Serial)
    {
        Serial.println("Reset calibration values to default settings.");
    }
}