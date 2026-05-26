import serial
import csv
import time
import os

PORT = "COM5"
BAUD = 115200
CSV_FILE = "mq137_data.csv"

ser = serial.Serial(PORT, BAUD, timeout=1)
time.sleep(2)

print("Connected to ESP32...")
print("Saving to:", os.path.abspath(CSV_FILE))

file_exists = os.path.exists(CSV_FILE)

with open(CSV_FILE, "a", newline="", encoding="utf-8") as f:
    writer = csv.writer(f)

    if not file_exists:
        writer.writerow([
            "time_ms",
            "recording",
            "present",
            "mq137_raw",
            "voltage",
            "ammonia_change_percent",
            "status",
            "temp_C",
            "hum_percent",
            "delta_temp",
            "delta_hum",
        ])
        f.flush()

    while True:
        try:
            line = ser.readline().decode(errors="ignore").strip()

            if not line:
                continue

            print(line)

            if not line.startswith("CSV,"):
                continue

            parts = line.split(",")

            if len(parts) == 12:
                writer.writerow(parts[1:])
                f.flush()
                print("Saved:", parts[1:])

        except KeyboardInterrupt:
            print("Stopped.")
            break
