## Summary

<!-- What changed and why does it belong in the SDK? -->

## Compatibility

<!-- Public ABI, wire protocol, capability, CMake consumer, or migration impact. -->

## Validation

<!-- Commands run and any PS5 hardware/firmware validation. -->

## Checklist

- [ ] The change has one clear responsibility and no unrelated refactor.
- [ ] Host tests pass with `ctest --test-dir build --output-on-failure`.
- [ ] New behavior and failure paths have focused tests.
- [ ] Public structures remain versioned, fixed-width, and C-compatible.
- [ ] `docs/architecture.md` is updated for ABI, lifecycle, service, or transport changes.
- [ ] Both READMEs are updated for user-facing setup or capability changes.
- [ ] No credentials, proprietary SDK files, keys, or console dumps are included.

