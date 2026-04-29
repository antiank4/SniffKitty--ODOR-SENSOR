import serial
import csv
import time
import os

PORT = "COM5"   # 改成你的端口
BAUD = 115200
CSV_FILE = "mq135_data.csv"

ser = serial.Serial(PORT, BAUD, timeout=1)
time.sleep(2)

print("Connected to ESP32...")
print("Saving to:", os.path.abspath(CSV_FILE))

file_exists = os.path.exists(CSV_FILE)

with open(CSV_FILE, "a", newline="", encoding="utf-8") as f:
    writer = csv.writer(f)

    if not file_exists:
        writer.writerow(["time_ms", "raw", "voltage", "change_percent", "status"])
        f.flush()

    while True:
        try:
            line = ser.readline().decode(errors="ignore").strip()

            if not line:
                continue

            print(line)

            # 只保存 CSV 开头的数据行
            if not line.startswith("CSV,"):
                continue

            parts = line.split(",")

            # CSV,time_ms,raw,voltage,change_percent,status = 6 个
            if len(parts) == 6:
                writer.writerow(parts[1:])  # 去掉 CSV 标记
                f.flush()
                print("Saved:", parts[1:])

        except KeyboardInterrupt:
            print("Stopped.")
            break