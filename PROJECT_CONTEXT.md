# FoodLabel BLE Project Context

## Project Overview

This project implements an intelligent food label system with cold chain monitoring, converting an ESP32-C3 based design to WCH CH583 RISC-V MCU with BLE broadcast capability.

## Migration History

### Original Platform
- **MCU**: ESP32-C3 (RISC-V, WiFi/BLE)
- **Language**: C/Arduino framework
- **Display**: 2.9" E-paper
- **Sensor**: SHT4x temperature/humidity

### Target Platform Evolution

**First Attempt: CH572**
- **MCU**: CH572 (RISC-V QingKe V3C, RV32IMBC)
- **RAM**: 12KB SRAM
- **Flash**: 256KB
- **Result**: FAILED - Insufficient RAM

**Final Target: CH583**
- **MCU**: CH583 (RISC-V QingKe V3C, RV32IMBC)
- **RAM**: 32KB SRAM ✅
- **Flash**: 448KB
- **Additional**: NFC Tag capability
- **Result**: SUCCESS - Adequate resources

## Technical Architecture

### System Components

1. **BLE Stack**
   - WCH BLE 5.3 peripheral implementation
   - Manufacturer-specific advertisement (Company ID: 0x07D7)
   - 6KB RAM allocation for BLE heap
   - Non-connectable broadcast mode

2. **E-Paper Display**
   - Controller: SSD1680/IL3897
   - Resolution: 296×128 pixels (2.9" diagonal)
   - Framebuffer: 4,736 bytes (main RAM consumer)
   - Communication: SPI
   - Features: Full/partial refresh, 4-wire interface

3. **Environmental Sensor**
   - Model: Sensirion SHT4x
   - Interface: I2C
   - Measurement: Temperature (-40 to +125°C), Humidity (0-100%)
   - Accuracy: ±0.2°C, ±2% RH
   - CRC-8 data validation

4. **User Interface**
   - 3 GPIO buttons with interrupt-based handling
   - 300ms software debouncing
   - Long-press detection (5 seconds)
   - Fields: Food type, Expiry date (day/month/year)

5. **Persistent Storage**
   - Flash-based EEPROM emulation
   - Stores: Cold chain status, expiry date, food type
   - Library: ISP583 (WCH flash management)

### Memory Layout (CH583)

```
Flash (448KB):
├─ Code: ~50KB
├─ BLE Stack: ~10KB
├─ Graphics Fonts: ~2KB
├─ Constants: ~1KB
└─ Available: ~385KB

RAM (32KB):
├─ BLE Heap: 6KB (BLE_MEMHEAP_SIZE)
├─ E-paper Framebuffer: 4.7KB
├─ Stack: 512 bytes
├─ Application Data: ~300 bytes
└─ Available: ~21KB
```

## BLE Broadcast Protocol

### Advertisement Structure

```
Flags: 0x06 (General Discoverable, BR/EDR not supported)
Manufacturer Data:
  Company ID: 0xD7 0x07 (WCH - Little Endian)
  Payload (9 bytes):
    Byte 0: Food type index (0-7)
    Byte 1-2: Temperature (°F × 10, int16_t LE)
    Byte 3: Humidity (0-100%)
    Byte 4: Days left until expiry
    Byte 5: Cold chain broken flag (0x00=OK, 0x01=Broken)
    Byte 6: Expiry day (1-31)
    Byte 7: Expiry month (1-12)
    Byte 8: Expiry year (0-99, offset from 2000)
```

### Advertisement Interval
- Default: 1 second (fast advertising)
- Can be adjusted for power optimization

## Food Database

Hardcoded food profiles with temperature ranges and shelf life:

| Index | Food | Min Temp (°F) | Max Temp (°F) | Shelf Life (days) |
|-------|------|---------------|---------------|-------------------|
| 0 | Milk | 33 | 40 | 7 |
| 1 | Eggs | 33 | 40 | 21 |
| 2 | Chicken | 32 | 40 | 2 |
| 3 | Fish | 30 | 34 | 1 |
| 4 | Vegetables | 32 | 40 | 7 |
| 5 | Beef | 32 | 40 | 3 |
| 6 | Cheese | 34 | 38 | 14 |
| 7 | Yogurt | 36 | 40 | 14 |

## Cold Chain Monitoring Algorithm

```c
void checkColdChain(float tempF, const FoodItem_t *food) {
    if (!appState.coldChainBroken) {  // Only check if not already broken
        if (tempF < food->minTempF || tempF > food->maxTempF) {
            appState.coldChainBroken = 1;
            FlashStorage_Save(&appState);  // Persist permanently
            EPaper_ShowAlert();             // Visual warning
        }
    }
}
```

**Key Features:**
- One-way flag (cannot be cleared by user)
- Persistent across power cycles
- Immediate visual feedback on display
- Broadcasted via BLE

## Build System

### MounRiver Studio (.cproject)

Eclipse-based IDE with integrated debugger and flash tools.

**Key Configuration:**
- Architecture: RV32IMBC + zicsr + zifencei
- ABI: ilp32 (32-bit integer, no floating-point)
- Optimization: -Os (size)
- Linker script: `Ld/Link.ld` (32KB RAM, 448KB Flash)
- Libraries: libCH58xBLE.a, libISP583.a

**Source Exclusions:**
- `APP/peripheral.c` and `APP/peripheral_main.c` (original example files)
- `Drivers/epaper_driver.c` (stub replaced by epaper_driver_full.c)
- Unused StdPeriphDriver files (PWM, ADC, USB, etc.)

### Toolchain

**Preferred**: xPack RISC-V Embedded GCC
- macOS: `brew install xpack-riscv-none-embed-gcc`
- Linux: Download from xPack releases
- Windows: Included with MounRiver Studio

**Alternative Toolchains:**
- riscv64-unknown-elf-gcc
- riscv-none-elf-gcc
- riscv32-unknown-elf-gcc

## Critical API Differences: CH572 → CH583

| Function | CH572 | CH583 |
|----------|-------|-------|
| BLE Init | `CH57x_BLEInit()` | `CH58X_BLEInit()` |
| Clock Source | `CLK_SOURCE_HSE_PLL_60MHz` | `CLK_SOURCE_PLL_60MHz` |
| UART TX Pin | `bTXD_1` or `bTXD_0` | `bTXD1` |
| UART Init | `UART_DefInit()` | `UART1_DefInit()` |
| Common Header | `CH57x_common.h` | `CH58x_common.h` |
| BLE Library | `CH572BLE_PERI` | `CH58xBLE` |
| Flash Library | `ISP572` | `ISP583` |
| UART Remap | Required | Not required |
| HSE Capacitance | `HSECFG_Capacitance()` | Not used |

## Known Issues and Solutions

### Issue 1: RAM Overflow on CH572
**Problem**:
```
region 'RAM' overflowed by 3360 bytes
```
**Root Cause**: CH572 has only 12KB RAM, but application needs ~12KB (BLE 6KB + Framebuffer 4.7KB + App 1KB)

**Solution**: Migrated to CH583 with 32KB RAM

### Issue 2: Duplicate Symbol Definitions
**Problem**:
```
warning: "MS1_TO_SYSTEM_TIME" redefined
warning: "US_TO_RTC" redefined
```
**Root Cause**: Macros defined in both CONFIG.h and RTC.h/BLE library

**Solution**: Removed duplicate definitions from CONFIG.h

### Issue 3: Custom mcpy Instruction
**Problem**:
```
Error: unrecognized opcode `mcpy a2,a1,a5'
```
**Root Cause**: WCH-specific custom instruction not in standard RISC-V toolchains

**Solution**: Replaced with standard C implementation in core_riscv.h:
```c
__attribute__((always_inline)) RV_STATIC_INLINE void __MCPY(void *dst, void *start, void *end)
{
    char *d = (char *)dst;
    char *s = (char *)start;
    char *e = (char *)end;
    while (s < e) {
        *d++ = *s++;
    }
}
```

### Issue 4: Zicsr and Zifencei Extensions
**Problem**:
```
Error: unrecognized opcode `csrr', extension `zicsr' required
Error: unrecognized opcode `fence.i', extension `zifencei' required
```
**Solution**: Update -march flag:
```
-march=rv32imac_zicsr_zifencei
```

## Project Structure

```
ch5NFC/
├── EVT/
│   ├── EXAM/
│   │   ├── BLE/
│   │   │   ├── Peripheral/           # Main project directory
│   │   │   │   ├── APP/
│   │   │   │   │   ├── foodlabel_main.c
│   │   │   │   │   ├── foodlabel.c
│   │   │   │   │   └── include/
│   │   │   │   │       ├── CONFIG.h
│   │   │   │   │       └── foodlabel.h
│   │   │   │   ├── Drivers/
│   │   │   │   │   ├── sht4x_driver.c
│   │   │   │   │   ├── button_driver.c
│   │   │   │   │   ├── epaper_driver_full.c
│   │   │   │   │   ├── flash_storage.c
│   │   │   │   │   ├── gfx.c
│   │   │   │   │   └── include/
│   │   │   │   ├── HAL/              # From SDK
│   │   │   │   ├── Profile/          # GATT profile
│   │   │   │   ├── StdPeriphDriver/  # From SDK
│   │   │   │   ├── RVMSIS/           # RISC-V core headers
│   │   │   │   ├── Startup/          # startup_CH583.S
│   │   │   │   ├── Ld/               # Link.ld
│   │   │   │   ├── LIB/              # libCH58xBLE.a, libISP583.a
│   │   │   │   ├── .cproject         # MounRiver Studio project
│   │   │   │   └── README.md
│   │   │   └── LIB/                  # Shared BLE library headers
│   │   └── SRC/
│   │       ├── StdPeriphDriver/      # Shared peripheral drivers
│   │       ├── RVMSIS/               # Shared RISC-V headers
│   │       └── Ld/                   # Shared linker scripts
│   └── ...
├── Tool/                             # Flash programming tools
├── doc/                              # Datasheets and manuals
└── PROJECT_CONTEXT.md                # This file
```

## Testing Tools

### BLE Scanner (Python)
Located in original CH572 project:
- `tools/ble_scanner.py`: Real-time BLE scanning
- `tools/decode_advert.py`: Offline hex decoder

**Usage**:
```bash
python3 tools/ble_scanner.py
# Look for device advertising WCH company ID (0x07D7)
```

## Future Enhancements

### NFC Integration
CH583 includes NFC Tag Type 2 capability (not yet implemented):
- Store static food information
- Allow smartphone reading without BLE pairing
- Complement to BLE broadcast

### Power Optimization
- Implement deep sleep with RTC wake-up
- Reduce BLE advertising interval after initial period
- Use e-paper partial refresh exclusively
- Target: >6 months on CR2032 battery

### Enhanced Features
- Multi-language support
- Custom food profiles via BLE config
- Temperature history graph on e-paper
- Barcode/QR code generation on display

## Development Workflow

1. **Code Development**
   - Edit files in MounRiver Studio or external editor
   - Build: Ctrl+B (MounRiver) or `make`
   - Fix errors, repeat

2. **Flash and Debug**
   - Connect WCH-Link debugger
   - Flash: Right-click project → Flash Download
   - Debug: F11 (MounRiver)

3. **Version Control**
   - Branch naming: `claude/add-ble-broadcast-<session-id>`
   - Commit messages: Descriptive, present tense
   - Push: `git push -u origin <branch>`

## References

- [CH583 Datasheet](https://www.wch.cn/products/CH583.html)
- [WCH BLE SDK Documentation](https://github.com/openwch/ch583)
- [SHT4x Datasheet](https://www.sensirion.com/en/environmental-sensors/humidity-sensors/humidity-sensor-sht4x/)
- [RISC-V Specifications](https://riscv.org/technical/specifications/)

## Version

- **Document Version**: 1.0
- **Date**: 2025-12-06
- **Project Version**: 1.1 (CH583)
- **Previous Version**: 1.0 (CH572 - abandoned due to RAM limitation)
