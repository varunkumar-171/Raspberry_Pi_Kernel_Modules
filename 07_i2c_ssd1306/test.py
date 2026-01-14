import cv2
import numpy as np
import time
import argparse
import sys
from PIL import Image, ImageEnhance

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


def process_frame(frame, threshold_val=90, contrast_val=2.5):
    """
    Optimized processing:
    1. Resizes and Enhances Contrast for clear silhouettes.
    2. Uses hard thresholding (no dithering) for 'Bad Apple' style sharpness.
    3. Uses NumPy vectorization for fast bit-packing.
    """
    frame = cv2.flip(frame, 1)  # 1 = Horizontal flip
    resized_frame = cv2.resize(frame, (WIDTH, HEIGHT), interpolation=cv2.INTER_AREA)

    pil_img = Image.fromarray(cv2.cvtColor(resized_frame, cv2.COLOR_BGR2RGB))
    pil_img = ImageEnhance.Contrast(pil_img).enhance(contrast_val)
    gray_img = pil_img.convert('L')

    bw_image = gray_img.point(lambda x: 0 if x < threshold_val else 255, '1')

    # Fast Bit-Packing with NumPy
    # Reshape to (8 pages, 8 vertical bits per page, 128 columns)
    bw_array = np.array(bw_image, dtype=np.uint8) > 0
    pages = bw_array.reshape(8, 8, 128)
    # Pack bits vertically (little endian: top pixel is LSB)
    packed_data = np.packbits(pages, axis=1, bitorder='little').flatten()
    return packed_data.tobytes()


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
