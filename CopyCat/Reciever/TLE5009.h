#ifndef TLE5009_h
#define TLE5009_h
#include <Preferences.h>
#include <SimpleFOC.h>
#include "arduino.h"


class TLE5009: public Sensor 
{
    public:
        //**            Constructors            **//
        TLE5009(uint8_t init_pin_VGMR, uint8_t init_pin_SINN, uint8_t init_pin_SINP, uint8_t init_pin_COSN, uint8_t init_pin_COSP);
        
        //**            Methods                 **//
        int calibrate();
        int calibrate(uint16_t steps, uint16_t settle_ms_per_step, bool saveToFlashAfter);
        void setCalibrationEnabled(bool enabled);
        void setDefaultCalibrationValues();
        bool isCalibrationEnabled() const;
        bool hasCalibrationInFlash() const;

        void attachMotor(BLDCMotor* motor);
        float getSensorAngle() override;
        float angleDifference(float a, float b);
        void init() override;
        void readADC();
        void update() override;

        void resetFlash();

        //**            Settings                **//
        uint8_t analogResolution = 12;

    private: 
        //**            Pin assigments          **//
        uint8_t _pin_VGMR = 0; 
        uint8_t _pin_SINN = 0; 
        uint8_t _pin_SINP = 0; 
        uint8_t _pin_COSN = 0; 
        uint8_t _pin_COSP = 0;

        //**            Sampled values          **/
        float _VGMR = 0.0f;
        float _SINN = 0.0f;
        float _SINP = 0.0f;
        float _COSN = 0.0f;
        float _COSP = 0.0f;

        //**            Differential values     **/
        float _SIN = 0.0f;
        float _COS = 0.0f;

        //**            Calculation values      **/
        float _offset_corrX = 0.0f;
        float _offset_corrY = 0.0f;
        float _amplitude_corrX = 1.0f;
        float _amplitude_corrY = 1.0f;
        float _angle_corrX = 0.0f;
        float _angle_corrY = 0.0f;
        float _angle_corr = 0.0f;

        //**            Max/Min for X Y         **/
        float _maxCos = 0.0f;
        float _minCos = 0.0f;
        float _maxSin = 0.0f;
        float _minSin = 0.0f;

        //**            Calibration             **/
        bool _calibrationEnabled = false;
        bool _hasStoredCalibration = false;
        void sampleRawDiff(float& cosDiff, float& sinDiff);

        BLDCMotor* _motor = nullptr;

        //**        Saving calibration in flash **/
        void saveToFlash(float offset_corrX, float offset_corrY, float amplitude_corrX, float amplitude_corrY, float angle_corrX, float angle_corrY);
        void readFromFlash();
        Preferences flashMemory;

        int tellerEnkoder = 0;
};      

#endif