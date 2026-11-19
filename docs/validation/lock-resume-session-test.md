# Lock and resume session test

## Status

Manual Windows lock and resume verified: the glass effect resumed after unlock.

## Recovery

The renderer releases Desktop Duplication on session lock. On session unlock it marks the backdrop and blur cache dirty, recreates output duplication, and draws immediately.

