#include <Wire.h>

#define MPU 0x68 // I2C address of the MPU6050
#define NUM_SAMPLES 10 // Number of samples for moving average

int16_t AccX, AccY, AccZ;
int16_t GyroX, GyroY, GyroZ;

int16_t AccX_sum, AccY_sum, AccZ_sum;
int16_t GyroX_sum, GyroY_sum, GyroZ_sum;
int sample_count = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // Wake up MPU-6050
  Wire.beginTransmission(MPU);
  Wire.write(0x6B); // PWR_MGMT_1 register
  Wire.write(0x00); // Clear sleep bit
  Wire.endTransmission(true);

  delay(100); // Wait for MPU-6050 to settle
}

void loop() {
  // Read accelerometer data
  Wire.beginTransmission(MPU);
  Wire.write(0x3B); // Start with register 0x3B (ACCEL_XOUT_H)
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 6, true); // Read 6 bytes of accelerometer data

  AccX = Wire.read() << 8 | Wire.read();
  AccY = Wire.read() << 8 | Wire.read();
  AccZ = Wire.read() << 8 | Wire.read();

  // Read gyroscope data
  Wire.beginTransmission(MPU);
  Wire.write(0x43); // Start with register 0x43 (GYRO_XOUT_H)
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 6, true); // Read 6 bytes of gyroscope data

  GyroX = Wire.read() << 8 | Wire.read();
  GyroY = Wire.read() << 8 | Wire.read();
  GyroZ = Wire.read() << 8 | Wire.read();

  // Accumulate readings for moving average
  AccX_sum += AccX;
  AccY_sum += AccY;
  AccZ_sum += AccZ;
  GyroX_sum += GyroX;
  GyroY_sum += GyroY;
  GyroZ_sum += GyroZ;

  sample_count++;

  if (sample_count >= NUM_SAMPLES) {
    // Calculate moving average
    int16_t AccX_avg = AccX_sum / NUM_SAMPLES;
    int16_t AccY_avg = AccY_sum / NUM_SAMPLES;
    int16_t AccZ_avg = AccZ_sum / NUM_SAMPLES;
    int16_t GyroX_avg = GyroX_sum / NUM_SAMPLES;
    int16_t GyroY_avg = GyroY_sum / NUM_SAMPLES;
    int16_t GyroZ_avg = GyroZ_sum / NUM_SAMPLES;

    // Print filtered data
    Serial.print("AccX: ");
    Serial.print(AccX_avg);
    Serial.print(" | AccY: ");
    Serial.print(AccY_avg);
    Serial.print(" | AccZ: ");
    Serial.print(AccZ_avg);
    Serial.print(" | GyroX: ");
    Serial.print(GyroX_avg);
    Serial.print(" | GyroY: ");
    Serial.print(GyroY_avg);
    Serial.print(" | GyroZ: ");
    Serial.println(GyroZ_avg);

    // Reset accumulators and sample count
    AccX_sum = 0;
    AccY_sum = 0;
    AccZ_sum = 0;
    GyroX_sum = 0;
    GyroY_sum = 0;
    GyroZ_sum = 0;
    sample_count = 0;
  }

  delay(10); // Adjust delay as needed
}
