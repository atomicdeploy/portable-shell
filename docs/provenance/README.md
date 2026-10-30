# Provenance and historical evidence

This directory preserves source identities, attribution, integration comparisons,
old naming maps and earlier verification captures. Product names here identify
their actual sources; they are not PortableShell APIs, build profiles or brands.

- [Device integration](device-integration.md)
- [Serial integration](serial-integration.md)
- [Control design](control-design.md)
- [Workspace design](workspace-design.md)
- [Command profile](command-profile.md)
- [Runtime reconciliation](runtime-history.md)
- [Preservation audit](preservation-audit.md)
- [Historical verification](verification-history.md)

The captures in `previews/` are retained historical evidence, not current release
previews. The current gallery lives in the repository's root `previews/` and is
rebuilt from current source by `tools/rebuild-previews.cmake`.
The image rebuild workflow and shared device fixture were adapted from the
serial consumer's MIT-licensed preview tooling; the C replay renderer is owned here.

## Recorded consumers

Current coordinated consumers: `atomicdeploy/PakarController` (application owner
implements device behavior) and `DRSDavidSoft/Ardush` (serial-console adapter).
Keep their existing source locks and review ledgers; do not create divergent local
copies of shared headers. Test results and deployed/physical acceptance remain
separate evidence.
