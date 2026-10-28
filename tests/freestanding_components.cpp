/* Compile/link audit: force real firmware paths into a standalone object. */
#include "portable_device_view.hpp"
extern "C" {
void component_editor(ps_editor *editor, unsigned char byte, ps_terminal_write write, void *ctx) {
    ps_editor_feed(editor, byte, write, ctx);
}
void component_view(PortableShell::DeviceView *view,
                    const PortableShell::DeviceView::Snapshot *snapshot,
                    PortableShell::DeviceView::Action *action, unsigned char byte,
                    ps_terminal_write write, void *ctx) {
    view->feed(byte, *snapshot, *action);
    view->render(*snapshot, [&](const unsigned char *p, size_t n) { write(ctx, p, n); });
}
}
