SYSTEM_MODE(MANUAL);
int led = D7;
byte mac[6];
const String wifiSSID = "guesswhat";
const String wifiPassword = "12345678";
void setup() {
  pinMode(led, OUTPUT);
  Serial.begin(9600);
  Serial.println("Hello World!");

  WiFi.on();
	WiFi.setCredentials(wifiSSID, wifiPassword);
	WiFi.connect(WIFI_CONNECT_SKIP_LISTEN);
	// block until connect to local router
	for (; !WiFi.localIP(); Particle.process());
}

void loop()
{
  digitalWrite(led, HIGH);
  delay(500);
  digitalWrite(led, LOW);
  delay(500);
}
