# GDEY029T94 E-Paper Display Setup Guide

## Display Specifications

**Model**: GDEY029T94
**Manufacturer**: Good Display
**Size**: 2.9 inches diagonal
**Resolution**: 296 × 128 pixels
**Controller**: UC8151D (also known as IL0373)
**Interface**: 4-wire SPI
**Voltage**: 3.3V
**Viewing Area**: 66.9mm × 29.06mm
**Dot Pitch**: 0.227mm × 0.227mm
**Color**: Black & White (1-bit)
**Refresh Time**: ~2 seconds (full), ~1 second (partial)
**Viewing Angle**: >170°
**Power Consumption**: ~0µA (sleep), ~26.4mW (active @3.3V)

## Pin Configuration

### CH583 to GDEY029T94 Wiring

```
GDEY029T94 Pin    CH583 Pin     Description
══════════════    ═════════     ═══════════════════════════════
VCC               3.3V          Power supply (3.3V only!)
GND               GND           Ground
DIN (MOSI)        PA13          SPI0 Data In (MOSI)
CLK (SCK)         PA12          SPI0 Clock
CS                PA4           Chip Select (active LOW)
DC                PA1           Data/Command select (LOW=CMD, HIGH=DATA)
RST (RESET)       PA2           Reset (active LOW)
BUSY              PA3           Busy indicator (HIGH=busy, LOW=idle)
```

### Detailed Pin Connections

```
CH583 Development Board
┌─────────────────────────────────┐
│                                 │
│  PA12 (SPI0_SCK)  ────────────────> CLK   (E-Paper)
│  PA13 (SPI0_MOSI) ────────────────> DIN   (E-Paper)
│  PA4              ────────────────> CS    (E-Paper)
│  PA1              ────────────────> DC    (E-Paper)
│  PA2              ────────────────> RST   (E-Paper)
│  PA3              <────────────────  BUSY  (E-Paper)
│                                 │
│  3.3V             ────────────────> VCC   (E-Paper)
│  GND              ────────────────> GND   (E-Paper)
│                                 │
└─────────────────────────────────┘
```

## Hardware Setup

### Required Components

1. **CH583 Development Board** (QFN48 package recommended)
2. **GDEY029T94 E-Paper Display** with FPC connector
3. **24-pin FPC breakout board** (if display doesn't have pin headers)
4. **Jumper wires** (male-to-female, 8 wires minimum)
5. **Breadboard** (optional, for prototyping)
6. **3.3V Power supply** (stable, low-noise)

### Important Notes

⚠️ **CRITICAL**: The GDEY029T94 operates at **3.3V only**. DO NOT connect to 5V!

⚠️ **Power Supply**: E-paper displays require clean, stable 3.3V. Use a quality voltage regulator (e.g., AMS1117-3.3) if powered from battery or USB.

⚠️ **ESD Protection**: E-paper displays are sensitive to static electricity. Handle with care and use ESD protection.

⚠️ **Refresh Cycles**: Limit full refreshes to extend display lifetime. Use partial refresh when possible.

### Physical Mounting

The GDEY029T94 typically comes with:
- FPC (Flexible Printed Circuit) connector
- Optional breakout board with pin headers

**If using FPC breakout board:**
1. Carefully insert FPC cable into breakout connector
2. Lock the connector (usually a flip-lock mechanism)
3. Verify pin numbering matches the datasheet
4. Connect breakout pins to CH583

**Display Orientation:**
- The display can be mounted in landscape (296×128) orientation
- Default orientation has 296 pixels width, 128 pixels height
- Ensure correct orientation when designing enclosure

## Software Configuration

### 1. Enable GDEY029T94 Driver

In `APP/include/CONFIG.h`:

```c
/*********************************************************************
 * E-PAPER Display Configuration
 */
#define EPAPER_GDEY029T94               1  // Use GDEY029T94 driver
```

### 2. Build Configuration

**MounRiver Studio:**
1. Open project `.cproject` file
2. Verify `Drivers/epaper_gdey029t94.c` is included
3. Verify `Drivers/epaper_driver_full.c` is EXCLUDED (or delete it)
4. Build project (Ctrl+B)

**Command Line:**
```bash
cd EVT/EXAM/BLE/Peripheral
make clean
make
```

### 3. Pin Verification

Double-check pin definitions in `APP/include/foodlabel.h`:

```c
// E-Paper SPI Pins
#define EPAPER_CS_PIN      GPIO_Pin_4   // PA4 - Chip Select
#define EPAPER_DC_PIN      GPIO_Pin_1   // PA1 - Data/Command
#define EPAPER_RST_PIN     GPIO_Pin_2   // PA2 - Reset
#define EPAPER_BUSY_PIN    GPIO_Pin_3   // PA3 - Busy
```

## Testing the Display

### First Boot Test

1. **Power on the CH583**
2. **Watch serial output** (115200 baud on PA9):
   ```
   === SmartFood Cold Chain Monitor (CH583) ===
   EPaper Init (GDEY029T94 - UC8151D)
   EPaper initialized (296x128)
   ```

3. **Observe display**:
   - First boot: Splash screen for 3 seconds
   - Then: Full food label with temperature

### Display Test Pattern

To verify display is working correctly:

```c
// Add to foodlabel_main.c after EPaper_Init()
EPaper_Clear();  // Should show all white

// Draw test pattern (in foodlabel.c)
GFX_DrawRect(0, 0, 296, 128, GFX_COLOR_BLACK);  // Border
GFX_DrawLine(0, 64, 296, 64, GFX_COLOR_BLACK);  // Horizontal center
GFX_DrawLine(148, 0, 148, 128, GFX_COLOR_BLACK); // Vertical center
EPaper_Display();
```

### Troubleshooting

**Problem**: Display shows nothing (all white)

**Solutions**:
1. Check power supply is stable 3.3V
2. Verify all SPI connections (especially CLK and DIN)
3. Check CS, DC, RST pins are connected correctly
4. Verify BUSY pin is not stuck HIGH
5. Try hardware reset: power cycle CH583

**Problem**: Display shows garbage or corrupted image

**Solutions**:
1. Reduce SPI clock speed (modify SPI0_MasterDefInit())
2. Add delay after EPaper_Init() before first display
3. Verify framebuffer size matches display (4736 bytes)
4. Check FPC connector is fully inserted

**Problem**: Display updates very slowly

**Solutions**:
1. This is normal for e-paper (2-3 seconds full refresh)
2. Use partial refresh for temperature updates (EPaper_UpdateTemperature)
3. Limit refresh frequency to preserve display lifetime

**Problem**: BUSY pin timeout

**Solutions**:
1. Increase timeout in EPaper_WaitUntilIdle() (currently 5 seconds)
2. Check BUSY pin is connected to PA3
3. Verify BUSY pin pull-up resistor (10kΩ recommended)
4. Some displays need longer wait after power-on (add DelayMs(100))

**Problem**: Ghosting or image retention

**Solutions**:
1. Perform full refresh periodically (every 10-20 partial refreshes)
2. Use EPaper_Clear() to clear ghosting
3. Power cycle display if ghosting persists
4. Avoid displaying static images for extended periods

## GDEY029T94 vs Other Displays

### Comparison Table

| Feature | GDEY029T94 (UC8151D) | Generic (SSD1680) |
|---------|----------------------|-------------------|
| **Controller** | UC8151D/IL0373 | SSD1680 |
| **Resolution** | 296×128 | 296×128 |
| **Refresh Time** | ~2s (full) | ~2s (full) |
| **Partial Refresh** | Yes (with ghosting) | Yes (better) |
| **LUT Customization** | Supported | Supported |
| **Power (active)** | ~26.4mW | ~30mW |
| **Cost** | $10-15 | $8-12 |
| **Availability** | Good Display | Various |

### When to Use GDEY029T94

✅ **Use GDEY029T94 if**:
- You already have this display
- You need Good Display quality/reliability
- You prefer documented, proven hardware

✅ **Use Generic SSD1680 if**:
- Lower cost is priority
- Better partial refresh needed
- More flexible LUT customization required

## Advanced Configuration

### Partial Refresh Optimization

For production use, you can enable optimized partial refresh:

```c
// In epaper_gdey029t94.c, EPaper_DisplayPartial():
static void EPaper_DisplayPartial(void)
{
    // Load partial refresh LUT
    EPaper_SetLUT_Partial();

    // Send only changed regions
    EPaper_SendCommand(CMD_PARTIAL_IN);

    // Define partial window (x, y, width, height)
    EPaper_SendCommand(CMD_PARTIAL_WINDOW);
    EPaper_SendData(x_start & 0xFF);
    EPaper_SendData(x_end & 0xFF);
    EPaper_SendData((y_start >> 8) & 0xFF);
    EPaper_SendData(y_start & 0xFF);
    EPaper_SendData((y_end >> 8) & 0xFF);
    EPaper_SendData(y_end & 0xFF);
    EPaper_SendData(0x01);  // Use partial LUT

    // Update display
    EPaper_SendCommand(CMD_DISPLAY_REFRESH);
    EPaper_WaitUntilIdle();

    EPaper_SendCommand(CMD_PARTIAL_OUT);
}
```

### Custom LUT (Look-Up Table)

For faster refresh or reduced flicker, customize the LUT:

```c
// Temperature-optimized LUT (for 20-25°C operation)
const unsigned char lut_20degC[] = {
    0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00,
    // ... (full LUT table)
};

// Load custom LUT
EPaper_SendCommand(CMD_VCOM_LUT);
EPaper_SendDataBurst(lut_20degC, sizeof(lut_20degC));
```

### Power Optimization

To minimize power consumption:

```c
// After each display update
EPaper_Sleep();  // Put display in deep sleep (< 1µA)

// Before next update
EPaper_Init();   // Wake up display
```

## Display Lifetime

### Expected Lifetime

- **Full Refresh**: ~1 million cycles (Good Display spec)
- **Partial Refresh**: ~100 cycles between full refreshes recommended
- **Static Image**: Can be displayed indefinitely without damage

### Best Practices

1. **Limit refreshes**: Update only when necessary (e.g., temperature change > 2°F)
2. **Use partial refresh**: For temperature and small updates
3. **Periodic full refresh**: Every 10-20 partial refreshes to clear ghosting
4. **Sleep mode**: Put display to sleep between updates
5. **Avoid static borders**: Don't leave black borders displayed for months

## Datasheet and Resources

- **Datasheet**: Available from Good Display website
- **Controller IC**: UC8151D datasheet (online)
- **Demo Code**: Good Display provides Arduino examples
- **Technical Support**: contact@good-display.com

## Revision History

- **V1.0** (2025-12-06): Initial GDEY029T94 driver implementation
- Optimized for CH583 BLE food label application
- Full refresh mode (partial refresh in development)

---

**Your GDEY029T94 display is now ready to use with the CH583 Smart Food Label!**
