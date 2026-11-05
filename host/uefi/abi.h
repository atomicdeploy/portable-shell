/* Minimal x64 UEFI 2.x ABI declarations. No SDK or runtime library required.
 * Layouts follow UEFI specification chapters 4, 7 and 12. */
#ifndef PS_UEFI_ABI_H
#define PS_UEFI_ABI_H
#include <stdint.h>
#include <stddef.h>
#if !defined(__x86_64__)
#error "This adapter currently supports x64 UEFI only"
#endif
#define EFIAPI __attribute__((ms_abi))
typedef uintptr_t efi_status;
typedef uint16_t efi_char;
typedef void *efi_event;
typedef struct {uint64_t signature;uint32_t revision,size,crc,reserved;} efi_header;
typedef struct {uint16_t scan;efi_char unicode;} efi_key;
typedef struct efi_input efi_input;
struct efi_input {
    efi_status (EFIAPI *reset)(efi_input *,uint8_t);
    efi_status (EFIAPI *read)(efi_input *,efi_key *);
    efi_event wait;
};
typedef struct {int32_t max_mode,mode,attribute,column,row;uint8_t visible;} efi_mode;
typedef struct efi_output efi_output;
struct efi_output {
    efi_status (EFIAPI *reset)(efi_output *,uint8_t);
    efi_status (EFIAPI *write)(efi_output *,efi_char *);
    efi_status (EFIAPI *test)(efi_output *,efi_char *);
    efi_status (EFIAPI *query)(efi_output *,uintptr_t,uintptr_t *,uintptr_t *);
    efi_status (EFIAPI *set_mode)(efi_output *,uintptr_t);
    efi_status (EFIAPI *attribute)(efi_output *,uintptr_t);
    efi_status (EFIAPI *clear)(efi_output *);
    efi_status (EFIAPI *cursor)(efi_output *,uintptr_t,uintptr_t);
    efi_status (EFIAPI *enable_cursor)(efi_output *,uint8_t);
    efi_mode *mode;
};
typedef void (EFIAPI *efi_unused)(void);
typedef struct {
    efi_header header;
    efi_unused raise_tpl,restore_tpl,allocate_pages,free_pages,get_memory_map,allocate_pool,free_pool;
    efi_status (EFIAPI *create_event)(uint32_t,uintptr_t,void *,void *,efi_event *);
    efi_status (EFIAPI *set_timer)(efi_event,unsigned,uint64_t);
    efi_status (EFIAPI *wait)(uintptr_t,efi_event *,uintptr_t *);
    efi_unused signal_event;
    efi_status (EFIAPI *close_event)(efi_event);
} efi_boot;
typedef struct {
    efi_header header;efi_char *vendor;uint32_t revision;
    void *input_handle;efi_input *input;void *output_handle;efi_output *output;
    void *error_handle;efi_output *error;void *runtime;efi_boot *boot;
    uintptr_t table_count;void *tables;
} efi_system;
typedef char efi_layout_check[(offsetof(efi_system,output)==64 && offsetof(efi_system,boot)==96 &&
    offsetof(efi_boot,wait)==96 && offsetof(efi_output,mode)==72)?1:-1];
#define EFI_SYSTEM_SIGNATURE UINT64_C(0x5453595320494249)
#define EFI_ERROR_BIT ((efi_status)1 << (sizeof(efi_status)*8-1))
#define EFI_UNSUPPORTED (EFI_ERROR_BIT|3)
#define EFI_NOT_READY (EFI_ERROR_BIT|6)
#define EFI_EVENT_TIMER UINT32_C(0x80000000)
#endif
