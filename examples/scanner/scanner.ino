/*
 * Copyright (C) 2011 J. Coliz <maniacbug@ymail.com>
 * Updated 2020 TMRh20
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

/**
 * Channel scanner and Continuous Carrier Wave Output
 *
 * Example to detect interference on the various channels available.
 * This is a good diagnostic tool to check whether you're picking a
 * good channel for your application.
 *
 * Run this sketch on two devices. On one device, start CCW output by sending a 'g'
 * character over Serial. The other device scanning should detect the output of the sending
 * device on the given channel. Adjust channel and output power of CCW below.
 *
 * Inspired by cpixip.
 * See http://arduino.cc/forum/index.php/topic,54795.0.html
 */

/*
 * How to read the output:
 * - The header is a list of supported channels in decimal written vertically.
 * - Each column corresponding to the vertical header is a hexadecimal count of
 *   detected signals (max is 15 or 'f').
 *
 * The following example
 *    000
 *    111
 *    789
 *    ~~~   <- just a divider between the channel's vertical labels and signal counts
 *    1-2
 * can be interpreted as
 * - 1 signal detected on channel 17
 * - 0 signals (denoted as '-') detected on channel 18
 * - 2 signals detected on channel 19
 *
 * Each line of signal counts represent 100 passes of the supported spectrum.
 */

#include "RF24.h"
#include "printf.h"

//
// Hardware configuration
//

#define CE_PIN 7
#define CSN_PIN 8
// instantiate an object for the nRF24L01 transceiver
RF24 radio(CE_PIN, CSN_PIN);

//
// Channel info
//

const uint8_t num_channels = 126;  // 0-125 are supported
uint8_t values[num_channels];      // the array to store summary of signal counts per channel

// To detect noise, we'll use the worst addresses possible (a reverse engineering tactic).
// These addresses are designed to confuse the radio into thinking
// that the RF signal's preamble is part of the packet/payload.
const uint8_t noiseAddress[][2] = { { 0x55, 0x55 }, { 0xAA, 0xAA } };

const int num_reps = 100;   // number of passes for each scan of the entire spectrum
bool constCarrierMode = 0;  // this flag controls example behavior (scan mode is default)

void printHeader();  // prototype function for printing the channels' header


void setup(void) {

  // Print preamble
  Serial.begin(115200);
  while (!Serial) {
    // some boards need this to wait for Serial connection
  }
  Serial.println(F("RF24/examples/scanner/"));
  Serial.println(F("!!! Please do not send line endings in the serial monitor !!!"));

  // Setup and configure rf radio
  if (!radio.begin()) {
    Serial.println(F("radio hardware not responding!"));
    while (true) {
      // hold in an infinite loop
    }
  }
  radio.setAutoAck(false);   // Don't acknowledge arbitrary signals
  radio.disableCRC();        // Accept any signal we find
  radio.setAddressWidth(2);  // A reverse engineering tactic (not typically recommended)
  radio.openReadingPipe(0, noiseAddress[0]);
  radio.openReadingPipe(1, noiseAddress[1]);

  // set the data rate
  Serial.print(F("Select your Data Rate. "));
  Serial.println(F("Enter '1' for 1Mbps, 2 for 2Mbps, 3 for 250kbps. Defaults to 1Mbps."));
  while (!Serial.available()) {
    // wait for user input
  }
  uint8_t dataRate = Serial.read();
  if (dataRate == 50) {
    Serial.println(F("Using 2 Mbps."));
    radio.setDataRate(RF24_2MBPS);
  } else if (dataRate == 51) {
    Serial.println(F("Using 250 kbps."));
    radio.setDataRate(RF24_250KBPS);
  } else {
    Serial.println(F("Using 1 Mbps."));
    radio.setDataRate(RF24_1MBPS);
  }
  Serial.println(F("***Enter a channel number to emit a constant carrier wave."));
  Serial.println(F("***Enter a negative number to switch back to scanner mode."));

  // Get into standby mode
  radio.startListening();
  radio.stopListening();
  radio.flush_rx();

  // printf_begin();
  // radio.printPrettyDetails();
  // delay(1000);

  // Print out vertical header
  printHeader();
}

void loop(void) {
  /****************************************/
  // Send a number over Serial to begin Constant Carrier Wave output
  // Configure the power amplitude level below
  if (Serial.available()) {
    int c = Serial.read();
    if (c >= 0) {
      c = min(125, max(0, c));  // clamp channel to supported range
      constCarrierMode = 1;
      radio.stopListening();
      delay(2);
      Serial.print("\nStarting Carrier Wave Output on channel ");
      Serial.println(c);
      radio.startConstCarrier(RF24_PA_LOW, c);
    } else {
      constCarrierMode = 0;
      radio.stopConstCarrier();
      Serial.println("\nStopping Carrier Out");
      printHeader();
    }
  }
  /****************************************/

  if (constCarrierMode == 0) {
    // Clear measurement values
    memset(values, 0, sizeof(values));

    // Scan all channels num_reps times
    int rep_counter = num_reps;
    while (rep_counter--) {
      int i = num_channels;
      while (i--) {
        // Select this channel
        radio.setChannel(i);

        // Listen for a little
        radio.startListening();
        delayMicroseconds(128);
        radio.stopListening();

        // Did we get a signal?
        if (radio.testRPD()) {
          ++values[i];
          radio.flush_rx();  // discard packets of noise
        }
      }
    }

    // Print out channel measurements, clamped to a single hex digit
    for (int i = 0; i < num_channels; ++i) {
      if (values[i])
        Serial.print(min(0xf, values[i]), HEX);
      else
        Serial.print(F("-"));
    }
    Serial.println();

  }  // if constCarrierMode == 0
  else {
    // show some output to prove that the program isn't bricked
    Serial.print(F("."));
    delay(1000);  // delay a second to keep output readable
  }
}  // end loop()

void printHeader() {
  // Print the hundreds digits
  for (int i = 0; i < num_channels; ++i)
    Serial.print(i / 100);
  Serial.println();

  // Print the tens digits
  for (int i = 0; i < num_channels; ++i)
    Serial.print((i % 100) / 10);
  Serial.println();

  // Print the singles digits
  for (int i = 0; i < num_channels; ++i)
    Serial.print(i % 10);
  Serial.println();

  // Print the header's divider
  for (int i = 0; i < num_channels; ++i)
    Serial.print(F("~"));
  Serial.println();
}