/*
* 86Box    A hypervisor and IBM PC system emulator that specializes in
*          running old operating systems and software designed for IBM
*          PC systems and compatibles from 1981 through fairly recent
*          system designs based on the PCI bus.
*
*          This file is part of the 86Box distribution.
*
*          NV4: Methods for class 0x0055 (Direct3D 6.0 accelerated triangle)
*
*
*
* Authors: Connor Hyde, <mario64crashed@gmail.com> I need a better email address ;^)
*
*          Copyright 2024-2025 Connor Hyde
*/

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <86box/86box.h>
#include <86box/device.h>
#include <86box/mem.h>
#include <86box/pci.h>
#include <86box/rom.h>
#include <86box/video.h>
#include <86box/nv/vid_nv.h>
#include <86box/nv/vid_nv4.h>

void nv4_class_0055_method(uint32_t param, uint32_t method_id, nv4_ramin_context_t context, nv4_grobj_t grobj)
{
    switch (method_id)
    {
        default:
            warning("%s: Invalid or unimplemented method 0x%04x\n", nv4_class_names[context.class_id], method_id);
            nv4_pgraph_interrupt_invalid(NV4_PGRAPH_INTR_1_SOFTWARE_METHOD_PENDING);
            return;
    }
}