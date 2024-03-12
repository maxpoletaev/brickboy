# BrickBoy

WIP Gameboy Emulator

## Building

You will need `cmake`, `make` and a C compiler (GCC or Clang).

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```

## Status

 * [x] CPU
 * [ ] Timer
 * [ ] Background
 * [ ] Window
 * [ ] DMA
 * [ ] Sprites
 * [ ] Scrolling
 * [ ] Sound
 * [ ] Input
 * [ ] Mappers
 * [ ] Save states

## CPU tests (Blargg's)

 - [x] 01-special
 - [ ] 02-interrupts
 - [x] 03-op sp,hl
 - [x] 04-op r,imm
 - [x] 05-op rp
 - [x] 06-ld r,r
 - [x] 07-jr,jp,call,ret,rst
 - [x] 08-misc instrs
 - [x] 09-op r,r
 - [x] 10-bit ops
 - [x] 11-op a,(hl)

## Resources

- [Pan Docs](https://gbdev.io/pandocs/)
- [CPU Manual](http://marc.rawer.de/Gameboy/Docs/GBCPUman.pdf)
- [Opcodes Table](https://meganesu.github.io/generate-gb-opcodes/) by meganesu
- [Gameboy Logs](https://github.com/wheremyfoodat/Gameboy-logs) by wheremyfoodat
- [Test ROMs](https://github.com/retrio/gb-test-roms) (mirror of Blargg’s tests)
- [The Gameboy Emulator Development Guide](https://hacktix.github.io/GBEDG/)
- [Gameboy Emulation in JavaScript](https://imrannazar.com/series/gameboy-emulation-in-javascript)
- [Lazy Gameboy Emulator](https://cturt.github.io/cinoop.html)
