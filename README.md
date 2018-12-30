# ESP32 MQTT RELAY

This is a setup of a heat switch controlled via mqtt for the esp32. It's made with the esp-idf framework.

## Requirements

* esp-idf
* mqtt library


## Build

Make sure you have setup the esp-idf toolchain correctly and installed the mqtt library to the components/ folder of your esp-idf installation.

```
# Setup wifi and mqtt access
make menuconfig

# Build the project
make

# flash the esp32
make flash
```

