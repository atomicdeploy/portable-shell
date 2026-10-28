/* Optional hosted ownership convenience. Compile only where libc is available.
   The core remains caller-owned; firmware can use static aligned storage instead. */
#include "portable_shell.h"
#include <stdlib.h>
ps_session *example_create_session(const ps_config *config) {
    unsigned bytes = ps_sizeof_session();
    void *storage = malloc(bytes);
    ps_session *session;
    if (!storage)
        return 0;
    session = ps_init(storage, bytes, config);
    if (!session)
        free(storage);
    return session;
}
void example_destroy_session(ps_session *session) {
    if (session) {
        ps_close(session);
        free(session);
    }
}
