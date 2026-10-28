/* Freestanding application-owned panel. No hardware or OS dependencies.
   Compile with the core. Retain the model for the entire session lifetime.
   Replace transport submission/readback with your existing device protocol. */
#include "portable_shell.h"
typedef struct {
    ps_panel panel;
    ps_control_value observed[2];
    unsigned requested[2];
} application_controls;
static const ps_control outputs[] = {{0, "RELAYS", "Supply", PS_CONTROL_TOGGLE, 0, 1, 1, 0, 0},
                                     {1, "PWM", "Fan", PS_CONTROL_LEVEL, 0, 4095, 41, 0, 0}};
static const ps_control_page pages[] = {{"Control", outputs, 2}};
static int read_control(void *context, unsigned id, ps_control_value *value) {
    application_controls *model = (application_controls *)context;
    if (id >= 2)
        return 0;
    *value = model->observed[id];
    return 1;
}
static int write_control(void *context, unsigned id, const ps_control_value *value,
                         ps_u32 expected_revision) {
    application_controls *model = (application_controls *)context;
    if (id >= 2 || model->observed[id].revision != expected_revision ||
        model->observed[id].flags != PS_CONTROL_VALID)
        return 0;
    /* Submit once from your owning loop. Do not copy this request to observed. */
    model->requested[id] = value->number;
    model->observed[id].flags |= PS_CONTROL_PENDING;
    return 2;
}
int application_controls_attach(ps_session *s, application_controls *model) {
    unsigned i;
    for (i = 0; i < 2; ++i) {
        model->observed[i].number = 0;
        model->observed[i].flags = 0; /* Unknown until actual readback. */
        model->observed[i].revision = 0;
        model->observed[i].text[0] = 0;
        model->requested[i] = 0;
    }
    model->panel.title = "Device controls";
    model->panel.status = "External model";
    model->panel.pages = pages;
    model->panel.count = 1;
    model->panel.read = read_control;
    model->panel.write = write_control;
    model->panel.context = model;
    return ps_panel_attach(s, &model->panel);
}
/* Invoke on validated, current-connection readback, including after a write.
   The application must fence stale connection generations before calling this. */
void application_controls_observe(ps_session *s, application_controls *model, unsigned id,
                                  unsigned number, unsigned valid, unsigned locked) {
    if (id >= 2)
        return;
    model->observed[id].number = number;
    model->observed[id].flags = (valid ? PS_CONTROL_VALID : 0) | (locked ? PS_CONTROL_LOCKED : 0);
    ++model->observed[id].revision;
    ps_panel_refresh(s);
}
