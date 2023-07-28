/* HP Scanjet 3900 series - SANE Backend controller
   Copyright (C) 2005-2009 Jonathan Bravo Lopez <jkdsoft@gmail.com>

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

/* Backend Code for SANE*/

#define HP3900_CONFIG_FILE "hp3900.conf"
#define GAMMA_DEFAULT 1.0

#include "../include/sane/config.h"
#include "../include/sane/sane.h"
#include "../include/sane/sanei.h"
#include "../include/sane/sanei_backend.h"
#include "../include/sane/sanei_config.h"
#include "../include/sane/saneopts.h"
#include "../include/sane/sanei_usb.h"
#include "../include/sane/sanei_debug.h"

/* next typedef is mandatory before calling hp3900_rts8822.c */
typedef SANE_Int USB_Handle;

#include "hp3900_rts8822.c"

struct st_convert
{
	SANE_Int colormode;
	SANE_Int depth;
	SANE_Int threshold;
	SANE_Int negative;
	SANE_Int real_depth;
};

/* options enumerator */
typedef enum
{
	opt_begin = 0,

	grp_geometry,
	opt_tlx, opt_tly, opt_brx, opt_bry,
	opt_resolution,

	/* gamma tables */
	opt_gamma_red,
	opt_gamma_green,
	opt_gamma_blue,

	opt_preview,
	opt_scantype,
	opt_colormode,
	opt_depth,
	opt_threshold,

	/* debugging options */
	grp_debug,
	opt_model,
	opt_negative,
	opt_nogamma,
	opt_nowshading,
	opt_realdepth,
	opt_emulategray,
	opt_nowarmup,
	opt_dbgimages,
	opt_reset,

	/* device information */
	grp_info,
	opt_chipname,
	opt_chipid,
	opt_scancount,
	opt_infoupdate,

	/* supported buttons. RTS8822 supports up to 6 buttons */
	grp_sensors,
	opt_button_0,
	opt_button_1,
	opt_button_2,
	opt_button_3,
	opt_button_4,
	opt_button_5,

	opt_count
} EOptionIndex;

/* linked list of SANE_Device structures */
typedef struct TDevListEntry
{
	struct TDevListEntry *pNext;
	SANE_Device dev;
	char* devname;
} TDevListEntry;

typedef struct
{
	char *pszVendor;
	char *pszName;
} TScannerModel;

typedef union
{
	SANE_Word w;
	SANE_Word *wa;		/* word array */
	SANE_String s;
} TOptionValue;

typedef struct
{
	/* next structure contains all neccesary vars to manage rts8822 */
	st_device *device;

	SANE_Option_Descriptor aOptions[opt_count];
	TOptionValue           aValues[opt_count];
	struct params ScanParams;

	/* lists */
	SANE_String_Const *list_colormodes;
	SANE_Int          *list_depths;
	char              **list_models;
	SANE_Int          *list_resolutions;
	SANE_String_Const *list_sources;

	SANE_Word *aGammaTable[3]; /* a 16-to-16 bit color lookup table */
	SANE_Range rng_gamma;

	/* reading image */
	SANE_Byte *image;
	SANE_Byte *rest;
	SANE_Int   rest_amount;
	SANE_Int   mylin;

	/* convertion settings */
	struct st_convert cnv;

	/* ranges */
	SANE_Range rng_threshold;
	SANE_Range rng_horizontal;
	SANE_Range rng_vertical;

	SANE_Int scan_count;
	SANE_Int fScanning; /* SANE_TRUE if actively scanning */
} TScanner;

/* functions to manage backend's options */
static void        options_init(TScanner *scanner);
static void        options_free(TScanner *scanner);

/* devices listing */
static SANE_Status _ReportDevice(TScannerModel *pModel, const char *pszDeviceName);
static SANE_Status attach_one_device (SANE_String_Const devname);

/* capabilities */
static SANE_Status bknd_colormodes(TScanner *scanner, SANE_Int model);
static void        bknd_constrains(TScanner *scanner, SANE_Int source, SANE_Int type);
static SANE_Status bknd_depths(TScanner *scanner, SANE_Int model);
static SANE_Status bknd_info(TScanner *scanner);
static SANE_Status bknd_models(TScanner *scanner);
static SANE_Status bknd_resolutions(TScanner *scanner, SANE_Int model);
static SANE_Status bknd_sources(TScanner *scanner, SANE_Int model);

/* convertions */
static void        Color_Negative(SANE_Byte *buffer, SANE_Int size, SANE_Int depth);
static void        Color_to_Gray(SANE_Byte *buffer, SANE_Int size, SANE_Int depth);
static void        Gray_to_Lineart(SANE_Byte *buffer, SANE_Int size, SANE_Int threshold);
static void        Depth_16_to_8(SANE_Byte *from_buffer, SANE_Int size, SANE_Byte *to_buffer);

/* gamma functions */
static void        gamma_apply(TScanner *s, SANE_Byte *buffer, SANE_Int size, SANE_Int depth);
static SANE_Status gamma_create(TScanner *s, double gamma);
static void        gamma_free(TScanner *s);

static SANE_Int    Get_Colormode(SANE_String colormode);
static SANE_Int    Get_Model(SANE_String model);
static SANE_Int    Get_Source(SANE_String source);
static SANE_Int    GetUSB_device_model(SANE_String_Const name);

static size_t      max_string_size (char **strings);
static size_t      max_string_size2 (const SANE_String_Const strings[]);

static SANE_Status get_button_status(TScanner *s);

/* reading buffers */
static SANE_Status img_buffers_alloc(TScanner *scanner, SANE_Int size);
static SANE_Status img_buffers_free(TScanner *scanner);

static SANE_Status option_get(TScanner *scanner, SANE_Int optid, void *result);
static SANE_Status option_set(TScanner *scanner, SANE_Int optid, void *value, SANE_Int *pInfo);

static void        Set_Coordinates(TScanner *scanner, SANE_Int scantype, SANE_Int resolution, struct st_coords *coords);
static void        Silent_Compile(void);
static SANE_Status Translate_coords(struct st_coords *coords);

/* SANE functions */
void        sane_cancel(SANE_Handle h);
void        sane_close(SANE_Handle h);
SANE_Status sane_control_option (SANE_Handle h, SANE_Int n, SANE_Action Action, void *pVal, SANE_Int * pInfo);
void        sane_exit(void);
SANE_Status sane_get_devices (const SANE_Device *** device_list, SANE_Bool local_only);
const SANE_Option_Descriptor *sane_get_option_descriptor (SANE_Handle h, SANE_Int n);
SANE_Status sane_get_parameters (SANE_Handle h, SANE_Parameters * p);
SANE_Status sane_get_select_fd(SANE_Handle handle, SANE_Int * fd);
SANE_Status sane_init(SANE_Int * version_code, SANE_Auth_Callback authorize);
SANE_Status sane_open(SANE_String_Const name, SANE_Handle * h);
SANE_Status sane_read (SANE_Handle h, SANE_Byte * buf, SANE_Int maxlen, SANE_Int * len);
SANE_Status sane_set_io_mode(SANE_Handle handle, SANE_Bool non_blocking);
SANE_Status sane_start(SANE_Handle h);

/* variables */
static TDevListEntry      *_pFirstSaneDev = 0;
static SANE_Int            iNumSaneDev = 0;
static const SANE_Device **_pSaneDevList = 0;

/* Own functions */

static SANE_Status bknd_resolutions(TScanner *s, SANE_Int model)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "> bknd_resolutions(*scanner, model=%i)\n", model);

	if (s != NULL)
	{
		SANE_Int *res = NULL;

		switch(model)
		{
			case BQ5550:
			case UA4900:
				{
					SANE_Int myres[] = {8, 50, 75, 100, 150, 200, 300, 600, 1200};

					res = (SANE_Int *) malloc(sizeof(myres));
					if (res != NULL)
						memcpy(res, &myres, sizeof(myres));
				}
				break;

			case HPG2710:
			case HP3800:
				{
					/* 1200 and 2400 dpi are disabled until problems are solved */
					SANE_Int myres[] = {7, 50, 75, 100, 150, 200, 300, 600};

					res = (SANE_Int *) malloc(sizeof(myres));
					if (res != NULL)
						memcpy(res, &myres, sizeof(myres));
				}
				break;

			case HP4370:
			case HPG3010:
			case HPG3110:
				{
					SANE_Int myres[] = {10, 50, 75, 100, 150, 200, 300, 600, 1200, 2400, 4800};

					res = (SANE_Int *) malloc(sizeof(myres));
					if (res != NULL)
						memcpy(res, &myres, sizeof(myres));
				}
				break;

			default: /* HP3970 & HP4070 & UA4900 */
				{
					SANE_Int myres[] = {9, 50, 75, 100, 150, 200, 300, 600, 1200, 2400};

					res = (SANE_Int *) malloc(sizeof(myres));
					if (res != NULL)
						memcpy(res, &myres, sizeof(myres));
				}
				break;
		}

		if (res != NULL)
		{
			if (s->list_resolutions != NULL)
				free(s->list_resolutions);

			s->list_resolutions = res;
			rst = SANE_STATUS_GOOD;
		}
	}

	return rst;
}

static SANE_Status bknd_models(TScanner *s)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "> bknd_models:\n");

	if (s != NULL)
	{
		/* generate list of supported devices */

		SANE_Int cnt;
		char **list;

		/* get number of supported devices and allocate space */
		cnt = RTS_device_count();

		if ((list = calloc (1, sizeof(char **) * (cnt + 1))) != NULL)
		{
			SANE_Int a;
			char *smodel;

			for (a = 0; a < cnt; a++)
			{
				smodel = RTS_device_name(RTS_device_supported(a));
				if (smodel != NULL)
					list[a] = smodel;
						else break;
			}

			/* free previous list */
			if (s->list_models != NULL)
			{
				a = 0;
				while (s->list_models[a] != NULL)
				{
					free (s->list_models[a]);
					a++;
				}

				free (s->list_models);
			}

			/* set new list */
			s->list_models = list;

			rst = SANE_STATUS_GOOD;
		}
	}

	return rst;
}

static SANE_Status bknd_colormodes(TScanner *s, SANE_Int model)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "> bknd_colormodes(*scanner, model=%i)\n", model);

	if (s != NULL)
	{
		SANE_String_Const *colormode = NULL;

		/* at this moment all devices use the same list */
		SANE_String_Const mycolormode[] = {SANE_I18N("Color"), SANE_I18N("Gray"), SANE_I18N("Lineart"), 0};

		/* silence gcc */
		model = model;

		colormode = (SANE_String_Const *)malloc(sizeof(mycolormode));
		if (colormode != NULL)
			memcpy(colormode, &mycolormode, sizeof(mycolormode));

		if (colormode != NULL)
		{
			if (s->list_colormodes != NULL)
				free(s->list_colormodes);

			s->list_colormodes = colormode;
			rst = SANE_STATUS_GOOD;
		}
	}

	return rst;
}

static SANE_Status bknd_sources(TScanner *s, SANE_Int model)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "> bknd_sources(*scanner, model=%i)\n", model);

	if (s != NULL)
	{
		SANE_String_Const *source = NULL;

		switch (model)
		{
			case UA4900:
				{
					SANE_String_Const mysource[] = {SANE_I18N("Flatbed"), 0};
					source = (SANE_String_Const *)malloc(sizeof(mysource));
					if (source != NULL)
						memcpy(source, &mysource, sizeof(mysource));
				}
				break;

			default: /* hp3970, hp4070, hp4370 and others */
				{
					SANE_String_Const mysource[] = {SANE_I18N("Flatbed"), SANE_I18N("Slide"), SANE_I18N("Negative"), 0};
					if ((source = (SANE_String_Const *)malloc(sizeof(mysource))) != NULL)
						memcpy(source, &mysource, sizeof(mysource));
				}
				break;
		}

		if (source != NULL)
		{
			if (s->list_sources != NULL)
				free(s->list_sources);

			s->list_sources = source;
			rst = SANE_STATUS_GOOD;
		}
	}

	return rst;
}

static SANE_Status bknd_depths(TScanner *s, SANE_Int model)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "> bknd_depths(*scanner, model=%i\n", model);

	if (s != NULL)
	{
		SANE_Int *depth = NULL;

		/* at this moment all devices use the same list */
		SANE_Int mydepth[] = {2, 8, 16}; /*{3, 8, 12, 16};*/

		/* silence gcc */
		model = model;

		depth = (SANE_Int *)malloc(sizeof(mydepth));
		if (depth != NULL)
			memcpy(depth, &mydepth, sizeof(mydepth));

		if (depth != NULL)
		{
			if (s->list_depths != NULL)
				free(s->list_depths);

			s->list_depths = depth;
			rst = SANE_STATUS_GOOD;
		}
	}

	return rst;
}

static SANE_Status bknd_info(TScanner *s)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "> bknd_info(*scanner)");

	if (s != NULL)
	{
		char data[256];

		/* update chipset name */
		RTS_chip_name(s->device, data, 255);
		if (s->aValues[opt_chipname].s != NULL)
		{
			free(s->aValues[opt_chipname].s);
			s->aValues[opt_chipname].s = NULL;
		}

		s->aValues[opt_chipname].s = strdup(data);
		s->aOptions[opt_chipname].size = strlen(data) + 1;

		/* update chipset id */
		s->aValues[opt_chipid].w = RTS_chip_id(s->device);

		/* update scans counter */
		s->aValues[opt_scancount].w = RTS_scan_count_get(s->device);

		rst = SANE_STATUS_GOOD;
	}

	return rst;
}

static SANE_Int GetUSB_device_model(SANE_String_Const name)
{
	SANE_Int usbid, model;

	/* default model is unknown */
	model = -1;

	/* open usb device */
	if (sanei_usb_open(name, &usbid) == SANE_STATUS_GOOD)
	{
		SANE_Int vendor, product;

		if (sanei_usb_get_vendor_product(usbid, &vendor, &product) == SANE_STATUS_GOOD)
			model = RTS_device_get(product, vendor);

		sanei_usb_close(usbid);
	}

	return model;
}

static void Silent_Compile()
{
	/*
		There are some functions in hp3900_rts8822.c that aren't used yet.
		To avoid compilation warnings we will use them here
	*/

	if (1 == 0)
	{
		RTS_btn_status(NULL);
		RTS_btn_name(NULL, 0);
		Gamma_GetTables(NULL, NULL);
		RTS_gamma_depth_set (NULL, 0, 0);
	}
}

static void bknd_constrains(TScanner *s, SANE_Int source, SANE_Int type)
{
	if (s != NULL)
	{
		struct st_coords *coords = RTS_area_get(s->device, source);

		if (coords != NULL)
		{
			switch(type)
			{
				case 1: /* Y */
					s->rng_vertical.max = coords->height;
					break;
				default: /* X */
					s->rng_horizontal.max = coords->width;
					break;
			}
		}
	}
}

static SANE_Status img_buffers_free(TScanner *s)
{
	if (s != NULL)
	{
		if (s->image != NULL)
		{
			free(s->image);
			s->image = NULL;
		}

		if (s->rest != NULL)
		{
			free(s->rest);
			s->rest = NULL;
		}

		s->rest_amount = 0;
	}

	return SANE_STATUS_GOOD;
}

static SANE_Status img_buffers_alloc(TScanner *s, SANE_Int size)
{
	SANE_Status rst;

	/* default result at this point */
	rst = SANE_STATUS_INVAL;

	if (s != NULL)
	{
		/* default result at this point */
		rst = SANE_STATUS_NO_MEM;

		/* free previous allocs */
		img_buffers_free(s);

		s->image = (SANE_Byte *) malloc(size * sizeof(SANE_Byte));
		if (s->image != NULL)
		{
			s->rest = (SANE_Byte *) malloc(size * sizeof(SANE_Byte));
			if (s->rest != NULL)
				rst = SANE_STATUS_GOOD; /* ok !! */
		}

		if (rst != SANE_STATUS_GOOD)
			img_buffers_free(s);
	}

	return rst;
}

static void Set_Coordinates(TScanner *s, SANE_Int scantype, SANE_Int resolution, struct st_coords *coords)
{
	struct st_coords *limits = RTS_area_get(s->device, scantype);

	DBG(DBG_FNC, "> Set_Coordinates(res=%i, *coords):\n", resolution);

	if (coords->left == -1)
		coords->left = 0;

	if (coords->width == -1)
		coords->width = limits->width;

	if (coords->top == -1)
		coords->top = 0;

	if (coords->height == -1)
		coords->height = limits->height;

	DBG(DBG_FNC, " -> Coords [MM] : xy(%i, %i) wh(%i, %i)\n", coords->left, coords->top, coords->width, coords->height);

	coords->left   = MM_TO_PIXEL(coords->left, resolution);
	coords->width  = MM_TO_PIXEL(coords->width, resolution);
	coords->top    = MM_TO_PIXEL(coords->top, resolution);
	coords->height = MM_TO_PIXEL(coords->height, resolution);

	DBG(DBG_FNC, " -> Coords [px] : xy(%i, %i) wh(%i, %i)\n", coords->left, coords->top, coords->width, coords->height);

	RTS_area_check(s->device, resolution, scantype, coords);

	DBG(DBG_FNC, " -> Coords [check]: xy(%i, %i) wh(%i, %i)\n", coords->left, coords->top, coords->width, coords->height);
}

static void Color_Negative(SANE_Byte *buffer, SANE_Int size, SANE_Int depth)
{
	if (buffer != NULL)
	{
		SANE_Int a, value;
		SANE_Int max_value = (1 << depth) - 1;
		SANE_Int ch_size = (depth + 7) / 8;

		for (a = 0; a < size; a += ch_size)
		{
			value = max_value - data_lsb_get(buffer + a, ch_size);
			data_lsb_set(buffer + a, value, ch_size);
		}
	}
}

static SANE_Status get_button_status(TScanner *s)
{
	if (s != NULL)
	{
		SANE_Int a, b, status, btn;

		b = 1;
		status = RTS_btn_released(s->device) & 63;
		for (a = 0; a < 6; a++)
		{
			if ((status & b) != 0)
			{
				btn = RTS_btn_order(s->device, b);
				if (btn != -1)
					s->aValues[opt_button_0 + btn].w = SANE_TRUE;
			}

			b <<= 1;
		}
	}

	return SANE_STATUS_GOOD;
}

static void Depth_16_to_8(SANE_Byte *from_buffer, SANE_Int size, SANE_Byte *to_buffer)
{
	if ((from_buffer != NULL)&&(to_buffer != NULL))
	{
		SANE_Int a, b;

		a = 1;
		b = 0;

		while (a < size)
		{
			*(to_buffer + b) = *(from_buffer + a);
			a += 2;
			b++;
		}
	}
}

static void Gray_to_Lineart(SANE_Byte *buffer, SANE_Int size, SANE_Int threshold)
{
	/* code provided by tobias leutwein */

	if (buffer != NULL)
	{
		SANE_Byte toBufferByte;
		SANE_Int fromBufferPos_i = 0;
		SANE_Int toBufferPos_i = 0;
		SANE_Int bitPos_i;

		while ( fromBufferPos_i < size )
		{
			toBufferByte = 0;

			for ( bitPos_i=7; bitPos_i!=(-1); bitPos_i-- )
			{
				if ( (fromBufferPos_i < size) && (buffer[fromBufferPos_i] < threshold) )
					toBufferByte |= (1u << bitPos_i);

				fromBufferPos_i++;
			}

			buffer[toBufferPos_i] = toBufferByte;
			toBufferPos_i++;
		}
	}
}

static void Color_to_Gray(SANE_Byte *buffer, SANE_Int size, SANE_Int depth)
{
	/* converts 3 color channel into 1 gray channel of specified bit depth */

	if (buffer != NULL)
	{
		SANE_Int c, chn, chn_size;
		SANE_Byte *ptr_src = NULL;
		SANE_Byte *ptr_dst = NULL;
		float data, chn_data;
		float coef[3] = {0.299, 0.587, 0.114}; /* coefficients per channel */

		chn_size = (depth > 8) ? 2: 1;
		ptr_src = (void *) buffer;
		ptr_dst = (void *) buffer;

		for (c = 0; c < size / (3 * chn_size); c++)
		{
			data = 0.;

			/* get, apply coeffs and sum channels */
			for (chn = 0; chn < 3; chn++)
			{
				chn_data = data_lsb_get(ptr_src + (chn * chn_size), chn_size);
				data += (chn_data * coef[chn]);
			}

			/* save result */
			data_lsb_set(ptr_dst, (SANE_Int) data, chn_size);

			ptr_src += 3 * chn_size; /* next triplet */
			ptr_dst += chn_size;
		}
	}
}

static void gamma_free(TScanner *s)
{
	DBG(DBG_FNC, "> gamma_free()\n");

	if (s != NULL)
	{
		/* Destroy gamma tables */
		SANE_Int a;

		for (a = CL_RED; a <= CL_BLUE; a++)
		{
			if (s->aGammaTable[a] != NULL)
			{
				free(s->aGammaTable[a]);
				s->aGammaTable[a] = NULL;
			}
		}
	}
}

static SANE_Status gamma_create(TScanner *s, double gamma)
{
	SANE_Status rst = SANE_STATUS_INVAL; /* by default */

	DBG(DBG_FNC, "> gamma_create(*s)\n");

	if (s != NULL)
	{
		SANE_Int a;
		double value, c;

		/* default result */
		rst = SANE_STATUS_GOOD;

		/* destroy previus gamma tables */
		gamma_free(s);

		/* check gamma value */
		if (gamma < 0)
			gamma = GAMMA_DEFAULT;

		/* allocate space for 16 bit gamma tables */
		for (a = CL_RED; a <= CL_BLUE; a++)
		{
			s->aGammaTable[a] = malloc(65536 * sizeof(SANE_Word));
			if (s->aGammaTable[a] == NULL)
			{
				rst = SANE_STATUS_INVAL;
				break;
			}
		}

		if (rst == SANE_STATUS_GOOD)
		{
			/* fill tables*/
			for (a = 0; a < 65536; a++)
			{
				value = (a / (65536. - 1));
				value = pow(value, (1. / gamma));
				value = value * (65536. - 1 );

				c = (SANE_Int) value;
				if (c > (65536. - 1))
					c = (65536. - 1);
						else if (c < 0)
							c = 0;

				s->aGammaTable[CL_RED][a] = c;
				s->aGammaTable[CL_GREEN][a] = c;
				s->aGammaTable[CL_BLUE][a] = c;
			}
		} else gamma_free(s);
	}

	return rst;
}

static void gamma_apply(TScanner *s, SANE_Byte *buffer, SANE_Int size, SANE_Int depth)
{
	if ((s != NULL)&&(buffer != NULL))
	{
		SANE_Int c, value;
		SANE_Int chn, chn_size = (depth + 7) / 8;
		SANE_Int dot_size = 3 * chn_size;
		SANE_Byte *ptr;

		ptr = buffer;

		for (c = 0; c < size; c += chn_size)
		{
			chn = (c % dot_size) / chn_size;

			value = data_lsb_get(ptr, chn_size);
			value = (s->aGammaTable[chn] != NULL)? s->aGammaTable[chn][value] : value;
			data_lsb_set(ptr, value, chn_size);

			ptr += chn_size;
		}
	}
}

static SANE_Int Get_Model(SANE_String model)
{
	SANE_Int rst = -1;
	SANE_Int a;

	for (a = 0; a < RTS_device_count(); a++)
	{
		char *name = RTS_device_name(a);

		if (name != NULL)
		{
			if (strcmp(model, name) == 0)
				rst = a;

			free (name);
		}

		if (rst != -1)
			break;
	}

	/* default ? */
	if (rst == -1)
		rst = HP3970;

	return rst;
}

static SANE_Int Get_Source(SANE_String source)
{
	SANE_Int rst;

	if (strcmp(source, SANE_I18N("Flatbed")) == 0)
		rst = ST_NORMAL;
	else if (strcmp(source, SANE_I18N("Slide")) == 0)
		rst = ST_TA;
	else if (strcmp(source, SANE_I18N("Negative")) == 0)
		rst = ST_NEG;
	else rst = ST_NORMAL; /* default */

	return rst;
}

static SANE_Int Get_Colormode(SANE_String colormode)
{
	SANE_Int rst;

	if (strcmp(colormode, SANE_I18N("Color")) == 0)
		rst = CM_COLOR;
	else if (strcmp(colormode, SANE_I18N("Gray")) == 0)
		rst = CM_GRAY;
	else if (strcmp(colormode, SANE_I18N("Lineart")) == 0)
		rst = CM_LINEART;
	else rst = CM_COLOR; /* default */

	return rst;
}

static SANE_Status Translate_coords(struct st_coords *coords)
{
	SANE_Int data;

	DBG(DBG_FNC, "> Translate_coords(*coords)\n");

	if ((coords->left < 0) || (coords->top < 0) ||
	   (coords->width < 0) || (coords->height < 0))
		return SANE_STATUS_INVAL;

	if (coords->width < coords->left)
	{
		data = coords->left;
		coords->left = coords->width;
		coords->width = data;
	}

	if (coords->height < coords->top)
	{
		data = coords->top;
		coords->top = coords->height;
		coords->height = data;
	}

	coords->width -= coords->left;
	coords->height -= coords->top;

	if (coords->width == 0)
		coords->width++;

	if (coords->height == 0)
		coords->height++;

	return SANE_STATUS_GOOD;
}

static size_t max_string_size (char **strings)
{
	size_t size, max_size = 0;
	SANE_Int i;

	for (i = 0; strings[i]; ++i)
	{
		size = strlen (strings[i]) + 1;
		if (size > max_size)
			max_size = size;
	}

	return max_size;
}

static size_t max_string_size2 (const SANE_String_Const strings[])
{
	size_t size, max_size = 0;
	SANE_Int i;

	for (i = 0; strings[i]; ++i)
	{
		size = strlen (strings[i]) + 1;
		if (size > max_size)
			max_size = size;
	}

	return max_size;
}

static void options_free(TScanner *s)
{
	/* frees all information contained in controls */

	DBG(DBG_FNC, "> options_free\n");

	if (s != NULL)
	{
		SANE_Int i;
		SANE_Option_Descriptor *pDesc;
		TOptionValue *pVal;

		/* free gamma tables */
		gamma_free(s);

		/* free lists */
		if (s->list_resolutions != NULL)
			free(s->list_resolutions);

		if (s->list_depths != NULL)
			free(s->list_depths);

		if (s->list_sources != NULL)
			free(s->list_sources);

		if (s->list_colormodes != NULL)
			free(s->list_colormodes);

		if (s->list_models != NULL)
		{
			i = 0;
			while (s->list_models[i] != NULL)
			{
				free (s->list_models[i]);
				i++;
			}

			free(s->list_models);
		}

		/* free values in certain controls */
		for (i = opt_begin; i < opt_count; i++)
		{
			pDesc = &s->aOptions[i];
			pVal = &s->aValues[i];

			if (pDesc->type == SANE_TYPE_STRING)
			{
				if (pVal->s != NULL)
					free(pVal->s);
			}
		}
	}
}

static void options_init(TScanner *s)
{
	/* initializes all controls */

	DBG(DBG_FNC, "> options_init\n");

	if (s != NULL)
	{
		SANE_Int i;
		SANE_Option_Descriptor *pDesc;
		TOptionValue *pVal;

		/* set gamma */
		gamma_create(s, 2.2);

		/* color convertion */
		s->cnv.colormode  = -1;
		s->cnv.negative   = SANE_FALSE;
		s->cnv.threshold  = 40;
		s->cnv.real_depth = SANE_FALSE;
		s->cnv.depth      = -1;

		/* setting threshold */
		s->rng_threshold.min   = 0;
		s->rng_threshold.max   = 255;
		s->rng_threshold.quant = 0;

		/* setting gamma range (16 bits depth) */
		s->rng_gamma.min   = 0;
		s->rng_gamma.max   = 65535;
		s->rng_gamma.quant = 0;

		/* setting default horizontal constrain in milimeters */
		s->rng_horizontal.min   = 0;
		s->rng_horizontal.max   = 220;
		s->rng_horizontal.quant = 1;

		/* setting default vertical constrain in milimeters */
		s->rng_vertical.min   = 0;
		s->rng_vertical.max   = 300;
		s->rng_vertical.quant = 1;

		/* allocate option lists */
		bknd_info(s);
		bknd_colormodes(s, s->device->model);
		bknd_depths(s, s->device->model);
		bknd_models(s);
		bknd_resolutions(s, s->device->model);
		bknd_sources(s, s->device->model);

		/* By default preview scan */
		s->ScanParams.scantype      = ST_NORMAL;
		s->ScanParams.colormode     = CM_COLOR;
		s->ScanParams.resolution_x  = 75;
		s->ScanParams.resolution_y  = 75;
		s->ScanParams.coords.left   = 0;
		s->ScanParams.coords.top    = 0;
		s->ScanParams.coords.width  = 220;
		s->ScanParams.coords.height = 300;
		s->ScanParams.depth         = 8;
		s->ScanParams.channel       = 0;

		for (i = opt_begin; i < opt_count; i++)
		{
			pDesc = &s->aOptions[i];
			pVal = &s->aValues[i];

			/* defaults */
			pDesc->name   = "";
			pDesc->title  = "";
			pDesc->desc   = "";
			pDesc->type   = SANE_TYPE_INT;
			pDesc->unit   = SANE_UNIT_NONE;
			pDesc->size   = sizeof(SANE_Word);
			pDesc->constraint_type = SANE_CONSTRAINT_NONE;
			pDesc->cap    = 0;

			switch(i)
			{
				case opt_begin:
					pDesc->title  = SANE_TITLE_NUM_OPTIONS;
					pDesc->desc   = SANE_DESC_NUM_OPTIONS;
					pDesc->cap    = SANE_CAP_SOFT_DETECT;
					pVal->w       = (SANE_Word)opt_count;
					break;

				case grp_geometry:
					pDesc->name  = SANE_NAME_GEOMETRY;
					pDesc->title = SANE_TITLE_GEOMETRY;
					pDesc->desc  = SANE_DESC_GEOMETRY;
					pDesc->type  = SANE_TYPE_GROUP;
					pDesc->unit  = SANE_UNIT_NONE;
					pDesc->size  = 0;
					pDesc->cap   = 0;
					pDesc->constraint_type = SANE_CONSTRAINT_NONE;
					pDesc->constraint.range = 0;
					pVal->w      = 0;
					break;

				case opt_tlx:
					pDesc->name  = SANE_NAME_SCAN_TL_X;
					pDesc->title = SANE_TITLE_SCAN_TL_X;
					pDesc->desc  = SANE_DESC_SCAN_TL_X;
					pDesc->unit  = SANE_UNIT_MM;
					pDesc->constraint_type  = SANE_CONSTRAINT_RANGE;
					pDesc->constraint.range = &s->rng_horizontal;
					pDesc->cap   = SANE_CAP_SOFT_SELECT | SANE_CAP_SOFT_DETECT;
					pVal->w      = 0;
					break;

				case opt_tly:
					pDesc->name  = SANE_NAME_SCAN_TL_Y;
					pDesc->title = SANE_TITLE_SCAN_TL_Y;
					pDesc->desc  = SANE_DESC_SCAN_TL_Y;
					pDesc->unit  = SANE_UNIT_MM;
					pDesc->constraint_type  = SANE_CONSTRAINT_RANGE;
					pDesc->constraint.range = &s->rng_vertical;
					pDesc->cap   = SANE_CAP_SOFT_SELECT | SANE_CAP_SOFT_DETECT;
					pVal->w      = 0;
					break;

				case opt_brx:
					pDesc->name  = SANE_NAME_SCAN_BR_X;
					pDesc->title = SANE_TITLE_SCAN_BR_X;
					pDesc->desc  = SANE_DESC_SCAN_BR_X;
					pDesc->unit  = SANE_UNIT_MM;
					pDesc->constraint_type  = SANE_CONSTRAINT_RANGE;
					pDesc->constraint.range = &s->rng_horizontal;
					pDesc->cap   = SANE_CAP_SOFT_SELECT | SANE_CAP_SOFT_DETECT;
					pVal->w      = s->rng_horizontal.max;
					break;

				case opt_bry:
					pDesc->name  = SANE_NAME_SCAN_BR_Y;
					pDesc->title = SANE_TITLE_SCAN_BR_Y;
					pDesc->desc  = SANE_DESC_SCAN_BR_Y;
					pDesc->unit  = SANE_UNIT_MM;
					pDesc->constraint_type  = SANE_CONSTRAINT_RANGE;
					pDesc->constraint.range = &s->rng_vertical;
					pDesc->cap   = SANE_CAP_SOFT_SELECT | SANE_CAP_SOFT_DETECT;
					pVal->w      = s->rng_vertical.max;
					break;

				case opt_resolution:
					pDesc->name  = SANE_NAME_SCAN_RESOLUTION;
					pDesc->title = SANE_TITLE_SCAN_RESOLUTION;
					pDesc->desc  = SANE_DESC_SCAN_RESOLUTION;
					pDesc->unit  = SANE_UNIT_DPI;
					pDesc->constraint_type  = SANE_CONSTRAINT_WORD_LIST;
					pDesc->constraint.word_list = s->list_resolutions;
					pDesc->cap   = SANE_CAP_SOFT_SELECT | SANE_CAP_SOFT_DETECT;
					pVal->w      = s->list_resolutions[1];
					break;

				case opt_gamma_red:
					pDesc->name  = SANE_NAME_GAMMA_VECTOR_R;
					pDesc->title = SANE_TITLE_GAMMA_VECTOR_R;
					pDesc->desc  = SANE_DESC_GAMMA_VECTOR_R;
					pDesc->size  = s->rng_gamma.max * sizeof(SANE_Word);
					pDesc->unit  = SANE_UNIT_NONE;
					pDesc->constraint_type = SANE_CONSTRAINT_RANGE;
					pDesc->constraint.range = &s->rng_gamma;
					pDesc->cap   = SANE_CAP_SOFT_SELECT | SANE_CAP_SOFT_DETECT;
					pVal->wa     = s->aGammaTable[CL_RED];
					break;

				case opt_gamma_green:
					pDesc->name  = SANE_NAME_GAMMA_VECTOR_G;
					pDesc->title = SANE_TITLE_GAMMA_VECTOR_G;
					pDesc->desc  = SANE_DESC_GAMMA_VECTOR_G;
					pDesc->size  = s->rng_gamma.max * sizeof(SANE_Word);
					pDesc->unit  = SANE_UNIT_NONE;
					pDesc->constraint_type = SANE_CONSTRAINT_RANGE;
					pDesc->constraint.range = &s->rng_gamma;
					pDesc->cap   = SANE_CAP_SOFT_SELECT | SANE_CAP_SOFT_DETECT;
					pVal->wa     = s->aGammaTable[CL_GREEN];
					break;

				case opt_gamma_blue:
					pDesc->name  = SANE_NAME_GAMMA_VECTOR_B;
					pDesc->title = SANE_TITLE_GAMMA_VECTOR_B;
					pDesc->desc  = SANE_DESC_GAMMA_VECTOR_B;
					pDesc->size  = s->rng_gamma.max * sizeof(SANE_Word);
					pDesc->unit  = SANE_UNIT_NONE;
					pDesc->constraint_type = SANE_CONSTRAINT_RANGE;
					pDesc->constraint.range = &s->rng_gamma;
					pDesc->cap   = SANE_CAP_SOFT_SELECT | SANE_CAP_SOFT_DETECT;
					pVal->wa     = s->aGammaTable[CL_BLUE];
					break;

				case opt_preview:
					pDesc->name  = "preview";
					pDesc->title = "Preview mode";
					pDesc->desc  = "Preview mode";
					pDesc->type  = SANE_TYPE_BOOL;
					pDesc->size  = sizeof(SANE_Word);
					pDesc->cap   = SANE_CAP_SOFT_SELECT | SANE_CAP_SOFT_DETECT | SANE_CAP_ADVANCED;
					break;

				case opt_scantype:
					pDesc->name  = SANE_NAME_SCAN_SOURCE;
					pDesc->title = SANE_TITLE_SCAN_SOURCE;
					pDesc->desc  = SANE_DESC_SCAN_SOURCE;
					pDesc->type  = SANE_TYPE_STRING;
					pDesc->size  = max_string_size2(s->list_sources);
					pDesc->constraint_type = SANE_CONSTRAINT_STRING_LIST;
					pDesc->constraint.string_list = s->list_sources;
					pDesc->cap   = SANE_CAP_SOFT_SELECT | SANE_CAP_SOFT_DETECT;
					pVal->s      = strdup(s->list_sources[0]);
					break;

				case opt_colormode:
					pDesc->name  = SANE_NAME_SCAN_MODE;
					pDesc->title = SANE_TITLE_SCAN_MODE;
					pDesc->desc  = SANE_DESC_SCAN_MODE;
					pDesc->type  = SANE_TYPE_STRING;
					pDesc->size  = max_string_size2(s->list_colormodes);
					pDesc->constraint_type = SANE_CONSTRAINT_STRING_LIST;
					pDesc->constraint.string_list = s->list_colormodes;
					pDesc->cap   = SANE_CAP_SOFT_SELECT | SANE_CAP_SOFT_DETECT;
					pVal->s      = strdup (s->list_colormodes[0]);
					break;

				case opt_depth:
					pDesc->name  = SANE_NAME_BIT_DEPTH;
					pDesc->title = SANE_TITLE_BIT_DEPTH;
					pDesc->desc  = SANE_DESC_BIT_DEPTH;
					pDesc->type  = SANE_TYPE_INT;
					pDesc->unit  = SANE_UNIT_BIT;
					pDesc->constraint_type  = SANE_CONSTRAINT_WORD_LIST;
					pDesc->constraint.word_list = s->list_depths;
					pDesc->cap   = SANE_CAP_SOFT_SELECT | SANE_CAP_SOFT_DETECT;
					pVal->w      = s->list_depths[1];
					break;

				case opt_threshold:
					pDesc->name  = SANE_NAME_THRESHOLD;
					pDesc->title = SANE_TITLE_THRESHOLD;
					pDesc->desc  = SANE_DESC_THRESHOLD;
					pDesc->type  = SANE_TYPE_INT;
					pDesc->unit  = SANE_UNIT_NONE;
					pDesc->constraint_type = SANE_CONSTRAINT_RANGE;
					pDesc->constraint.range = &s->rng_threshold;
					pDesc->cap  |= SANE_CAP_SOFT_SELECT | SANE_CAP_SOFT_DETECT | SANE_CAP_INACTIVE;
					pVal->w      = 0x80;
					break;

				/* debugging options */
				case grp_debug:
					pDesc->name  = "grp_debug";
					pDesc->title = SANE_I18N("Debugging Options");
					pDesc->desc  = "";
					pDesc->type  = SANE_TYPE_GROUP;
					pDesc->unit  = SANE_UNIT_NONE;
					pDesc->size  = 0;
					pDesc->cap   = SANE_CAP_ADVANCED;
					pDesc->constraint_type = SANE_CONSTRAINT_NONE;
					pDesc->constraint.range = 0;
					pVal->w      = 0;
					break;

				case opt_model:
					pDesc->name  = "opt_model";
					pDesc->title = SANE_I18N("Scanner model");
					pDesc->desc  = SANE_I18N("Allows to test device behaviour with other supported models");
					pDesc->type  = SANE_TYPE_STRING;
					pDesc->size  = max_string_size (s->list_models);
					pDesc->constraint_type = SANE_CONSTRAINT_STRING_LIST;
					pDesc->constraint.string_list = (SANE_String_Const *) s->list_models;
					pDesc->cap   = SANE_CAP_ADVANCED | SANE_CAP_SOFT_SELECT | SANE_CAP_SOFT_DETECT;
					pVal->s      = RTS_device_name(s->device->model);
					break;

				case opt_negative:
					pDesc->name  = "opt_negative";
					pDesc->title = SANE_I18N("Negative");
					pDesc->desc  = SANE_I18N("Image colours will be inverted");
					pDesc->type  = SANE_TYPE_BOOL;
					pDesc->unit  = SANE_UNIT_NONE;
					pDesc->size  = sizeof(SANE_Word);
					pDesc->constraint_type = SANE_CONSTRAINT_NONE;
					pDesc->constraint.range = 0;
					pDesc->cap   = SANE_CAP_ADVANCED | SANE_CAP_SOFT_DETECT | SANE_CAP_SOFT_SELECT;
					pVal->w      = SANE_FALSE;
					break;

				case opt_nogamma:
					pDesc->name  = "opt_nogamma";
					pDesc->title = SANE_I18N("Disable gamma correction");
					pDesc->desc  = SANE_I18N("Gamma correction will be disabled");
					pDesc->type  = SANE_TYPE_BOOL;
					pDesc->unit  = SANE_UNIT_NONE;
					pDesc->size  = sizeof(SANE_Word);
					pDesc->constraint_type = SANE_CONSTRAINT_NONE;
					pDesc->constraint.range = 0;
					pDesc->cap   = SANE_CAP_ADVANCED | SANE_CAP_SOFT_DETECT | SANE_CAP_SOFT_SELECT;
					pVal->w      = SANE_FALSE;
					break;

				case opt_nowshading:
					pDesc->name  = "opt_nowshading";
					pDesc->title = SANE_I18N("Disable white shading correction");
					pDesc->desc  = SANE_I18N("White shading correction will be disabled");
					pDesc->type  = SANE_TYPE_BOOL;
					pDesc->unit  = SANE_UNIT_NONE;
					pDesc->size  = sizeof(SANE_Word);
					pDesc->constraint_type = SANE_CONSTRAINT_NONE;
					pDesc->constraint.range = 0;
					pDesc->cap   = SANE_CAP_ADVANCED | SANE_CAP_SOFT_DETECT | SANE_CAP_SOFT_SELECT;
					pVal->w      = SANE_FALSE;
					break;

				case opt_nowarmup:
					pDesc->name  = "opt_nowarmup";
					pDesc->title = SANE_I18N("Skip warmup process");
					pDesc->desc  = SANE_I18N("Warmup process will be disabled");
					pDesc->type  = SANE_TYPE_BOOL;
					pDesc->unit  = SANE_UNIT_NONE;
					pDesc->size  = sizeof(SANE_Word);
					pDesc->constraint_type = SANE_CONSTRAINT_NONE;
					pDesc->constraint.range = 0;
					pDesc->cap   = SANE_CAP_ADVANCED | SANE_CAP_SOFT_DETECT | SANE_CAP_SOFT_SELECT;
					pVal->w      = SANE_FALSE;
					break;

				case opt_realdepth:
					pDesc->name  = "opt_realdepth";
					pDesc->title = SANE_I18N("Force real depth");
					pDesc->desc  = SANE_I18N("If gamma is enabled, scans are always made in 16 bits depth to improve image quality and then converted to the selected depth. This option avoids depth emulation.");
					pDesc->type  = SANE_TYPE_BOOL;
					pDesc->unit  = SANE_UNIT_NONE;
					pDesc->size  = sizeof(SANE_Word);
					pDesc->constraint_type = SANE_CONSTRAINT_NONE;
					pDesc->constraint.range = 0;
					pDesc->cap   = SANE_CAP_ADVANCED | SANE_CAP_SOFT_DETECT | SANE_CAP_SOFT_SELECT;
					pVal->w      = SANE_FALSE;
					break;

				case opt_emulategray:
					pDesc->name  = "opt_emulategray";
					pDesc->title = SANE_I18N("Emulate Grayscale");
					pDesc->desc  = SANE_I18N("If enabled, image will be scanned in color mode and then converted to grayscale by software. This may improve image quality in some circumstances.");
					pDesc->type  = SANE_TYPE_BOOL;
					pDesc->unit  = SANE_UNIT_NONE;
					pDesc->size  = sizeof(SANE_Word);
					pDesc->constraint_type = SANE_CONSTRAINT_NONE;
					pDesc->constraint.range = 0;
					pDesc->cap   = SANE_CAP_ADVANCED | SANE_CAP_SOFT_DETECT | SANE_CAP_SOFT_SELECT;
					pVal->w      = SANE_FALSE;
					break;

				case opt_dbgimages:
					pDesc->name  = "opt_dbgimages";
					pDesc->title = SANE_I18N("Save debugging images");
					pDesc->desc  = SANE_I18N("If enabled, some images involved in scanner processing are saved to analyze them.");
					pDesc->type  = SANE_TYPE_BOOL;
					pDesc->unit  = SANE_UNIT_NONE;
					pDesc->size  = sizeof(SANE_Word);
					pDesc->constraint_type = SANE_CONSTRAINT_NONE;
					pDesc->constraint.range = 0;
					pDesc->cap   = SANE_CAP_ADVANCED | SANE_CAP_SOFT_DETECT | SANE_CAP_SOFT_SELECT;
					pVal->w      = SANE_FALSE;
					break;

				case opt_reset:
					pDesc->name  = "opt_reset";
					pDesc->title = SANE_I18N("Reset chipset");
					pDesc->desc  = SANE_I18N("Resets chipset data");
					pDesc->type  = SANE_TYPE_BUTTON;
					pDesc->unit  = SANE_UNIT_NONE;
					pDesc->size  = 0;
					pDesc->constraint_type = SANE_CONSTRAINT_NONE;
					pDesc->constraint.string_list = 0;
					pDesc->cap   = SANE_CAP_ADVANCED | SANE_CAP_SOFT_SELECT;
					pVal->w      = 0;
					break;

				/* device information */
				case grp_info:
					pDesc->name  = "grp_info";
					pDesc->title = SANE_I18N("Information");
					pDesc->desc  = "";
					pDesc->type  = SANE_TYPE_GROUP;
					pDesc->unit  = SANE_UNIT_NONE;
					pDesc->size  = 0;
					pDesc->cap   = 0;
					pDesc->constraint_type = SANE_CONSTRAINT_NONE;
					pDesc->constraint.range = 0;
					pVal->w      = 0;
					break;

				case opt_chipname:
					pDesc->name  = "opt_chipname";
					pDesc->title = SANE_I18N("Chipset name");
					pDesc->desc  = SANE_I18N("Shows chipset name used in device.");
					pDesc->type  = SANE_TYPE_STRING;
					pDesc->constraint_type = SANE_CONSTRAINT_NONE;
					pDesc->cap   = SANE_CAP_ADVANCED | SANE_CAP_SOFT_DETECT;
					pVal->s      = strdup(SANE_I18N("Unknown"));
					pDesc->size  = strlen(pVal->s) + 1; /* maximum size */
					break;

				case opt_chipid:
					pDesc->name  = "opt_chipid";
					pDesc->title = SANE_I18N("Chipset ID");
					pDesc->desc  = SANE_I18N("Shows the chipset ID");
					pDesc->type  = SANE_TYPE_INT;
					pDesc->unit  = SANE_UNIT_NONE;
					pDesc->constraint_type = SANE_CONSTRAINT_NONE;
					pDesc->cap   = SANE_CAP_ADVANCED | SANE_CAP_SOFT_DETECT;
					pVal->w      = -1;
					break;

				case opt_scancount:
					pDesc->name  = "opt_scancount";
					pDesc->title = SANE_I18N("Scan counter");
					pDesc->desc  = SANE_I18N("Shows the number of scans made by scanner");
					pDesc->type  = SANE_TYPE_INT;
					pDesc->unit  = SANE_UNIT_NONE;
					pDesc->constraint_type = SANE_CONSTRAINT_NONE;
					pDesc->cap   = SANE_CAP_ADVANCED | SANE_CAP_SOFT_DETECT;
					pVal->w      = -1;
					break;

					case opt_infoupdate:
					pDesc->name  = "opt_infoupdate";
					pDesc->title = SANE_I18N("Update information");
					pDesc->desc  = SANE_I18N("Updates information about device");
					pDesc->type  = SANE_TYPE_BUTTON;
					pDesc->unit  = SANE_UNIT_NONE;
					pDesc->size  = 0;
					pDesc->constraint_type = SANE_CONSTRAINT_NONE;
					pDesc->constraint.string_list = 0;
					pDesc->cap   = SANE_CAP_ADVANCED | SANE_CAP_SOFT_SELECT;
					pVal->w      = 0;
					break;

				/* buttons support */
				case grp_sensors:
					pDesc->name  = SANE_NAME_SENSORS;
					pDesc->title = SANE_TITLE_SENSORS;
					pDesc->desc  = SANE_DESC_SENSORS;
					pDesc->type  = SANE_TYPE_GROUP;
					pDesc->unit  = SANE_UNIT_NONE;
					pDesc->size  = 0;
					pDesc->cap   = 0;
					pDesc->constraint_type = SANE_CONSTRAINT_NONE;
					pDesc->constraint.range = 0;
					pVal->w      = 0;
					break;

				case opt_button_0:
				case opt_button_1:
				case opt_button_2:
				case opt_button_3:
				case opt_button_4:
				case opt_button_5:
				{
					char name [12];
					char title [128];

					sprintf (name, "button %d", i - opt_button_0);
					sprintf (title, "Scanner button %d", i - opt_button_0);
					pDesc->name   = strdup (name);
					pDesc->title  = strdup (title);
					pDesc->desc   = SANE_I18N("This option reflects a front panel scanner button");
					pDesc->type   = SANE_TYPE_BOOL;
					pDesc->cap    = SANE_CAP_SOFT_DETECT | SANE_CAP_ADVANCED;

					if (i - opt_button_0 >= RTS_btn_count(s->device))
						pDesc->cap |= SANE_CAP_INACTIVE;

					pDesc->unit   = SANE_UNIT_NONE;
					pDesc->size   = sizeof (SANE_Word);
					pDesc->constraint_type = SANE_CONSTRAINT_NONE;
					pVal->w       = SANE_FALSE;
				}
					break;
			}
		}
	}
}

static SANE_Status _ReportDevice(TScannerModel *pModel, const char *pszDeviceName)
{
	SANE_Status rst = SANE_STATUS_INVAL;
	TDevListEntry *pNew, *pDev;

	DBG(DBG_FNC, "> _ReportDevice:\n");

	pNew = malloc(sizeof(TDevListEntry));
	if (pNew != NULL)
	{
		rst = SANE_STATUS_GOOD;

		/* add new element to the end of the list */
		if (_pFirstSaneDev != NULL)
		{
			/* Add at the end of existing list*/
			for (pDev = _pFirstSaneDev; pDev->pNext; pDev = pDev->pNext);

			pDev->pNext = pNew;
		} else _pFirstSaneDev = pNew;

		/* fill in new element */
		pNew->pNext      = NULL;
		pNew->devname    = (char *)strdup(pszDeviceName);
		pNew->dev.name   = pNew->devname;
		pNew->dev.vendor = pModel->pszVendor;
		pNew->dev.model  = pModel->pszName;
		pNew->dev.type   = SANE_I18N("flatbed scanner");

		iNumSaneDev++;
	}

	return rst;
}

static SANE_Status attach_one_device (SANE_String_Const devname)
{
	static TScannerModel sModel;
	SANE_Int device;

	DBG(DBG_FNC, "> attach_one_device(devname=%s)\n", devname);

	device  = GetUSB_device_model(devname);
	sModel.pszVendor = RTS_device_vendor (device);
	sModel.pszName   = RTS_device_product(device);

	_ReportDevice(&sModel, devname);

	return SANE_STATUS_GOOD;
}

/* Sane default functions */

SANE_Status sane_init(SANE_Int * version_code, SANE_Auth_Callback authorize)
{
	FILE *conf_fp;		/* Config file stream  */
	SANE_Char line[PATH_MAX];
	SANE_Char *str = NULL;
	SANE_String_Const proper_str;
	SANE_Int nline = 0;

	/* Initialize debug */
	DBG_INIT();

	/* reporting core's version could be useful */
	DBG(DBG_VRB, "hp3900 backend using core version %s\n\n", BACKEND_VRSN);

	DBG(DBG_FNC, "> sane_init\n");

	/* silence gcc */
	authorize = authorize;

	/* Initialize usb*/
	sanei_usb_init();

	/* Parse config file */
	conf_fp = sanei_config_open(HP3900_CONFIG_FILE);
	if (conf_fp)
	{
		while (sanei_config_read (line, sizeof (line), conf_fp))
		{
			nline++;
			if (str)
				free(str);

			proper_str = sanei_config_get_string (line, &str);

			/* Discards white lines and comments */
			if ((str != NULL) && (proper_str != line) && (str[0] != '#'))
			{
				/* If line's not blank or a comment, then it's the device
				 * filename or a usb directive. */
				sanei_usb_attach_matching_devices (line, attach_one_device);
		  }
		}
		fclose (conf_fp);
	} else
	{
		/* default */
		SANE_Int devcount = RTS_device_count();

		DBG(DBG_VRB, "- %s not found. Looking for hardcoded usb ids ...\n", HP3900_CONFIG_FILE);

		if (devcount > 0)
		{
			SANE_Int c;
			SANE_Int device, vendor, product;

			/* add usb ids supported by driver (see hp3900_config.c) */
			for (c = 0; c < devcount; c++)
			{
				device = RTS_device_supported(c);

				if (device != -1)
				{
					/* device found */
					vendor  = RTS_device_usb_vendor (device);
					product = RTS_device_usb_product(device);

					if ((vendor != -1) && (product != -1))
					{
						char usbstr[20];
						if (snprintf(usbstr, 20, "usb 0x%04x 0x%04x", vendor, product) > 0)
						{
							/* attach device */
							char *sdev = RTS_device_name(device);

							if (sdev != NULL)
							{
								DBG(DBG_VRB, " -> Attaching '%s' : %s \n", usbstr, sdev);
								free (sdev);
							}

							sanei_usb_attach_matching_devices(usbstr, attach_one_device);
						}
					}
				}
			}
		}

		/* more devices can be added here like next example */
		/* sanei_usb_attach_matching_devices("usb 0x03f0 0x2605", attach_one_device); */
	}

	/* Return backend version */
	if (version_code != NULL)
		*version_code = SANE_VERSION_CODE(SANE_CURRENT_MAJOR, V_MINOR, 0);

	return SANE_STATUS_GOOD;
}

SANE_Status sane_get_devices (const SANE_Device *** device_list, SANE_Bool local_only)
{
	SANE_Status rst = SANE_STATUS_GOOD;

	local_only = local_only;

	if (_pSaneDevList)
		free(_pSaneDevList);

	_pSaneDevList = malloc(sizeof(*_pSaneDevList) * (iNumSaneDev + 1));
	if (_pSaneDevList != NULL)
	{
		TDevListEntry *pDev;
		SANE_Int i = 0;

		for (pDev = _pFirstSaneDev; pDev; pDev = pDev->pNext)
			_pSaneDevList[i++] = &pDev->dev;

		_pSaneDevList[i++] = 0; /* last entry is 0 */
		*device_list = _pSaneDevList;
	} else rst = SANE_STATUS_NO_MEM;

	DBG(DBG_FNC, "> sane_get_devices: %i\n", rst);

	return rst;
}

SANE_Status sane_open(SANE_String_Const name, SANE_Handle *h)
{
	TScanner *s;
	SANE_Status rst;

	s = (TScanner *) malloc(sizeof(TScanner));
	if (s != NULL)
	{
		memset(s, 0, sizeof(TScanner));

		/* check the name */
		if (strlen(name) == 0)
			/* default to first available device */
			name = _pFirstSaneDev->dev.name;

		/* allocate space for RTS environment */
		s->device = RTS_alloc();
		if (s->device != NULL)
		{
			/* open device */
			rst = sanei_usb_open(name, &s->device->usb->handle);
			if (rst == SANE_STATUS_GOOD)
			{
				/* Initializing RTS */
				SANE_Int vendor, product, model;

				/* Setting device model */
				if (sanei_usb_get_vendor_product(s->device->usb->handle, &vendor, &product) == SANE_STATUS_GOOD)
					model = RTS_device_get(product, vendor);
						else model = HP3970;

				/* try to set model */
				RTS_device_set(s->device, model, product, vendor);

				/* Initialize device */
				if (RTS_scanner_init(s->device) == SANE_STATUS_GOOD)
				{
					/* silencing unused functions */
					Silent_Compile();

					/* initialize backend options */
					options_init(s);
					*h = s;

					/* everything went ok */
					rst = SANE_STATUS_GOOD;
				} else
				{
					free ((void *) s);
					rst = SANE_STATUS_INVAL;
				}
			}
		} else rst = SANE_STATUS_NO_MEM;
	} else rst = SANE_STATUS_NO_MEM;

	DBG(DBG_FNC, "> sane_open(name=%s): %i\n", name, rst);

	return rst;
}

const SANE_Option_Descriptor *sane_get_option_descriptor (SANE_Handle h, SANE_Int n)
{
	SANE_Option_Descriptor *rst = NULL;

	if ((n >= opt_begin)&&(n < opt_count))
	{
		TScanner *s = (TScanner *) h;
		rst = &s->aOptions[n];
	}

	DBG(DBG_FNC, "> SANE_Option_Descriptor(handle, n=%i): %i\n", n, (rst == NULL)? -1: 0);

	return rst;
}

static SANE_Status option_get(TScanner *s, SANE_Int optid, void *result)
{
	/* This function returns value contained in selected option */

	DBG(DBG_FNC, "> option_get(optid=%i)\n", optid);

	if ((s != NULL)&&(result != NULL))
	{
		switch (optid)
		{
			/* SANE_Word */
			case opt_begin: /* null */
			case opt_reset: /* null */
			case opt_negative:
			case opt_nogamma:
			case opt_nowshading:
			case opt_emulategray:
			case opt_dbgimages:
			case opt_nowarmup:
			case opt_realdepth:
			case opt_depth:
			case opt_resolution:
			case opt_threshold:
			case opt_preview:
			case opt_brx:
			case opt_tlx:
			case opt_bry:
			case opt_tly:
				*(SANE_Word *) result = s->aValues[optid].w;
				break;

			/* SANE_Int */
			case opt_chipid:
			case opt_scancount:
				*(SANE_Int *) result = s->aValues[optid].w;
				break;

			/* SANE_Word array */
			case opt_gamma_red:
			case opt_gamma_green:
			case opt_gamma_blue:
				memcpy(result, s->aValues[optid].wa, s->aOptions[optid].size);
				break;

			/* String */
			case opt_colormode:
			case opt_scantype:
			case opt_model:
			case opt_chipname:
				strncpy(result, s->aValues[optid].s, s->aOptions[optid].size);
				((char*)result)[s->aOptions[optid].size-1] = '\0';
				break;

			/* scanner buttons */
			case opt_button_0:
				get_button_status(s);
			case opt_button_1:
			case opt_button_2:
			case opt_button_3:
			case opt_button_4:
			case opt_button_5:
				/* copy the button state */
				*(SANE_Word*) result = s->aValues[optid].w;
				/* clear the button state */
				s->aValues[optid].w = SANE_FALSE;
				break;
		}
	}

	return SANE_STATUS_GOOD;
}

static SANE_Status option_set(TScanner *s, SANE_Int optid, void *value, SANE_Int *pInfo)
{
	SANE_Status rst;

	DBG(DBG_FNC, "> option_set(optid=%i)\n", optid);

	rst = SANE_STATUS_INVAL;

	if (s != NULL)
	{
		if (s->fScanning == SANE_FALSE)
		{
			SANE_Int info = 0;

			rst = SANE_STATUS_GOOD;

			switch (optid)
			{
				case opt_brx:
				case opt_tlx:
				case opt_bry:
				case opt_tly:
				case opt_depth:
				case opt_nogamma:
				case opt_nowshading:
				case opt_nowarmup:
				case opt_negative:
				case opt_emulategray:
				case opt_dbgimages:
				case opt_threshold:
				case opt_resolution:
				case opt_preview:
					info |= SANE_INFO_RELOAD_PARAMS;
					s->aValues[optid].w = *(SANE_Word *) value;
					break;

				case opt_gamma_red:
				case opt_gamma_green:
				case opt_gamma_blue:
					memcpy (s->aValues[optid].wa, value, s->aOptions[optid].size);
					break;

				case opt_scantype:
					if (strcmp(s->aValues[optid].s, value) != 0)
					{
						struct st_coords *coords;
						SANE_Int source;

						if (s->aValues[optid].s)
							free (s->aValues[optid].s);

						s->aValues[optid].s = strdup(value);

						source = Get_Source(s->aValues[opt_scantype].s);
						coords = RTS_area_get(s->device, source);
						if (coords != NULL)
						{
							bknd_constrains(s, source, 0);
							bknd_constrains(s, source, 1);
							s->aValues[opt_tlx].w = 0;
							s->aValues[opt_tly].w = 0;
							s->aValues[opt_brx].w = coords->width;
							s->aValues[opt_bry].w = coords->height;
						}

						info |= SANE_INFO_RELOAD_PARAMS | SANE_INFO_RELOAD_OPTIONS;
					}
					break;

				case opt_colormode:
					if (strcmp(s->aValues[optid].s, value) != 0)
					{
						if (s->aValues[optid].s)
							free (s->aValues[optid].s);
						s->aValues[optid].s = strdup(value);
						if (Get_Colormode(s->aValues[optid].s) == CM_LINEART)
							s->aOptions[opt_threshold].cap &= ~SANE_CAP_INACTIVE;
								else s->aOptions[opt_threshold].cap |= SANE_CAP_INACTIVE;
						info |= SANE_INFO_RELOAD_PARAMS | SANE_INFO_RELOAD_OPTIONS;
					}
					break;

				case opt_model:
					if (strcmp(s->aValues[optid].s, value) != 0)
					{
						SANE_Int model;

						if (s->aValues[optid].s)
							free (s->aValues[optid].s);
						s->aValues[optid].s = strdup(value);

						model = Get_Model(s->aValues[optid].s);
						if (model != s->device->model)
						{
							SANE_Int source;
							struct st_coords *coords;

							/* free configuration of last model */
							RTS_free_config(s->device);

							/* set new model */
							s->device->model = model;

							/* and load configuration of current model */
							RTS_load_config(s->device);

							/* update options according to selected device */
							bknd_info(s);
							bknd_colormodes(s, model);
							bknd_depths(s, model);
							bknd_resolutions(s, model);
							bknd_sources(s, model);

							/* updating lists */
							s->aOptions[opt_colormode].size = max_string_size2(s->list_colormodes);
							s->aOptions[opt_colormode].constraint.string_list = s->list_colormodes;
							s->aOptions[opt_depth].constraint.word_list = s->list_depths;
							s->aOptions[opt_resolution].constraint.word_list = s->list_resolutions;
							s->aOptions[opt_scantype].size = max_string_size2(s->list_sources);
							s->aOptions[opt_scantype].constraint.string_list = s->list_sources;

							/* default values */
							if (s->aValues[opt_colormode].s != NULL)
								free(s->aValues[opt_colormode].s);

							if (s->aValues[opt_scantype].s != NULL)
								free(s->aValues[opt_scantype].s);

							s->aValues[opt_colormode].s  = strdup(s->list_colormodes[0]);
							s->aValues[opt_scantype].s   = strdup(s->list_sources[0]);
							s->aValues[opt_resolution].w = s->list_resolutions[1];
							s->aValues[opt_depth].w      = s->list_depths[1];

							source = Get_Source(s->aValues[opt_scantype].s);
							coords = RTS_area_get(s->device, source);
							if (coords != NULL)
							{
								bknd_constrains(s, source, 0);
								bknd_constrains(s, source, 1);
								s->aValues[opt_tlx].w = 0;
								s->aValues[opt_tly].w = 0;
								s->aValues[opt_brx].w = coords->width;
								s->aValues[opt_bry].w = coords->height;
							}
						}

						info |= SANE_INFO_RELOAD_PARAMS | SANE_INFO_RELOAD_OPTIONS;
					}
					break;

				case opt_reset:
					RTS_chip_reset(s->device);
					break;

				case opt_realdepth:
					s->aValues[optid].w = s->cnv.real_depth;
					break;

				case opt_infoupdate:
					if (bknd_info(s) == SANE_STATUS_GOOD)
						info |= SANE_INFO_RELOAD_OPTIONS;
					break;

				default:
					rst = SANE_STATUS_INVAL;
					break;
			}

			if (pInfo != NULL)
				*pInfo = info;
		}
	}

	return rst;
}

SANE_Status sane_control_option (SANE_Handle h, SANE_Int n, SANE_Action Action, void *pVal, SANE_Int * pInfo)
{
	TScanner *s;
	SANE_Status rst;

	DBG(DBG_FNC, "> sane_control_option\n");

	s = (TScanner *) h;

	switch(Action)
	{
		case SANE_ACTION_GET_VALUE:
			rst = option_get(s, n, pVal);
			break;

		case SANE_ACTION_SET_VALUE:
			rst = option_set(s, n, pVal, pInfo);
			break;

		case SANE_ACTION_SET_AUTO:
			rst = SANE_STATUS_UNSUPPORTED;
			break;

		default:
			rst = SANE_STATUS_INVAL;
			break;
	}

	return rst;
}

SANE_Status sane_get_parameters (SANE_Handle h, SANE_Parameters * p)
{
	SANE_Status rst = SANE_STATUS_INVAL;
	TScanner *s;

	DBG(DBG_FNC, "+ sane_get_parameters:");

	s = (TScanner *) h;
	if (s != NULL)
	{
		struct st_coords coords;
		SANE_Int res, source, depth, colormode, frameformat, bpl;

		/* first do some checks */

		/* colormode */
		colormode = Get_Colormode(s->aValues[opt_colormode].s);

		/* frameformat */
		frameformat = (colormode == CM_COLOR)? SANE_FRAME_RGB: SANE_FRAME_GRAY;

		/* depth */
		depth = (colormode == CM_LINEART)? 1: s->aValues[opt_depth].w;

		/* scan type */
		source = Get_Source(s->aValues[opt_scantype].s);

		/* resolution */
		res = s->aValues[opt_resolution].w;

		/* image coordinates in milimeters */
		coords.left   = s->aValues[opt_tlx].w;
		coords.top    = s->aValues[opt_tly].w;
		coords.width  = s->aValues[opt_brx].w;
		coords.height = s->aValues[opt_bry].w;

		/* validate coords */
		if (Translate_coords(&coords) == SANE_STATUS_GOOD)
		{
			Set_Coordinates(s, source, res, &coords);

			if (colormode != CM_LINEART)
			{
				bpl = coords.width * ((depth > 8)? 2: 1);
				if (colormode == CM_COLOR)
					bpl *= 3; /* three channels */
			} else bpl = (coords.width + 7) / 8;

			/* return the data */
			p->format          = frameformat;
			p->last_frame      = SANE_TRUE;
			p->depth           = depth;
			p->lines           = coords.height;
			p->pixels_per_line = coords.width;
			p->bytes_per_line  = bpl;

			DBG(DBG_FNC, " -> Depth : %i\n", depth);
			DBG(DBG_FNC, " -> Height: %i\n", coords.height);
			DBG(DBG_FNC, " -> Width : %i\n", coords.width);
			DBG(DBG_FNC, " -> BPL   : %i\n", bpl);

			rst = SANE_STATUS_GOOD;
		}
	}

	DBG(DBG_FNC, "- sane_get_parameters: %i\n", rst);

	return rst;
}

SANE_Status sane_start(SANE_Handle h)
{
	SANE_Status rst = SANE_STATUS_INVAL;
	TScanner *s;

	DBG(DBG_FNC, "+ sane_start\n");

	s = (TScanner *) h;
	if (s != NULL)
	{
		struct st_coords coords;
		SANE_Int res, source, colormode, depth, channel;

		/* first do some checks */
		/* Get Scan type */
		source = Get_Source(s->aValues[opt_scantype].s);

		/* Check if scanner supports slides and negatives in case selected source is tma */
		if (!((source != ST_NORMAL)&&(RTS_isTmaAttached(s->device) == SANE_FALSE)))
		{
			/* Get depth */
			depth = s->aValues[opt_depth].w;

			/* Get color mode*/
			colormode = Get_Colormode(s->aValues[opt_colormode].s);

			/* Emulating certain color modes */
			if (colormode == CM_LINEART)
			{
				/* emulate lineart */
				s->cnv.colormode = CM_LINEART;
				colormode = CM_GRAY;
				depth = 8;
			} else if ((colormode == CM_GRAY)&&(s->aValues[opt_emulategray].w == SANE_TRUE))
			{
				/* emulate grayscale */
				s->cnv.colormode = CM_GRAY;
				colormode = CM_COLOR;
			} else s->cnv.colormode = -1;

			/* setting channel for colormodes different than CM_COLOR */
			channel = (colormode != CM_COLOR)? 1 : 0;

			/* negative colors */
			s->cnv.negative = s->aValues[opt_negative].w;

			/* Get threshold */
			s->cnv.threshold = s->aValues[opt_threshold].w;

			/* Get resolution */
			res = s->aValues[opt_resolution].w;

			/* set depth emulation */
			if (s->cnv.colormode == CM_LINEART)
				s->cnv.real_depth = SANE_TRUE;
					else s->cnv.real_depth = s->aValues[opt_realdepth].w;

			/* is preview */
			s->device->status->preview = s->aValues[opt_preview].w;

			/* use gamma? */
			s->device->options->use_gamma = (s->aValues[opt_nogamma].w == SANE_TRUE)? SANE_FALSE: SANE_TRUE;

			/* disable white shading correction? */
			s->device->options->shd_white = (s->aValues[opt_nowshading].w == SANE_TRUE)? SANE_FALSE: SANE_TRUE;

			/* skip warmup process? */
			s->device->options->do_warmup = (s->aValues[opt_nowarmup].w == SANE_TRUE)? SANE_FALSE: SANE_TRUE;

			/* save debugging images? */
			s->device->options->dbg_image = s->aValues[opt_dbgimages].w;

			/* Get image coordinates in milimeters */
			coords.left   = s->aValues[opt_tlx].w;
			coords.top    = s->aValues[opt_tly].w;
			coords.width  = s->aValues[opt_brx].w;
			coords.height = s->aValues[opt_bry].w;

			/* Validate coords */
			if (Translate_coords(&coords) == SANE_STATUS_GOOD)
			{

				/* Stop previusly started scan */
				RTS_scanner_stop(s->device, SANE_TRUE);

				s->ScanParams.scantype     = source;
				s->ScanParams.colormode    = colormode;
				s->ScanParams.resolution_x = res;
				s->ScanParams.resolution_y = res;
				s->ScanParams.channel      = channel;

				memcpy(&s->ScanParams.coords, &coords, sizeof(struct st_coords));
				Set_Coordinates(s, source, res, &s->ScanParams.coords);

				/* emulating depth? */
				if ((s->cnv.real_depth == SANE_FALSE)&&(depth < 16)&&(s->device->options->use_gamma == SANE_TRUE))
				{
					/* In order to improve image quality, we will scan at 16bits if
						we are using gamma correction */
					s->cnv.depth = depth;
					s->ScanParams.depth = 16;
				} else
				{
					s->ScanParams.depth = depth;
					s->cnv.depth = -1;
				}

				/* set scanning parameters */
				if (RTS_scanner_setup(s->device, &s->ScanParams) == SANE_STATUS_GOOD)
				{
					/* Start scanning process */
					if (RTS_scanner_start(s->device) == SANE_STATUS_GOOD)
					{
						/* Allocate buffer to read one line */
						s->mylin = 0;
						rst = img_buffers_alloc(s, bytesperline);
					}
				}
			}
		} else rst = SANE_STATUS_COVER_OPEN;
	}

	DBG(DBG_FNC, "- sane_start: %i\n", rst);

	return rst;
}

SANE_Status sane_read(SANE_Handle h, SANE_Byte *buf, SANE_Int maxlen, SANE_Int *len)
{
	SANE_Status rst = SANE_STATUS_GOOD;
	TScanner *s;

	DBG(DBG_FNC, "+ sane_read\n");

	s = (TScanner *) h;
	if ((s != NULL)&&(buf != NULL)&&(len != NULL))
	{
		/* nothing has been read at the moment */
		*len = 0;

		/* if we read all the lines return EOF */
		if ((s->mylin == s->ScanParams.coords.height)||(s->device->status->cancel == SANE_TRUE))
		{
			rst = (s->device->status->cancel == SANE_TRUE)? SANE_STATUS_CANCELLED : SANE_STATUS_EOF;

			RTS_scanner_stop(s->device, SANE_FALSE);
			img_buffers_free(s);
		} else
		{
			SANE_Int emul_len, emul_maxlen;
			SANE_Int thwidth, transferred, bufflength;
			SANE_Byte *buffer, *pbuffer;

			emul_len = 0;
			if (s->cnv.depth != -1)
				emul_maxlen = maxlen * (s->ScanParams.depth / s->cnv.depth);
					else emul_maxlen = maxlen;

			/* if grayscale emulation is enabled check that retrieved data is multiple of three */
			if (s->cnv.colormode == CM_GRAY)
			{
				SANE_Int chn_size, rest;

				chn_size = (s->ScanParams.depth > 8)? 2: 1;
				rest = emul_maxlen % (3 * chn_size);

				if (rest != 0)
					emul_maxlen -= rest;
			}

			/* this is important to keep lines alignment in lineart mode */
			if (s->cnv.colormode == CM_LINEART)
				emul_maxlen = s->ScanParams.coords.width;

			/* if we are emulating depth, we scan at 16bit when frontend waits
			   for 8bit data. Next buffer will be used to retrieve data from
			   scanner prior to convert to 8 bits depth */
			buffer = (SANE_Byte *)malloc(emul_maxlen * sizeof(SANE_Byte));

			if (buffer != NULL)
			{
				pbuffer = buffer;

				/* get bytes per line */
				if (s->ScanParams.colormode != CM_LINEART)
				{
					thwidth = s->ScanParams.coords.width * ((s->ScanParams.depth > 8)? 2: 1);

					if (s->ScanParams.colormode == CM_COLOR)
						thwidth *= 3; /* three channels */
				} else thwidth = (s->ScanParams.coords.width + 7) / 8;

				/* read as many lines the buffer may contain and while there are lines to be read */
				while ((emul_len < emul_maxlen)&&(s->mylin < s->ScanParams.coords.height))
				{
					/* Is there any data waiting for being passed ? */
					if (s->rest_amount != 0)
					{
						/* copy to buffer as many bytes as we can */
						bufflength = min(emul_maxlen - emul_len, s->rest_amount);
						memcpy(pbuffer, s->rest, bufflength);
						emul_len += bufflength;
						pbuffer += bufflength;
						s->rest_amount -= bufflength;
						if (s->rest_amount == 0)
							s->mylin++;
					} else
					{
						/* read from scanner up to one line */
						if (RTS_scanner_read(s->device, bytesperline, s->image, &transferred) != SANE_STATUS_GOOD)
						{
							/* error, exit function */
							rst = SANE_STATUS_EOF;
							break;
						}

						/* is there any data? */
						if (transferred != 0)
						{
							/* copy to buffer as many bytes as we can */
							bufflength = min(emul_maxlen - emul_len, thwidth);

							memcpy(pbuffer, s->image, bufflength);
							emul_len += bufflength;
							pbuffer += bufflength;

							/* the rest will be copied to s->rest buffer */
							if (bufflength < thwidth)
							{
								s->rest_amount = thwidth - bufflength;
								memcpy(s->rest, s->image + bufflength, s->rest_amount);
							} else s->mylin++;
						} else break;
					}
				} /* while */

				/* process buffer before sending to frontend */
				if ((emul_len > 0)&&(rst != SANE_STATUS_EOF))
				{
					/* at this point ...
					   buffer  : contains retrieved image
					   emul_len: contains size in bytes of retrieved image

					   after this code ...
					   buf : will contain postprocessed image
					   len : will contain size in bytes of postprocessed image */

					/* apply gamma if neccesary */
					if (s->device->options->use_gamma == SANE_TRUE)
						gamma_apply(s, buffer, emul_len , s->ScanParams.depth);

					/* if we are scanning negatives, let's invert colors */
					if (s->ScanParams.scantype == ST_NEG)
					{
						if (s->cnv.negative == SANE_FALSE)
							Color_Negative(buffer, emul_len, s->ScanParams.depth);
					} else if (s->cnv.negative != SANE_FALSE)
							Color_Negative(buffer, emul_len, s->ScanParams.depth);

					/* emulating grayscale ? */
					if (s->cnv.colormode == CM_GRAY)
					{
						Color_to_Gray(buffer, emul_len, s->ScanParams.depth);
						emul_len /= 3;
					}

					/* emulating depth */
					if (s->cnv.depth != -1)
					{
						switch(s->cnv.depth)
						{
							/* case 1: treated separately as lineart */
							/*case 12: in the future*/
							case 8:
								Depth_16_to_8(buffer, emul_len, buffer);
								emul_len /= 2;
								break;
						}
					}

					/* lineart mode ? */
					if (s->cnv.colormode == CM_LINEART)
					{
						/* I didn't see any scanner supporting lineart mode.
						   Windows drivers scan in grayscale and then convert image to lineart
						   so let's perform convertion */
						SANE_Int rest = emul_len % 8;
						
						Gray_to_Lineart(buffer, emul_len, s->cnv.threshold);
						emul_len /= 8;
						if (rest > 0)
							emul_len++;
					}

					/* copy postprocessed image */
					*len = emul_len;
					memcpy(buf, buffer, *len);
				}

				free(buffer);
			}
		}
	} else rst = SANE_STATUS_EOF;

	DBG(DBG_FNC, "- sane_read: %s\n", sane_strstatus(rst));

	return rst;
}

void sane_cancel(SANE_Handle h)
{
	TScanner *s;

	DBG(DBG_FNC, "> sane_cancel\n");

	/* silence gcc */
	s = (TScanner *) h;
	if (s != NULL)
		s->device->status->cancel = SANE_TRUE;
}

SANE_Status sane_set_io_mode(SANE_Handle handle, SANE_Bool non_blocking)
{
	DBG(DBG_FNC, "> sane_set_io_mode\n");

	/* silence gcc */
	handle = handle;
	non_blocking = non_blocking;

	return SANE_STATUS_UNSUPPORTED;
}

SANE_Status sane_get_select_fd(SANE_Handle handle, SANE_Int * fd)
{
	DBG(DBG_FNC, "> sane_get_select_fd\n");

	/* silence gcc */
	handle = handle;
	fd = fd;

	return SANE_STATUS_UNSUPPORTED;
}

void sane_close (SANE_Handle h)
{
	TScanner *s;

	DBG(DBG_FNC,"- sane_close...\n");

	s = (TScanner *) h;
	if (s != NULL)
	{
		/* stop previus scans */
		RTS_scanner_stop(s->device, SANE_TRUE);

		/* close usb */
		sanei_usb_close(s->device->usb->handle);

		/* free RTS enviroment */
		RTS_free(s->device);

		/* free backend variables */
		options_free(s);

		img_buffers_free(s);
	}
}

void sane_exit(void)
{
	/* free device list memory */
	if (_pSaneDevList)
	{
		TDevListEntry *pDev, *pNext;

		for (pDev = _pFirstSaneDev; pDev; pDev = pNext)
		{
			pNext = pDev->pNext;
			/* pDev->dev.name is the same pointer that pDev->devname */
			free(pDev->devname);
			free(pDev);
		}

		_pFirstSaneDev = NULL;
		free(_pSaneDevList);
		_pSaneDevList = NULL;
	}
}
