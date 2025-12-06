# Smart Food Label - CH583 Cold Chain Monitor

Advanced cold chain monitoring system for food safety using CH583 BLE SoC with e-paper display and environmental sensors.

## Features

- **Real-time Monitoring**: SHT4x sensor for temperature and humidity tracking
- **Visual Display**: 2.9" e-paper display (296×128) for at-a-glance status
- **BLE Broadcast**: Advertise temperature, humidity, and cold chain status
- **Cold Chain Validation**: Automatic detection of temperature excursions
- **Low Power**: Optimized for battery operation with periodic wake-up
- **Food Database**: Pre-configured profiles for common perishable foods
- **Expiry Tracking**: Automatic shelf-life countdown and alerts

## Hardware

- **MCU**: CH583 (RISC-V, 32KB RAM, 448KB Flash, BLE 5.3)
- **Display**: 2.9" E-Paper (SSD1680/IL3897 controller)
- **Sensor**: SHT4x I2C Temperature/Humidity
- **Interface**: 3 GPIO buttons for user input
- **Storage**: Flash for persistent state

## Pin Configuration

### E-Paper Display (SPI)
- CS: PA12
- DC: PA8
- RST: PA9
- BUSY: PA10

### SHT4x Sensor (I2C)
- SCL: PB13
- SDA: PB12

### User Buttons
- Field Select: PA4
- Value Up: PA5
- Value Down: PA13

### Debug UART
- TX: PA9 (bTXD1)

## BLE Advertisement Format

The device broadcasts temperature and status via BLE manufacturer-specific data:

```
Company ID: 0x07D7 (WCH)
Payload (9 bytes):
  [0]: Food type index
  [1-2]: Temperature (°F × 10, int16_t)
  [3]: Humidity (%)
  [4]: Days remaining until expiry
  [5]: Cold chain status (0=OK, 1=Broken)
  [6]: Expiry day
  [7]: Expiry month
  [8]: Expiry year
```

## Food Database

Pre-configured food items with temperature ranges and shelf life:

1. **Milk**: 33-40°F, 7 days
2. **Eggs**: 33-40°F, 21 days
3. **Chicken**: 32-40°F, 2 days
4. **Fish**: 30-34°F, 1 day
5. **Vegetables**: 32-40°F, 7 days
6. **Beef**: 32-40°F, 3 days
7. **Cheese**: 34-38°F, 14 days
8. **Yogurt**: 36-40°F, 14 days

## Building the Project

### MounRiver Studio (Recommended)

1. Open MounRiver Studio
2. Import project: File → Open Projects from File System
3. Select `/EVT/EXAM/BLE/Peripheral` directory
4. Build: Project → Build Project (Ctrl+B)
5. Flash: right-click project → Flash Download

### Command Line (Linux/macOS)

```bash
cd EVT/EXAM/BLE/Peripheral
make clean
make
```

## Memory Usage

- **Flash**: ~60KB (program + BLE stack)
- **RAM**: ~11KB total
  - BLE stack: 6KB
  - E-paper framebuffer: 4.7KB
  - Application: ~300 bytes

**Note**: This project requires CH583 with 32KB RAM. CH572 (12KB RAM) is insufficient.

## Architecture

### Event-Driven System (TMOS)

The application uses WCH's TMOS (Tiny Multi-task Operating System) for BLE and task scheduling:

- **BLE Task**: Handle BLE stack events, update advertisements
- **Sensor Task**: Periodic temperature/humidity readings (every 60s)
- **Display Task**: Update e-paper when data changes
- **Button Task**: Handle user input with debouncing

### Cold Chain Algorithm

```c
if (temperature < minTemp || temperature > maxTemp) {
    coldChainBroken = true;  // Permanent flag
    flashStorage.save();     // Persist to flash
}
```

Once the cold chain is broken, the flag persists across power cycles and cannot be reset by the user.

## Power Management

- **Active**: 60MHz CPU, BLE advertising every 1s
- **Sleep**: Deep sleep with RTC wake-up every 10 minutes
- **E-Paper**: Partial refresh for updates, full refresh daily

## Development Notes

### CH583 vs CH572 Differences

This project was migrated from CH572 to CH583. Key API changes:

| CH572 | CH583 |
|-------|-------|
| `CH57x_BLEInit()` | `CH58X_BLEInit()` |
| `CLK_SOURCE_HSE_PLL_60MHz` | `CLK_SOURCE_PLL_60MHz` |
| `bTXD_1` | `bTXD1` |
| `UART_DefInit()` | `UART1_DefInit()` |
| `CH57x_common.h` | `CH58x_common.h` |

### Toolchain

- **RISC-V GCC**: riscv-none-embed-gcc
- **Architecture**: RV32IMBC (not RV32IMAC)
  - I: Integer
  - M: Multiply
  - B: Bit manipulation
  - C: Compressed instructions
- **Extensions**: _zicsr, _zifencei
- **ABI**: ilp32

## File Structure

```
Peripheral/
├── APP/
│   ├── foodlabel_main.c      # Main entry point
│   ├── foodlabel.c           # Application logic
│   └── include/
│       ├── CONFIG.h          # BLE and system configuration
│       └── foodlabel.h       # Application header
├── Drivers/
│   ├── sht4x_driver.c        # Temperature/humidity sensor
│   ├── button_driver.c       # GPIO button handling
│   ├── epaper_driver_full.c  # E-paper display controller
│   ├── flash_storage.c       # Persistent storage
│   ├── gfx.c                 # Graphics library
│   └── include/              # Driver headers
├── Profile/                  # GATT profile (from SDK)
├── HAL/                      # Hardware abstraction (from SDK)
├── StdPeriphDriver/          # Peripheral drivers (from SDK)
├── .cproject                 # MounRiver Studio project
└── README.md                 # This file
```

## Troubleshooting

### Build Errors

**Error**: `fatal error: epaper_driver.h: No such file or directory`
- **Solution**: Clean and rebuild project. Ensure `Drivers/include` is in include paths.

**Error**: `multiple definition of 'EPaper_Init'`
- **Solution**: Ensure `epaper_driver.c` stub is excluded in `.cproject` sourceEntries.

**Error**: `region 'RAM' overflowed`
- **Solution**: This project requires CH583 (32KB RAM). CH572 has insufficient memory.

### Runtime Issues

**Display not updating**
- Check SPI connections (CS, DC, RST, BUSY pins)
- Verify BUSY pin is not stuck low
- Try full refresh: `EPaper_Clear(0xFF);`

**BLE not advertising**
- Check BLE_MEMHEAP_SIZE (minimum 6KB)
- Verify MAC address is configured (BLE_MAC = TRUE)
- Use BLE scanner app to verify advertisement

**Sensor reading 0.0**
- Verify I2C connections (SCL=PB13, SDA=PB12)
- Check SHT4x power supply (3.3V)
- Test with I2C scanner tool

## License

This project includes code from:
- WCH CH583 SDK (Nanjing Qinheng Microelectronics Co., Ltd.)
- Original FoodLabel application (converted from ESP32-C3)

## Version History

- **V1.1** (2025-12-06): Migrated to CH583, 32KB RAM support
- **V1.0** (2025-12-05): Initial CH572 implementation (insufficient RAM)
