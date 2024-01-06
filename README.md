# BrickBoy

WIP Gameboy Emulator

## Building

You will need `cmake`, `make` and a C compiler (GCC or Clang).

## Blargg’s tests

 - [x] 01-special
 - [ ] 02-interrupts
 - [ ] 03-op sp,hl
 - [x] 04-op r,imm
 - [x] 05-op rp
 - [x] 06-ld r,r
 - [ ] 07-jr,jp,call,ret,rst
 - [ ] 08-misc instrs
 - [ ] 09-op r,r
 - [x] 10-bit ops
 - [ ] 11-op a,(hl)

```bash
cmake .
make
```

## Resources

- [Pan Docs](https://gbdev.io/pandocs/)
- [CPU Manual](http://marc.rawer.de/Gameboy/Docs/GBCPUman.pdf)
- [Opcodes Table](https://meganesu.github.io/generate-gb-opcodes/) by meganesu
- [Gameboy Logs](https://github.com/wheremyfoodat/Gameboy-logs/tree/master) by wheremyfoodat
