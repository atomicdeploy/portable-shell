# PortableShell conventions

- Shell, parser and Telnet logic remains OS-independent C. The optional snapshot/action
  device view and callback facade may use freestanding C++11, with no OS or C++ runtime dependency.
  Default builds use
  freestanding primitives; the explicitly selectable libc backend may use host
  memory/string functions. Never enable libc silently for a firmware build.
- Only the optional C host adapters perform native terminal, serial and socket I/O.
- Default presentation should feel vibrant and polished. Prefer violet/magenta or red accents;
  do not default to an industrial dark-blue/pale-cyan appearance.
- Preserve the original Lumen color scheme as an available theme.
- Name implementation APIs, files, macros and profiles by their function, never by
  a donor product. Keep exact donor identities, legal attribution and old-name
  migration mappings in provenance documents and Git history.
- Use actual key symbols in Unicode UI hints: ↑/↓, ←/→ and related characters rather than
  generic words such as "Arrows". Supply legible text fallbacks in ASCII mode.
- Terminal pictures must come from emitted program output. Include a dark Agave rendition,
  verify the font actually used, and render with proper antialiasing. Label replay fixtures.
- Existing table values must support editing in their own cells; dialogs and
  below-table prompts must not be the only editing path.
- Preserve running user sessions and their in-memory data when publishing a new build.
- Import other TUIs only after the user supplies the intended projects/files and scope.
- Source/test/render evidence does not imply hardware compatibility or human acceptance.
