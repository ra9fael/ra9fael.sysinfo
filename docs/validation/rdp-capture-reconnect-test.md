# RDP capture reconnect validation

## Result

The glass backdrop resumed across the full panel after RDP disconnect and
reconnect. The partial stale capture did not return.

The test used the locally installed `ra9fael.sysinfo` 1.6.5 build on Windows
with the capture diagnostics enabled.

## Automated coverage

- `third_party/liquid-glass-cpp`: 21/21 CTest cases passed.
- `sysinfo_core`: 1/1 passed.
- Regression cases include `render-capture-disconnect-reconnect`,
  `render-capture-invalid-call`, and `render-capture-not-found`.
