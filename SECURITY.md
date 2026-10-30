# 🛡️ Security policy

Please report suspected vulnerabilities through
[GitHub private vulnerability reporting](https://github.com/atomicdeploy/portable-shell/security/advisories/new).
Include the commit, build flags, transport, input bytes, and a minimal reproduction.
Do not publish credentials or sensitive device data.

Security fixes target the current `main` branch. There is no promised response-time
SLA or maintained historic release series. CodeQL scans C/C++ and GitHub Actions;
Dependabot proposes action updates. Neither replaces protocol validation or review.
The default TCP listener is loopback-only. Telnet is plaintext and has no built-in
authentication; applications own authorization and deployment boundaries.
