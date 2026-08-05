# Hardware and wiring

This document records the signal assignments implemented in `include/config.h`. It is intended to help explain and reproduce the prototype; verify the voltage requirements and pinout of each exact breakout board before powering new hardware.

![SniffKitty wiring diagram](images/wiring-diagram.svg)

## Signal connections

| Module | Module signal | ESP32-S3 pin | Firmware constant |
| --- | --- | ---: | --- |
| MQ137 | Analog output | GPIO 1 | `MQ137_PIN` |
| MQ135 | Analog output | GPIO 2 | `MQ135_PIN` |
| AHT10 | SDA | GPIO 14 | `AHT_SDA` |
| AHT10 | SCL | GPIO 21 | `AHT_SCL` |
| microSD | SCK | GPIO 42 | `SD_SCK_PIN` |
| microSD | MISO | GPIO 41 | `SD_MISO_PIN` |
| microSD | MOSI | GPIO 47 | `SD_MOSI_PIN` |
| microSD | CS | GPIO 45 | `SD_CS_PIN` |
| Addressable LED | Data input | GPIO 48 | `STATUS_LED_PIN` |
| Camera | XCLK | GPIO 15 | `XCLK_GPIO_NUM` |
| Camera | SIOD | GPIO 4 | `SIOD_GPIO_NUM` |
| Camera | SIOC | GPIO 5 | `SIOC_GPIO_NUM` |
| Camera | Y2–Y9 | GPIO 11, 9, 8, 10, 12, 18, 17, 16 | `Y2_GPIO_NUM`–`Y9_GPIO_NUM` |
| Camera | VSYNC | GPIO 6 | `VSYNC_GPIO_NUM` |
| Camera | HREF | GPIO 7 | `HREF_GPIO_NUM` |
| Camera | PCLK | GPIO 13 | `PCLK_GPIO_NUM` |

All modules must share a common ground.

## Electrical notes

- ESP32-S3 GPIO and ADC inputs are not 5 V tolerant. Ensure each MQ module's analog output cannot exceed 3.3 V; use a voltage divider or signal-conditioning circuit if required.
- MQ sensor heater circuits commonly require 5 V and draw substantially more current than logic-only sensors. Size the power supply for both heater modules and avoid sourcing their heater current from a GPIO pin.
- Confirm whether the specific microSD breakout includes regulation and level shifting. A bare microSD card uses 3.3 V signaling.
- Keep analog signal wiring short and route it away from camera clocks, SD traffic, and switching power wiring where practical.
- The current firmware uses relative baseline changes. Meaningful ppm estimates would require sensor-specific calibration with known gas concentrations, load resistance documentation, warm-up characterization, and environmental compensation.

## Mechanical files

| File | Purpose |
| --- | --- |
| [`enclosure-body.3mf`](../hardware/cad/enclosure-body.3mf) | Printable enclosure body |
| [`lid-v1.3mf`](../hardware/cad/lid-v1.3mf) | Original printable lid |
| [`lid-v2.3mf`](../hardware/cad/lid-v2.3mf) | Revised printable lid |
| [`enclosure-part-1.SLDPRT`](../hardware/cad/enclosure-part-1.SLDPRT) | SolidWorks source part |
| [`enclosure-part-2.SLDPRT`](../hardware/cad/enclosure-part-2.SLDPRT) | SolidWorks source part |

![Enclosure CAD overview](images/enclosure-cad-overview.svg)
