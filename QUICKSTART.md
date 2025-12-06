# Quick Start Guide - CH583 Smart Food Label

## What You Need

### Hardware
- CH583 development board (QFN48 package)
- WCH-Link programmer/debugger
- 2.9" E-Paper display (SSD1680/IL3897 controller)
- SHT4x temperature/humidity sensor
- 3× push buttons
- 2× LEDs (with 220Ω resistors)
- Breadboard and jumper wires (for prototyping)
- Power supply (3.3V, or CR2032 battery for production)

### Software
- MounRiver Studio IDE (recommended) OR
- RISC-V GCC toolchain
- WCH-LinkUtility (for flashing)
- BLE scanner app (nRF Connect or BLE Scanner)

## Hardware Setup

### Wiring Diagram

```
CH583 Pin Assignment:
=====================

E-PAPER DISPLAY (SPI):
  CS   (PA4)  ─── E-Paper CS
  DC   (PA1)  ─── E-Paper DC
  RST  (PA2)  ─── E-Paper RST
  BUSY (PA3)  ─── E-Paper BUSY
  SCK  (PA12) ─── E-Paper SCK  (SPI0_SCK)
  MOSI (PA13) ─── E-Paper MOSI (SPI0_MOSI)
  GND         ─── E-Paper GND
  3.3V        ─── E-Paper VCC

SHT4X SENSOR (I2C):
  SDA  (PB12) ─── SHT4x SDA
  SCL  (PB13) ─── SHT4x SCL
  GND         ─── SHT4x GND
  3.3V        ─── SHT4x VCC

USER BUTTONS (Active HIGH with pull-down):
  BTN1 (PA4)  ─── [Button] ─── 3.3V  (+ 10kΩ to GND)
  BTN2 (PA5)  ─── [Button] ─── 3.3V  (+ 10kΩ to GND)
  BTN3 (PA13) ─── [Button] ─── 3.3V  (+ 10kΩ to GND)

STATUS LEDS:
  LED1 (PA8)  ─── [LED] ─── 220Ω ─── GND (Status)
  LED2 (PA9)  ─── [LED] ─── 220Ω ─── GND (Waste Alert)

DEBUG UART:
  TX   (PA9)  ─── UART RX (3.3V TTL)

WCH-LINK DEBUGGER:
  SWDIO       ─── WCH-Link SWDIO
  GND         ─── WCH-Link GND
  3.3V        ─── WCH-Link 3.3V (if powering from debugger)
```

### Important Notes

⚠️ **All GPIOs are 3.3V only** - Do NOT connect 5V signals!
⚠️ **E-paper requires stable 3.3V** - Use quality power supply
⚠️ **Buttons share pins with SPI** - See note below

**Pin Conflict Resolution**:
- PA4 is used for both EPAPER_CS and BTN_FIELD
- PA13 is used for both SPI MOSI and BTN_VALUE_DOWN
- This is acceptable because buttons are only read when NOT updating display
- In production, consider using different pins or multiplexing

## Software Setup

### Method 1: MounRiver Studio (Easiest)

1. **Download MounRiver Studio**
   - Visit: http://www.mounriver.com/
   - Download for your OS (Windows/Linux/macOS)
   - Install and launch

2. **Import Project**
   ```
   File → Open Projects from File System
   Select: /path/to/ch5NFC/EVT/EXAM/BLE/Peripheral
   Click: Finish
   ```

3. **Build**
   ```
   Right-click project → Build Project
   Or press: Ctrl+B (Windows/Linux) / Cmd+B (macOS)
   ```

4. **Flash**
   ```
   Connect WCH-Link to CH583
   Right-click project → Flash Download
   Wait for "Download complete" message
   ```

5. **Debug (Optional)**
   ```
   Press F11 to start debugging
   Set breakpoints, step through code, inspect variables
   ```

### Method 2: Command Line

1. **Install Toolchain**
   ```bash
   # macOS
   brew install xpack-riscv-none-embed-gcc

   # Linux (Ubuntu/Debian)
   wget https://github.com/xpack-dev-tools/riscv-none-embed-gcc-xpack/releases/download/v10.2.0-1.2/xpack-riscv-none-embed-gcc-10.2.0-1.2-linux-x64.tar.gz
   tar -xzf xpack-riscv-none-embed-gcc-10.2.0-1.2-linux-x64.tar.gz
   export PATH=$PATH:$PWD/xpack-riscv-none-embed-gcc-10.2.0-1.2/bin

   # Windows
   # Download from: https://xpack.github.io/riscv-none-embed-gcc/install/
   ```

2. **Build Project**
   ```bash
   cd EVT/EXAM/BLE/Peripheral
   make clean
   make
   ```

3. **Flash**
   ```bash
   # Using WCH-LinkUtility
   wch-link flash obj/Peripheral.hex

   # Or using MounRiver's command-line tool
   ./beforedebug.sh
   ```

## First Boot

### What to Expect

1. **Splash Screen** (3 seconds)
   ```
   ┌──────────────────────────┐
   │      SMART               │
   │      LABELING            │
   │  Cold Chain Food Tracking│
   └──────────────────────────┘
   ```

2. **Main Display**
   ```
   ┌──────────────────────┬────────┐
   │ CHICKEN              │  TEMP  │
   │                      │ ────── │
   │ Created: 05-12-25    │  77°F  │
   │ Expires: 12-12-25    │        │
   │ Days left: 7         │        │
   │ Status: Fresh        │   [SL] │
   └──────────────────────┴────────┘
   ```

3. **Serial Output** (115200 baud, PA9)
   ```
   === SmartFood Cold Chain Monitor (CH583) ===
   CH583 BLE Protocol Stack V3.30
   Food Label Init
   Boot count: 1
   SHT4x initialized
   Flash storage initialized
   Expiry: 12-12-25 (Days left: 7)
   BLE advert data built (16 bytes)
   Starting advertising...
   Temperature: 77.0°F, Humidity: 45%
   ```

## Using the Device

### Button Controls

**Normal Mode (View Only)**:
- Device displays current food information
- Temperature updates automatically every 10 minutes
- No button interaction needed

**Edit Mode**:
1. **Enter Edit Mode**: Hold Button 1 (Field Select) for 5 seconds
   - LED blinks rapidly (5 times)
   - First field (Product Type) is highlighted

2. **Navigate Fields**: Press Button 1 (short press)
   - Cycles through: Product → Day → Month → Year → Days Left → Product...

3. **Change Values**:
   - Button 2 (Value Up): Increase value
   - Button 3 (Value Down): Decrease value

4. **Save and Exit**: Hold Button 1 for 5 seconds again
   - LED blinks slowly (3 times)
   - Changes saved to flash
   - Expiry date recalculated automatically

### Editable Fields

1. **Product Type** (8 options)
   - CHICKEN, BEEF, FISH, PORK, SEAFOOD, VEGETABLES, DAIRY, FRUITS

2. **Created Day** (1-31)
   - Day the product was labeled

3. **Created Month** (1-12)
   - Month the product was labeled

4. **Created Year** (25-30)
   - Year (20xx format, e.g., 25 = 2025)

5. **Days Left** (1-90)
   - Shelf life in days
   - Expiry date auto-calculated: Created Date + Days Left

### Cold Chain Monitoring

**How it Works**:
- Device reads temperature every 10 minutes
- Compares to safe range for selected food type
- If temperature goes outside range → Cold chain broken

**When Cold Chain Breaks**:
- Display shows: "Status: SPOILED" (instead of "SL" logo)
- Waste LED blinks every 5 minutes (10 blinks per cycle)
- Flag is **permanent** - cannot be reset
- Broadcast via BLE with broken flag set

**Temperature Ranges**:
- CHICKEN: 32-40°F (0-4°C)
- BEEF: 32-40°F (0-4°C)
- FISH: 30-34°F (-1 to 1°C) ← stricter!
- PORK: 32-40°F (0-4°C)
- SEAFOOD: 30-34°F (-1 to 1°C) ← stricter!
- VEGETABLES: 32-50°F (0-10°C)
- DAIRY: 32-40°F (0-4°C)
- FRUITS: 32-45°F (0-7°C)

## BLE Monitoring

### Scan for Device

**iOS/Android**:
1. Install "nRF Connect" or "BLE Scanner" app
2. Open app and scan for devices
3. Look for device named "SmartFood"
4. Check "Manufacturer Data" field

**Example nRF Connect Output**:
```
Device: SmartFood
Address: 84:C2:E4:03:F0:0D
RSSI: -45 dBm

Advertisement Data:
  Flags: 0x06 (General Discoverable, BR/EDR not supported)
  Manufacturer Data:
    Company ID: 0x07D7 (WCH)
    Data: 00 E0 01 41 05 00 0C 0C 19
          ^^-Food(0=Chicken)
             ^^-Temp(480=48.0°F)
                ^^-Humid(65%)
                   ^^-Days(5)
                      ^^-Status(0=OK)
                         ^^-Expiry(12-12-25)
```

### Decode BLE Data

**Python Script**:
```python
# See MIGRATION_GUIDE.md for full Python BLE scanner
# Or use online decoder at: https://foodlabel-decoder.example.com
```

**Manual Decoding**:
```
Hex: 00 E0 01 41 05 00 0C 0C 19
     ├─ 00: Food type 0 (CHICKEN)
     ├─ E001: Temp 0x01E0 = 480 → 48.0°F
     ├─ 41: Humidity 0x41 = 65%
     ├─ 05: Days left = 5
     ├─ 00: Flags (bit 0=0 → cold chain OK)
     └─ 0C0C19: Expiry 12-12-25 (Dec 12, 2025)
```

## Power Consumption

### Active Mode (BLE advertising)
- Current: ~15 mA @ 3.3V
- Power: ~50 mW

### Sleep Mode (between sensor reads)
- Current: <2 µA @ 3.3V
- Power: <7 µW

### Battery Life Calculation

**CR2032 Battery (225 mAh)**:
- Active: 10 min every 10 min = 100% duty
- Avg current: 15 mA
- Life: 225 / 15 = 15 hours

**Optimized (sensor read every 10 min)**:
- Active: 30 sec every 10 min = 5% duty
- Sleep: 9.5 min every 10 min = 95% duty
- Avg: 15mA × 5% + 0.002mA × 95% = 0.75 mA
- Life: 225 / 0.75 = 300 hours = **12.5 days**

**Production Optimization (1 hour wake)**:
- Active: 30 sec every 1 hour = 0.83% duty
- Sleep: 59.5 min = 99.17% duty
- Avg: 15mA × 0.83% + 0.002mA × 99.17% = 0.127 mA
- Life: 225 / 0.127 = 1,772 hours = **2.4 months**

**With Larger Battery (1000 mAh LiPo)**:
- 1 hour wake cycle: 1000 / 0.127 = **10.8 months**

## Troubleshooting

### Display Issues

**Nothing on display**:
- Check 3.3V power to display
- Verify all SPI connections (CS, DC, RST, BUSY, SCK, MOSI)
- Check BUSY pin is not stuck LOW
- Try full refresh in code: `EPaper_Clear(0xFF);`

**Display partially updated**:
- Increase BUSY timeout in epaper_driver_full.c
- Use full refresh instead of partial
- Check SPI clock speed (max 10 MHz for most e-papers)

**Ghosting/burn-in**:
- Perform full refresh: power cycle device
- Some ghosting is normal with partial updates
- Full refresh clears ghosting

### Sensor Issues

**Temperature always 0.0°F**:
- Check I2C connections (PB12=SDA, PB13=SCL)
- Verify 3.3V power to SHT4x
- Check I2C pull-up resistors (4.7kΩ recommended)
- Verify sensor address: 0x44 (SHT4x default)

**Humidity reading wrong**:
- SHT4x needs 30-60 seconds to stabilize after power-on
- Check for condensation on sensor
- Verify sensor is not covered/blocked

**Cold chain always broken**:
- Check initial temperature is reasonable (not 0°F or 999°F)
- Verify food type temperature range
- Reset device: power cycle with Button 1 held (clears flash)

### BLE Issues

**Device not advertising**:
- Check `BLE_MEMHEAP_SIZE >= 6144` in CONFIG.h
- Verify MAC address: `BLE_MAC = TRUE`
- Check serial output for BLE init messages
- Ensure antenna is connected (if using external)

**Cannot scan device**:
- Check BLE is enabled on phone
- Try different BLE scanner app
- Verify device is within range (<10 meters)
- Check device is not in sleep mode (should wake every 10 min)

**Manufacturer data empty**:
- Rebuild advertisement: call `buildAdvertData()`
- Check sensor readings are valid (not NaN or 0)
- Verify WCH company ID: 0x07D7

### Build/Flash Issues

**Build error: "RAM overflow"**:
- You're using CH572 (12KB RAM) - need CH583 (32KB)
- Check linker script: `Ld/Link.ld`
- Verify MCU selection in MounRiver

**Flash error: "Cannot connect to target"**:
- Check WCH-Link connections (SWDIO, GND)
- Verify target power (3.3V)
- Try slower flash speed in WCH-LinkUtility
- Press reset on CH583 before flashing

**Code uploaded but not running**:
- Check option bytes (use WCH-LinkUtility)
- Verify correct linker script for CH583
- Check watchdog settings
- Try erasing chip before flashing

## Next Steps

1. **Calibrate Sensor**: Compare with reference thermometer, adjust if needed
2. **Optimize Power**: Increase wake interval to 1 hour for production
3. **Develop Mobile App**: iOS/Android app to scan and display labels
4. **Design PCB**: Create custom board for production
5. **Add Antenna**: Chip antenna or PCB trace antenna for BLE
6. **Get Certified**: FCC/CE certification for wireless product

## Support

- **GitHub Issues**: https://github.com/openwch/ch583/issues
- **WCH Forum**: http://www.wch.cn/bbs
- **Documentation**: See `EVT/EXAM/BLE/Peripheral/README.md`
- **Migration Guide**: See `MIGRATION_GUIDE.md`

---

**Ready to start? Connect your hardware and flash the firmware!**
