# RPMegaChopper

**RPMegaChopper** is a clone of the classic game *Choplifter*, developed for the [Picocomputer 6502 (RP6502)](https://picocomputer.github.io).

## Gameplay

Command your helicopter behind enemy lines! Your mission is to rescue hostages being held in prisoner of war camps. 

- **Fly**: Navigate your chopper through hostile territory.
- **Rescue**: Land near the barracks to pick up hostages.
- **Return**: Transport them safely to the US base.
- **Survive**: Avoid or destroy enemy tanks and jet fighters.

## Development

This project uses the [LLVM-MOS](https://llvm-mos.org/) compiler suite and CMake for the build system.

### Prerequisites

1.  **LLVM-MOS SDK**: Ensure you have the LLVM-MOS SDK installed and available in your path (or configured in CMake presets).
2.  **CMake**: Version 3.18 or higher.
3.  **Python 3**: Required for the asset conversion tools and upload scripts.
4.  **Python Libraries**: `pyserial` and `Pillow` (for image processing).
    ```bash
    pip install pyserial Pillow
    ```

### Building

This project is configured with CMake presets.

1.  **Configure**:
    ```bash
    cmake --preset jason-local
    ```
    *(Note: You may need to adjust `CMakeUserPresets.json` to point to your specific LLVM-MOS installation path.)*

2.  **Build**:
    ```bash
    cmake --build build
    ```

### Running

Connect your RP6502 via USB.

```bash
./tools/rp6502.py run build/RPMegaChopper.rp6502
```

### Asset Pipeline

Graphics are stored in the `Sprites/` directory as PNG files. They are converted into RP6502-compatible binary formats using the `Sprites/convert_sprite.py` script.

- **Sprites**: Converted to 16-bit RGB555 format (with alpha transparency).
- **Tiles**: Converted to 4-bit IRGB format.

Example conversion:
```bash
python3 Sprites/convert_sprite.py Sprites/Chopper.png -o images/Chopper.bin --mode sprite
```

## Credits

- Original *Choplifter* game by Dan Gorlin.
- RP6502 Platform by Rumbledethumps.
