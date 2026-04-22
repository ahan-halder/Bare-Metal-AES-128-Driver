# Development Notes

This file is kept as a short pointer. The consolidated setup, build, test, and integration guide now lives in [README.md](README.md).

### Hardware Resources

| Resource | Consumption |
|----------|------------|
| LUTs | ~15K (estimated) |
| FFs | ~3K (estimated) |
| Block RAM | 256 bytes (S-box) |
| Max Clock | 200+ MHz (technology dependent) |

### Throughput

At 200 MHz with 11-cycle latency:
- **Throughput**: 128 bits / 11 cycles = ~1.45 Gbps
- **Latency**: 55 ns

---

## Verification Results

All tests validate against NIST standard AES-128 test vectors:

✓ Encryption correctness
✓ Decryption correctness
✓ Round-trip (encrypt then decrypt) verification
✓ Hardware status and control signals
✓ MMIO register interface

---

## Debugging Tips

### Register Inspection

Print hardware status:
```c
printf("Status: 0x%08x\n", aes_get_status(&ctx));
```

### Waveform Analysis

Enable Verilator tracing:
```bash
make rtl_sim
./obj_dir/Vaes_128_wrapper --trace
gtkwave dump.vcd &
```

### Adding Debug Output

Modify `testbench.c` to add detailed register dumps:
```c
printf("CTRL: 0x%08x, STATUS: 0x%08x\n",
    aes_read_reg(AES_REG_CTRL),
    aes_read_reg(AES_REG_STATUS));
```

---

## License

This project is provided for educational purposes as part of a SoC design and silicon bring-up curriculum.

---

**Generated**: 2026-04-16  
**Project Status**: Complete - All phases implemented and tested
