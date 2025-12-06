# Windows MounRiver Studio - Linked Resources Fix

## Problem
On Windows, MounRiver Studio shows:
```
fatal error: CH58xBLE_LIB.h: No such file or directory
Invalid project path: Include path not found (Peripheral\HAL\include)
Invalid project path: Include path not found (Peripheral\StdPeriphDriver\inc)
```

This happens because **Windows doesn't handle Linux symlinks** the same way.

## Solution: Use Eclipse Linked Resources

The `.project` file already has linked resources configured, but MounRiver needs to recognize them.

### Method 1: Close and Re-open Project (FASTEST)

1. **Close MounRiver Studio completely**
2. **Reopen MounRiver Studio**
3. **Right-click "Peripheral" project**
4. **Select "Close Project"**
5. **Right-click "Peripheral" again**
6. **Select "Open Project"**
7. **Verify linked resources appear**:
   - Look in Project Explorer
   - You should see HAL, LIB, Ld, etc. with a chain-link icon 🔗
8. **Clean and Build**:
   - Right-click → Clean Project
   - Right-click → Build Project

### Method 2: Delete and Re-import Project

If Method 1 doesn't work:

1. **Right-click "Peripheral" → Delete**
   - ⚠️ **IMPORTANT**: UNcheck "Delete project contents on disk"
   - Only delete from workspace, NOT from filesystem!
2. **File → Import → General → Existing Projects into Workspace**
3. **Browse to**: `ch5NFC/EVT/EXAM/BLE/Peripheral`
4. **Check "Peripheral"**
5. **Click "Finish"**
6. **Verify linked resources** (should see 🔗 icons)
7. **Build Project**

### Method 3: Manually Verify Linked Resources

1. **Right-click "Peripheral" → Properties**
2. **Resource → Linked Resources**
3. **Should see these PATH variables**:
   ```
   PARENT-1-PROJECT_LOC = /path/to/EVT/EXAM/BLE
   PARENT-2-PROJECT_LOC = /path/to/EVT/EXAM
   ```
4. **Should see these Linked Resources**:
   ```
   HAL         → PARENT-1-PROJECT_LOC/HAL
   LIB         → PARENT-1-PROJECT_LOC/LIB
   Ld          → PARENT-2-PROJECT_LOC/SRC/Ld
   RVMSIS      → PARENT-2-PROJECT_LOC/SRC/RVMSIS
   Startup     → PARENT-2-PROJECT_LOC/SRC/Startup
   StdPeriphDriver → PARENT-2-PROJECT_LOC/SRC/StdPeriphDriver
   ```

If any are missing:

5. **Click "New..."**
6. **Add missing linked resources manually**

### Method 4: Windows Symlink Setup (Advanced)

If you want real symlinks to work on Windows:

**Option A: Enable Developer Mode (Windows 10/11)**
```
1. Settings → Update & Security → For Developers
2. Enable "Developer Mode"
3. Restart computer
4. Symlinks will now work without admin privileges
```

**Option B: Git Bash with Symlinks**
```bash
# Re-clone repository with symlinks enabled
git clone -c core.symlinks=true <repository-url>
```

**Option C: WSL2 (Windows Subsystem for Linux)**
```
1. Install WSL2
2. Clone repository in WSL2
3. Run MounRiver Studio from WSL2
4. Symlinks work natively
```

## Verify It's Working

After fixing linked resources, verify in Project Explorer:

```
Peripheral/
├── APP/
├── Drivers/
├── Profile/
├── HAL/ 🔗            ← Should have chain-link icon
├── LIB/ 🔗            ← Should have chain-link icon
├── Ld/ 🔗             ← Should have chain-link icon
├── RVMSIS/ 🔗         ← Should have chain-link icon
├── Startup/ 🔗        ← Should have chain-link icon
└── StdPeriphDriver/ 🔗 ← Should have chain-link icon
```

If you see the 🔗 chain-link icons, linked resources are working!

## Build Test

After linked resources are recognized:

1. **Clean Project**: Right-click → Clean Project
2. **Refresh**: Right-click → Refresh (F5)
3. **Rebuild Index**: Right-click → Index → Rebuild
4. **Build**: Right-click → Build Project (Ctrl+B)

Should see:
```
Building file: ../APP/foodlabel.c
Building file: ../Drivers/button_driver.c
...
Linking target: Peripheral.elf
Build Finished. 0 errors, 0 warnings.
```

## Still Not Working?

### Check Include Paths

1. **Right-click project → Properties**
2. **C/C++ General → Paths and Symbols**
3. **Includes tab → GNU C**
4. **Verify these paths exist**:
   ```
   ${workspace_loc:/Peripheral/LIB}
   ${workspace_loc:/Peripheral/HAL/include}
   ${workspace_loc:/Peripheral/StdPeriphDriver/inc}
   ```

If paths show errors (red X):
- The linked resources aren't being recognized
- Try Method 2 (Delete and Re-import)

### Manually Add LIB to Include Paths

As a workaround, manually add the BLE library:

1. **Right-click project → Properties**
2. **C/C++ General → Paths and Symbols**
3. **Includes → Add...**
4. **Directory**: `${workspace_loc:/Peripheral/LIB}`
5. **Check "Add to all configurations"**
6. **Click OK → Apply → OK**
7. **Clean and Build**

## Alternative: Use Absolute Paths (Not Recommended)

If nothing else works, you can use absolute paths:

1. **Find absolute paths**:
   ```bash
   cd /path/to/ch5NFC/EVT/EXAM/BLE
   pwd  # Note this path
   cd ../SRC
   pwd  # Note this path
   ```

2. **Right-click project → Properties**
3. **C/C++ General → Paths and Symbols**
4. **Add absolute paths**:
   ```
   C:/Users/YourName/ch5NFC/EVT/EXAM/BLE/LIB
   C:/Users/YourName/ch5NFC/EVT/EXAM/BLE/HAL/include
   C:/Users/YourName/ch5NFC/EVT/EXAM/SRC/StdPeriphDriver/inc
   ```

⚠️ **Not recommended**: Paths won't work if you move the project or use on another computer.

## Expected Result

After fixing linked resources, build output should be:
```
**** Build of configuration obj for project Peripheral ****
make all
Building file: ../APP/foodlabel_main.c
Invoking: GNU RISC-V Cross C Compiler
riscv-none-embed-gcc ... -c -o "APP/foodlabel_main.o" "../APP/foodlabel_main.c"
Finished building: ../APP/foodlabel_main.c

Building file: ../APP/foodlabel.c
...
Building target: Peripheral.elf
Invoking: GNU RISC-V Cross C Linker
riscv-none-embed-gcc ... -o "Peripheral.elf" ...
Finished building target: Peripheral.elf

Invoking: GNU RISC-V Cross Create Flash Image
riscv-none-embed-objcopy -O ihex "Peripheral.elf" "Peripheral.hex"
Finished building: Peripheral.hex

Invoking: GNU RISC-V Cross Print Size
riscv-none-embed-size --format=berkeley "Peripheral.elf"
   text    data     bss     dec     hex filename
  67234    2156    9012   78402   13242 Peripheral.elf
Finished building: Peripheral.siz

**** Build Finished ****
```

---

**TL;DR**:
1. Close MounRiver completely
2. Reopen and close/open the Peripheral project
3. Linked resources should appear with 🔗 icons
4. Clean and Build → Should work!
