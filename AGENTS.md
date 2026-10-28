# PortableShell conventions

- All shell, parser, TUI and Telnet logic stays freestanding C, with no runtime or OS calls.
- Only the optional C host adapters perform native terminal, serial and socket I/O.
- Default presentation should feel vibrant and polished. Prefer violet/magenta or red accents;
  do not default to an industrial dark-blue/pale-cyan appearance.
- Preserve the original Lumen color scheme as an available theme.
- Use actual key symbols in Unicode UI hints: ↑/↓, ←/→ and related characters rather than
  generic words such as "Arrows". Supply legible text fallbacks in ASCII mode.
- Terminal pictures must come from emitted program output. Include a dark Agave rendition,
  verify the font actually used, and render with proper antialiasing. Label replay fixtures.
- Preserve running user sessions and their in-memory data when publishing a new build.
- Import other TUIs only after the user supplies the intended projects/files and scope.
- Source/test/render evidence does not imply hardware compatibility or human acceptance.
