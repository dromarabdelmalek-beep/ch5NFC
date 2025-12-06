# MounRiver Studio Build Error Fix

## Problem
After opening the Peripheral project, you see errors like:
```
fatal error: CONFIG.h: No such file or directory
Invalid project path: Include path not found (Peripheral\HAL\include)
```

## Solution

### Step 1: Clean the Project
```
1. Right-click on "Peripheral" project in Project Explorer
2. Select "Clean Project"
3. Wait for clean to complete
```

### Step 2: Refresh the Project
```
1. Right-click on "Peripheral" project
2. Select "Refresh" (or press F5)
3. Wait for refresh to complete
```

### Step 3: Rebuild Index
```
1. Right-click on "Peripheral" project
2. Select "Index" → "Rebuild"
3. Wait for indexing to complete (check progress in bottom-right)
```

### Step 4: Build Project
```
1. Right-click on "Peripheral" project
2. Select "Build Project" (or press Ctrl+B)
3. Should build successfully now
```

## If Still Having Errors

### Option A: Close and Reopen
```
1. Close MounRiver Studio completely
2. Reopen MounRiver Studio
3. Open project again
4. Try building
```

### Option B: Re-import Project
```
1. Close project (right-click → Close Project)
2. Delete from workspace (DO NOT delete files on disk)
3. File → Import → Existing Projects into Workspace
4. Browse to: ch5NFC/EVT/EXAM/BLE/Peripheral
5. Click Finish
6. Build project
```

### Option C: Clean Workspace
```
1. Close MounRiver Studio
2. Navigate to workspace directory
3. Delete .metadata folder (this resets workspace settings)
4. Reopen MounRiver Studio
5. Re-import project
6. Build
```

## Verify Include Paths

After importing, verify include paths are correct:
```
1. Right-click project → Properties
2. C/C++ General → Paths and Symbols
3. Includes tab → GNU C
4. Should see these paths:
   ✓ ${workspace_loc:/Peripheral/APP/include}
   ✓ ${workspace_loc:/Peripheral/Drivers/include}
   ✓ ${workspace_loc:/Peripheral/Profile/include}
   ✓ ${workspace_loc:/Peripheral/HAL/include}
   ✓ ${workspace_loc:/Peripheral/StdPeriphDriver/inc}
   ✓ ${workspace_loc:/Peripheral/RVMSIS}
   ✓ ${workspace_loc:/Peripheral/Startup}
   ✓ ${workspace_loc:/Peripheral/LIB}
   ✓ ${workspace_loc:/Peripheral/Ld}
```

## Expected Build Output

Successful build should show:
```
Building file: ../APP/foodlabel.c
Building file: ../APP/foodlabel_main.c
Building file: ../Drivers/button_driver.c
Building file: ../Drivers/epaper_gdey029t94.c
Building file: ../Drivers/flash_storage.c
Building file: ../Drivers/gfx.c
Building file: ../Drivers/sht4x_driver.c
...
Linking target: Peripheral.elf
Finished building target: Peripheral.elf
Creating hex file: Peripheral.hex
Creating lst file: Peripheral.lst
Invoking: GNU RISC-V Cross Print Size
   text    data     bss     dec     hex filename
  65432    2048    8764   76244   129f4 Peripheral.elf

Build Finished. 0 errors, 0 warnings.
```

## Common Causes

### 1. Symlinks Not Recognized (Windows)
Windows may not recognize Linux symlinks properly. If this is the issue:
- Use "Git for Windows" with symlink support enabled
- Or run MounRiver Studio in WSL2 (Windows Subsystem for Linux)
- Or manually copy SDK directories instead of using symlinks

### 2. Workspace Variable Not Set
The ${ProjName} variable might not be expanded correctly.
- Close and reopen project
- Or manually set project name in .project file to "Peripheral"

### 3. CDT Indexer Out of Sync
Eclipse CDT indexer might be out of sync:
- Project → C/C++ Index → Rebuild
- Wait for indexing to complete (can take 1-2 minutes)

## Alternative: Build via Command Line

If MounRiver GUI continues to have issues, build from command line:

```bash
cd /path/to/ch5NFC/EVT/EXAM/BLE/Peripheral
make clean
make

# Should see output:
# CC APP/foodlabel.c
# CC APP/foodlabel_main.c
# CC Drivers/button_driver.c
# ...
# LD Peripheral.elf
# OBJCOPY Peripheral.hex
# Build complete!
```

Then flash using WCH-Link:
```bash
wch-link flash obj/Peripheral.hex
```

## Verification

After successful build, verify files exist:
```
obj/
├── Peripheral.elf  (main executable)
├── Peripheral.hex  (flash image)
├── Peripheral.lst  (assembly listing)
└── *.o             (object files)
```

## Still Not Working?

If none of the above works, the issue might be:
1. **Wrong workspace**: Make sure workspace path doesn't contain spaces
2. **Corrupted project**: Delete .cproject and .project, let MounRiver regenerate
3. **Toolchain issue**: Verify RISC-V toolchain is installed (riscv-none-embed-gcc)
4. **SDK missing**: Ensure EVT/EXAM/SRC directories exist with SDK files

Check toolchain:
```bash
riscv-none-embed-gcc --version
# Should show: riscv-none-embed-gcc (xPack ...) 10.2.0 or newer
```

---

**Most likely solution**: Clean Project + Refresh (F5) + Rebuild Index should fix it!
