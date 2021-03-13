#include "am2302.h"

int pi_dht_read(int pin, float* humidity, float* temperature) {
  // Validate humidity and temperature arguments and set them to zero.
  if (humidity == NULL || temperature == NULL) {
    return -3;
  }
  *temperature = 0.0f;
  *humidity = 0.0f;

  // Store the count that each DHT bit pulse is low and high.
  // Make sure array is initialized to start at zero.
  int pulseCounts[DHT_PULSES*2] = {0};

  // Set pin to output.
  bcm2835_gpio_fsel(pin, BCM2835_GPIO_FSEL_OUTP);

  // Set pin high for ~500 milliseconds.
  bcm2835_gpio_write(pin, HIGH);
  delay(500);

  // The next calls are timing critical and care should be taken
  // to ensure no unnecssary work is done below.

  // Set pin low for ~20 milliseconds.
  bcm2835_gpio_write(pin, LOW);
  delay(20);

  // Set pin at input.
  bcm2835_gpio_fsel(pin, BCM2835_GPIO_FSEL_INPT);
  // Need a very short delay before reading pins or else value is sometimes still low.
  for (volatile int i = 0; i < 500; ++i) {
  }

  // Wait for DHT to pull pin low.
  uint32_t count = 0;
  while (bcm2835_gpio_lev(pin)) {
    if (++count >= DHT_MAXCOUNT) {
      // Timeout waiting for response.
      return -1;
    }
  }

  // Record pulse widths for the expected result bits.
  for (int i=0; i < DHT_PULSES*2; i+=2) {
    // Count how long pin is low and store in pulseCounts[i]
    while (!bcm2835_gpio_lev(pin)) {
      if (++pulseCounts[i] >= DHT_MAXCOUNT) {
        // Timeout waiting for response.
        return -1;
      }
    }
    // Count how long pin is high and store in pulseCounts[i+1]
    while (bcm2835_gpio_lev(pin)) {
      if (++pulseCounts[i+1] >= DHT_MAXCOUNT) {
        // Timeout waiting for response.
        return -1;
      }
    }
  }

  // Done with timing critical code, now interpret the results.

  // Compute the average low pulse width to use as a 50 microsecond reference threshold.
  // Ignore the first two readings because they are a constant 80 microsecond pulse.
  uint32_t threshold = 0;
  for (int i=2; i < DHT_PULSES*2; i+=2) {
    threshold += pulseCounts[i];
  }
  threshold /= DHT_PULSES-1;

  // Interpret each high pulse as a 0 or 1 by comparing it to the 50us reference.
  // If the count is less than 50us it must be a ~28us 0 pulse, and if it's higher
  // then it must be a ~70us 1 pulse.
  uint8_t data[5] = {0};
  for (int i=3; i < DHT_PULSES*2; i+=2) {
    int index = (i-3)/16;
    data[index] <<= 1;
    if (pulseCounts[i] >= threshold) {
      // One bit for long pulse.
      data[index] |= 1;
    }
    // Else zero bit for short pulse.
  }

  // Useful debug info:
  //printf("Data: 0x%x 0x%x 0x%x 0x%x 0x%x\n", data[0], data[1], data[2], data[3], data[4]);

  // Verify checksum of received data.
  if (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
      // Calculate humidity and temp for DHT22 sensor.
      *humidity = (data[0] * 256 + data[1]) / 10.0f;
      *temperature = ((data[2] & 0x7F) * 256 + data[3]) / 10.0f;
      if (data[2] & 0x80) {
        *temperature *= -1.0f;
      }
    return 0;
  }
  else {
    return -1;
  }
}