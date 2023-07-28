/* HP Scanjet 3900 series - RTS8822 Core

   Copyright (C) 2005-2013 Jonathan Bravo Lopez <jkdsoft@gmail.com>

   This file is part of the SANE package.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

   As a special exception, the authors of SANE give permission for
   additional uses of the libraries contained in this release of SANE.

   The exception is that, if you link a SANE library with other files
   to produce an executable, this does not by itself cause the
   resulting executable to be covered by the GNU General Public
   License.  Your use of that executable is in no way restricted on
   account of linking the SANE library code into it.

   This exception does not, however, invalidate any other reasons why
   the executable file might be covered by the GNU General Public
   License.

   If you submit changes to SANE to the maintainers to be included in
   a subsequent release, you agree by submitting the changes that
   those changes may be distributed with this exception intact.

   If you write modifications of your own for SANE, it is your choice
   whether to permit this exception to apply to your modifications.
   If you do not wish that, delete this exception notice.
*/


/*
 This code is still a bit ugly due to it's the result of applying
 reverse engineering techniques to windows driver. So at this
 moment what you see is exactly what windows driver does.
 And so many global vars exist that will be erased when driver
 is entirely debugged. There are some variables with unknown
 purpose. So they have no meaning name in form v+address. I
 hope to change their names when driver is debugged completely.
*/

#ifndef RTS8822_CORE

#define RTS8822_CORE

#define GetTickCount() (time(0) * 1000)
#define min(A,B) (((A)<(B)) ? (A) : (B))
#define max(A,B) (((A)>(B)) ? (A) : (B))
#define PIXEL_TO_MM(_pixel_, _dpi_) ((_pixel_) * 25.4 / (_dpi_))
#define MM_TO_PIXEL(_mm_, _dpi_) ((_mm_) * (_dpi_) / 25.4)

#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* memset()          */
#include <time.h>   /* clock()           */
#include <math.h>   /* floor, ceil, pow  */
#include <ctype.h>  /* tolower()         */
#include <unistd.h> /* usleep()          */

#include "hp3900_types.c"
#include "hp3900_debug.c"
#include "hp3900_config.c"
#include "hp3900_usb.c"

/*-------------------- Exported function headers --------------------*/

static void wfree(void *ptr);

/* functions to allocate and free space for a device */
static st_device  *RTS_alloc(void);
static void        RTS_free(st_device *);

/* Scanner level commands */
static SANE_Status RTS_scanner_init (st_device *);
static SANE_Status RTS_scanner_setup(st_device *, struct params *);
static SANE_Status RTS_scanner_start(st_device *);
static SANE_Status RTS_scanner_read (st_device *, SANE_Int buffer_size, SANE_Byte *buffer, SANE_Int *transferred);
static void        RTS_scanner_stop (st_device *, SANE_Int wait);

/* device information retrieving functions */
static SANE_Int  RTS_device_count      (void);
static SANE_Int  RTS_device_supported  (SANE_Int pos);
static SANE_Int  RTS_device_get        (SANE_Int product, SANE_Int vendor);
static char     *RTS_device_vendor     (SANE_Int device);
static char     *RTS_device_product    (SANE_Int device);
static SANE_Int  RTS_device_usb_vendor (SANE_Int device);
static SANE_Int  RTS_device_usb_product(SANE_Int device);
static char     *RTS_device_name       (SANE_Int device);
static SANE_Int  RTS_device_set        (st_device *, SANE_Int proposed, SANE_Int product, SANE_Int vendor);

/* Chipset related commands */
static SANE_Int    RTS_chip_id          (st_device *);
static SANE_Status RTS_chip_name        (st_device *, char *name, SANE_Int size);
static SANE_Status RTS_chip_reset       (st_device *);

/* loading configuration functions */
static SANE_Status RTS_load_buttons      (st_device *);
static SANE_Status RTS_load_chip         (st_device *);
static SANE_Status RTS_load_config       (st_device *);
static SANE_Status RTS_load_constrains   (st_device *);
static SANE_Status RTS_load_motor        (st_device *);
static SANE_Status RTS_load_motor_curves (st_device *);
static SANE_Status RTS_load_RTS_mtr_moves(st_device *);
static SANE_Status RTS_load_scanmodes    (st_device *);
static SANE_Status RTS_load_sensor       (st_device *);
static SANE_Status RTS_load_timings      (st_device *);

/* freeing configuration functions */
static void      RTS_free_buttons      (st_device *);
static void      RTS_free_chip         (st_device *);
static void      RTS_free_config       (st_device *);
static void      RTS_free_constrains   (st_device *);
static void      RTS_free_motor        (st_device *);
static void      RTS_free_motor_curves (st_device *);
static void      RTS_free_RTS_mtr_moves(st_device *);
static void      RTS_free_scanmodes    (st_device *);
static void      RTS_free_sensor       (st_device *);
static void      RTS_free_timings      (st_device *);

/* Functions to manage data */
static SANE_Byte data_bitget        (SANE_Byte *address, SANE_Int mask);
static void      data_bitset        (SANE_Byte *address, SANE_Int mask, SANE_Byte data);
static SANE_Int  data_lsb_get       (SANE_Byte *address, SANE_Int size);
static void      data_lsb_set       (SANE_Byte *address, SANE_Int data, SANE_Int size);
static void      data_lsb_cpy       (SANE_Byte *source, SANE_Byte *dest, SANE_Int size);
static void      data_lsb_inc       (SANE_Byte *address, SANE_Int data, SANE_Int size);
static void      data_msb_set       (SANE_Byte *address, SANE_Int data, SANE_Int size);
static SANE_Int  data_wide_bitget   (SANE_Byte *address, SANE_Int mask);
static void      data_wide_bitset   (SANE_Byte *address, SANE_Int mask, SANE_Int data);
static SANE_Int  data_swap_endianess(SANE_Int address, SANE_Int size);

/* scanmode functions */
static SANE_Int  RTS_scanmode_get   (st_device *, SANE_Int scantype, SANE_Int colormode, SANE_Int resolution);
static SANE_Int  RTS_scanmode_fitres(st_device *, SANE_Int scantype, SANE_Int colormode, SANE_Int resolution);
static SANE_Int  RTS_scanmode_maxres(st_device *, SANE_Int scantype, SANE_Int colormode);
static SANE_Int  RTS_scanmode_minres(st_device *, SANE_Int scantype, SANE_Int colormode);

/* Chipset management useful commands*/
static SANE_Int  RTS_isTmaAttached(st_device *);

/* sensor related functions */
static SANE_Status RTS_sensor_clock_set(SANE_Byte *Regs, struct st_cph *);
static SANE_Status RTS_sensor_enable   (st_device *, SANE_Byte *Regs, SANE_Int channels);
static SANE_Byte   RTS_sensor_type     (st_device *);

/* DMA management commands */
static SANE_Status RTS_dma_cancel      (st_device *);
static SANE_Status RTS_dma_type_check  (st_device *, SANE_Byte *Regs);
static SANE_Status RTS_dma_read_enable (st_device *, SANE_Int dmacs, SANE_Int size, SANE_Int segment);
static SANE_Status RTS_dma_write_enable(st_device *, SANE_Int dmacs, SANE_Int size, SANE_Int segment);
static SANE_Status RTS_dma_read        (st_device *, SANE_Int dmacs, SANE_Int segment, SANE_Int size, SANE_Byte *buffer);
static SANE_Status RTS_dma_reset       (st_device *);
static SANE_Status RTS_dma_type_set    (st_device *, SANE_Byte *Regs, SANE_Byte ramtype);
static SANE_Status RTS_dma_wait        (st_device *, SANE_Int msecs);
static SANE_Status RTS_dma_write       (st_device *, SANE_Int dmacs, SANE_Int segment, SANE_Int size, SANE_Byte *buffer);

/* NVRAM management commands */
static SANE_Status RTS_nvram_read_byte    (st_device *, SANE_Int address, SANE_Byte *data);
static SANE_Status RTS_nvram_read_integer (st_device *, SANE_Int address, SANE_Int *data);
static SANE_Status RTS_nvram_read_word    (st_device *, SANE_Int address, SANE_Int *data);
static SANE_Status RTS_nvram_write_buffer (st_device *, SANE_Int address, SANE_Byte *data, SANE_Int size);
static SANE_Status RTS_nvram_write_byte   (st_device *, SANE_Int address, SANE_Byte data);
static SANE_Status RTS_nvram_write_integer(st_device *, SANE_Int address, SANE_Int data);
static SANE_Status RTS_nvram_write_word   (st_device *, SANE_Int address, SANE_Int data);

/* main scanning process functions */
static SANE_Status RTS_scan_start    (st_device *);
static void        RTS_scan_count_inc(st_device *);
static SANE_Int    RTS_scan_count_get(st_device *);
static SANE_Status RTS_scan_run      (st_device *);
static SANE_Byte   RTS_scan_running  (st_device *, SANE_Byte *Regs);
static SANE_Status RTS_scan_wait     (st_device *, SANE_Int msecs);

static SANE_Status RTS_simple_scan(st_device *, SANE_Byte *Regs, struct st_scanparams *, struct st_gain_offset *, SANE_Byte *buffer, struct st_calibration *, SANE_Int options, SANE_Int gainmode);
static SANE_Status RTS_simple_scan_read(st_device *, SANE_Byte *buffer, struct st_scanparams *, struct st_hwdconfig *);

/* functions to wait for a process tp finish */
static SANE_Status RTS_WaitInitEnd(st_device *, SANE_Int msecs);

/* functions to read/write control registers */
static SANE_Byte  *RTS_regs_alloc(SANE_Int clearbuffer);
static SANE_Status RTS_regs_cpy  (SANE_Byte *dest, SANE_Byte *source);
static SANE_Byte  *RTS_regs_dup  (SANE_Byte *source);
static SANE_Status RTS_regs_init (st_device *);
static SANE_Status RTS_regs_read (st_device *, SANE_Byte *buffer);
static SANE_Status RTS_regs_write(st_device *, SANE_Byte *buffer);

/* functions to manage scancfg buffers */
static struct st_scanparams *RTS_scancfg_alloc(SANE_Int clearbuffer);
static SANE_Status           RTS_scancfg_cpy  (struct st_scanparams *, struct st_scanparams *);
static struct st_scanparams *RTS_scancfg_dup  (struct st_scanparams *);

/* functions to manage hwdcfg buffers */
static struct st_hwdconfig *RTS_hwdcfg_alloc(SANE_Int clearbuffer);

/* functions to setup control registers */
static SANE_Status RTS_setup               (st_device *, SANE_Byte *Regs, struct st_scanparams *, struct st_hwdconfig *, struct st_gain_offset *);
static void        RTS_setup_arrangeline   (st_device *, struct st_hwdconfig *, SANE_Int colormode);
static void        RTS_setup_channels      (st_device *, SANE_Byte *Regs, struct st_scanparams *, SANE_Int mycolormode);
static void        RTS_setup_coords        (SANE_Byte *Regs, SANE_Int iLeft, SANE_Int iTop, SANE_Int width, SANE_Int height);
static SANE_Int    RTS_setup_depth         (SANE_Byte *Regs, struct st_scanparams *, SANE_Int mycolormode);
static void        RTS_setup_exposure      (st_device *, SANE_Byte *Regs, struct st_scanparams *, struct st_scanmode *);
static void        RTS_setup_gnoff         (st_device *, SANE_Byte *Regs, struct st_gain_offset *);
static void        RTS_setup_gamma         (SANE_Byte *Regs, struct st_hwdconfig *);
static SANE_Int    RTS_setup_line_distances(st_device *, SANE_Byte *Regs, struct st_scanparams *, struct st_hwdconfig *, SANE_Int mycolormode, SANE_Int arrangeline);
static SANE_Status RTS_setup_motor         (st_device *, SANE_Byte *Regs, struct st_scanparams *, SANE_Int somevalue);
static void        RTS_setup_refvoltages   (st_device *, SANE_Byte *Regs);
static void        RTS_setup_sensor_clocks (st_device *, SANE_Int mytiming, SANE_Byte *Regs);
static void        RTS_setup_shading       (SANE_Byte *Regs, struct st_scanparams *, struct st_hwdconfig *, SANE_Int bytes_per_line);


static void SetLock(st_device *, SANE_Byte *Regs, SANE_Byte Enable);

static SANE_Status RTS_read_bff_alloc   (st_device *);
static void        RTS_read_bff_free    (st_device *);
static SANE_Int    RTS_read_bff_size_get(st_device *, SANE_Byte channels_per_dot, SANE_Int channel_size);
static SANE_Status RTS_read_bff_size_set(st_device *, SANE_Int data, SANE_Int size);
static SANE_Status RTS_read_bff_wait    (st_device *, SANE_Byte Channels_per_dot, SANE_Byte Channel_size, SANE_Int size, SANE_Int *last_amount, SANE_Int seconds, SANE_Byte op);

static SANE_Status Read_ResizeBlock(st_device *, SANE_Byte *buffer, SANE_Int buffer_size, SANE_Int *transferred);
static SANE_Status Read_Block(st_device *, SANE_Int buffer_size, SANE_Byte *buffer, SANE_Int *transferred);
static SANE_Status Read_NonColor_Block(st_device *, SANE_Byte *buffer, SANE_Int buffer_size, SANE_Byte ColorMode, SANE_Int *transferred);

/* reference functions */
static SANE_Status RTS_refs_analyze     (st_device *, struct st_scanparams *, SANE_Byte *scanned_pattern, SANE_Int *ler1, SANE_Int ler1order, SANE_Int *ser1, SANE_Int ser1order);
static SANE_Status RTS_refs_counter_inc (st_device *);
static SANE_Byte   RTS_refs_counter_load(st_device *);
static SANE_Status RTS_refs_counter_save(st_device *, SANE_Byte data);
static SANE_Status RTS_refs_detect      (st_device *, SANE_Byte *Regs, SANE_Int resolution_x, SANE_Int resolution_y, SANE_Int *x, SANE_Int *y);
static SANE_Status RTS_refs_load        (st_device *, SANE_Int *x, SANE_Int *y);
static SANE_Status RTS_refs_save        (st_device *, SANE_Int x, SANE_Int y);
static SANE_Status RTS_refs_set         (st_device *, SANE_Byte *Regs, struct st_scanparams *);

/* Coordinates' constrains functions */
static SANE_Status       RTS_area_check(st_device *, SANE_Int Resolution, SANE_Int scantype, struct st_coords *);
static struct st_coords *RTS_area_get  (st_device *, SANE_Byte scantype);

/* Operations in eeprom about gain and offset values */
static SANE_Status RTS_gnoff_clear       (st_device *);
static SANE_Status RTS_gnoff_get         (st_device *);
static SANE_Status RTS_gnoff_save        (st_device *, SANE_Int *offset, SANE_Byte *gain);
static SANE_Status RTS_gnoff_counter_inc (st_device *, SANE_Int *arg1);
static SANE_Byte   RTS_gnoff_counter_load(st_device *);
static SANE_Status RTS_gnoff_counter_save(st_device *, SANE_Byte data);

/* Gamma functions*/
static SANE_Status Gamma_GetTables  (st_device *, SANE_Byte *Gamma_buffer);

static void        RTS_gamma_free      (st_device *);
static void        RTS_gamma_alloc     (st_device *, SANE_Int colour, SANE_Int depth, SANE_Byte *table);
static SANE_Int    RTS_gamma_depth_get (st_device *);
static SANE_Status RTS_gamma_depth_set (st_device *, SANE_Int depth, SANE_Int conv);
static SANE_Byte  *RTS_gamma_depth_conv(SANE_Byte *table, SANE_Int src_depth, SANE_Int dst_depth);
static SANE_Status RTS_gamma_apply     (st_device *, SANE_Byte *Regs, struct st_scanparams *scancfg, struct st_hwdconfig *);
static SANE_Status RTS_gamma_write     (st_device *, SANE_Byte *Regs, SANE_Byte *gammatable, SANE_Int size);
static SANE_Int    RTS_gamma_get       (st_device *, SANE_Int colour, SANE_Int item);

/* Lamp functions */
static SANE_Byte   RTS_lamp_gaincontrol_get (st_device *, SANE_Int resolution, SANE_Byte scantype);
static void        RTS_lamp_gaincontrol_set (st_device *, SANE_Byte *Regs, SANE_Int resolution, SANE_Byte gainmode);
static SANE_Status RTS_lamp_pwm_duty_get    (st_device *, SANE_Int *data);
static SANE_Status RTS_lamp_pwm_duty_set    (st_device *, SANE_Int duty_cycle);
static SANE_Status RTS_lamp_pwm_setup       (st_device *, SANE_Int lamp);
static SANE_Status RTS_lamp_pwm_use         (st_device *, SANE_Int enable);
static SANE_Status RTS_lamp_pwm_checkstable (st_device *, SANE_Int scantype, SANE_Int resolution, SANE_Int lamp);
static SANE_Status RTS_lamp_pwm_save        (st_device *, SANE_Int fixedpwm);
static SANE_Status RTS_lamp_pwm_status_save (st_device *, SANE_Byte status);
static SANE_Status RTS_lamp_status_get      (st_device *, SANE_Byte *flb_lamp, SANE_Byte *tma_lamp);
static SANE_Status RTS_lamp_status_set      (st_device *, SANE_Byte *Regs, SANE_Int turn_on, SANE_Int lamp);
static SANE_Status RTS_lamp_status_timer_set(st_device *, SANE_Int minutes);
static SANE_Status RTS_lamp_warmup          (st_device *, SANE_Byte *Regs, SANE_Int lamp, SANE_Int scantype, SANE_Int resolution);
static SANE_Status RTS_lamp_warmup_reset    (st_device *);

/* Head related functions */
static SANE_Int    RTS_head_athome  (st_device *, SANE_Byte *Regs);
static SANE_Status RTS_head_park    (st_device *, SANE_Int bWait, SANE_Int movement);
static SANE_Status RTS_head_relocate(st_device *, SANE_Int speed, SANE_Int direction, SANE_Int ypos);

/* motor functions */
static SANE_Byte             *RTS_mtr_addstep    (SANE_Byte *steps, SANE_Int *bwriten, SANE_Int step);
static SANE_Status            RTS_mtr_change     (st_device *, SANE_Byte *Regs, SANE_Byte value);
static SANE_Int               RTS_mtr_get        (st_device *, SANE_Int resolution);
static SANE_Status            RTS_mtr_move       (st_device *, SANE_Byte *Regs, struct st_motormove *, struct st_motorpos *);
static void                   RTS_mtr_release    (st_device *);
static SANE_Int               RTS_mtr_setup_steps(st_device *, SANE_Byte *Regs, SANE_Int mysetting);
static SANE_Int               RTS_mtr_curve_equal(st_device *, SANE_Int motorsetting, SANE_Int direction, SANE_Int curve1, SANE_Int curve2);
static void                   RTS_mtr_curve_free (struct st_motorcurve **motorcurves, SANE_Int *mtc_count);
static struct st_curve       *RTS_mtr_curve_get  (st_device *, SANE_Int motorcurve, SANE_Int direction, SANE_Int itype);
static struct st_motorcurve **RTS_mtr_curve_parse(SANE_Int *mtc_count, SANE_Int *buffer);

/* Functions to arrange scanned lines */
static SANE_Status Arrange_Colour   (st_device *, SANE_Byte *buffer, SANE_Int buffer_size, SANE_Int *transferred);
static SANE_Status Arrange_Compose  (st_device *, SANE_Byte *buffer, SANE_Int buffer_size, SANE_Int *transferred);
static SANE_Status Arrange_NonColour(st_device *, SANE_Byte *buffer, SANE_Int buffer_size, SANE_Int *transferred);

/* Composing RGB triplet functions */
static void Triplet_Gray         (SANE_Byte *pPointer1, SANE_Byte *pPointer2, SANE_Byte *buffer, SANE_Int channels_count);
static void Triplet_Lineart      (SANE_Byte *pPointer1, SANE_Byte *pPointer2, SANE_Byte *buffer, SANE_Int channels_count);
static void Triplet_Compose_Order(st_device *, SANE_Byte *pRed, SANE_Byte *pGreen, SANE_Byte *pBlue, SANE_Byte *buffer, SANE_Int dots);
static void Triplet_Compose_HRes (st_device *, SANE_Byte *pPointer1, SANE_Byte *pPointer2, SANE_Byte *pPointer3, SANE_Byte *pPointer4, SANE_Byte *pPointer5, SANE_Byte *pPointer6, SANE_Byte *buffer, SANE_Int Width);
static void Triplet_Compose_LRes (st_device *, SANE_Byte *pRed, SANE_Byte *pGreen, SANE_Byte *pBlue, SANE_Byte *buffer, SANE_Int dots);
static void Triplet_Colour_Order (st_device *dev, SANE_Byte *pRed, SANE_Byte *pGreen, SANE_Byte *pBlue, SANE_Byte *buffer, SANE_Int Width);
static void Triplet_Colour_HRes  (SANE_Byte *pRed1, SANE_Byte *pGreen1, SANE_Byte *pBlue1, SANE_Byte *pRed2, SANE_Byte *pGreen2, SANE_Byte *pBlue2, SANE_Byte *buffer, SANE_Int Width);
static void Triplet_Colour_LRes  (SANE_Int Width, SANE_Byte *Buffer, SANE_Byte *pChannel1, SANE_Byte *pChannel2, SANE_Byte *pChannel3);

/* Functions used to resize retrieved image */
static SANE_Status RTS_rsz_start   (st_device *, SANE_Int *transferred);
static SANE_Status RTS_rsz_alloc   (st_device *, SANE_Int size1, SANE_Int size2, SANE_Int size3);
static void        RTS_rsz_free    (st_device *);
static SANE_Status RTS_rsz_increase(SANE_Byte *to_buffer, SANE_Int to_resolution, SANE_Int to_width, SANE_Byte *from_buffer, SANE_Int from_resolution, SANE_Int from_width, SANE_Int myresize_mode);
static SANE_Status RTS_rsz_decrease(SANE_Byte *to_buffer, SANE_Int to_resolution, SANE_Int to_width, SANE_Byte *from_buffer, SANE_Int from_resolution, SANE_Int from_width, SANE_Int myresize_mode);

/* Scanner buttons support */
static SANE_Int    RTS_btn_count   (st_device *);
static SANE_Status RTS_btn_enable  (st_device *);
static SANE_Int    RTS_btn_name    (st_device *, SANE_Int mask);
static SANE_Int    RTS_btn_order   (st_device *, SANE_Int mask);
static SANE_Int    RTS_btn_status  (st_device *);
static SANE_Int    RTS_btn_released(st_device *);

/* Calibration functions */
static void        Calib_LoadCut(st_device *, struct st_scanparams *, SANE_Int scantype, struct st_calibration_config *);
static SANE_Status Calib_BWShading(struct st_calibration_config *, struct st_calibration *, SANE_Int gainmode);
static SANE_Status Calib_LoadConfig(st_device *, struct st_calibration_config *, SANE_Int scantype, SANE_Int resolution, SANE_Int bitmode);
static SANE_Status Calibration(st_device *, SANE_Byte *Regs, struct st_scanparams *, struct st_calibration_data *, struct st_calibration *, SANE_Int value);

/* functions for gain and offset calibration */
static SANE_Status RTS_cal_gain  (st_device *, struct st_calibration_config *, struct st_calibration_data *, SANE_Int arg2, SANE_Int gaincontrol);
static SANE_Status RTS_cal_offset(st_device *, struct st_calibration_config *, struct st_calibration_data *, SANE_Int value);
static SANE_Status RTS_cal_pagain(st_device *, struct st_calibration_config *, struct st_calibration_data *, SANE_Int gainmode);

/* functions for shading calibration */
static void        cal_shd_preview_free (st_device *);
static SANE_Status cal_shd_preview_alloc(st_device *, SANE_Int dots);

static void        cal_shd_buffers_free (struct st_calibration *caltables);
static SANE_Status cal_shd_buffers_alloc(st_device *, struct st_scanparams *, struct st_calibration *);

static SANE_Status RTS_cal_shd_black(st_device *, struct st_calibration_config *, struct st_calibration_data *, struct st_calibration *, SANE_Int gaincontrol);
static SANE_Status RTS_cal_shd_white(st_device *, struct st_calibration_config *, struct st_calibration_data *, struct st_calibration *, SANE_Int gainmode);

static SANE_Status cal_shd_apply(st_device *, SANE_Byte *Regs, struct st_scanparams *, struct st_calibration *);
static SANE_Status cal_shd_apply_buffer(st_device *, SANE_Byte *Regs, SANE_Int channels, struct st_calibration *, SANE_Int shtype);

/* functions to check shading buffers sent to scanner */
static SANE_Status RTS_shd_check(st_device *, struct st_check_shading *, SANE_Byte *Regs, SANE_Byte *shadtable, SANE_Int channel);
static SANE_Status RTS_shd_check_alloc(SANE_Byte *Regs, struct st_check_shading *, SANE_Int shading_length, SANE_Int min_size);
static void        RTS_shd_check_free(struct st_check_shading *);
static SANE_Status RTS_shd_check_fill(SANE_Byte *source_table, SANE_Byte **dest_table, SANE_Int dest_size, SANE_Int from_pos, SANE_Int table_count, SANE_Int *table_sizes);
static SANE_Status RTS_shd_check_fillex(SANE_Byte *source_table, SANE_Byte *dest_table, SANE_Int from_pos, SANE_Int dest_size, SANE_Int table_count, SANE_Int *table_sizes);
static SANE_Status RTS_shd_check_run(st_device *, SANE_Byte *Regs, SANE_Byte *ee2c, SANE_Byte **tables, SANE_Int edac, SANE_Int ee14, SANE_Int ee0c, SANE_Int *table_sizes, SANE_Int table_count, SANE_Int edb0);

/* Spread-Spectrum Clock Generator functions */
static SANE_Status SSCG_Enable(st_device *);

static void Split_into_12bit_channels(SANE_Byte *destino, SANE_Byte *fuente, SANE_Int size);
static SANE_Status Scan_Read_BufferA(st_device *, SANE_Int buffer_size, SANE_Int arg2, SANE_Byte *pBuffer, SANE_Int *bytes_transfered);

static SANE_Status GetOneLineInfo(st_device *, SANE_Int scantype, SANE_Int resolution, SANE_Int *maximus, SANE_Int *minimus, double *average);

static SANE_Status RTS_exposure_set(st_device *, SANE_Byte *Regs);

static void Set_E950_Mode(st_device *, SANE_Byte mode);

static SANE_Status LoadImagingParams(st_device *, SANE_Int inifile);

static SANE_Status SetScanParams(st_device *, SANE_Byte *Regs, struct st_scanparams *, struct st_hwdconfig *);
static SANE_Status IsScannerLinked(st_device *);

static SANE_Status Read_FE3E(st_device *, SANE_Byte *destino);

static double get_shrd(double value, SANE_Int desp);
static char get_byte(double value);

/* ----------------- Implementation ------------------*/

static void wfree(void *ptr)
{
	/* check pointer before trying to release it */
	if (ptr != NULL)
		free(ptr);
}

static void RTS_free(st_device *dev)
{
	/* this function frees space of devices's variable */

	if (dev == NULL)
		return;

	/* free config before releasing memory */
	RTS_free_config(dev);

	wfree(dev->usb);
	wfree(dev->init_regs);
	wfree(dev->chipset);
	wfree(dev->options);
	wfree(dev->Resize);
	wfree(dev->Reading);
	wfree(dev->scanning);
	wfree(dev->preview);

	if (dev->gamma != NULL)
	{
		RTS_gamma_free(dev);
		free(dev->gamma);
	}

	wfree(dev->status);
	free(dev);
}

static st_device *RTS_alloc()
{
	/* this function allocates space for device's variable */

	st_device *dev = NULL;
	SANE_Status rst = SANE_STATUS_INVAL;

	if ((dev = calloc (1, sizeof(st_device))) == NULL)
		return dev;

	if (((dev->usb = calloc (1, sizeof(struct st_usb))) != NULL) &&
			((dev->init_regs = RTS_regs_alloc(SANE_TRUE)) != NULL) &&
			((dev->chipset = calloc (1, sizeof(struct st_chip))) != NULL) &&
			((dev->options = calloc (1, sizeof(struct st_options))) != NULL) &&
			((dev->scanning = calloc (1, sizeof(struct st_scanning))) != NULL) &&
			((dev->Reading = calloc (1, sizeof(struct st_readimage))) != NULL) &&
			((dev->Resize = calloc (1, sizeof(struct st_resize))) != NULL) &&
			((dev->status = calloc (1, sizeof(struct st_status))) != NULL) &&
			((dev->gamma = calloc (1, sizeof(struct st_gammatables))) != NULL) &&
			((dev->preview = calloc (1, sizeof(struct st_preview))) != NULL))
				rst = SANE_STATUS_GOOD;

	if (rst == SANE_STATUS_GOOD)
	{
		/* initialize structure */
		dev->model     = HP3970;

		/* default values for usb structure */
		dev->usb->type         = -1;   /* unknown, autodetect */
		dev->usb->timeout      = 1000; /* msecs */
		dev->usb->blk_read_ep  = 0x81;
		dev->usb->blk_write_ep = 0x02;

		/* these values will be overwritten if chipset is known */
		dev->chipset->dma.transfer_size = 0x80000;
		dev->chipset->dma.set_length    = 0x7c0000;
		dev->chipset->dma.buffer_size   = 0x400000;

		/* Lamp settings */
		dev->options->overdrive_flb = 10000; /* msecs */
		dev->options->overdrive_ta  = 10000; /* msecs */

		dev->options->dbg_image     = SANE_FALSE;
		dev->options->wht_board     = SANE_FALSE;

		dev->options->fixed_pwm     = SANE_TRUE;
		dev->options->do_warmup     = SANE_TRUE;

		/* Calibration settings */
		dev->options->calibrate     = 2;
		dev->options->shd_white     = SANE_TRUE;
		dev->options->shd_black     = SANE_TRUE;

		/* setting default gamma 8 bits and its tables */
		dev->options->use_gamma     = SANE_TRUE;
		dev->options->gamma_depth   = GAMMA_8BIT;

		dev->status->preview        = SANE_FALSE;

		/* create default gamma tables in 8 bit depth */
		RTS_gamma_alloc(dev, -1, 8, NULL);
	} else
	{
		/* something failed, free space */
		RTS_free(dev);
		dev = NULL;
	}

	return dev;
}

/** SEC: device information */
static SANE_Int RTS_device_get(SANE_Int product, SANE_Int vendor)
{
	SANE_Int rst = -1;
	SANE_Int a = 0, up, uv;

	do
	{
		up = cfg_device_info(a, -1, 1); /* product */
		uv = cfg_device_info(a, -1, 0); /* vendor */

		if ((up == -1)||(uv == -1))
			break;

		if ((up == product)&&(uv = vendor))
		{
			/* found */
			rst = cfg_device_info(a, -1, 2); /* vendor */
		}

		a++;
	} while (rst == -1);

	return rst;
}

static SANE_Int RTS_device_supported(SANE_Int pos)
{
	/* returns the internal device name of given pos*/
	return cfg_device_info(pos, -1, 2);
}

static SANE_Int RTS_device_usb_vendor(SANE_Int device)
{
	return cfg_device_info(-1, device, 0);
}

static SANE_Int RTS_device_usb_product(SANE_Int device)
{
	return cfg_device_info(-1, device, 1);
}

static char *RTS_device_vendor(SANE_Int device)
{
	/* returns the vendor string about given device */

	char *rst = cfg_device_info_str(device, 1);

	if (rst == NULL)
		rst = (char *) strdup("Unknown");

	return rst;
}

static char *RTS_device_product(SANE_Int device)
{
	/* returns the product string about given device */

	char *rst = NULL;

	if ((rst = cfg_device_info_str(device, 2)) == NULL)
		rst = (char *) strdup("RTS8822 chipset based");

	return rst;
}

static SANE_Int RTS_device_count()
{
	/* returns the number of scanners supported by this driver */

	SANE_Int rst = 0;

	while (cfg_device_info(rst, -1, 0) != -1)
		rst++;

	return rst;
}

static char *RTS_device_name(SANE_Int device)
{
	/* returns the internal name string about given device */

	char *rst = cfg_device_info_str(device, 0);

	if (rst == NULL)
		rst = (char *) strdup("Unknown");

	return rst;
}

static SANE_Int RTS_device_set(st_device *dev, SANE_Int proposed, SANE_Int product, SANE_Int vendor)
{
	/* This function will set the device behaviour */

	SANE_Int current  = -1;
	char *sname;

	if (dev == NULL)
		return current;

	/* autodetect ? */
	if ((product != -1)&&(vendor != -1))
	{
		current = RTS_device_get (product, vendor);
		if ((sname   = RTS_device_name(current)) != NULL)
		{
			DBG(DBG_VRB, "- Device (%04x:%04x) model detected as %s\n", vendor, product, sname);
			free (sname);
		}
	}

	/* propose model ? */
	if ((proposed != -1)&&(proposed < RTS_device_count()))
	{
		current = proposed;
		if ((sname = RTS_device_name(proposed)) != NULL)
		{
			DBG(DBG_VRB, "- Treating device as %s\n", sname);
			free (sname);
		}
	}

	/* set to default model ? */
	if (current == -1)
	{
		current = HP3970;
		if ((sname = RTS_device_name(current)) != NULL)
		{
			DBG(DBG_VRB, "- Defaulting to %s\n", sname);
			free (sname);
		}
	}

	dev->model = current;

	return current;
}

static SANE_Status RTS_scanner_init(st_device *dev)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "+ RTS_scanner_init:\n");
	DBG(DBG_FNC, " -> Backend version: %s\n", BACKEND_VRSN);

	if (dev == NULL)
	{
		DBG(DBG_FNC, "-- ASSERT failed\n");
		return rst;
	}

	/* gets usb type of this scanner if it's not already set by user */
	if (dev->usb->type == -1)
	{
		SANE_Byte data;

		if (RTS_ctl_read_byte(dev, 0xfe11, &data) == SANE_STATUS_GOOD)
		{
			dev->usb->type = (data & 1);
			DBG(DBG_FNC, " -> USB type detected by chipset: %s\n", (dev->usb->type == USB11)? "USB1.1" : "USB2.0");
		}
	}

	if (dev->usb->type != -1)
	{
		DBG(DBG_FNC, " -> Chipset model ID: %i\n", RTS_chip_id(dev));

		/* reset chipset data to avoid unwanted status and other bin */
		RTS_chip_reset(dev);

		/* load configuration based on selected device/chipset */
		if (RTS_load_config(dev) == SANE_STATUS_GOOD)
		{
			if (IsScannerLinked(dev) == SANE_STATUS_GOOD)
			{
				Set_E950_Mode(dev, 0);
				rst = SANE_STATUS_GOOD;
			} else RTS_free_config(dev);
		}
	}

	DBG(DBG_FNC, "- RTS_scanner_init: %i\n", rst);

	return rst;
}

/** SEC: Chipset registers control functions */

static SANE_Status RTS_regs_write(st_device *dev, SANE_Byte *buffer)
{
	/* send registers buffer to chipset */
	return RTS_ctl_write_buffer(dev, 0xe800, buffer, RT_BUFFER_LEN * sizeof(SANE_Byte));
}

static SANE_Status RTS_regs_read(st_device *dev, SANE_Byte *buffer)
{
	/* read registers buffer from chipset */
	return RTS_ctl_read_buffer(dev, 0xe800, buffer, RT_BUFFER_LEN * sizeof(SANE_Byte));
}

static SANE_Byte *RTS_regs_alloc(SANE_Int clearbuffer)
{
	/* allocate buffer for rts registers and reset if desired */
	SANE_Byte *rst = (SANE_Byte *) malloc(RT_BUFFER_LEN * sizeof(SANE_Byte));

	if ((rst != NULL)&&(clearbuffer != SANE_FALSE))
		memset(rst, 0, sizeof(SANE_Byte) * RT_BUFFER_LEN);

	return rst;
}

static SANE_Status RTS_regs_cpy(SANE_Byte *dest, SANE_Byte *source)
{
	/* copy rts registers buffer to another buffer */
	SANE_Status rst = SANE_STATUS_GOOD;

	if ((dest != NULL)&&(source != NULL))
		memcpy(dest, source, RT_BUFFER_LEN * sizeof(SANE_Byte));
			else rst = SANE_STATUS_INVAL;

	return rst;
}

static SANE_Byte *RTS_regs_dup(SANE_Byte *source)
{
	/* generate a new registers buffer from given source */
	SANE_Byte *rst = NULL;

	if (source != NULL)
	{
		if ((rst = RTS_regs_alloc(SANE_FALSE)) != NULL)
			RTS_regs_cpy(rst, source);
	}

	return rst;
}

/** SEC: Scanning config control functions */

static struct st_scanparams *RTS_scancfg_alloc(SANE_Int clearbuffer)
{
	struct st_scanparams *rst = (struct st_scanparams *) malloc(sizeof(struct st_scanparams));

	if ((rst != NULL)&&(clearbuffer != SANE_FALSE))
		memset(rst, 0, sizeof(struct st_scanparams));

	return rst;
}

static SANE_Status RTS_scancfg_cpy(struct st_scanparams *dest, struct st_scanparams *source)
{
	SANE_Status rst = SANE_STATUS_GOOD;

	if ((dest != NULL)&&(source != NULL))
		memcpy(dest, source, sizeof(struct st_scanparams));
			else rst = SANE_STATUS_INVAL;

	return rst;
}

static struct st_scanparams *RTS_scancfg_dup(struct st_scanparams *source)
{
	struct st_scanparams *rst = NULL;

	if (source != NULL)
	{
		if ((rst = RTS_scancfg_alloc(SANE_FALSE)) != NULL)
			RTS_scancfg_cpy(rst, source);
	}

	return rst;
}

static struct st_hwdconfig *RTS_hwdcfg_alloc(SANE_Int clearbuffer)
{
	struct st_hwdconfig *rst = (struct st_hwdconfig *) malloc(sizeof(struct st_hwdconfig));

	if ((rst != NULL)&&(clearbuffer != SANE_FALSE))
		memset(rst, 0, sizeof(struct st_hwdconfig));

	return rst;
}

static void SetLock(st_device *dev, SANE_Byte *Regs, SANE_Byte Enable)
{
	DBG(DBG_FNC, "+ SetLock(Enable=%i):\n", Enable);

	if (dev != NULL)
	{
		SANE_Byte lock;

		if (Regs == NULL)
		{
			if (RTS_ctl_read_byte(dev, 0xee00, &lock) != SANE_STATUS_GOOD)
				lock = 0;
		} else lock = Regs[0x600];

		if (Enable == SANE_FALSE)
			lock &= 0xfb;
				else lock |= 4;

		if (Regs != NULL)
			Regs[0x600] = lock;

		RTS_ctl_write_byte(dev, 0xee00, lock);
	}

	DBG(DBG_FNC, "- SetLock\n");
}

static void Set_E950_Mode(st_device *dev, SANE_Byte mode)
{
	DBG(DBG_FNC, "+ Set_E950_Mode(mode=%i):\n", mode);

	if (dev != NULL)
	{
		SANE_Int data;

		if (RTS_ctl_read_word(dev, 0xe950, &data) == SANE_STATUS_GOOD)
		{
			data = (mode == 0)? data & 0xffbf : data | 0x40;
			RTS_ctl_write_word(dev, 0xe950, data);
		}
	}

	DBG(DBG_FNC, "- Set_E950_Mode\n");
}

static struct st_curve *RTS_mtr_curve_get(st_device *dev, SANE_Int motorcurve, SANE_Int direction, SANE_Int itype)
{
	struct st_curve *rst = NULL;

	if (dev == NULL)
		return NULL;

	if ((dev->mtrsetting != NULL)&&(motorcurve < dev->mtrsetting_count))
	{
		struct st_motorcurve *mtc = dev->mtrsetting[motorcurve];

		if (mtc == NULL)
			return NULL;

		if ((mtc->curve != NULL)&&(mtc->curve_count > 0))
		{
			struct st_curve *crv;
			SANE_Int a = 0;

			while (a < mtc->curve_count)
			{
				/* get each curve */
				if ((crv = mtc->curve[a]) != NULL)
				{
					/* check direction and type */
					if ((crv->crv_speed == direction)&&(crv->crv_type == itype))
					{
						/* found ! */
						rst = crv;
						break;
					}
				}
				a++;
			}
		}
	}

	return rst;
}

static SANE_Int RTS_mtr_curve_equal(st_device *dev, SANE_Int motorsetting, SANE_Int direction, SANE_Int curve1, SANE_Int curve2)
{
	/* compares two curves of the same direction
	   returns SANE_TRUE if both buffers are equal */

	SANE_Int rst = SANE_FALSE;
	struct st_curve *crv1, *crv2;

	if (dev == NULL)
		return rst;

	crv1 = RTS_mtr_curve_get(dev, motorsetting, direction, curve1);
	crv2 = RTS_mtr_curve_get(dev, motorsetting, direction, curve2);

	if ((crv1 != NULL)&&(crv2 != NULL))
	{
		if (crv1->step_count == crv2->step_count)
		{
			if (memcmp(crv1->step, crv2->step, crv1->step_count * sizeof(SANE_Int)) == 0)
				rst = SANE_TRUE;
		}
	}

	return rst;
}

static struct st_motorcurve **RTS_mtr_curve_parse(SANE_Int *mtc_count, SANE_Int *buffer)
{
	/* this function parses motorcurve buffer to get all motor settings */
	struct st_motorcurve **rst = NULL;

	if ((buffer != NULL) && (mtc_count != NULL))
	{
		/* phases:
		   -1 : null phase
		    0 :
		   -3 : initial config
		*/
		struct st_motorcurve *mtc = NULL;
		SANE_Int phase = -1;

		*mtc_count = 0;

		while (*buffer != -1)
		{
			if (*buffer == -2)
			{
				/* end of motorcurve */
				/* complete any openned phase */
				/* close phase */
				phase = -1;
			} else
			{
				/* step */
				if (phase == -1)
				{
					/* new motorcurve */
					phase = 0;
					mtc = (struct st_motorcurve *) malloc(sizeof(struct st_motorcurve));
					if (mtc != NULL)
					{
						*mtc_count += 1;
						rst = realloc(rst, sizeof(struct st_motorcurve **) * *mtc_count);
						if (rst != NULL)
						{
							rst[*mtc_count - 1] = mtc;
							memset(mtc, 0, sizeof(struct st_motorcurve));
							phase = -3; /* initial config */
						} else
						{
							/* memory error */
							*mtc_count = 0;
							break;
						}
					} else break; /* some error */
				}

				if (mtc != NULL)
				{
					switch(phase)
					{
						case -3: /* initial config */
							mtc->mri = *(buffer);
							mtc->msi = *(buffer + 1);
							mtc->skiplinecount = *(buffer + 2);
							mtc->motorbackstep = *(buffer + 3);
							buffer += 3;

							phase = -4;
							break;

						case -4: /**/
							{
								/* create new step curve */
								struct st_curve *curve = malloc(sizeof(struct st_curve));
								if (curve != NULL)
								{
									/* add to step curve list */
									mtc->curve = (struct st_curve **)realloc(mtc->curve, sizeof(struct st_curve **) * (mtc->curve_count + 1));
									if (mtc->curve != NULL)
									{
										mtc->curve_count++;
										mtc->curve[mtc->curve_count - 1] = curve;

										memset(curve, 0, sizeof(struct st_curve));
										/* read crv speed and type */
										curve->crv_speed = *buffer;
										curve->crv_type  = *(buffer + 1);
										buffer += 2;

										/* get length of step buffer */
										while (*(buffer + curve->step_count) != 0)
											curve->step_count++;

										if (curve->step_count > 0)
										{
											/* allocate step buffer */
											if ((curve->step = (SANE_Int *) malloc(sizeof(SANE_Int) * curve->step_count)) != NULL)
											{
												memcpy(curve->step, buffer, sizeof(SANE_Int) * curve->step_count);
												buffer += curve->step_count;
											} else curve->step_count = 0;
										}
									} else
									{
										mtc->curve_count = 0;
										free(curve);
									}
								} else break;
							}
							break;
					}
				}
			}
			buffer++;
		}
	}

	return rst;
}

static void RTS_mtr_curve_free(struct st_motorcurve **motorcurves, SANE_Int *mtc_count)
{
	struct st_motorcurve *mtc;
	struct st_curve *curve;

	if (motorcurves == NULL || mtc_count == NULL)
		return;

	while (*mtc_count > 0)
	{
		if ((mtc = motorcurves[*mtc_count - 1]) != NULL)
		{
			if (mtc->curve != NULL)
			{
				while(mtc->curve_count > 0)
				{
					curve = mtc->curve[mtc->curve_count - 1];
					if ((curve = mtc->curve[mtc->curve_count - 1]) != NULL)
					{
						wfree(curve->step);
						free(curve);
					}
					mtc->curve_count--;
				}
			}
			free(mtc);
		}
		*mtc_count -= 1;
	}

	free(motorcurves);
}

static SANE_Byte RTS_sensor_type(st_device *dev)
{
	/*
	 Returns sensor type
	 01 = CCD
	 00 = CIS
	*/

	SANE_Byte rst = CCD_SENSOR; /* default */

	DBG(DBG_FNC, "+ RTS_sensor_type:\n");

	if (dev != NULL)
	{
		SANE_Int a, b, c;

		a = b = c = 0;

		/* Save data first */
		RTS_ctl_read_word(dev, 0xe950, &a);
		RTS_ctl_read_word(dev, 0xe956, &b);
		
		/* Enables GPIO 0xe950 writing directly 0x13ff */
		RTS_ctl_write_word(dev, 0xe950, 0x13ff);
		/* Sets GPIO 0xe956 writing 0xfcf0 */
		RTS_ctl_write_word(dev, 0xe956, 0xfcf0);
		/* Makes a sleep of 200 ms */
		usleep(1000*200);
		/* Get GPIO 0xe968 */
		RTS_ctl_read_word(dev, 0xe968, &c);
		/* Restore data */
		RTS_ctl_write_word(dev, 0xe950, a);
		RTS_ctl_write_word(dev, 0xe956, b);

		rst = ((_B1(c) & 1) == 0)? CCD_SENSOR : CIS_SENSOR;
	}

	DBG(DBG_FNC, "- RTS_sensor_type: %s\n", (rst == CCD_SENSOR)? "CCD" : "CIS");

	return rst;
}

static void RTS_free_scanmodes(st_device *dev)
{
	DBG(DBG_FNC, "> RTS_free_scanmodes\n");

	if (dev == NULL)
		return;

	if (dev->scanmodes != NULL)
	{
		if (dev->scanmodes_count > 0)
		{
			SANE_Int a;
			for (a = 0; a < dev->scanmodes_count; a++)
				wfree(dev->scanmodes[a]);
		}

		free(dev->scanmodes);
		dev->scanmodes = NULL;
	}

	dev->scanmodes_count = 0;
}

static SANE_Status RTS_load_scanmodes(st_device *dev)
{
	SANE_Status rst = SANE_STATUS_INVAL;
	SANE_Int a, b;
	struct st_scanmode reg, *mode;

	DBG(DBG_FNC, "> RTS_load_scanmodes\n");

	if (dev == NULL)
		return rst;

	rst = SANE_STATUS_GOOD;

	if ((dev->scanmodes != NULL)||(dev->scanmodes_count > 0))
		RTS_free_scanmodes(dev);

	a = 0;
	while ((cfg_scanmode_get(dev, a, &reg) == SANE_STATUS_GOOD)&&(rst == SANE_STATUS_GOOD))
	{
		if ((mode = (struct st_scanmode *) malloc(sizeof(struct st_scanmode))) != NULL)
		{
			memcpy(mode, &reg, sizeof(struct st_scanmode));

			for (b = 0; b < 3; b++)
			{
				if (mode->mexpt[b] == 0)
				{
					mode->mexpt[b] = mode->ctpc;
					if (mode->multiexposure != 1)
						mode->expt[b] = mode->ctpc;
				}
			}

			mode->ctpc = ((mode->ctpc + 1) * mode->multiexposure) - 1;

			dev->scanmodes = (struct st_scanmode **)realloc(dev->scanmodes, (dev->scanmodes_count + 1) * sizeof(struct st_scanmode **));
			if (dev->scanmodes != NULL)
			{
				dev->scanmodes[dev->scanmodes_count] = mode;
				dev->scanmodes_count++;
			} else rst = SANE_STATUS_INVAL;
		} else rst = SANE_STATUS_INVAL;

		a++;
	}

	if (rst == SANE_STATUS_INVAL)
		RTS_free_scanmodes(dev);

	DBG(DBG_FNC, " -> Found %i scanmodes\n", dev->scanmodes_count);
	dbg_scanmodes(dev);

	return rst;
}

static void RTS_free_config(st_device *dev)
{
	DBG(DBG_FNC, "+ RTS_free_config\n");

	if (dev != NULL)
	{
		/* free buttons */
		RTS_free_buttons(dev);

		/* free motor general configuration */
		RTS_free_motor(dev);

		/* free sensor main configuration */
		RTS_free_sensor(dev);

		/* free ccd sensor timing tables */
		RTS_free_timings(dev);

		/* free motor curves */
		RTS_free_motor_curves(dev);

		/* free motor movements */
		RTS_free_RTS_mtr_moves(dev);

		/* free scan modes */
		RTS_free_scanmodes(dev);

		/* free constrains */
		RTS_free_constrains(dev);

		/* free chipset configuration */
		RTS_free_chip(dev);
	}

	DBG(DBG_FNC, "- RTS_free_config\n");
}

static void RTS_free_chip(st_device *dev)
{
	DBG(DBG_FNC, "> RTS_free_chip\n");

	if (dev != NULL)
		if (dev->chipset != NULL)
		{
			wfree(dev->chipset->name);
			memset(dev->chipset, 0, sizeof(struct st_chip));
		}
}

static SANE_Status RTS_load_chip(st_device *dev)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "> RTS_load_chip\n");

	if (dev != NULL)
		if (dev->chipset != NULL)
		{
			SANE_Int model;

			RTS_free_chip(dev);

			/* get chipset model of selected scanner */
			model = cfg_chipset_model_get(dev->model);

			/* get configuration for selected chipset */
			rst = cfg_chipset_get(model, dev->chipset);
		}

	/* if rst == SANE_STATUS_INVAL may be related to allocating space for chipset name */

	return rst;
}

static SANE_Status RTS_load_config(st_device *dev)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "+ RTS_load_config\n");

	if (dev != NULL)
	{
		/* load chipset configuration */
		RTS_load_chip(dev);

		/* load scanner's buttons */
		RTS_load_buttons(dev);

		/* load scanner area constrains */
		RTS_load_constrains(dev);

		/* load motor general configuration */
		RTS_load_motor(dev);

		/* load sensor main configuration */
		RTS_load_sensor(dev);

		if (dev->sensorcfg->type == -1)
			/* get sensor from gpio */
			dev->sensorcfg->type = RTS_sensor_type(dev);

		/* load ccd sensor timing tables */
		RTS_load_timings(dev);

		/* load motor curves */
		RTS_load_motor_curves(dev);

		/* load motor movements */
		RTS_load_RTS_mtr_moves(dev);

		/* load scan modes */
		RTS_load_scanmodes(dev);

		/* deprecated */
		if (dev->sensorcfg->type == CCD_SENSOR)
			/* ccd */ usbfile = (dev->usb->type == USB20)? T_RTINIFILE : T_USB1INIFILE;
				else /* cis */ usbfile = (dev->usb->type == USB20)? S_RTINIFILE : S_USB1INIFILE;

		scantype = ST_NORMAL;

		pwmlamplevel   = get_value(dev, SCAN_PARAM, PWMLAMPLEVEL, 1, usbfile);

		arrangeline2 = get_value(dev, SCAN_PARAM, ARRANGELINE, FIX_BY_HARD, usbfile);

		shadingbase     = get_value(dev, TRUE_GRAY_PARAM, SHADINGBASE, 3, usbfile);
		shadingfact[0]  = get_value(dev, TRUE_GRAY_PARAM, SHADINGFACT1, 1, usbfile);
		shadingfact[1]  = get_value(dev, TRUE_GRAY_PARAM, SHADINGFACT2, 1, usbfile);
		shadingfact[2]  = get_value(dev, TRUE_GRAY_PARAM, SHADINGFACT3, 1, usbfile);

		Lumping = 0; /* this value should change with each scanner*/

		LoadImagingParams(dev, usbfile);

		rst = SANE_STATUS_GOOD;
	}

	DBG(DBG_FNC, "- RTS_load_config\n");

	return rst;
}

static SANE_Status LoadImagingParams(st_device *dev, SANE_Int inifile)
{
	DBG(DBG_FNC, "> LoadImagingParams(inifile='%i'):\n", inifile);

	arrangeline    = get_value(dev, SCAN_PARAM, ARRANGELINE, FIX_BY_HARD, inifile);
	compression    = get_value(dev, SCAN_PARAM, COMPRESSION, 0, inifile);

	linedarlampoff = get_value(dev, CALI_PARAM, LINEDARLAMPOFF, 0, inifile);

	pixeldarklevel = get_value(dev, CALI_PARAM, PIXELDARKLEVEL, 0x00ffff, inifile);

	binarythresholdh     = get_value(dev, PLATFORM, BINARYTHRESHOLDH, 0x80, inifile);
	binarythresholdl     = get_value(dev, PLATFORM, BINARYTHRESHOLDL, 0x7f, inifile);

	return SANE_STATUS_GOOD;
}

static SANE_Status Arrange_Colour(st_device *dev, SANE_Byte *buffer, SANE_Int buffer_size, SANE_Int *transferred)
{
	/*
	05F0FA78   04EC00D8  /CALL to Assumed StdFunc2 from hpgt3970.04EC00D3
	05F0FA7C   05D10048  |Arg1 = 05D10048
	05F0FA80   0000F906  \Arg2 = 0000F906
	*/

	SANE_Status rst = SANE_STATUS_INVAL;
	SANE_Int mydistance;
	SANE_Int Lines_Count;
	SANE_Int space;
	SANE_Int c;
	struct st_scanning *scn;

	DBG(DBG_FNC, "> Arrange_Colour(buffer_size=%i)\n", buffer_size);

	if (dev == NULL || buffer == NULL || transferred == NULL)
	{
		DBG(DBG_FNC, "-- ASSERT failed\n");
		return rst;
	}

	rst = SANE_STATUS_GOOD;

	/* this is just to make code more legible */
	scn = dev->scanning;

	if (scn->imagebuffer == NULL)
	{
		if (dev->sensorcfg->type == CCD_SENSOR)
			mydistance = (dev->sensorcfg->line_distance * scan2.resolution_y) / dev->sensorcfg->resolution;
					else mydistance = 0;

		/*aafa*/
		if (mydistance != 0)
		{
			scn->bfsize = (scn->arrange_hres == SANE_TRUE) ? scn->arrange_sensor_evenodd_dist: 0;
			scn->bfsize = (scn->bfsize + (mydistance * 2) + 1) * line_size;
		} else scn->bfsize = line_size * 2;

		/*ab3c */
		space = (((scn->bfsize / line_size) * bytesperline) > scn->bfsize) ? (scn->bfsize / line_size) * bytesperline : scn->bfsize;

		if ((scn->imagebuffer = (SANE_Byte *) malloc(space * sizeof (SANE_Byte))) != NULL)
		{
			scn->imagepointer = scn->imagebuffer;

			if (Read_Block(dev, scn->bfsize, scn->imagebuffer, transferred) == SANE_STATUS_GOOD)
			{
				scn->arrange_orderchannel = SANE_FALSE;
				scn->channel_size = (scan2.depth == 8)? 1: 2;

				/* Calculate channel displacements */
				for (c = CL_RED; c <= CL_BLUE; c++)
				{
					if (mydistance == 0)
					{
						/*ab9b*/
						if (scn->arrange_hres == SANE_FALSE)
						{
							if ((((dev->sensorcfg->line_distance * scan2.resolution_y) * 2) / dev->sensorcfg->resolution) == 1)
								scn->arrange_orderchannel = SANE_TRUE;

							if (scn->arrange_orderchannel == SANE_TRUE)
								scn->desp[c] = ((dev->sensorcfg->rgb_order[c] / 2) * line_size) + (scn->channel_size * c);
									else scn->desp[c] = scn->channel_size * c;
						}
					} else
					{
						/*ac32*/
						scn->desp[c] = (dev->sensorcfg->rgb_order[c] * (mydistance * line_size)) + (scn->channel_size * c);

						if (scn->arrange_hres == SANE_TRUE)
						{
							scn->desp1[c] = scn->desp[c];
							scn->desp2[c] = ((scn->channel_size * 3) + scn->desp[c]) + (scn->arrange_sensor_evenodd_dist * line_size);
						}
					}
				}

				/*ace2*/
				for (c = CL_RED; c <= CL_BLUE; c++)
				{
					if (scn->arrange_hres == SANE_TRUE)
					{
						scn->pColour2[c] = scn->imagebuffer + scn->desp2[c];
						scn->pColour1[c] = scn->imagebuffer + scn->desp1[c];
					} else scn->pColour[c] = scn->imagebuffer + scn->desp[c];
				}
			} else rst = SANE_STATUS_INVAL;
		} else rst = SANE_STATUS_INVAL;
	}

	if (rst == SANE_STATUS_GOOD)
	{
		/*ad91 */
		Lines_Count = buffer_size / line_size;
		while (Lines_Count > 0)
		{
			if (scn->arrange_orderchannel == SANE_FALSE)
			{
				if (scn->arrange_hres == SANE_TRUE)
					Triplet_Colour_HRes(scn->pColour1[CL_RED], scn->pColour1[CL_GREEN], scn->pColour1[CL_BLUE], scn->pColour2[CL_RED], scn->pColour2[CL_GREEN], scn->pColour2[CL_BLUE], buffer, line_size / (scn->channel_size * 3));
						else Triplet_Colour_LRes(line_size / (scn->channel_size * 3), buffer, scn->pColour[CL_RED], scn->pColour[CL_GREEN], scn->pColour[CL_BLUE]);
			} else Triplet_Colour_Order(dev, scn->pColour[CL_RED], scn->pColour[CL_GREEN], scn->pColour[CL_BLUE], buffer, line_size / (scn->channel_size * 3));

			scn->arrange_size -= bytesperline;
			if (scn->arrange_size < 0)
				v15bc--;

			buffer += line_size;

			Lines_Count--;
			if (Lines_Count == 0)
			{
				if ((scn->arrange_size | v15bc) == 0)
					break; /* rst is SANE_STATUS_GOOD */
			}

			if (Read_Block(dev, line_size, scn->imagepointer, transferred) != SANE_STATUS_INVAL)
			{
				/* Update displacements */
				for (c = CL_RED; c <= CL_BLUE; c++)
				{
					if (scn->arrange_hres == SANE_TRUE)
					{
						/*aeb7*/
						scn->desp2[c] = (scn->desp2[c] + line_size) % scn->bfsize;
						scn->desp1[c] = (scn->desp1[c] + line_size) % scn->bfsize;

						scn->pColour2[c] = scn->imagebuffer + scn->desp2[c];
						scn->pColour1[c] = scn->imagebuffer + scn->desp1[c];
					} else
					{
						/*af86 */
						scn->desp[c] = (scn->desp[c] + line_size) % scn->bfsize;
						scn->pColour[c] = scn->imagebuffer + scn->desp[c];
					}
				}

				/*aff3 */
				scn->imagepointer += line_size;
				if (scn->imagepointer >= (scn->imagebuffer + scn->bfsize))
					scn->imagepointer = scn->imagebuffer;
			} else
			{
				rst = SANE_STATUS_INVAL;
				break;
			}
		}
	}

	return rst;
}

static SANE_Status RTS_scanner_setup(st_device *dev, struct params *param)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "+ RTS_scanner_setup:\n");

	if (dev == NULL || param == NULL)
	{
		DBG(DBG_FNC, "-- ASSERT failed\n");
		return rst;
	}

	DBG(DBG_FNC, "->  param->resolution_x=%i\n", param->resolution_x);
	DBG(DBG_FNC, "->  param->resolution_y=%i\n", param->resolution_y);
	DBG(DBG_FNC, "->  param->left        =%i\n", param->coords.left);
	DBG(DBG_FNC, "->  param->width       =%i\n", param->coords.width);
	DBG(DBG_FNC, "->  param->top         =%i\n", param->coords.top);
	DBG(DBG_FNC, "->  param->height      =%i\n", param->coords.height);
	DBG(DBG_FNC, "->  param->colormode   =%s\n", dbg_colour(param->colormode));
	DBG(DBG_FNC, "->  param->scantype    =%s\n", dbg_scantype(param->scantype));
	DBG(DBG_FNC, "->  param->depth       =%i\n", param->depth);
	DBG(DBG_FNC, "->  param->channel     =%i\n", param->channel);

	/* validate area size to scan */
	if ((param->coords.width != 0) && (param->coords.height != 0))
	{
		SANE_Byte mybuffer[1];
		struct st_hwdconfig *hwdcfg = RTS_hwdcfg_alloc(SANE_TRUE);

		if (hwdcfg != NULL)
		{
			/* setting coordinates */
			memcpy(&scan.coord, &param->coords, sizeof(struct st_coords));

			/* setting resolution */
			scan.resolution_x = param->resolution_x;
			scan.resolution_y = param->resolution_y;

			/* setting colormode and depth */
			scan.colormode = param->colormode;
			scan.depth = (param->colormode == CM_LINEART)? 8: param->depth;

			/* setting color channel for non color scans */
			scan.channel = _B0(param->channel);

			arrangeline = FIX_BY_HARD;
			if ((scan.resolution_x == 2400)||((scan.resolution_x == 4800)))
			{
				if (scan.colormode != CM_COLOR)
				{
					if (scan.colormode == CM_GRAY)
					{
						if (scan.channel == 3)
							arrangeline = FIX_BY_SOFT;
					}
				} else arrangeline = FIX_BY_SOFT;
			}

			/* setting scan type */
			if ((param->scantype > 0)&&(param->scantype < 4))
				scan.scantype  = param->scantype;
					else scan.scantype  = ST_NORMAL;

			/* setting scanner lamp */
			data_bitset(&dev->init_regs[0x146], 0x40, ((dev->sensorcfg->type == CIS_SENSOR)? 0: 1));

			/* turn on appropiate lamp */
			RTS_lamp_status_set(dev, NULL, SANE_TRUE, (scan.scantype == ST_NORMAL)? FLB_LAMP: TMA_LAMP);

			mybuffer[0] = 0;
			if (RTS_scan_running(dev, mybuffer) == SANE_FALSE)
				RTS_regs_write(dev, dev->init_regs);

			if (scan.depth == 16)
				compression = SANE_FALSE;

			/* setting low level config */
			hwdcfg->scantype = scan.scantype;
			hwdcfg->calibrate = mybuffer[0];
			hwdcfg->arrangeline = arrangeline; /*1*/
			hwdcfg->highresolution = (scan.resolution_x > 1200)? SANE_TRUE : SANE_FALSE;
			hwdcfg->sensorevenodddistance = dev->sensorcfg->evenodd_distance;

			SetScanParams(dev, dev->init_regs, &scan, hwdcfg);

			scan.shadinglength = (((((scan.sensorresolution << 4) + scan.sensorresolution) >> 1) + 3) >> 2) * 4;

			rst = SANE_STATUS_GOOD;

			free (hwdcfg);
		}
	}

	DBG(DBG_FNC, "- RTS_scanner_setup: %i\n", rst);

	return rst;
}

static SANE_Status SetScanParams(st_device *dev, SANE_Byte *Regs, struct st_scanparams *scancfg, struct st_hwdconfig *hwdcfg)
{
	struct st_coords mycoords;
	SANE_Int mycolormode;
	SANE_Int myvalue;
	SANE_Int mymode;
	SANE_Int channel_size;
	SANE_Int channel_count;
	SANE_Int dots_per_block;
	SANE_Int  aditional_dots;

	DBG(DBG_FNC, "+ SetScanParams:\n");
	dbg_ScanParams(scancfg);
	dbg_hwdcfg(hwdcfg);

	memset(&mycoords, 0, sizeof(struct st_coords));

	/* Copy scancfg to scan2 */
	RTS_scancfg_cpy(&scan2, scancfg);

	mycolormode = scancfg->colormode;
	myvalue     = scancfg->colormode;
	scantype    = hwdcfg->scantype;
	
	if (scancfg->colormode == CM_LINEART)
		scan2.depth = 8;

	if ((scancfg->colormode != CM_COLOR) && (scancfg->channel == 3)) /*channel = 0x00*/
	{
		if (scancfg->colormode == CM_GRAY)
		{
			mycolormode = (hwdcfg->arrangeline != FIX_BY_SOFT)? 3 : CM_COLOR;
		} else mycolormode = 3;
		myvalue = mycolormode;
	}

	dev->Resize->resolution_x = scancfg->resolution_x;
	dev->Resize->resolution_y = scancfg->resolution_y;

	mymode = RTS_scanmode_get(dev, hwdcfg->scantype, myvalue,  scancfg->resolution_x); /*0x0b*/
	if (mymode == -1)
	{
		/* Non supported resolution. We will resize image after scanning */
		SANE_Int fitres;

		fitres = RTS_scanmode_fitres(dev, hwdcfg->scantype, scancfg->colormode, scancfg->resolution_x);
		if (fitres != -1)
		{
			/* supported resolution found */
			dev->Resize->type = RSZ_DECREASE;
		} else
		{
			dev->Resize->type = RSZ_INCREASE;
			fitres = RTS_scanmode_maxres(dev, hwdcfg->scantype, scancfg->colormode);
		}

		scan2.resolution_x = fitres;
		scan2.resolution_y = fitres;

		if ((mymode = RTS_scanmode_get(dev, hwdcfg->scantype, myvalue,  scan2.resolution_x)) == -1)
			return SANE_STATUS_INVAL;

		imageheight     = scancfg->coord.height;
		dev->Resize->towidth = scancfg->coord.width;

		/* Calculate coords for new resolution */
		mycoords.left   = (scan2.resolution_x * scancfg->coord.left) / dev->Resize->resolution_x;
		mycoords.width  = (scan2.resolution_x * scancfg->coord.width) / dev->Resize->resolution_x;
		mycoords.top    = (scan2.resolution_y * scancfg->coord.top) / dev->Resize->resolution_y;
		mycoords.height = ((scan2.resolution_y * scancfg->coord.height) / dev->Resize->resolution_y) + 2;

		switch(scan2.colormode)
		{
			case CM_GRAY:
				if ((dev->scanmodes[mymode]->samplerate == PIXEL_RATE) && (mycolormode != 3))
					dev->Resize->towidth *= 2;

				channel_size  = (scan2.depth == 8) ? 1 : 2;
				dev->Resize->mode  = (scan2.depth == 8) ? RSZ_GRAYL : RSZ_GRAYH;
				dev->Resize->bytesperline = dev->Resize->towidth * channel_size;
				break;
			case CM_LINEART:
				if (dev->scanmodes[mymode]->samplerate == PIXEL_RATE)
					dev->Resize->towidth *= 2;
 
				dev->Resize->mode   = RSZ_LINEART;
				dev->Resize->bytesperline = (dev->Resize->towidth + 7) / 8;
				break;
			default: /*CM_COLOR*/
				channel_count = 3;
				channel_size  = (scan2.depth == 8) ? 1 : 2;
				dev->Resize->mode  = (scan2.depth == 8) ? RSZ_COLOURL : RSZ_COLOURH;
				dev->Resize->bytesperline = scancfg->coord.width * (channel_count * channel_size);
				break;
		}
	} else
	{
		/* Supported scanmode */
		dev->Resize->type = RSZ_NONE;
		scan2.resolution_x  = scancfg->resolution_x;
		scan2.resolution_y  = scancfg->resolution_y;
		mycoords.left	      = scancfg->coord.left;
		mycoords.top	      = scancfg->coord.top;
		mycoords.width	    = scancfg->coord.width;
		mycoords.height	    = scancfg->coord.height;
	}

	scancfg->timing = dev->scanmodes[mymode]->timing;

	scan2.sensorresolution = dev->timings[scancfg->timing]->sensorresolution;
	if ((scantype > 0) && (scantype < 5))
		scan2.shadinglength = (((((scan2.sensorresolution << 4) + scan2.sensorresolution) >> 1) + 3) >> 2) * 4;

	scancfg->sensorresolution = scan2.sensorresolution;
	scancfg->shadinglength    = scan2.shadinglength;

	dev->scanning->arrange_compression = ((mycolormode != CM_LINEART) && (scan2.depth <= 8))? hwdcfg->compression : SANE_FALSE;

	if ((arrangeline2 == FIX_BY_HARD) || (mycolormode == CM_LINEART))
		arrangeline2 = mycolormode; /*�?*/
			else if ((mycolormode == CM_GRAY) && (hwdcfg->highresolution == SANE_FALSE))
				arrangeline2 = 0;

	if (hwdcfg->highresolution == SANE_FALSE)
	{
		/* resolution < 1200 dpi */
		dev->scanning->arrange_hres = SANE_FALSE;
		dev->scanning->arrange_sensor_evenodd_dist = 0;
	} else
	{
		/* resolution > 1200 dpi */
		dev->scanning->arrange_hres = SANE_TRUE;
		dev->scanning->arrange_sensor_evenodd_dist = hwdcfg->sensorevenodddistance;
	}

	/* with must be adjusted to fit in the dots count per block */
	aditional_dots = 0;
	if (mycolormode != CM_LINEART)
	{
		dots_per_block = ((scan2.resolution_x > 2400) && (scancfg->samplerate == PIXEL_RATE)) ? 8 : 4;

		/* fit width */
		if ((mycoords.width % dots_per_block) != 0)
		{
			aditional_dots = dots_per_block - (mycoords.width % dots_per_block);
			mycoords.width += aditional_dots;
		}
	} else
	{
		/* Lineart */
		if ((dots_per_block = 32 - (mycoords.width & 0x1f)) < 32)
		{
			/* line size must be multiple of 32, add necessary size to fit it*/

			DBG(DBG_FNC, " -> adjusting width to be multiple of 32\n");

			mycoords.width += dots_per_block;
			aditional_dots = (dots_per_block / 8);
		}
	}

	DBG(DBG_FNC, " -> dots_per_block: %i\n", dots_per_block);
	DBG(DBG_FNC, " -> aditional_dots: %i\n", aditional_dots);

	if (mycolormode == CM_LINEART)
	{
		bytesperline  = (dev->scanmodes[mymode]->samplerate == PIXEL_RATE)? mycoords.width / 4 : mycoords.width / 8;
		imagewidth3   = bytesperline;
		lineart_width = bytesperline * 8;
		line_size     = bytesperline - aditional_dots;
		dev->Resize->fromwidth = line_size * 8;
	} else
	{
		/*4510*/
		switch(mycolormode)
		{
			case CM_COLOR:
				channel_count = 3;
				break;
			case 3:
				channel_count = 1;
				break;
			case CM_GRAY:
				channel_count = (dev->scanmodes[mymode]->samplerate == PIXEL_RATE) ? 2 : 1; /*1*/
				break;
		}

		channel_size  = (scan2.depth + 7) / 8;
		bytesperline  = mycoords.width * (channel_count * channel_size);
		imagewidth3   = bytesperline / channel_count;
		lineart_width = imagewidth3 / channel_size;
		line_size     = bytesperline - (aditional_dots * (channel_count * channel_size));
		dev->Resize->fromwidth  = line_size / (channel_count * channel_size);
	}

	imagesize = mycoords.height * bytesperline;
	v15b4 = 0;
	dev->scanning->arrange_size = imagesize;
	v15bc = 0;

	/* set resolution ratio */
	data_bitset(&Regs[0xc0], 0x1f, scancfg->sensorresolution / scancfg->resolution_x);

	scancfg->coord.left   = mycoords.left;
	scancfg->coord.top    = mycoords.top;
	scancfg->coord.width  = mycoords.width;
	scancfg->coord.height = mycoords.height;
	scancfg->resolution_x = scan2.resolution_x;
	scancfg->resolution_y = scan2.resolution_y;

	myvalue = (dev->Resize->type == RSZ_NONE)? line_size : dev->Resize->bytesperline;
	scancfg->bytesperline = bytesperline;
	
	scancfg->v157c = myvalue;
	
	if (scan.colormode != CM_COLOR)
	{
		if (mycolormode == CM_COLOR)
			scancfg->v157c = (scancfg->v157c / 3);
	}

	DBG(DBG_FNC, "- SetScanParams:\n");

	return SANE_STATUS_GOOD;
}

static SANE_Status RTS_gnoff_counter_save(st_device *dev, SANE_Byte data)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "> RTS_gnoff_counter_save(data=%i):\n", data);

	if (dev != NULL)
	{
		rst = SANE_STATUS_GOOD;

		/* check if chipset supports accessing eeprom */
		if ((dev->chipset->capabilities & CAP_EEPROM) != 0)
		{
			data = min(data, 0x0f);
			rst = RTS_nvram_write_byte(dev, 0x0077, data);
		}
	}

	return rst;
}

static SANE_Status RTS_gnoff_counter_inc(st_device *dev, SANE_Int *arg1)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "+ RTS_gnoff_counter_inc:\n");

	if (dev != NULL)
	{
		/* check if chipset supports accessing eeprom */
		if ((dev->chipset->capabilities & CAP_EEPROM) != 0)
		{
			SANE_Byte count = RTS_gnoff_counter_load(dev);

			if ((count >= 0x0f)||(RTS_gnoff_get(dev) != SANE_STATUS_GOOD))
			{
				/* reset preview gain and offset to perform calibration */
				dev->preview->offset[0] = dev->preview->offset[1] = dev->preview->offset[2] = 0;
				dev->preview->gain[0] = dev->preview->gain[1] = dev->preview->gain[2] = 0;
				count = 0;
			} else
			{
				count++;
				if (arg1 != NULL)
					*arg1 = 1;
			}

			/* save current count */
			rst = RTS_gnoff_counter_save(dev, count);
		} else rst = SANE_STATUS_GOOD;
	}

	DBG(DBG_FNC, "- RTS_gnoff_counter_inc: %i\n", rst);

	return rst;
}

static SANE_Status RTS_gnoff_get(st_device *dev)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "+ RTS_gnoff_get:\n");

	if (dev != NULL)
	{
		SANE_Int a, data;
		SANE_Byte checksum = 0;

		/* check if chipset supports accessing eeprom */
		if ((dev->chipset->capabilities & CAP_EEPROM) != 0)
		{
			/* get current checksum */
			if (RTS_nvram_read_byte(dev, 0x76, &checksum) == SANE_STATUS_GOOD)
			{
				rst = SANE_STATUS_GOOD;

				/* read gain and offset values from EEPROM */
				for (a = CL_RED; a <= CL_BLUE; a++)
				{
					if (RTS_nvram_read_word(dev, 0x70 + (2 * a), &data) == SANE_STATUS_INVAL)
					{
						rst = SANE_STATUS_INVAL;
						break;
					} else dev->preview->offset[a] = data;
				}

				/* check checksum */
				checksum = _B0(checksum + dev->preview->offset[CL_GREEN] + dev->preview->offset[CL_BLUE] + dev->preview->offset[CL_RED]);
			}
		}

		/* extract gain and offset values */
		if ((rst == SANE_STATUS_GOOD) && (checksum == 0x5b))
		{
			for (a = CL_RED; a <= CL_BLUE; a++)
			{
				dev->preview->gain[a]    = (dev->preview->offset[a] >> 9) & 0x1f;
				dev->preview->offset[a] &= 0x01ff;
			}
		} else
		{
			/* null values, let's reset them */
			for (a = CL_RED; a <= CL_BLUE; a++)
			{
				dev->preview->gain[a]   = 0;
				dev->preview->offset[a] = 0;
			}

			rst = SANE_STATUS_INVAL;
		}

		DBG(DBG_FNC, "->   Preview gainR=%i, gainG=%i, gainB=%i, offsetR=%i, offsetG=%i, offsetB=%i\n",
						dev->preview->gain[CL_RED], dev->preview->gain[CL_GREEN], dev->preview->gain[CL_BLUE],
						dev->preview->offset[CL_RED], dev->preview->offset[CL_GREEN], dev->preview->offset[CL_BLUE]);
	}

	DBG(DBG_FNC, "- RTS_gnoff_get: %i\n", rst);

	return rst;
}

static SANE_Int RTS_scanmode_maxres(st_device *dev, SANE_Int scantype, SANE_Int colormode)
{
	/* returns position in scanmodes table where data fits with given arguments */
	SANE_Int rst = 0;

	if (dev != NULL)
	{
		SANE_Int a;
		struct st_scanmode *reg;

		for (a = 0; a < dev->scanmodes_count; a++)
		{
			if ((reg = dev->scanmodes[a]) != NULL)
			{
				if ((reg->scantype == scantype)&&(reg->colormode == colormode))
					rst = max(rst, reg->resolution); /* found ! */
			}
		}

		if (rst == 0)
		{
			/* There isn't any mode for these arguments.
				Most devices doesn't support specific setup to scan in lineart mode
				so they use gray colormode. Lets check this case */
			if (colormode == CM_LINEART)
				rst = RTS_scanmode_maxres(dev, scantype, CM_GRAY);
		}
	}

	DBG(DBG_FNC, "> RTS_scanmode_maxres(scantype=%s, colormode=%s): %i\n", dbg_scantype(scantype), dbg_colour(colormode), rst);

	return rst;
}

static SANE_Int RTS_scanmode_minres(st_device *dev, SANE_Int scantype, SANE_Int colormode)
{
		/* returns position in scanmodes table where data fits with given arguments */
	SANE_Int rst = 0;

	if (dev != NULL)
	{
		SANE_Int a;
		struct st_scanmode *reg;

		rst = RTS_scanmode_maxres(dev, scantype, colormode);

		for (a = 0; a < dev->scanmodes_count; a++)
		{
			if ((reg = dev->scanmodes[a]) != NULL)
			{
				if ((reg->scantype == scantype)&&(reg->colormode == colormode))
					rst = min(rst, reg->resolution); /* found ! */
			}
		}

		if (rst == 0)
		{
			/* There isn't any mode for these arguments.
				Most devices doesn't support specific setup to scan in lineart mode
				so they use gray colormode. Lets check this case */
			if (colormode == CM_LINEART)
				rst = RTS_scanmode_minres(dev, scantype, CM_GRAY);
		}
	}

	DBG(DBG_FNC, "> RTS_scanmode_minres(scantype=%s, colormode=%s): %i\n", dbg_scantype(scantype), dbg_colour(colormode), rst);

	return rst;
}

static SANE_Int RTS_scanmode_fitres(st_device *dev, SANE_Int scantype, SANE_Int colormode, SANE_Int resolution)
{
	/* returns a supported resolution */
	SANE_Int rst = 0;

	if (dev != NULL)
	{
		SANE_Int a, nullres;
		struct st_scanmode *reg;

		nullres = RTS_scanmode_maxres(dev, scantype, colormode) + 1;
		rst = nullres;

		for (a = 0; a < dev->scanmodes_count; a++)
		{
			if ((reg = dev->scanmodes[a]) != NULL)
			{
				if ((reg->scantype == scantype)&&(reg->colormode == colormode))
				{
					if ((reg->resolution < rst)&&(resolution <= reg->resolution))
						rst = reg->resolution;
				}
			}
		}

		if (rst == nullres)
		{
			/* There isn't any mode for these arguments.
				Most devices doesn't support specific setup to scan in lineart mode
				so they use gray colormode. Lets check this case */
			if (colormode != CM_LINEART)
			{
				/* at this point, given resolution is bigger than maximum supported resolution */
				rst = -1;
			} else rst = RTS_scanmode_minres(dev, scantype, CM_GRAY);
		}
	}

	DBG(DBG_FNC, "> RTS_scanmode_fitres(scantype=%s, colormode=%s, resolution=%i): %i\n", dbg_scantype(scantype), dbg_colour(colormode), resolution, rst);

	return rst;
}

static SANE_Int RTS_scanmode_get(st_device *dev, SANE_Int scantype, SANE_Int colormode, SANE_Int resolution)
{
	/* returns position in scanmodes table where data fits with given arguments */
	SANE_Int rst = -1;

	if (dev != NULL)
	{
		SANE_Int a;
		struct st_scanmode *reg;

		for (a = 0; a < dev->scanmodes_count; a++)
		{
			if ((reg = dev->scanmodes[a]) != NULL)
			{
				if ((reg->scantype == scantype)&&(reg->colormode == colormode)&&(reg->resolution == resolution))
				{
					/* found ! */
					rst = a;
					break;
				}
			}
		}

		if (rst == -1)
		{
			/* There isn't any mode for these arguments.
				May be given resolution isn't supported by chipset.
				Most devices doesn't support specific setup to scan in lineart mode
				so they use gray colormode. Lets check this case */
			if (colormode == CM_LINEART)
				rst = RTS_scanmode_get(dev, scantype, CM_GRAY, resolution);
		}
	}

	DBG(DBG_FNC, "> RTS_scanmode_get(scantype=%s, colormode=%s, resolution=%i): %i\n", dbg_scantype(scantype), dbg_colour(colormode), resolution, rst);

	return rst;
}

static void RTS_free_motor(st_device *dev)
{
	/* this function releases space for stepper motor */

	DBG(DBG_FNC, "> RTS_free_motor\n");

	if (dev != NULL)
	{
		wfree(dev->motorcfg);
		dev->motorcfg = NULL;
	}
}

static SANE_Status RTS_load_motor(st_device *dev)
{
	/* this function loads general configuration for motor */

	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "> RTS_load_motor\n");

	if (dev != NULL)
	{
		if (dev->motorcfg != NULL)
			RTS_free_motor(dev);

		if ((dev->motorcfg = malloc(sizeof(struct st_motorcfg))) != NULL)
		{
			rst = cfg_motor_get(dev, dev->motorcfg);
			dbg_motorcfg(dev->motorcfg);
		}
	}

	return rst;
}

static void RTS_free_sensor(st_device *dev)
{
	/* this function releases space for ccd sensor */

	DBG(DBG_FNC, "> RTS_free_sensor\n");

	if (dev != NULL)
	{
		wfree(dev->sensorcfg);
		dev->sensorcfg = NULL;
	}
}

static void RTS_free_buttons(st_device *dev)
{
	/* this function releases space for buttons */

	DBG(DBG_FNC, "> RTS_free_buttons\n");

	if (dev != NULL)
	{
		wfree(dev->buttons);
		dev->buttons = NULL;
	}
}

static SANE_Status RTS_load_buttons(st_device *dev)
{
	/* this function loads configuration for ccd sensor */

	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "> RTS_load_buttons\n");

	if (dev != NULL)
	{
		if (dev->buttons != NULL)
			RTS_free_buttons(dev);

		if ((dev->buttons = malloc(sizeof(struct st_buttons))) != NULL)
		{
			rst = cfg_buttons_get(dev, dev->buttons);
			dbg_buttons(dev->buttons);
		}
	}

	return rst;
}

static SANE_Status RTS_load_sensor(st_device *dev)
{
	/* this function loads configuration for ccd sensor */

	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "> RTS_load_sensor\n");

	if (dev != NULL)
	{
		if (dev->sensorcfg != NULL)
			RTS_free_sensor(dev);

		if ((dev->sensorcfg = malloc(sizeof(struct st_sensorcfg))) != NULL)
		{
			rst = cfg_sensor_get(dev, dev->sensorcfg);
			dbg_sensor(dev->sensorcfg);
		}
	}

	return rst;
}

static void RTS_free_timings(st_device *dev)
{
	/* this function frees all ccd sensor timing tables */
	DBG(DBG_FNC, "> RTS_free_timings\n");

	if (dev != NULL)
	{
		if (dev->timings != NULL)
		{
			if (dev->timings_count > 0)
			{
				SANE_Int a;
				for (a = 0; a < dev->timings_count; a++)
					wfree(dev->timings[a]);

				dev->timings_count = 0;
			}

			free(dev->timings);
			dev->timings = NULL;
		}
	}
}

static SANE_Status RTS_load_timings(st_device *dev)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "> RTS_load_timings\n");

	if (dev != NULL)
	{
		SANE_Int a;
		struct st_timing reg, *tmg;

		rst = SANE_STATUS_GOOD;

		if (dev->timings != NULL)
			RTS_free_timings(dev);

		a = 0;

		while ((cfg_timing_get(dev, a, &reg) == SANE_STATUS_GOOD)&&(rst == SANE_STATUS_GOOD))
		{
			if ((tmg = (struct st_timing *) malloc(sizeof(struct st_timing))) != NULL)
			{
				memcpy(tmg, &reg, sizeof(struct st_timing));

				dev->timings_count++;
				dev->timings = (struct st_timing **) realloc(dev->timings , sizeof(struct st_timing **) * dev->timings_count);
				if (dev->timings == NULL)
				{
					rst = SANE_STATUS_INVAL;
					dev->timings_count = 0;
				} else dev->timings[dev->timings_count - 1] = tmg;
			} else rst = SANE_STATUS_INVAL;

			a++;
		}

		if (rst == SANE_STATUS_INVAL)
			RTS_free_timings(dev);

		DBG(DBG_FNC, " -> Found %i timing registers\n", dev->timings_count);
	}

	return rst;
}

static SANE_Status IsScannerLinked(st_device *dev)
{
	SANE_Status rst = SANE_STATUS_INVAL; /* default */

	DBG(DBG_FNC, "+ IsScannerLinked:\n");

	if (dev != NULL)
	{
		Read_FE3E(dev, &v1619);

		if (RTS_regs_init(dev) == SANE_STATUS_GOOD)
		{
			SANE_Int var2;
			SANE_Byte lamp;

			rst = SANE_STATUS_GOOD;

			scan.scantype = ST_NORMAL;

			RTS_WaitInitEnd(dev, 0x30000);

			lamp = FLB_LAMP;
			
			/* Comprobar si es la primera conexi�n con el escaner */
			if (RTS_ctl_read_word(dev, 0xe829, &var2) == SANE_STATUS_GOOD)
			{
				SANE_Int firstconnection;

				#ifdef STANDALONE
				firstconnection = SANE_TRUE;
				#else
				firstconnection = (var2 == 0)? SANE_TRUE : SANE_FALSE;
				#endif

				if (firstconnection == SANE_TRUE)
				{
					/* primera conexi�n */
					SANE_Byte flb_lamp, tma_lamp;

					flb_lamp = 0;
					tma_lamp = 0;
					RTS_lamp_status_get(dev, &flb_lamp, &tma_lamp);

					/* despite some chipsets allow both lamps to be switched on, such status
						shouldn't be proper*/
					if ((flb_lamp == 0) && (tma_lamp != 0))
						lamp = TMA_LAMP;

					/*Clear GainOffset count */
					RTS_gnoff_clear(dev);
					RTS_gnoff_counter_save(dev, 0);

					/* Clear AutoRef count */
					RTS_refs_counter_save(dev, 0);

					RTS_btn_enable(dev);
					RTS_lamp_status_timer_set(dev, 13);
				} else lamp = (_B0(var2) == 0)? FLB_LAMP: TMA_LAMP;
			}

			if (RTS_lamp_warmup_reset(dev) == SANE_STATUS_GOOD)
			{
				/* move head home if it isn't */
				RTS_head_park(dev, SANE_TRUE, dev->motorcfg->parkhomemotormove);

				/* set lamp turnning off timer to 13 minutes */
				RTS_lamp_status_timer_set(dev, 13);

				/* Use fixed pwm? */
				if (dev->options->fixed_pwm != SANE_FALSE)
				{
					RTS_lamp_pwm_save(dev, cfg_fixedpwm_get(dev, ST_NORMAL));

					/* Lets enable using fixed pwm */
					RTS_lamp_pwm_status_save(dev, SANE_TRUE);
				}

				RTS_lamp_pwm_setup(dev, lamp);

				dev->status->overdrive = SANE_TRUE;
			} else rst = SANE_STATUS_INVAL;
		}
	}

	DBG(DBG_FNC, "- IsScannerLinked: %i\n", rst);

	return rst;
}

static SANE_Status RTS_lamp_pwm_status_save(st_device *dev, SANE_Byte status)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "+ RTS_lamp_pwm_status_save(status=%i):\n", status);

	if (dev != NULL)
	{
		/* check if chipset supports accessing eeprom */
		if ((dev->chipset->capabilities & CAP_EEPROM) != 0)
		{
			SANE_Byte mypwm;

			if (RTS_nvram_read_byte(dev, 0x007b, &mypwm) == SANE_STATUS_GOOD)
			{
				mypwm = (status == SANE_FALSE)? mypwm & 0x7f : mypwm | 0x80;

				rst = RTS_nvram_write_byte(dev, 0x007b, mypwm);
			}
		} else rst = SANE_STATUS_GOOD;
	}

	DBG(DBG_FNC, "- RTS_lamp_pwm_status_save: %i\n", rst);

	return rst;
}

static SANE_Status RTS_lamp_pwm_save(st_device *dev, SANE_Int fixedpwm)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "+ RTS_lamp_pwm_save(fixedpwm=%i):\n", fixedpwm);

	if (dev != NULL)
	{
		/* check if chipset supports accessing eeprom */
		if ((dev->chipset->capabilities & CAP_EEPROM) != 0)
			rst = RTS_nvram_write_byte(dev, 0x7b, ((fixedpwm << 2) | 0x80));
				else rst = SANE_STATUS_GOOD;
	}

	DBG(DBG_FNC, "- RTS_lamp_pwm_save: %i\n", rst);

	return rst;
}

static SANE_Status RTS_lamp_pwm_setup(st_device *dev, SANE_Int lamp)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "+ RTS_lamp_pwm_setup(lamp=%s):\n", (lamp == FLB_LAMP)? "FLB_LAMP": "TMA_LAMP");

	if (dev != NULL)
	{
		rst = SANE_STATUS_GOOD;

		if (RTS_lamp_pwm_use(dev, 1) == SANE_STATUS_GOOD)
		{
			SANE_Int fixedpwm, currentpwd;

			currentpwd = 0;
			fixedpwm = cfg_fixedpwm_get(dev, (lamp == FLB_LAMP)? ST_NORMAL: ST_TA);

			if (RTS_lamp_pwm_duty_get(dev, &currentpwd) == SANE_STATUS_GOOD)
			{
				/* set duty cycle if current one is different */
				if (currentpwd != fixedpwm)
					rst = RTS_lamp_pwm_duty_set(dev, fixedpwm);
			} else rst = RTS_lamp_pwm_duty_set(dev, fixedpwm);
		}
	}

	DBG(DBG_FNC, "- RTS_lamp_pwm_setup: %i\n", rst);

	return rst;
}

static SANE_Status RTS_lamp_pwm_duty_get(st_device *dev, SANE_Int *data)
{
	SANE_Status rst = SANE_STATUS_INVAL;
	SANE_Int duty = -1;

	DBG(DBG_FNC, "+ RTS_lamp_pwm_duty_get:\n");

	if ((data != NULL)&&(dev != NULL))
	{
		SANE_Byte a;

		if (RTS_ctl_read_byte(dev, 0xe948, &a) == SANE_STATUS_GOOD)
		{
			*data = a & 0x3f;
			duty = *data;
			rst = SANE_STATUS_GOOD;
		}
	}

	DBG(DBG_FNC, "- RTS_lamp_pwm_duty_get = %i: %i\n", duty, rst);

	return rst;
}

static SANE_Status RTS_lamp_pwm_duty_set(st_device *dev, SANE_Int duty_cycle)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "+ RTS_lamp_pwm_duty_set(duty_cycle=%i):\n", duty_cycle);

	if (dev != NULL)
	{
		SANE_Byte *Regs = RTS_regs_alloc(SANE_FALSE);

		if (Regs != NULL)
		{
			if (RTS_regs_read(dev, Regs) == SANE_STATUS_GOOD)
			{
				data_bitset(&Regs[0x148], 0x3f, duty_cycle);

				if (pwmlamplevel == 0)
				{
					data_bitset(&Regs[0x148], 0x40, 0);
					Regs[0x1e0] |= ((duty_cycle >> 1) & 0x40);
				}

				data_bitset(&dev->init_regs[0x148], 0x7f, Regs[0x148]);
				data_bitset(&dev->init_regs[0x1e0], 0x3f, Regs[0x1e0]);

				RTS_ctl_write_byte(dev, 0xe948, Regs[0x0148]);

				rst = RTS_ctl_write_byte(dev, 0xe9e0, Regs[0x01e0]);
			}

			free(Regs);
		}
	}

	DBG(DBG_FNC, "- RTS_lamp_pwm_duty_set: %i\n", rst);

	return rst;
}

static SANE_Status RTS_head_park(st_device *dev, SANE_Int bWait, SANE_Int movement)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "+ RTS_head_park(bWait=%i, movement=%i):\n", bWait, movement);

	if (dev != NULL)
	{
		SANE_Byte *Regs = RTS_regs_dup(dev->init_regs);

		if (Regs != NULL)
		{
			rst = SANE_STATUS_GOOD;

			/* Lets wait if it's required when is already executing */
			if (bWait != SANE_FALSE)
			{
				if (RTS_scan_wait(dev, 0x3a98) != SANE_STATUS_GOOD)
				{
					DBG(DBG_FNC, " -> RTS_head_park: RTS_scan_wait Timeout\n");
					rst = SANE_STATUS_INVAL; /* timeout */
				}
			} else
			{
				if (RTS_scan_running(dev, Regs) == SANE_FALSE)
				{
					DBG(DBG_FNC, " -> RTS_head_park: RTS_scan_running = 0, exiting function\n");
					rst = SANE_STATUS_INVAL; /* if NOT executing */
				}
			}

			/* Check if lamp is at home */
			if ((rst == SANE_STATUS_GOOD)&&(RTS_head_athome(dev, Regs) == SANE_FALSE))
			{
				DBG(DBG_FNC, "->   RTS_head_park: Lamp is not at home, lets move\n");

				if ((movement != -1)&&(movement < dev->motormove_count))
				{
					struct st_motormove mymotor;
					struct st_motorpos mtrpos;

					dev->status->parkhome = SANE_TRUE;

					memcpy(&mymotor, dev->motormove[movement], sizeof(struct st_motormove));

					mtrpos.options = MTR_ENABLED | MTR_BACKWARD;
					mtrpos.v12e448 = 0x01;
					mtrpos.v12e44c = 0x00;
					mtrpos.coord_y = 0x4e20;

					RTS_mtr_move(dev, Regs, &mymotor, &mtrpos);

					/* Should we wait? */
					if (bWait != SANE_FALSE)
						rst = RTS_scan_wait(dev, 15000);

					dev->status->parkhome = SANE_FALSE;
				} else rst = SANE_STATUS_INVAL;
			}

			free(Regs);
		}
	}

	DBG(DBG_FNC, "- RTS_head_park: %i:\n", rst);

	return rst;
}

static SANE_Status RTS_mtr_move(st_device *dev, SANE_Byte *Regs, struct st_motormove *mymotor, struct st_motorpos *mtrpos)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "+ RTS_mtr_move:\n");

	if ((dev != NULL) && (mymotor != NULL) && (mtrpos != NULL))
	{
		SANE_Byte *cpRegs = RTS_regs_dup(Regs);

		if (cpRegs != NULL)
		{
			SANE_Int v12dcf8, coord_y, step_type;

			v12dcf8 = 0;

			/* resolution = 1 dpi */
			data_bitset(&cpRegs[0xc0], 0x1f, 1); /*---xxxxx*/

			/* set motor step type */
			data_bitset(&cpRegs[0xd9], 0x70, mymotor->scanmotorsteptype); /*-xxx----*/

			/* set motor direction (polarity) */
			data_bitset(&cpRegs[0xd9], 0x80, mtrpos->options >> 3);        /*e-------*/

			/* next value doesn't seem to have any effect */
			data_bitset(&cpRegs[0xd9], 0x0f, mtrpos->options);             /*----efgh*/

			/* 0 enable/1 disable motor */
			data_bitset(&cpRegs[0xdd], 0x80, mtrpos->options >> 4);        /*d-------*/

			/* next value doesn't seem to have any effect */
			data_bitset(&cpRegs[0xdd], 0x40, mtrpos->options >> 4);        /*-d------*/

			switch (mymotor->scanmotorsteptype)
			{
				case STT_OCT  : step_type = 8; break;
				case STT_QUART: step_type = 4; break;
				case STT_HALF : step_type = 2; break;
				case STT_FULL : step_type = 1; break;
				default       : step_type = 0; break; /* shouldn't be used */
			}

			coord_y = (mtrpos->coord_y * step_type ) & 0xffff;
			if (coord_y < 2)
				coord_y = 2;

			/* Sets dummyline to 1 */
			data_bitset(&cpRegs[0xd6], 0xf0, 1);

			/* set step_size - 1 */
			cpRegs[0xe0] = 0;

			cpRegs[0x01] &= 0xf9;
			cpRegs[0x01] |= (mtrpos->v12e448 & 1) << 2;

			/* set dummy scan */
			data_bitset(&cpRegs[0x01], 0x10, 1); /*---x----*/

			/* set samplerate */
			data_bitset(&cpRegs[0x1cf], 0x40, PIXEL_RATE); /*-x------*/

			/* unknown data */
			data_bitset(&cpRegs[0x1cf], 0x80, 1); /*x-------*/

			/* sets one chanel per color */
			data_bitset(&cpRegs[0x12], 0x3f, 0); /* channel   */
			data_bitset(&cpRegs[0x12], 0xc0, 1); /* 1 channel */

			/* timing cnpp */
			data_bitset(&cpRegs[0x96], 0x3f, 0x0b); /*--001011*/

			/* set systemclock */
			data_bitset(&cpRegs[0x00], 0x0f, mymotor->systemclock); /*----xxxx*/

			/* set last step of accurve.smearing table to 2 */
			data_lsb_set(&cpRegs[0xe4], 2, 3);

			/* set last step of deccurve.scanbufferfull table to 16 */
			data_lsb_set(&Regs[0xea], 0x10, 3);

			/* set last step of deccurve.normalscan table to 16 */
			data_lsb_set(&Regs[0xed], 0x10, 3);

			/* set last step of deccurve.smearing table to 16*/
			data_lsb_set(&Regs[0xf0], 0x10, 3);

			/* set last step of deccurve.parkhome table to 16 */
			data_lsb_set(&Regs[0xf3], 0x10, 3);

			/* set msi */
			cpRegs[0xda] = 2;
			cpRegs[0xdd] &= 0xfc;

			/* set if motor has motorcurves */
			data_bitset(&cpRegs[0xdf], 0x10, ((mymotor->motorcurve != -1)? 1: 0));

			if (mymotor->motorcurve != -1)
			{
				struct st_curve *crv;

				/* set last step of accurve.normalscan table */
				crv = RTS_mtr_curve_get(dev, mymotor->motorcurve, ACC_CURVE, CRV_NORMALSCAN);
				if (crv != NULL)
					data_lsb_set(&cpRegs[0xe1], crv->step[crv->step_count - 1], 3);

				DBG(DBG_FNC, " -> Setting up stepper motor using motorcurve %i\n", mymotor->motorcurve);
				v12dcf8 = RTS_mtr_setup_steps(dev, cpRegs, mymotor->motorcurve);

				/* set step_size - 1 */	
				cpRegs[0xe0] = 0;

				crv = RTS_mtr_curve_get(dev, mymotor->motorcurve, DEC_CURVE, CRV_NORMALSCAN);
				if (crv != NULL)
					coord_y -= (v12dcf8 + crv->step_count);

				/* set line exposure time */
				data_lsb_set(&cpRegs[0x30], mymotor->ctpc, 3);

				/* set last step of accurve.smearing table */
				data_lsb_set(&cpRegs[0xe4], 0, 3);
			} else
			{
				/* Setting some motor step */
				SANE_Int total_time;
				SANE_Int exposure_time;

				switch(Regs[0x00] & 0x0f)
				{
					case 0x00: total_time = 0x00895440; break; /*  3 x 0x2DC6C0 */
					case 0x08:
					case 0x01: total_time = 0x00b71b00; break; /*  4 x 0x2DC6C0 */
					case 0x02: total_time = 0x0112a880; break; /*  6 x 0x2DC6C0 */
					case 0x0a:
					case 0x03: total_time = 0x016e3600; break; /*  8 x 0x2DC6C0 */
					case 0x04: total_time = 0x02255100; break; /* 12 x 0x2DC6C0 */
					case 0x0c: total_time = 0x02dc6c00; break; /* 16 x 0x2DC6C0 */
					case 0x05: total_time = 0x044aa200; break; /* 24 x 0x2DC6C0 */
					case 0x0d: total_time = 0x05b8d800; break; /* 32 x 0x2DC6C0 */

					case 0x09: total_time = 0x00f42400; break;
					case 0x0b: total_time = 0x01e84800; break; /* = case 9 * 2 */
					default  : total_time = 0x0478f7f8; break;
				}

				/* divide by timing.cnpp */
				exposure_time = total_time / ((cpRegs[0x96] & 0x3f) + 1);
				if (mymotor->ctpc > 0)
					exposure_time  /= mymotor->ctpc;

				/* set line exposure time */
				data_lsb_set(&cpRegs[0x30], exposure_time , 3);

				/* set last step of accurve.normalscan table */
				data_lsb_set(&cpRegs[0xe1], exposure_time , 3);
			}

			/* Setting coords */
			RTS_setup_coords(cpRegs, 100, coord_y - 1, 800, 1);

			/* enable head movement */
			data_bitset(&cpRegs[0xd8], 0x80, 1);

			/* release motor before executing */
			RTS_mtr_release(dev);

			RTS_lamp_warmup_reset(dev);

			/* action! */
			if (RTS_regs_write(dev, cpRegs) == SANE_STATUS_GOOD)
			{
				rst = SANE_STATUS_GOOD;
				RTS_scan_run(dev);
				RTS_scan_wait(dev, 20000);
			} /* else rst = v12dcf8;*/

			/* wait 10 seconds */
			RTS_scan_wait(dev, 10000);

			free(cpRegs);
		}
	}

	DBG(DBG_FNC, "- RTS_mtr_move: %i\n", rst);

	return rst;
}

static void RTS_free_RTS_mtr_moves(st_device *dev)
{
	DBG(DBG_FNC, "> RTS_free_RTS_mtr_moves\n");

	if (dev != NULL)
	{
		if (dev->motormove != NULL)
		{
			SANE_Int a;
			struct st_motormove *ms;

			for (a = 0; a < dev->motormove_count; a++)
			{
				if ((ms = dev->motormove[a]) != NULL)
					free(ms);
			}

			free(dev->motormove);
			dev->motormove = NULL;
		}

		dev->motormove_count = 0;
	}
}

static void RTS_free_motor_curves(st_device *dev)
{
	DBG(DBG_FNC, "> RTS_free_motor_curves\n");

	if (dev != NULL)
	{
		if (dev->mtrsetting != NULL)
			RTS_mtr_curve_free(dev->mtrsetting, &dev->mtrsetting_count);

		dev->mtrsetting = NULL;
		dev->mtrsetting_count = 0;
	}
}

static SANE_Status RTS_load_motor_curves(st_device *dev)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "> RTS_load_motor_curves\n");

	if (dev != NULL)
	{
		SANE_Int *mtc = NULL;

		if (dev->mtrsetting != NULL)
			RTS_free_motor_curves(dev);

		/* get motor setttings buffer for this device */
		if ((mtc = cfg_motorcurve_get(dev)) != NULL)
		{
			/* parse buffer to get all motorcurves */
			if ((dev->mtrsetting = RTS_mtr_curve_parse(&dev->mtrsetting_count, mtc)) != NULL)
				rst = SANE_STATUS_GOOD;
		}

		if (rst != SANE_STATUS_INVAL)
		{
			DBG(DBG_FNC," -> Found %i motor settings\n", dev->mtrsetting_count);
			dbg_motorcurves(dev);
		} else DBG(DBG_ERR, "- RTS_load_motor_curves error!!\n");
	}

	return rst;
}

static SANE_Status RTS_load_RTS_mtr_moves(st_device *dev)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "> RTS_load_RTS_mtr_moves\n");

	if (dev != NULL)
	{
		SANE_Int a;
		struct st_motormove reg, *mm;

		rst = SANE_STATUS_GOOD;

		/* if there is already any movement loaded let's free it */
		if (dev->motormove != NULL)
			RTS_free_RTS_mtr_moves(dev);

		a = 0;
		while ((cfg_motormove_get(dev, a, &reg) != SANE_STATUS_INVAL)&&(rst == SANE_STATUS_GOOD))
		{
			dev->motormove_count++;
			dev->motormove = (struct st_motormove **)realloc(dev->motormove, sizeof(struct st_motormove **) * dev->motormove_count);
			if (dev->motormove != NULL)
			{
				if ((mm = (struct st_motormove *) malloc(sizeof(struct st_motormove))) != NULL)
				{
					memcpy(mm, &reg, sizeof(struct st_motormove));
					dev->motormove[dev->motormove_count - 1] = mm;
				} else rst = SANE_STATUS_INVAL;
			} else rst = SANE_STATUS_INVAL;

			a++;
		}

		if (rst == SANE_STATUS_INVAL)
			RTS_free_RTS_mtr_moves(dev);

		DBG(DBG_FNC, " -> Found %i motormoves\n", dev->motormove_count);
		dbg_motormoves(dev);
	}

	return rst;
}

static SANE_Byte *RTS_mtr_addstep(SANE_Byte *steps, SANE_Int *bwriten, SANE_Int step)
{
	if ((steps = (SANE_Byte *)realloc(steps, sizeof(SANE_Byte) * (*bwriten + 3))) != NULL)
	{
		data_msb_set(&steps[*bwriten], step, 3);
		*bwriten += 3;
	} else *bwriten = 0;

	return steps;
}

static SANE_Int RTS_mtr_setup_steps(st_device *dev, SANE_Byte *Regs, SANE_Int mysetting)
{
	SANE_Int varx10, cont, last_acc_step, varx20, offset_to_curve, mystep, bwriten;
	SANE_Int myvar, var1, myvalor, mybwriten;
	struct st_curve *mycurve;
	SANE_Byte *steps;

	DBG(DBG_FNC, "+ RTS_mtr_setup_steps(*Regs, motorsetting=%i):\n", mysetting);

	varx10 = 0;
	cont   = 0;
	varx20 = 0;
	offset_to_curve = (mem_segment_curve << 4) & 0xffff;
	steps = NULL;
	bwriten = 0;
	deccurvecount = 0;
	acccurvecount = 0;
	last_acc_step = 0;

	/* mycurve points to acc normalscan steps table */
	if ((mycurve = RTS_mtr_curve_get(dev, mysetting, ACC_CURVE, CRV_NORMALSCAN)) != NULL)
	{
		/* acccurvecount has the number of steps in acc normalscan table */
		acccurvecount = mycurve->step_count;

		/* get last acccurve step from acc.normalscan step table */
		last_acc_step = data_lsb_get(&Regs[0xe1], 3);

		/* sets pointer to acc.normalscan step table */
		data_wide_bitset(&Regs[0xf6], 0x3fff, offset_to_curve);

		/* Separate each step in three bytes */
		if (mycurve->step_count > 0)
			for (cont = 0; cont < mycurve->step_count; cont++)
			{
				mystep = mycurve->step[cont];
				if (mystep <= last_acc_step)
				{
					acccurvecount = cont;
					break;
				}
				varx20 += mystep + 1;
				steps = RTS_mtr_addstep(steps, &bwriten, mystep);
			}
	}

	if (acccurvecount == 0)
	{
		/* Write one step (last_acc_step + 0x01) to buffer */
		acccurvecount++;
		varx20 += (last_acc_step + 1) + 1;
		steps = RTS_mtr_addstep(steps, &bwriten, last_acc_step + 1);
	}

	/* write another step (last_acc_step) */
	acccurvecount++;
	varx20 += last_acc_step + 1;
	steps = RTS_mtr_addstep(steps, &bwriten, last_acc_step);

	/* get line exposure time */
	myvar = data_lsb_get(&Regs[0x30], 3) + 1;

	var1 = (varx20 + myvar - 1) / myvar;
	var1 = ((var1 * myvar) + mycurve->step[0] - varx20) - 0x0d;
	if (steps != NULL)
		data_msb_set(&steps[0], var1, 3);

	/* dec.scanbufferfull step table */
	/* set pointer to next table */
	offset_to_curve += (acccurvecount * 3);
	data_wide_bitset(&Regs[0xf8], 0x3fff, offset_to_curve);

	/* set last step of deccurve.scanbufferfull table */
	mycurve = RTS_mtr_curve_get(dev, mysetting, DEC_CURVE, CRV_BUFFERFULL);
	deccurvecount = mycurve->step_count;
	data_lsb_set(&Regs[0xea], mycurve->step[mycurve->step_count - 1], 3);

	/* write another step mycurve->step_count,cont,last_acc_step */
	deccurvecount++;
	steps = RTS_mtr_addstep(steps, &bwriten, last_acc_step);

	/* Separate each step in three bytes */
	if (mycurve->step_count > 1)
		for (cont = 0; cont < (mycurve->step_count - 1); cont++)
		{
			mystep = mycurve->step[cont];
			if (mystep > last_acc_step)
				steps = RTS_mtr_addstep(steps, &bwriten, mystep);
					else deccurvecount--;
		}

	if ((myvalor = dev->mtrsetting[mysetting]->motorbackstep) > 0)
	{
		SANE_Int step_size = _B0(Regs[0xe0]) + 1;

		myvalor = ((myvalor - deccurvecount) - acccurvecount) + 2;
		varx10  = myvalor;
		myvalor = (myvalor / step_size) * step_size;
		var1 = mycurve->step[mycurve->step_count - 1]; /* last deccurve step */
		if (last_acc_step >= var1)
			var1 = last_acc_step + 1;
		deccurvecount += (varx10 - myvalor);
		myvalor = varx10 - myvalor;
	} else myvalor = varx10;

	if (myvalor > 0)
		for (cont = myvalor; cont > 0; cont--)
			steps = RTS_mtr_addstep(steps, &bwriten, var1 - 1);

	/* write another step , bwriten tiene 4b */
	steps = RTS_mtr_addstep(steps, &bwriten, var1);

	/* acc.smearing step table */
	if (RTS_mtr_curve_get(dev, mysetting, ACC_CURVE, CRV_SMEARING) != NULL)
	{
		/* acc.smearing curve enabled */
		if (RTS_mtr_curve_equal(dev, mysetting, ACC_CURVE, CRV_SMEARING, CRV_NORMALSCAN) == SANE_TRUE)
		{
			/* acc.smearing pointer points to acc.normalscan table */
			data_wide_bitset(&Regs[0xfa], 0x3fff, data_lsb_get(&Regs[0xf6], 2));
			/* last step of acc.smearing table is the same as acc.normalscan */
			data_lsb_set(&Regs[0xe4], data_lsb_get(&Regs[0xe1], 3), 3);
		} else
		{
			/* set pointer to next step table */
			offset_to_curve += (deccurvecount * 3);
			data_wide_bitset(&Regs[0xfa], 0x3fff, offset_to_curve);

			/* set last step of acc.smearing table */
			if ((mycurve = RTS_mtr_curve_get(dev, mysetting, ACC_CURVE, CRV_SMEARING)) != NULL)
			{
				smearacccurvecount  = mycurve->step_count;
				data_lsb_set(&Regs[0xe4], mycurve->step[mycurve->step_count - 1], 3);

				/* generate acc.smearing table */
				if (mycurve->step_count > 0)
					for (cont = 0; cont < mycurve->step_count; cont++)
						steps = RTS_mtr_addstep(steps, &bwriten, mycurve->step[cont]);
			}
		}
	} else
	{
		/* acc.smearing curve disabled */
		data_wide_bitset(&Regs[0xfa], 0x3fff, 0);
	}

	/* dec.smearing */
	if (RTS_mtr_curve_get(dev, mysetting, DEC_CURVE, CRV_SMEARING) != NULL)
	{
		/* dec.smearing curve enabled */
		if (RTS_mtr_curve_equal(dev, mysetting, DEC_CURVE, CRV_SMEARING, CRV_BUFFERFULL) == SANE_TRUE)
		{
			/* dec.smearing pointer points to dec.scanbufferfull table */
			data_wide_bitset(&Regs[0x00fc], 0x3fff, data_lsb_get(&Regs[0x00f8], 2));
			/* last step of dec.smearing table is the same as dec.scanbufferfull */
			data_lsb_set(&Regs[0x00f0], data_lsb_get(&Regs[0x00ea], 3), 3);
		} else
		{
			/* set pointer to next step table */
			if (mycurve != NULL)
				offset_to_curve += (mycurve->step_count * 3);
			data_wide_bitset(&Regs[0xfc], 0x3fff, offset_to_curve);

			/* set last step of dec.smearing table */
			if ((mycurve = RTS_mtr_curve_get(dev, mysetting, DEC_CURVE, CRV_SMEARING)) != NULL)
			{
				smeardeccurvecount = mycurve->step_count;
				data_lsb_set(&Regs[0xf0], mycurve->step[mycurve->step_count - 1], 3);

				/* generate dec.smearing table */
				if (mycurve->step_count > 0)
					for (cont = 0; cont < mycurve->step_count; cont++)
						steps = RTS_mtr_addstep(steps, &bwriten, mycurve->step[cont]);
			}
		}
	} else
	{
		/* dec.smearing curve disabled */
		data_wide_bitset(&Regs[0x00fc], 0x3fff, 0);
	}

	/* dec.normalscan */
	if (RTS_mtr_curve_get(dev, mysetting, DEC_CURVE, CRV_NORMALSCAN) != NULL)
	{
		/* dec.normalscan enabled */
		if (RTS_mtr_curve_equal(dev, mysetting, DEC_CURVE, CRV_NORMALSCAN, CRV_BUFFERFULL) == SANE_TRUE)
		{
			/* dec.normalscan pointer points to dec.scanbufferfull table */
			data_wide_bitset(&Regs[0xfe], 0x3fff, data_lsb_get(&Regs[0xf8], 2));
			/* last step of dec.normalscan table is the same as dec.scanbufferfull */
			data_lsb_set(&Regs[0xed], data_lsb_get(&Regs[0xea], 3), 3);
		} else if (RTS_mtr_curve_equal(dev, mysetting, DEC_CURVE, CRV_NORMALSCAN, CRV_SMEARING) == SANE_TRUE)
		{
			/* dec.normalscan pointer points to dec.smearing table */
			data_wide_bitset(&Regs[0xfe], 0x3fff, data_lsb_get(&Regs[0xfc], 2));
			/* last step of dec.normalscan table is the same as dec.smearing */
			data_lsb_set(&Regs[0xed], data_lsb_get(&Regs[0xf0], 3), 3);
		} else
		{
			/* set pointer to next step table */
			if (mycurve != NULL)
				offset_to_curve += (mycurve->step_count * 3);
			data_wide_bitset(&Regs[0xfe], 0x3fff, offset_to_curve);

			/* set last step of dec.normalscan table */
			if ((mycurve = RTS_mtr_curve_get(dev, mysetting, DEC_CURVE, CRV_NORMALSCAN)) != NULL)
			{
				data_lsb_set(&Regs[0xed], mycurve->step[mycurve->step_count - 1], 3);

				/* generate dec.normalscan table */
				if (mycurve->step_count > 0)
					for (cont = 0; cont < mycurve->step_count; cont++)
						steps = RTS_mtr_addstep(steps, &bwriten, mycurve->step[cont]);
			}
		}
	} else
	{
		/* dec.normalscan disabled */
		data_wide_bitset(&Regs[0xfe], 0x3fff, 0);
	}

	/* acc.parkhome */
	if (RTS_mtr_curve_get(dev, mysetting, ACC_CURVE, CRV_PARKHOME) != NULL)
	{
		/* parkhome curve enabled */

		if (RTS_mtr_curve_equal(dev, mysetting, ACC_CURVE, CRV_PARKHOME, CRV_NORMALSCAN) == SANE_TRUE)
		{
			/* acc.parkhome pointer points to acc.normalscan table */
			data_wide_bitset(&Regs[0x100], 0x3fff, data_lsb_get(&Regs[0xf6], 2));

			/* last step of acc.parkhome table is the same as acc.normalscan */
			data_lsb_set(&Regs[0xe7], data_lsb_get(&Regs[0xe1], 3), 3);
		} else if (RTS_mtr_curve_equal(dev, mysetting, ACC_CURVE, CRV_PARKHOME, CRV_SMEARING) == SANE_TRUE)
		{
			/* acc.parkhome pointer points to acc.smearing table */
			data_wide_bitset(&Regs[0x100], 0x3fff, data_lsb_get(&Regs[0xfa], 2));
			/* last step of acc.parkhome table is the same as acc.smearing */
			data_lsb_set(&Regs[0xe7], data_lsb_get(&Regs[0xe4], 3), 3);
		} else
		{
			/* set pointer to next step table */
			if (mycurve != NULL)
				offset_to_curve += (mycurve->step_count * 3);
			data_wide_bitset(&Regs[0x100], 0x3fff, offset_to_curve);

			/* set last step of acc.parkhome table */
			if ((mycurve = RTS_mtr_curve_get(dev, mysetting, ACC_CURVE, CRV_PARKHOME)) != NULL)
			{
				data_lsb_set(&Regs[0xe7], mycurve->step[mycurve->step_count - 1], 3);

				/* generate acc.parkhome table */
				if (mycurve->step_count > 0)
					for (cont = 0; cont < mycurve->step_count; cont++)
						steps = RTS_mtr_addstep(steps, &bwriten, mycurve->step[cont]);
			}
		}
	} else
	{
		/* parkhome curve is disabled */
		/* acc.parkhome pointer points to 0 */
		data_wide_bitset(&Regs[0x100], 0x3fff, 0);
		data_lsb_set(&Regs[0xe7], 16, 3);
	}

	/* dec.parkhome */
	if (RTS_mtr_curve_get(dev, mysetting, DEC_CURVE, CRV_PARKHOME) != NULL)
	{
		/* parkhome curve enabled */
		if (RTS_mtr_curve_equal(dev, mysetting, DEC_CURVE, CRV_PARKHOME, CRV_BUFFERFULL) == SANE_TRUE)
		{
			/* dec.parkhome pointer points to dec.scanbufferfull table */
			data_wide_bitset(&Regs[0x102], 0x3fff, data_lsb_get(&Regs[0xf8], 2));
			/* last step of dec.parkhome table is the same as dec.scanbufferfull */
			data_lsb_set(&Regs[0xf3], data_lsb_get(&Regs[0xe4], 3), 3);
		} else if (RTS_mtr_curve_equal(dev, mysetting, DEC_CURVE, CRV_PARKHOME, CRV_SMEARING) == SANE_TRUE)
		{
			/* dec.parkhome pointer points to dec.smearing table */
			data_wide_bitset(&Regs[0x102], 0x3fff, data_lsb_get(&Regs[0xfe], 2));
			/* last step of dec.parkhome table is the same as dec.smearing */
			data_lsb_set(&Regs[0xf3], data_lsb_get(&Regs[0xf0], 3), 3);
		} else if (RTS_mtr_curve_equal(dev, mysetting, DEC_CURVE, CRV_PARKHOME, CRV_NORMALSCAN) == SANE_TRUE)
		{
			/* dec.parkhome pointer points to dec.normalscan table */
			data_wide_bitset(&Regs[0x102], 0x3fff, data_lsb_get(&Regs[0xfe], 2));
			/* last step of dec.parkhome table is the same as dec.normalscan */
			data_lsb_set(&Regs[0xf3], data_lsb_get(&Regs[0xed], 3), 3);
		} else
		{
			/* set pointer to next step table */
			if (mycurve != NULL)
				offset_to_curve += (mycurve->step_count * 3);
			data_wide_bitset(&Regs[0x102], 0x3fff, offset_to_curve);

			/* set last step of dec.parkhome table */
			if ((mycurve = RTS_mtr_curve_get(dev, mysetting, DEC_CURVE, CRV_PARKHOME)) != NULL)
			{
				data_lsb_set(&Regs[0xf3], mycurve->step[mycurve->step_count - 1], 3);

				/* generate dec.parkhome table */
				if (mycurve->step_count > 0)
					for (cont = 0; cont < mycurve->step_count; cont++)
						steps = RTS_mtr_addstep(steps, &bwriten, mycurve->step[cont]);
			}
		}
	} else
	{
		/* parkhome curve is disabled */

		/* dec.parkhome pointer points to 0 */
		data_wide_bitset(&Regs[0x102], 0x3fff, 0);
		data_lsb_set(&Regs[0xf3], 16, 3);
	}

	mybwriten = bwriten & 0x8000000f;
	if (mybwriten < 0)
		mybwriten = ((mybwriten - 1) | 0xfffffff0) + 1;

	if (mybwriten != 0)
		bwriten = (((bwriten & 0xffff) >> 0x04) + 1) << 0x04;
	bwriten = bwriten & 0xffff;

	/* display table */
	DBG(DBG_FNC, " -> Direction Type           Offset Last step\n");
	DBG(DBG_FNC, " -> --------- -------------- ------ ---------\n");
	DBG(DBG_FNC, " -> ACC_CURVE CRV_NORMALSCAN %6i  %6i\n", data_lsb_get(&Regs[0x0f6], 2) & 0x3fff, data_lsb_get(&Regs[0x0e1], 3));
	DBG(DBG_FNC, " -> ACC_CURVE CRV_SMEARING   %6i  %6i\n", data_lsb_get(&Regs[0x0fa], 2) & 0x3fff, data_lsb_get(&Regs[0x0e4], 3));
	DBG(DBG_FNC, " -> ACC_CURVE CRV_PARKHOME   %6i  %6i\n", data_lsb_get(&Regs[0x100], 2) & 0x3fff, data_lsb_get(&Regs[0x0e7], 3));
	DBG(DBG_FNC, " -> DEC_CURVE CRV_NORMALSCAN %6i  %6i\n", data_lsb_get(&Regs[0x0fe], 2) & 0x3fff, data_lsb_get(&Regs[0x0ed], 3));
	DBG(DBG_FNC, " -> DEC_CURVE CRV_SMEARING   %6i  %6i\n", data_lsb_get(&Regs[0x0fc], 2) & 0x3fff, data_lsb_get(&Regs[0x0f0], 3));
	DBG(DBG_FNC, " -> DEC_CURVE CRV_PARKHOME   %6i  %6i\n", data_lsb_get(&Regs[0x102], 2) & 0x3fff, data_lsb_get(&Regs[0x0f3], 3));
	DBG(DBG_FNC, " -> DEC_CURVE CRV_BUFFERFULL %6i  %6i\n", data_lsb_get(&Regs[0x0f8], 2) & 0x3fff, data_lsb_get(&Regs[0x0ea], 3));

	RTS_lamp_warmup_reset(dev);

	/* send motor steps */
	if (steps != NULL)
	{
		if (bwriten > 0)
		{
			/* lock */
			SetLock(dev, Regs, SANE_TRUE);

			/* send steps */
			RTS_dma_write(dev, 0x0000, mem_segment_curve, bwriten, steps);

			/* unlock */
			SetLock(dev, Regs, SANE_FALSE);
		}

		free(steps);
	}

	DBG(DBG_FNC, "- RTS_mtr_setup_steps: %i\n", acccurvecount);

	return acccurvecount;
}

static SANE_Status RTS_lamp_pwm_use(st_device *dev, SANE_Int enable)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "+ RTS_lamp_pwm_use(enable=%i):\n", enable);

	if (dev != NULL)
	{
		SANE_Byte a, b;

		if (RTS_ctl_read_byte(dev, 0xe948, &a) == SANE_STATUS_GOOD)
		{
			if (RTS_ctl_read_byte(dev, 0xe9e0, &b) == SANE_STATUS_GOOD)
			{
				if (enable != 0)
				{
					if (pwmlamplevel != 0x00)
					{
						b |= 0x80;
						dev->init_regs[0x01e0] &= 0x3f;
						dev->init_regs[0x01e0] |= 0x80;
					} else
					{
						a |= 0x40;
						b &= 0x3f;
						dev->init_regs[0x0148] |= 0x40;
						dev->init_regs[0x01e0] &= 0x3f;
					}
				} else
				{
					b &= 0x7f;
					a &= 0xbf;
				}

				if (RTS_ctl_write_byte(dev, 0xe948, a) == SANE_STATUS_GOOD)
					rst = RTS_ctl_write_byte(dev, 0xe9e0, b);
			}
		}
	}

	DBG(DBG_FNC, "- RTS_lamp_pwm_use: %i\n", rst);

	return rst;
}

static SANE_Status SSCG_Enable(st_device *dev)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "+ SSCG_Enable:\n");

	if (dev != NULL)
	{
		SANE_Int sscg;
		SANE_Byte data1, data2;
		SANE_Int enable, mode, clock;

		rst = cfg_sscg_get(dev, &enable, &mode, &clock);

		if (rst == SANE_STATUS_GOOD)
		{
			if ((RTS_ctl_read_byte(dev, 0xfe3a, &data1) == SANE_STATUS_GOOD)&&(RTS_ctl_read_byte(dev, 0xfe39, &data2) == SANE_STATUS_GOOD))
			{
				if (enable != SANE_FALSE)
				{
					/* clock values: 0=0.25%; 1=0.50%; 2=0.75%; 3=1.00% */
					data2 = (mode == 0) ? data2 & 0x7f : data2 | 0x80;

					sscg = (data1 & 0xf3) | (((clock & 0x03) | 0x04) << 0x02);
					sscg = (sscg << 8) | data2;
				} else sscg = ((data1 & 0xef) << 8) | _B0(data2);

				rst = RTS_ctl_write_word(dev, 0xfe39, sscg);
			} else rst = SANE_STATUS_INVAL;
		}
	}

	DBG(DBG_FNC, "- SSCG_Enable: %i\n", rst);

	return rst;
}

static void RTS_setup_refvoltages(st_device *dev, SANE_Byte *Regs)
{
	/* this function sets top, midle and bottom reference voltages */

	DBG(DBG_FNC, "> RTS_setup_refvoltages\n");

	if ((dev != NULL) && (Regs != NULL))
	{
		SANE_Byte vrts, vrms, vrbs;

		cfg_refvoltages_get(dev, &vrts, &vrms, &vrbs);

		/* Top Reference Voltage */
		data_bitset(&Regs[0x14], 0xe0, vrts); /*xxx-----*/

		/* Middle Reference Voltage */
		data_bitset(&Regs[0x15], 0xe0, vrms); /*xxx-----*/

		/* Bottom Reference Voltage */
		data_bitset(&Regs[0x16], 0xe0, vrbs); /*xxx-----*/
	}
}

static SANE_Status RTS_regs_init(st_device *dev)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "+ RTS_regs_init:\n");

	if (dev != NULL)
	{
		SANE_Byte data;

		if (RTS_ctl_read_byte(dev, 0xf8ff, &data) == SANE_STATUS_GOOD)
		{
			data = (data | 1);
			rst = RTS_ctl_write_byte(dev, 0xf8ff, data);
		}

		if ((rst == SANE_STATUS_GOOD)&&(SSCG_Enable(dev) == SANE_STATUS_GOOD))
		{
			SANE_Byte *resource;
			SANE_Byte *rg;

			/* this is just for a cleaner code */
			rg = dev->init_regs;

			/* Lee dev->init_regs */
			memset(rg, 0, RT_BUFFER_LEN * sizeof(SANE_Byte));

			RTS_regs_read(dev, rg);

			Read_FE3E(dev, &v1619);
			
			if (dev->sensorcfg->type == CCD_SENSOR)
			{
				/* CCD sensor */
				data_bitset(&rg[0x11], 0xc0, 0);    /*xx------*/
				data_bitset(&rg[0x146], 0x80, 1);   /*x-------*/
				data_bitset(&rg[0x146], 0x40, 1);   /*-x------*/

			} else
			{
				/* CIS sensor */
				data_bitset(&rg[0x146], 0x80, 0);   /*0-------*/
				data_bitset(&rg[0x146], 0x40, 0);   /*-0------*/
				data_bitset(&rg[0x11], 0xc0, 2);    /*xx------*/
				data_bitset(&rg[0xae], 0x3f, 0x14); /*--xxxxxx*/
				data_bitset(&rg[0xaf], 0x07, 1);    /*-----xxx*/

				rg[0x9c] = rg[0xa2] = rg[0xa8] = (dev->model != UA4900)? 1: 0;
				rg[0x9d] = rg[0xa3] = rg[0xa9] = 0;
				rg[0x9e] = rg[0xa4] = rg[0xaa] = 0;
				rg[0x9f] = rg[0xa5] = rg[0xab] = 0;
				rg[0xa0] = rg[0xa6] = rg[0xac] = 0;
				rg[0xa1] = rg[0xa7] = rg[0xad] = (dev->model != UA4900)? 0x80: 0;
			}

			/* disable CCD channels*/
			data_bitset(&rg[0x10], 0xe0, 0);    /*xxx-----*/
			data_bitset(&rg[0x13], 0x80, 0);    /*x-------*/

			/* enable timer to switch off lamp */
			data_bitset(&rg[0x146], 0x10, 1);    /*---x----*/

			/* set time to switch off lamp */
			rg[0x147]  = 0xff;

			/* set last acccurve step */
			data_lsb_set(&rg[0xe1], 0x2af8, 3);

			/* set msi 0x02*/
			rg[0xda]  = 0x02;
			data_bitset(&rg[0xdd], 0x03, 0);    /*------xx*/

			/* set binary threshold high and low in little endian */
			data_lsb_set(&rg[0x19e], binarythresholdl, 2);
			data_lsb_set(&rg[0x1a0], binarythresholdh, 2);


			data_bitset(&rg[0x01], 0x08, 0);    /*----x---*/
			data_bitset(&rg[0x16f], 0x40, 0);   /*-x------*/
			rg[0x0bf]  = (rg[0x00bf] & 0xe0) | 0x20;
			rg[0x163]  = (rg[0x0163] & 0x3f) | 0x40;

			data_bitset(&rg[0xd6], 0x0f, 8);    /*----xxxx*/
			data_bitset(&rg[0x164], 0x80, 1);    /*x-------*/

			rg[0x0bc]  = 0x00;
			rg[0x0bd]  = 0x00;

			rg[0x165]  = (rg[0x0165] & 0x3f) | 0x80;
			rg[0x0ed]  = 0x10;
			rg[0x0be]  = 0x00;
			rg[0x0d5]  = 0x00;

			rg[0xee]  = 0x00;
			rg[0xef]  = 0x00;
			rg[0xde]  = 0xff;
			
			/* set bit[4] has_curves = 0 | bit[0..3] unknown = 0 */
			data_bitset(&rg[0xdf], 0x10, 0);    /*---x----*/
			data_bitset(&rg[0xdf], 0x0f, 0);    /*----xxxx*/

			/* Set motor type */
			data_bitset(&rg[0xd7], 0x80, dev->motorcfg->type); /*x-------*/

			if (dev->motorcfg->type == MT_ONCHIP_PWM)
			{
				data_bitset(&rg[0x14e], 0x10, 1); /*---x----*/

				/* set motorpwmfrequency */
				data_bitset(&rg[0xd7], 0x3f, dev->motorcfg->pwmfrequency); /*--xxxxxx*/
			}

			rg[0x600] &= 0xfb;
			rg[0x1d8] |= 0x08;

			v160c_block_size = 0x04;
			mem_total = 0x80000;

			/* check and setup installed ram */
			RTS_dma_type_check(dev, rg);
			rst = RTS_dma_wait(dev, 1500);

			/* Gamma table size = 0x100 */
			data_bitset(&rg[0x1d0], 0x30, 0x00); /*--00----*/

			/* Set 3 channels_per_dot */
			data_bitset(&rg[0x12], 0xc0, 0x03);  /*xx------*/

			/* systemclock */
			data_bitset(&rg[0x00], 0x0f, 0x05);  /*----xxxx*/

			/* timing cnpp */
			data_bitset(&rg[0x96], 0x3f, 0x17);  /*--xxxxxx*/

			/* set sensor_channel_color_order */
			data_bitset(&rg[0x60a], 0x7f, 0x24); /*-xxxxxxx*/

			/* set crvs */
			data_bitset(&rg[0x10], 0x1f, get_value(dev, SCAN_PARAM, CRVS, 7, usbfile)); /*---xxxxx*/

			/* set reference voltages */
			RTS_setup_refvoltages(dev, rg);

			rg[0x11] |= 0x10;

			data_bitset(&rg[0x26], 0x70, 5); /*-101----*/

			rg[0x185] = 0x88;
			rg[0x186] = 0x88;

			/* SDRAM clock*/
			data = get_value(dev, SCAN_PARAM, MCLKIOC, 8, usbfile);
			data_bitset(&rg[0x187], 0x0f, 0x08); /*----xxxx*/
			data_bitset(&rg[0x187], 0xf0, data); /*xxxx----*/

			data--;

			if (data < 7)
			{
				switch(data)
				{
					case 0: data |= 0xc0; break;
					case 1: data |= 0xa0; break;
					case 2: data |= 0xe0; break;
					case 3: data |= 0x90; break;
					case 4: data |= 0xd0; break;
					case 5: data |= 0xb0; break;
					case 6: data = (data & 0x0f); break;
				}
				rg[0x187] = _B0(data);
			}

			data_bitset(&rg[0x26], 0x0f, 0); /*----0000*/

			rg[0x27] &= 0x3f;
			rg[0x29]  = 0x24;
			rg[0x2a]  = 0x10;
			rg[0x150] = 0xff;
			rg[0x151] = 0x13;
			rg[0x156] = 0xf0;
			rg[0x157] = 0xfd;

			if (dev->motorcfg->changemotorcurrent != SANE_FALSE)
				RTS_mtr_change(dev, rg, 3);

			rg[0xde]  = 0;
			data_bitset(&rg[0xdf], 0x0f, 0);

			/* loads motor resource for this dev */
			resource = cfg_motor_resource_get(dev, &data);
			if ((resource != NULL)&&(data > 1))
				memcpy(&rg[0x104], resource, data);

			/* this bit is set but I don't know its purpose */
			rg[0x01] |= 0x40; /*-1------*/

			rg[0x124] = 0x94;

			/* release motor */
			RTS_mtr_release(dev);
		}
	}

	DBG(DBG_FNC, "- RTS_regs_init: %i\n", rst);

	return rst;
}

static SANE_Status Read_FE3E(st_device *dev, SANE_Byte *destino)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "+ Read_FE3E:\n");

	if ((dev != NULL)&&(destino != NULL))
	{
		SANE_Byte data;
		if (RTS_ctl_read_byte(dev, 0xfe3e, &data) == 0)
		{
			*destino = data;
			rst = SANE_STATUS_GOOD;
			DBG(DBG_FNC, " -> %02x\n", _B0(data));
		}
	}

	DBG(DBG_FNC, "- Read_FE3E: %i\n", rst);

	return rst;
}

static SANE_Int RTS_head_athome(st_device *dev, SANE_Byte *Regs)
{
	SANE_Int rst = SANE_TRUE;

	DBG(DBG_FNC, "+ RTS_head_athome:\n");

	if (dev != NULL)
	{
		/* if returns SANE_TRUE, lamp is at home. Otherwise it returns SANE_FALSE */
		rst = 0;
		
		if ((dev != NULL)&&(Regs != NULL))
		{
			SANE_Byte data;
			if (RTS_ctl_read_byte(dev, 0xe96f, &data) == SANE_STATUS_GOOD)
			{
				Regs[0x16f] = _B0(data);
				rst = (data >> 6) & 1;
			}
		}

		rst = (rst == 1)? SANE_TRUE: SANE_FALSE;
	}

	DBG(DBG_FNC, "- RTS_head_athome: %s\n", (rst == SANE_TRUE)? "Yes": "No");

	return rst;
}

static SANE_Byte RTS_scan_running(st_device *dev, SANE_Byte *Regs)
{
	SANE_Byte rst;

	DBG(DBG_FNC, "+ RTS_scan_running:\n");

	rst = 0;
	
	if ((dev != NULL)&&(Regs != NULL))
	{
		SANE_Byte data;
		if (RTS_ctl_read_byte(dev, 0xe800, &data) == SANE_STATUS_GOOD)
		{
			Regs[0x00] = data;
			rst = (data >> 7) & 1;
		}
	}
	
	DBG(DBG_FNC, "- RTS_scan_running: %i\n", rst);

	return rst;
}

static SANE_Status RTS_scan_wait(st_device *dev, SANE_Int msecs)
{
	SANE_Status rst;

	DBG(DBG_FNC, "+ RTS_scan_wait(msecs=%i):\n", msecs);

	/* returns SANE_STATUS_GOOD if ok or timeout
	   returns SANE_STATUS_INVAL if fails */
	
	rst = SANE_STATUS_INVAL;

	if (dev != NULL)
	{
		SANE_Byte data;

		if (RTS_ctl_read_byte(dev, 0xe800, &data) == SANE_STATUS_GOOD)
		{
			long ticks = GetTickCount() + msecs;
			rst = SANE_STATUS_GOOD;
			while (((data & 0x80) != 0) && (ticks > GetTickCount()) && (rst == SANE_STATUS_GOOD))
				rst = RTS_ctl_read_byte(dev, 0xe800, &data);
		}
	}

	DBG(DBG_FNC, "- RTS_scan_wait: %i\n", rst);

	return rst;
}

static SANE_Status RTS_sensor_enable(st_device *dev, SANE_Byte *Regs, SANE_Int channels)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "+ RTS_sensor_enable(*Regs, arg2=%i):\n", channels);

	if (dev != NULL)
		if (RTS_ctl_read_buffer(dev, 0xe810, &Regs[0x10], 4) == SANE_STATUS_GOOD)
		{
			data_bitset(&Regs[0x10], 0xe0, channels);      /*xxx-----*/
			data_bitset(&Regs[0x13], 0x80, channels >> 3); /*x-------*/

			RTS_ctl_write_buffer(dev, 0xe810, &Regs[0x10], 4);
			rst = SANE_STATUS_GOOD;
		}

	DBG(DBG_FNC, "- RTS_sensor_enable: %i\n", rst);

	return rst;
}

static SANE_Status RTS_lamp_warmup_reset(st_device *dev)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "+ RTS_lamp_warmup_reset:\n");

	if (dev != NULL)
	{
		SANE_Byte data;

		if (RTS_ctl_read_byte(dev, 0xe800, &data) == SANE_STATUS_GOOD)
		{
			data = (data & 0x3f) | 0x40; /*01------*/
			if (RTS_ctl_write_byte(dev, 0xe800, data) == SANE_STATUS_GOOD)
			{
				data &= 0xbf; /*-0------*/
				rst = RTS_ctl_write_byte(dev, 0xe800, data);
			}
		}
	}
	
	DBG(DBG_FNC, "- RTS_lamp_warmup_reset: %i\n", rst);

	return rst;
}

static SANE_Status RTS_lamp_status_timer_set(st_device *dev, SANE_Int minutes)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "+ RTS_lamp_status_timer_set(minutes=%i):\n", minutes);

	if (dev != NULL)
	{
		SANE_Byte MyBuffer[2];

		MyBuffer[0] = dev->init_regs[0x0146] & 0xef;
		MyBuffer[1] = dev->init_regs[0x0147];
		
		if (minutes > 0)
		{
			double ret, op2;

			minutes = _B0(minutes);
			op2  = 2.682163611980331;
			MyBuffer[0x00] |= 0x10;
			ret = (minutes * op2);
			MyBuffer[0x01] = (SANE_Byte) floor(ret);
		}
		
		dev->init_regs[0x147] = MyBuffer[1];
		dev->init_regs[0x146] = (dev->init_regs[0x146] & 0xef) | (MyBuffer[0] & 0x10);

		rst = RTS_ctl_write_word(dev, 0xe946, (SANE_Int)((MyBuffer[1] << 8) + MyBuffer[0]));
	}

	DBG(DBG_FNC, "- RTS_lamp_status_timer_set: %i\n", rst);

	return rst;
}

static SANE_Status RTS_btn_enable(st_device *dev)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "+ RTS_btn_enable:\n");

	if (dev != NULL)
	{
		SANE_Int data;

		if (RTS_ctl_read_word(dev, 0xe958, &data) == SANE_STATUS_GOOD)
		{
			data |= 0x0f;
			rst = RTS_ctl_write_word(dev, 0xe958, data);
		}
	}

	DBG(DBG_FNC, "- RTS_btn_enable: %i\n", rst);

	return rst;
}

static SANE_Int RTS_btn_count(st_device *dev)
{
	SANE_Int rst = 0;

	/* This chipset supports up to six buttons */

	if (dev != NULL)
		if (dev->buttons != NULL)
			rst = dev->buttons->count;

	DBG(DBG_FNC, "> RTS_btn_count: %i\n", rst);

	return rst;
}

static SANE_Int RTS_btn_status(st_device *dev)
{
	SANE_Int rst = -1;
	SANE_Byte data;

	DBG(DBG_FNC, "+ RTS_btn_status\n");

	/* Each bit is 1 if button is not pressed, and 0 if it is pressed
	   This chipset supports up to six buttons */

	if (dev != NULL)
		if (RTS_ctl_read_byte(dev, 0xe968, &data) == SANE_STATUS_GOOD)
			rst = _B0(data);

	DBG(DBG_FNC, "- RTS_btn_status: %i\n", rst);

	return rst;
}

static SANE_Int RTS_btn_released(st_device *dev)
{
	SANE_Int rst = -1;
	SANE_Byte data;

	DBG(DBG_FNC, "+ RTS_btn_released\n");

	/* Each bit is 1 if button is released, until reading this register. Then
	   entire register is cleared. This chipset supports up to six buttons */

	if (dev != NULL)
		if (RTS_ctl_read_byte(dev, 0xe96a, &data) == SANE_STATUS_GOOD)
			rst = _B0(data);

	DBG(DBG_FNC, "- RTS_btn_released: %i\n", rst);

	return rst;
}

static SANE_Int RTS_btn_order(st_device *dev, SANE_Int mask)
{
	/* this is a way to order each button according to its bit in register 0xe968 */
	SANE_Int rst = -1;

	if (dev != NULL)
		if (dev->buttons != NULL)
		{
			SANE_Int a;

			for (a = 0; a < 6; a++)
			{
				if (dev->buttons->mask[a][0] == mask)
				{
					rst = a;
					break;
				}
			}
		}

	return rst;
}

static SANE_Int RTS_btn_name(st_device *dev, SANE_Int mask)
{
	/* this function returns the button name */
	SANE_Int rst = -1;

	if (dev != NULL)
	{
		if ((rst = RTS_btn_order(dev, mask)) != -1)
			rst = dev->buttons->mask[rst][1];
	}

	return rst;
}

static SANE_Status RTS_gnoff_clear(st_device *dev)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "+ RTS_gnoff_clear:\n");

	if (dev != NULL)
	{
		/* clear offsets */
		dev->preview->offset[CL_RED] = dev->preview->offset[CL_GREEN] = dev->preview->offset[CL_BLUE] = 0;

		/* save offsets */
		/* check if chipset supports accessing eeprom */
		if ((dev->chipset->capabilities & CAP_EEPROM) != 0)
		{
			SANE_Int a;

			for (a = CL_RED; a <= CL_BLUE; a++)
				RTS_nvram_write_word(dev, 0x70 + (2 * a), 0);

			/* update checksum */
			rst = RTS_nvram_write_byte(dev, 0x76, 0);
		} else rst = SANE_STATUS_GOOD;
	}

	DBG(DBG_FNC, "- RTS_gnoff_clear: %i\n", rst);

	return rst;
}

static SANE_Status RTS_lamp_status_get(st_device *dev, SANE_Byte *flb_lamp, SANE_Byte *tma_lamp)
{
	/* The only reason that I think windows driver uses two variables to get
	   which lamp is switched on instead of one variable is that some chipset
	   model could have both lamps switched on, so I'm maintaining such var */

	SANE_Status rst = SANE_STATUS_INVAL; /* default */

	DBG(DBG_FNC, "+ RTS_lamp_status_get:\n");

	if ((dev != NULL) && (flb_lamp != NULL) && (tma_lamp != NULL))
	{
		SANE_Int data1;
		SANE_Byte data2;

		if (RTS_ctl_read_byte(dev, 0xe946, &data2) == SANE_STATUS_GOOD)
		{
			if (RTS_ctl_read_word(dev, 0xe954, &data1) == SANE_STATUS_GOOD)
			{
				*flb_lamp = 0;
				*tma_lamp = 0;

				switch(dev->chipset->model)
				{
					case RTS8822BL_03A:
						*flb_lamp = ((data2 & 0x40) != 0)? 1 : 0;
						*tma_lamp = (((data2 & 0x20) != 0)&&((data1 & 0x10)== 1))? 1: 0;
						break;
					default:
						if ((_B1(data1) & 0x10) == 0)
							*flb_lamp = (data2 >> 6) & 1;
								else *tma_lamp = (data2 >> 6) & 1;
						break;
				}

				rst = SANE_STATUS_GOOD;

				DBG(DBG_FNC, " -> flb=%i tma=%i\n", *flb_lamp, *tma_lamp);
			}
		}
	}

	DBG(DBG_FNC, "- RTS_lamp_status_get: rst=%i\n", rst);

	return rst;
}

static SANE_Status RTS_dma_wait(st_device *dev, SANE_Int msecs)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "+ RTS_dma_wait(msecs=%i):\n", msecs);

	if (dev != NULL)
	{
		SANE_Byte data;
		long mytime;

		rst = SANE_STATUS_GOOD;

		mytime = GetTickCount() + msecs;

		while ((mytime > GetTickCount()) && (rst == SANE_STATUS_GOOD))
		{
			if (RTS_ctl_read_byte(dev, 0xef09, &data) == SANE_STATUS_GOOD)
			{
				if ((data & 1) == 0)
					usleep(1000 * 100);
						else break;
			} else rst = SANE_STATUS_INVAL;
		}
	}

	DBG(DBG_FNC, "- RTS_dma_wait: %i\n", rst);

	return rst;
}

static SANE_Status RTS_WaitInitEnd(st_device *dev, SANE_Int msecs)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "+ RTS_WaitInitEnd(msecs=%i):\n", msecs);

	if (dev != NULL)
	{
		SANE_Byte data;
		long mytime;

		rst = SANE_STATUS_GOOD;

		mytime = GetTickCount() + msecs;
		
		while ((mytime > GetTickCount()) && (rst == SANE_STATUS_GOOD))
		{
			if (RTS_ctl_read_byte(dev, 0xf910, &data) == SANE_STATUS_GOOD)
			{
				if ((data & 8) == 0)
					usleep(1000 * 100);
						else break;
			} else rst = SANE_STATUS_INVAL;
		}
	}

	DBG(DBG_FNC, "- RTS_WaitInitEnd: %i\n", rst);

	return rst;
}

static SANE_Status RTS_mtr_change(st_device *dev, SANE_Byte *Regs, SANE_Byte value)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "+ RTS_mtr_change(value=%i):\n", value);

	if ((dev != NULL)&&(Regs != NULL))
	{
		SANE_Int data;

		if (RTS_ctl_read_word(dev, 0xe954, &data) == SANE_STATUS_GOOD)
		{
			data &= 0xcf; /*--00----*/
			value--;
			switch(value)
			{
				case 2: data |= 0x30; break; /*--11----*/
				case 1: data |= 0x20; break; /*--10----*/
				case 0: data |= 0x10; break; /*--01----*/
			}

			Regs[0x154] = data;

			rst = RTS_ctl_write_byte(dev, 0xe954, Regs[0x154]);
		}
	}

	DBG(DBG_FNC, "- RTS_mtr_change: %i\n", rst);

	return rst;
}

static SANE_Status RTS_dma_read(st_device *dev, SANE_Int dmacs, SANE_Int options, SANE_Int size, SANE_Byte *buffer)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "+ RTS_dma_read(dmacs=%04x, options=%04x, size=%i.):\n", dmacs, options, size);

	/* is there any buffer to send? */
	if ((dev != NULL) && (buffer != NULL) && (size > 0))
	{
		/* reset dma */
		if (RTS_dma_reset(dev) == SANE_STATUS_GOOD)
		{
			/* prepare dma to read */
			if (RTS_dma_read_enable(dev, dmacs, size, options) == SANE_STATUS_GOOD)
			{
				SANE_Int transferred;

				rst = RTS_blk_read(dev, size, buffer, &transferred);
			}
		}
	}

	DBG(DBG_FNC, "- RTS_dma_read(): %i\n", rst);

	return rst;
}

static SANE_Status RTS_dma_write(st_device *dev, SANE_Int dmacs, SANE_Int segment, SANE_Int size, SANE_Byte *buffer)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "+ RTS_dma_write(dmacs=%04x, segment=%04x, size=%i.):\n", dmacs, segment, size);

	/* is there any buffer to send? */
	if ((dev != NULL) && (buffer != NULL)&&(size > 0))
	{
		/* reset dma */
		if (RTS_dma_reset(dev) == SANE_STATUS_GOOD)
		{
			/* prepare dma to write */
			if (RTS_dma_write_enable(dev, dmacs, size, segment) == SANE_STATUS_GOOD)
			{
				SANE_Int transferred;
				SANE_Byte *check_buffer;

				check_buffer = (SANE_Byte *) malloc(size * sizeof(SANE_Byte));
				if (check_buffer != NULL)
				{
					/* if some transfer fails we try again until ten times */
					SANE_Int a;
					for (a = 10; a > 0; a--)
					{
						/* send buffer */
						RTS_blk_write(dev, size, buffer, &transferred);

						/* prepare dma to read */
						if (RTS_dma_read_enable(dev, dmacs, size, segment) == SANE_STATUS_GOOD)
						{
							SANE_Int b = 0, diff = SANE_FALSE;

							/* read buffer */
							RTS_blk_read(dev, size, check_buffer, &transferred);

							/* check buffers */
							while ((b < size)&&(diff == SANE_FALSE))
							{
								if (buffer[b] == check_buffer[b])
									b++;
										else diff = SANE_TRUE;
							}

							/* if buffers are equal we can break loop */
							if (diff == SANE_TRUE)
							{
								/* cancel dma */
								RTS_dma_cancel(dev);

								/* prepare dma to write buffer again */
								if (RTS_dma_write_enable(dev, dmacs, size, segment) != SANE_STATUS_GOOD)
									break;
							} else
							{
								/* everything went ok */
								rst = SANE_STATUS_GOOD;
								break;
							}
						} else break;
					}

					/* free check buffer */
					free(check_buffer);
				} else
				{
					/* for some reason it's not posible to allocate space to check
					   sent buffer so we just write data */
					RTS_blk_write(dev, size, buffer, &transferred);
					rst = SANE_STATUS_GOOD;
				}
			}
		}
	}

	DBG(DBG_FNC, "- RTS_dma_write(): %i\n", rst);

	return rst;
}

static SANE_Status RTS_dma_type_check(st_device *dev, SANE_Byte *Regs)
{
	/* This function tries to detect what kind of RAM supports chipset */

	#define BUFFER_LENGTH 2072

	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "+ RTS_dma_type_check(*Regs):\n");

	if ((dev != NULL) && (Regs != NULL))
	{
		SANE_Byte *out_buffer;

		/* define buffer to send */
		out_buffer = (SANE_Byte *) malloc(sizeof(SANE_Byte) * BUFFER_LENGTH);
		if (out_buffer != NULL)
		{
			SANE_Byte *in_buffer;

			/* define incoming buffer */
			in_buffer = (SANE_Byte *) malloc(sizeof(SANE_Byte) * BUFFER_LENGTH);
			if (in_buffer != NULL)
			{
				SANE_Int a, b, diff;

				/* fill outgoing buffer with a known pattern */
				b = 0;
				for (a = 0; a < BUFFER_LENGTH; a++)
				{
					out_buffer[a] = b;
					b++;
					if (b == 0x61)
						b = 0;
				}

				/* let's send buffer with different ram types and compare
				   incoming buffer until getting the right type */

				for (a = 4; a >= 0; a--)
				{
					/* set ram type */
					if (RTS_dma_type_set(dev, Regs, a) != SANE_STATUS_GOOD)
						break;

					/* wait 1500 miliseconds */
					if (RTS_dma_wait(dev, 1500) != SANE_STATUS_GOOD)
						break;

					/* reset dma */
					RTS_dma_reset(dev);

					/* write buffer */
					RTS_dma_write(dev, 0x0004, 0x102, BUFFER_LENGTH, out_buffer);

					/* now read buffer */
					RTS_dma_read(dev, 0x0004, 0x102, BUFFER_LENGTH, in_buffer);

					/* check buffers */
					diff = (memcmp(out_buffer, in_buffer, BUFFER_LENGTH) != 0)? SANE_TRUE : SANE_FALSE;

					/* if buffers are equal */
					if (diff == SANE_FALSE)
					{
						SANE_Int data = 0;

						/* buffers are equal so we've found the right ram type */
						memset(out_buffer, 0, 0x20);
						for (b = 0; b < 0x20; b += 2)
							out_buffer[b] = b;

						/* write buffer */
						if (RTS_dma_write(dev, 0x0004, 0x0000, 0x20, out_buffer) == SANE_STATUS_GOOD)
						{
							SANE_Int c = 0;
							diff = SANE_TRUE;

							do
							{
								c++;
								for (b = 1; b < 0x20; b += 2)
									out_buffer[b] = c;

								if (RTS_dma_write(dev, 0x0004, (_B0(c) << 0x11) >> 0x04, 0x20, out_buffer) == SANE_STATUS_GOOD)
								{
									if (RTS_dma_read(dev, 0x0004, 0x0000, 0x20, in_buffer) == SANE_STATUS_GOOD)
									{
										/* compare both buffers */
										diff = (memcmp(out_buffer, in_buffer, 0x20) != 0)? SANE_TRUE : SANE_FALSE;
										if (diff == SANE_FALSE)
											data = c << 7;
									}
								}
							} while ((c < 0x80)&&(diff == SANE_TRUE));
						}

						switch (data)
						{
							case 16384:
								Regs[0x708] &= 0x1f;
								Regs[0x708] |= 0x80;
								break;
							case 8192:
								Regs[0x708] &= 0x1f;
								Regs[0x708] |= 0x60;
								break;
							case 4096:
								Regs[0x708] &= 0x1f;
								Regs[0x708] |= 0x40;
								break;
							case 2048:
								Regs[0x708] &= 0x1f;
								Regs[0x708] |= 0x20;
								break;
							case 1024:
								Regs[0x708] &= 0x1f;
								data = 0x200;
								break;
							case 128:
								Regs[0x708] &= 0x1f;
								break;
						}

						DBG(DBG_FNC, " -> data1 = 0x%08x\n", (data * 4) * 1024);
						DBG(DBG_FNC, " -> data2 = 0x%08x\n", data * 1024);
						DBG(DBG_FNC, " -> type  = 0x%04x\n", Regs[0x708] >> 5);

						RTS_dma_type_set(dev, Regs, Regs[0x708] >> 5);

						rst = SANE_STATUS_GOOD;
						break;
					}
				}

				free(in_buffer);
			}

			free(out_buffer);
		}
	}

	DBG(DBG_FNC, "- RTS_dma_type_check(): %i\n", rst);

	return rst;
}

static SANE_Status RTS_dma_type_set(st_device *dev, SANE_Byte *Regs, SANE_Byte ramtype)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "+ RTS_dma_type_set(*Regs, ramtype=%i):\n", ramtype);

	if ((dev != NULL) && (Regs != NULL))
	{
		data_bitset(&Regs[0x708], 0x08, 0); /*----0---*/

		if (RTS_ctl_write_byte(dev, 0xef08, Regs[0x708]) == SANE_STATUS_GOOD)
		{
			data_bitset(&Regs[0x708], 0xe0, ramtype);

			if (RTS_ctl_write_byte(dev, 0xef08, Regs[0x708]) == SANE_STATUS_GOOD)
			{
				data_bitset(&Regs[0x708], 0x08, 1); /*----1---*/
				rst = RTS_ctl_write_byte(dev, 0xef08, Regs[0x708]);
			}
		}
	}

	DBG(DBG_FNC, "- RTS_dma_type_set: %i\n", rst);

	return rst;
}

static void RTS_mtr_release(st_device *dev)
{
	DBG(DBG_FNC, "+ RTS_mtr_release:\n");

	if (dev != NULL)
	{
		SANE_Byte data = 0;

		if (RTS_ctl_read_byte(dev, 0xe8d9, &data) == SANE_STATUS_GOOD)
		{
			data |= 4;
			RTS_ctl_write_byte(dev, 0xe8d9, data);
		}
	}

	DBG(DBG_FNC, "- RTS_mtr_release:\n");
}

static SANE_Byte RTS_gnoff_counter_load(st_device *dev)
{
	SANE_Byte data = 0x0f;

	DBG(DBG_FNC, "+ RTS_gnoff_counter_load:\n");

	if (dev != NULL)
	{
		/* check if chipset supports accessing eeprom */
		if ((dev->chipset->capabilities & CAP_EEPROM) != 0)
			if (RTS_nvram_read_byte(dev, 0x77, &data) != SANE_STATUS_GOOD)
				data = 0x0f;
	}

	DBG(DBG_FNC, "- RTS_gnoff_counter_load: %i\n", data);

	return data;
}

static SANE_Status RTS_scan_run(st_device *dev)
{
	SANE_Status ret = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "+ RTS_scan_run:\n");

	if (dev != NULL)
	{
		SANE_Byte e813, e800;

		e813 = 0;
		e800 = 0;

		if (RTS_ctl_read_byte(dev, 0xe800, &e800) == SANE_STATUS_GOOD)
		{
			if (RTS_ctl_read_byte(dev, 0xe813, &e813) == SANE_STATUS_GOOD)
			{
				e813 &= 0xbf;
				if (RTS_ctl_write_byte(dev, 0xe813, e813) == SANE_STATUS_GOOD)
				{
					e800 |= 0x40;
					if (RTS_ctl_write_byte(dev, 0xe800, e800) == SANE_STATUS_GOOD)
					{
						e813 |= 0x40;
						if (RTS_ctl_write_byte(dev, 0xe813, e813) == SANE_STATUS_GOOD)
						{
							e800 &= 0xbf;
							if (RTS_ctl_write_byte(dev, 0xe800, e800) == SANE_STATUS_GOOD)
							{
								usleep(1000 * 100);
								e800 |= 0x80;
								ret = RTS_ctl_write_byte(dev, 0xe800, e800);
							}
						}
					}
				}
			}
		}
	}

	DBG(DBG_FNC, "- RTS_scan_run: %i\n", ret);

	return ret;
}

static SANE_Int RTS_isTmaAttached(st_device *dev)
{
	SANE_Int rst = SANE_TRUE;

	DBG(DBG_FNC, "+ RTS_isTmaAttached:\n");

	/* returns 0 if Tma is attached. Otherwise 1 */
	if (dev != NULL)
		if (RTS_ctl_read_word(dev, 0xe968, &rst) == SANE_STATUS_GOOD)
		{
			rst = ((_B1(rst) & 2) != 0)? SANE_FALSE : SANE_TRUE;
		}

	DBG(DBG_FNC, "- RTS_isTmaAttached: %s\n", (rst == SANE_TRUE)? "Yes": "No");

	return rst;
}

static void RTS_gamma_free(st_device *dev)
{
	if (dev != NULL)
	{
		SANE_Int c;

		for (c = CL_RED; c <= CL_BLUE; c++)
		{
			wfree (dev->gamma->table[c]);
			dev->gamma->table[c] = NULL;
		}
	}
}

static void RTS_gamma_alloc(st_device *dev, SANE_Int colour, SANE_Int depth, SANE_Byte *table)
{
	DBG(DBG_FNC, "> RTS_gamma_alloc(colour=%i, depth=%i):\n", colour, depth);

	if (dev != NULL)
	{
		SANE_Int dst_depth = RTS_gamma_depth_get(dev);

		/* configured depth must be valid */
		if (dst_depth != -1)
		{
			SANE_Byte *tb = NULL;
			SANE_Int a, items, size, mask;

			items = 1 << depth;
			size  = ((items * depth) + 7) / 8;
			mask  = (1 << depth) - 1;

			/* we need some table to work with */
			if ((tb = (SANE_Byte *) malloc (size * sizeof(SANE_Byte))) != NULL)
			{
				/* assign retrieved values or default ones */
				if (table == NULL)
				{
					/* default value for table */
					SANE_Int byte, rbits;

					for (a = 0; a < items; a++)
					{
						byte = (a * depth) / 8;
						rbits = (a * depth) % 8;

						data_wide_bitset(tb + byte, mask << rbits, a);
					}
				} else memcpy(tb, table, size * sizeof(SANE_Byte));

				/* table must be in the same depth as configured in scanner */
				if (depth != dst_depth)
				{
					SANE_Byte *conv = RTS_gamma_depth_conv(tb, depth, dev->options->gamma_depth);
					if (conv != NULL)
					{
						/* assign new table */
						free(tb);
						tb = conv;
						depth = dst_depth;

						/* config has changed */
						items = 1 << depth;
						size  = ((items * depth) + 7) / 8;
						mask  = (1 << depth) - 1;
					} else
					{
						free(tb);
						tb = NULL;
					}
				}

				if (tb != NULL)
				{
					/* assign table */
					for (a = CL_RED; a <= CL_BLUE; a++)
					{
						if ((colour == a)||(colour == -1))
						{
							SANE_Byte *ctb = NULL;

							/* delete previous table */
							wfree(dev->gamma->table[a]);
							dev->gamma->table[a] = NULL;

							/* create table to assign */
							if ((ctb = (SANE_Byte *) malloc (size * sizeof(SANE_Byte))) != NULL)
							{
								memcpy(ctb, tb, size * sizeof(SANE_Byte));
								dev->gamma->table[a] = ctb;

								if (a == CL_RED)
								{
									SANE_Int b;

									/* locate threshold of bw */
									for (b = 0; b < items; b++)
									{
										if (RTS_gamma_get(dev, CL_RED, b) != 0)
										{
											dev->gamma->bw_threshold = b - 1;
											break;
										}
									}
								}
							}
						}
					}

					free(tb);
				}
			}
		} else DBG(DBG_FNC, " -> RTS_gamma_alloc: depth is not properly configured\n");
	}
}

static SANE_Int RTS_gamma_depth_get(st_device *dev)
{
	SANE_Int rst = -1; /* default */

	if (dev != NULL)
	{
		switch (dev->options->gamma_depth & 0x0c)
		{
			case GAMMA_8BIT : rst =  8; break; /*  8 bits per channel */
			case GAMMA_10BIT: rst = 10; break; /* 10 bits per channel */
			case GAMMA_12BIT: rst = 12; break; /* 12 bits per channel */
			default         : rst = -1; break;
		}
	}

	return rst;
}

static SANE_Status RTS_gamma_depth_set(st_device *dev, SANE_Int depth, SANE_Int conv)
{
	/* It's not necessary gamma tables to exist when setting depth. If tables are allocated
	   they will be converted to selected depth (unless conv is FALSE). If conversion must
	   be performed and it fails, gamma tables will be released */

	SANE_Status rst = SANE_STATUS_INVAL; /* default */

	if (dev != NULL)
	{
		SANE_Int dp;

		switch (depth & 0x0c)
		{
			case GAMMA_8BIT : dp =  8; break; /*  8 bits per channel */
			case GAMMA_10BIT: dp = 10; break; /* 10 bits per channel */
			case GAMMA_12BIT: dp = 12; break; /* 12 bits per channel */
			default         : dp = -1; break;
		}

		if (dp != -1)
		{
			SANE_Int cur_depth = RTS_gamma_depth_get(dev);

			/* set depth */
			dev->gamma->depth = depth;

			/* this operation succedes by default */
			rst = SANE_STATUS_GOOD;

			/* gamma depth is already set? */
			if (cur_depth != -1)
			{
				/* if depth is equal to the one configured, skip process */
				if (cur_depth != dp)
				{
					/* perform conversion ? */
					if (conv != SANE_FALSE)
					{
						SANE_Int a;

						for (a = CL_RED; a <= CL_BLUE; a++)
						{
							if (dev->gamma->table[a] != NULL)
							{
								SANE_Byte *tb = RTS_gamma_depth_conv(dev->gamma->table[a], cur_depth, depth);

								if (tb != NULL)
								{
									free(dev->gamma->table[a]);
									dev->gamma->table[a] = tb;
								} else
								{
									/* there is a problem converting table */
									rst = SANE_STATUS_INVAL;
									break;
								}
							}
						}
					}

					/* if there is an SANE_STATUS_INVAL during conversion or conversion is disabled,
					   free current gamma tables due to their depth is not the right one */
					if ((rst == SANE_STATUS_INVAL)||(conv == SANE_FALSE))
						RTS_gamma_free(dev);
				}
			}
		}
	}

	return rst;
}

/* get a gamma value whatever its depth is */
static SANE_Int RTS_gamma_get(st_device *dev, SANE_Int colour, SANE_Int item)
{
	SANE_Int rst = 0;

	if (dev != NULL)
	{
		/* colour is right ? */
		if ((colour >= CL_RED) && (colour <= CL_BLUE))
		{
			/* does table exist ? */
			if (dev->gamma->table[colour] != NULL)
			{
				/* is current depth properly set ? */
				SANE_Int depth = RTS_gamma_depth_get(dev);

				if (depth != -1)
				{
					/* is item in depth range ? */
					if (item < (1 << depth))
					{
						SANE_Int mask, bitpos, byte, rbits;

						mask = (1 << depth) - 1;
						bitpos = depth * item;
						byte   = bitpos / 8;
						rbits  = bitpos % 8;

						rst = data_wide_bitget(dev->gamma->table[colour] + byte, mask << rbits);
					}
				}
			}
		}
	}

	return rst;
}

static SANE_Status RTS_gamma_apply(st_device *dev, SANE_Byte *Regs, struct st_scanparams *scancfg, struct st_hwdconfig *hwdcfg)
{
	SANE_Status rst = SANE_STATUS_INVAL; /* default */

	DBG(DBG_FNC, "+ RTS_gamma_apply:\n");

	if ((dev != NULL) && (scancfg != NULL) && (hwdcfg != NULL))
	{
		dbg_ScanParams(scancfg);

		if (hwdcfg->use_gamma != SANE_FALSE)
		{
			SANE_Int depth, extra, items;
			SANE_Int table_size, buffersize, c;
			SANE_Byte channels, *gammabuffer;

			/* get channels count */
			channels = 3; /* default */

			if (scancfg->colormode != CM_COLOR)
			{
				if (scancfg->channel != 3)
				{
					if (scancfg->colormode != 3)
						channels = (scancfg->samplerate == PIXEL_RATE)? 2:1;
				}
			}

			extra = dev->options->gamma_depth & 1;

			if ((depth = RTS_gamma_depth_get(dev)) != -1)
			{
				items = (1 << depth);
				table_size = (((items + extra) * depth) + 7)/ 8;

				/* allocate space for gamma buffer */
				buffersize = table_size * channels;

				if ((gammabuffer = (SANE_Byte *) calloc(1, buffersize * sizeof(SANE_Byte))) != NULL)
				{
					/* include gamma tables into one buffer */
					for (c = 0; c < channels; c++)
					{
						if (dev->gamma->table[c] != NULL)
							memcpy(gammabuffer + (c * table_size), dev->gamma->table[c], table_size * sizeof(SANE_Byte));
								else DBG(DBG_FNC, "-> Gamma table[%i] is null\n", c);
					}

					/* send gamma buffer to scanner */
					RTS_ctl_write_byte(dev, 0xee0b, Regs[0x060b] & 0xaf);
					rst = RTS_gamma_write(dev, Regs, gammabuffer, buffersize);
					RTS_ctl_write_byte(dev, 0xee0b, Regs[0x060b]);

					/* free gamma buffer */
					free(gammabuffer);

					rst = SANE_STATUS_GOOD;
				}
			}
		} else
		{
			DBG(DBG_FNC, "-> Gamma tables are not used\n");
			rst = SANE_STATUS_GOOD;
		}
	}

	DBG(DBG_FNC, "- RTS_gamma_apply: %i\n", rst);

	return rst;
}

static SANE_Status RTS_refs_analyze(st_device *dev, struct st_scanparams *scancfg, SANE_Byte *scanned_pattern, SANE_Int *ler1, SANE_Int ler1order, SANE_Int *ser1, SANE_Int ser1order)
{
	SANE_Status rst = SANE_STATUS_INVAL; /* default */
	SANE_Int buffersize, xpos, ypos, coord, cnt, chn_size, dist;
	double *color_sum, *color_dif, diff_max;
	SANE_Int vector[3];

	if (dev == NULL || scancfg == NULL || scanned_pattern == NULL)
	{
		DBG(DBG_FNC, "> RTS_refs_analyze: ASSERT failed\n");
		return rst;
	}

	DBG(DBG_FNC, "+ RTS_refs_analyze(depth=%i, width=%i, height=%i, *scanned_pattern, *ler1, ler1order=%i, *ser1, ser1order=%i)\n",
			scancfg->depth, scancfg->coord.width, scancfg->coord.height, ler1order, ser1order);

	dist       = 5;  /* distance to compare */
	chn_size   = (scancfg->depth + 7) / 8;
	buffersize = max(scancfg->coord.width, scancfg->coord.height);

	color_sum = (double *) malloc(sizeof(double) * buffersize);
	color_dif = (double *) malloc(sizeof(double) * buffersize);
	
	if (color_sum == NULL || color_dif == NULL)
	{
		wfree(color_sum);
		wfree(color_dif);
		
		return rst;
	}

	/*-------- 1st SER -------- */
	coord = 1;

	if ((scancfg->coord.width - dist) > 1)
	{
		/* clear buffers */
		memset(color_sum, 0, sizeof(double) * buffersize);
		memset(color_dif, 0, sizeof(double) * buffersize);

		for (xpos = 0; xpos < scancfg->coord.width; xpos++)
		{
			for (ypos = 0; ypos < 20; ypos++)
				color_sum[xpos] += data_lsb_get(scanned_pattern + (scancfg->coord.width * ypos) + xpos, chn_size);
		}

		diff_max = (ser1order != 0)? color_sum[0] - color_sum[1]: color_sum[1] - color_sum[0];
		color_dif[0] = diff_max;
		cnt = 1;

		do
		{
			color_dif[cnt] = (ser1order != 0)? color_sum[cnt] - color_sum[cnt + dist]: color_sum[cnt + dist] - color_sum[cnt];

			if ((color_dif[cnt] >= 0) && (color_dif[cnt] > diff_max))
			{
				/*d4df*/
				diff_max  = color_dif[cnt];
				if (abs(color_dif[cnt] - color_dif[cnt -1]) > abs(color_dif[coord] - color_dif[coord - 1]))
					coord = cnt;
			}

			cnt++;
		} while (cnt < (scancfg->coord.width - dist));
	}

	vector[0] = coord + dist;

	/*-------- 1st LER -------- */
	coord = 1;

	if ((scancfg->coord.height - dist) > 1)
	{
		/* clear buffers */
		memset(color_sum, 0, sizeof(double) * buffersize);
		memset(color_dif, 0, sizeof(double) * buffersize);

		for (ypos = 0; ypos < scancfg->coord.height; ypos++)
		{
			for (xpos = vector[0]; xpos < scancfg->coord.width - dist; xpos++)
				color_sum[ypos] += data_lsb_get(scanned_pattern + (scancfg->coord.width * ypos) + xpos, chn_size);
		}

		diff_max = (ler1order != 0)? color_sum[0] - color_sum[1] : color_sum[1] - color_sum[0];
		color_dif[0] = diff_max;

		cnt = 1;

		do
		{
			color_dif[cnt] = (ler1order != 0)? color_sum[cnt] - color_sum[cnt + dist]: color_sum[cnt + dist] - color_sum[cnt];

			if ((color_dif[cnt] >= 0) && (color_dif[cnt] > diff_max))
			{
				diff_max  = color_dif[cnt];
				if (abs(color_dif[cnt] - color_dif[cnt -1]) > abs(color_dif[coord] - color_dif[coord - 1]))
					coord = cnt;
			}

			cnt++;
		} while (cnt < (scancfg->coord.height - dist));
	}

	vector[1] = coord + dist;

	/*-------- 1st LER -------- */
	if ((scancfg->coord.width - dist) > 1)
	{
		/* clear buffers */
		memset(color_sum, 0, sizeof(double) * buffersize);
		memset(color_dif, 0, sizeof(double) * buffersize);

		for (xpos = 0; xpos < scancfg->coord.width; xpos++)
		{
			for (ypos = coord + 4; ypos < scancfg->coord.height; ypos++)
				color_sum[xpos] += data_lsb_get(scanned_pattern + (scancfg->coord.width * ypos) + xpos, chn_size);
		}

		diff_max = (ser1order != 0)? color_sum[0] - color_sum[1] : color_sum[1] - color_sum[0];
		color_dif[0] = diff_max;
		cnt = 1;

		do
		{
			color_dif[cnt] = (ser1order != 0)? color_sum[cnt] - color_sum[cnt + dist]: color_sum[cnt + dist] - color_sum[cnt];

			if ((color_dif[cnt] >= 0)&&(color_dif[cnt] > diff_max))
			{
				diff_max  = color_dif[cnt];
				if (abs(color_dif[cnt] - color_dif[cnt -1]) > abs(color_dif[coord] - color_dif[coord - 1]))
					coord = cnt;
			}

			cnt++;
		} while (cnt < (scancfg->coord.width - dist));
	}

	vector[2] = coord + dist;

	/* save image */
	if (dev->options->dbg_image != SANE_FALSE)
		dbg_autoref(scancfg, scanned_pattern, vector[0], vector[2], vector[1]);

	/* assign values detected */
	if (ser1 != NULL)
		*ser1 = vector[2];

	if (ler1 != NULL)
		*ler1 = vector[1];

	/* show values */
	DBG(DBG_FNC, " -> Vectors found: x1=%i, x2=%i, y=%i\n", vector[0], vector[2], vector[1]);

	rst = SANE_STATUS_GOOD;

	free(color_dif);
	free(color_sum);

	DBG(DBG_FNC, "- RTS_refs_analyze: %i\n", rst);

	return rst;
}

static double get_shrd(double value, SANE_Int desp)
{
	return (desp <= 0x40)? value / pow(2, desp) : 0;
}

static char get_byte(double value)
{
	unsigned int data;
	double temp;

	if (value > 0xffffffff)
	{
		temp = floor(get_shrd(value, 0x20));
		temp *= pow(2, 32);
		value -= temp;
	}

	data = (unsigned int) value;

	data = _B0(data);

	return data;
}

static SANE_Status RTS_sensor_clock_set(SANE_Byte *Regs, struct st_cph *cph)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "+ RTS_sensor_clock_set(SANE_Byte *Regs, struct st_cph *cph)\n");

	dbg_sensorclock(cph);

	if ((Regs != NULL)&&(cph != NULL))
	{
		Regs[0x00] = get_byte(cph->p1);
		Regs[0x01] = get_byte(get_shrd(cph->p1, 0x08));
		Regs[0x02] = get_byte(get_shrd(cph->p1, 0x10));
		Regs[0x03] = get_byte(get_shrd(cph->p1, 0x18));

		Regs[0x04] &= 0x80;
		Regs[0x04] |= ((get_byte(get_shrd(cph->p1, 0x20))) & 0x0f);
		Regs[0x04] |= ((cph->ps & 1) << 6);
		Regs[0x04] |= ((cph->ge & 1) << 5);
		Regs[0x04] |= ((cph->go & 1) << 4);

		Regs[0x05] = get_byte(cph->p2);
		Regs[0x06] = get_byte(get_shrd(cph->p2, 0x08));
		Regs[0x07] = get_byte(get_shrd(cph->p2, 0x10));
		Regs[0x08] = get_byte(get_shrd(cph->p2, 0x18));
		Regs[0x09] &= 0xf0;
		Regs[0x09] |= ((get_byte(get_shrd(cph->p2, 0x20))) & 0x0f);

		rst = SANE_STATUS_GOOD;
	}

	DBG(DBG_FNC, "- RTS_sensor_clock_set: %i\n", rst);

	return rst;
}

static void RTS_setup_sensor_clocks(st_device *dev, SANE_Int mytiming, SANE_Byte *Regs)
{
	DBG(DBG_FNC, "+ RTS_setup_sensor_clocks(mytiming=%i, *Regs):\n", mytiming);

	if (dev != NULL)
		if ((Regs != NULL)&&(mytiming < dev->timings_count))
		{
			struct st_timing *mt = dev->timings[mytiming];

			if (mt != NULL)
			{
				dbg_timing(mt);

				/* Correlated-Double-Sample 1 & 2 */
				data_bitset(&Regs[0x92], 0x3f, mt->cdss[0]);
				data_bitset(&Regs[0x93], 0x3f, mt->cdsc[0]);
				data_bitset(&Regs[0x94], 0x3f, mt->cdss[1]);
				data_bitset(&Regs[0x95], 0x3f, mt->cdsc[1]);

				data_bitset(&Regs[0x96], 0x3f, mt->cnpp);

				/* Linear image sensor transfer gates */
				data_bitset(&Regs[0x45], 0x80, mt->cvtrp[0]);
				data_bitset(&Regs[0x45], 0x40, mt->cvtrp[1]);
				data_bitset(&Regs[0x45], 0x20, mt->cvtrp[2]);

				data_bitset(&Regs[0x45], 0x1f, mt->cvtrfpw);
				data_bitset(&Regs[0x46], 0x1f, mt->cvtrbpw);

				data_lsb_set(&Regs[0x47], mt->cvtrw, 1);

				data_lsb_set(&Regs[0x84], mt->cphbp2s, 3);
				data_lsb_set(&Regs[0x87], mt->cphbp2e, 3);

				data_lsb_set(&Regs[0x8a], mt->clamps, 3);
				data_lsb_set(&Regs[0x8d], mt->clampe, 3);

				if (dev->chipset->model == RTS8822L_02A)
				{
					if (mt->clampe == -1)
						data_lsb_set(&Regs[0x8d], mt->cphbp2e, 3);
				}

				Regs[0x97] = get_byte(mt->adcclkp[0]);
				Regs[0x98] = get_byte(get_shrd(mt->adcclkp[0], 0x08));
				Regs[0x99] = get_byte(get_shrd(mt->adcclkp[0], 0x10));
				Regs[0x9a] = get_byte(get_shrd(mt->adcclkp[0], 0x18));
				Regs[0x9b] &= 0xf0;
				Regs[0x9b] |= ((get_byte(get_shrd(mt->adcclkp[0], 0x20))) & 0x0f);
				
				Regs[0xc1] = get_byte(mt->adcclkp[1]);
				Regs[0xc2] = get_byte(get_shrd(mt->adcclkp[1], 0x08));
				Regs[0xc3] = get_byte(get_shrd(mt->adcclkp[1], 0x10));
				Regs[0xc4] = get_byte(get_shrd(mt->adcclkp[1], 0x18));
				Regs[0xc5] &= 0xe0;
				Regs[0xc5] |= ((get_byte(get_shrd(mt->adcclkp[1], 0x20))) & 0x0f);

				/* bit(4) = bit(0) */
				Regs[0xc5] |= ((mt->adcclkp2e & 1) << 4);

				/* set linear image sensor clock*/
				RTS_sensor_clock_set(&Regs[0x48], &mt->cph[0]);
				RTS_sensor_clock_set(&Regs[0x52], &mt->cph[1]);
				RTS_sensor_clock_set(&Regs[0x5c], &mt->cph[2]);
				RTS_sensor_clock_set(&Regs[0x66], &mt->cph[3]);
				RTS_sensor_clock_set(&Regs[0x70], &mt->cph[4]);
				RTS_sensor_clock_set(&Regs[0x7a], &mt->cph[5]);
			}
		}
}

static SANE_Int RTS_mtr_get(st_device *dev, SANE_Int resolution)
{
	SANE_Int ret = 3;

	if (dev != NULL)
	{
		if (dev->usb->type != USB11)
		{
			if (scan.scantype != ST_NORMAL)
			{
				/* scantype is ST_NEG or ST_TA */
				if (resolution >= 600)
					ret = 0;
			} else if (resolution >= 1200)
				ret = 0;
		} else if (resolution >= 600)
			ret = 0;
	}

	DBG(DBG_FNC, "> RTS_mtr_get(resolution=%i): %i\n", resolution, ret);

	return ret;
}

static SANE_Status RTS_exposure_set(st_device *dev, SANE_Byte *Regs)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "> RTS_exposure_set:\n");

	if ((dev != NULL) && (Regs != NULL))
	{
		SANE_Int iValue, myctpc;

		/* set motor has no curves */
		data_bitset(&Regs[0xdf], 0x10, 0); /*---0----*/

		/* select case systemclock */
		switch(Regs[0x00] & 0x0f)
		{
			case 0x00: iValue = 0x00895440; break; /*  3 x 0x2DC6C0 */
			case 0x08:
			case 0x01: iValue = 0x00b71b00; break; /*  4 x 0x2DC6C0 */
			case 0x02: iValue = 0x0112a880; break; /*  6 x 0x2DC6C0 */
			case 0x0a:
			case 0x03: iValue = 0x016e3600; break; /*  8 x 0x2DC6C0 */
			case 0x04: iValue = 0x02255100; break; /* 12 x 0x2DC6C0 */
			case 0x0c: iValue = 0x02dc6c00; break; /* 16 x 0x2DC6C0 */
			case 0x05: iValue = 0x044aa200; break; /* 24 x 0x2DC6C0 */
			case 0x0d: iValue = 0x05b8d800; break; /* 32 x 0x2DC6C0 */

			case 0x09: iValue = 0x00f42400; break;
			case 0x0b: iValue = 0x01e84800; break; /* = case 9 * 2 */
			default  : iValue = 0x0478f7f8; break;
		}

		/* divide by timing.cnpp */
		iValue /= ((Regs[0x96] & 0x3f) + 1);
		iValue /= dev->motorcfg->basespeedpps;

		/* get line exposure time */
		myctpc = data_lsb_get(&Regs[0x30], 3) + 1;

		DBG(DBG_FNC, "CTPC -- RTS_exposure_set -- 1 =%i\n", myctpc);

		/* if last step of accurve.normalscan table is lower than iValue ...*/
		if (data_lsb_get(&Regs[0xe1], 3) < iValue)
		{
			SANE_Int traget;
			SANE_Int step_size = _B0(Regs[0xe0]) + 1;

			/* set exposure time [RED] if zero */
			if (data_lsb_get(&Regs[0x36], 3) == 0)
				data_lsb_set(&Regs[0x36], myctpc - 1, 3);

			/* set exposure time [GREEN] if zero */
			if (data_lsb_get(&Regs[0x3c], 3) == 0)
				data_lsb_set(&Regs[0x3c], myctpc - 1, 3);

			/* set exposure time [BLUE] if zero */
			if (data_lsb_get(&Regs[0x42], 3) == 0)
				data_lsb_set(&Regs[0x42], myctpc - 1, 3);

			iValue = (iValue + 1) * step_size;

			/* update line exposure time */
			traget = (((myctpc + iValue - 1) / myctpc)  * myctpc);
			data_lsb_set(&Regs[0x30], traget - 1, 3);

			traget = (traget / step_size) - 1;
			data_lsb_set(&Regs[0x00e1], traget, 3);
		}

		rst = SANE_STATUS_GOOD;
	}

	return rst;
}

static SANE_Int data_lsb_get(SANE_Byte *address, SANE_Int size)
{
	SANE_Int ret = 0;

	if ((address != NULL)&&(size > 0)&&(size < 5))
	{
		SANE_Int a;
		SANE_Byte b;

		size--;

		for (a = size; a >= 0; a--)
		{
			b = address[a];
			ret = (ret << 8) + b;
		}
	}

	return ret;
}

static SANE_Byte data_bitget(SANE_Byte *address, SANE_Int mask)
{
	SANE_Byte rst = 0;

	if (address != NULL)
	{
		SANE_Int desp = 0;

		if (mask & 1);
		else if (mask & 2)   desp = 1;
		else if (mask & 4)   desp = 2;
		else if (mask & 8)   desp = 3;
		else if (mask & 16)  desp = 4;
		else if (mask & 32)  desp = 5;
		else if (mask & 64)  desp = 6;
		else if (mask & 128) desp = 7;

		rst = (*address & mask) >> desp;
	}

	return rst;
}

static void data_bitset(SANE_Byte *address, SANE_Int mask, SANE_Byte data)
{
	/* This function fills mask bits of just a byte with bits given in data */

	if (address != NULL)
	{
		if (mask & 1);
		else if (mask & 2)   data <<= 1;
		else if (mask & 4)   data <<= 2;
		else if (mask & 8)   data <<= 3;
		else if (mask & 16)  data <<= 4;
		else if (mask & 32)  data <<= 5;
		else if (mask & 64)  data <<= 6;
		else if (mask & 128) data <<= 7;
		
		*address = (*address & (0xff - mask)) | (data & mask);
	}
}

static void data_wide_bitset(SANE_Byte *address, SANE_Int mask, SANE_Int data)
{
	/* Setting bytes bit per bit
	   mask is 4 bytes size
	   Example:
	   data  = 0111010111
	   mask  = 00000000 11111111 11000000 00000000
	   rst   = 00000000 01110101 11000000 00000000 */

	if ((address != NULL)&&(mask != 0))
	{
		SANE_Int mymask, started = SANE_FALSE;

		while (mask != 0)
		{
			mymask = _B0(mask);

			if (started == SANE_FALSE)
			{
				if (mymask != 0)
				{
					SANE_Int a, myvalue;

					for (a = 0; a < 8; a++)
						if ((mymask & (1 << a)) != 0)
							break;

					myvalue = _B0(data << a);
					myvalue >>= a;
					data_bitset(address, mymask, myvalue);
					data >>= (8 - a);
					started = SANE_TRUE;
				}
			} else
			{
				data_bitset(address, mymask, _B0(data));
				data >>= 8;
			}

			address++;
			mask >>= 8;
		}
	}
}

static SANE_Int data_wide_bitget(SANE_Byte *address, SANE_Int mask)
{
	/* getting bytes bit per bit
	   mask is 4 bytes size
	   Example:
	   data  = 11001010 01110101 11110101 10011011
	   mask  = 00000000 11111111 11000000 00000000
	   rst   = 00000000 01110101 11000000 00000000 */

	SANE_Int rst = 0;

	if ((address != NULL)&&(mask != 0))
	{
		SANE_Int mymask, started = SANE_FALSE;
		SANE_Int dsp = 0;

		while (mask != 0)
		{
			mymask = _B0(mask);

			if (started == SANE_FALSE)
			{
				if (mymask != 0)
				{
					for (dsp = 0; dsp < 8; dsp++)
						if ((mymask & (1 << dsp)) != 0)
							break;

					rst = data_bitget(address, mymask) >> dsp;
					started = SANE_TRUE;
					dsp = 8 - dsp;
				}
			} else
			{
				rst = (data_bitget(address, mymask) << dsp) | rst;
				dsp += 8;
			}

			address++;
			mask >>= 8;
		}
	}

	return rst;
}

static void data_lsb_set(SANE_Byte *address, SANE_Int data, SANE_Int size)
{
	if ((address != NULL)&&(size > 0)&&(size < 5))
	{
		SANE_Int a;
		for (a = 0; a < size; a++)
		{
			address[a] = _B0(data);
			data >>= 8;
		}
	}
}

static void data_lsb_inc(SANE_Byte *address, SANE_Int data, SANE_Int size)
{
	if (address != NULL)
	{
		SANE_Int value = data_lsb_get(address, size);

		data_lsb_set(address, value + data, size);
	}
}

static void data_lsb_cpy(SANE_Byte *source, SANE_Byte *dest, SANE_Int size)
{
	if ((source != NULL) && (dest != NULL))
	{
		SANE_Int value = data_lsb_get(source, size);

		data_lsb_set(dest, value, size);
	}
}

static void data_msb_set(SANE_Byte *address, SANE_Int data, SANE_Int size)
{
	if ((address != NULL)&&(size > 0)&&(size < 5))
	{
		SANE_Int a;

		for (a = size - 1; a >= 0; a--)
		{
			address[a] = _B0(data);
			data >>= 8;
		}
	}
}

static SANE_Int data_swap_endianess(SANE_Int address, SANE_Int size)
{
	SANE_Int rst = 0;

	if ((size > 0)&&(size < 5))
	{
		SANE_Int a;

		for (a = 0; a < size; a++)
		{
			rst = (rst << 8) | _B0(address);
			address >>= 8;
		}
	}

	return rst;
}

static void RTS_lamp_gaincontrol_set(st_device *dev, SANE_Byte *Regs, SANE_Int resolution, SANE_Byte gainmode)
{
	DBG(DBG_FNC, "> RTS_lamp_gaincontrol_set(resolution=%i, gainmode=%i):\n", resolution, gainmode);

	if ((dev != NULL) && (Regs != NULL))
	{
		if (dev->chipset->model == RTS8822L_02A)
		{
			/* hp4370 */
			SANE_Int data1, data2;
			
			data1 = data_lsb_get(&Regs[0x154], 2) & 0xfe7f;
			data2 = data_lsb_get(&Regs[0x156], 2);
			
			switch(resolution)
			{
				case 4800:
					data2 |= 0x40;
					data1 &= 0xffbf;
					break;
				case 100:
				case 150:
				case 200:
				case 300:
				case 600:
				case 1200:
				case 2400:
					data1 |= 0x40;
					data2 &= 0xffbf;
					break;
			}

			data_lsb_set(&Regs[0x154], data1, 2);
			data_lsb_set(&Regs[0x156], data2, 2);
		} else
		{
			/* hp3970 hp4070 ua4900 */
			SANE_Int data;
			
			data = data_lsb_get(&Regs[0x154], 2) & 0xfe7f;
			data = (gainmode == SANE_FALSE)? data | 0x0040 : data & 0xffbf;
		
			switch(resolution)
			{
				case 100:
				case 200:
				case 300:
				case 600:
					data |= 0x0100;
					break;
				case 2400:
					data |= 0x0180;
					break;
				case 1200:
					if (dev->sensorcfg->type == CIS_SENSOR)
						data |= 0x80;
							else if (dev->sensorcfg->type == CCD_SENSOR)
								data |= 0x0180;
					break;
			}

			data_lsb_set(&Regs[0x0154], data, 2);
		}
	}
}

static SANE_Status RTS_scanner_start(st_device *dev)
{
	SANE_Status rst = SANE_STATUS_INVAL; /* default */

	DBG(DBG_FNC, "+ RTS_scanner_start():\n");

	if (dev != NULL)
	{
		SANE_Int data;

		data = 0;
		RTS_lamp_pwm_duty_get(dev, &data);
		data = _B0(data);
		
		DBG(DBG_FNC, "->   Pwm used = %i\n", data);

		/*
			windows driver saves pwm used, in file usbfile
			Section [SCAN_PARAM], field PwmUsed
		*/

		dev->status->cancel = SANE_FALSE;

		if (RTS_scan_start(dev) == SANE_STATUS_GOOD)
		{
			SANE_Int transferred;

			rst = SANE_STATUS_GOOD;

			wfree(dev->scanning->imagebuffer);
			dev->scanning->imagebuffer = NULL;

			SetLock(dev, NULL, (scan2.depth == 16)? SANE_FALSE : SANE_TRUE);

			/* Reservamos los buffers necesarios para leer la imagen */
			RTS_read_bff_alloc(dev);

			if (dev->Resize->type != RSZ_NONE)
				RTS_rsz_start(dev, &transferred); /* 6729 */

			RTS_scan_count_inc(dev);
		}
	}

	DBG(DBG_FNC, "- RTS_scanner_start: %i\n", rst);

	return rst;
}

static void Triplet_Gray(SANE_Byte *pPointer1, SANE_Byte *pPointer2, SANE_Byte *buffer, SANE_Int channels_count)
{ 
	/*
		pPointer1 = FAB8
		pPointer2 = FABC
		buffer    = FAC0
		channels_count = FAC4
	*/

	SANE_Int channel_size;

	DBG(DBG_FNC, "> Triplet_Gray(*pPointer1, *pPointer2, *buffer, channels_count=%i)\n", channels_count);

	channel_size = (scan2.depth + 7) / 8;
	channels_count = channels_count / 2;

	while (channels_count > 0)
	{
		data_lsb_cpy(pPointer1, buffer, channel_size);
		data_lsb_cpy(pPointer2, buffer + channel_size, channel_size);

		pPointer1 += 2 * channel_size;
		pPointer2 += 2 * channel_size;
		buffer    += 2 * channel_size;

		channels_count--;
	}
}

static void Triplet_Lineart(SANE_Byte *pPointer1, SANE_Byte *pPointer2, SANE_Byte *buffer, SANE_Int channels_count)
{
	/* Composing colour in lineart mode */
	
	SANE_Int dots_count = 0;
	SANE_Int channel;
	SANE_Byte mask;
	SANE_Byte value;
	SANE_Int C;
	
	DBG(DBG_FNC, "> Triplet_Lineart(*pPointer1, *pPointer2, *buffer, channels_count=%i)\n", channels_count);
	
	if (channels_count > 0)
	{
		dots_count = (channels_count + 1) / 2;
		while (dots_count > 0)
		{
			mask = 0x80;
			channel = 2;
			do
			{
				value = 0;
				for (C = 4; C > 0; C--)
				{
					value = (value << 2) + (((*pPointer2 & mask) << 1) | (*pPointer1 & mask));
					mask = mask >> 1;
				}
				*buffer = value;
				buffer++;
				channel--;
			} while (channel > 0);
			pPointer2 += 2;
			pPointer1 += 2;
			dots_count--;
		}
	}
}

static SANE_Status Arrange_NonColour(st_device *dev, SANE_Byte *buffer, SANE_Int buffer_size, SANE_Int *transferred)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	if ((dev != NULL) && (buffer != NULL) && (transferred != NULL))
	{
		SANE_Int lines_count = 0; /* ebp */ 
		SANE_Int channels_count = 0; /* fadc pisa buffer */
		struct st_scanning *scn;

		DBG(DBG_FNC, "+ Arrange_NonColour(*buffer, buffer_size=%i, *transferred):\n", buffer_size);

		/* this is just to make code more legible */
		scn = dev->scanning;

		if (scn->imagebuffer == NULL)
		{
			if ((scn->arrange_hres == SANE_TRUE)||(scan2.colormode == CM_LINEART))
			{
				scn->bfsize = (scn->arrange_sensor_evenodd_dist + 1) * line_size;

				if ((scn->imagebuffer = (SANE_Byte *) malloc(scn->bfsize * sizeof(SANE_Byte))) != NULL)
				{
					if (Read_Block(dev, scn->bfsize, scn->imagebuffer, transferred) == SANE_STATUS_GOOD)
					{
						scn->channel_size     = (scan2.depth == 8)? 1: 2;
						scn->desp1[CL_RED]    = 0;
						scn->desp2[CL_RED]    = scn->channel_size + (scn->arrange_sensor_evenodd_dist * line_size);
						scn->pColour2[CL_RED] = scn->imagebuffer + scn->desp2[CL_RED];
						scn->pColour1[CL_RED] = scn->imagebuffer + scn->desp1[CL_RED];
						rst = SANE_STATUS_GOOD;
					}
				}
			}
		} else rst = SANE_STATUS_GOOD;
		
		/* b0f4 */
		if (rst == SANE_STATUS_GOOD)
		{
			scn->imagepointer = scn->imagebuffer;
			lines_count = buffer_size / line_size;
			channels_count = line_size / scn->channel_size;
			while (lines_count > 0)
			{
				if (scan2.colormode == CM_LINEART)
					Triplet_Lineart(scn->pColour1[CL_RED], scn->pColour2[CL_RED], buffer, channels_count);
						else Triplet_Gray(scn->pColour1[CL_RED], scn->pColour2[CL_RED], buffer, channels_count);

				buffer += line_size;
				scn->arrange_size -= bytesperline;

				lines_count--;
				if (lines_count == 0)
				{
					if ((scn->arrange_size | v15bc) == 0)
						break;
				}

				rst = Read_Block(dev, line_size, scn->imagepointer, transferred);
				if (rst != SANE_STATUS_GOOD)
					break;

				if (scn->arrange_hres == SANE_TRUE)
				{
					scn->desp2[CL_RED] = (line_size + scn->desp2[CL_RED]) % scn->bfsize;
					scn->desp1[CL_RED] = (line_size + scn->desp1[CL_RED]) % scn->bfsize;

					scn->pColour2[CL_RED] = scn->imagebuffer + scn->desp2[CL_RED];
					scn->pColour1[CL_RED] = scn->imagebuffer + scn->desp1[CL_RED];
				}

				/* b21d */
				scn->imagepointer += line_size;
				if (scn->imagepointer >= (scn->imagebuffer + scn->bfsize))
					scn->imagepointer = scn->imagebuffer;
			}
		}
	}
	/* 2246 */
	
	DBG(DBG_FNC, "- Arrange_NonColour(*transferred=%i): %i\n", *transferred, rst);
	
	return rst;
}

static SANE_Status RTS_rsz_decrease(SANE_Byte *to_buffer, SANE_Int to_resolution, SANE_Int to_width, SANE_Byte *from_buffer, SANE_Int from_resolution, SANE_Int from_width, SANE_Int myresize_mode)
{
	/*
		to_buffer = FAC8 = 0x236200
		to_resolution      = FACC = 0x4b
		to_width    = FAD0 = 0x352
		from_buffer = FAD4 = 0x235460
		from_resolution      = FAD8 = 0x64
		from_width    = FADC = 0x46d
		myresize_mode   = FAE0 = 1
	*/
	
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "+ RTS_rsz_decrease(to_resolution=%i, to_width=%i, from_resolution=%i, from_width=%i, myresize_mode=%i):\n",
	    to_resolution, to_width, from_resolution, from_width, myresize_mode);

	if ((to_buffer != NULL) && (from_buffer != NULL))
	{
		SANE_Int channels = 0; /* fac8 */
		SANE_Int depth = 0; /* fae0 */
		SANE_Int color[3] = {0, 0, 0}; /* fab8 | fabc | fac0 */
		SANE_Int to_pos = 0; /* fad4 */
		SANE_Int rescont = 0;
		SANE_Int from_pos = 0; /* fab4 */
		SANE_Int C;
		SANE_Int smres = 0; /* fab0 */
		SANE_Int value;
		SANE_Int channel_size;

		if (myresize_mode != RSZ_LINEART)
		{
			switch (myresize_mode)
			{
				case RSZ_GRAYL:
					channels = 1;
					depth = 8;
					break;
				case RSZ_COLOURL:
					channels = 3;
					depth = 8;
					break;
				case RSZ_COLOURH:
					channels = 3;
					depth = 16;
					break;
				case RSZ_GRAYH:
					channels = 1;
					depth = 16;
					break;
			}

			channel_size = (depth + 7) / 8;
			to_pos = 0;
			rescont = 0;

			while (to_pos < to_width)
			{
				from_pos++;
				if (from_pos > from_width)
					from_buffer -= (((depth + 7) / 8) * channels);

				rescont += to_resolution;
				if (rescont < from_resolution)
				{
					/* Adds 3 color channel values */
					for (C = 0; C < channels; C++)
					{
						color[C] += data_lsb_get(from_buffer, channel_size) * to_resolution;
						from_buffer += channel_size;
					}
				} else
				{
					/* fc3c */
					to_pos++;
					smres = to_resolution - (rescont - from_resolution);
					for (C = 0; C < channels; C++)
					{
						value = ((data_lsb_get(from_buffer, channel_size) * smres) + color[C]) / from_resolution;
						data_lsb_set(to_buffer, value, channel_size);
						color[C] = data_lsb_get(from_buffer, channel_size) * (rescont - from_resolution);

						to_buffer += channel_size;
						from_buffer += channel_size;
					}
					rescont -= from_resolution;
				}
			}

			rst = SANE_STATUS_GOOD;
		} else
		{
			/* fd60 */
			SANE_Int bit, pos, desp, rescont2;
			
			*to_buffer = 0;
			bit = 0;
			pos = 0;
			desp  = 0;
			rescont = 0;
			rescont2 = 0;
			if (to_width > 0)
			{
				do
				{
					if (bit == 8)
					{
						/* fda6 */
						bit = 0;
						to_buffer++;
						*to_buffer = 0;
					}

					rescont += to_resolution;
					if (rescont < from_resolution)
					{
						if ((*from_buffer & (0x80 >> desp)) != 0)
							rescont2 += to_resolution;
					} else 
					{
						/*fdd5*/
						pos++;
						rescont -= from_resolution;
						if ((*from_buffer & (0x80 >> desp)) != 0)
							/*fdee*/
							rescont2 += (to_resolution - rescont);
						if (rescont2 > (to_resolution / 2))
							/* fe00 */
							*to_buffer = _B0(*to_buffer | (0x80 >> bit));
						rescont2 = ((*from_buffer & (0x80 >> desp)) != 0) ? rescont : 0;
						bit++;
					}

					/* fe2f */
					desp++;
					if (desp == 8)
					{
						desp = 0;
						from_buffer++;
					}
				} while (pos < to_width);
			} else rst = SANE_STATUS_GOOD;
		}
	}

	DBG(DBG_FNC, "- RTS_rsz_decrease: %i\n", rst);
	
	return rst;
}

static SANE_Status RTS_rsz_increase(SANE_Byte *to_buffer, SANE_Int to_resolution, SANE_Int to_width, SANE_Byte *from_buffer, SANE_Int from_resolution, SANE_Int from_width, SANE_Int myresize_mode)
{
	/*

		to_buffer       = FAC8 = 0x2353f0
		to_resolution   = FACC = 0x4b
		to_width        = FAD0 = 0x352
		from_buffer     = FAD4 = 0x234650
		from_resolution = FAD8 = 0x64
		from_width      = FADC = 0x46d
		myresize_mode   = FAE0 = 1
	*/

	SANE_Status rst = SANE_STATUS_INVAL;


	DBG(DBG_FNC, "+ RTS_rsz_increase(*to_buffer, to_resolution=%i, to_width=%i, *from_buffer, from_resolution=%i, from_width=%i, myresize_mode=%i):\n",
	    to_resolution, to_width, from_resolution, from_width, myresize_mode);

	if ((to_buffer != NULL) && (from_buffer != NULL))
	{
		SANE_Int desp; /* fac0 */
		SANE_Byte *myp2; /* faac */
		SANE_Int mywidth; /* fab4 fab8 */
		SANE_Int mychannels; /* fabc */
		SANE_Int channels = 0; /* faa4 */
		SANE_Int depth = 0; /* faa8 */
		SANE_Int pos = 0; /* fae0 */
		SANE_Int rescount;
		SANE_Int val6 = 0;
		SANE_Int val7 = 0;
		SANE_Int value; /**/

		if (myresize_mode != RSZ_LINEART)
		{
			switch (myresize_mode)
			{
				case RSZ_GRAYL:
					channels = 1;
					depth = 8;
					break;
				case RSZ_COLOURL:
					channels = 3;
					depth = 8;
					break;
				case RSZ_COLOURH:
					channels = 3;
					depth = 16;
					break;
				case RSZ_GRAYH:
					channels = 1;
					depth = 16;
					break;
			}

			if (channels > 0)
			{
				SANE_Byte channel_size;
				SANE_Byte *p_dst; /* fac8 */
				SANE_Byte *p_src; /* fad4 */

				desp = to_buffer - from_buffer;
				myp2 = from_buffer;
				channel_size = (depth == 8)? 1 : 2;

				for (mychannels = 0; mychannels < channels; mychannels++)
				{
					pos = 0;
					rescount = (from_resolution / 2) + to_resolution;

					p_src = myp2;
					p_dst = myp2 + desp;

					/* f938 */
					val7 = data_lsb_get(p_src, channel_size);

					if (to_width > 0)
					{
						for (mywidth = 0; mywidth < to_width; mywidth++)
						{
							if (rescount >= to_resolution)
							{
								rescount -= to_resolution;
								val6 = val7;
								pos++;
								if (pos < from_width)
								{
									p_src += (channels * channel_size);
									val7 = data_lsb_get(p_src, channel_size);
								}
							}

							/*f9a5*/
							data_lsb_set(p_dst, ((((to_resolution - rescount) * val6) + (val7 * rescount)) / to_resolution), channel_size);
							rescount += from_resolution;
							p_dst += (channels * channel_size);
						}
					}

					myp2 += channel_size;
				}

				rst = SANE_STATUS_GOOD;
			} else rst = SANE_STATUS_GOOD;
		} else
		{
			/* RSZ_LINEART mode */
			/* fa02 */
			/*
			to_buffer = FAC8 = 0x2353f0
			to_resolution      = FACC = 0x4b
			to_width    = FAD0 = 0x352
			from_buffer = FAD4 = 0x234650
			from_resolution      = FAD8 = 0x64
			from_width    = FADC = 0x46d
			myresize_mode   = FAE0 = 1
			*/
			SANE_Int myres2; /* fac8 */
			SANE_Int sres;
			SANE_Int lfae0;
			SANE_Int lfad8;
			SANE_Int myres;
			SANE_Int cont = 1;
			SANE_Int someval;
			SANE_Int bit; /*lfaa8*/

			myres2 = from_resolution;
			sres = (myres2 / 2) + to_resolution;
			value = _B0(*from_buffer);
			bit = 0;
			lfae0 = 0;
			lfad8 = value >> 7;
			someval = lfad8;
			*to_buffer = 0;

			if (to_width > 0)
			{
				myres = to_resolution;
				to_resolution = myres / 2;
				do
				{
					if (sres >= myres)
					{
						sres -= myres;
						lfae0++;
						cont++;
						lfad8 = someval;
						if (lfae0 < from_width)
						{
							if (cont == 8)
							{
								cont = 0;
								from_buffer++;
							}
							bit = (((0x80 >> cont) & *from_buffer) != 0) ? 1: 0;
						}
					}
					/*faa6*/
					if ((((myres - sres) * lfad8) + (bit * sres)) > to_resolution)
						*to_buffer |= (0x80 >> bit);
					
					bit++;
					if (bit == 8)
					{
						bit = 0;
						to_buffer++;
						*to_buffer = 0;
					}
					to_width--;
					sres += myres2;
				} while (to_width > 0);
				rst = SANE_STATUS_GOOD;
			}
		}
	}

	DBG(DBG_FNC, "- RTS_rsz_increase: %i\n", rst);

	return rst;
}

static SANE_Status RTS_rsz_start(st_device *dev, SANE_Int *transferred)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "+ RTS_rsz_start:\n");

	if ((dev != NULL) && (transferred != NULL))
	{
		struct st_resize *rz = dev->Resize;

		if (RTS_rsz_alloc(dev, line_size, rz->bytesperline, rz->bytesperline) != SANE_STATUS_INVAL)
		{
			if (arrangeline2 == FIX_BY_SOFT)
			{
				/* fee0 */
				if (scan2.colormode == CM_COLOR)
					rst = Arrange_Colour(dev, rz->v3624, line_size, transferred);
						else rst = Arrange_NonColour(dev, rz->v3624, line_size, transferred);
			} else rst = Read_Block(dev, line_size, rz->v3624, transferred); /* ff03 */

			/* Redimensionado */
			switch (rz->type)
			{
				case RSZ_DECREASE:
					/* ff1b */
					RTS_rsz_decrease(rz->v3628, rz->resolution_x, rz->towidth, rz->v3624, scan2.resolution_x, rz->fromwidth, rz->mode);
					break;
				case RSZ_INCREASE:
					/* ff69 */
					rz->rescount = 0;
					RTS_rsz_increase(rz->v3628, rz->resolution_x, rz->towidth, rz->v3624, scan2.resolution_x, rz->fromwidth, rz->mode);
					if (arrangeline2 == FIX_BY_SOFT)
					{
						/* ffb1 */
						if (scan2.colormode == CM_COLOR)
							rst = Arrange_Colour(dev, rz->v3624, line_size, transferred);
								else rst = Arrange_NonColour(dev, rz->v3624, line_size, transferred);
					} else rst = Read_Block(dev, line_size, rz->v3624, transferred); /* ffe0 */

					/* fff2 */
					RTS_rsz_increase(rz->v362c, rz->resolution_x, rz->towidth, rz->v3624, scan2.resolution_x, rz->fromwidth, rz->mode);
					break;
			}

			DBG(DBG_FNC, " -> transferred=%i\n", *transferred);
		}
	}

	DBG(DBG_FNC, "- RTS_rsz_start: %i\n", rst);

	return rst;
}

static SANE_Status RTS_rsz_alloc(st_device *dev, SANE_Int size1, SANE_Int size2, SANE_Int size3)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	if (dev != NULL)
	{
		struct st_resize *rz = dev->Resize;

		rz->v3624 = (SANE_Byte *) malloc((size1 + 0x40) * sizeof(SANE_Byte));
		rz->v3628 = (SANE_Byte *) malloc((size2 + 0x40) * sizeof(SANE_Byte));
		rz->v362c = (SANE_Byte *) malloc((size3 + 0x40) * sizeof(SANE_Byte));

		if ((rz->v3624 == NULL)||(rz->v3628 == NULL)||(rz->v362c == NULL))
			RTS_rsz_free(dev);
				else rst = SANE_STATUS_GOOD;
	}

	DBG(DBG_FNC, "> RTS_rsz_alloc(size1=%i, size2=%i, size3=%i): %i\n", size1, size2, size3, rst);

	return rst;
}

static void RTS_rsz_free(st_device *dev)
{
	if (dev != NULL)
	{
		struct st_resize *rz;

		if ((rz = dev->Resize) != NULL)
		{
			wfree(rz->v3624);
			wfree(rz->v3628);
			wfree(rz->v362c);
			rz->v3624 = NULL;
			rz->v3628 = NULL;
			rz->v362c = NULL;
		}
	}
}

static void RTS_read_bff_free(st_device *dev)
{
	DBG(DBG_FNC, "> RTS_read_bff_free():\n");

	if (dev != NULL)
	{
		wfree(dev->Reading->DMABuffer);
		wfree(dev->scanning->imagebuffer);
		dev->scanning->imagebuffer = NULL;
		memset(dev->Reading, 0, sizeof(struct st_readimage));
	}
}

static SANE_Status RTS_gamma_write(st_device *dev, SANE_Byte *Regs, SANE_Byte *gammatable, SANE_Int size)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "+ RTS_gamma_write(size=%i):\n", size);
  
	if ((dev != NULL) && (Regs != NULL) && (gammatable != NULL) && (size > 0))
	{
		SANE_Int transferred;
		SANE_Int first_table;
		SANE_Int cont = 0;
		SANE_Int retry = SANE_TRUE;
		SANE_Byte *mybuffer;

		/* lock */
		SetLock(dev, Regs, SANE_TRUE);

		first_table = (data_lsb_get(&Regs[0x1b4], 2) & 0x3fff) >> 4;

		if ((mybuffer = (SANE_Byte *) malloc(sizeof(SANE_Byte) * size)) != NULL)
		{
			/* Try to send buffer during 10 seconds */
			long tick = GetTickCount() + 10000;
			while ((retry == SANE_TRUE)&&(tick > GetTickCount()))
			{
				retry = SANE_FALSE;

				/* Operation type 0x14 */
				if (RTS_ctl_iwrite_word(dev, 0x0000, 0x0014, 0x0800) == SANE_STATUS_GOOD)
				{
					/* Send size to write */
					if (RTS_dma_write_enable(dev, 0x0000, size, first_table) == SANE_STATUS_GOOD)
					{
						/* Send data */
						if (RTS_blk_write(dev, size, gammatable, &transferred) == SANE_STATUS_GOOD)
						{
							/* Send size to read */
							if (RTS_dma_read_enable(dev, 0x0000, size, first_table) == SANE_STATUS_GOOD)
							{
								/* Retrieve data */
								if (RTS_blk_read(dev, size, mybuffer, &transferred) == SANE_STATUS_GOOD)
								{
									/* Check data */
									while ((cont < size) && (retry == SANE_FALSE))
									{
										if (mybuffer[cont] != gammatable[cont])
											retry = SANE_TRUE;
										cont++;
									}
									
									if (retry == SANE_FALSE)
										rst = SANE_STATUS_GOOD;
 								}
							}
						}
					}
				}
			}
			
			free(mybuffer);
		}

		/* unlock */
		SetLock(dev, Regs, SANE_FALSE);
	}

	DBG(DBG_FNC, "- RTS_gamma_write: %i\n", rst);

	return rst;
}

static SANE_Status Gamma_GetTables(st_device *dev, SANE_Byte *Gamma_buffer)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "+ Gamma_GetTables(SANE_Byte *Gamma_buffer):\n");

	if ((dev != NULL) && (Gamma_buffer != NULL))
	{
		/* Operation type 0x14 */
		if (RTS_ctl_iwrite_word(dev, 0x0000, 0x0014, 0x0800) == 0x00)
		{
			SANE_Int size = 768;

			if (RTS_dma_read_enable(dev, 0x0000, size, 0) == SANE_STATUS_GOOD)
			{
				SANE_Int transferred = 0;
				usleep(1000 * 500);

				/* Read buffer */
				rst = RTS_blk_read(dev, size, Gamma_buffer, &transferred);
			}
		}
	}

	DBG(DBG_FNC, "- Gamma_GetTables: %i\n", rst);

	return rst;
}

static void RTS_scanner_stop(st_device *dev, SANE_Int wait)
{
	DBG(DBG_FNC, "+ RTS_scanner_stop():\n");

	if (dev != NULL)
	{
		SANE_Byte data = 0;
		
		RTS_read_bff_free(dev);
		RTS_rsz_free(dev);

		RTS_dma_reset(dev);

		data_bitset(&dev->init_regs[0x60b], 0x10, 0);
		data_bitset(&dev->init_regs[0x60a], 0x40, 0);

		if (RTS_ctl_write_buffer(dev, 0xee0a, &dev->init_regs[0x60a], 2) == SANE_STATUS_GOOD)
			RTS_mtr_change(dev, dev->init_regs, 3);

		usleep(1000 * 200);

		if (wait == SANE_FALSE)
		{
			RTS_ctl_read_byte(dev, 0xe801, &data);
			if ((data & 0x02) == 0)
			{
				if (RTS_head_athome(dev, dev->init_regs) == SANE_FALSE)
				{
					/* clear execution bit */
					data_bitset(&dev->init_regs[0x00], 0x80, 0);

					RTS_ctl_write_byte(dev, 0x00, dev->init_regs[0x00]);
					RTS_head_park(dev, SANE_TRUE, dev->motorcfg->parkhomemotormove);
				}
			}
		} else
		{
			/*66a1 */
			/* clear execution bit */
			data_bitset(&dev->init_regs[0x00], 0x80, 0);

			RTS_ctl_write_byte(dev, 0x00, dev->init_regs[0x00]);
			if (RTS_head_athome(dev, dev->init_regs) == SANE_FALSE)
				RTS_head_park(dev, SANE_TRUE, dev->motorcfg->parkhomemotormove);
		}

		/*66e0*/
		RTS_sensor_enable(dev, dev->init_regs, 0);

		RTS_lamp_status_timer_set(dev, 13);
	}

	DBG(DBG_FNC, "- RTS_scanner_stop()\n");
}

static SANE_Status RTS_read_bff_alloc(st_device *dev)
{
	DBG(DBG_FNC, "+ RTS_read_bff_alloc():\n");

	if (dev != NULL)
	{
		SANE_Byte data;
		SANE_Int mybytesperline;
		SANE_Int mybuffersize, a, b;

		data = 0;
		
		/* Gets BinarythresholdH */
		if (RTS_ctl_read_byte(dev, 0xe9a1, &data) == SANE_STATUS_GOOD)
			binarythresholdh = data;

		mybytesperline = (scan2.depth == 12)? (bytesperline * 3) / 4: bytesperline;

		dev->Reading->Max_Size = 0xfc00;
		dev->Reading->DMAAmount = 0;

		a = (dev->chipset->dma.buffer_size / 63);
		b = (((dev->chipset->dma.buffer_size - a) / 2) + a) >> 0x0f;
		mybuffersize = ((b << 6) - b) << 10;
		if (mybuffersize < 0x1f800)
			mybuffersize = 0x1f800;

		dev->Reading->DMABufferSize = mybuffersize; /*3FFC00 4193280*/

		do
		{
			dev->Reading->DMABuffer = (SANE_Byte *) malloc(dev->Reading->DMABufferSize * sizeof(SANE_Byte));
			if (dev->Reading->DMABuffer != NULL)
				break;
			dev->Reading->DMABufferSize -= dev->Reading->Max_Size;
		} while(dev->Reading->DMABufferSize >= dev->Reading->Max_Size);

		/* 6003 */
		dev->Reading->Starting = SANE_TRUE;

		dev->Reading->Size4Lines = (mybytesperline > dev->Reading->Max_Size)?
			mybytesperline : (dev->Reading->Max_Size / mybytesperline) * mybytesperline;
		
		dev->Reading->ImageSize = imagesize;
		read_v15b4 = v15b4;

		DBG(DBG_FNC, "- RTS_read_bff_alloc():\n");
	}

	return SANE_STATUS_GOOD;
}

static void RTS_scan_count_inc(st_device *dev)
{
	/* Keep a count of the number of scans done by this scanner */
	DBG(DBG_FNC, "+ RTS_scan_count_inc():\n");

	/* check if chipset supports accessing eeprom */
	if (dev != NULL)
		if ((dev->chipset->capabilities & CAP_EEPROM) != 0)
		{
			SANE_Int idata;
			SANE_Byte cdata = 0;
			SANE_Byte somebuffer[26];

			switch(dev->chipset->model)
			{
				case RTS8822L_02A:
				case RTS8822BL_03A:
					/* value is 4 bytes size starting from address 0x21 in msb format */
					if (RTS_nvram_read_integer(dev, 0x21, &idata) == SANE_STATUS_GOOD)
					{
						idata = data_swap_endianess(idata, 4) + 1;
						idata = data_swap_endianess(idata, 4);
						RTS_nvram_write_integer(dev, 0x21, idata);
					}
					break;
				default:
					/* value is 4 bytes size starting from address 0x21 in lsb format */
					memset(&somebuffer, 0, sizeof(somebuffer));
					somebuffer[4] = 0x0c;

					RTS_nvram_read_integer(dev, 0x21, &idata);
					data_lsb_set(&somebuffer[0], idata + 1, 4);

					RTS_nvram_read_byte(dev, 0x003a, &cdata);
					somebuffer[25] = cdata;
					RTS_nvram_write_buffer(dev, 0x21, somebuffer, 0x1a);
					break;
			}
		}

	DBG(DBG_FNC, "- RTS_scan_count_inc\n");
}

static SANE_Int RTS_scan_count_get(st_device *dev)
{
	/* Returns the number of scans done by this scanner */

	SANE_Int idata = 0;

	DBG(DBG_FNC, "+ RTS_scan_count_get():\n");

	/* check if chipset supports accessing eeprom */
	if (dev != NULL)
		if ((dev->chipset->capabilities & CAP_EEPROM) != 0)
		{
			RTS_nvram_read_integer(dev, 0x21, &idata);

			switch(dev->chipset->model)
			{
				case RTS8822L_02A:
				case RTS8822BL_03A:
					/* value is 4 bytes size starting from address 0x21 in msb format */
					idata = data_swap_endianess(idata, 4);
					break;
				default: /* RTS8822L_01H */
						/* value is 4 bytes size starting from address 0x21 in lsb format */
						idata &= 0xffffffff;
					break;
			}
		}

	DBG(DBG_FNC, "- RTS_scan_count_get(): %i\n", idata);

	return idata;
}

static SANE_Status RTS_scanner_read(st_device *dev, SANE_Int buffer_size, SANE_Byte *buffer, SANE_Int *transferred)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "+ RTS_scanner_read(buffer_size=%i):\n", buffer_size);

	if ((dev != NULL) && (transferred != NULL))
	{
		SANE_Byte mycolormode = scan2.colormode;

		*transferred = 0;

		if (dev->Resize->type == RSZ_NONE)
		{
			if (arrangeline == FIX_BY_SOFT)
			{
				switch(mycolormode)
				{
					case CM_COLOR: rst = Arrange_Colour(dev, buffer, buffer_size, transferred); break;
					case 3       : rst = Arrange_Compose(dev, buffer, buffer_size, transferred); break;
					default      : rst = Arrange_NonColour(dev, buffer, buffer_size, transferred); break;
				}
			} else rst = Read_Block(dev, buffer_size, buffer, transferred);   /*00fe*/
		} else rst = Read_ResizeBlock(dev, buffer, buffer_size, transferred);         /*010d*/

		DBG(DBG_FNC, " -> transferred=%i\n", *transferred);
	}

	DBG(DBG_FNC, "- RTS_scanner_read: %i\n", rst);

	return rst;
}

static SANE_Status Arrange_Compose(st_device *dev, SANE_Byte *buffer, SANE_Int buffer_size, SANE_Int *transferred)
{
	/*
		fnb250

		0600FA7C   05E10048 buffer
		0600FA80   0000F906 buffer_size
	*/
	SANE_Status rst = SANE_STATUS_INVAL; /* default */

	DBG(DBG_FNC, "+ Arrange_Compose(buffer_size=%i):\n", buffer_size);

	if ((dev != NULL) && (buffer != NULL) && (transferred != NULL))
	{
		SANE_Byte *mybuffer = buffer; /* fa7c */
		SANE_Int mydistance; /*ebp */
		SANE_Int mydots; /*fa74 */
		SANE_Int channel_size;
		SANE_Int c;
		struct st_scanning *scn;

		rst = SANE_STATUS_GOOD;

		channel_size = (scan2.depth + 7) / 8;

		/* this is just to make code more legible */
		scn = dev->scanning;

		/* is the first time arrange is being executed? */
		if (scn->imagebuffer == NULL)
		{
			if (dev->sensorcfg->type == CCD_SENSOR)
				mydistance = (dev->sensorcfg->line_distance * scan2.resolution_y) / dev->sensorcfg->resolution;
					else mydistance = 0;

			if (mydistance != 0)
			{
				scn->bfsize = (scn->arrange_hres == SANE_TRUE) ? scn->arrange_sensor_evenodd_dist: 0;
				scn->bfsize = line_size * (scn->bfsize + (mydistance * 2) + 1);
			} else scn->bfsize = line_size * 2;

			/*b2f0 */
			scn->imagebuffer = (SANE_Byte *) malloc(scn->bfsize * sizeof(SANE_Byte));
			if (scn->imagebuffer != NULL)
			{
				scn->imagepointer = scn->imagebuffer;
				if (Read_Block(dev, scn->bfsize, scn->imagebuffer, transferred) != SANE_STATUS_INVAL)
				{
					/* Calculate channel displacements */
					scn->arrange_orderchannel = SANE_FALSE;
					for (c = CL_RED; c <= CL_BLUE; c++)
					{
						if (mydistance == 0)
						{
							/*b34e*/
							if (scn->arrange_hres == SANE_FALSE)
							{
								if ((((dev->sensorcfg->line_distance * scan2.resolution_y) * 2) / dev->sensorcfg->resolution) == 1)
									scn->arrange_orderchannel = SANE_TRUE;

								if (scn->arrange_orderchannel == SANE_TRUE)
									scn->desp[c] = ((dev->sensorcfg->rgb_order[c] / 2) * line_size) + (channel_size * c);
										else scn->desp[c] = channel_size * c;
							}
						} else
						{
							/*b3e3*/
							scn->desp[c] = (dev->sensorcfg->rgb_order[c] * (mydistance * line_size)) + (channel_size * c);

							if (scn->arrange_hres == SANE_TRUE)
							{
								/*b43b */
								scn->desp1[c] = scn->desp[c];
								scn->desp2[c] = ((channel_size * 3) + scn->desp1[c]) + (scn->arrange_sensor_evenodd_dist * line_size);
							};
						}
					}

					for (c = CL_RED; c <= CL_BLUE; c++)
					{
						if (scn->arrange_hres == SANE_TRUE)
						{
							scn->pColour2[c] = scn->imagebuffer + scn->desp2[c];
							scn->pColour1[c] = scn->imagebuffer + scn->desp1[c];
						} else scn->pColour[c] = scn->imagebuffer + scn->desp[c];
					}
				} else rst = SANE_STATUS_INVAL; /* Read_Block failed */
			} else rst = SANE_STATUS_INVAL; /* malloc failed */
		}

		if (rst == SANE_STATUS_GOOD)
		{
			/*b545*/
			buffer_size /= line_size;
			mydots = line_size / (channel_size * 3);
			
			while (buffer_size > 0)
			{
				if (scn->arrange_orderchannel == SANE_FALSE)
				{
					/*b5aa*/
					if (scn->arrange_hres == SANE_TRUE)
						Triplet_Compose_HRes(dev, scn->pColour1[CL_RED], scn->pColour1[CL_GREEN], scn->pColour1[CL_BLUE], scn->pColour2[CL_RED], scn->pColour2[CL_GREEN], scn->pColour2[CL_BLUE], mybuffer, mydots);
							else Triplet_Compose_LRes(dev, scn->pColour[CL_RED], scn->pColour[CL_GREEN], scn->pColour[CL_BLUE], mybuffer, mydots);
				} else Triplet_Compose_Order(dev, scn->pColour[CL_RED], scn->pColour[CL_GREEN], scn->pColour[CL_BLUE], mybuffer, mydots);

				/*b5f8*/
				mybuffer += line_size;
				scn->arrange_size -= bytesperline;
				if (scn->arrange_size < 0)
					v15bc--;

				buffer_size--;
				if (buffer_size == 0)
				{
					if ((scn->arrange_size | v15bc) == 0)
						break; /* rst is SANE_STATUS_GOOD so finish */
				}

				/*b63f*/
				if (Read_Block(dev, line_size, scn->imagepointer, transferred) != SANE_STATUS_INVAL)
				{
					for (c = CL_RED; c <= CL_BLUE; c++)
					{
						if (scn->arrange_hres == SANE_TRUE)
						{
							/*b663*/
							scn->desp2[c] = (scn->desp2[c] + line_size) % scn->bfsize;
							scn->desp1[c] = (scn->desp1[c] + line_size) % scn->bfsize;

							scn->pColour2[c] = scn->imagebuffer + scn->desp2[c];
							scn->pColour1[c] = scn->imagebuffer + scn->desp1[c];
						} else
						{
							/*b74a*/
							scn->desp[c]    = (scn->desp[c] + line_size) % scn->bfsize;
							scn->pColour[c] = scn->imagebuffer + scn->desp[c];
						}
					}

					/*b7be*/
					scn->imagepointer += line_size;
					if (scn->imagepointer >= (scn->imagebuffer + scn->bfsize))
						scn->imagepointer = scn->imagebuffer;
				} else
				{
					/* Read_Block failed */
					rst = SANE_STATUS_INVAL;
					break;
				}
			}
		}
	}

	DBG(DBG_FNC, "- Arrange_Compose: %i\n", rst);

	return rst;
}

static void Triplet_Compose_HRes(st_device *dev, SANE_Byte *pRed1, SANE_Byte *pGreen1, SANE_Byte *pBlue1, SANE_Byte *pRed2, SANE_Byte *pGreen2, SANE_Byte *pBlue2, SANE_Byte *buffer, SANE_Int Width)
{
	SANE_Int Value;
	SANE_Int chn_size;
	SANE_Int max_value;

	DBG(DBG_FNC, "> Triplet_Compose_HRes(*pRed1, *pGreen1, *pBlue1, *pRed2 *pGreen2, *pBlue2, *buffer, Width=%i):\n", Width);

	Width /= 2;
	chn_size = (scan2.depth + 7) / 8;
	max_value = (1 << scan2.depth) - 1;

	while (Width > 0)
	{
		Value = data_lsb_get(pRed1, chn_size) + data_lsb_get(pGreen1, chn_size) + data_lsb_get(pBlue1, chn_size);

		Value = min(Value, max_value);

		if (dev->gamma->table[CL_RED] != NULL)
		{
			if (scan2.depth > 8)
				Value = _B0(Value) | RTS_gamma_get(dev, CL_RED, Value >> 8);
					else Value = RTS_gamma_get(dev, CL_RED, Value);
		}

		data_lsb_set(buffer, Value, chn_size);
		buffer += chn_size;

		Value = data_lsb_get(pRed2, chn_size) + data_lsb_get(pGreen2, chn_size) + data_lsb_get(pBlue2, chn_size);

		Value = min(Value, max_value);

		if (dev->gamma->table[CL_RED] != NULL)
		{
			if (scan2.depth > 8)
				Value = _B0(Value) | RTS_gamma_get(dev, CL_RED, Value >> 8);
					else Value = RTS_gamma_get(dev, CL_RED, Value);
		}

		data_lsb_set(buffer, Value, chn_size);
		buffer += chn_size;

		pRed1   += 6 * chn_size;
		pGreen1 += 6 * chn_size;
		pBlue1  += 6 * chn_size;

		pRed2   += 6 * chn_size;
		pGreen2 += 6 * chn_size;
		pBlue2  += 6 * chn_size;

		Width--;
	}
}

static void Triplet_Compose_Order(st_device *dev, SANE_Byte *pRed, SANE_Byte *pGreen, SANE_Byte *pBlue, SANE_Byte *buffer, SANE_Int dots)
{
	SANE_Int Value;

	DBG(DBG_FNC, "> Triplet_Compose_Order(*pRed, *pGreen, *pBlue, *buffer, dots=%i):\n", dots);

	if (scan2.depth > 8)
	{
		/* c0fe */
		dots = dots / 2;
		while (dots > 0)
		{
			Value = min(data_lsb_get(pRed, 2) + data_lsb_get(pGreen, 2) + data_lsb_get(pBlue, 2), 0xffff);

			if (dev->gamma->table[CL_RED] != NULL)
				Value = _B0(Value) | (RTS_gamma_get(dev, CL_RED, Value >> 8) << 8);

			data_lsb_set(buffer, Value, 2);

			buffer += 2;
			pRed   += 6;
			pGreen += 6;
			pBlue  += 6;
			dots--;
		}
	} else
	{
		SANE_Byte *myp1, *myp2, *myp3;

		if (dev->sensorcfg->rgb_order[CL_RED] == 1)
		{
			myp1 = pRed;
			myp2 = pGreen;
			myp3 = pBlue;
		} else if (dev->sensorcfg->rgb_order[CL_GREEN] == 1)
		{
			myp1 = pGreen;
			myp2 = pRed;
			myp3 = pBlue;
		} else
		{
			myp1 = pBlue;
			myp2 = pRed;
			myp3 = pGreen;
		}

		while (dots > 0)
		{
			Value = min(((*myp1 + *(line_size + myp1)) / 2) + *myp2 + *myp3, 0xff);

			if (dev->gamma->table[CL_RED] != NULL)
				*buffer = RTS_gamma_get(dev, CL_RED, Value);
					else *buffer = _B0(Value);

			buffer++;
			myp1 += 3;
			myp2 += 3;
			myp3 += 3;
			dots--;
		}
	}
}

static void Triplet_Compose_LRes(st_device *dev, SANE_Byte *pRed, SANE_Byte *pGreen, SANE_Byte *pBlue, SANE_Byte *buffer, SANE_Int dots)
{
	SANE_Int Value;
	SANE_Int chn_size;
	SANE_Int max_value;

	DBG(DBG_FNC, "> Triplet_Compose_LRes(*pRed, *pGreen, *pBlue, *buffer, dots=%i):\n", dots);

	chn_size = (scan2.depth + 7) / 8;
	max_value = (1 << scan2.depth) - 1;

	/*bf59 */
	while (dots > 0)
	{
		Value = data_lsb_get(pRed, chn_size) + data_lsb_get(pGreen, chn_size) + data_lsb_get(pBlue, chn_size);

		Value = min(Value, max_value);

		if (dev->gamma->table[CL_RED] != NULL)
		{
			if (scan2.depth > 8)
				Value = _B0(Value) | (RTS_gamma_get(dev, CL_RED, Value >> 8) << 8);
					else Value = _B0(RTS_gamma_get(dev, CL_RED, Value));
		}

		data_lsb_set(buffer, Value, chn_size);

		buffer += chn_size;
		pRed   += chn_size * 3;
		pGreen += chn_size * 3;
		pBlue  += chn_size * 3;
		dots--;
	}
}

static void Triplet_Colour_Order(st_device *dev, SANE_Byte *pRed, SANE_Byte *pGreen, SANE_Byte *pBlue, SANE_Byte *buffer, SANE_Int Width)
{
	SANE_Int Value;

	DBG(DBG_FNC, "> Triplet_Colour_Order(*pRed, *pGreen, *pBlue, *buffer, Width=%i):\n", Width);

	if (scan2.depth > 8)
	{
		Width = Width / 2;
		while (Width > 0)
		{
			Value = data_lsb_get(pRed, 2);
			data_lsb_set(buffer, Value, 2);

			Value = data_lsb_get(pGreen, 2);
			data_lsb_set(buffer + 2, Value, 2);

			Value = data_lsb_get(pBlue, 2);
			data_lsb_set(buffer + 4, Value, 2);

			pRed   += 6;
			pGreen += 6;
			pBlue  += 6;
			buffer += 6;
			Width--;
		}
	} else
	{
		SANE_Int Colour;

		if (dev->sensorcfg->rgb_order[CL_RED] == 1)
			Colour = CL_RED;
		else if (dev->sensorcfg->rgb_order[CL_GREEN] == 1)
			Colour = CL_GREEN;
		else Colour = CL_BLUE;
		
		while (Width > 0)
		{
			switch (Colour)
			{
				case CL_RED:
					*buffer       = (*pRed + *(pRed + line_size)) / 2;
					*(buffer + 1) = *pGreen;
					*(buffer + 2) = *pBlue;
					break;
				case CL_GREEN:
					*buffer       = *pRed;
					*(buffer + 1) = ((*pGreen + *(pGreen + line_size)) / 2);
					*(buffer + 2) = *pBlue;
					break;
				case CL_BLUE:
					*buffer       = *pRed;
					*(buffer + 1) = *pGreen;
					*(buffer + 2) = ((*pBlue + *(pBlue + line_size)) / 2);
					break;
			}

			pRed   += 3;
			pGreen += 3;
			pBlue  += 3;
			buffer += 3;

			Width--;
		}
	}
}

static void Triplet_Colour_HRes(SANE_Byte *pRed1, SANE_Byte *pGreen1, SANE_Byte *pBlue1, SANE_Byte *pRed2, SANE_Byte *pGreen2, SANE_Byte *pBlue2, SANE_Byte *buffer, SANE_Int Width)
{
	SANE_Int Value;
	SANE_Int chn_size;
	SANE_Int c;
	SANE_Byte *pPointers[6];

	pPointers[0] = pRed1;
	pPointers[1] = pGreen1;
	pPointers[2] = pBlue1;

	pPointers[3] = pRed2;
	pPointers[4] = pGreen2;
	pPointers[5] = pBlue2;

	DBG(DBG_FNC, "> Triplet_Colour_HRes(*pRed1, *pGreen1, *pBlue1, *pRed2, *pGreen2, *pBlue2, *buffer, Width=%i):\n", Width);

	chn_size = (scan2.depth + 7) / 8;

	Width = Width / 2;
	while (Width > 0)
	{
		for (c = 0; c < 6; c++)
		{
			Value = data_lsb_get(pPointers[c], chn_size);
			data_lsb_set(buffer, Value, chn_size);

			pPointers[c] += (6 * chn_size);
			buffer       += (chn_size);
		}
		Width--;
	}
}

static void Triplet_Colour_LRes(SANE_Int Width, SANE_Byte *Buffer, SANE_Byte *pChannel1,
                     SANE_Byte *pChannel2, SANE_Byte *pChannel3)
{
	/*
	05F0FA4C   04EBAE4A  /CALL to Assumed StdFunc6 from hpgt3970.04EBAE45
	05F0FA50   00234FF8  |Arg1 = 00234FF8 pChannel3
	05F0FA54   002359EF  |Arg2 = 002359EF pChannel2
	05F0FA58   002363E6  |Arg3 = 002363E6 pChannel1
	05F0FA5C   05D10048  |Arg4 = 05D10048 Buffer
	05F0FA60   00000352  |Arg5 = 00000352 Width
	*/
	
	/* Esta funcion une los tres canales de color en un triplete
	   Inicialmente cada color est� separado en 3 buffers apuntados
	   por pChannel1 ,2 y 3
	*/
	SANE_Int Value;
	SANE_Int chn_size;
	SANE_Int c;
	SANE_Byte *pChannels[3];

	pChannels[0] = pChannel3;
	pChannels[1] = pChannel2;
	pChannels[2] = pChannel1;

	DBG(DBG_FNC, "> Triplet_Colour_LRes(Width=%i, *Buffer2, *p1, *p2, *p3):\n", Width);

	chn_size = (scan2.depth + 7) / 8;
	while (Width > 0)
	{
		/* ba74 */
		for (c = 0; c < 3; c++)
		{
			Value = data_lsb_get(pChannels[c], chn_size);
			data_lsb_set(Buffer, Value, chn_size);

			pChannels[c] += chn_size;
			Buffer += chn_size;
		}
		Width--;
	}
}

static SANE_Status Read_ResizeBlock(st_device *dev, SANE_Byte *buffer, SANE_Int buffer_size, SANE_Int *transferred)
{
	/*The Beach
		buffer      = FA7C   05E30048
		buffer_size = FA80   0000F906
	*/

	SANE_Status rst = SANE_STATUS_INVAL; /* fa68 */

	DBG(DBG_FNC, "+ Read_ResizeBlock(buffer_size=%i):\n", buffer_size);

	if ((dev != NULL) && (buffer != NULL) && (transferred != NULL))
	{
		SANE_Int lfa54;
		SANE_Int lfa58;
		SANE_Byte *pP1; /* fa5c */
		SANE_Byte *pP2; /* fa60 */
		SANE_Int bOk;
		struct st_resize *rz = dev->Resize;

		/* fa74 = Resize->resolution_y*/
		/* fa70 = Resize->resolution_x*/
		/* fa64 = scan2.resolution_y  */
		/* fa6c = scan2.resolution_x  */

		if (rz->type == RSZ_DECREASE)
		{
			lfa58 = 0;
			do
			{
				bOk = 1;
				if (arrangeline2 == FIX_BY_SOFT)
				{
					if (scan2.colormode == CM_COLOR)
						rst = Arrange_Colour(dev, rz->v3624, line_size, transferred);
							else rst = Arrange_NonColour(dev, rz->v3624, line_size, transferred);
				} else rst = Read_Block(dev, line_size, rz->v3624, transferred);

				/*f2df*/
				RTS_rsz_decrease(rz->v362c, rz->resolution_x, rz->towidth, rz->v3624, scan2.resolution_x, rz->fromwidth, rz->mode);
				rz->rescount += rz->resolution_y;

				if (rz->rescount > scan2.resolution_y)
				{
					/*f331*/
					rz->rescount -= scan2.resolution_y;
					if (scan2.depth == 8)
					{
						/* f345 */
						pP1 = rz->v3628;
						pP2 = rz->v362c;
						if (rz->mode == RSZ_LINEART)
						{
							/* f36b */
							SANE_Int bit = 0;
							SANE_Byte *pP3 = rz->v362c;
							SANE_Int value;

							*buffer = 0;
							lfa54 = 0;
							while (lfa54 < rz->towidth)
							{
								if (bit == 8)
								{
									buffer++;
									*buffer = 0;
									pP1++;
									bit = 0;
									pP3++;
								}

								value = ((*pP1 & (0x80 >> bit)) != 0) ? rz->rescount : 0;

								if ((*pP3 & (0x80 >> bit)) != 0)
									value += (scan2.resolution_y - rz->rescount);

								if (value > rz->resolution_y)
									*buffer |= (0x80 >> bit);

								bit++;
								lfa54++;
							}
						} else
						{
							/* f414 */
							lfa54 = 0;
							while (lfa54 < rz->bytesperline)
							{
								*buffer = _B0((((scan2.resolution_y - rz->rescount) * *pP2) + (*pP1 * rz->rescount)) / scan2.resolution_y);
								pP1++;
								pP2++;
								buffer++;
								lfa54++;
							}
						}
					} else
					{
						/* f47d */
						lfa54 = 0;
						pP1 = rz->v3628;
						pP2 = rz->v362c;
						
						if ((rz->bytesperline & 0xfffffffe) > 0)
						{
							SANE_Int value;
							do
							{
								value = (((scan2.resolution_y - rz->rescount) * data_lsb_get(pP2, 2)) + (data_lsb_get(pP1, 2) * rz->rescount)) / scan2.resolution_y;
								data_lsb_set(buffer, value, 2);

								buffer += 2;
								pP1 += 2;
								pP2 += 2;
								lfa54++;
							} while (lfa54 < (rz->bytesperline / 2));
						}
					}
				} else bOk = 0;
				/* f4fd f502 */
				pP1 = rz->v3628;
				/* swap pointers*/
				rz->v3628 = rz->v362c;
				rz->v362c = pP1;
			} while (bOk == 0);
		} else
		{
			/*f530*/
			SANE_Int lfa68;
			SANE_Int transferred;
			SANE_Int channel_size;
			
			rz->rescount += scan2.resolution_y;
			lfa58 = 0;
			if (rz->rescount > rz->resolution_y)
			{
				lfa68 = 1;
				rz->rescount -= rz->resolution_y;
			} else lfa68 = 0;

			pP1 = rz->v3628;
			pP2 = rz->v362c;
				
			if (rz->mode == RSZ_LINEART)
			{
				/*f592*/
				*buffer = 0;

				if (rz->towidth > 0)
				{
					SANE_Int mask, mres;
					/* lfa60 = rz->resolution_y     */
					/* lfa7c = rz->resolution_y / 2 */

					for (lfa54 = 0; lfa54 < rz->towidth; lfa54++)
					{
						mask = 0x80 >> lfa58;

						mres = ((mask & *pP1) != 0)? rz->rescount: 0;

						if ((mask & *pP2) != 0)
							mres += (rz->resolution_y - rz->rescount);

						if (mres > (rz->resolution_y / 2))
							*buffer = *buffer | mask;

						lfa58++;
						if (lfa58 == 8)
						{
							lfa58 = 0;
							buffer++;
							pP1++;
							pP2++;
							*buffer = 0;
						}
					}
				}
			} else
			{
				/*f633*/
				channel_size = (scan2.depth + 7) / 8;
					
				if (rz->rescount < scan2.resolution_y)
				{
					if (rz->bytesperline != 0)
					{
						SANE_Int value;
						
						for (lfa54 = 0; lfa54 < rz->bytesperline; lfa54++)
						{
							value = (((scan2.resolution_y - rz->rescount) * data_lsb_get(pP2, channel_size)) + (rz->rescount * data_lsb_get(pP1, channel_size))) / scan2.resolution_y;
							data_lsb_set(buffer, value, channel_size);

							pP1 += channel_size;
							pP2 += channel_size;
							buffer += channel_size;
						}
					}
				} else memcpy(buffer, rz->v3628, rz->bytesperline); /*f6a8*/
			}
			
			/*f736*/
			if (lfa68 != 0)
			{
				SANE_Byte *temp;
				
				if (arrangeline2 == FIX_BY_SOFT)
				{
					/*f74b*/
					if (scan2.colormode == CM_COLOR)
						rst = Arrange_Colour(dev, rz->v3624, line_size, &transferred);
							else rst = Arrange_NonColour(dev, rz->v3624, line_size, &transferred);
				} else rst = Read_Block(dev, line_size, rz->v3624, &transferred); /*f77a*/
				
				/*f78c*/
				/* swap buffers */
				temp = rz->v3628;
				rz->v3628 = rz->v362c;
				rz->v362c = temp;
				
				RTS_rsz_increase(temp, rz->resolution_x, rz->towidth, rz->v3624, scan2.resolution_x, rz->fromwidth, rz->mode);
			} else rst = SANE_STATUS_GOOD;
		}

		DBG(DBG_FNC, " -> transferred=%i\n", *transferred);
	}

	DBG(DBG_FNC, "- Read_ResizeBlock: %i\n", rst);

	return rst;
}

static void Split_into_12bit_channels(SANE_Byte *destino, SANE_Byte *fuente, SANE_Int size)
{
	/*
		Each letter represents a bit
		abcdefgh 12345678 lmnopqrs << before splitting
		[efgh1234 0000abcd] [lmnopqrs 00005678]  << after splitting, in memory
		[0000abcd efgh1234] [00005678 lmnopqrs]  << resulting channels
	*/

	DBG(DBG_FNC, "> Split_into_12bit_channels(source size=%i\n", size);

	if ((destino != NULL)&&(fuente != NULL))
	{
		if ((size - (size & 0x03)) != 0)
		{
			SANE_Int C;

			C = (size - (size & 0x03) + 3) / 4;
			do
			{
				*destino = _B0((*(fuente + 1) >> 4) + (*fuente << 4));
				*(destino + 1) = _B0(*fuente >> 4);
				*(destino + 2) = _B0(*(fuente + 2));
				*(destino + 3) = *(fuente + 1) & 0x0f;
				destino += 4;
				fuente += 3;
				C--;
			} while (C > 0);
		}

		/**/
		if ((size & 0x03) != 0)
		{
			*destino = _B0((*(fuente + 1) >> 4) + (*fuente << 4));
			*(destino + 1) = _B0(*fuente >> 4);
		}
	}
}

static SANE_Status Read_NonColor_Block(st_device *dev, SANE_Byte *buffer, SANE_Int buffer_size, SANE_Byte ColorMode, SANE_Int *transferred)
{
	/* FA50   05DA0048 buffer
	   FA54   0000F906 buffer_size
	   FA58   00       ColorMode
	*/

	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "+ Read_NonColor_Block(*buffer, buffer_size=%i, ColorMode=%s):\n", buffer_size, dbg_colour(ColorMode));

	if ((dev != NULL) && (buffer != NULL) && (transferred != NULL))
	{
		SANE_Int lfa38 = 0;
		SANE_Byte *gamma = dev->gamma->table[CL_RED];
		SANE_Int block_bytes_per_line;
		SANE_Int mysize;
		SANE_Byte *mybuffer;

		rst = SANE_STATUS_GOOD;

		if (ColorMode != CM_GRAY)
		{
			/* Lineart mode */
			if ((lineart_width & 7) != 0)
				lfa38 = 8 - (lineart_width & 7);
			block_bytes_per_line = (lineart_width + 7) / 8;
		} else block_bytes_per_line = line_size;
		/*61b2*/

		mysize    = (buffer_size / block_bytes_per_line) * bytesperline;

		if ((mybuffer = (SANE_Byte *) malloc(mysize * sizeof(SANE_Byte))) != NULL)
		{
			SANE_Int LinesCount;
			SANE_Int mysize4lines;
			SANE_Byte *pBuffer = buffer;
			SANE_Byte *pImage  = NULL; /* fa30 */
			SANE_Int puntero;
			SANE_Int value;

			do
			{
				mysize4lines = (mysize <= dev->Reading->Size4Lines)? mysize : dev->Reading->Size4Lines;
				LinesCount = mysize4lines / bytesperline;
			
				if (ColorMode == CM_GRAY)
				{
					if (scan2.depth == 12)
					{
						/* 633b */
						/*GRAY Bit mode 12*/
						if ((rst = Scan_Read_BufferA(dev, (mysize4lines * 3) / 4, 0, mybuffer, transferred)) == SANE_STATUS_GOOD)
						{
							pImage = mybuffer;
							pBuffer += LinesCount * block_bytes_per_line;
							while (LinesCount > 0)
							{
								Split_into_12bit_channels(mybuffer, pImage, line_size);
								pImage += (bytesperline * 3) / 4;
								LinesCount--;
							}
						} else break;
					} else
					{
						/* grayscale 8 and 16 bits */
						if ((rst = Scan_Read_BufferA(dev, mysize4lines, 0, mybuffer, transferred)) == SANE_STATUS_GOOD)
						{
							SANE_Int channel_size = (scan2.depth > 8)? 2: 1;

							pImage = mybuffer;

							/* No gamma tables */
							while (LinesCount > 0)
							{
								if (line_size > 0)
								{
									puntero = 0;
									do
									{
										value = data_lsb_get(pImage + puntero, channel_size);
		
										if (gamma != NULL)
											value += *gamma << (8 * (channel_size - 1));

										data_lsb_set(pBuffer, value, channel_size);

										pBuffer += channel_size;
										puntero += channel_size;
									} while (puntero < line_size);
								}
								pImage += bytesperline;
								LinesCount--;
							}
						} else break;
					}
				} else
				{
					/*6429 */
					/* LINEART */
					SANE_Int desp;

					if ((rst = Scan_Read_BufferA(dev, mysize4lines, 0, mybuffer, transferred)) == SANE_STATUS_GOOD)
					{
						pImage = mybuffer;
						while (LinesCount > 0)
						{
							if (lineart_width > 0)
							{
								desp = 0;
								do
								{
									if ((desp % 7) == 0)
										*pBuffer = 0;

									/* making a byte bit per bit */
									*pBuffer = *pBuffer << 1;

									/* bit 1 if data is under thresholdh value*/
									if (*(pImage + desp) >= binarythresholdh) /* binarythresholdh = 0x0c */
										*pBuffer = *pBuffer | 1;

									desp++;
									if ((desp % 7) == 0)
										pBuffer++;

								} while (desp < lineart_width);
							}

							if (lfa38 != 0)
							{
								*pBuffer = (*pBuffer << lfa38);
								pBuffer++;
							}
							/* 64b0 */
							pImage += bytesperline;
							LinesCount--;
						}
					} else break;
				}
				/* 64c0 */
				mysize -= mysize4lines;
			} while ((mysize > 0)&&(dev->status->cancel == SANE_FALSE));
		
			free(mybuffer);
		} else rst = SANE_STATUS_INVAL;

		DBG(DBG_FNC, " -> transferred=%i\n", *transferred);
	}

	DBG(DBG_FNC, "- Read_NonColor_Block: %i\n", rst);

	return rst;
}

static SANE_Status Read_Block(st_device *dev, SANE_Int buffer_size, SANE_Byte *buffer, SANE_Int *transferred)
{
	/*
		SANE_Int buffer_size          fa80
		SANE_Byte *buffer    fa7c
	*/
/*
scan2:
04F0155C  01 08 00 02 03 00 58 02  ..X
04F01564  58 02 58 02 C5 00 00 00  XX�...
04F0156C  B4 07 00 00 8B 01 00 00  �..�..
04F01574  10 06 00 00 EC 13 00 00  ..�..
04F0157C  B2 07 00 00 B4 07 00 00  �..�..
04F01584  CF 08 00 00              �..

arrangeline2 = 1
*/
	SANE_Status rst = SANE_STATUS_INVAL; /* default */

	DBG(DBG_FNC, "+ Read_Block(buffer_size=%i):\n", buffer_size);

	if ((dev != NULL) && (buffer != NULL) && (transferred != NULL))
	{
		SANE_Int mysize, LinesCount;
		SANE_Byte *readbuffer = NULL;
		SANE_Byte *pImage = NULL;

		*transferred = 0;

		if ((scan2.colormode != CM_COLOR)&&(scan2.channel == 3)&&(arrangeline2 != FIX_BY_SOFT))
		{
			/*6510*/
			return Read_NonColor_Block(dev, buffer, buffer_size, scan2.colormode, transferred);
		}
		
		/*6544*/
		mysize = (buffer_size / line_size) * bytesperline;
		pImage = buffer;

		if ((readbuffer = (SANE_Byte *) malloc(mysize * sizeof(SANE_Byte))) != NULL)
		{
			do
			{
				buffer_size = (dev->Reading->Size4Lines < mysize)? dev->Reading->Size4Lines : mysize;
				LinesCount = buffer_size / bytesperline;
		
				if (scan2.depth == 12)
				{
					if ((rst = Scan_Read_BufferA(dev, buffer_size, 0, readbuffer, transferred)) == SANE_STATUS_GOOD)
					{
						if (LinesCount > 0)
						{
							SANE_Byte *destino, *fuente;
							destino = buffer;
							fuente  = readbuffer;
							do
							{
								Split_into_12bit_channels(destino, fuente, line_size);
								destino += line_size;
								fuente  += (bytesperline * 3) / 4;
								LinesCount--;
							} while (LinesCount > 0);
						}
					} else break;
				} else
				{
					/*65d9*/
					if ((rst = Scan_Read_BufferA(dev, buffer_size, 0, readbuffer, transferred)) == SANE_STATUS_GOOD)
					{
						memcpy(pImage, readbuffer, *transferred);

						pImage += *transferred;
					} else break;
				}
				/*6629 */
				mysize -= buffer_size;
			} while ((mysize > 0)&&(dev->status->cancel == SANE_FALSE));
		
			free(readbuffer);
		}

		DBG(DBG_FNC, " -> transferred=%i\n", *transferred);
	}

	DBG(DBG_FNC, "- Read_Block: %i\n", rst);

	return rst;
}

static SANE_Status Scan_Read_BufferA(st_device *dev, SANE_Int buffer_size, SANE_Int arg2, SANE_Byte *pBuffer, SANE_Int *bytes_transfered)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "+ Scan_Read_BufferA(buffer_size=%i):\n", buffer_size);

	arg2 = arg2; /* silence gcc */

	if ((dev != NULL) && (pBuffer != NULL) && (bytes_transfered != NULL))
	{
		SANE_Byte *ptBuffer = NULL;
		SANE_Byte *ptImg    = NULL;
		struct st_readimage *rd = dev->Reading;

		rst = SANE_STATUS_GOOD;

		*bytes_transfered = 0;

		ptBuffer = pBuffer;

		while ((buffer_size > 0)&&(rst == SANE_STATUS_GOOD)&&(dev->status->cancel == SANE_FALSE))
		{
			/* Check if we've already started */
			if (rd->Starting == SANE_TRUE)
			{
				/* Get channels per dot and channel's size in bytes */
				SANE_Byte data;

				rd->Channels_per_dot = 1;
				if (RTS_ctl_read_byte(dev, 0xe812, &data) == SANE_STATUS_GOOD)
				{
					data = data >> 6;
					if (data != 0)
						rd->Channels_per_dot = data;
				}

				rd->Channel_size = 1;
				if (RTS_ctl_read_byte(dev, 0xee0b, &data) == SANE_STATUS_GOOD)
					if (((data & 0x40) != 0)&&((data & 0x08) == 0))
						rd->Channel_size = 2;

				rd->RDStart = rd->DMABuffer;
				rd->RDSize  = 0;
				rd->DMAAmount = 0;
				rd->Starting = SANE_FALSE;
			}

			/* Is there any data to read from scanner? */
			if ((rd->ImageSize > 0)&&(rd->RDSize == 0))
			{
				/* Try to read from scanner all possible data to fill DMABuffer */
				if (rd->RDSize < rd->DMABufferSize)
				{
					SANE_Int iAmount, dofree;
	
					/* Check if we have already notify buffer size */
					if (rd->DMAAmount <= 0)
					{
						/* Initially I suppose that I can read all image*/
						iAmount = min(rd->ImageSize, rd->Max_Size);
						rd->DMAAmount = ((dev->chipset->dma.set_length * 2) / iAmount) * iAmount;
						rd->DMAAmount = min(rd->DMAAmount, rd->ImageSize);
						RTS_read_bff_size_set(dev, 0, rd->DMAAmount);
						iAmount = min(iAmount, rd->DMABufferSize - rd->RDSize);
					} else
					{
						iAmount = min(rd->DMAAmount, rd->ImageSize);
						iAmount = min(iAmount, rd->Max_Size);
					}
	
					/* Allocate buffer to read image if it's necessary */
					if ((rd->RDSize == 0)&&(iAmount <= buffer_size))
					{
						ptImg = ptBuffer;
						dofree = SANE_FALSE;
					} else
					{
						ptImg = (SANE_Byte *) malloc(iAmount * sizeof(SANE_Byte));
						dofree = SANE_TRUE;
					}
	
					if (ptImg != NULL)
					{
						/* We must wait for scanner to get data*/
						SANE_Int opStatus, sc;

						sc = (iAmount < rd->Max_Size)? SANE_TRUE:SANE_FALSE;
						opStatus = RTS_read_bff_wait(dev, rd->Channels_per_dot, rd->Channel_size, iAmount,
						                                 &rd->Bytes_Available, 10, sc);

						/* If something fails, perhaps we can read some bytes... */
						if (opStatus != SANE_STATUS_GOOD)
						{
							if (rd->Bytes_Available > 0)
								iAmount = rd->Bytes_Available;
									else rst = SANE_STATUS_INVAL;
						}
		
						if (rst == SANE_STATUS_GOOD)
						{
							/* Try to read from scanner */
							SANE_Int transferred = 0;
							opStatus = RTS_blk_read(dev, iAmount, ptImg, &transferred);
	
							DBG(DBG_FNC, "> Scan_Read_BufferA: Bulk read %i bytes\n", transferred);

							/*if something fails may be we can read some bytes */
							if ((iAmount = (SANE_Int)transferred) != 0)
							{
								/* Lets copy data into DMABuffer if it's necessary */
								if (ptImg != ptBuffer)
								{
									SANE_Byte *ptDMABuffer;
		
									ptDMABuffer = rd->RDStart + rd->RDSize;
									if ((ptDMABuffer - rd->DMABuffer) >= rd->DMABufferSize)
										ptDMABuffer -= rd->DMABufferSize;
		
									if ((ptDMABuffer + iAmount) >= (rd->DMABuffer + rd->DMABufferSize))
									{
										SANE_Int rest = iAmount - (rd->DMABufferSize - (ptDMABuffer - rd->DMABuffer));
										memcpy(ptDMABuffer, ptImg, iAmount - rest);
										memcpy(rd->DMABuffer, ptImg + (iAmount - rest), rest);
									} else memcpy(ptDMABuffer, ptImg, iAmount);
									rd->RDSize += iAmount;
								} else
								{
									*bytes_transfered += iAmount;
									buffer_size -= iAmount;
								}
	
								rd->DMAAmount -= iAmount;
								rd->ImageSize -= iAmount;
							} else rst = SANE_STATUS_INVAL;
						}
		
						/* Lets free buffer */
						if (dofree == SANE_TRUE)
						{
							free(ptImg);
							ptImg = NULL;
						}
					} else rst = SANE_STATUS_INVAL;
				}
			}
	
			/* is there any data read from scanner? */
			if (rd->RDSize > 0)
			{
				/* Add to the given buffer so many bytes as posible */
				SANE_Int iAmount;
	
				iAmount = min(buffer_size, rd->RDSize);
				if ((rd->RDStart + iAmount) >= (rd->DMABuffer + rd->DMABufferSize))
				{
					SANE_Int rest = rd->DMABufferSize - (rd->RDStart - rd->DMABuffer);
					memcpy(ptBuffer, rd->RDStart, rest);
					memcpy(ptBuffer + rest, rd->DMABuffer, iAmount - rest);
					rd->RDStart = rd->DMABuffer + (iAmount - rest);
				} else
				{
					memcpy(ptBuffer, rd->RDStart, iAmount);
					rd->RDStart += iAmount;
				}
			
				ptBuffer += iAmount;
				rd->RDSize -= iAmount;
				buffer_size -= iAmount;
				*bytes_transfered += iAmount;

				/* if there isn't any data in DMABuffer we can point RDStart
				   to the begining of DMABuffer */
				if (rd->RDSize == 0)
					rd->RDStart = rd->DMABuffer;
			}

			/* in case of all data is read we return SANE_STATUS_GOOD with bytes_transfered = 0 */
			if ((*bytes_transfered == 0)||((rd->RDSize == 0)&&(rd->ImageSize == 0)))
				break;
		}

		if (rst == SANE_STATUS_INVAL)
			RTS_dma_cancel(dev);

		DBG(DBG_FNC, "->   *bytes_transfered=%i\n", *bytes_transfered);
		DBG(DBG_FNC, "->   Reading->ImageSize=%i\n", rd->ImageSize);
		DBG(DBG_FNC, "->   Reading->DMAAmount=%i\n", rd->DMAAmount);
		DBG(DBG_FNC, "->   Reading->RDSize   =%i\n", rd->RDSize);
	}

	DBG(DBG_FNC, "- Scan_Read_BufferA: %i\n", rst);

	return rst;
}

static SANE_Int RTS_read_bff_size_get(st_device *dev, SANE_Byte channels_per_dot, SANE_Int channel_size)
{
	/* returns the ammount of bytes in scanner's buffer ready to be read */
	SANE_Int rst = 0;

	DBG(DBG_FNC, "+ RTS_read_bff_size_get(channels_per_dot=%i, channel_size=%i):\n", channels_per_dot, channel_size);

	if (dev != NULL)
	{
		if (channel_size > 0)
		{
			SANE_Int myAmount;

			if (channels_per_dot < 1)
			{
				/* read channels per dot from registers */
				if (RTS_ctl_read_byte(dev, 0xe812, &channels_per_dot) == SANE_STATUS_GOOD)
					channels_per_dot = _B0(channels_per_dot >> 6);

				if (channels_per_dot == 0)
					channels_per_dot++;
			}

			if (RTS_ctl_read_int(dev, 0xef16, &myAmount) == SANE_STATUS_GOOD)
				rst = ((channels_per_dot * 32) / channel_size) * myAmount;
		}
	}

	DBG(DBG_FNC, "- RTS_read_bff_size_get: %i bytes\n", rst);

	return rst;
}

static SANE_Status RTS_lamp_warmup(st_device *dev, SANE_Byte *Regs, SANE_Int lamp, SANE_Int scantype, SANE_Int resolution)
{
	SANE_Status rst = SANE_STATUS_GOOD;

	DBG(DBG_FNC, "+ RTS_lamp_warmup(lamp=%i, scantype=%s, resolution=%i)\n", lamp, dbg_scantype(scantype), resolution);

	if (dev != NULL)
	{
		if (Regs != NULL)
		{
			SANE_Byte flb_lamp, tma_lamp;
			SANE_Int overdrivetime;

			RTS_lamp_status_get(dev, &flb_lamp, &tma_lamp);

			/* ensure that selected lamp is switched on */
			if (lamp == FLB_LAMP)
			{
				overdrivetime = dev->options->overdrive_flb;

				if (flb_lamp == 0)
				{
					/* FLB-Lamp is turned off, lets turn on */
					RTS_lamp_status_set(dev, Regs, SANE_TRUE, FLB_LAMP);
					dev->status->overdrive = SANE_TRUE;
				}
			} else
			{
				/* is tma device attached to scanner ?*/
				if (RTS_isTmaAttached(dev) == SANE_TRUE)
				{
					overdrivetime = dev->options->overdrive_ta;

					if (tma_lamp == 0)
					{
						/* tma lamp is turned off */
						RTS_lamp_status_set(dev, Regs, SANE_FALSE, TMA_LAMP);
						dev->status->overdrive = SANE_TRUE;
					}
				} else rst = SANE_STATUS_INVAL;
			}

			/* perform warmup process */
			if (rst == SANE_STATUS_GOOD)
			{
				RTS_lamp_pwm_setup(dev, lamp);

				/*RTS_lamp_pwm_duty_set(dev, (lamp == TMA_LAMP)? 0x0e : 0x00);*/

				if (dev->options->use_gamma == SANE_TRUE)
				{
					DBG(DBG_VRB, "- Lamp Warmup process. Please wait...\n");

					dev->status->warmup = SANE_TRUE;

					if (dev->status->overdrive == SANE_TRUE)
					{
						/* lamp has been switched off... lets wait some time ... */
						long ticks = GetTickCount() + overdrivetime;

						while (GetTickCount() <= ticks)
							usleep(1000 * 200);

						dev->status->overdrive = SANE_FALSE;
					}

					RTS_lamp_pwm_checkstable(dev, scantype, resolution, lamp);

				} else DBG(DBG_VRB, "- Lamp Warmup process disabled.\n");
			}
		} else rst = SANE_STATUS_INVAL;

		dev->status->warmup = SANE_FALSE;
	}

	DBG(DBG_FNC, "- RTS_lamp_warmup: %i\n", rst);

	return rst;
}

static SANE_Status RTS_scan_start(st_device *dev)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "+ RTS_scan_start:\n");

	if (dev != NULL)
	{
		struct st_calibration_data *cal_data;

		if ((cal_data = calloc(1, sizeof(struct st_calibration_data))) != NULL)
		{
			if (RTS_sensor_enable(dev, dev->init_regs, 0x0f) == SANE_STATUS_GOOD)
			{
				SANE_Byte *Regs = RTS_regs_dup(dev->init_regs);

				if (Regs != NULL)
				{
					struct st_scanparams *scancfg = RTS_scancfg_dup(&scan);

					if (scancfg != NULL)
					{
						struct st_hwdconfig *hwdcfg = RTS_hwdcfg_alloc(SANE_TRUE);

						if (hwdcfg != NULL)
						{
							SANE_Byte mlock;
							SANE_Int ypos, runb1;
							/*SANE_Int cl;*/

							struct st_calibration myCalib;
							long tick;

							memset(&myCalib, 0, sizeof(struct st_calibration));

							dbg_ScanParams(scancfg);

							/*cl = (pixeldarklevel == 0xffff) ? 0 : 1;*/
							
							/* wait till lamp is at home (should use timeout
								windows driver doesn't use it)
							*/
							tick = GetTickCount() + 10000;
							while ((RTS_head_athome(dev, Regs) == SANE_FALSE)&&(tick > GetTickCount()));

							if (dev->status->preview == SANE_TRUE)
							{
								SANE_Int lfaa0 = 0;

								RTS_gnoff_counter_inc(dev, &lfaa0);
							}

							tick = GetTickCount();

							/* set margin references */
							RTS_refs_set(dev, Regs, scancfg);

							/* locate head to right position */
							ypos = cfg_strippos_get(dev, scancfg->scantype, 1);
							if (ypos != 0)
								RTS_head_relocate(dev, dev->motorcfg->parkhomemotormove, MTR_FORWARD, ypos);

							/* perform lamp warmup */
							if (RTS_lamp_warmup(dev, Regs, (scancfg->scantype == ST_NORMAL)? FLB_LAMP: TMA_LAMP, scancfg->scantype, scan.resolution_x) == SANE_STATUS_INVAL)
								return SANE_STATUS_INVAL;

							/* Calibration process */
							/* allocate space for shading correction */
							if (cal_shd_buffers_alloc(dev, scancfg, &myCalib) != SANE_STATUS_GOOD)
								return SANE_STATUS_INVAL;

							hwdcfg->calibrate = dev->options->calibrate;

							if (dev->options->calibrate != 0)
							{
								/* Let's calibrate */
								if ((scancfg->colormode != CM_COLOR)&&(scancfg->channel == 3))
									scancfg->colormode = CM_COLOR;

								hwdcfg->arrangeline = 0;

								if (scancfg->scantype == ST_NORMAL)
								{
									/* Calibration for reflective type */

									/*59e3*/
									RTS_regs_cpy(Regs, dev->init_regs);

									if (Calibration(dev, Regs, scancfg, cal_data, &myCalib, 0) != SANE_STATUS_GOOD)
									{
										if (dev->status->preview == SANE_FALSE)
											cal_shd_buffers_free(&myCalib);
									}
								} else
								{
									/*59ed*/
									/* Calibration for negative/slide type */
								}

								/*5af1*/
								if (dev->options->wht_board != SANE_FALSE)
								{
									RTS_head_park(dev, SANE_TRUE, dev->motorcfg->basespeedmotormove);
									scancfg->origin_y = 1;
								}

								scancfg->colormode = scan.colormode;
							} else
							{
								/*5b1e*/
								/*Don't calibrate*/
								if (scancfg->scantype == ST_NORMAL)
								{
									RTS_lamp_status_set(dev, NULL, SANE_TRUE, FLB_LAMP);
								} else
								{
									if ((scancfg->scantype == ST_TA)||(scancfg->scantype == ST_NEG))
									{
										/*SANE_Int ta_y_start;*/
										RTS_lamp_status_set(dev, NULL, SANE_FALSE, TMA_LAMP);
										/*ta_y_start =
											get_value(dev, SCAN_PARAM, TA_Y_START, 0x2508, usbfile);
										ta_y_start += (((((scan.coord.top * 3) * 5) * 5) * 32) / scancfg->resolution_x);
										if (ta_y_start >= 500)
										{
											RTS_head_relocate(dev, dev->motorcfg->highspeedmotormove, MTR_FORWARD, ta_y_start);
											scancfg->coord.top = 1;
											scancfg->origin_y = 1;
										} else
										{
											/ *5ba9* /
											if (ta_y_start > 0)
											{
												RTS_head_relocate(dev, dev->motorcfg->basespeedmotormove, MTR_FORWARD, ta_y_start);
												scancfg->coord.top = 1;
												scancfg->origin_y = 1;
											}
										}*/
									}
								}
							}

							/*5bd0*/
							usleep(1000 * 200);

							hwdcfg->scantype = scancfg->scantype;
							hwdcfg->motor_direction  = MTR_FORWARD;
							
							/* Set Origin */
							if ((scancfg->scantype >= ST_NORMAL) || (scancfg->scantype <= ST_NEG))
							{
								SANE_Int origin_x;
								if ((Lumping == 0)&&(scancfg->resolution_x == RTS_scanmode_minres(dev, scancfg->scantype, scancfg->colormode)))
									origin_x = scancfg->origin_x / 2;
										else origin_x = scancfg->origin_x;
								scancfg->coord.left += origin_x;
								scancfg->coord.top  += scancfg->origin_y;
							}

							hwdcfg->sensorevenodddistance = dev->sensorcfg->evenodd_distance;
							hwdcfg->highresolution = (scancfg->resolution_x <= 1200)? SANE_FALSE : SANE_TRUE;

							/*5c55*/
							/*
							if (dev->options->calibrate == SANE_FALSE)
							{
								SANE_Int mytop = (((scancfg->coord.top * 5) * 5) * 16) / scancfg->resolution_y;
								if ((scancfg->resolution_y <= 150)&&(mytop < 300))
								{
									scancfg->coord.top = scancfg->resolution_y / 4;
								} else
								{
									if (mytop < 100)
										scancfg->coord.top = scancfg->resolution_y / 12;
								}
							}
							*/

							/*5cd9*/
							/* setting up compression mode */
							hwdcfg->compression = compression;

							/* setting arrangeline option */
							hwdcfg->arrangeline = arrangeline;
							if (scancfg->resolution_x == 2400)
							{
								/* 5cfa*/
								if (scancfg->colormode != CM_COLOR)
								{
									if ((scancfg->colormode == CM_GRAY)&&(scancfg->channel == 3))
										hwdcfg->arrangeline = FIX_BY_SOFT;
								} else hwdcfg->arrangeline = FIX_BY_SOFT;
							}

							/*5d12*/
							/* correct x coord depending on sensor type and horizontal resolution */
							if (dev->sensorcfg->type == CCD_SENSOR)
							{
								if ((Lumping == 0)&&(scancfg->resolution_x == RTS_scanmode_minres(dev, scancfg->scantype, scancfg->colormode)))
									scancfg->coord.left += 12;
								else
								{
									/*5d3a*/
									scancfg->coord.left += 24;
									switch(scancfg->resolution_x)
									{
										case 1200: scancfg->coord.left -= 63;  break;
										case 2400: scancfg->coord.left -= 127; break;
									}
								}
							} else
							{
								/*5d5a*/
								/* CIS sensor */
								if ((Lumping == 0)&&(scancfg->resolution_x == RTS_scanmode_minres(dev, scancfg->scantype, scancfg->colormode)))
									scancfg->coord.left += 25;
								else
								{
									/*5d6d*/
									scancfg->coord.left += 50;
									switch(scancfg->resolution_x)
									{
										case 1200: scancfg->coord.left -= 63;  break;
										case 2400: scancfg->coord.left -= 127; break;
									}
								}
							}

							/* 5d92 */
							DBG(DBG_FNC, " -> RTS_scan_start xStart=%i, xExtent=%i\n", scancfg->coord.left, scancfg->coord.width);
							
							runb1 = 1;
							if (scancfg->scantype == ST_NORMAL)
							{
								/*5db7*/
								if ((scancfg->resolution_x == 1200)||(scancfg->resolution_x == 2400))
								{
									/*5e41*/
									if ((scancfg->resolution_y / 10) > scancfg->coord.top)
										runb1 = 0;
								} else
								{
									if ((scancfg->resolution_x == 600)&&(dev->usb->type == USB11)&&(scancfg->colormode == CM_COLOR))
									{
										/*5ded*/
										if ((scancfg->resolution_y / 10) > scancfg->coord.top)
											runb1 = 0;
									} else
									{
										if ((scancfg->resolution_x == 600)||(scancfg->resolution_x == 300))
										{
											/*5e11*/
											if (scancfg->resolution_y > scancfg->coord.top)
												runb1 = 0;
										} else runb1 = 0;
									}
								}
							} else
							{
								/*5e7c*/ /* entra aqu� */
								if ((scancfg->resolution_y / 10) > scancfg->coord.top)
									runb1 = 0;
							}
							/*5eb1*/
							if (runb1 == 1) /*entra */
							{
								SANE_Int val1 = scancfg->coord.top - (scancfg->resolution_y / 10);
								scancfg->coord.top -= val1;
								RTS_head_relocate(dev, dev->motorcfg->highspeedmotormove, MTR_FORWARD,
															(dev->motorcfg->resolution / scancfg->resolution_y) * val1); /*x168*/
							}

							/*5efe*/
							if (dev->options->calibrate != SANE_FALSE)
							{
								/* gamma   */
								hwdcfg->use_gamma     = dev->options->use_gamma;
								hwdcfg->gamma_depth   = dev->options->gamma_depth;

								/* shading */
								hwdcfg->white_shading = dev->options->shd_white;
								hwdcfg->black_shading = dev->options->shd_black;
								hwdcfg->unk3 = 0;

								RTS_setup(dev, Regs, scancfg, hwdcfg, &cal_data->gain_offset);

								myCalib.shading_type = 0;
								myCalib.shading->dots = min(myCalib.shading->dots, scan.shadinglength);

								if (scancfg->colormode != CM_COLOR)
								{
									if ((scancfg->channel > 0)&&(scancfg->channel < 3))
										myCalib.WRef[0] = myCalib.WRef[scancfg->channel];
								}

								RTS_regs_write(dev, Regs);

								/* apply gamma if required */
								RTS_gamma_apply(dev, Regs, scancfg, hwdcfg);

								cal_shd_apply(dev, Regs, scancfg, &myCalib);
							} else
							{
								/* don't calibrate so use default gain and offset values */
								struct st_gain_offset default_gain_offset;

								cfg_default_gainoffset_get(dev, &default_gain_offset);

								RTS_setup(dev, Regs, scancfg, hwdcfg, &default_gain_offset);
							}

							/*602a*/
							dev->options->calibrate = hwdcfg->calibrate;
							binarythresholdh = dev->gamma->bw_threshold;
							binarythresholdl = dev->gamma->bw_threshold;
							DBG(DBG_FNC, ">  bw threshold -- hi=%i, lo=%i\n", binarythresholdh, binarythresholdl);
							
							/* set threshold high */
							data_lsb_set(&Regs[0x1a0], binarythresholdh, 2);

							/* set threshold low */
							data_lsb_set(&Regs[0x19e], binarythresholdl, 2);

							/* if has motorcurves... */
							if ((Regs[0xdf] & 0x10) != 0)
								data_bitset(&Regs[0x01], 0x02, 1);

							/* Set MLOCK */
							mlock = get_value(dev, SCAN_PARAM, MLOCK, 0, usbfile) & 1;
							data_bitset(&Regs[0x00], 0x10, mlock); /*---x----*/

							if (dev->motorcfg->changemotorcurrent != SANE_FALSE)
								RTS_mtr_change(dev, Regs, RTS_mtr_get(dev, scancfg->resolution_x));

							/* set gain control mode */
							RTS_lamp_gaincontrol_set(dev, Regs, scancfg->resolution_x,
															RTS_lamp_gaincontrol_get(dev, scancfg->resolution_x, scancfg->scantype));

							RTS_scan_wait(dev, 15000);

							if (dev->status->preview == SANE_FALSE)
								cal_shd_buffers_free(&myCalib);

							/* release motor */
							RTS_mtr_release(dev);

							if (RTS_lamp_warmup_reset(dev) == SANE_STATUS_GOOD)
							{
								RTS_regs_write(dev, Regs);
								usleep(1000 * 500);

								/* let's go ! */
								if (RTS_scan_run(dev) == SANE_STATUS_GOOD)
								{
									/* disable lamp off timer */
									RTS_lamp_status_timer_set(dev, 0);

									/* Let scanner some time to store some data */
									if ((dev->chipset->model == RTS8822L_02A)&&(scancfg->resolution_x > 2400))
										usleep(1000 * 5000);

									rst = SANE_STATUS_GOOD;
								}
							}

							free (hwdcfg);
						}

						free(scancfg);
					}

					free(Regs);
				}
			}

			free(cal_data);
		}
	}

	DBG(DBG_FNC, "- RTS_scan_start: %i\n", rst);

	return rst;
}

static SANE_Status RTS_setup_motor(st_device *dev, SANE_Byte *Regs, struct st_scanparams *scancfg, SANE_Int somevalue)
{
	SANE_Status rst = SANE_STATUS_INVAL; /* default */

	DBG(DBG_FNC, "+ RTS_setup_motor(somevalue=%i):\n", somevalue);
	dbg_ScanParams(scancfg);

	if ((dev != NULL) && (Regs != NULL) && (scancfg != NULL))
	{
		SANE_Int colormode, mymode;

		DBG(DBG_FNC, "---------------> sti = %s , ste = %s\n", dbg_scantype(scancfg->scantype), dbg_scantype(scantype));

		colormode = ((scancfg->colormode != CM_COLOR)&&(scancfg->channel == 3))? 3 : scancfg->colormode;

		if ((mymode = RTS_scanmode_get(dev, scantype, colormode, scancfg->resolution_x)) != -1)
		{
			SANE_Int mbs[2] = {0}; /* motor back steps */
			SANE_Int step_size, step_type, dummyline, myvalue, lf02c;
			struct st_scanmode *sm;

			sm = dev->scanmodes[mymode];

			/* set motor step type */
			data_bitset(&Regs[0xd9], 0x70, sm->scanmotorsteptype); /*-xxx----*/

			/* set motor direction (polarity) */
			data_bitset(&Regs[0xd9], 0x80, somevalue >> 3);        /*e-------*/

			/* next value doesn't seem to have any effect */
			data_bitset(&Regs[0xd9], 0x0f, somevalue);             /*----efgh*/

			/* 0 enable/1 disable motor */
			data_bitset(&Regs[0xdd], 0x80, somevalue >> 4);        /*d-------*/

			/* next value doesn't seem to have any effect */
			data_bitset(&Regs[0xdd], 0x40, somevalue >> 4);        /*-d------*/

			switch (sm->scanmotorsteptype)
			{
				case STT_OCT  : step_type = 8; break;
				case STT_QUART: step_type = 4; break;
				case STT_HALF : step_type = 2; break;
				default       : step_type = 1; break; /* STT_FULL */
			}

			/* set dummy lines */
			if ((dummyline = sm->dummyline) == 0)
				dummyline++;

			data_bitset(&Regs[0xd6], 0xf0, dummyline); /*xxxx----*/

			/* Set if motor has curves */
			data_bitset(&Regs[0xdf], 0x10, ((sm->motorcurve != -1)? 1 : 0)); /*---x----*/

			/* set last step of deccurve.scanbufferfull table to 16 */
			data_lsb_set(&Regs[0xea], 0x10, 3);

			/* set last step of deccurve.normalscan table to 16 */
			data_lsb_set(&Regs[0xed], 0x10, 3);

			/* set last step of deccurve.smearing table to 16 */
			data_lsb_set(&Regs[0xf0], 0x10, 3);

			/* set last step of deccurve.parkhome table to 16 */
			data_lsb_set(&Regs[0xf3], 0x10, 3);

			/* set step size */
			step_size = _B0((dev->motorcfg->resolution * step_type) / (dummyline * scancfg->resolution_y));
			data_lsb_set(&Regs[0xe0], step_size - 1, 1);

			/* set line exposure time */
			myvalue = data_lsb_get(&Regs[0x30], 3);
			myvalue += ((myvalue + 1) % step_size);
			data_lsb_set(&Regs[0x30], myvalue, 3);

			/* set last step of accurve.normalscan table */
			myvalue = ((myvalue + 1) / step_size) - 1;
			data_lsb_set(&Regs[0xe1], myvalue, 3);

			/* 42b30eb */
			lf02c = 0;
			if (sm->motorcurve != -1)
			{
				if (sm->motorcurve < dev->mtrsetting_count)
				{
					struct st_motorcurve *ms = dev->mtrsetting[sm->motorcurve];
					ms->motorbackstep = sm->motorbackstep;
				}

				DBG(DBG_FNC, " -> Setting up step motor using motorcurve %i\n", sm->motorcurve);
				lf02c = RTS_mtr_setup_steps(dev, Regs, sm->motorcurve);

				/* set motor back steps */
				mbs[1] = sm->motorbackstep;
				if (mbs[1] >= (smeardeccurvecount + smearacccurvecount))
					mbs[0] = mbs[1] - (smeardeccurvecount + smearacccurvecount) + 2;
						else mbs[0] = 0;

				if (mbs[1] >= (deccurvecount + acccurvecount))
					mbs[1] -= (deccurvecount + acccurvecount) + 2;
						else mbs[1] = 0;
			} else
			{
				/* this scanner hasn't got any motorcurve */

				/* set last step of accurve.smearing table (same as accurve.normalscan) */
				data_lsb_set(&Regs[0xe4], myvalue, 3);

				/* set last step of accurve.parkhome table (same as accurve.normalscan) */
				data_lsb_set(&Regs[0xe7], myvalue, 3);

				/* both motorbacksteps are equal */
				mbs[0] = sm->motorbackstep;
				mbs[1] = sm->motorbackstep;
			}

			/* show msi and motorbacksteps */
			DBG(DBG_FNC, " -> msi            = %i\n", sm->msi);
			DBG(DBG_FNC, " -> motorbackstep1 = %i\n", mbs[0]);
			DBG(DBG_FNC, " -> motorbackstep2 = %i\n", mbs[1]);

			/* set msi */
			data_bitset(&Regs[0xda], 0xff, _B0(sm->msi)); /*xxxxxxxx*/
			data_bitset(&Regs[0xdd], 0x03, _B1(sm->msi)); /*------xx*/

			/* set motorbackstep (a) */
			data_bitset(&Regs[0xdb], 0xff, _B0(mbs[0]));  /*xxxxxxxx*/
			data_bitset(&Regs[0xdd], 0x0c, _B1(mbs[0]));  /*----xx--*/

			/* set motorbackstep (b) */
			data_bitset(&Regs[0xdc], 0xff, _B0(mbs[1]));  /*xxxxxxxx*/
			data_bitset(&Regs[0xdd], 0x30, _B1(mbs[1]));  /*--xx----*/

			/* 328b */

			/* get dummy lines count */
			dummyline = data_bitget(&Regs[0xd6], 0xf0);

			myvalue = scancfg->coord.top * (dummyline * step_size);

			if (lf02c >= myvalue)
				scancfg->coord.top = 1;
					else scancfg->coord.top -= (lf02c / (dummyline * step_size)) - 1;

			rst = lf02c; /* Result from RTS_mtr_setup_steps */
		}
	}

	DBG(DBG_FNC, "- RTS_setup_motor: %i\n", rst);

	return rst;
}

static void RTS_setup_exposure(st_device *dev, SANE_Byte *Regs, struct st_scanparams *scancfg, struct st_scanmode *sm)
{
	SANE_Int myexpt[3], linexpt, a;

	DBG(DBG_FNC, "> RTS_setup_exposure\n");

	if (dev == NULL || sm == NULL || Regs == NULL || scancfg == NULL)
		return;

	/* calculate line exposure time */
	linexpt = sm->ctpc + 1;
	if (dev->usb->type == USB11)
		linexpt *= sm->multiexposureforfullspeed;

	if (scancfg->depth > 8)
		linexpt *= sm->multiexposurefor16bitmode;

	linexpt--;

	/* generate exposure times for each channel color */
	for (a = CL_RED; a <= CL_BLUE; a++)
	{
		if ((linexpt > sm->mexpt[a]) && (sm->expt[a] == 0))
			sm->expt[a] = sm->mexpt[a];

		myexpt[a] = (sm->expt[a] == 0) ? sm->mexpt[a] : sm->expt[a];
	}

	/* save exposure times */
	DBG(DBG_FNC, "-> Exposure times : %04x, %04x, %04x\n", sm->expt[0], sm->expt[1], sm->expt[2]);
	data_lsb_set(&Regs[0x36], sm->expt[CL_RED], 3);
	data_lsb_set(&Regs[0x3c], sm->expt[CL_GREEN], 3);
	data_lsb_set(&Regs[0x42], sm->expt[CL_BLUE], 3);

	/* save maximum exposure times */
	DBG(DBG_FNC, "-> Maximum exposure times: %04x, %04x, %04x\n", sm->mexpt[0], sm->mexpt[1], sm->mexpt[2]);
	data_lsb_set(&Regs[0x33], sm->mexpt[CL_RED], 3);
	data_lsb_set(&Regs[0x39], sm->mexpt[CL_GREEN], 3);
	data_lsb_set(&Regs[0x3f], sm->mexpt[CL_BLUE], 3);

	/* save line exposure time */
	data_lsb_set(&Regs[0x30], linexpt, 3);

	/* scancfg->expt = lowest value */
	scancfg->expt = min(min(myexpt[1], myexpt[2]), myexpt[0]);
}

static SANE_Int RTS_setup_line_distances(st_device *dev, SANE_Byte *Regs, struct st_scanparams *scancfg, struct st_hwdconfig *hwdcfg, SANE_Int mycolormode, SANE_Int arrangeline)
{
	SANE_Int iLineDistance = 0;

	if ((dev != NULL) && (Regs != NULL) && (scancfg != NULL) && (hwdcfg != NULL))
	{
		if (arrangeline == FIX_BY_HARD)
		{
			/* we don't need to arrange retrieved line */
			SANE_Int mylinedistance, myevenodddist;

			mylinedistance = (dev->sensorcfg->line_distance * scancfg->resolution_y) / dev->sensorcfg->resolution;

			if (hwdcfg->highresolution == SANE_TRUE)
				myevenodddist = (hwdcfg->sensorevenodddistance * scancfg->resolution_y) / dev->sensorcfg->resolution;
					else myevenodddist = 0;

			data_bitset(&Regs[0x149], 0x3f, myevenodddist);
			data_bitset(&Regs[0x14a], 0x3f, mylinedistance);
			data_bitset(&Regs[0x14b], 0x3f, mylinedistance + myevenodddist);
			data_bitset(&Regs[0x14c], 0x3f, mylinedistance * 2);
			data_bitset(&Regs[0x14d], 0x3f, (mylinedistance * 2) + myevenodddist);
		} else
		{
			/* arrange retrieved line */
			data_bitset(&Regs[0x149], 0x3f, 0);
			data_bitset(&Regs[0x14a], 0x3f, 0);
			data_bitset(&Regs[0x14b], 0x3f, 0);
			data_bitset(&Regs[0x14c], 0x3f, 0);
			data_bitset(&Regs[0x14d], 0x3f, 0);

			if (arrangeline == FIX_BY_SOFT)
			{
				if (hwdcfg->highresolution == SANE_FALSE)
				{
					if (mycolormode == CM_COLOR)
					{
						iLineDistance = (dev->sensorcfg->line_distance * scan2.resolution_y) * 2;
						iLineDistance = (iLineDistance / dev->sensorcfg->resolution) + 1;
						if (iLineDistance < 2)
							iLineDistance = 2;
					}
				} else
				{
					/* bcc */
					if (mycolormode == CM_COLOR)
							iLineDistance = ((dev->sensorcfg->line_distance * 2) + hwdcfg->sensorevenodddistance) * scan2.resolution_y;
								else iLineDistance = dev->sensorcfg->line_distance * scan2.resolution_y;

					iLineDistance = (iLineDistance / dev->sensorcfg->resolution) + 1;
					if (iLineDistance < 2)
						iLineDistance = 2;
				}

				/* c25 */
				iLineDistance &= 0xffff;
				v15b4 = (iLineDistance > 0)? 1 : 0;
				imagesize += iLineDistance * bytesperline;
			}
		}
	}

	DBG(DBG_FNC, "> RTS_setup_line_distances(mycolormode=%i, arrangeline=%i): %i\n", mycolormode, arrangeline, iLineDistance);

	return iLineDistance;
}

static SANE_Int RTS_setup_depth(SANE_Byte *Regs, struct st_scanparams *scancfg, SANE_Int mycolormode)
{
	/* channels_per_line = channels_per_dot * scan.width
	   bytes_per_line = channels_per_line * bits_per_channel
	*/

	SANE_Int bytes_per_line, channels_per_line;

	if (scancfg == NULL || Regs == NULL)
		return 0;

	channels_per_line = data_bitget(&Regs[0x12], 0xc0) * scancfg->coord.width;
	bytes_per_line = channels_per_line;

	/* set bits per channel in shading correction's register (0x1cf) */
	if (mycolormode == CM_LINEART)
	{
		/* lineart mode */
		bytes_per_line = (bytes_per_line + 7) / 8;
		data_bitset(&Regs[0x1cf], 0x30, 3); /*--11----*/
	} else
	{
		/*f0c*/
		switch(scancfg->depth)
		{
			case 16:
				/* 16 bits per channel */
				bytes_per_line *= 2;
				data_bitset(&Regs[0x1cf], 0x30, 2); /*--10----*/
				break;
			case 12:
				/* 12 bits per channel */
				bytes_per_line *= 2;
				data_bitset(&Regs[0x1cf], 0x30, 1); /*--01----*/
				break;
			default:
				/* 8 bits per channel */
				data_bitset(&Regs[0x1cf], 0x30, 0); /*--00----*/
				break;
		}
	}

	return bytes_per_line;
}

static void RTS_setup_shading(SANE_Byte *Regs, struct st_scanparams *scancfg, struct st_hwdconfig *hwdcfg, SANE_Int bytes_per_line)
{
	SANE_Int dots_count, myvalue, myvalue2, mem_available, resolution_ratio, sensor_line_distance;
	SANE_Int channels, table_size;

	DBG(DBG_FNC, "> RTS_setup_shading(bytes_per_line=%i)\n", bytes_per_line);

	if (Regs == NULL || hwdcfg == NULL)
		return;

	myvalue = 0;
	resolution_ratio = Regs[0x0c0] & 0x1f;

	/* 50de */
	data_bitset(&Regs[0x1bf], 0x18, hwdcfg->unk3); /*---xx---*/

	/* Enable black shading correction ? */
	data_bitset(&Regs[0x1cf], 0x08, (hwdcfg->black_shading == SANE_FALSE)? 0 : 1); /*----x---*/

	/* Enable white shading correction ? */
	data_bitset(&Regs[0x1cf], 0x04, (hwdcfg->white_shading == SANE_FALSE)? 0 : 1); /*-----x--*/

	if ((hwdcfg->white_shading != SANE_FALSE)&&(hwdcfg->black_shading != SANE_FALSE)&&(hwdcfg->unk3 != 0))
		data_bitset(&Regs[0x1cf], 0x04, 0); /*-----x--*/

	table_size = 0;

	/* if hwdcfg->black_shading */
	if ((Regs[0x1cf] & 8) != 0)
		table_size = (resolution_ratio * scancfg->coord.width) * 2; /* black shading buffer size? */

	/* if hwdcfg->white_shading */
	if ((Regs[0x1cf] & 4) != 0)
		table_size += (resolution_ratio * scancfg->coord.width) * 2; /* white shading buffer size? */

	/* Regs 0x1ba, 0x1bb, 0x1bd, 0x1c0 seem to be 4 pointers
		to some buffer related to shading correction */

	Regs[0x1ba] = 0x00;
	table_size = (table_size + v160c_block_size - 1) / v160c_block_size;
	table_size = ((table_size + (mem_segment_size - 1)) / mem_segment_size) + 16; /* plus 16 segments (256 bytes) */

	Regs[0x1bf] &= 0xfe;
	Regs[0x1bb]  = _B0(table_size);
	Regs[0x1bc]  = _B1(table_size);
	Regs[0x1bf] |= _B2(table_size) & 1; /*-------x*/

	Regs[0x1bf] &= 0xf9;
	Regs[0x1bd]  =  _B0(table_size * 2);
	Regs[0x1be]  =  _B1(table_size * 2);
	Regs[0x1bf] |= (_B2(table_size * 2) & 3) << 1; /*-----xx-*/

	data_wide_bitset(&Regs[0x1c0], 0xfffff, table_size * 3);

	mem_available = mem_total - ((table_size * 3) * mem_segment_size);
	sensor_line_distance = Regs[0x14a] & 0x3f;

	/* select case channels_per_dot */
	channels = data_lsb_get(&Regs[0x12], 1) >> 6;

	switch(channels)
	{
		case 3: /* 3 channels per dot */
			/* 528d */
			dots_count = bytes_per_line / 3; /* 882 */
			myvalue = (((sensor_line_distance + 1) * dots_count) + v160c_block_size - 1) / v160c_block_size;
			myvalue2 = myvalue;
			mem_available = (mem_available - (myvalue * 3) + 2) / 3;

			printf("bytes per line: %i\n", bytes_per_line);
			printf("dots_count  : %i\n", dots_count);
			printf("myvalue     : %i\n", myvalue);
			printf("myvalue2    : %i\n", myvalue2);
			printf("table_size  : %i\n", table_size);
			printf("(table_size*3)*8: %i\n", (table_size * 3) * 8);
			
			myvalue += (table_size * 3) * 8;

			printf("myvalue+ : %i\n", myvalue);
			printf("mem_available: %i\n", mem_available);
			
			myvalue = ((myvalue * 2) + mem_available);

			printf("myvalue*2+mem: %i\n", myvalue);
			printf("myvalue/16: %i\n", myvalue / mem_segment_size);

			data_bitset(&Regs[0x1c2], 0xf0, _B2((myvalue / mem_segment_size) + 1)); /* 4 higher bits   xxxx---- */
			data_wide_bitset(&Regs[0x1c3], 0xffff, (myvalue / mem_segment_size) + 1); /* 16 lower bits */

			myvalue = myvalue + myvalue2 + mem_available;

			printf("myvalue+myvalue2+mem: %i\n", myvalue);
			
			data_wide_bitset(&Regs[0x1c5], 0xfffff, (myvalue / mem_segment_size) + 1);
			break;
		case 2: /* 2 channels per dot */
			dots_count = bytes_per_line / 2;
			myvalue = (((sensor_line_distance + 1) * dots_count) + v160c_block_size - 1) / v160c_block_size;
			mem_available = ((mem_available - myvalue) + 1) / 2;
			myvalue += (((table_size * 3) + mem_available) / mem_segment_size) + 1;

			data_bitset(&Regs[0x1c2], 0xf0, _B2(myvalue)); /* 4 higher bits   xxxx---- */
			data_wide_bitset(&Regs[0x1c3], 0xffff, myvalue); /* 16 lower bits */
			break;
		default:
			dots_count = bytes_per_line;
			break;
	}

	DBG(DBG_FNC, " -> dots_count = %i\n", dots_count);
	DBG(DBG_FNC, " -> myvalue   = %i\n", myvalue);
	DBG(DBG_FNC, " -> mem_total = %i\n", mem_total);
	DBG(DBG_FNC, " -> mem_available before dots_count= %i\n", mem_available);

	Regs[0x01c7] &= 0x0f;
	Regs[0x01c8]  = _B0((mem_total - 1) / mem_segment_size);
	Regs[0x01c9]  = _B1((mem_total - 1) / mem_segment_size);
	Regs[0x01c7] |= (_B2((mem_total - 1) / mem_segment_size) & 0x0f) << 4;
	
	mem_available -= (dots_count + v160c_block_size - 1) / v160c_block_size;
	DBG(DBG_FNC, " -> mem_available after dots_count = %i\n", mem_available);

	mem_available /= mem_segment_size;
	Regs[0x0712] &= 0x0f;
	Regs[0x0710]  = _B0(mem_available);
	Regs[0x0711]  = _B1(mem_available);
	Regs[0x0712] |= _B0(_B2(mem_available) << 4);  /*xxxx----*/

	Regs[0x0713]  = 0x00;
	Regs[0x0714]  = 0x10;
	Regs[0x0715] &= 0xf0;
}

static void RTS_setup_arrangeline(st_device *dev, struct st_hwdconfig *hwdcfg, SANE_Int colormode)
{
	if ((dev != NULL) && (hwdcfg != NULL))
	{
		dev->scanning->arrange_compression = (colormode == CM_LINEART)? SANE_FALSE : hwdcfg->compression;

		if ((colormode == CM_LINEART)||((colormode == CM_GRAY)&&(hwdcfg->highresolution == SANE_FALSE)))
			arrangeline2 = 0;
				else arrangeline2 = hwdcfg->arrangeline;

		dev->scanning->arrange_hres = hwdcfg->highresolution;
		dev->scanning->arrange_sensor_evenodd_dist = (hwdcfg->highresolution == SANE_FALSE)? 0 : hwdcfg->sensorevenodddistance;
	}
}

static void RTS_setup_channels(st_device *dev, SANE_Byte *Regs, struct st_scanparams *scancfg, SANE_Int mycolormode)
{
	DBG(DBG_FNC, "> RTS_setup_channels(colormode=%i)\n", mycolormode);

	if (dev == NULL || scancfg == NULL || Regs == NULL)
		return;

	if ((mycolormode != CM_COLOR) && (mycolormode != 3))
	{
		/* CM_GRAY || CM_LINEART */
		if (scancfg->samplerate == LINE_RATE)
		{
			/* Setting channels_per_dot to 1 */
			data_bitset(&Regs[0x12], 0xc0, 1); /*01------*/

			/* setting one rgb_channel_order */
			data_bitset(&Regs[0x12], 0x03, dev->sensorcfg->rgb_order[scancfg->channel]); /*------xx*/

			/* set sensor_channel_color_order */
			data_bitset(&Regs[0x60a], 0x3f, 6); /*--xxxxxx*/

			/* set samplerate */
			data_bitset(&Regs[0x1cf], 0x40, PIXEL_RATE); /*-x------*/

			/* set unknown data */
			data_bitset(&Regs[0x1cf], 0x80, 1); /*x-------*/

			if (scancfg->channel == dev->sensorcfg->rgb_order[1])
			{
				/* mexpts[CL_RED] = mexpts[CL_GREEN] */
				data_lsb_set(&Regs[0x33], data_lsb_get(&Regs[0x39], 3), 3);

				/* expts[CL_RED] = expts[CL_GREEN] */
				data_lsb_set(&Regs[0x36], data_lsb_get(&Regs[0x3c], 3), 3);
			} else if (scancfg->channel == dev->sensorcfg->rgb_order[2])
			{
				/* mexpts[CL_RED] = mexpts[CL_BLUE] */
				data_lsb_set(&Regs[0x33], data_lsb_get(&Regs[0x3f], 3), 3);

				/* expts[CL_RED] = expts[CL_BLUE] */
				data_lsb_set(&Regs[0x36], data_lsb_get(&Regs[0x42], 3), 3);
			}
		} else
		{
			/* e01 */
			/* setting channels_per_dot to 2 */
			data_bitset(&Regs[0x12], 0xc0, 2);

			/* set two channel color order */
			data_bitset(&Regs[0x12], 0x03, dev->sensorcfg->channel_gray[0]); /*------xx*/
			data_bitset(&Regs[0x12], 0x0c, dev->sensorcfg->channel_gray[1]); /*----xx--*/

			/* set samplerate */
			data_bitset(&Regs[0x1cf], 0x40, LINE_RATE);

			/* set unknown data */
			data_bitset(&Regs[0x1cf], 0x80, 1);
		}
	} else
	{
		/* CM_COLOR || 3 */
		/* e42 */

		/* setting channels_per_dot to 3 */
		data_bitset(&Regs[0x12], 0xc0, 3);

		/* setting samplerate*/
		data_bitset(&Regs[0x1cf], 0x40, scancfg->samplerate);

		/* set unknown data */
		data_bitset(&Regs[0x1cf], 0x80, 0);

		/* set sensor chanel_color_order */
		data_bitset(&Regs[0x60a], 0x03, dev->sensorcfg->channel_color[2]); /*------xx*/
		data_bitset(&Regs[0x60a], 0x0c, dev->sensorcfg->channel_color[1]); /*----xx--*/
		data_bitset(&Regs[0x60a], 0x30, dev->sensorcfg->channel_color[0]); /*--xx----*/

		/* set rgb_channel_order */
		data_bitset(&Regs[0x12], 0x03, dev->sensorcfg->rgb_order[0]); /*------xx*/
		data_bitset(&Regs[0x12], 0x0c, dev->sensorcfg->rgb_order[1]); /*----xx--*/
		data_bitset(&Regs[0x12], 0x30, dev->sensorcfg->rgb_order[2]); /*--xx----*/
	}
}

static SANE_Status RTS_setup(st_device *dev, SANE_Byte *Regs, struct st_scanparams *scancfg, struct st_hwdconfig *hwdcfg, struct st_gain_offset *gain_offset)
{
	SANE_Status rst = SANE_STATUS_INVAL; /* default */

	DBG(DBG_FNC, "+ RTS_setup:\n");
	dbg_ScanParams(scancfg);
	dbg_hwdcfg(hwdcfg);

	if ((dev != NULL) && (Regs != NULL) && (scancfg != NULL) && (hwdcfg != NULL) && (gain_offset != NULL))
	{
		SANE_Int lSMode;
		SANE_Byte mycolormode = scancfg->colormode;

		if (scancfg->colormode != CM_COLOR)
		{
			if (scancfg->colormode == CM_LINEART)
				scancfg->depth = 8;

			if (scancfg->channel == 3)
			{
				if (scancfg->colormode == CM_GRAY)
					mycolormode = (hwdcfg->arrangeline != FIX_BY_SOFT)? 3: CM_COLOR;
						else mycolormode = 3;
			}
		}

		/* 42b47d6 */
		RTS_scancfg_cpy(&scan2, scancfg);

		scantype = hwdcfg->scantype;
		lSMode = RTS_scanmode_get(dev, scantype, mycolormode, scancfg->resolution_x);
		if (lSMode >= 0)
		{
			struct st_scanmode *sm = dev->scanmodes[lSMode];

			if (sm != NULL)
			{
				SANE_Int dummyline, iLineDistance, resolution_ratio, bytes_per_line;
				struct st_coords rts_coords;

				iLineDistance = 0;

				scancfg->timing           = sm->timing;
				scancfg->sensorresolution = dev->timings[scancfg->timing]->sensorresolution;
				scancfg->shadinglength    = (((((scancfg->sensorresolution << 4) + scancfg->sensorresolution) >> 1) + 3) >> 2) * 4;
				scancfg->samplerate       = sm->samplerate;

				hwdcfg->motorplus         = sm->motorplus;
				
				/* set systemclock */
				data_bitset(&Regs[0x00], 0x0f, sm->systemclock);

				/* setting exposure times */
				RTS_setup_exposure(dev, Regs, scancfg, sm);

				/* setting arranges */
				RTS_setup_arrangeline(dev, hwdcfg, mycolormode);

				/* set up line distances */
				iLineDistance = RTS_setup_line_distances(dev, Regs, scancfg, hwdcfg, mycolormode, arrangeline);

				/* setup channel colors */
				RTS_setup_channels(dev, Regs, scancfg, mycolormode);

				/* setup depth */
				bytes_per_line = RTS_setup_depth(Regs, scancfg, mycolormode);

				/* Set resolution ratio */
				resolution_ratio = (scancfg->sensorresolution / scancfg->resolution_x) & 0x1f;
				data_bitset(&Regs[0xc0], 0x1f, resolution_ratio);

				/* set sensor timing values */
				RTS_setup_sensor_clocks(dev, scancfg->timing, Regs);

				data_bitset(&Regs[0xd8], 0x40, ((scantype == ST_NORMAL)? 0 : 1)); /*-x------*/

				/* Use static head ? */
				data_bitset(&Regs[0xd8], 0x80, ((hwdcfg->static_head == SANE_FALSE)? 1 : 0)); /*x-------*/

				/* Setting up gamma */
				RTS_setup_gamma(Regs, hwdcfg);

				/* setup shading correction */
				RTS_setup_shading(Regs, scancfg, hwdcfg, bytes_per_line);

				/* setup stepper motor */
				hwdcfg->startpos = RTS_setup_motor(dev, Regs, scancfg, hwdcfg->motor_direction | MTR_ENABLED);

				/* set coordinates */
				dummyline = data_bitget(&Regs[0xd6], 0xf0);

				if (scancfg->coord.left == 0) scancfg->coord.left++;
				if (scancfg->coord.top == 0) scancfg->coord.top++;

				rts_coords.left   = scancfg->coord.left  * resolution_ratio;
				rts_coords.width  = scancfg->coord.width * resolution_ratio;
				rts_coords.top    = scancfg->coord.top   * dummyline;
				rts_coords.height = ((Regs[0x14d] & 0x3f) + scancfg->coord.height + iLineDistance) * dummyline;

				if ((rts_coords.left & 1) == 0) rts_coords.left++;

				RTS_setup_coords(Regs, rts_coords.left, rts_coords.top, rts_coords.width, rts_coords.height);

				data_bitset(&Regs[0x01], 0x06, 0); /*-----xx-*/

				/* dummy_scan? */
				data_bitset(&Regs[0x01], 0x10, hwdcfg->dummy_scan); /*---x----*/

				data_bitset(&Regs[0x163], 0xc0, 1); /*xx------*/

				if (dev->scanning->arrange_compression != SANE_FALSE)
				{
					Regs[0x60b] &= 0x8f;
					data_bitset(&Regs[0x60b], 0x10, 1);      /*-001----*/
				} else data_bitset(&Regs[0x60b], 0x7f, 0); /*-0000000*/

				/* Set calibration table */
				RTS_setup_gnoff(dev, Regs, gain_offset);

				rst = SANE_STATUS_GOOD;
			}
		}
	}

	DBG(DBG_FNC, "- RTS_setup: %i\n", rst);

	return rst;
}

static void RTS_setup_coords(SANE_Byte *Regs, SANE_Int iLeft, SANE_Int iTop, SANE_Int width, SANE_Int height)
{
	DBG(DBG_FNC, "> RTS_setup_coords(*Regs, iLeft=%i, iTop=%i, width=%i, height=%i)\n", iLeft, iTop, width, height);

	if (Regs != NULL)
	{
		/* Set Left coord */
		data_lsb_set(&Regs[0xb0], iLeft, 2);

		/* Set Right coord */
		data_lsb_set(&Regs[0xb2], iLeft + width, 2);

		/* Set Top coord */
		data_lsb_set(&Regs[0xd0], iTop, 2);
		data_bitset(&Regs[0xd4], 0x0f, _B2(iTop));

		/* Set Down coord */
		data_lsb_set(&Regs[0xd2], iTop + height, 2);
		data_bitset(&Regs[0xd4], 0xf0, _B2(iTop + height));
	}
}

static void RTS_setup_gnoff(st_device *dev, SANE_Byte *Regs, struct st_gain_offset *gain_offset)
{
	DBG(DBG_FNC, "> RTS_setup_gnoff(*Regs, *gain_offset)\n");
	dbg_calibtable(gain_offset);

	if ((dev != NULL) && (Regs != NULL) && (gain_offset != NULL))
	{
		SANE_Int a;

		data_bitset(&Regs[0x13], 0x03, gain_offset->pag[CL_RED]);   /*------xx*/
		data_bitset(&Regs[0x13], 0x0c, gain_offset->pag[CL_GREEN]); /*----xx--*/
		data_bitset(&Regs[0x13], 0x30, gain_offset->pag[CL_BLUE]);  /*--xx----*/

		for (a = CL_RED; a <= CL_BLUE; a++)
		{
			/* Offsets */
			Regs[0x1a + (a * 4)] = _B0(gain_offset->edcg1[a]);
			Regs[0x1b + (a * 4)] = ((gain_offset->edcg1[a] >> 1) & 0x80) | (gain_offset->edcg2[a] & 0x7f);
			Regs[0x1c + (a * 4)] = _B0(gain_offset->odcg1[a]);
			Regs[0x1d + (a * 4)] = ((gain_offset->odcg1[a] >> 1) & 0x80) | (gain_offset->odcg2[a] & 0x7f);

			/* Variable Gain Amplifier */
			data_bitset(&Regs[0x14 + a], 0x1f, gain_offset->vgag1[a]);
			data_bitset(&Regs[0x17 + a], 0x1f, gain_offset->vgag2[a]);
		}
	}
}

static SANE_Status cal_shd_apply_buffer(st_device *dev, SANE_Byte *Regs, SANE_Int channels, struct st_calibration *myCalib, SANE_Int shtype)
{
	#define TIMES 10

	SANE_Status rst = SANE_STATUS_INVAL; /* default */

	DBG(DBG_FNC, "+ cal_shd_apply_buffer(channels=%i)\n", channels);

	if ((dev != NULL) && (Regs != NULL) && (myCalib != NULL))
	{
		if (channels > 0)
		{
			struct st_check_shading shading;

			memset(&shading, 0, sizeof(struct st_check_shading));

			/* allocate some buffers */
			if (RTS_shd_check_alloc(Regs, &shading, myCalib->shading->dots, 0x40) == SANE_STATUS_GOOD)
			{
				SANE_Int ch, transferred, dmacs, ch_color;
				SANE_Byte *ptr, **ptrbase;

				/* set params based on shading type */
				switch (shtype)
				{
					case 0:  /* black shading */
						ptrbase = myCalib->shading->black;
						dmacs = 0x10;
						break;
					default: /* white shading */
						ptrbase = myCalib->shading->white;
						dmacs = 0x14;
						break;
				}

				ch = 0;

				do
				{
					rst = SANE_STATUS_INVAL; /* default */

					ch_color = dev->sensorcfg->channel_color[ch];

					/* point to buffer to send */
					if ((ptr = *(ptrbase + ch_color) + ((myCalib->first_position - 1) * 2)) != NULL)
					{
						/* windows driver makes ten tries until data is sent successfuly */

						SANE_Byte tries = TIMES;

						do
						{
							/* send size to write */
							if (RTS_dma_write_enable(dev, ch_color | dmacs, myCalib->shading->dots * 2, 0) == SANE_STATUS_GOOD)
							{
								/* send buffer */
								if (RTS_blk_write(dev, myCalib->shading->dots * 2, ptr, &transferred) == SANE_STATUS_GOOD)
									/* check that buffer is sent properly */
									rst = RTS_shd_check(dev, &shading, Regs, ptr, ch_color);
							}

							tries--;
						} while ((tries != 0)&&(rst != SANE_STATUS_GOOD));
					} else DBG(DBG_FNC, " -> Pointer to shading buffer[%i] is null\n", ch_color);

					/* if error, cancel dma operations and exit loop*/
					if (rst != SANE_STATUS_GOOD)
						RTS_dma_cancel(dev);
							else ch++;
				} while ((ch < channels)&&(rst == SANE_STATUS_GOOD));

				/* release buffers */
				RTS_shd_check_free(&shading);
			}
		} else rst = SANE_STATUS_GOOD;
	}

	DBG(DBG_FNC, "- cal_shd_apply_buffer: %i\n", rst);

	return rst;
}

static SANE_Status cal_shd_apply(st_device *dev, SANE_Byte *Regs, struct st_scanparams *myvar, struct st_calibration *myCalib)
{
	/*
		Regs f1bc
		myvar     f020
		hwdcfg  e838
		arg4      e81c
		myCalib   e820
	*/

	SANE_Status rst = SANE_STATUS_INVAL; /* default */

	DBG(DBG_FNC, "+ cal_shd_apply(*Regs, *myvar, *mygamma, *myCalib):\n");

	if ((dev != NULL) && (Regs != NULL) && (myvar != NULL) && (myCalib != NULL))
	{
		SANE_Int myfact; /* e820 */
		SANE_Int shadata;
		SANE_Byte channels; /* f9d4 */
		SANE_Int  myShadingBase; /* e818 */

		char lf9d1;
		char lf9d0;

		dbg_ScanParams(myvar);
		
		lf9d0 = (Regs[0x60b] >> 6) & 1;
		lf9d1 = (Regs[0x60b] >> 4) & 1;
		Regs[0x060b] &= 0xaf;

		if (RTS_ctl_write_byte(dev, 0xee0b, Regs[0x060b]) == SANE_STATUS_GOOD)
		{
			SANE_Byte colormode = myvar->colormode; /*fa24*/

			if (colormode != CM_COLOR)
			{
				if (myvar->channel != 3)
				{
					if (colormode != 3)
						channels = (myvar->samplerate == PIXEL_RATE)? 2:1;
							else channels = 3;
				} else
				{
					colormode = 3;
					channels = 3;
				}
			} else channels = 3;

			/*
			White shading formula :    2000H x Target / (Wn-Dn) = White Gain data ----- for 8 times system
			White shading formula :    4000H x Target / (Wn-Dn) = White Gain data ----- for 4 times system
			For example : Target = 3FFFH   Wn = 2FFFH     Dn = 0040H  and 8 times system operation
										then   White Gain = 2000H x 3FFFH / (2FFFH-0040H) = 2AE4H (1.34033 times)
			*/
			if (myCalib->shading_postprocess != SANE_FALSE)
			{
				/* 3b46 */
				SANE_Int ch, pos;
				SANE_Int bsize = sizeof(SANE_Byte) * 2;

				DBG(DBG_FNC, "-> Shading type: %i\n", myCalib->shading_type);

				for (ch = 0; ch < channels; ch++)
				{
					myShadingBase = ((Regs[0x1cf] & 2) != 0)? 0x2000 : 0x4000;

					myfact = myCalib->WRef[ch] * myShadingBase;

					if (myCalib->shading_type == 2)
					{
						/*3bd8 */
						if ((myCalib->shading->black[ch] != NULL) && (myCalib->shading->white[ch] != NULL))
						{
							for (pos = myCalib->first_position - 1; pos < myCalib->shading->dots; pos++)
							{
								SANE_Int value = data_lsb_get(myCalib->shading->white[ch] + (pos * bsize), 2);

								shadata = (value == 0)? myShadingBase : myfact / value;
								shadata = min((shadata * shadingbase) / shadingbase, 0xff00);

								value = (value & 0xff) | (shadata & 0xff00);
								data_lsb_set(myCalib->shading->white[ch] + (pos * bsize), value, 2);
							}
						} else break;
					} else
					{
						/*3c63 */
						if (myCalib->shading_type == 3)
						{
							/*3c68*/
							if (myCalib->shading->black[ch] != NULL)
							{
								for (pos = myCalib->first_position - 1; pos < myCalib->shading->dots; pos++)
								{
									SANE_Int value = data_lsb_get(myCalib->shading->black[ch] + (pos * bsize), 2);

									shadata = (value == 0)? myShadingBase : myfact / value;
									shadata = min ((shadata * shadingbase) / shadingbase, 0xffc0);

									value = (value & 0x3f) | (shadata & 0xffc0);
									data_lsb_set(myCalib->shading->black[ch] + (pos * bsize), value, 2);
								}
							} else break;
						} else
						{
							if (myCalib->shading->white[ch] != NULL)
							{
								for (pos = 0; pos < myCalib->shading->dots; pos++)
								{
									SANE_Int value = data_lsb_get(myCalib->shading->white[ch] + (pos * bsize), 2);

									shadata = (value == 0)? myShadingBase : myfact / value;
									shadata = min((shadata * shadingbase) / shadingbase, 0xffff);

									data_lsb_set(myCalib->shading->white[ch] + (pos * bsize), shadata, 2);
								}
							} else break;
						}
					}
				}
			}

			/* if black shading correction is enabled ... */
			if ((Regs[0x1cf] & 8) != 0)
				cal_shd_apply_buffer(dev, Regs, channels, myCalib, 0);

			/* if white shading correction is enabled ... */
			if ((Regs[0x1cf] & 4) != 0)
				cal_shd_apply_buffer(dev, Regs, channels, myCalib, 1);

			data_bitset(&Regs[0x60b], 0x40, lf9d0); /*-x------*/
			data_bitset(&Regs[0x60b], 0x10, lf9d1); /*---x----*/

			rst = RTS_ctl_write_byte(dev, 0xee0b, Regs[0x060b]);
		}
	}

	DBG(DBG_FNC, "- cal_shd_apply: %i\n", rst);

	return rst;
}

static SANE_Status RTS_read_bff_size_set(st_device *dev, SANE_Int data, SANE_Int size)
{
	SANE_Status rst = SANE_STATUS_INVAL; /* default */

	DBG(DBG_FNC, "+ RTS_read_bff_size_set(data=%i, size=%i):\n", data, size);

	if (dev != NULL)
		rst = RTS_dma_read_enable(dev, 0x0008, size, data);

	DBG(DBG_FNC, "- RTS_read_bff_size_set: %i\n", rst);

	return rst;
}

static SANE_Status RTS_read_bff_wait(st_device *dev, SANE_Byte Channels_per_dot, SANE_Byte Channel_size, SANE_Int size, SANE_Int *last_amount, SANE_Int seconds, SANE_Byte op)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "+ RTS_read_bff_wait(Channels_per_dot=%i, Channel_size=%i, size=%i, seconds=%i, op=%i):\n", Channels_per_dot, Channel_size, size, seconds, op);

	if (dev != NULL)
	{
		SANE_Byte cTimeout, executing;
		SANE_Int  lastAmount, myAmount;
		long tick;

		rst = SANE_STATUS_GOOD;
		cTimeout   = SANE_FALSE;
		lastAmount = 0;

		if ((myAmount = RTS_read_bff_size_get(dev, Channels_per_dot, Channel_size)) < size)
		{
			/* Wait until scanner fills its buffer */
			if (seconds == 0)
				seconds = 10;

			tick = GetTickCount() + (seconds * 1000);

			while (cTimeout == SANE_FALSE)
			{
				myAmount = RTS_read_bff_size_get(dev, Channels_per_dot, Channel_size);
				
				/* check special case */
				if (op == SANE_TRUE)
				{
					if (((myAmount + 0x450) > size)||(RTS_scan_running(dev, &executing) == SANE_FALSE))
						break;
				}
					
				if (myAmount < size)
				{
					/* Check timeout */
					if (myAmount == lastAmount)
					{
						/* we are in timeout? */
						if (tick < GetTickCount())
						{
							/* TIMEOUT */
							rst = SANE_STATUS_INVAL;
							cTimeout = SANE_TRUE;
						} else usleep(100 * 1000);
					} else
					{
						/* Amount increased, update tick */
						lastAmount = myAmount;
						tick = GetTickCount() + (seconds * 1000);
					}
				} else
				{
					lastAmount = myAmount;
					break; /* buffer full */
				}
			}
		}

		if (last_amount != NULL)
			*last_amount = myAmount;

		DBG(DBG_FNC, " -> last_amount=%i\n", myAmount);
	}

	DBG(DBG_FNC, "- RTS_read_bff_wait: %i\n", rst);

	return rst;
}

static SANE_Status RTS_simple_scan_read(st_device *dev, SANE_Byte *buffer, struct st_scanparams *scancfg, struct st_hwdconfig *hwdcfg)
{
	/*buffer   f80c = esp+14
	  scancfg    f850 = esp+18
	  hwdcfg faac = */

	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "+ RTS_simple_scan_read:\n");

	if (dev == NULL || buffer == NULL || scancfg == NULL || hwdcfg == NULL)
	{
		DBG(DBG_FNC, "-- ASSERT failed\n");
		return SANE_STATUS_INVAL;
	}

	/*3ff6*/
	/* windows driver supposes that simple scans won't ever use compression */
	if (hwdcfg->compression == SANE_FALSE)
	{
		double dSize, transferred;
		SANE_Int myLength, itransferred, iPos;

		dSize = scancfg->bytesperline * scancfg->coord.height;
		if (scancfg->depth == 12)
			dSize = (dSize * 3) / 4;

		iPos = 0;
		dSize /= 2;
		while (dSize > 0)
		{
			itransferred = 0;
			myLength = (dSize <= dev->chipset->dma.set_length)? dSize : dev->chipset->dma.set_length;

			if (myLength > 0x1ffe0)
				myLength = 0x1ffe0;

			rst = SANE_STATUS_INVAL;
			if (RTS_read_bff_wait(dev, 0, 1, myLength * 2, NULL, 5, SANE_FALSE) == SANE_STATUS_GOOD)
			{
				if (RTS_read_bff_size_set(dev, 0, myLength * 2) == SANE_STATUS_GOOD)
					rst = RTS_blk_read(dev, myLength * 2, &buffer[iPos], &itransferred);
			}

			if (rst != SANE_STATUS_GOOD)
				break;

			iPos  += itransferred;
			dSize -= itransferred;
			transferred += itransferred * 2;
		} 

		if (rst != SANE_STATUS_GOOD)
			RTS_dma_cancel(dev);
	}

	if (rst == SANE_STATUS_GOOD)
		RTS_scan_wait(dev, 1500);
	
	DBG(DBG_FNC, "- RTS_simple_scan_read: %i\n", rst);

	return rst;
}

static SANE_Status RTS_simple_scan(st_device *dev, SANE_Byte *Regs, struct st_scanparams *scancfg, struct st_gain_offset *gain_offset, SANE_Byte *buffer, struct st_calibration *myCalib, SANE_Int options, SANE_Int gaincontrol)
{
	/* 42b8e10 */

	SANE_Status rst = SANE_STATUS_INVAL; /* default */

	DBG(DBG_FNC, "+ RTS_simple_scan(*Regs, *scancfg, *gain_offset, *buffer, myCalib, options=0x%08x, gaincontrol=%i):\n", options, gaincontrol);
	dbg_ScanParams(scancfg);

	/* validate arguments */
	if ((dev != NULL) && (Regs != NULL) && (scancfg != NULL))
	{
		if ((scancfg->coord.width != 0) && (scancfg->coord.height != 0))
		{
			/* let's make a copy of scan config*/
			struct st_scanparams *myscancfg = RTS_scancfg_dup(scancfg);

			if (myscancfg != NULL)
			{
				struct st_hwdconfig  *hwdcfg = RTS_hwdcfg_alloc(SANE_TRUE);

				if (hwdcfg != NULL)
				{
					if (((options & 2) != 0)||((_B1(options) & 1) != 0))
					{
						/* switch off lamp */
						data_bitset(&Regs[0x146], 0x40, 0);

						RTS_ctl_write_byte(dev, 0xe946, Regs[0x146]);
						usleep(1000 * ((dev->status->preview == SANE_FALSE)? 500 : 300));
					}

					hwdcfg->scantype        = scan.scantype;
					hwdcfg->use_gamma       = ((options & OP_USE_GAMMA  ) != 0)? 1: 0;
					hwdcfg->white_shading   = ((options & OP_WHITE_SHAD ) != 0)? 1: 0;
					hwdcfg->black_shading   = ((options & OP_BLACK_SHAD ) != 0)? 1: 0;
					hwdcfg->motor_direction = ((options & OP_BACKWARD   ) != 0)? MTR_BACKWARD: MTR_FORWARD;
					hwdcfg->compression     = ((options & OP_COMPRESSION) != 0)? 1: 0;
					hwdcfg->static_head     = ((options & OP_STATIC_HEAD) != 0)? 1: 0;
					hwdcfg->dummy_scan      = (buffer == NULL)? SANE_TRUE: SANE_FALSE;
					hwdcfg->arrangeline     = 0;
					hwdcfg->highresolution  = (myscancfg->resolution_x > 1200)? SANE_TRUE: SANE_FALSE;
					hwdcfg->unk3            = 0;
					hwdcfg->gamma_depth     = dev->options->gamma_depth;

					/* Set Left coord */
					myscancfg->coord.left += ((dev->sensorcfg->type == CCD_SENSOR)? 24 : 50);

					switch (myscancfg->resolution_x)
					{
							case 1200: myscancfg->coord.left -= 63; break;
							case 2400: myscancfg->coord.left -= 126; break;
					}

					if (myscancfg->coord.left < 0)
						myscancfg->coord.left = 0;

					RTS_setup(dev, Regs, myscancfg, hwdcfg, gain_offset);

					/* Setting exposure time */
					switch(scan.scantype)
					{
						case ST_NORMAL:
							if (scan.resolution_x == 100)
							{
								SANE_Byte *myRegs = RTS_regs_alloc(SANE_TRUE);

								if (myRegs != NULL)
								{
									RTS_setup(dev, myRegs, &scan, hwdcfg, gain_offset);

									data_lsb_cpy(&myRegs[0x30], &Regs[0x30], 3);

									/* Copy myregisters mexpts to Regs mexpts */
									data_lsb_cpy(&myRegs[0x33], &Regs[0x33], 3);
									data_lsb_cpy(&myRegs[0x39], &Regs[0x39], 3);
									data_lsb_cpy(&myRegs[0x3f], &Regs[0x3f], 3);

									free(myRegs);
								}
							}
							break;
						case ST_NEG:
							{
								SANE_Int myvalue;

								/* Setting exposure times for Negative scans */
								data_lsb_set(&Regs[0x30], myscancfg->expt, 3);
								data_lsb_set(&Regs[0x33], myscancfg->expt, 3);
								data_lsb_set(&Regs[0x39], myscancfg->expt, 3);
								data_lsb_set(&Regs[0x3f], myscancfg->expt, 3);

								data_lsb_set(&Regs[0x36], 0, 3);
								data_lsb_set(&Regs[0x3c], 0, 3);
								data_lsb_set(&Regs[0x42], 0, 3);

								myvalue = ((myscancfg->expt + 1) / (data_lsb_get(&Regs[0xe0], 1) + 1)) - 1;
								data_lsb_set(&Regs[0xe1], myvalue, 3);
							}
							break;
					}

					/* 91a0 */
					if (myscancfg->resolution_y > 600)
					{
						options |= 0x20000000;
						if (options != 0) /* Always true ...*/
							RTS_exposure_set(dev, Regs);
								else myscancfg->coord.top += hwdcfg->startpos;
					} else RTS_exposure_set(dev, Regs);

					/* 91e2 */
					RTS_regs_write(dev, Regs);
					if (myCalib != NULL)
						cal_shd_apply(dev, Regs, myscancfg, myCalib);

					if (dev->motorcfg->changemotorcurrent != SANE_FALSE)
						RTS_mtr_change(dev, Regs, RTS_mtr_get(dev, myscancfg->resolution_x));

					/* mlock = 0*/
					data_bitset(&Regs[0x00], 0x10, 0);

					data_wide_bitset(&Regs[0xde], 0xfff, 0);

					/* release motor */
					RTS_mtr_release(dev);

					if (RTS_lamp_warmup_reset(dev) == SANE_STATUS_GOOD)
					{
						rst = SANE_STATUS_GOOD;

						SetLock(dev, Regs, (myscancfg->depth == 16)? SANE_FALSE: SANE_TRUE);

						/* set gain control */
						RTS_lamp_gaincontrol_set(dev, Regs, myscancfg->resolution_x, gaincontrol);

						/* send registers to scanner */
						if (RTS_regs_write(dev, Regs) == SANE_STATUS_GOOD)
						{
							/* execute! */
							if (RTS_scan_run(dev) == SANE_STATUS_GOOD)
								RTS_simple_scan_read(dev, buffer, myscancfg, hwdcfg); /*92e7 */
						}

						/*92fc */
						SetLock(dev, Regs, SANE_FALSE);

						if ((options & 0x200) != 0)
						{
							/* switch on lamp */
							data_bitset(&Regs[0x146], 0x40, 1);

							RTS_ctl_write_byte(dev, 0xe946, Regs[0x146]);
							/* Wait 3 seconds */
							usleep(1000*3000);
						}

						/*9351 */
						if (dev->motorcfg->changemotorcurrent == SANE_TRUE)
							RTS_mtr_change(dev, dev->init_regs, 3);
					}

					/* free low level configuration */
					free(hwdcfg);
				}

				/* free scanning configuration */
				free(myscancfg);
			}
		}
	}

	DBG(DBG_FNC, "- RTS_simple_scan: %i\n", rst);

	return rst;
}

static SANE_Status RTS_refs_detect(st_device *dev, SANE_Byte *Regs, SANE_Int resolution_x, SANE_Int resolution_y, SANE_Int *x, SANE_Int *y)
{
	SANE_Status rst = SANE_STATUS_INVAL; /* default */

	DBG(DBG_FNC, "+ RTS_refs_detect(resolution_x=%i, resolution_y=%i):\n", resolution_x, resolution_y);

	if ((x != NULL) && (y != NULL))
	{
		*x = *y = 0; /* default */

		if ((dev != NULL) && (Regs != NULL))
		{
			struct st_scanparams *scancfg = RTS_scancfg_alloc(SANE_TRUE);

			if (scancfg != NULL)
			{
				SANE_Byte *image;

				/* set configuration to scan a little area at the top-left corner*/
				scancfg->depth         = 8;
				scancfg->colormode     = CM_GRAY;
				scancfg->channel       = CL_RED;
				scancfg->resolution_x  = resolution_x;
				scancfg->resolution_y  = resolution_y;
				scancfg->coord.left    = 4;
				scancfg->coord.width   = (resolution_x * 3) / 10;
				scancfg->coord.top     = 1;
				scancfg->coord.height  = (resolution_y * 4) / 10;
				scancfg->shadinglength = (((((resolution_x << 4) + resolution_x) >> 1) + 3) >> 2) * 4;
				scancfg->bytesperline  = scancfg->coord.width;
				scancfg->scantype      = ST_NORMAL;

				/* allocate space to store image */
				if ((image = (SANE_Byte *) malloc ((scancfg->coord.height * scancfg->coord.width) * sizeof(SANE_Byte))) != NULL)
				{
					struct st_gain_offset gain_offset;
					SANE_Int gaincontrol, pwmlamplevel_backup, C;

					gaincontrol = 0;
					if (dev->options->fixed_pwm == SANE_FALSE)
					{
						/* 3877 */
						gaincontrol = RTS_lamp_gaincontrol_get(dev, resolution_x, ST_NORMAL); /* scan.scantype */
						pwmlamplevel = 0;
						RTS_lamp_pwm_use(dev, 1);
						RTS_lamp_pwm_duty_set(dev, (gaincontrol == 0)? 0x12: 0x26);

						/* switch on flb lamp */
						RTS_lamp_status_set(dev, NULL, SANE_TRUE, FLB_LAMP);
						usleep(1000 * 2000);
					}

					/* 38d6 */
					pwmlamplevel_backup = pwmlamplevel;
					pwmlamplevel = 0;
					RTS_lamp_pwm_use(dev, 1);

					/* allocate default values for gain and offset structure */
					memset(&gain_offset, 0, sizeof(struct st_gain_offset));
					for (C = CL_RED; C <= CL_BLUE; C++)
					{
						gain_offset.pag[C]   = 3;
						gain_offset.vgag1[C] = 4;
						gain_offset.vgag2[C] = 4;
					}

					/* perform lamp warmup */
					RTS_lamp_warmup(dev, Regs, FLB_LAMP, ST_NORMAL, resolution_x);

					/* retrieve image from scanner */
					if (RTS_simple_scan(dev, Regs, scancfg, &gain_offset, image, 0, 0x20000000, gaincontrol) == SANE_STATUS_GOOD)
					{
						SANE_Int x_offset, y_offset;

						/* save image to disk if required by user */
						if (dev->options->dbg_image != SANE_FALSE)
						{
							dbg_tiff_save("pre-autoref.tiff",
												scancfg->coord.width,
												scancfg->coord.height,
												scancfg->depth,
												CM_GRAY,
												scancfg->resolution_x,
												scancfg->resolution_y,
												image,
												scancfg->coord.height * scancfg->coord.width);
						}

						/* analyze image and calculate reference position */
						if (RTS_refs_analyze(dev, scancfg, image, &y_offset, 1, &x_offset, 0) == SANE_STATUS_GOOD)
						{
							DBG(DBG_FNC, " -> Detected offsets: x=%i , y=%i\n", x_offset, y_offset);

							*y = scancfg->coord.top  + y_offset;
							*x = scancfg->coord.left + x_offset;

							rst = SANE_STATUS_GOOD;
						}
					}

					free(image);

					pwmlamplevel = pwmlamplevel_backup;
				}

				free(scancfg);
			}
		}

		DBG(DBG_FNC, " -> Detected refs: x=%i , y=%i\n", *x, *y);
	}

	DBG(DBG_FNC, "- RTS_refs_detect: %i\n", rst);

	return rst;
}

static SANE_Status RTS_refs_set(st_device *dev, SANE_Byte *Regs, struct st_scanparams *scancfg)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "+ RTS_refs_set:\n");

	if ((dev != NULL) && (Regs != NULL) && (scancfg != NULL))
	{
		SANE_Int y, x;
		struct st_autoref refcfg;

		dbg_ScanParams(scancfg);

		rst = SANE_STATUS_GOOD;

		/* get fixed references for given resolution */
		cfg_vrefs_get(dev, scancfg->resolution_x, &scancfg->origin_y, &scancfg->origin_x);

		/* get auto reference configuration */
		cfg_autoref_get(dev, &refcfg);

		if (refcfg.type != REF_NONE)
		{
			/* if reference counter is == 0 perform auto detection */
			if (RTS_refs_counter_load(dev) == 0)
			{
				DBG(DBG_FNC, " -> RTS_refs_set - Autodetection mandatory (counter == 0)\n");

				refcfg.type = REF_AUTODETECT;
			}

			switch (refcfg.type)
			{
				case REF_AUTODETECT:
					/* try to autodetect references scanning a little area */
					if (RTS_refs_detect(dev, Regs, refcfg.resolution, refcfg.resolution, &x, &y) == SANE_STATUS_GOOD)
						RTS_refs_save(dev, x, y);
							else rst = SANE_STATUS_INVAL;

					RTS_head_park(dev, SANE_TRUE, dev->motorcfg->parkhomemotormove);
					break;

				case REF_TAKEFROMSCANNER:
					/* Try to get values from scanner */
					if (RTS_refs_load(dev, &x, &y) == SANE_STATUS_INVAL)
					{
						if (RTS_refs_detect(dev, Regs, refcfg.resolution, refcfg.resolution, &x, &y) == SANE_STATUS_GOOD)
							RTS_refs_save(dev, x, y);
								else rst = SANE_STATUS_INVAL;

						RTS_head_park(dev, SANE_TRUE, dev->motorcfg->parkhomemotormove);
					}
					break;
			}

			if (rst == SANE_STATUS_GOOD)
			{
				/* values are based on resolution given by refcfg.resolution.

					offset_x and y are based on 2400 dpi so convert values to that dpi
					before adding offsets and then return to resolution given by user */

				SANE_Int lmp;

				if ((Lumping == 0)&&(scancfg->resolution_x == RTS_scanmode_minres(dev, scancfg->scantype, scancfg->colormode)))
					lmp = 2;
						else lmp = 1;

				x *= (2400 / refcfg.resolution);
				y *= (2400 / refcfg.resolution);

				scancfg->origin_x = ((x + refcfg.offset_x) * (scancfg->resolution_x * lmp)) / 2400;
				scancfg->origin_y = ((y + refcfg.offset_y) * scancfg->resolution_y) / 2400;

				DBG(DBG_FNC, " -> After SEROffset and LEROffset, xoffset = %i, yoffset =%i\n", scancfg->origin_x, scancfg->origin_y);
			}

			/* increase refs counter */
			RTS_refs_counter_inc(dev);
		}
	}

	DBG(DBG_FNC, "- RTS_refs_set: %i\n", rst);

	return rst;
}

static SANE_Status RTS_lamp_status_set(st_device *dev, SANE_Byte *Regs, SANE_Int turn_on, SANE_Int lamp)
{
	SANE_Status rst = SANE_STATUS_INVAL; /* default */

	DBG(DBG_FNC, "+ RTS_lamp_status_set(*Regs, turn_on=%i->%s, lamp=%s)\n",
	    turn_on,
	    ((((lamp - 1) | turn_on) & 1) == 1)? "Yes" : "No",
	    (lamp == FLB_LAMP)? "FLB_LAMP" : "TMA_LAMP");

	if (dev != NULL)
	{
		SANE_Byte freevar = SANE_FALSE;

		if (Regs == NULL)
		{
			Regs = RTS_regs_alloc(SANE_FALSE);

			if (Regs != NULL)
				freevar = SANE_TRUE;
		}

		if (Regs != NULL)
		{
			RTS_regs_read(dev, Regs);

			/* next op depends on chipset */
			switch (dev->chipset->model)
			{
				case RTS8822BL_03A:
					/* register 0xe946 has 2 bits and each one referres one lamp
						0x40: FLB_LAMP | 0x20 : TMA_LAMP
						if both were enabled both lamps would be switched on */
					data_bitset(&Regs[0x146], 0x20, ((lamp == TMA_LAMP)&&(turn_on == SANE_TRUE))? 1: 0); /* TMA */
					data_bitset(&Regs[0x146], 0x40, ((lamp == FLB_LAMP)&&(turn_on == SANE_TRUE))? 1: 0); /* FLB */

					data_bitset(&Regs[0x155], 0x10, (lamp != FLB_LAMP)? 1: 0);
					break;
				default:
					/* the other chipsets only use one bit to indicate when a lamp is
						switched on or not being bit 0x10 in 0xe955 who decides which lamp
						is affected */
					/* switch on lamp? yes if TMA_LAMP, else whatever turn_on says */
					data_bitset(&Regs[0x146], 0x40, ((lamp - 1) | turn_on));
					/* what lamp must be switched on? */
					if ((Regs[0x146] & 0x40) != 0)
						data_bitset(&Regs[0x155], 0x10, (lamp != FLB_LAMP)? 1: 0);
					break;
			}

			/*42b8cd1*/
			/* switch on/off lamp*/
			/*dev->init_regs[0x0146] = (dev->init_regs[0x146] & 0xbf) | (Regs[0x146] & 0x40);*/
			dev->init_regs[0x0146] = (dev->init_regs[0x146] & 0x9f) | (Regs[0x146] & 0x60); /*-xx-----*/

			/* Which lamp */
			dev->init_regs[0x0155] = Regs[0x0155];
			RTS_ctl_write_byte(dev, 0xe946, Regs[0x0146]);
			usleep(1000*200);
			RTS_ctl_write_buffer(dev, 0xe954, &Regs[0x0154], 2);
		}

		if (freevar != SANE_FALSE)
		{
			free(Regs);
			Regs = NULL;
		}
	}

	DBG(DBG_FNC, "- RTS_lamp_status_set: %i\n", rst);

	return rst;
}

static SANE_Byte RTS_lamp_gaincontrol_get(st_device *dev, SANE_Int resolution, SANE_Byte scantype)
{
	SANE_Byte ret = 0;

	if (dev != NULL)
	{
		SANE_Int mygain, iValue;

		switch(scantype)
		{
			case ST_TA:
				ret = 0;
				iValue = DPIGAINCONTROL_TA600;
				break;
			case ST_NEG:
				ret = 1;
				iValue = DPIGAINCONTROL_NEG600;
				break;
			default: /* Reflective */
				ret = 1;
				iValue = DPIGAINCONTROL600;
				break;
		}

		mygain = get_value(dev, SCAN_PARAM, iValue, ret, usbfile);
		ret = 0;

		if (scantype == ST_NORMAL)
		{
			if (dev->chipset->model == RTS8822L_02A)
			{
				switch(resolution)
				{
					case 100:
					case 150:
					case 300:
					case 600:
					case 1200:
					case 2400:
					case 4800:
						ret = ((dev->usb->type != USB11)&&(mygain != 0))? 1: 0;
						break;
				}
			} else
			{
				switch(resolution)
				{
					case 100:
					case 200:
					case 300:
					case 600:
						if (dev->usb->type != USB11)
							ret = (mygain != 0)? 1: 0;
								else ret = (resolution == 100)? 1: 0;
						break;
					case 1200:
					case 2400:
						ret = 0;
						break;
				}
			}
		} else if (scantype == ST_TA)
		{
			switch(resolution)
			{
				/*hp3970*/
				case 100:
				case 200:
				/*common*/
				case 300:
				case 600:
				case 1200:
				case 2400:
				/*hp4370*/
				case 150:
				case 4800:
					ret = ((dev->usb->type != USB11)&&(mygain != 0))? 1: 0;
					break;
			}
		} else
		{
			/* ST_NEG */
			switch(resolution)
			{
				case 100:
				case 200:
				case 300:
				case 600:
					ret = ((dev->usb->type != USB11)&&(mygain != 0))? 1: 0;
					break;
				case 1200:
				case 2400:
				case 4800: /*hp4370*/
					ret = 0;
					break;
			}
		}
	}

	DBG(DBG_FNC, "> RTS_lamp_gaincontrol_get(resolution=%i, scantype=%s): %i\n", resolution, dbg_scantype(scantype), ret);

	return ret;
}

static SANE_Status GetOneLineInfo(st_device *dev, SANE_Int scantype, SANE_Int resolution, SANE_Int *maximus, SANE_Int *minimus, double *average)
{
	SANE_Int  rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "+ GetOneLineInfo(scantype=%s, resolution=%i):\n", dbg_scantype(scantype), resolution);

	/* Check parameters */
	if (dev != NULL)
	{
		SANE_Byte *Regs = RTS_regs_dup(dev->init_regs);

		if (Regs != NULL)
		{
			struct st_scanparams *scancfg = RTS_scancfg_alloc(SANE_TRUE);

			if (scancfg != NULL)
			{
				SANE_Byte *image;
				SANE_Int a, gainmode;
				struct st_gain_offset gain_offset;

				/* Setting some registers */
				for (a = 0x192; a <= 0x19d; a++)
					Regs[a] = 0;

				/* Create calibration table */
				for (a = CL_RED; a <= CL_BLUE; a++)
				{
					gain_offset.edcg1[a] = 256;
					gain_offset.edcg2[a] = 0;
					gain_offset.odcg1[a] = 256;
					gain_offset.odcg2[a] = 0;
					gain_offset.vgag1[a] = 4;
					gain_offset.vgag2[a] = 4;
					gain_offset.pag[a]   = cfg_pag_get(dev, scan.scantype, a);
				}

				RTS_scanmode_get(dev, scantype, 0, resolution);

				/* Setting scanning params */
				scancfg->colormode     = CM_COLOR;
				scancfg->resolution_x  = resolution;
				scancfg->resolution_y  = resolution;
				scancfg->coord.left    = 100;
				scancfg->coord.width   = (resolution * 8.5) - 100;
				scancfg->coord.top     = 1;
				scancfg->coord.height  = 1;
				scancfg->depth         = 8;
				scancfg->shadinglength = (((((resolution << 4) + resolution) >> 1) + 3) >> 2) * 4;
				scancfg->v157c         = scancfg->coord.width * 3;
				scancfg->bytesperline  = scancfg->coord.width * 3;
				scancfg->scantype      = scantype;

				/* Reserve buffer for line */
				image = (SANE_Byte *) malloc(((scancfg->coord.width * 0x21) * 3) * sizeof(SANE_Byte));
				if (image != NULL)
				{
					gainmode = RTS_lamp_gaincontrol_get(dev, resolution & 0xffff, scan.scantype);
					if (RTS_simple_scan(dev, Regs, scancfg, &gain_offset, image, 0, OP_STATIC_HEAD, gainmode) != SANE_STATUS_INVAL)
					{
						/* Read all image to take max min and average colours */
						SANE_Byte *pointer1 = image;
						SANE_Byte *pointer2;
						SANE_Byte *pointer3;
						SANE_Int cmin[3]; /* min values */
						SANE_Int cmax[3]; /* max values */
						double   cave[3]; /* average values */
						SANE_Int mysize;

						if (scancfg->colormode != CM_GRAY)
						{
							pointer2 = image;
							pointer3 = image;
						} else
						{
							pointer2 = image + 1;
							pointer3 = image + 2;
						}

						for (a = CL_RED; a <= CL_BLUE; a++)
						{
							cmin[a] = 255;
							cmax[a] = 0;
							cave[a] = 0;
						}

						if (scancfg->coord.height > 0)
						{
							SANE_Int y, x;
							SANE_Byte *mypointer;
							SANE_Byte color;
							SANE_Int  desp[3];

							desp[CL_RED]   = pointer1 - pointer3;
							desp[CL_GREEN] = pointer2 - pointer3;
							desp[CL_BLUE]  = 0;

							for (y = 0; y < scancfg->coord.height; y++)
							{
								if (scancfg->coord.width > 0)
								{
									mypointer  = pointer3;

									for (x = 0; x < scancfg->coord.width; x++)
									{
										for (a = CL_RED; a <= CL_BLUE; a++)
										{
											/* Take colour values */
											color = *(mypointer + desp[a]);

											/* Take max values for each color */
											cmax[a] = max(cmax[a], color);

											/* Take min values for each color */
											cmin[a] = min(cmin[a], color);

											/* Average */
											cave[a] += color;
										}

										mypointer += 3;
									}
								}

								/* point to the pixel that is below */
								pointer1 += scancfg->coord.width * 3;
								pointer2 += scancfg->coord.width * 3;
								pointer3 += scancfg->coord.width * 3;
							}
						}

						if ((mysize = scancfg->coord.height * scancfg->coord.width) < 1)
							mysize = 1;

						for (a = CL_RED; a <= CL_BLUE; a++)
						{
							if (maximus != NULL)
								maximus[a] = cmax[a];

							if (minimus != NULL)
								minimus[a] = cmin[a];

							if (average != NULL)
								average[a] = cave[a] / mysize;
						}

						if (maximus != NULL)
							DBG(DBG_FNC, " -> max r=%3i g=%3i b=%3i\n",
									maximus[CL_RED], maximus[CL_GREEN], maximus[CL_BLUE]);

						if (minimus != NULL)
							DBG(DBG_FNC, " -> min r=%3i g=%3i b=%3i\n",
									minimus[CL_RED], minimus[CL_GREEN], minimus[CL_BLUE]);

						if (average != NULL)
							DBG(DBG_FNC, " -> avg r=%3.0f g=%3.0f b=%3.0f\n",
									average[CL_RED], average[CL_GREEN], average[CL_BLUE]);

						rst = SANE_STATUS_GOOD;
					}

					free(image);
				}

				free(scancfg);
			}

			free(Regs);
		}
	}

	DBG(DBG_FNC, "- GetOneLineInfo: %i\n", rst);

	return rst;
}

static SANE_Status RTS_lamp_pwm_checkstable(st_device *dev, SANE_Int scantype, SANE_Int resolution, SANE_Int lamp)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "+ RTS_lamp_pwm_checkstable(scantype=%s, resolution=%i, lamp=%i):\n", dbg_scantype(scantype), resolution, lamp);

	if (dev != NULL)
	{
		struct st_checkstable check;

		if ((rst = cfg_checkstable_get(dev, lamp, &check)) == SANE_STATUS_GOOD)
		{
			SANE_Int maximus[3] = {0};
			SANE_Int minimus[3] = {0};
			double   average[3] = {0};
			SANE_Int maxbigger;
			SANE_Int last_colour = 0;

			double diff = check.diff * 0.01;
			long tottime = GetTickCount() + check.tottime;

			while (GetTickCount() <= tottime)
			{
				if ((rst = GetOneLineInfo(dev, scantype, resolution, maximus, minimus, average)) == SANE_STATUS_GOOD)
				{
					/* Takes maximal colour value */
					maxbigger = max(maximus[0], max(maximus[1], maximus[2]));

					/*breaks when colour intensity increases 'diff' or lower*/
					if (abs(maxbigger - last_colour) < diff)
					{
						DBG(DBG_FNC, " -> PWM is ready\n");
						break;
					}

					last_colour = maxbigger;
				}

				usleep(1000 * check.interval);
			}
		}
	}

	DBG(DBG_FNC, "- RTS_lamp_pwm_checkstable: %i\n", rst);

	return rst;
}

static SANE_Byte RTS_refs_counter_load(st_device *dev)
{
	SANE_Byte data = 15;

	DBG(DBG_FNC, "+ RTS_refs_counter_load:\n");

	/* check if chipset supports accessing eeprom */
	if (dev != NULL)
	{
		if ((dev->chipset->capabilities & CAP_EEPROM) != 0)
			if (RTS_nvram_read_byte(dev, 0x78, &data) != SANE_STATUS_GOOD)
				data = 15;
	}

	DBG(DBG_FNC, "- RTS_refs_counter_load: %i\n", _B0(data));

	return data;
}

static SANE_Status RTS_refs_counter_save(st_device *dev, SANE_Byte data)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "+ RTS_refs_counter_save(data=%i):\n", data);

	if (dev != NULL)
	{
		/* check if chipset supports accessing eeprom */
		if ((dev->chipset->capabilities & CAP_EEPROM) != 0)
		{
			if (data > 15)
				data = 15;

			rst = RTS_nvram_write_byte(dev, 0x78, data);
		} else rst = SANE_STATUS_GOOD;
	}

	DBG(DBG_FNC, "- RTS_refs_counter_save: %i\n", rst);

	return rst;
}

static SANE_Status RTS_refs_counter_inc(st_device *dev)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "+ RTS_refs_counter_inc:\n");

	if (dev != NULL)
	{
		SANE_Byte data = RTS_refs_counter_load(dev) + 1;

		if (data >= 15)
			data = 0;

		rst = RTS_refs_counter_save(dev, data);

		DBG(DBG_FNC, " -> data=%i\n", data);
	}

	DBG(DBG_FNC, "- RTS_refs_counter_inc: %i\n", rst);

	return rst;
}

static SANE_Status RTS_head_relocate(st_device *dev, SANE_Int speed, SANE_Int direction, SANE_Int ypos)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "+ RTS_head_relocate(speed=%i, direction=%i, ypos=%i):\n", speed, direction, ypos);

	if (dev != NULL)
	{
		SANE_Byte *Regs;

		Regs = RTS_regs_dup(dev->init_regs);

		if (Regs != NULL)
		{
			struct st_motormove mymotor;
			struct st_motorpos mtrpos;

			memset(&mymotor, 0, sizeof(struct st_motormove));

			if (speed < dev->motormove_count)
				memcpy(&mymotor, dev->motormove[speed], sizeof(struct st_motormove));

			/*83fe */
			mtrpos.coord_y = ypos;
			mtrpos.options = MTR_ENABLED| ((direction == MTR_BACKWARD)? MTR_BACKWARD: MTR_FORWARD);
			mtrpos.v12e448 = 0;
			mtrpos.v12e44c = 1;

			RTS_mtr_move(dev, Regs, &mymotor, &mtrpos);
		
			/* waits 15 seconds */
			RTS_scan_wait(dev, 15000);

			free(Regs);
			rst = SANE_STATUS_GOOD;
		}
	}

	DBG(DBG_FNC, "- RTS_head_relocate: %i\n", rst);

	return rst;
}

static void cal_shd_preview_free(st_device *dev)
{
	if ((dev != NULL)&&(dev->preview != NULL)&&(dev->preview->shading != NULL))
	{
		/* free space of each buffer (both black and white) */
		SANE_Int ch;

		for (ch = 0; ch < 3; ch++)
		{
			wfree(dev->preview->shading->black[ch]);
			wfree(dev->preview->shading->white[ch]);
		}

		/* now free st_shading structure */
		free(dev->preview->shading);
		dev->preview->shading = NULL;
	}
}

static SANE_Status cal_shd_preview_alloc(st_device *dev, SANE_Int dots)
{
	SANE_Status ret = SANE_STATUS_INVAL; /* by default */

	if ((dev != NULL)&&(dots > 0))
	{
		/* free previous buffer */
		cal_shd_preview_free(dev);

		/* allocate space for preview shading */
		if ((dev->preview->shading = (struct st_shading *) malloc (sizeof(struct st_shading))) != NULL)
		{
			SANE_Byte ch;
			SANE_Int bsize = sizeof(SANE_Byte) * 2;

			ret = SANE_STATUS_GOOD; /* by default */
			dev->preview->shading->dots = dots;

			ch = 0;

			while ((ch < 3) && (ret == SANE_STATUS_GOOD))
			{
				/* First table */
				dev->preview->shading->black[ch] = (SANE_Byte *) malloc(dots * bsize);
				dev->preview->shading->white[ch] = (SANE_Byte *) malloc(dots * bsize);

				if ((dev->preview->shading->black[ch] != NULL)&&(dev->preview->shading->white[ch] != NULL))
				{
					SANE_Int pos;

					for (pos = 0; pos < dots; pos++)
					{
						data_lsb_set(dev->preview->shading->black[ch] + (pos * bsize), 0x0000, 2);
						data_lsb_set(dev->preview->shading->white[ch] + (pos * bsize), 0x4000, 2);
					}
				} else
				{
					/* some error happened allocating space */
					ret = SANE_STATUS_INVAL;
					cal_shd_preview_free(dev);
				}

				ch++;
			}
		}
	}

	DBG(DBG_FNC, "> cal_shd_preview_alloc(%i): %i\n", dots, ret);

	return ret;
}

static void cal_shd_buffers_free(struct st_calibration *caltables)
{
	DBG(DBG_FNC, "> cal_shd_buffers_free(*caltables)\n");
	
	if ((caltables != NULL)&&(caltables->shading != NULL))
	{
		SANE_Int ch;

		for (ch = 0; ch < 3; ch++)
		{
			wfree(caltables->shading->black[ch]);
			wfree(caltables->shading->white[ch]);
		}

		/* free st_shading structure */
		free(caltables->shading);
		caltables->shading = NULL;
	}
}

static SANE_Status cal_shd_buffers_alloc(st_device *dev, struct st_scanparams *scancfg, struct st_calibration *buffer)
{
	SANE_Status ret = SANE_STATUS_INVAL; /* default */

	DBG(DBG_FNC, "+ cal_shd_buffers_alloc:\n");

	if ((dev != NULL) && (scancfg != NULL) && (buffer != NULL))
	{
		SANE_Int ebp = 0x14;
		SANE_Int dots;

		ret = SANE_STATUS_GOOD;
		dots = scancfg->coord.width;

		if ((Lumping == 0)&&(scancfg->resolution_x == RTS_scanmode_minres(dev, scancfg->scantype, scancfg->colormode)))
		{
			dots *= 2;
			ebp  *= 2;
		}

		if (dev->status->preview == SANE_TRUE)
		{
			/* 673d */
			/* allocate space if necessary */
			if (dev->preview->shading == NULL)
				ret = cal_shd_preview_alloc(dev, dots);
					else if (dev->preview->shading->dots != dots)
						ret = cal_shd_preview_alloc(dev, dots);

			/* assign preview shading structure to main shading structure */
			if (ret == SANE_STATUS_GOOD)
				buffer->shading = dev->preview->shading;
		} else
		{
			/* allocate space for shading */
			if ((buffer->shading = (struct st_shading *) malloc (sizeof(struct st_shading))) != NULL)
			{
				SANE_Int ch;
				SANE_Int bsize = sizeof(SANE_Byte) * 2;

				buffer->shading->dots = dots;

				ch = 0;
				while ((ch < 3) && (ret == SANE_STATUS_GOOD))
				{
					buffer->shading->black[ch] = (SANE_Byte *) malloc(ebp + (dots * bsize));
					buffer->shading->white[ch] = (SANE_Byte *) malloc(ebp + (dots * bsize));

					if ((buffer->shading->black[ch] != NULL)&&(buffer->shading->white[ch] != NULL))
					{
						SANE_Int pos;

						for (pos = 0; pos < dots; pos++)
						{
							data_lsb_set(buffer->shading->black[ch] + (pos * bsize), 0x0000, 2);
							data_lsb_set(buffer->shading->white[ch] + (pos * bsize), 0x4000, 2);
						}
					} else
					{
						ret = SANE_STATUS_INVAL;
						cal_shd_buffers_free(buffer);
					}

					ch++;
				}
			} else ret = SANE_STATUS_INVAL;
		}

		DBG(DBG_FNC, " -> dots=%i\n", dots);
	}

	DBG(DBG_FNC, "- cal_shd_buffers_alloc: %i\n", ret);

	return ret;
}

static SANE_Status Calib_LoadConfig(st_device *dev, struct st_calibration_config *calcfg, SANE_Int scantype, SANE_Int resolution, SANE_Int bitmode)
{
	SANE_Int section, a;
	struct st_autoref refcfg;

	DBG(DBG_FNC, "> Calib_LoadConfig(*calcfg, scantype=%s, resolution=%i, bitmode=%i)\n", dbg_scantype(scantype), resolution, bitmode);
	
	switch(scantype)
	{
		case ST_NEG: section = CALIBNEGATIVEFILM; break;
		case ST_TA : section = CALIBTRANSPARENT;  break;
		default    : section = CALIBREFLECTIVE;  break;
	}

	calcfg->WStripXPos = cfg_strippos_get(dev, scantype, 0);
	calcfg->WStripYPos = cfg_strippos_get(dev, scantype, 1);
	calcfg->BStripXPos = get_value(dev, section, BSTRIPXPOS, 0, FITCALIBRATE);
	calcfg->BStripYPos = get_value(dev, section, BSTRIPYPOS, 0, FITCALIBRATE);

	/* get calibration wrefs */
	cfg_wrefs_get(dev, bitmode, resolution, scantype, &calcfg->WRef[CL_RED], &calcfg->WRef[CL_GREEN], &calcfg->WRef[CL_BLUE]);

	/* 4913 */

	for (a = CL_RED; a <= CL_BLUE; a++)
	{
		WRef[a]   = _B0(calcfg->WRef[a]);
		
		calcfg->BRef[a]        = get_value(dev, section, BREFR + a, 10, FITCALIBRATE);
		calcfg->OffsetEven1[a] = get_value(dev, section, OFFSETEVEN1R + a, 256, FITCALIBRATE);
		calcfg->OffsetEven2[a] = get_value(dev, section, OFFSETEVEN2R + a, 0, FITCALIBRATE);
		calcfg->OffsetOdd1[a]  = get_value(dev, section, OFFSETODD1R + a, 256, FITCALIBRATE);
		calcfg->OffsetOdd2[a]  = get_value(dev, section, OFFSETODD2R + a, 0, FITCALIBRATE);
	}

	calcfg->RefBitDepth    = _B0(get_value(dev, section, REFBITDEPTH, 8, FITCALIBRATE));
	calcfg->CalibOffset10n = _B0(get_value(dev, section, CALIBOFFSET10N, 3, FITCALIBRATE));
	calcfg->CalibOffset20n = _B0(get_value(dev, section, CALIBOFFSET20N, 0, FITCALIBRATE));

	/* get coordinates and size of area to calibrate offset */
	cfg_offsetcoords_get(dev, resolution, scantype, &calcfg->offset_coords);

	calcfg->OffsetNSigma         = get_value(dev, section, OFFSETNSIGMA, 2, FITCALIBRATE);
	calcfg->OffsetTargetMax      = get_value(dev, section, OFFSETTARGETMAX, 0x32, FITCALIBRATE) * 0.01;
	calcfg->OffsetTargetMin      = get_value(dev, section, OFFSETTARGETMIN, 2, FITCALIBRATE) * 0.01;
	calcfg->OffsetBoundaryRatio1 = get_value(dev, section, OFFSETBOUNDARYRATIO1, 0x64, FITCALIBRATE) * 0.01;
	calcfg->OffsetBoundaryRatio2 = get_value(dev, section, OFFSETBOUNDARYRATIO2, 0x64, FITCALIBRATE) * 0.01;

	calcfg->OffsetAvgRatio1 = get_value(dev, section, OFFSETAVGRATIO1, 0x64, FITCALIBRATE) * 0.01;
	calcfg->OffsetAvgRatio2 = get_value(dev, section, OFFSETAVGRATIO2, 0x64, FITCALIBRATE) * 0.01;
	calcfg->AdcOffQuickWay = get_value(dev, section, ADCOFFQUICKWAY, 1, FITCALIBRATE);
	calcfg->AdcOffPredictStart = get_value(dev, section, ADCOFFPREDICTSTART, 0xc8, FITCALIBRATE);
	calcfg->AdcOffPredictEnd = get_value(dev, section, ADCOFFPREDICTEND, 0x1f4, FITCALIBRATE);
	calcfg->AdcOffEvenOdd = get_value(dev, section, ADCOFFEVENODD, 1, FITCALIBRATE);
	calcfg->OffsetTuneStep1 = _B0(get_value(dev, section, OFFSETTUNESTEP1, 1, FITCALIBRATE));
	calcfg->OffsetTuneStep2 = _B0(get_value(dev, section, OFFSETTUNESTEP2, 1, FITCALIBRATE));
	calcfg->CalibGain10n = get_value(dev, section, CALIBGAIN10N, 1, FITCALIBRATE);
	calcfg->CalibGain20n = get_value(dev, section, CALIBGAIN20N, 0, FITCALIBRATE);
	calcfg->CalibPAGOn = get_value(dev, section, CALIBPAGON, 0, FITCALIBRATE);

	for (a = CL_RED; a <= CL_BLUE; a++)
	{
		calcfg->OffsetAvgTarget[a] = _B0(get_value(dev, section, OFFSETAVGTARGETR + a, 0x0d, FITCALIBRATE));
		calcfg->PAG[a]   = cfg_pag_get(dev, scantype, a);
		calcfg->Gain1[a] = get_value(dev, section, GAIN1R + a, 4, FITCALIBRATE);
		calcfg->Gain2[a] = get_value(dev, section, GAIN2R + a, 4, FITCALIBRATE);
		calcfg->WShadingPreDiff[a] = get_value(dev, section, WSHADINGPREDIFFR + a, -1, FITCALIBRATE);
		calcfg->BShadingPreDiff[a] = get_value(dev, section, BSHADINGPREDIFFR + a, 2, FITCALIBRATE);
	}

	calcfg->GainHeight = get_value(dev, section, GAINHEIGHT, 0x1e, FITCALIBRATE);
	calcfg->GainTargetFactor = get_value(dev, section, GAINTARGETFACTOR, 0x5a, FITCALIBRATE) * 0.01;
	calcfg->TotShading = get_value(dev, section, TOTSHADING, 0, FITCALIBRATE);

	/* White shading */
	calcfg->WShadingOn = get_value(dev, section, WSHADINGON, 3, FITCALIBRATE);
	calcfg->WShadingHeight = get_value(dev, section, WSHADINGHEIGHT, 0x18, FITCALIBRATE);
	
	/* Black shading */
	calcfg->BShadingOn     = get_value(dev, section, BSHADINGON, 2, FITCALIBRATE);
	calcfg->BShadingHeight = get_value(dev, section, BSHADINGHEIGHT, 0x1e, FITCALIBRATE);
	
	calcfg->BShadingDefCutOff = get_value(dev, section, BSHADINGDEFCUTOFF, 0, FITCALIBRATE);

	cfg_autoref_get(dev, &refcfg);
	calcfg->ExternBoundary = refcfg.extern_boundary * 0.01;

	calcfg->EffectivePixel = cfg_effectivepixel_get(dev, resolution);

	return SANE_STATUS_GOOD;
}

static SANE_Status RTS_cal_gain(st_device *dev, struct st_calibration_config *calcfg, struct st_calibration_data *cal_data, SANE_Int arg2, SANE_Int gaincontrol)
{
	/*
	0606F8E0   04F60738  |Arg1 = 04F60738
	0606F8E4   0606F90C  |Arg2 = 0606F90C calcfg
	0606F8E8   00000001  |Arg3 = 00000001 arg2
	0606F8EC   00000001  \Arg4 = 00000001 gaincontrol
	*/

	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "+ RTS_cal_gain(arg2=%i, gaincontrol=%i)\n", arg2, gaincontrol);

	if ((dev != NULL) && (calcfg != NULL) && (cal_data != NULL))
	{
		/* get register values to perform adc gain calibration */
		SANE_Byte *myRegs = RTS_regs_dup(cal_data->Regs);

		if (myRegs != NULL)
		{
			struct st_scanparams *scancfg = RTS_scancfg_dup(&cal_data->scancfg);

			if (scancfg != NULL)
			{
				SANE_Int bytes_per_line, bytes_to_next_colour, bytes_per_pixel;
				SANE_Byte *image, *pgain, *pcalgain;

				/* set gain control type */
				RTS_lamp_gaincontrol_set(dev, myRegs, scancfg->resolution_x, gaincontrol);

				/* 8-bit depth */
				scancfg->depth = 8;

				/* set coordinates */
				if ((scan.scantype > 0)&&(scan.scantype < 4))
					scancfg->coord.left += scancfg->origin_x;
				
				if ((scancfg->coord.width & 1) == 0)
					scancfg->coord.width++;
				
				scancfg->coord.top    = calcfg->offset_coords.top;
				scancfg->coord.height = calcfg->offset_coords.height;

				/* three more values to read image data after getting image from scanner */
				switch (scancfg->colormode)
				{
					case CM_GRAY:
					case CM_LINEART:
						bytes_to_next_colour = 0;
						bytes_per_pixel = 1;
						bytes_per_line = scancfg->coord.width;
						break;
					default: /* CM_COLOR */
						/* c027 */
						bytes_to_next_colour = 1;
						bytes_per_line = scancfg->coord.width * 3;
						if (scancfg->samplerate == LINE_RATE)
						{
							bytes_to_next_colour = scancfg->coord.width;
							bytes_per_pixel = 1;
						} else bytes_per_pixel = 3;
						break;
				}

				/*7fc7*/
				scancfg->v157c = bytes_per_line;
				scancfg->bytesperline = bytes_per_line;

				/* select type of gain parameters to set */
				if (arg2 != 0)
				{
					pgain = cal_data->gain_offset.vgag1;
					pcalgain = calcfg->Gain1;
				} else
				{
					/*7ff2*/
					pgain = cal_data->gain_offset.vgag2;
					pcalgain = calcfg->Gain2;
				}
				
				/*8002*/
				/* Allocate space for image  | size = 132912*/
				if ((image = (SANE_Byte *) malloc(sizeof(SANE_Byte) * ((scancfg->coord.height + 16) * bytes_per_line))) != NULL)
				{
					/* Lets read image */
					if (RTS_simple_scan(dev, myRegs, scancfg, &cal_data->gain_offset, image, NULL, OP_STATIC_HEAD, gaincontrol) == SANE_STATUS_GOOD)
					{
						SANE_Int a;
						SANE_Int vmin[3], vmax[3];
						double  dval[3] = {0.0};  /*f1a8 f1b0 f1b8*/
						SANE_Byte *pimage = image;

						/* initialize values */
						for (a = CL_RED; a <= CL_BLUE; a++)
						{
							calcfg->adcgain_max[a] = 0;
							calcfg->adcgain_min[a] = 255;

							vmin[a] = 255;
							vmax[a] = 0;
						}

						/* process image data */
						if (scancfg->coord.width > 0)
						{
							/*8104*/
							SANE_Int pos, myheight /*f164*/;
							SANE_Int chn_sum[3];

							for (pos = scancfg->coord.width; pos > 0; pos--)
							{
								chn_sum[CL_RED] = chn_sum[CL_GREEN] = chn_sum[CL_BLUE] = 0;

								if (scancfg->coord.height > 0)
									for (myheight = 0; myheight < scancfg->coord.height; myheight++)
										for (a = CL_RED; a <= CL_BLUE; a++)
											chn_sum[a] += *(pimage + (bytes_per_line * myheight) + (bytes_to_next_colour * a));

								/*816e*/
								for (a = CL_RED; a <= CL_BLUE; a++)
								{
									vmin[a] = min(vmin[a], chn_sum[a] / scancfg->coord.height);
									vmax[a] = max(vmax[a], chn_sum[a] / scancfg->coord.height);

									calcfg->adcgain_max[a] = max(calcfg->adcgain_max[a], vmax[a]);
									calcfg->adcgain_min[a] = min(calcfg->adcgain_min[a], vmin[a]);

									dval[a] += vmax[a] & 0xffff;
								}

								pimage += bytes_per_pixel;
							}
						}

						/*82b0*/
						dval[CL_RED]   /= scancfg->coord.width;
						dval[CL_GREEN] /= scancfg->coord.width;
						dval[CL_BLUE]  /= scancfg->coord.width;

						DBG(DBG_FNC, " -> adcgain (av/l): r=%f, g=%f, b=%f\n", dval[CL_RED], dval[CL_GREEN], dval[CL_BLUE]);
						DBG(DBG_FNC, " ->         (max ): R=%i, G=%i, B=%i\n", calcfg->adcgain_max[CL_RED], calcfg->adcgain_max[CL_GREEN], calcfg->adcgain_max[CL_BLUE]);
						DBG(DBG_FNC, " ->         (min ): r=%i, g=%i, b=%i\n", calcfg->adcgain_min[CL_RED], calcfg->adcgain_min[CL_GREEN], calcfg->adcgain_min[CL_BLUE]);

						if (scancfg->colormode == CM_COLOR)
						{
							/*8353*/
							double dvalue;
							SANE_Int ival;

							for (a = CL_RED; a <= CL_BLUE; a++)
							{
								dvalue = ((((calcfg->WRef[a] * (1 << scancfg->depth)) * calcfg->GainTargetFactor) * 0.00390625) / dval[a]) * ((44 - pgain[a]) / 40);
								if (dvalue > 0.9090909090909091)
								{
									/*83d7*/
									dvalue = min(44 - (40 / dvalue), 31);
									ival = dvalue;
									pgain[a]    = _B0(ival);
									pcalgain[a] = _B0(ival);
								} else
								{
									pgain[a]    = 0;
									pcalgain[a] = 0;
								}
							}
						} else
						{
							/*843c*/
							/*falta codigo*/
							double dvalue;
							SANE_Int ival;

							dvalue = ((44 - pgain[CL_RED]) / 40) * ((((1 << scancfg->depth) * calcfg->WRef[scancfg->channel]) * 0.9) * 0.00390625) / 17.08509389671362;
							
							for (a = CL_RED; a <= CL_BLUE; a++)
							{
								if (dvalue > 0.9090909090909091)
								{
									dvalue = min(44 - (40 / dvalue), 31);
									ival = dvalue;
									pgain[a] = _B0(ival);
									pcalgain[a] = _B0(ival);
								} else
								{
									/*84e3*/
									pgain[a] = 0;
									pcalgain[a] = 0;
								}
							}
						}
						
						/*84fa*/
						/* Save buffer */
						if (dev->options->dbg_image != SANE_FALSE)
						{
							dbg_tiff_save("adcgain.tiff",
												scancfg->coord.width,
												scancfg->coord.height,
												scancfg->depth,
												CM_COLOR,
												scancfg->resolution_x,
												scancfg->resolution_y,
												image,
												(scancfg->coord.height + 16) * bytes_per_line);
						}

						/* check if peak values are above offset average target + 5 */
						for (a = CL_RED; a <= CL_BLUE; a++)
							if (calcfg->adcgain_max[a] >= calcfg->OffsetAvgTarget[a] + 5)
							{
								rst = SANE_STATUS_GOOD;
								break;
							}
					}

					free(image);
				}

				free(scancfg);
			}

			free(myRegs);
		}

		/* v14b8 = (rst == SANE_STATUS_GOOD)? 0: 1;*/

		/* show */
		dbg_calibtable(&cal_data->gain_offset);
	}

	DBG(DBG_FNC, "- RTS_cal_gain: %i\n", rst);

	return rst;
}

static SANE_Status RTS_gnoff_save(st_device *dev, SANE_Int *offset, SANE_Byte *gain)
{
	/* save gain and offset values in nvram */

	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "+ RTS_gnoff_save(*offset, *gain):\n");

	if ((dev != NULL) && (offset != NULL) && (gain != NULL))
	{
		/* check if chipset supports accessing eeprom */
		if ((dev->chipset->capabilities & CAP_EEPROM) != 0)
		{
			if ((offset != NULL)&&(gain != NULL))
			{
				SANE_Int chn, crc, value;

				crc = 0x5B;
				for (chn = 0; chn < 3; chn++)
				{
					value = (*(gain + chn) << 9) | *(offset + chn);
					crc = _B0(abs(crc - _B0(value)));
					if ((rst = RTS_nvram_write_word(dev, 0x70 + (chn * 2), value)) != SANE_STATUS_GOOD)
						break;
				}

				if (rst == SANE_STATUS_GOOD)
					rst = RTS_nvram_write_byte(dev, 0x76, crc);
			}
		} else rst = SANE_STATUS_GOOD;
	}

	DBG(DBG_FNC, "- RTS_gnoff_save: %i\n", rst);

	return rst;
}

static SANE_Status RTS_cal_pagain(st_device *dev, struct st_calibration_config *calcfg, struct st_calibration_data *cal_data, SANE_Int gainmode)
{
	SANE_Status ret = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "+ RTS_cal_pagain(gainmode=%i)\n", gainmode);

	if ((dev != NULL) && (calcfg != NULL) && (cal_data != NULL))
	{
		SANE_Byte *Regs = RTS_regs_dup(cal_data->Regs);

		if (Regs != NULL)
		{
			struct st_scanparams *scancfg = RTS_scancfg_dup(&cal_data->scancfg);

			if (scancfg != NULL)
			{
				SANE_Int channel_size;
				SANE_Int bytes_to_next_colour = 0;
				SANE_Int bytes_per_pixel = 0;
				SANE_Int bytes_per_line = 0;
				SANE_Byte *image;

				if (scan.scantype == ST_NORMAL)
				{
					/* bfa5 */
					scancfg->coord.left  = scancfg->origin_x;
					scancfg->coord.width = (scancfg->sensorresolution * 17) / 2;
				} else
				{
					scancfg->coord.left = scancfg->origin_x + v0750;
					scancfg->coord.width = (scancfg->sensorresolution * 3) / 2;
				}

				/* bfca */
				if ((scancfg->coord.width & 1) == 1)
					scancfg->coord.width++;

				scancfg->coord.top    = calcfg->offset_coords.top;
				scancfg->coord.height = calcfg->offset_coords.height;

				channel_size = (scancfg->depth + 7) / 8;

				switch (scancfg->colormode)
				{
					case CM_GRAY:
					case CM_LINEART:
						bytes_to_next_colour = 0;
						bytes_per_pixel = 1;
						bytes_per_line = channel_size * scancfg->coord.width;
						break;
					default: /* CM_COLOR */
						/* c027 */
						bytes_to_next_colour = 1;
						bytes_per_line = (channel_size * scancfg->coord.width) * 3;
						if (scancfg->samplerate == LINE_RATE)
						{
							bytes_to_next_colour = scancfg->coord.width;
							bytes_per_pixel = 1;
						} else bytes_per_pixel = 3;
						break;
				}

				/* c070 */
				scancfg->v157c = bytes_per_line;

				/* allocate space to store image */
				if ((image = (SANE_Byte*) malloc((scancfg->coord.height * bytes_per_line) * sizeof(SANE_Byte))) != NULL)
				{
					/* scan image */
					ret = RTS_simple_scan(dev, Regs, scancfg, &cal_data->gain_offset, image, 0, OP_STATIC_HEAD, gainmode);
					if (ret == SANE_STATUS_GOOD)
					{
						int a, chn;
						SANE_Byte *ptr[3];
						int vmin[3] = {255, 255, 255}; /* f16c|f16e|f170 */
						int vmax[3] = {1, 1, 1};       /* f164|f166|f168 */
						int total[3];

						/* set pointer for each channel */
						ptr[0] = image;
						ptr[1] = image + bytes_to_next_colour;
						ptr[2] = image + (bytes_to_next_colour * 2);

						if (scancfg->coord.width > 0)
						{
							SANE_Int pos;

							for (pos = 0; pos < scancfg->coord.width; pos++)
							{
								/* reset values in all channels */
								total[0] = total[1] = total[2] = 0;

								/* sum all channels in a column */
								for (a = 0; a < scancfg->coord.height; a++)
								{
									for (chn = 0; chn < 3; chn++)
										total[chn] += *(ptr[chn] + ((scancfg->coord.height - a) * bytes_per_line));
								}

								/* get averages */
								for (chn = 0; chn < 3; chn++)
								{
									total[chn] /= scancfg->coord.height;
									vmin[chn]   = min(vmin[chn], total[chn]);
									vmax[chn]   = max(vmax[chn], total[chn]);

									/* next pixel */
									ptr[chn]   += bytes_per_pixel;
								}
							}
						}

						/* calculate gain for each channel */
						for (chn = 0; chn < 3; chn++)
						{
							double rst, coefs[3] = { 1.125, 1.286, 1.5 };
							int pag;

							rst = (calcfg->WRef[chn] * calcfg->GainTargetFactor) / vmax[chn];

							pag = 0;
							for (a = 0; a < 3; a++)
								if (rst > coefs[a])
									pag++;

							cal_data->gain_offset.pag[chn] = pag;
						}
					}
					free(image);
				}
				free(scancfg);
			}
			free(Regs);
		}
	}

	DBG(DBG_FNC, "- RTS_cal_pagain: %i\n", ret);

	return ret;
}

static SANE_Int RTS_chip_id(st_device *dev)
{
	SANE_Int ret = 0;

	if (dev != NULL)
	{
		if (RTS_ctl_read_word(dev, 0xfe3c, &ret) == SANE_STATUS_GOOD)
			ret = _B0(ret);
				else ret = 0;
	}

	DBG(DBG_FNC, "> RTS_chip_id(): %i\n", ret);

	return ret;
}

static SANE_Status RTS_chip_name(st_device *dev, char *name, SANE_Int size)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	if ((dev != NULL) && (name != NULL))
	{
		strncpy(name, dev->chipset->name, size);
		rst = SANE_STATUS_GOOD;
	}

	return rst;
}

static SANE_Status RTS_refs_load(st_device *dev, SANE_Int *x, SANE_Int *y)
{
	SANE_Status ret = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "+ RTS_refs_load:\n");

	if ((dev != NULL) && (x != NULL) && (y != NULL))
	{
		*y = *x = 0;

		/* check if chipset supports accessing eeprom */
		if ((dev->chipset->capabilities & CAP_EEPROM) != 0)
		{
			SANE_Int data;

			if (RTS_nvram_read_word(dev, 0x6a, &data) == SANE_STATUS_GOOD)
			{
				*x = data;
				if (RTS_nvram_read_word(dev, 0x6c, &data) == SANE_STATUS_GOOD)
				{
					*y = data;
					if (RTS_nvram_read_word(dev, 0x6e, &data) == SANE_STATUS_GOOD)
					{
						if ((_B0(*y + *x + data)) == 0x5a)
							ret = SANE_STATUS_GOOD;
					}
				}
			}
		} else ret = SANE_STATUS_GOOD;

		DBG(DBG_FNC, " -> y=%i, x=%i\n", *y, *x);
	}

	DBG(DBG_FNC, "- RTS_refs_load: %i\n", ret);
	
	return ret;
}

static SANE_Status RTS_refs_save(st_device *dev, SANE_Int x, SANE_Int y)
{
	SANE_Status ret = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "+ RTS_refs_save(x=%i, y=%i)\n", x, y);

	if (dev != NULL)
	{
		/* check if chipset supports accessing eeprom */
		if ((dev->chipset->capabilities & CAP_EEPROM) != 0)
		{
			if (RTS_nvram_write_word(dev, 0x6a, x) == SANE_STATUS_GOOD)
			{
				if (RTS_nvram_write_word(dev, 0x6c, y) == SANE_STATUS_GOOD)
				{
					SANE_Byte data = _B0(0x5a - (y + x));
					ret = RTS_nvram_write_byte(dev, 0x6e, data);
				}
			}
		} else ret = SANE_STATUS_GOOD;
	}

	DBG(DBG_FNC, "- RTS_refs_save: %i\n", ret);

	return ret;
}

static SANE_Status RTS_cal_offset(st_device *dev, struct st_calibration_config *calcfg, struct st_calibration_data *cal_data, SANE_Int value)
{
	SANE_Status rst = SANE_STATUS_INVAL; /* default */
	SANE_Byte *Regs;

	DBG(DBG_FNC, "+ RTS_cal_offset(*calcfg, value=%i)\n", value);
	
	if (dev == NULL || calcfg == NULL || cal_data == NULL)
	{
		DBG(DBG_FNC, "-- ASSERT failed!\n");
		return rst;
	}

	if ((Regs = RTS_regs_dup(cal_data->Regs)) != NULL)
	{
		struct st_scanparams *scancfg = RTS_scancfg_dup(&cal_data->scancfg);

		if (scancfg != NULL)
		{
			SANE_Int channels_per_dot, *pedcg, *podcg, *poffseteven, *poffsetodd;
			SANE_Int avgtarget[3];
			SANE_Byte *image;
			SANE_Int a, scan_options, gainmode;

			rst = SANE_STATUS_GOOD;

			channels_per_dot = (scancfg->colormode == CM_COLOR)? 3: 1;

			if (value != 0)
			{
				pedcg = cal_data->gain_offset.edcg1;
				podcg = cal_data->gain_offset.odcg1;
				poffseteven = calcfg->OffsetEven1;
				poffsetodd  = calcfg->OffsetOdd1;
			} else
			{
				/*c37c */
				pedcg = cal_data->gain_offset.edcg2;
				podcg = cal_data->gain_offset.odcg2;
				poffseteven = calcfg->OffsetEven2;
				poffsetodd  = calcfg->OffsetOdd2;
			}

			/*c3a4 */
			for (a = 0; a < channels_per_dot; a++)
			{
				avgtarget[a] = calcfg->OffsetAvgTarget[a] << 8;
				if (avgtarget[a] == 0)
					avgtarget[a] = 0x80;
			}

			/* set image coordinates to scan */
			memcpy(&scancfg->coord, &calcfg->offset_coords, sizeof(struct st_coords));

			if ((scancfg->coord.width & 1) == 0)
				scancfg->coord.width++;

			scancfg->bytesperline = channels_per_dot * scancfg->coord.width;
			scancfg->depth = 8;

			scan_options = (linedarlampoff == 1) ? 1 : 0x101;
			gainmode = RTS_lamp_gaincontrol_get(dev, scancfg->resolution_x, scan.scantype);

			/* allocate memory to store image */
			image = (SANE_Byte*) malloc((scancfg->bytesperline * calcfg->offset_coords.height) * sizeof(SANE_Byte));
			if (image != NULL)
			{
				SANE_Byte do_loop;
				SANE_Int off_max[3] = {255, 255, 255};
				SANE_Int off_min[3] = {0, 0, 0};
				SANE_Int off_avg[3] = {0, 0, 0};
				SANE_Int imgcount = 0;

				do
				{
					do_loop = SANE_FALSE;

					/* set current offset */
					for (a = 0; a < channels_per_dot; a++)
					{
						pedcg[a] = (((off_max[a] - off_min[a]) / 2) + off_min[a]) + 256;
						podcg[a] = pedcg[a];
					}

					/* get image */
					if (RTS_simple_scan(dev, Regs, scancfg, &cal_data->gain_offset, image, 0, scan_options, gainmode) == SANE_STATUS_GOOD)
					{
						/* save retrieved image */
						if (dev->options->dbg_image != SANE_FALSE)
						{
							char fname[30];

							imgcount++;
							if (snprintf(fname, 30, "adcoffset_%i.tiff", imgcount) > 0)
								dbg_tiff_save(fname,
													scancfg->coord.width,
													scancfg->coord.height,
													scancfg->depth,
													CM_COLOR,
													scancfg->resolution_x,
													scancfg->resolution_y,
													image,
													scancfg->bytesperline * calcfg->offset_coords.height);
						}

						/* get color averages */
						memset(&off_avg, 0, sizeof(SANE_Int) * channels_per_dot);

						for (a = 0; a < scancfg->coord.height * scancfg->coord.width; a++)
						{
							off_avg[CL_RED]   += *(image + (a * channels_per_dot));
							off_avg[CL_GREEN] += *(image + (a * channels_per_dot) + 1);
							off_avg[CL_BLUE]  += *(image + (a * channels_per_dot) + 2);
						}

						for (a = 0; a < channels_per_dot; a++)
						{
							if (abs(off_max[a] - off_min[a]) > 2)
							{
								off_avg[a] = (off_avg[a] << 8) / (scancfg->coord.height * scancfg->coord.width);

								if (off_avg[a] < avgtarget[a])
								{
									/* below target */
									off_min[a] = pedcg[a] - 256;
								} else
								{
									/* above target */
									off_max[a] = pedcg[a] - 256;
								}

								do_loop = (abs(off_max[a] - off_min[a]) > 2)? SANE_TRUE : SANE_FALSE;
							}

							if (do_loop == SANE_FALSE)
							{
								pedcg[a] = ((off_max[a] + off_min[a]) / 2) + 256;
								podcg[a] = pedcg[a];
								poffseteven[a] = pedcg[a];
								poffsetodd[a]  = pedcg[a];
							}
						}
					} else rst = SANE_STATUS_INVAL;
				} while (do_loop != SANE_FALSE);

				dbg_calibtable(&cal_data->gain_offset);

				free(image);
			} else rst = SANE_STATUS_INVAL;

			free(scancfg);
		}

		free(Regs);
	}

	DBG(DBG_FNC, "- RTS_cal_offset: %i\n", rst);

	return rst;
}

static void Calib_LoadCut(st_device *dev, struct st_scanparams *scancfg, SANE_Int scantype, struct st_calibration_config *calcfg)
{
	if ((dev != NULL) && (scancfg != NULL) && (calcfg != NULL))
	{
		double mylong; /*ee78*/
		double mylong2; /**/
		SANE_Int channel[3];
		SANE_Int a;

		cfg_shading_cut_get(dev, scancfg->depth, scancfg->resolution_x, scantype, &channel[0], &channel[1], &channel[2]);

		mylong = 1 << scancfg->depth;

		for (a = CL_RED; a <= CL_BLUE; a++)
		{
			mylong2 = channel[a];
			calcfg->ShadingCut[a] = (mylong * mylong2) * 0.000390625;
		}
	}
}

static SANE_Status Calib_BWShading(struct st_calibration_config *calcfg, struct st_calibration *myCalib, SANE_Int gainmode)
{
	/*
	0603F8E4   0603F90C  |Arg1 = 0603F90C calcfg
	0603F8E8   0603FAAC  |Arg2 = 0603FAAC myCalib
	0603F8EC   00000001  \Arg3 = 00000001 gainmode
	*/
	
	/*falta codigo*/

	/*silence gcc*/
	calcfg = calcfg;
	myCalib = myCalib;
	gainmode = gainmode;

	return SANE_STATUS_GOOD;
}

static SANE_Status RTS_cal_shd_white(st_device *dev, struct st_calibration_config *calcfg, struct st_calibration_data *cal_data, struct st_calibration *myCalib, SANE_Int gainmode)
{
	SANE_Status rst = SANE_STATUS_INVAL; /* default */

	DBG(DBG_FNC, "> RTS_cal_shd_white(*calcfg, *myCalib, gainmode=%i)\n", gainmode);

	if ((dev != NULL) && (calcfg != NULL) && (cal_data != NULL) && (myCalib != NULL))
	{
		SANE_Byte *Regs = RTS_regs_dup(cal_data->Regs);

		if (Regs != NULL)
		{
			struct st_scanparams *scancfg = RTS_scancfg_dup(&cal_data->scancfg);

			if (scancfg != NULL)
			{
				SANE_Int a, sample_width, bytes_per_pixel, bytes_per_channel, bytes_per_line, channels_per_line;
				double WhiteRef[3];
				double buffer1[2], buffer2[2];
				SANE_Byte *image, *ptr, *shptr; /*f148*/
				SANE_Int chn_pos, dot, total_lines, channel, b2_high, b1_low;
				double sumatorio;

				RTS_lamp_gaincontrol_set(dev, Regs, scancfg->resolution_x, gainmode);

				/*scancfg->resolution_y = 200;*/
				/*scancfg->resolution_y = 150;*/ /* hpg3010 */

				switch(scan.scantype)
				{
					case ST_NORMAL:
						/*a184*/
						scancfg->coord.left += scancfg->origin_x;
						scancfg->coord.width &= 0xffff;
						break;
					case ST_TA:
					case ST_NEG:
						scancfg->coord.left += scancfg->origin_x;
						break;
				}

				/*a11b*/
				if ((scancfg->coord.width & 1) != 0)
					scancfg->coord.width++;

				scancfg->coord.top = 1;
				scancfg->coord.height = calcfg->WShadingHeight;

				bytes_per_channel = ((scancfg->depth + 7) / 8);

				switch(scancfg->colormode)
				{
					case CM_GRAY:
					case CM_LINEART:
						channels_per_line = scancfg->coord.width;
						sample_width = 0;
						bytes_per_line = bytes_per_channel * channels_per_line;
						bytes_per_pixel = bytes_per_channel;
						break;
					default: /* CM_COLOR */
						channels_per_line = scancfg->coord.width * 3;
						bytes_per_line = bytes_per_channel * channels_per_line;
						sample_width = (scancfg->samplerate == LINE_RATE)? scancfg->coord.width : 1;
						bytes_per_pixel = bytes_per_channel * ((scancfg->samplerate == PIXEL_RATE)? 3: 1);
						break;
				}

				/*a1e8*/
				scancfg->v157c = bytes_per_line;
				scancfg->bytesperline = bytes_per_line;

				for (a = 0; a < 3; a++)
					WhiteRef[a] = (calcfg->WRef[a] * (1 << scancfg->depth)) >> 8;

				DBG(DBG_FNC, " -> White references: R=%f, G=%f, B=%f\n", WhiteRef[CL_RED], WhiteRef[CL_GREEN], WhiteRef[CL_BLUE]);

				total_lines = calcfg->WShadingHeight - 3;

				image = (SANE_Byte *) malloc((scancfg->coord.height * bytes_per_line) * sizeof(SANE_Byte));
				if (image != NULL)
				{
					/* Scan image */
					myCalib->shading_postprocess = SANE_FALSE;
					rst = RTS_simple_scan(dev, Regs, scancfg, &cal_data->gain_offset, image, myCalib, 0x20000080/*0x20000000*/, gainmode);

					if (rst == SANE_STATUS_GOOD)
					{
						for (a = 0; a < 3; a++)
							myCalib->WRef[a] *= ((1 << scancfg->depth) >> 8);

						/* go through all image columns */
						for (chn_pos = 0; chn_pos < bytes_per_line; chn_pos += bytes_per_channel)
						{
							switch(scancfg->colormode)
							{
								case CM_GRAY:
								case CM_LINEART:
									channel = 0;
									dot = chn_pos / bytes_per_pixel;
									break;
								default: /*CM_COLOR*/
									if (scancfg->samplerate == PIXEL_RATE)
									{
										channel = (chn_pos % bytes_per_pixel) / bytes_per_channel;
										dot = chn_pos / bytes_per_pixel;
									} else
									{
										channel = chn_pos / sample_width;
										dot = chn_pos % sample_width;
									}
									break;
							}

							for (a = 0; a < 2; a++)
							{
								buffer1[a] = 0;
								buffer2[a] = (1 << scancfg->depth) - 1.0;
							}

							b2_high = 0;
							b1_low = 0;
							ptr = image + chn_pos;
							sumatorio = 0;

							if (total_lines > 0)
							{
								double myst;
								SANE_Int line;

								for (line = 0; line < total_lines; line++)
								{
									/* take channel average from 4 lines */
									myst = 0;
									for (a = 0; a < 4; a++)
										myst += data_lsb_get(ptr + (a * bytes_per_line), bytes_per_channel);

									myst = myst / 4;

									if (line < (total_lines - 4))
									{
										if (myst < buffer2[b2_high])
										{
											buffer2[b2_high] = myst;

											for (a = 0; a < 2; a++)
												if (buffer2[b2_high] < buffer2[a])
													b2_high = a; /* takes the biggest value */
										}

										if (myst >= buffer1[b1_low])
										{
											buffer1[b1_low] = myst;

											for (a = 0; a < 2; a++)
												if (buffer1[b1_low] >= buffer1[a])
													b1_low = a;
										}
										sumatorio += myst;
									} else
									{
										if (line == (total_lines - 4))
										{
											for (a = 0; a < 2; a++)
											{
												if (buffer2[b2_high] >= buffer2[a])
													b2_high = a;

												if (buffer1[b1_low] < buffer1[a])
													b1_low = a;
											}
										}

										if (myst >= buffer2[b2_high])
										{
											sumatorio -= buffer2[b2_high];
											sumatorio += myst;
											buffer2[b2_high] = myst;

											for (a = 0; a < 2; a++)
												if (buffer2[b2_high] >= buffer2[a])
													b2_high = a;
										} else
										{
											if (myst < buffer1[b1_low])
											{
												sumatorio -= buffer1[b1_low];
												sumatorio += myst;
												buffer1[b1_low] = myst;

												for (a = 0; a < 2; a++)
													if (buffer1[b1_low] < buffer1[a])
														b1_low = a;
											}
										}
									}

									/* next line */
									ptr += bytes_per_line;
								}
							}

							if (dot < myCalib->shading->dots)
							{
								if ((total_lines - 4) != 0)
									sumatorio = sumatorio / abs(total_lines - 4);

								if (sumatorio < 1.)
									sumatorio = 1.;

								shptr = myCalib->shading->white[channel] + (dot * 2);

								if (myCalib->shading_postprocess != SANE_FALSE)
								{
									data_lsb_set(shptr, (SANE_Int) sumatorio, 2);
								} else
								{
									if ((scancfg->colormode != CM_GRAY)&&(scancfg->colormode != CM_LINEART))
										sumatorio = WhiteRef[channel] / sumatorio;
											else sumatorio = WhiteRef[scancfg->channel] / sumatorio;

									sumatorio = min(sumatorio * 16384, 65535);

									if (((Regs[0x1bf] >> 3) & 3) != 0)
									{
										SANE_Int value = data_lsb_get(shptr, 2);

										value |= (320 - (((Regs[0x1bf] >> 3) & 3) * 192)) & ((SANE_Int) sumatorio);
										data_lsb_set(shptr, value, 2);
									} else data_lsb_set(shptr, (SANE_Int) sumatorio, 2);
								}
							}
						}

						/*aa12*/
						if (dev->options->dbg_image != SANE_FALSE)
						{
							dbg_tiff_save("whiteshading.tiff",
												scancfg->coord.width,
												scancfg->coord.height,
												scancfg->depth,
												CM_COLOR,
												scancfg->resolution_x,
												scancfg->resolution_y,
												image,
												scancfg->coord.height * bytes_per_line);
						}
					}

					free(image);
				}

				free(scancfg);
			}

			free(Regs);
		}
	}

	return rst;
}

static SANE_Status RTS_cal_shd_black(st_device *dev, struct st_calibration_config *calcfg, struct st_calibration_data *cal_data, struct st_calibration *myCalib, SANE_Int gaincontrol)
{
	/*
	gainmode  f8ec
	myCalib   f8e8
	calcfg f8e4
	*/
	SANE_Status rst = SANE_STATUS_INVAL;
	SANE_Byte *Regs;

	DBG(DBG_FNC, "> RTS_cal_shd_black(gaincontrol=%i)\n", gaincontrol);

	if (dev == NULL || calcfg == NULL || cal_data == NULL || myCalib == NULL)
	{
		DBG(DBG_FNC, "-- ASSERT failed\n");
		return SANE_STATUS_INVAL;
	}

	if ((Regs = RTS_regs_dup(cal_data->Regs)) != NULL)
	{
		struct st_scanparams *scancfg = RTS_scancfg_dup(&cal_data->scancfg);

		if (scancfg != NULL)
		{
			double mylong; /*f018*/
			double maxvalue; /*eff8*/
			double sumatorio = 0.0;
			double myst;
			SANE_Int a;
			SANE_Int current_line; /*efe4*/
			SANE_Int bytes_per_line; /*efd8*/
			SANE_Int chn_pos; /*lefcc*/
			SANE_Int sample_width, bytes_per_pixel, lefd0;
			SANE_Byte *image; /*efd4*/
			SANE_Byte *ptr; /*f008*/
			SANE_Int my14b4; /*f008 pisa ptr*/
			SANE_Int biggest; /*bx*/
			SANE_Int lowest; /*dx*/
			SANE_Int dot;
			SANE_Int channel, chn_size;
			SANE_Int smvalues[3]; /*f04c f04e f050*/
			double dbvalue[6]; /*lf05c lf060, lf064 lf068, lf06c lf070,
													lf074 lf078, lf07c lf080, lf084 lf088*/

			double shd_prediff[3], shd_cut[3];

			rst = SANE_STATUS_GOOD;
	
			RTS_lamp_gaincontrol_set(dev, Regs, scancfg->resolution_x, gaincontrol);

			for (a = CL_RED; a <= CL_BLUE; a++)
				shd_prediff[a] = calcfg->BShadingPreDiff[a];

			scancfg->coord.left += scancfg->origin_x;

			if ((scancfg->coord.width & 1) != 0)
				scancfg->coord.width++;
			
			scancfg->coord.top = 1;
			/*scancfg->depth = 8;*/
			scancfg->coord.height = calcfg->BShadingHeight;
			
			if (scancfg->colormode != CM_COLOR)
			{
				bytes_per_line = scancfg->coord.width;
				sample_width = 0;
				bytes_per_pixel = 1;
			} else
			{
				/*876c*/
				bytes_per_line = scancfg->coord.width * 3;
				if (scancfg->samplerate == LINE_RATE)
				{
					sample_width = scancfg->coord.width;
					bytes_per_pixel = 1;
				} else
				{
					sample_width = 1;
					bytes_per_pixel = 3;
				}
			}
			
			scancfg->v157c = bytes_per_line;
			scancfg->bytesperline = bytes_per_line;

			mylong = 1 << (16 - scancfg->depth);

			if (((Regs[0x1bf] >> 3) & 3) != 0)
				mylong = mylong / (1 << (((Regs[0x1bf] >> 5) & 3) + 4));

			lefd0 = (((Regs[0x1bf] >> 3) & 2) << 8) - (((Regs[0x1bf] >> 3) & 1) * 0xc0);
			lefd0--; /* ffff */
			if (lefd0 < 0)
				lefd0 = 0x10000 + lefd0;

			chn_size = (scancfg->depth + 7) / 8;

			if (scancfg->depth >= 8)
				maxvalue = ((1 << (scancfg->depth - 8)) << 8) - 1; /* ff */
					else maxvalue = (256 / (1 << (8 - scancfg->depth))) - 1;

			Calib_LoadCut(dev, scancfg, scan.scantype, calcfg);
			for (a = CL_RED; a <= CL_BLUE; a++)
				shd_cut[a] = calcfg->ShadingCut[a];

			if (calcfg->BShadingOn == -1)
			{
				SANE_Int value;
				double e;
				
				for (a = CL_RED; a <= CL_BLUE; a++)
				{
					myst = max(shd_cut[a], 0);
					myst = (maxvalue >= myst)? shd_cut[a]: maxvalue;
					shd_cut[a] = max(myst, 0);
				}
				
				chn_pos = 0;
				while (chn_pos < bytes_per_line)
				{
					if (scancfg->colormode != CM_COLOR)
					{
						channel = 0;
						dot = chn_pos;
					} else
					{
						if (scancfg->samplerate == PIXEL_RATE)
						{
							channel = chn_pos % bytes_per_pixel;
							dot = chn_pos / bytes_per_pixel;
						} else
						{
							channel = chn_pos / sample_width;
							dot = chn_pos % sample_width;
						}
					}
					/*89d0*/
					e = min(lefd0, mylong * shd_cut[channel]);

					value = data_lsb_get(myCalib->shading->black[channel] + (dot * 2), 2) | (SANE_Int) e;

					data_lsb_set(myCalib->shading->black[channel] + (dot * 2), value, 2);

					chn_pos++;
				}

				return SANE_STATUS_GOOD;
			}

			/* Allocate buffer to read image */
			if ((image = (SANE_Byte *) malloc(((scancfg->coord.height + 16) * bytes_per_line) * sizeof(SANE_Byte))) != NULL)
			{
				/* Turn off lamp*/
				RTS_lamp_status_set(dev, NULL, SANE_FALSE, FLB_LAMP);
				usleep(200 * 1000);

				/* Scan image */
				myCalib->shading_postprocess = SANE_FALSE;
				rst = RTS_simple_scan(dev, Regs, scancfg, &cal_data->gain_offset, image, myCalib, 0x101, gaincontrol);

				/* Turn on lamp again */
				if (scan.scantype != ST_NORMAL)
				{
					RTS_lamp_status_set(dev, NULL, SANE_FALSE, TMA_LAMP);
					usleep(1000 * 1000);
				} else RTS_lamp_status_set(dev, NULL, SANE_TRUE, FLB_LAMP);

				if (rst != SANE_STATUS_INVAL)
				{
					double *sums;

					/* Save image */
					if (dev->options->dbg_image != SANE_FALSE)
					{
						dbg_tiff_save("blackshading.tiff",
											scancfg->coord.width,
											scancfg->coord.height,
											scancfg->depth,
											CM_COLOR,
											scancfg->resolution_x,
											scancfg->resolution_y,
											image,
											scancfg->coord.height * bytes_per_line);
					}

					/*
						allocates memory for unknown purpose
						size ede4  = bytes_per_line * 8
						pone buffer a 0
					*/
					if ((sums = (double *) calloc (1, bytes_per_line * sizeof(double))) != NULL)
					{
						/*8eb6*/
						memset(&dbvalue, 0, 6 * sizeof(double));
						chn_pos = 0;

						if (bytes_per_line > 0)
						{
							SANE_Byte *coltimes = calloc(1, ((1 << scancfg->depth) - 1) * sizeof(SANE_Byte));

							if (coltimes != NULL)
							{
								do
								{
									SANE_Int value;
									SANE_Int color;

									sumatorio = 0;
									/* ptr points to the next chn_pos of the first line */
									ptr = image + (chn_pos * chn_size);
									biggest = 0;
									lowest = (int)maxvalue;
									current_line = 0;

									/* Toma los valores de una columna*/
									if (scancfg->coord.height > 0)
									{
										do
										{
											color = data_lsb_get(ptr, chn_size);

											biggest = max(biggest, color);

											if (dev->status->preview == SANE_FALSE)
											{
												/*8fd7*/
												if (current_line < scancfg->coord.height)
												{
													sumatorio += color;

													lowest  = min(lowest, color);
													biggest = max(biggest, color);

													data_lsb_inc(coltimes + (color * chn_size), 1, chn_size);
												} else
												{
													/*9011*/
													if (color > lowest)
													{
														sumatorio += color;

														data_lsb_inc(coltimes + (lowest * chn_size), -1, chn_size);
														data_lsb_inc(coltimes + (color  * chn_size),  1, chn_size);

														sumatorio -= lowest;

														if (data_lsb_get(coltimes + (lowest * chn_size), chn_size) == 0)
														{
															do
															{
																lowest++;
															} while (data_lsb_get(coltimes + (lowest * chn_size), chn_size) == 0);
														}
													}
												}
											} else
											{
												/* preview mode */
												sumatorio += color;
												data_lsb_inc(coltimes + (color  * chn_size),    1, chn_size);
											}
											/*9067*/
											/* Point to the next pixel under current line*/
											ptr += bytes_per_line;
											current_line++;
										} while (current_line < scancfg->coord.height);
									}

									/*908a*/
									/* Calculates average of each column */
									sumatorio = sumatorio / scancfg->coord.height;
									if (sumatorio < 1.)
										sumatorio = 1.;

									if (scancfg->colormode != CM_COLOR)
									{
										channel = 0;
										dot = chn_pos;
									} else
									{
										/*90c5*/
										if (scancfg->samplerate == PIXEL_RATE)
										{
											channel = chn_pos % (bytes_per_pixel / chn_size);
											dot = (chn_pos / (bytes_per_pixel / chn_size)) & 0xffff;
										} else
										{
											/*90fb*/
											channel = chn_pos / sample_width;
											dot = chn_pos % sample_width;
										}
									}

									/*911f*/
									/* save all sumatories */
									sums[(channel * scancfg->coord.width) + dot] = sumatorio;

									dbvalue[channel] += sumatorio;
									if (scancfg->colormode == CM_COLOR)
										sumatorio += shd_cut[channel];
											else sumatorio += shd_cut[scancfg->channel];

									/*9151*/
									/* myst must be between 0 and maxvalue */
									myst = min(max(0, sumatorio), maxvalue);

									/*9198*/
									if (chn_pos >= 3)
									{
										double myst2;
										double st;

										myst -= dbvalue[channel + 3];
										myst2 = myst;
										myst = min(myst, shd_prediff[channel]);

										st = -shd_prediff[channel];
										if (myst >= st)
											myst = min(myst2, shd_prediff[channel]);
												else myst = st;

										myst += dbvalue[channel + 3];
									}

									/*9203*/
									dbvalue[channel + 3] = myst;

									if (dot < myCalib->shading->dots)
									{
										switch (calcfg->BShadingOn)
										{
											case 1:
												value = data_lsb_get(myCalib->shading->black[channel] + (dot * 2), 2);

												value |= ((SANE_Int) myst & 0xff) << 8;

												data_lsb_set(myCalib->shading->black[channel] + (dot * 2), value, 2);
												break;
											case 2:
												/*9268*/
												my14b4 = calcfg->BRef[channel] / (1 << (8 - scancfg->depth));
												myst -= my14b4;
												myst = max(myst, 0);
												myst *= mylong;

												data_lsb_set(myCalib->shading->black[channel] + (dot * 2), min(myst, lefd0), 2);
												break;
										}
									}
									/*92d8*/
									chn_pos++;
								} while (chn_pos < (bytes_per_line / chn_size));

								free(coltimes);
							}
						}

						free(sums);
					} else rst = SANE_STATUS_INVAL;

					/*9306*/
					if (calcfg->BShadingOn == -2)
					{
						for (channel = 0; channel < 3; channel++)
						{
							dbvalue[channel] = (dbvalue[channel] / scancfg->coord.width) + calcfg->ShadingCut[channel];
							if (dbvalue[channel] < 0)
								dbvalue[channel] = 0;
							smvalues[channel] = min((SANE_Int)(dbvalue[channel] + 0.5), maxvalue);
						}
						
						if (scancfg->coord.width > 0)
						{
							for (dot = 0; dot < scancfg->coord.width; dot++)
								for (channel = 0; channel < 3; channel++)
								{
									SANE_Int value = data_lsb_get(myCalib->shading->black[channel] + (dot * 2), 2);
									value |= (SANE_Int) min(smvalues[channel] * mylong, lefd0);

									data_lsb_set(myCalib->shading->black[channel] + (dot * 2), value, 2);
								}
						}
					}
				} else RTS_regs_cpy(cal_data->Regs, Regs);

				free(image);
			}

			free(scancfg);
		}

		free(Regs);
	}

	return rst;
}

static SANE_Status Calibration(st_device *dev, SANE_Byte *Regs, struct st_scanparams *scancfg, struct st_calibration_data *cal_data, struct st_calibration *myCalib, SANE_Int value)
{
	struct st_calibration_config calcfg; /* f90c*/
	SANE_Int a;
	SANE_Byte gaincontrol;
	SANE_Status lf900;

	DBG(DBG_FNC, "> Calibration\n");

	value = value; /*silence gcc*/

	if (dev == NULL || Regs == NULL || scancfg == NULL || cal_data == NULL || myCalib == NULL)
		return SANE_STATUS_GOOD;

	dbg_ScanParams(scancfg);

	memset(&calcfg, 0x30, sizeof(struct st_calibration_config));
	Calib_LoadConfig(dev, &calcfg, scan.scantype, scancfg->resolution_x, scancfg->depth);

	memset(&cal_data->gain_offset, 0, sizeof(struct st_gain_offset));
	for (a = CL_RED; a <= CL_BLUE; a++)
	{
		myCalib->WRef[a] = calcfg.WRef[a];

		cal_data->gain_offset.edcg1[a] = 256;
		cal_data->gain_offset.odcg1[a] = 256;
		cal_data->gain_offset.vgag1[a] = 4;
		cal_data->gain_offset.vgag2[a] = 4;
	}

	/* update calibration config */
	RTS_regs_cpy(cal_data->Regs, Regs);
	RTS_scancfg_cpy(&cal_data->scancfg, scancfg);

	gaincontrol = RTS_lamp_gaincontrol_get(dev, scancfg->resolution_x, scan.scantype); /* [lf904] = 1 */

	if ((Lumping == 0)&&(scancfg->resolution_x == RTS_scanmode_minres(dev, scancfg->scantype, scancfg->colormode)))
	{
		/* duplicate resolution, so duplicate coords */
		cal_data->scancfg.resolution_x *= 2;
		cal_data->scancfg.resolution_y *= 2;
		cal_data->scancfg.coord.width  *= 2;
		cal_data->scancfg.coord.height *= 2;
		cal_data->scancfg.coord.top    *= 2;
		cal_data->scancfg.coord.left   *= 2;

		cal_data->scancfg.v157c        *= 2;
		cal_data->scancfg.bytesperline *= 2;
	}

	/* 3cf3 */
	myCalib->first_position = 1;
	myCalib->shading_type = 0;
	/*myCalib->shading_dots = cal_data->scancfg.coord.width;*/

	if (cal_data->scancfg.colormode == CM_LINEART)
	{
		cal_data->scancfg.colormode = CM_GRAY;
		calcfg.GainTargetFactor = 1.3;
	}

	lf900 = SANE_STATUS_GOOD;   /*SANE_STATUS_GOOD*/

	/* PAG Calibration */
	DBG(DBG_FNC, " -> PAG calibration: %s\n", (calcfg.CalibPAGOn != 0)? "ON" : "OFF");

	if (calcfg.CalibPAGOn == 0)
	{
		/* PAG disabled, set default values according to color mode */
		if (cal_data->scancfg.colormode == CM_COLOR)
		{
			for (a = CL_RED; a <= CL_BLUE; a++)
				cal_data->gain_offset.pag[a] = calcfg.PAG[a];
		} else
		{
			/* gray and lineart */
			if (cal_data->scancfg.channel > 2)
				cal_data->scancfg.channel = CL_RED;

			for (a = CL_RED; a <= CL_BLUE; a++)
				cal_data->gain_offset.pag[a] = calcfg.PAG[cal_data->scancfg.channel];
		}
	} else lf900 = RTS_cal_pagain(dev, &calcfg, cal_data, gaincontrol);

	/* AdcOffset 1 Calibration */
	DBG(DBG_FNC, " -> AdcOffset 1 calibration: %s\n", (calcfg.CalibOffset10n != 0)? "ON" : "OFF");

	if (calcfg.CalibOffset10n != 0) /*==2*/
	{
		/* if we are performing a preview, try to avoid calibration */
		if ((dev->status->preview != SANE_FALSE)&&(dev->preview->offset[CL_RED] != 0)&&(dev->preview->offset[CL_GREEN] != 0)&&(dev->preview->offset[CL_BLUE] != 0))
		{
			/* offset values taken from eeprom if posible */
			for (a = CL_RED; a <= CL_BLUE; a++)
			{
				cal_data->gain_offset.edcg1[a] = dev->preview->offset[a];
				cal_data->gain_offset.odcg1[a] = dev->preview->offset[a];
			}
		} else
		{
			/* perform offset calibration */
			lf900 = RTS_cal_offset(dev, &calcfg, cal_data, 1);
		}
	} else
	{
		/* calibration disabled, set default values */
		for (a = CL_RED; a <= CL_BLUE; a++)
		{
			cal_data->gain_offset.edcg1[a] = abs(calcfg.OffsetEven1[a] - 0x100);
			cal_data->gain_offset.odcg1[a] = abs(calcfg.OffsetOdd1[a] - 0x100);
		}
	}

	/* AdcGain 1 Calibration */
	DBG(DBG_FNC, " -> AdcGain 1 calibration: %s\n", ((gaincontrol != 0)&&(calcfg.CalibGain10n != 0))? "ON" : "OFF");

	if ((gaincontrol != 0)&&(calcfg.CalibGain10n != 0))
	{
		/* if we are performing a preview, try to avoid calibration */
		if ((dev->status->preview != SANE_FALSE)&&(dev->preview->gain[CL_RED] != 0)&&(dev->preview->gain[CL_GREEN] != 0)&&(dev->preview->gain[CL_BLUE] != 0))
		{
			/* gain values are taken from eeprom */
			for (a = CL_RED; a <= CL_BLUE; a++)
				cal_data->gain_offset.vgag1[a] = dev->preview->gain[a];
		} else
		{
			/* this is not a preview or  preview values are not correct */
			if (RTS_cal_gain(dev, &calcfg, cal_data, 1, gaincontrol) == SANE_STATUS_GOOD)
			{
				/* if it's a preview, update values and save to eeprom if it's posible */
				if (dev->status->preview != SANE_FALSE)
					RTS_gnoff_save(dev, &cal_data->gain_offset.edcg1[0], &cal_data->gain_offset.vgag1[0]);

				lf900 = SANE_STATUS_GOOD;
			}
		}
	} else
	{
		/* setting default values*/
		for (a = CL_RED; a <= CL_BLUE; a++)
			cal_data->gain_offset.vgag1[a] = calcfg.Gain1[a];
	}

	/* AdcOffset 2 Calibration */
	DBG(DBG_FNC, " -> AdcOffset 2 calibration: %s\n", ((gaincontrol != 0)&&(calcfg.CalibOffset20n != 0))? "ON" : "OFF");

	if ((gaincontrol != 0)&&(calcfg.CalibOffset20n != 0))
	{
		lf900 = RTS_cal_offset(dev, &calcfg, cal_data, 2);
	} else
	{
		/* setting default values */
		for (a = CL_RED; a <= CL_BLUE; a++)
		{
			cal_data->gain_offset.edcg2[a] = abs(calcfg.OffsetEven2[a] - 0x40);
			cal_data->gain_offset.odcg2[a] = abs(calcfg.OffsetOdd2[a] - 0x40);
		}
	}

	/*41d6*/
	/* AdcGain 2 Calibration */
	DBG(DBG_FNC, " -> AdcGain 2 calibration: %s\n", ((gaincontrol != 0)&&(calcfg.CalibGain20n != 0))? "ON" : "OFF");

	if ((gaincontrol != 0)&&(calcfg.CalibGain20n != 0))
	{
		lf900 = RTS_cal_gain(dev, &calcfg, cal_data, 0, gaincontrol);
	} else
	{
		/*423c*/
		for (a = CL_RED; a <= CL_BLUE; a++)
			cal_data->gain_offset.vgag2[a] = calcfg.Gain2[a];
	}

	/*4258*/
	if (calcfg.TotShading != 0)
	{
		lf900 = Calib_BWShading(&calcfg, myCalib, gaincontrol);
		/*falta codigo*/
	} else
	{
		/*428f*/
		if (gaincontrol != 0)
		{
			SANE_Int brst, wrst;

			brst = wrst = SANE_STATUS_GOOD;

			/* black shading */
			DBG(DBG_FNC, " -> Black shading calibration: %s\n", (calcfg.BShadingOn != 0)? "ON" : "OFF");
			if (calcfg.BShadingOn != 0)
				brst = RTS_cal_shd_black(dev, &calcfg, cal_data, myCalib, gaincontrol);

			/* white shading */
			DBG(DBG_FNC, " -> White shading calibration: %s\n", (calcfg.WShadingOn != 0)? "ON" : "OFF");
			if (calcfg.WShadingOn != 0)
				wrst = RTS_cal_shd_white(dev, &calcfg, cal_data, myCalib, gaincontrol);

			if ((brst != SANE_STATUS_GOOD)||(wrst != SANE_STATUS_GOOD))
				myCalib->shading_postprocess = SANE_FALSE;
		} else myCalib->shading_postprocess = SANE_FALSE;
	}

	/*43ca*/
	memcpy(&myCalib->gain_offset, &cal_data->gain_offset, sizeof(struct st_gain_offset));

	/* park home after calibration */
	if (get_value(dev, SCANINFO, PARKHOMEAFTERCALIB, SANE_TRUE, FITCALIBRATE) == SANE_FALSE)
	{
		SANE_Int lmp;

		if ((Lumping == 0)&&(scancfg->resolution_x == RTS_scanmode_minres(dev, scancfg->scantype, scancfg->colormode)))
			lmp = 2;
				else lmp = 1;
		scancfg->origin_y -= (calcfg.WShadingHeight * lmp);
	} else RTS_head_park(dev, SANE_TRUE, dev->motorcfg->parkhomemotormove);

	return SANE_STATUS_GOOD;
}

static SANE_Status RTS_load_constrains(st_device *dev)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	if (dev != NULL)
	{
		if (dev->constrains != NULL)
			RTS_free_constrains(dev);

		dev->constrains = (struct st_constrains *) malloc(sizeof(struct st_constrains));
		if (dev->constrains != NULL)
		{
			cfg_constrains_get(dev, dev->constrains);
			rst = SANE_STATUS_GOOD;
		}
	}

	DBG(DBG_FNC, "> RTS_load_constrains: %i\n", rst);

	return rst;
}

static SANE_Status RTS_area_check(st_device *dev, SANE_Int Resolution, SANE_Int scantype, struct st_coords *mycoords)
{
	/*
	Constrains: 
	 100 dpi   850 x  1170   | 164 x 327
	 300 dpi  2550 x  3510
	 600 dpi  5100 x  7020
	1200 dpi 10200 x 14040
	*/

	SANE_Status rst = SANE_STATUS_INVAL;

	if ((dev != NULL) && (mycoords != NULL))
	{
		if (dev->constrains != NULL)
		{
			struct st_coords limit;
			struct st_coords *mc;

			if ((scantype < ST_NORMAL)||(scantype > ST_NEG))
				scantype = ST_NORMAL;

			switch(scantype)
			{
				case ST_TA:
					mc = &dev->constrains->slide;
					break;
				case ST_NEG:
					mc = &dev->constrains->negative;
					break;
				default:
					mc = &dev->constrains->reflective;
					break;
			}

			/* constrain coords are in milimeters. Translate to pixels */
			limit.left   = MM_TO_PIXEL(mc->left, Resolution);
			limit.width  = MM_TO_PIXEL(mc->width, Resolution);
			limit.top    = MM_TO_PIXEL(mc->top, Resolution);
			limit.height = MM_TO_PIXEL(mc->height, Resolution);

			/* Check left and top */
			if (mycoords->left < 0)
				mycoords->left = 0;

			if (mycoords->top < 0)
				mycoords->top = 0;

			if (mycoords->width < 0)
				mycoords->width = limit.width;

			if (mycoords->height < 0)
				mycoords->height = limit.height;

			/* apply constrains */

			/* Check width and height */
			if ((mycoords->left + mycoords->width) > limit.width)
				mycoords->width = limit.width - mycoords->left;

			if ((mycoords->top + mycoords->height) > limit.height)
				mycoords->height = limit.height - mycoords->top;

			mycoords->left += limit.left;
			mycoords->top  += limit.top;

			rst = SANE_STATUS_GOOD;
		}

		DBG(DBG_FNC, " -> Source=%s, Res=%i, LW=(%i,%i), TH=(%i,%i)\n",
	       dbg_scantype(scantype), Resolution, mycoords->left, mycoords->width,
	       mycoords->top, mycoords->height);
	}

	DBG(DBG_FNC, "> RTS_area_check: %i\n", rst);

	return rst;
}

static struct st_coords *RTS_area_get(st_device *dev, SANE_Byte scantype)
{
	static struct st_coords *rst = NULL;

	if (dev != NULL)
		if (dev->constrains != NULL)
		{
			switch(scantype)
			{
				case ST_TA:
					rst = &dev->constrains->slide;
					break;
				case ST_NEG:
					rst = &dev->constrains->negative;
					break;
				default:
					rst = &dev->constrains->reflective;
					break;
			}
		}

	return rst;
}

static void RTS_free_constrains(st_device *dev)
{
	DBG(DBG_FNC, "> RTS_free_constrains\n");

	if (dev != NULL)
	{
		wfree(dev->constrains);
		dev->constrains = NULL;
	}
}

static void RTS_setup_gamma(SANE_Byte *Regs, struct st_hwdconfig *hwdcfg)
{
	DBG(DBG_FNC, "> RTS_setup_gamma(*Regs, *hwdcfg)\n");

	if (hwdcfg == NULL || Regs == NULL)
		return;

	if (hwdcfg->use_gamma != SANE_FALSE)
	{
		SANE_Int table_size;
		SANE_Int extra_size = hwdcfg->gamma_depth & 1;

		/* set gamma depth */
		data_bitset(&Regs[0x1d0], 0x0f, hwdcfg->gamma_depth);

		/* enable gamma correction */
		data_bitset(&Regs[0x1d0], 0x40, 1);

		/* get table size according to selected gamma depth */
		switch(hwdcfg->gamma_depth & 0x0c)
		{
			case GAMMA_8BIT : table_size = 256  + extra_size; break;
			case GAMMA_10BIT: table_size = 1024 + extra_size; break;
			case GAMMA_12BIT: table_size = 4096 + extra_size; break;
			default: table_size = hwdcfg->startpos & 0xffff; break;
		}

		/* 5073 */
		/* points to red gamma table */
		data_wide_bitset(&Regs[0x1b4], 0x3fff, 0);

		/* points to green gamma table */
		data_wide_bitset(&Regs[0x1b6], 0x3fff, table_size);

		/* points to blue gamma table */
		data_wide_bitset(&Regs[0x1b8], 0x3fff, table_size * 2);

		/* stepper motor curves are allocated after gamma tables
			Next variable points to segment where curves start.
			segment seems to be 16 bytes long
		*/
		mem_segment_curve = ((table_size * 3) + 15) /16;
	} else
	{
		/* disable gamma correction */
		data_bitset(&Regs[0x1d0], 0x40, 0);
		mem_segment_curve = 0;
	}
}

static SANE_Status RTS_chip_reset(st_device *dev)
{
	SANE_Status rst = SANE_STATUS_INVAL; /* default */

	DBG(DBG_FNC, "+ RTS_chip_reset:\n");

	if (dev != NULL)
	{
		/* I've found two ways to reset chipset. Next one will stay commented
		rst = SANE_STATUS_INVAL;
		if (RTS_ctl_read_byte(dev, 0xe800, &data) == SANE_STATUS_GOOD)
		{
			data |= 0x20;
			if (RTS_ctl_write_byte(dev, 0xe800, data) == SANE_STATUS_GOOD)
			{
				data &= 0xdf;
				rst = RTS_ctl_write_byte(dev, 0xe800, data);
			}
		}
		*/

		rst = RTS_ctl_iwrite_buffer(dev, 0x0000, NULL, 0, 0x0801);
	}

	DBG(DBG_FNC, "- RTS_chip_reset: %i\n", rst);

	return rst;
}

/** DMA functions */

static SANE_Status RTS_dma_read_enable(st_device *dev, SANE_Int dmacs, SANE_Int size, SANE_Int segment)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "+ RTS_dma_read_enable(dmacs=0x%04x, size=%i, segment=0x%06x)\n", dmacs, size, segment);

	if (dev != NULL)
	{
		SANE_Byte buffer[6];

		data_msb_set(&buffer[0], segment, 3);

		/* buffer size divided by 2 (words count) */
		data_lsb_set(&buffer[3], size / 2, 3);

		rst = RTS_ctl_iwrite_buffer(dev, dmacs, buffer, 6, 0x0400);
	}

	DBG(DBG_FNC, "- RTS_dma_read_enable: %i\n", rst);

	return rst;
}

static SANE_Status RTS_dma_write_enable(st_device *dev, SANE_Int dmacs, SANE_Int size, SANE_Int segment)
{
	SANE_Status rst = SANE_STATUS_INVAL; /* default */

	DBG(DBG_FNC, "+ RTS_dma_write_enable(dmacs=0x%04x, size=%i, segment=0x%06x)\n", dmacs, size, segment);

	if (dev != NULL)
	{
		SANE_Byte buffer[6];

		data_msb_set(&buffer[0], segment, 3);

		/* buffer size divided by 2 (words count) */
		data_lsb_set(&buffer[3], size / 2, 3);

		rst = RTS_ctl_iwrite_buffer(dev, dmacs, buffer, 6, 0x0401);
	}

	DBG(DBG_FNC, "- RTS_dma_write_enable: %i\n", rst);

	return rst;
}

static SANE_Status RTS_dma_cancel(st_device *dev)
{
	SANE_Status rst = RTS_ctl_iwrite_word(dev, 0x0000, 0, 0x0600);

	DBG(DBG_FNC, "> RTS_dma_cancel: %i\n", rst);

	return rst;
}

static SANE_Status RTS_dma_reset(st_device *dev)
{
	SANE_Status rst = RTS_ctl_iwrite_word(dev, 0x0000, 0x0000, 0x0800);

	DBG(DBG_FNC, "> RTS_dma_reset: %i\n", rst);

	return rst;
}

/** NVRAM functions */

static SANE_Status RTS_nvram_write_byte(st_device *dev, SANE_Int address, SANE_Byte data)
{
	SANE_Status rst = RTS_ctl_iwrite_byte(dev, address, data, 0x200, 0x200);

	DBG(DBG_FNC, "> RTS_nvram_write_byte(address=%04x, data=%i): %i\n", address, data, rst);

	return rst;
}

static SANE_Status RTS_nvram_read_word(st_device *dev, SANE_Int address, SANE_Int *data)
{
	SANE_Status rst = RTS_ctl_iread_word(dev, address, data, 0x200);

	DBG(DBG_FNC, "> RTS_nvram_read_word(address=%04x): %i\n", address, rst);

	return rst;
}

static SANE_Status RTS_nvram_read_byte(st_device *dev, SANE_Int address, SANE_Byte *data)
{
	SANE_Status rst = RTS_ctl_iread_byte(dev, address, data, 0x200);

	DBG(DBG_FNC, "> RTS_nvram_read_byte(address=%04x): %i\n", address, rst);

	return rst;
}

static SANE_Status RTS_nvram_write_word(st_device *dev, SANE_Int address, SANE_Int data)
{
	SANE_Status rst = RTS_ctl_iwrite_word(dev, address, data, 0x0200);

	DBG(DBG_FNC, "> RTS_nvram_write_word(address=%04x, data=%i): %i\n", address, data, rst);

	return rst;
}

static SANE_Status RTS_nvram_read_integer(st_device *dev, SANE_Int address, SANE_Int *data)
{
	SANE_Status rst = RTS_ctl_iread_int(dev, address, data, 0x200);

	DBG(DBG_FNC, "> RTS_nvram_read_integer(address=%04x): %i\n", address, rst);

	return rst;
}

static SANE_Status RTS_nvram_write_integer(st_device *dev, SANE_Int address, SANE_Int data)
{
	SANE_Status rst = RTS_ctl_iwrite_int(dev, address, data, 0x200);

	DBG(DBG_FNC, "> RTS_nvram_write_integer(address=%04x, data=%i): %i\n", address, data, rst);

	return rst;
}

static SANE_Status RTS_nvram_write_buffer(st_device *dev, SANE_Int address, SANE_Byte *data, SANE_Int size)
{
	SANE_Status rst = RTS_ctl_iwrite_buffer(dev, address, data, size, 0x200);

	DBG(DBG_FNC, "> RTS_nvram_write_buffer(address=%04x, data, size=%i): %i\n", address, size, rst);

	return rst;
}

/** Shading check functions */

static void RTS_shd_check_free(struct st_check_shading *shading)
{
	if (shading != NULL)
	{
		SANE_Int a;
		for (a = 0; a < 4; a++)
		{
			wfree(shading->sh_tables[a]);
			shading->sh_tables[a] = NULL;
			shading->sh_table_size[a] = 0;
		}

		for (a = 0; a < 3; a++)
		{
			wfree(shading->ex_tables[a]);
			shading->ex_tables[a] = NULL;
		}

		memset(shading, 0, sizeof(struct st_check_shading));
	}

	DBG(DBG_FNC, "> RTS_shd_check_free\n");
}

static SANE_Status RTS_shd_check_alloc(SANE_Byte *Regs, struct st_check_shading *shading, SANE_Int shading_length, SANE_Int min_size)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	if ((shading != NULL)&&(Regs != NULL)&&(shading_length > 0))
	{
		SANE_Int buffer_size;

		/* free already allocated tables */
		RTS_shd_check_free(shading);

		if (((Regs[0x1bf] & 0x18) == 0)&&((((Regs[0x1cf] >> 1) & Regs[0x1cf]) & 4) != 0))
			shading->table_count = 2;
				else shading->table_count = 4;

		buffer_size = shading_length * 2; /* words */

		shading->ee04 = min(buffer_size, min_size);
		shading->edb0 = buffer_size / min_size;

		if (((buffer_size % min_size) != 0)&&(buffer_size >= min_size))
			shading->ee0c = min_size * 2;
				else shading->ee0c = min_size;

		if (buffer_size > min_size)
			shading->ee04 = (buffer_size % shading->ee04) + shading->ee04;

		if (buffer_size > (min_size * 2))
		{
			shading->ee14 = shading->edb0 - 1;
			shading->ee08 = shading->ee14 * min_size;
			shading->ee01 = SANE_TRUE;
			shading->ee10 = (((shading->ee08 / shading->table_count) + (min_size - 1)) / min_size) * min_size;
		} else
		{
			shading->ee14 = 0;
			shading->ee01 = SANE_FALSE;
		}

		shading->ee14 = (min_size >> 4) * shading->ee14;

		/* now we've got all sizes, lets create neccesary tables */
		rst = SANE_STATUS_GOOD;

		if (shading->table_count > 0)
		{
			SANE_Int a = 0;

			while ((rst == SANE_STATUS_GOOD)&&(a < 4))
			{
				shading->sh_tables[a] = (SANE_Byte *) calloc(1, sizeof(SANE_Byte) * shading->ee0c);
				if (shading->sh_tables[a] == NULL)
					rst = SANE_STATUS_INVAL;

				a++;
			}
		}

		/**/
		if (rst == SANE_STATUS_GOOD)
		{
			shading->ex_tables[1] = (SANE_Byte *) calloc(1, sizeof(SANE_Byte) * shading->ee0c);
			if (shading->ex_tables[1] == NULL)
				rst = SANE_STATUS_INVAL;
		}

		if ((rst == SANE_STATUS_GOOD)&&(shading->ee01 != SANE_FALSE))
		{
			shading->ex_tables[0] = (SANE_Byte *) calloc(1, sizeof(SANE_Byte) * (shading->ee08 / shading->table_count));
			if (shading->ex_tables[0] != NULL)
			{
				shading->ex_tables[2] = (SANE_Byte *) calloc(1, sizeof(SANE_Byte) * shading->ee10);
				if (shading->ex_tables[2] == NULL)
					rst = SANE_STATUS_INVAL;
			} else rst = SANE_STATUS_INVAL;
		}

		/* if some error happened, free allocated tables */
		if (rst == SANE_STATUS_INVAL)
			RTS_shd_check_free(shading);

		/* show debugging information */
		dbg_checkshading(shading);
	}

	DBG(DBG_FNC, "> RTS_shd_check_alloc(length:%i >>> tables=%i): %i\n", shading_length, shading->table_count, rst);

	return rst;
}

static SANE_Status RTS_shd_check_fill(SANE_Byte *source_table, SANE_Byte **dest_table, SANE_Int dest_size, SANE_Int from_pos, SANE_Int table_count, SANE_Int *table_sizes)
{
	/*
		Arg1 = tabla de la cual partir 3fb0
		Arg2 = ptr a tablas a rellenar ee18
		Arg3 = tama�o tabla 0x70 ee04 -> ed70
		Arg4 = ee14 -> 0x138  ed6c
		Arg5 = table_count ->ed74

		fills tables and returns their sizes
	*/

	SANE_Status rst = SANE_STATUS_INVAL; /* default */

	if ((source_table != NULL)&&(dest_table != NULL)&&(table_sizes != NULL))
	{
		memset(table_sizes, 0, sizeof(SANE_Int) * 4);
		if (dest_size > 0)
		{
			SANE_Byte *ptr = source_table + (from_pos << 4);
			SANE_Int blsize, table = 0;

			while (dest_size > 0)
			{
				blsize = min(dest_size, 16);

				memcpy(*(dest_table + table) + table_sizes[table], ptr, blsize);
				table_sizes[table] += blsize;

				table++;
				if (table == table_count)
					table = 0;

				dest_size -= blsize;
				ptr += blsize;
			}
		}

		rst = SANE_STATUS_GOOD;
	}

	DBG(DBG_FNC, "> RTS_shd_check_fill: %i\n", rst);

	return rst;
}

static SANE_Status RTS_shd_check_fillex(SANE_Byte *source_table, SANE_Byte *dest_table, SANE_Int from_pos, SANE_Int dest_size, SANE_Int table_count, SANE_Int *table_sizes)
{
	/*
		ed64 Arg1 = tabla de la cual partir 6001108
		ed68 Arg2 = ptr a tabla a rellenar ee28
		ed6c Arg3 = puntero first position 0
		ed70 Arg4 = ee08  1380
		ed74 Arg5 = table_count
		ed78 Arg6 = ptr a table sizes

		Ademas de rellenar las tablas devuelve el tama�o de las mismas
	*/
	SANE_Status rst = SANE_STATUS_INVAL; /* default */

	if ((dest_table != NULL)&&(source_table != NULL)&&(table_sizes != NULL))
	{
		memset(table_sizes, 0, sizeof(SANE_Int) * 4);
		if (dest_size > 0)
		{
			SANE_Byte *ptr = source_table + (from_pos << 4);
			SANE_Int blsize, table_size = 0, table = 0;

			while (dest_size > 0)
			{
				blsize = min(dest_size, 16);

				if (table == 0)
				{
					memcpy(dest_table + table_size, ptr, blsize);
					table_size += blsize;
				}

				table++;
				if (table == table_count)
					table = 0;

				dest_size -= blsize;
				ptr += blsize;
			}

			table_sizes[0] = table_size;
		}

		rst = SANE_STATUS_GOOD;
	}

	DBG(DBG_FNC, "> RTS_shd_check_fillex: %i\n", rst);

	return rst;
}

static SANE_Status RTS_shd_check_run(st_device *dev, SANE_Byte *Regs, SANE_Byte *in_buffer, SANE_Byte **tables, SANE_Int channel, SANE_Int ee14, SANE_Int buf_size, SANE_Int *table_sizes, SANE_Int table_count, SANE_Int edb0)
{
	/*
		ed58 Arg1 = Regs ef90
		ed5c Arg2 = prt a in_buffer
		ed60 Arg3 = ptr de las 4 tablas ee18
		ed64 Arg4 = channel ???????
		ed68 Arg5 = ee14
		ed6c Arg6 = buf_size
		ed70 Arg7 = ptr al tama�o de cada tabla ed8c
		ed74 Arg8 = table-count
		ed78 Arg9 = edb0
	*/

	SANE_Status rst = SANE_STATUS_INVAL; /* default */

	DBG(DBG_FNC, "+ RTS_shd_check_run():\n");

	if ((Regs != NULL)&&(in_buffer != NULL)&&(tables != NULL)&&(table_sizes != NULL))
	{
		rst = SANE_STATUS_GOOD;

		if (table_count > 0)
		{
			SANE_Int tb = 0;
			SANE_Int options = 0, op[3]= {0, 0, 0};

			do
			{
				if (table_sizes[tb] > 0)
				{
					if (table_count == 2)
					{
						switch (tb)
						{
							case 0:
								if (edb0 == 0)
								{
									op[CL_RED] = 0x000000;
									op[CL_BLUE] = 0x000000;
									op[CL_GREEN]  = 0x300000;
								} else
								{
									op[CL_RED] = 0x200000;
									op[CL_BLUE] = 0x200000;
									op[CL_GREEN]  = 0x100000;
								}
								break;

							case 1:
								if (edb0 == 0)
								{
									op[CL_RED] = 0x100000;
									op[CL_BLUE] = 0x100000;
									op[CL_GREEN]  = 0x200000;
								} else
								{
									op[CL_RED] = 0x300000;
									op[CL_BLUE] = 0x300000;
									op[CL_GREEN]  = 0x000000;
								}
								break;
						}
					} else
					{
						switch (tb)
						{
							case 0:
								op[CL_RED] = 0x000000;
								op[CL_BLUE] = 0x000000;
								op[CL_GREEN]  = 0x300000;
								break;

							case 1:
								op[CL_RED] = 0x200000;
								op[CL_BLUE] = 0x200000;
								op[CL_GREEN]  = 0x100000;
								break;

							case 2:
								op[CL_RED] = 0x100000;
								op[CL_BLUE] = 0x100000;
								op[CL_GREEN]  = 0x200000;
								break;

							case 3:
								op[CL_RED] = 0x300000;
								op[CL_BLUE] = 0x300000;
								op[CL_GREEN]  = 0x000000;
								break;
						}
					}

					/*bbd7*/
					switch (channel)
					{
						case 0: /* red */
							options = op[CL_RED]   | (Regs[0x1ba] + (ee14 / table_count));
							break;

						case 1: /* green */
							options = op[CL_GREEN] | (Regs[0x1bb] + (((((Regs[0x1bf] & 1) << 8) + Regs[0x1bc]) << 8) + (ee14 / table_count)));
							break;

						case 2: /* blue */
							options = op[CL_BLUE]  | (Regs[0x1bb] + ((((((Regs[0x1bf] >> 1) & 3) << 8) + Regs[0x1be]) << 8) + (ee14 / table_count)));
							break;
					}

					/* bc53 */
					/* read from scanner shading buffer */
					if (RTS_dma_read (dev, 0x0004, options, buf_size, in_buffer) == SANE_STATUS_GOOD)
					{
#ifdef aa
						if ((table_sizes[tb]) > 0)
						{
							SANE_Int a, value1, value2;

							/* compare values with given tables. Should be equal */
							a = 0;
							while ((a < table_sizes[tb]) && (rst == SANE_STATUS_GOOD))
							{
								value1 = data_lsb_get(in_buffer + a, 1);
								value2 = data_lsb_get(tables[tb] + a, 1);

								if (value1 != value2)
								{
									rst = SANE_STATUS_INVAL;
									DBG(DBG_FNC, " -> Buffer different for table[%i] in pos %i of total size %i\n", tb, a, buf_size);
								} else a++;
							}
						}
#endif
					} else rst = SANE_STATUS_INVAL;

					tb++;
				} else break;
			} while ((tb < table_count)&&(rst == SANE_STATUS_GOOD));
		} else rst = SANE_STATUS_GOOD;
	}

	DBG(DBG_FNC, "- RTS_shd_check_run: %i\n", rst);

	return rst;
}

static SANE_Status RTS_shd_check(st_device *dev, struct st_check_shading *shading, SANE_Byte *Regs, SANE_Byte *shadtable, SANE_Int channel)
{
	/*
		eda0 Arg1: shading structure ee00
		eda4 Arg2: Regs ef90
		eda8 Arg3: shadtable
		edac Arg4: channel color
		edb0 Arg5: 0
	*/

	SANE_Status rst = SANE_STATUS_INVAL; /* default */

	if ((shading != NULL)&&(dev != NULL)&&(Regs != NULL)&&(shadtable != NULL))
	{
		SANE_Int table_sizes[4];

		memset(&table_sizes, 0, sizeof(SANE_Int) * 4);

		RTS_shd_check_fill(shadtable, shading->sh_tables, shading->ee04, shading->ee14, shading->table_count, table_sizes);

		rst = RTS_shd_check_run(dev, Regs, shading->ex_tables[1], shading->sh_tables, channel, shading->ee14, shading->ee0c, table_sizes, shading->table_count, shading->edb0);

		if (rst == SANE_STATUS_GOOD)
		{
			if (shading->ee01 != SANE_FALSE)
			{
				RTS_shd_check_fillex(shadtable, shading->ex_tables[0], 0, shading->ee08, shading->table_count, table_sizes);

				rst = RTS_shd_check_run(dev, Regs, shading->ex_tables[2], shading->ex_tables, channel, 0, shading->ee10 , table_sizes, shading->table_count, shading->edb0);
			}
		}
	}

	DBG(DBG_FNC, "> RTS_shd_check: %i\n", rst);

	return rst;
}

static SANE_Byte *RTS_gamma_depth_conv(SANE_Byte *table, SANE_Int src_depth, SANE_Int dst_depth)
{
	SANE_Byte *rst = NULL;

	if (table != NULL)
	{
		SANE_Int dst_items, src_items;
		SANE_Int dst_size;
		SANE_Int extra = dst_depth & 1;

		switch (dst_depth & 0x0c)
		{
			case GAMMA_8BIT : dst_depth =  8; break;
			case GAMMA_10BIT: dst_depth = 10; break;
			case GAMMA_12BIT: dst_depth = 12; break;
		}

		dst_items = 1 << dst_depth;
		dst_size  = (((dst_items + extra) * dst_depth) + 7)/ 8;
		src_items = 1 << src_depth;

		if ((rst = (SANE_Byte *) malloc (dst_size * sizeof(SANE_Byte))) != NULL)
		{
			if (src_depth != dst_depth)
			{
				SANE_Int a, value;
				SANE_Int src_bitpos, src_byte, src_rbits, src_mask;
				SANE_Int dst_bitpos, dst_byte, dst_rbits, dst_mask;

				src_bitpos = 0;
				dst_bitpos = 0;

				src_mask   = (1 << src_depth) - 1;
				dst_mask   = (1 << dst_depth) - 1;

				if (src_depth > dst_depth)
				{
					/* decrease size */
					SANE_Int jump = src_items / dst_items;

					for (a = 0; a < dst_items; a++)
					{
						/* get value from source */
						src_byte  = src_bitpos / 8;
						src_rbits = src_bitpos % 8;

						value = data_wide_bitget(table + src_byte, src_mask << src_rbits);

						/* perform conversion */
						value >>= (src_depth - dst_depth);

						/* save value to destination */
						dst_byte  = dst_bitpos / 8;
						dst_rbits = dst_bitpos % 8;

						data_wide_bitset(rst + dst_byte, dst_mask << dst_rbits, value);

						/* next item */
						dst_bitpos += dst_depth;
						src_bitpos += (src_depth * jump);
					}
				} else
				{
					/* increase size */
					SANE_Int old_value;
					SANE_Int jump = dst_items / src_items;
					
					old_value = 0;

					for (a = 0; a < src_items; a++)
					{
						/* get value from source */
						src_byte  = src_bitpos / 8;
						src_rbits = src_bitpos % 8;

						value = data_wide_bitget(table + src_byte, src_mask << src_rbits);

						/* perform conversion */
						value <<= (dst_depth - src_depth);

						if (jump > 1)
						{
							if (a != 0)
							{
								SANE_Int b, sum = (value - old_value) / jump;

								for (b = 0; b < jump - 1; b++)
								{
									/* save value to destination */
									dst_byte  = dst_bitpos / 8;
									dst_rbits = dst_bitpos % 8;

									old_value += sum;

									data_wide_bitset(rst + dst_byte, dst_mask << dst_rbits, old_value);

									/* next item */
									dst_bitpos += dst_depth;
								}
							}
						}

						/* save value to destination */
						dst_byte  = dst_bitpos / 8;
						dst_rbits = dst_bitpos % 8;

						data_wide_bitset(rst + dst_byte, dst_mask << dst_rbits, value);

						/* next item */
						dst_bitpos += dst_depth;
						src_bitpos += src_depth;

						old_value = value;
					}
				}
			} else memcpy(rst, table, dst_size * sizeof(SANE_Byte));
		}
	}

	return rst;
}

#endif /* RTS8822_CORE */
