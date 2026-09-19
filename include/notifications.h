#pragma once
#include <Arduino.h>

class NotificationManager {
public:
  void begin(const char* ntfyTopic);
  void sendAlert(const char* title, const char* message, int priority = 3, const char* tags = "warning");

private:
  String _topic;
};

extern NotificationManager notifications;