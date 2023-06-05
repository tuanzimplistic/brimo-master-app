#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "py/persistentcode.h"

void nlr_jump_fail(void *val)
{
    printf("NLR jump failed, val=%p\n", val);
    esp_restart();
}

void *esp_native_code_commit(void *buf, size_t len, void *reloc)
{
    len = (len + 3) & ~3;
    uint32_t *p = heap_caps_malloc(len, MALLOC_CAP_EXEC);
    if (p == NULL)
    {
        m_malloc_fail(len);
    }
    if (reloc)
    {
        mp_native_relocate(reloc, buf, (uintptr_t)p);
    }
    memcpy(p, buf, len);
    return p;
}
