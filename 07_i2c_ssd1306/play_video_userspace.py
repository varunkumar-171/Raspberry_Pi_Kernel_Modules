import cv2
import numpy as np
import time
import argparse
import sys
from PIL import Image

# SSD1306 Command Definitions
SSD1306_SET_MEM_ADDR_MODE = 0x20
SSD1306_SET_COLUMN        = 0x21
SSD1306_SET_PAGE          = 0x22

# Values
ADDR_MODE_HZ         = 0x00
DISPLAY_BEGIN_COL    = 0x00
DISPLAY_END_COL      = 0x7f
DISPLAY_BEGIN_PAGE   = 0x00
DISPLAY_END_PAGE     = 0x07

SSD1306_CLEAR_SCREEN        = 0x69
SSD1306_SET_CURSOR_AT_START = 0x68

WIDTH = 128
HEIGHT = 64
PAGE_COUNT = 8
DEV_PATH = "/dev/my_ssd1306-1"

def send_command(device, cmd_byte):
    """Sends 0x00 (Control Byte) + Command Byte and flushes."""
    device.write(bytes([0x00, cmd_byte]))
    device.flush()

def send_data_buffer(device, data_bytes):
    """Sends 0x40 (Control Byte) + Full Data Buffer and flushes."""
    # Prefix the data with 0x40
    payload = bytes([0x40]) + data_bytes
    device.write(payload)
    device.flush()

def reset_cursor(device):
    """Sets the cursor to (0,0) using individual command flushes."""
    device.write(bytes([SSD1306_SET_CURSOR_AT_START]))
    device.flush()

def ssd1306_clear_screen(device):
    """Clears the screen by writing 1024 bytes of 0x00."""
    device.write(bytes([SSD1306_CLEAR_SCREEN]))
    device.flush()

def process_frame(frame):
    frame = cv2.flip(frame, 1)
    resized_frame = cv2.resize(frame, (128, 64), interpolation=cv2.INTER_AREA)

    # 2. Convert to PIL for Dithering
    frame_rgb = cv2.cvtColor(resized_frame, cv2.COLOR_BGR2RGB)
    pil_image = Image.fromarray(frame_rgb)

    # 3. Floyd-Steinberg Dithering (converts to 1-bit pixels)
    dithered_image = pil_image.convert('1')

    # 4. Convert back to array (0 or 1)
    bw_frame = np.array(dithered_image, dtype=np.uint8)

    # 5. SSD1306 Page Addressing bit-packing
    # This matches the physical hardware layout of the SSD1306
    buffer = bytearray(1024)
    for page in range(8):
        for col in range(128):
            byte_val = 0
            for bit in range(8):
                if bw_frame[page * 8 + bit, col]:
                    byte_val |= (1 << bit)
            buffer[(page * 128) + col] = byte_val

    return buffer


def main():
    parser = argparse.ArgumentParser(description="SSD1306 I2C Video Player")
    parser.add_argument("input", help="Path to video file")
    parser.add_argument("--fps", type=int, default=15, help="Target FPS")
    args = parser.parse_args()

    cap = cv2.VideoCapture(args.input)
    if not cap.isOpened():
        print(f"Error: Could not open {args.input}")
        return

    try:
        with open(DEV_PATH, "wb") as oled:
            # ssd1306_clear_screen(oled)
            print(f"Streaming {args.input} to {DEV_PATH}...")
            while cap.isOpened():
                start_time = time.time()
                ret, frame = cap.read()
                if not ret:
                    break

                reset_cursor(oled)

                # Process and Send Frame Data
                pixel_data = process_frame(frame)
                send_data_buffer(oled, pixel_data)

                # Maintain Timing
                # elapsed = time.time() - start_time
                # wait_time = (1.0 / args.fps) - elapsed
                # if wait_time > 0:
                #    time.sleep(wait_time)

    except KeyboardInterrupt:
        print("\nStopped.")
    finally:
        cap.release()

if __name__ == "__main__":
    main()
