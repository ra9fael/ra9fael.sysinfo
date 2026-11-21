# Lock, resume and RDP reconnect validation

## Status

Windows lock/unlock and RDP disconnect/reconnect were verified with the native
glass overlay. After reconnect, the full backdrop region resumed instead of
retaining a partial stale capture.

## Recovery

Session lock releases Desktop Duplication while preserving the last backdrop.
Unlock marks the backdrop and blur cache dirty, recreates output duplication,
and draws immediately. RDP disconnect invalidates duplication; reconnect marks
the backdrop dirty and retries against the current monitor output.

The RDP reconnect regression is covered by `render-capture-disconnect-reconnect`.
The full library suite passed 21/21 cases, and `sysinfo_core` passed 1/1. Manual
RDP disconnect/reconnect passed on the locally installed 1.6.5 test build.
