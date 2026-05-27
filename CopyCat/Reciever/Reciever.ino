#include <SimpleFOC.h>
#include <Pin.h>
#include <TLE5009.h>
#include <SparkFun_STUSB4500.h>

STUSB4500 usb;

// ============================================================================
//                        PWM CONFIGURATION
// ============================================================================
const uint32_t pwmFrequency = 22000;    // PWM frequency in Hz for the gate driver


// ============================================================================
//                        MOTOR CONFIGURATION
// ============================================================================
const uint8_t supplyVoltage   = 15;     // Input power supply voltage [V]
const uint8_t maxMotorVoltage = 15;     // Maximum voltage limit allowed for the motor [V]
const uint8_t numberOfPoles   = 7;      // Number of motor pole pairs


// ============================================================================
//                        USB-C PD CONFIGURATION
// ============================================================================
const bool    USE_USB_PD   = true;      // Set false to skip USB-C PD setup
const uint8_t PD_PDO       = 3;         // PDO number to use (1-3)
const float   PD_VOLTAGE   = 9.0;      // Requested voltage [V] (5-20V)
const float   PD_CURRENT   = 2.0;       // Requested current [A] (0-5A)


// ============================================================================
//                        MOTOR & DRIVER OBJECTS
// ============================================================================
BLDCMotor motor = BLDCMotor(numberOfPoles);

BLDCDriver6PWM driver = BLDCDriver6PWM(
    Pin::PWM_HA, Pin::PWM_LA,
    Pin::PWM_HB, Pin::PWM_LB,
    Pin::PWM_HC, Pin::PWM_LC
);


// ============================================================================
//                        SENSOR OBJECTS
// ============================================================================
TLE5009 encoder = TLE5009(
    Pin::ENK_Vgmr,
    Pin::ENK_SinN, Pin::ENK_SinP,
    Pin::ENK_CosN, Pin::ENK_CosP
);

LowsideCurrentSense current_sense = LowsideCurrentSense(150.0f, Pin::MC_SOA, Pin::MC_SOB, Pin::MC_SOC);


// ============================================================================
//                        COMMANDER (Serial tuning)
// ============================================================================
Commander command = Commander(Serial);
void onMotor(char* cmd) { command.motor(&motor, cmd); }


// ============================================================================
//                        SETUP
// ============================================================================
void setup() {
    // ========== Serial & Debug Init ==========
    Serial.begin(115200);
    delay(500);
    Serial.println("Starting up...");
    SimpleFOCDebug::enable(&Serial);

    pinMode(Pin::PDA_RST, OUTPUT);

    // ========== DRV8316 Gate Driver Pins ==========
    pinMode(Pin::MC_DRVOFF, OUTPUT);      
    digitalWrite(Pin::MC_DRVOFF, LOW);      // Gate driver active

    pinMode(Pin::MC_nSLEEP, OUTPUT);
    digitalWrite(Pin::MC_nSLEEP, HIGH);     // Wake up the chip

    pinMode(Pin::MC_nFAULT, INPUT);         // Fault input interrupt/status pin

    delay(20);

    // ========== USB Power Delivery (STUSB4500) ==========
    if (USE_USB_PD) {
        Wire.begin(Pin::I2C_SDA, Pin::I2C_SCL);
        delay(500);

        if (!usb.begin()) {
            Serial.println("Error: Could not find STUSB4500!");
            while (1);
        }
        Serial.println("STUSB4500 connected!");

        usb.setPdoNumber(PD_PDO);
        usb.setVoltage(PD_PDO, PD_VOLTAGE);
        usb.setCurrent(PD_PDO, PD_CURRENT);
        usb.write();             // Write to NVM
        usb.read();              // Read back to verify
        usb.softReset();

        uint8_t activePDO = usb.getPdoNumber();
        Serial.printf("PD enabled - PDO %d: %.0fV\n", activePDO, usb.getVoltage(activePDO));
        Wire.end();
    }
    else {
        Serial.println("USB-C PD disabled");
    }


    // ========== Driver Initialization ==========
    driver.pwm_frequency        = pwmFrequency;
    driver.voltage_power_supply = supplyVoltage;
    driver.voltage_limit        = maxMotorVoltage;
    driver.init();
    driver.enable();


    // ========== Encoder Initialization ==========
    encoder.attachMotor(&motor);
    encoder.setCalibrationEnabled(true);
    encoder.init();

    for(int i = 0; i < 50; i++)     // Stabilize sensor readings before calibration
    {
        encoder.update();
    }


    // ========== Current Sensing Init ==========
    current_sense.linkDriver(&driver);
    current_sense.init();


    // ========== Motor Linking ==========
    motor.linkDriver(&driver);
    motor.linkSensor(&encoder);
    motor.linkCurrentSense(&current_sense);


    // ========== Motor Configuration Parameters ==========
    motor.voltage_sensor_align = 3;
    motor.voltage_limit        = maxMotorVoltage;
    motor.torque_controller = TorqueControlType::foc_current;
    motor.controller           = MotionControlType::velocity;


    // ========== Velocity Loop PID & LPF Settings ==========
    motor.PID_velocity.P = 0.3;
    motor.PID_velocity.I = 0.0;
    motor.PID_velocity.D = 0.005;
    motor.PID_velocity.output_ramp = 1000.0;
    motor.PID_velocity.limit = 2.0;
    motor.LPF_velocity.Tf = 0.5;


    // ========== Angle Loop PID & LPF Settings ==========
    motor.P_angle.P = 12.0;
    motor.P_angle.I = 0.0;
    motor.P_angle.D = 0.05;
    motor.P_angle.output_ramp = 1000.0;
    motor.P_angle.limit = 100.0;
    motor.LPF_angle.Tf = 0.05;


    // ========== Current Q Loop PID & LPF Settings ==========
    motor.PID_current_q.P = 3;
    motor.PID_current_q.I = 30.0;
    motor.PID_current_q.D = 0.0;
    motor.PID_current_q.output_ramp = 0.0;
    motor.PID_current_q.limit = 10.0;
    motor.LPF_current_q.Tf = 0.05;


    // ========== Current D Loop PID & LPF Settings ==========
    motor.PID_current_d.P = 3;
    motor.PID_current_d.I = 30.0;
    motor.PID_current_d.D = 0.0;
    motor.PID_current_d.output_ramp = 0.0;
    motor.PID_current_d.limit = 10.0;
    motor.LPF_current_d.Tf = 0.05;


    // ========== PWM Modulation Settings ==========
    motor.foc_modulation = FOCModulationType::SpaceVectorPWM;
    motor.modulation_centered = 1.0;


    // ========== Limits & Target Settings ==========
    motor.velocity_limit = 100;
    motor.target         = 0;


    // ========== Monitoring & Commander Settings ==========
    motor.useMonitoring(Serial);
    motor.monitor_downsample = 10;   


    // ========== Start Motor & FOC ==========
    motor.init();
    encoder.calibrate();
    motor.initFOC();
    motor.enable();

    // ========== Register Commander Commands ==========
    command.add('M', onMotor, "motor");

}


// ============================================================================
//                        MAIN LOOP
// ============================================================================
void loop() {
    motor.loopFOC();        // Run inner FOC current loop
    motor.move();           // Run outer motion control loop (uses motor.target)
    motor.monitor();        // Stream data to serial plot/monitor if enabled
    command.run();          // Process incoming serial tuning commands
}