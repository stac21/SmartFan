#include <Stepper.h>
#include <SimpleDHT.h>
#include <LiquidCrystal.h>

#define DHT11_PIN 13
#define STEPPER_PIN_ONE 8
// power button pin
#define BUTTON_ONE_PIN 2
// oscillation button pin
#define BUTTON_TWO_PIN 6
// automatic button pin
#define BUTTON_THREE_PIN 7
// cycle speed button pin
#define BUTTON_FOUR_PIN 12
#define FAN_ENABLE 5
#define DIR_A 3
#define DIR_B 4
#define FAN_SPEED_LOW 100
#define FAN_SPEED_MIDDLE 177
#define FAN_SPEED_HIGH 255
// number of steps per revolution of internal shaft
#define STEPS 32
// speed at which stepper motor rotates
#define STEPPER_SPEED 500
// number of steps that the motor will take. 2048 is one revolution.
#define STEPS_TO_TAKE 700
#define STEP_INTERVAL 50

void pollButtonStates();
void oscillate();
// set the fan speed based on the temperature of the room
void setFanSpeedAuto(int temp);
// sets the fan speed based on what the user has cycled through
void setFanSpeedManual();
void powerOff();

Stepper stepper(STEPS, STEPPER_PIN_ONE, STEPPER_PIN_ONE + 2, STEPPER_PIN_ONE + 1, STEPPER_PIN_ONE + 3);
SimpleDHT11 dht11;

// 0 indicates off, 1 indicates on
int isOn;
// 0 indicates off, 1 indicates on
int isOscillating;
// 0 indicates off, 1 indicates on
int isAutomatic;
// keeps track of the fan speed that the user is cycling. through. Will be set to whatever the automatic fan speed last was
int currentManualFanSpeed;
// tracks the amount of steps that the stepper motor has taken up until it reaches STEPS_TO_TAKE
int numSteps;
// +1 for clockwise, -1 for counterclockwise
int oscillation_direction;

// 0 indicates not pressed, 1 indicates is pressed
int buttonOneState;			// button for toggling power
int buttonTwoState;			// button for toggling oscillation
int buttonThreeState;		// button for toggling automatic fan speeds
int buttonFourState;		// button for manually cycling speeds

void setup() {
	isOn = 0;
	isOscillating = 0;
	isAutomatic = 0;
	currentManualFanSpeed = FAN_SPEED_LOW;
	// begin the oscillation motion in the center of the oscillation swivel
	numSteps = STEPS_TO_TAKE / 2;
	oscillation_direction = 1;

	buttonOneState = 0;
	buttonTwoState = 0;
	buttonThreeState = 0;
	buttonFourState = 0;

  	pinMode(FAN_ENABLE, OUTPUT);
  	pinMode(DIR_A, OUTPUT);
  	pinMode(DIR_B, OUTPUT);

  	stepper.setSpeed(STEPPER_SPEED);

  	pinMode(BUTTON_ONE_PIN, INPUT);
  	pinMode(BUTTON_TWO_PIN, INPUT);
  	pinMode(BUTTON_THREE_PIN, INPUT);
  	pinMode(BUTTON_FOUR_PIN, INPUT);

  	// initialize the serial port
  	Serial.begin(9600);
}

void loop() {
	byte temperature = 0;
	byte humidity = 0;
	byte data[40] = {0};
 	
 	// read temperature and humidity data from the DHT11 sensor
 	if (dht11.read(DHT11_PIN, &temperature, &humidity, data)) {
    	Serial.print("Read DHT11 failed\n");
    	return;
  	}

  	// get the state of all of the buttons
  	pollButtonStates();

	digitalWrite(DIR_A, HIGH);
	
	if (isOn) {
		if (isOscillating) {
			oscillate();
		}
		if (isAutomatic) {
			setFanSpeedAuto(temperature);
		} else {
			setFanSpeedManual();
		}
	} else {
		powerOff();
	}
}

void pollButtonStates() {
	// power button
	if (digitalRead(BUTTON_ONE_PIN) == HIGH) {
		if (buttonOneState == 0) {
			buttonOneState = 1;

			isOn = (isOn == 0) ? 1 : 0;
			Serial.println("Button 1 pressed");
		}
	} else {
		buttonOneState = 0;
	}
	// oscillation button
	if (digitalRead(BUTTON_TWO_PIN) == HIGH) {
		if (buttonTwoState == 0) {
			buttonTwoState = 1;

			isOscillating = (isOscillating == 0) ? 1 : 0;
			Serial.println("Button 2 pressed");
		}
	} else {
		buttonTwoState = 0;
	}
	// automatic speed button
	if (digitalRead(BUTTON_THREE_PIN) == HIGH) {
		if (buttonThreeState == 0) {
			buttonThreeState = 1;

			isAutomatic = (isAutomatic == 0) ? 1 : 0;
			Serial.println("Button 3 pressed");
		}
	} else {
		buttonThreeState = 0;
	}
	// manually cycle speeds
	if (digitalRead(BUTTON_FOUR_PIN) == HIGH) {
		if (buttonFourState == 0) {
			buttonFourState = 1;
			
			// the fan speed will stop being automatic 
			isAutomatic = 0;

			// cycle fan speed
			switch (currentManualFanSpeed) {
			case FAN_SPEED_LOW:
				currentManualFanSpeed = FAN_SPEED_MIDDLE;
				break;
			case FAN_SPEED_MIDDLE:
				currentManualFanSpeed = FAN_SPEED_HIGH;
				break;
			case FAN_SPEED_HIGH:
				currentManualFanSpeed = FAN_SPEED_LOW;
				break;
			default:
				// should be impossible to get here
				break;
			}

			Serial.println("Button 4 pressed");
		}
	} else {
		buttonFourState = 0;
	}
}

void oscillate() {
	if (numSteps >= STEPS_TO_TAKE) {
		numSteps = 0;
		oscillation_direction *= -1;
	}

	stepper.step(STEP_INTERVAL * oscillation_direction);

	numSteps += STEP_INTERVAL;
}

/*
sets the fan speed based on the current temperature read by the DHT11.
Temperature is in celcius
*/
void setFanSpeedAuto(int temp) {
	if (temp <= 26) {
		currentManualFanSpeed = FAN_SPEED_LOW;
		analogWrite(FAN_ENABLE, FAN_SPEED_LOW);
	} else if (temp <= 30) {
		currentManualFanSpeed = FAN_SPEED_MIDDLE;
		analogWrite(FAN_ENABLE, FAN_SPEED_MIDDLE);
	} else {
		currentManualFanSpeed = FAN_SPEED_HIGH;
		analogWrite(FAN_ENABLE, FAN_SPEED_HIGH);
	}
}

void setFanSpeedManual() {
	analogWrite(FAN_ENABLE, currentManualFanSpeed);
}

void returnToCenter() {
	if (numSteps < STEPS_TO_TAKE / 2) {
		stepper.step(oscillation_direction * ((STEPS_TO_TAKE / 2) - numSteps));
	} else if (numSteps > STEPS_TO_TAKE / 2) {
		oscillation_direction *= -1;
		stepper.step(oscillation_direction * (numSteps - (STEPS_TO_TAKE / 2)));
	}
}

void powerOff() {
	// shut fan off
	digitalWrite(FAN_ENABLE, LOW);

	if (numSteps != STEPS_TO_TAKE / 2) {
		returnToCenter();
		numSteps = STEPS_TO_TAKE / 2;
	} else {
		// reset all values
		isOscillating = 0;
		isAutomatic = 0;
		currentManualFanSpeed = FAN_SPEED_LOW;
		numSteps = STEPS_TO_TAKE / 2;
		int oscillation_direction = 1;
	}
}