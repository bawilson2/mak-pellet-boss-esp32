#include "notifications.h"
#include <WiFi.h>
#include <HTTPClient.h>

NotificationManager notifications;

void NotificationManager::begin(const char* ntfyTopic) {
  _topic = ntfyTopic;
}

void NotificationManager::sendAlert(const char* title, const char* message, int priority, const char* tags) {
  if (WiFi.status() != WL_CONNECTED || _topic.length() == 0) return;

  HTTPClient http;
  String url = "http://ntfy.sh/" + _topic;

  http.begin(url);
  http.addHeader("Content-Type", "text/plain");
  http.addHeader("Title", title);
  http.addHeader("Priority", String(priority)); // 1 (min) to 5 (urgent)
  http.addHeader("Tags", tags);

  int httpCode = http.POST((uint8_t*)message, strlen(message));
  if (httpCode > 0) {
    Serial.printf("[ntfy] Alert sent (%d): %s\n", httpCode, message);
  } else {
    Serial.printf("[ntfy] Failed to send alert: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}