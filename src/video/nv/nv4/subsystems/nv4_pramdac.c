/*
 * 86Box    A hypervisor and IBM PC system emulator that specializes in
 *          running old operating systems and software designed for IBM
 *          PC systems and compatibles from 1981 through fairly recent
 *          system designs based on the PCI bus.
 *
 *          This file is part of the 86Box distribution.
 *
 *          nv4 bringup and device emulation.
 *
 *
 *
 * Authors: Connor Hyde, <mario64crashed@gmail.com> I need a better email address ;^)
 *
 *          Copyright 2024-2025 starfrost
 */

// nv4_pramdac.c: nv4 RAMDAC
// Todo: Allow overridability using 68050C register...

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <86box/86box.h>
#include <86box/device.h>
#include <86box/mem.h>
#include <86box/pci.h>
#include <86box/rom.h> // DEPENDENT!!!
#include <86box/video.h>
#include <86box/nv/vid_nv.h>
#include <86box/nv/vid_nv4.h>

void nv4_pramdac_init(void)
{
    nv_log("Initialising PRAMDAC\n");

    // defaults, these come from vbios in reality
    // driver defaults are nonsensical(?), or the algorithm is wrong
    // munged this to 100mhz for now
    nv4->pramdac.memory_clock_m = nv4->pramdac.pixel_clock_m = 0x07;
    nv4->pramdac.memory_clock_n = nv4->pramdac.pixel_clock_n = 0xc8;
    nv4->pramdac.memory_clock_p = nv4->pramdac.pixel_clock_p = 0x0c;

    nv4_pramdac_set_pixel_clock();
    nv4_pramdac_set_vram_clock();

    nv_log("Initialising PRAMDAC: Done\n");
}

// Polls the pixel clock.
void nv4_pramdac_pixel_clock_poll(double real_time)
{
    /* Ignore in VGA mode */
    if (!nv4->nvbase.svga.override)
        return; 

    /* Figure out our refresh time. */
    if (!nv4->nvbase.refresh_time)
        nv4->nvbase.refresh_time = (1/60.0); // rivatimers count in microseconds but present the info as seconds
        
    nv4->nvbase.refresh_clock += real_time;

    if (nv4->nvbase.refresh_clock > nv4->nvbase.refresh_time)
    {
        /* Update the screen because something changed */
        video_blit_memtoscreen(0, 0, xsize, ysize);
        nv4->nvbase.refresh_clock = 0;
    }

    // TODO: ????
}

// Polls the memory clock.
// This updates the 2D/3D engine PGRAPH, PTIMER and more 
void nv4_pramdac_memory_clock_poll(double real_time)
{
    nv4_ptimer_tick(real_time);

    nv4_pfifo_cache0_pull();
    nv4_pfifo_cache1_pull();
    // TODO: UPDATE PGRAPH!
}

// Gets the vram clock register.
uint32_t nv4_pramdac_get_vram_clock_register(void)
{
    // the clock format is packed into 19 bits
    // M divisor            [7-0]
    // N divisor            [16-8]
    // P divisor            [18-16]
    return (nv4->pramdac.memory_clock_m)
    + (nv4->pramdac.memory_clock_n << 8)
    + (nv4->pramdac.memory_clock_p << 16); // 0-3
}

uint32_t nv4_pramdac_get_pixel_clock_register(void)
{
    return (nv4->pramdac.pixel_clock_m)
    + (nv4->pramdac.pixel_clock_n << 8)
    + (nv4->pramdac.pixel_clock_p << 16); // 0-3
}

void nv4_pramdac_set_vram_clock_register(uint32_t value)
{
    nv4->pramdac.memory_clock_m = value & 0xFF;
    nv4->pramdac.memory_clock_n = (value >> 8) & 0xFF;
    nv4->pramdac.memory_clock_p = (value >> 16) & 0x07;
    
    nv4_pramdac_set_vram_clock();
}

void nv4_pramdac_set_pixel_clock_register(uint32_t value)
{
    nv4->pramdac.pixel_clock_m = value & 0xFF;
    nv4->pramdac.pixel_clock_n = (value >> 8) & 0xFF;
    nv4->pramdac.pixel_clock_p = (value >> 16) & 0x07;

    nv4_pramdac_set_pixel_clock();
}

void nv4_pramdac_set_vram_clock(void)
{
    // from driver and vbios source
    float frequency = 13500000.0f;

    // prevent division by 0
    if (nv4->pramdac.memory_clock_m == 0)
        nv4->pramdac.memory_clock_m = 1;
    
    if (nv4->pramdac.memory_clock_n == 0)
        nv4->pramdac.memory_clock_n = 1;
    
    // Convert to microseconds
    frequency = (frequency * nv4->pramdac.memory_clock_n) / (nv4->pramdac.memory_clock_m << nv4->pramdac.memory_clock_p); 

    double time = (1000000.0 * NV4_86BOX_TIMER_SYSTEM_FIX_QUOTIENT) / (double)frequency; // needs to be a double for 86box

    nv_log("Memory clock = %.2f MHz\n", frequency / 1000000.0f);    

    nv4->nvbase.memory_clock_frequency = frequency;
    
    // Create and start if it it's not running.
    if (!nv4->nvbase.memory_clock_timer)
    {
        nv4->nvbase.memory_clock_timer = rivatimer_create(time, nv4_pramdac_memory_clock_poll);
        rivatimer_start(nv4->nvbase.memory_clock_timer);
    }

    rivatimer_set_period(nv4->nvbase.memory_clock_timer, time);
}

void nv4_pramdac_set_pixel_clock(void)
{
    // frequency divider algorithm from old varcem/86box/pcbox riva driver,
    // verified by reversing NT drivers v1.50e CalcMNP [symbols] function

    // todo: actually implement it

    // missing section
    // not really needed.
    // if (nv4->pfb.boot.clock_crystal == CLOCK_CRYSTAL_13500)
    // {
    //      freq = 13500000.0f;
    // }
    // else 
    //
    // {
    //      freq = 14318000.0f;
    // }

    float frequency = 13500000.0f;

    // prevent division by 0
    if (nv4->pramdac.pixel_clock_m == 0)
        nv4->pramdac.pixel_clock_m = 1;
    
    if (nv4->pramdac.memory_clock_n == 0)
        nv4->pramdac.memory_clock_n = 1;

    frequency = (frequency * nv4->pramdac.pixel_clock_n) / (nv4->pramdac.pixel_clock_m << nv4->pramdac.pixel_clock_p); 

    nv4->nvbase.svga.clock = cpuclock / frequency;

    double time = (1000000.0 * NV4_86BOX_TIMER_SYSTEM_FIX_QUOTIENT) / (double)frequency; // needs to be a double for 86box

    nv_log("Pixel clock = %.2f MHz\n", frequency / 1000000.0f);

    nv4->nvbase.pixel_clock_frequency = frequency;

    // Create and start if it it's not running.
    if (!nv4->nvbase.pixel_clock_timer)
    {
        nv4->nvbase.pixel_clock_timer = rivatimer_create(time, nv4_pramdac_pixel_clock_poll);
        rivatimer_start(nv4->nvbase.pixel_clock_timer);
    }

    rivatimer_set_period(nv4->nvbase.pixel_clock_timer, time);
}

//
// ****** PRAMDAC register list START ******
//

// NULL means handle in read functions
nv_register_t pramdac_registers[] = 
{
    { NV4_PRAMDAC_CLOCK_PIXEL, "PRAMDAC - nv4 GPU Core - Pixel clock", nv4_pramdac_get_pixel_clock_register, nv4_pramdac_set_pixel_clock_register },
    { NV4_PRAMDAC_CLOCK_MEMORY, "PRAMDAC - nv4 GPU Core - Memory clock", nv4_pramdac_get_vram_clock_register, nv4_pramdac_set_vram_clock_register },
    { NV4_PRAMDAC_COEFF_SELECT, "PRAMDAC - PLL Clock Coefficient Select", NULL, NULL},
    { NV4_PRAMDAC_GENERAL_CONTROL, "PRAMDAC - General Control", NULL, NULL },
    { NV4_PRAMDAC_VSERR_WIDTH, "PRAMDAC - Vertical Sync Error Width", NULL, NULL},
    { NV4_PRAMDAC_VEQU_END, "PRAMDAC - VEqu End", NULL, NULL},
    { NV4_PRAMDAC_VBBLANK_START, "PRAMDAC - VBBlank Start", NULL, NULL},
    { NV4_PRAMDAC_VBBLANK_END, "PRAMDAC - VBBlank End", NULL, NULL},
    { NV4_PRAMDAC_HBLANK_END, "PRAMDAC - Horizontal Blanking Interval End", NULL, NULL},
    { NV4_PRAMDAC_HBLANK_START, "PRAMDAC - Horizontal Blanking Interval Start", NULL, NULL},
    { NV4_PRAMDAC_VBLANK_END, "PRAMDAC - Vertical Blanking Interval End", NULL, NULL},
    { NV4_PRAMDAC_VBLANK_START, "PRAMDAC - Vertical Blanking Interval Start", NULL, NULL},
    { NV4_PRAMDAC_VEQU_START, "PRAMDAC - VEqu Start", NULL, NULL},
    { NV4_PRAMDAC_VTOTAL, "PRAMDAC - Total Vertical Lines", NULL, NULL},
    { NV4_PRAMDAC_HSYNC_WIDTH, "PRAMDAC - Horizontal Sync Pulse Width", NULL, NULL},
    { NV4_PRAMDAC_HBURST_START, "PRAMDAC - Horizontal Burst Signal Start", NULL, NULL},
    { NV4_PRAMDAC_HBURST_END, "PRAMDAC - Horizontal Burst Signal Start", NULL, NULL},
    { NV4_PRAMDAC_HTOTAL, "PRAMDAC - Total Horizontal Lines", NULL, NULL},
    { NV4_PRAMDAC_HEQU_WIDTH, "PRAMDAC - HEqu End", NULL, NULL},
    { NV4_PRAMDAC_HSERR_WIDTH, "PRAMDAC - Horizontal Sync Error", NULL, NULL},
    { NV_REG_LIST_END, NULL, NULL, NULL}, // sentinel value 
};

//
// ****** Read/Write functions start ******
//

uint32_t nv4_pramdac_read(uint32_t address) 
{ 
    nv_register_t* reg = nv_get_register(address, pramdac_registers, sizeof(pramdac_registers)/sizeof(pramdac_registers[0]));

    uint32_t ret = 0x00;

    // todo: friendly logging

    nv_log_verbose_only("PRAMDAC Read from 0x%08x\n", address);

    // if the register actually exists
    if (reg)
    {
        // on-read function
        if (reg->on_read)
            ret = reg->on_read();
        else
        {
            //s hould be pretty easy to understand
            switch (reg->address)
            {
                case NV4_PRAMDAC_COEFF_SELECT:
                    ret = nv4->pramdac.coeff_select;
                    break;
                case NV4_PRAMDAC_GENERAL_CONTROL:
                    ret = nv4->pramdac.general_control;
                    break;
                case NV4_PRAMDAC_VSERR_WIDTH:
                    ret = nv4->pramdac.vserr_width;
                    break;
                case NV4_PRAMDAC_VBBLANK_END:
                    ret = nv4->pramdac.vbblank_end;
                    break;
                case NV4_PRAMDAC_VBLANK_END:
                    ret = nv4->pramdac.vblank_end;
                    break;
                case NV4_PRAMDAC_VBLANK_START:
                    ret = nv4->pramdac.vblank_start;
                    break;
                case NV4_PRAMDAC_VEQU_START:
                    ret = nv4->pramdac.vequ_start;
                    break;
                case NV4_PRAMDAC_VTOTAL:
                    ret = nv4->pramdac.vtotal;
                    break;
                case NV4_PRAMDAC_HSYNC_WIDTH:
                    ret = nv4->pramdac.hsync_width;
                    break;
                case NV4_PRAMDAC_HBURST_START:
                    ret = nv4->pramdac.hburst_start;
                    break;
                case NV4_PRAMDAC_HBURST_END:
                    ret = nv4->pramdac.hburst_end;
                    break;
                case NV4_PRAMDAC_HBLANK_START:
                    ret = nv4->pramdac.hblank_start;
                    break;
                case NV4_PRAMDAC_HBLANK_END:
                    ret =  nv4->pramdac.hblank_end;
                    break;
                case NV4_PRAMDAC_HTOTAL:
                    ret = nv4->pramdac.htotal;
                    break;
                case NV4_PRAMDAC_HEQU_WIDTH:
                    ret = nv4->pramdac.hequ_width;
                    break;
                case NV4_PRAMDAC_HSERR_WIDTH:
                    ret = nv4->pramdac.hserr_width;
                    break;
            }
        }

        if (reg->friendly_name)
            nv_log_verbose_only(": 0x%08x <- %s\n", ret, reg->friendly_name);
        else   
            nv_log_verbose_only("\n");
    }
    else
    {
        nv_log(": Unknown register read (address=0x%08x), returning 0x00\n", address);
    }

    return ret; 
}

void nv4_pramdac_write(uint32_t address, uint32_t value) 
{
    nv_register_t* reg = nv_get_register(address, pramdac_registers, sizeof(pramdac_registers)/sizeof(pramdac_registers[0]));

    nv_log_verbose_only("PRAMDAC Write 0x%08x -> 0x%08x\n", value, address);

    // if the register actually exists
    if (reg)
    {


        // on-read function
        if (reg->on_write)
            reg->on_write(value);
        else
        {
            //s hould be pretty easy to understand
            // we also update the SVGA state here
            switch (reg->address)
            {
                case NV4_PRAMDAC_COEFF_SELECT:
                    nv4->pramdac.coeff_select = value;
                    break;
                case NV4_PRAMDAC_GENERAL_CONTROL:
                    nv4->pramdac.general_control = value;
                    nv4_recalc_timings(&nv4->nvbase.svga);
                    break;
                case NV4_PRAMDAC_VSERR_WIDTH:
                    //vslines?
                    nv4->pramdac.vserr_width = value;
                    break;
                case NV4_PRAMDAC_VBBLANK_END:
                    nv4->pramdac.vbblank_end = value;
                    break;
                case NV4_PRAMDAC_VBLANK_END:
                    nv4->pramdac.vblank_end = value;
                    break;
                case NV4_PRAMDAC_VBLANK_START:
                    //nv4->nvbase.svga.vblankstart = value;
                    nv4->pramdac.vblank_start = value;
                    break;
                case NV4_PRAMDAC_VEQU_START:
                    nv4->pramdac.vequ_start = value;
                    break;
                case NV4_PRAMDAC_VTOTAL:
                    //nv4->pramdac.vtotal = value;
                    nv4->nvbase.svga.vtotal = value;
                    break;
                case NV4_PRAMDAC_HSYNC_WIDTH:
                    nv4->pramdac.hsync_width = value;
                    break;
                case NV4_PRAMDAC_HBURST_START:
                    nv4->pramdac.hburst_start = value;
                    break;
                case NV4_PRAMDAC_HBURST_END:
                    nv4->pramdac.hburst_end = value;
                    break;
                case NV4_PRAMDAC_HBLANK_START:
                    //nv4->nvbase.svga.hblankstart = value;
                    nv4->pramdac.hblank_start = value;
                    break;
                case NV4_PRAMDAC_HBLANK_END:
                    //nv4->nvbase.svga.hblank_end_val = value;
                    nv4->pramdac.hblank_end = value;
                    break;
                case NV4_PRAMDAC_HTOTAL:
                    nv4->pramdac.htotal = value;
                    //nv4->nvbase.svga.htotal = value;
                    break;
                case NV4_PRAMDAC_HEQU_WIDTH:
                    nv4->pramdac.hequ_width = value;
                    break;
                case NV4_PRAMDAC_HSERR_WIDTH:
                
                    nv4->pramdac.hserr_width = value;
                    break;
            }
        }

        if (reg->friendly_name)
            nv_log_verbose_only(": %s\n", reg->friendly_name);
        else   
            nv_log_verbose_only("\n");
    }
    else /* Completely unknown */
    {
        nv_log(": Unknown register write (address=0x%08x)\n", address);
    }
}