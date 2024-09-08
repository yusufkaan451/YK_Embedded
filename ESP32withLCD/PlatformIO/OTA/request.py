import requests

# URL of the ESP32 OTA update server
url = 'http://192.168.1.149:8080/update'

# Path to the firmware file
firmware_path = '/home/yusuf/esp32/ESP32withLCD/PlatformIO/otatest1/.pio/build/esp32dev/firmware.bin'

# Headers for the request
headers = {'Content-Type': 'application/octet-stream'}

# Open the file in binary mode and send as POST request
with open(firmware_path, 'rb') as firmware:
    response = requests.post(url, data=firmware, headers=headers)

# Check the response
if response.ok:
    print("Upload successful.")
else:
    print(f"Upload failed with status code: {response.status_code}")
    print(response.text)
