#pragma once
#include <Arduino.h>

struct TelemetryRecord {
  uint32_t timestamp; // Seconds since boot
  int16_t pitTemp;
  int16_t setPoint;
  int16_t probe1;
  int16_t probe2;
  int16_t probe3;
};

const int TELEMETRY_CAPACITY = 600; // 100 minutes at 10s intervals

struct TelemetryHistory {
  TelemetryRecord buffer[TELEMETRY_CAPACITY];
  int head = 0;
  int count = 0;
  unsigned long lastRecordTime = 0;
};

struct AlarmConfig {
  int targets[3] = {0, 0, 0};
  bool triggered[3] = {false, false, false};

  bool flameoutTriggered = false;
  unsigned long flameoutStartTime = 0;
};

struct GrillState {
  int grillId = 0;
  int pitTemp = 0;
  int currentSetPoint = 0;
  int probe1 = 0;
  int probe2 = 0;
  int probe3 = 0;
  int power = 0;
  String flags = "";
  unsigned long lastSeen = 0;
  bool connected = false;
  unsigned long cookStartTime = 0; // Epoch/millis tracking for active cook
};

struct GrillCommand {
  int targetSetPoint = 150;
  int power = 1;
  int cookMode = 1;
  int zoneProbe = 1;
};