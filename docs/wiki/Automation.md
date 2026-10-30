# 🛡️ CI, security and documentation publishing

## Build evidence

The build workflow runs native Windows/Linux/macOS checks, explicit feature
profiles, ARM32 archive builds, AVR component audits, and preview regeneration.
Each job publishes a summary with its actual step outcomes, configuration,
commit, binary sizes, SHA-256 hashes and artifact download link. Native jobs
also capture the core session storage and heap report. Missing binaries are
reported as missing, never as zero-size successes. Failure logs and JUnit reports
are retained with artifacts for 14 days. No summary depends on an external badge
service or executes a sourced metadata file.

The reusable reporting scripts use CMake: `tools/ci/summary.cmake` and
`tools/ci/profile.cmake`. They are host tooling, not runtime dependencies.
Binary size is not RAM usage; the session profile excludes process, adapter and
stack memory. Cross-compilation is not board execution.

## CodeQL and dependencies

CodeQL manually builds the C/C++ targets on Linux and Windows, including tests
that instantiate shared C++ headers. A separate Actions scan checks workflows.
It runs on pull requests, main pushes, manual dispatch and a weekly schedule.
[GitHub's compiled-language guidance](https://docs.github.com/en/code-security/how-tos/find-and-fix-code-vulnerabilities/manage-your-configuration/codeql-for-compiled-languages)
explains why manual builds determine which compiled code is analyzed.

Dependabot checks GitHub Actions weekly; minor/patch updates are grouped and major
updates remain separate. Actions are pinned to commit IDs with version comments.
There is no package-manager dependency in the freestanding runtime. Dependabot
does not update the separately checksum-pinned Agave/resvg downloads: maintainers
must review their versions, hashes and regenerated previews together.

See the [security policy](https://github.com/atomicdeploy/portable-shell/blob/main/SECURITY.md).

## Wiki ownership and publication

Edit `docs/wiki/*.md` in the main repository. These are the canonical, reviewable,
offline sources. The wiki is the published reading surface; direct edits to managed
wiki pages are replaced on the next publication. Unknown wiki pages are preserved.
Historical source identity, licenses and archived design evidence remain in
`docs/provenance/` and `docs/legacy/`, rather than becoming current user guidance.
Root `OPTIONS.md`, `PORTABILITY.md` and `CONSUMERS.md` remain forwarding pages.

```text
cmake -P tools/check-public-docs.cmake
cmake -P tools/export-wiki.cmake
git clone https://github.com/atomicdeploy/portable-shell.wiki.git build/wiki-checkout
cmake -DWIKI_DIR=build/wiki-checkout -P tools/publish-wiki.cmake
```

The first wiki page must exist before GitHub exposes its Git repository. The
publisher requires a clean checkout, pulls fast-forward only, stages only managed
pages, and never force-pushes or deletes unmanaged pages. Existing wiki content
remains in its Git history.

The documentation workflow always validates and exports a downloadable wiki
bundle. Main-branch publication additionally needs a repository secret named
`WIKI_TOKEN` with permission to write this repository's wiki. Use a token accepted
by GitHub for wiki Git access; do not paste credentials into documentation or
workflow files. PR runs never publish. Without the secret the summary explicitly
reports that publication was skipped; successful export is not a publication claim.
