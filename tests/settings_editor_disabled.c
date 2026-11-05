#define PS_SETTINGS_EDITOR 0
#include "portable_settings_editor.h"
#include <assert.h>
int main(void){
    ps_settings registry;const ps_setting *slots[1];ps_settings_editor editor;
    ps_control controls[1];ps_settings_editor_row rows[1];
    ps_settings_init(&registry,slots,PS_COUNT_OF(slots));
    assert(!ps_settings_editor_init(&editor,&registry,controls,rows,PS_COUNT_OF(rows),"Settings"));
    return 0;
}
