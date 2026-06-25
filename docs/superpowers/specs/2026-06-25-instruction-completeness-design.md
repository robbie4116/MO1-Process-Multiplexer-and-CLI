# Instruction Completeness Design

## Scope

Close the remaining MO1 specification gaps after scheduler/config correctness:

- `PRINT` may append one process variable.
- `FOR` executes a body repeatedly and supports nesting to depth 3.
- Random programs include `FOR`.
- Instruction counters no longer narrow configured `uint32` values to signed `int`.
- Submission documentation accurately describes and runs the program.

## Execution model

Instruction trees are expanded once when a `Process` is constructed. Expansion recursively copies ordinary instructions into a flat executable stream and expands each `FOR` body `forRepeats` times. Recursion deeper than three nested loops is rejected. The scheduler therefore retains its existing one-flat-instruction-per-CPU-tick behavior.

`totalInstructions` and `currentInstruction` use 64-bit unsigned counters. The random generator receives `uint32_t` bounds and constructs a program whose expanded instruction count equals the selected configured count.

## PRINT variables

An instruction stores a literal message and an optional variable name. At execution, an undeclared referenced variable is automatically created with value `0`, matching arithmetic operand behavior. The log output concatenates the literal message and decimal variable value.

## Random FOR generation

The generator uses a bounded recursive helper. It may replace a portion of the remaining executable-instruction budget with a `FOR` node whose expanded body consumes exactly that budget. Maximum generated nesting depth is three. A seeded overload is provided for deterministic tests while production uses the existing time-seeded engine.

## Error handling

Malformed loops with empty bodies, zero repeats, excessive nesting, or expanded counts that overflow addressable memory are rejected with `std::invalid_argument` or `std::length_error`. Process creation catches generation/construction failures at the CLI boundary and reports an error instead of terminating the emulator.

## Verification

Deterministic tests cover variable printing, undeclared variables, nested loop execution, depth rejection, exact expanded counts, seeded random `FOR` generation, and 64-bit instruction counters. Existing scheduler and black-box configuration tests remain mandatory.
