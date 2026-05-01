BOARD  := arduino:samd:mkrwifi1010
SKETCH := rain_barrel_controller/rain_barrel_controller.ino

# Set PORT when uploading, e.g.: make upload PORT=/dev/cu.usbmodem14101
PORT   ?= $(shell ls /dev/cu.usbmodem* 2>/dev/null | head -1)

.PHONY: build upload install-deps

build:
	arduino-cli compile --fqbn $(BOARD) $(SKETCH)

upload: build
	@[ -n "$(PORT)" ] || (echo "Error: no USB device found. Set PORT manually: make upload PORT=/dev/cu.usbmodemXXXX" && exit 1)
	arduino-cli upload --fqbn $(BOARD) --port $(PORT) $(SKETCH)

install-deps:
	arduino-cli core install arduino:samd
	arduino-cli lib install ArduinoMqttClient WiFiNINA ArduinoLowPower ArduinoJson
