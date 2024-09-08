import serial
import struct

# Initialize the serial port
ser = serial.Serial('/dev/ttyUSB0', 9600, timeout=1)

try:
    while True:
        # Wait for enough data
        if ser.in_waiting >= 4:
            # Read 4 bytes
            data = ser.read(4)
            # Unpack the bytes to get the individual values
            frame_header, data_H, data_L, received_checksum = struct.unpack('BBBB', data)
            
            # Check the frame header
            if frame_header == 0xFF:
                # Calculate the checksum
                calculated_checksum = (frame_header + data_H + data_L) & 0xFF
                # If the checksum is correct, calculate and print the distance
                if calculated_checksum == received_checksum:
                    distance_mm = (data_H << 8) + data_L
                    distance_m = distance_mm / 1000.0  # Convert mm to m
                    print(f"Measured Distance: {distance_m} meters")
                else:
                    print(f"Checksum error: {calculated_checksum} != {received_checksum}")
            else:
                print(f"Invalid frame header: {frame_header}")
except KeyboardInterrupt:
    print("Program terminated")
finally:
    ser.close()
