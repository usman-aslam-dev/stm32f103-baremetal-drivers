# MISRA C:2012 analysis scope

This project uses cppcheck's MISRA addon as an **advisory** static-analysis
step. It is not a claim of MISRA compliance: the addon covers only a subset of
MISRA C:2012, and this repository has not been assessed with a qualified
compliance toolchain or a formal deviation process.

## Project-level deviations / review notes

- Direct register access uses integer-to-pointer mappings for memory-mapped I/O.
  This is required by the MCU programming model and is isolated in the device
  header.
- Hardware register objects are `volatile`; host tests replace those mappings
  with fake register storage where applicable.
- Busy-wait loops are limited to hardware bring-up or bounded peripheral timing
  paths; production systems should add watchdog/recovery policies appropriate
  to the safety requirements.
- No dynamic allocation is used in the firmware data path.

Any MISRA findings produced by CI should be reviewed individually. A green
advisory job means only that the configured checker found no reportable issues;
it does **not** establish MISRA compliance.
