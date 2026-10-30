# CI summary design reference

The build-summary presentation was compared with
[atomicdeploy/pu850-esp build.yml](https://github.com/atomicdeploy/pu850-esp/blob/main/.github/workflows/build.yml)
(blob `54b261dcf52514af2a2a1664ea87ba4852199d57`) and the sibling application's
artifact-upload workflow. PortableShell adopts the emoji section headings,
configuration tables, sizes and integrity/download information.

The reporting implementation is new cross-platform CMake code. It uses SHA-256,
upload-action artifact URLs, and actual step outcomes; it does not source shell
metadata, poll artifact APIs, or use remote progress badge services. No firmware
flash percentage is invented for a host executable.
