/* HP Scanjet 3900 series - Stand-alone Backend controller

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

/*#define developing*/

/* operations we can do once device is initialized */
#define OP_SCAN     0x00   /* scan image            */
#define OP_RESET    0x01   /* resets chipset        */
#define OP_BUTTONS  0x02   /* testing buttons       */
#define OP_CHIPSET  0x03   /* show chipset name     */
#define OP_SCANCNT  0x04   /* show scans counter    */
#define OP_QBUTTONS 0x05   /* Wait for button press, then report and exit */

#define GAMMA_DEFAULT 1.0
#define GAMMA_DEPTH    16

#include <usb.h>

/* in standalone app, tiffio libraries are mandatory*/
#define HAVE_TIFFIO_H

/* next definitions and typedefs are set to be compatible with SANE API */
#define SANE_FALSE       0x00
#define SANE_TRUE        0x01
/*#define SANE_STATUS_GOOD 0x00*/

typedef unsigned char SANE_Byte;
typedef int SANE_Int;
typedef enum
{
	SANE_STATUS_GOOD = 0,
	SANE_STATUS_INVAL
} SANE_Status;

typedef usb_dev_handle * USB_Handle;

/*now we can include rts core */
#include "hp3900_rts8822.c"

struct st_convert
{
	SANE_Int colormode;
	SANE_Int depth;
	SANE_Int threshold;
	SANE_Int negative;
	SANE_Int real_depth;
};

struct st_gamma
{
	float      correction[3];
	SANE_Byte  *table[3];
};

static SANE_Status proof(st_device *dev);

/* managing device */
static SANE_Status        device_close        (usb_dev_handle *usb_handle);
static SANE_Status        device_open         (struct usb_device *dev);
static struct usb_device *device_usb_get      (SANE_Int ibus, SANE_Int idev);
static struct usb_device *device_usb_get_first(void);
static void               device_usb_list     (void);

/* image tiff file format */
static TIFF       *TIFF_Open (char *sFile, SANE_Int width, SANE_Int height, SANE_Int depth, SANE_Int colortype, SANE_Int resolution_x, SANE_Int resolution_y, SANE_Int channels);
static SANE_Status TIFF_Add  (TIFF *image, SANE_Byte *buffer, SANE_Int size);
static void        TIFF_Fill (TIFF *image, SANE_Int size);
static void        TIFF_Close(TIFF *image);

/* application related */
static void        Silent_Compile(void);
static SANE_Status Parse_args(st_device *dev, SANE_Int argc, char *argv[], struct params *data);

/* displaying functions */
static void show_app_header  (SANE_Int do_exit);
static void show_args        (SANE_Int argc, char *argv[]);
static void show_buttons     (void);
static void show_RTS_chip_name(SANE_Int do_exit);
static void show_help        (void);
static void show_help_debug  (void);
static void show_scancount   (void);
static void show_version     (SANE_Int do_exit);

static void query_buttons (void);

static SANE_Status make_scan(struct params *data);

static SANE_Status Get_Image(struct params *data);
static void        Set_Coordinates(SANE_Int scantype, SANE_Int resolution, struct st_coords *coords);

/* gamma */
static void        Gamma_Init(void);
static SANE_Status Gamma_CreateTable(SANE_Byte *table, double gamma, SANE_Int depth);
static void        Gamma_CheckConstrains(double *gamma);
static void        Gamma_Free(void);

/* converters */
static void Depth_16_to_8  (SANE_Byte *from_buffer, SANE_Int size, SANE_Byte *to_buffer);
static void Color_to_Gray  (SANE_Byte *buffer, SANE_Int size, SANE_Int depth);
static void Color_Negative (SANE_Byte *buffer, SANE_Int size, SANE_Int depth);
static void Gray_to_Lineart(SANE_Byte *buffer, SANE_Int size, SANE_Int threshold);

/* variables */
static SANE_Int  app_operation = OP_SCAN; /* scan by default */
static SANE_Int  Scanner_Model = -1;
static char     *File_Path;
static SANE_Int  ubus = -1;
static SANE_Int  udev = -1;
static SANE_Byte inches = SANE_FALSE;
static SANE_Byte verbose = SANE_FALSE;

static struct usb_device *scanner = NULL;
static struct st_gamma    myGamma;
static struct st_convert  cnv;

static st_device *device = NULL;

/* main function */
int main(SANE_Int argc, char *argv[])
{
	int rst = SANE_STATUS_INVAL;
	struct params data;

	/* Silencing unused functions */
	Silent_Compile();

	File_Path = (char *)malloc(512);
	if (File_Path == NULL)
	{
		DBG(DBG_ERR, "Couldn't allocate memory for image file path. Exiting...\n");
		return SANE_STATUS_INVAL;
	} else bzero(File_Path, 512);

	/* Initialize USB */
	usb_init();
	/* usb_set_debug(0x02); */

	/* Setting default data */
	data.scantype      = ST_NORMAL;
	data.colormode     = 0;
	data.resolution_x  = 100;
	data.resolution_y  = 100;
	data.depth         = 8;
	data.channel       = 0;
	data.coords.left   = -1;
	data.coords.top    = -1;
	data.coords.width  = -1;
	data.coords.height = -1;

	/* convertion variables */
	cnv.colormode  = -1;
	cnv.depth      = -1;
	cnv.threshold  = 0x80;
	cnv.negative   = SANE_FALSE;
	cnv.real_depth = SANE_FALSE;

	/* allocating space for rts enviroment */
	device = RTS_alloc();
	if (device == NULL)
	{
		DBG(DBG_ERR, "Error allocating memory for RTS enviroment\n");
		exit(SANE_STATUS_INVAL);
	}

	/* Parse arguments */
	Parse_args(device, argc, argv, &data);

	/* Show parameters passed to application */
	show_args(argc, argv);

	/* emulating depth? */
	if ((cnv.real_depth == SANE_FALSE)&&(data.depth < 16)&&(device->options->use_gamma == SANE_TRUE))
	{
		/* In order to improve image quality, we will scan at 16bits if
		   we are using gamma correction */
		cnv.depth = data.depth;
		data.depth = 16;
	}

	/* Setting destiny file */
	if (File_Path[0] == '\0')
		snprintf(File_Path, 512, "./image.tiff");

	/* Create gamma tables with given depth */
	Gamma_Init();

	/* Show application header */
	if (verbose == SANE_TRUE)
		show_app_header(0);

	if ((ubus == -1)||(udev == -1))
		scanner = device_usb_get_first();
			else scanner = device_usb_get(ubus, udev);

	if (scanner == NULL)
	{
		/* if no device is set, try to find one available */
		DBG(DBG_VRB, "- Device not found in bus %i dev %i\n", ubus, udev);

		scanner = device_usb_get_first();
	}

	if (scanner != NULL)
	{
		/* this function only sets device->model var according to device's vendor and product ids */
		RTS_device_set(device, Scanner_Model, scanner->descriptor.idProduct, scanner->descriptor.idVendor);

		/* Accessing device via usb */
		DBG(DBG_VRB, "- Accessing device via USB...\n");
		if (device_open(scanner) == SANE_STATUS_GOOD)
		{
			/* all operations need scanner initialization except for chipset reset */
			if (app_operation != OP_RESET)
			{
				/* Initialice device */
				DBG(DBG_VRB, "- Initializing device...\n");
				if (RTS_scanner_init(device) == SANE_STATUS_GOOD)
				{
					/*proof(device);
					exit(0);*/

					/* make selected operation */
					switch(app_operation)
					{
						case OP_SCANCNT:
							show_scancount();
							break;
						case OP_CHIPSET:
							show_RTS_chip_name(0);
							break;
						case OP_BUTTONS:
							show_buttons();
							break;
						case OP_QBUTTONS:
							query_buttons();
							break;
						default:
							make_scan(&data);
							break;
					}

				} else DBG(DBG_ERR, "ERROR: Could not initialize scanner\n");
			} else RTS_chip_reset(device);

			/* close device */
			DBG(DBG_VRB, "- Closing device...\n");

			device_close(device->usb->handle);
		} else DBG(DBG_ERR, "ERROR: Could not access device via usb\n");
	} else DBG(DBG_ERR, "ERROR: Could not find any supported scanner\n");

	free(File_Path);

	/* Destroy gamma tables */
	Gamma_Free();

	/* free RTS enviroment */
	RTS_free(device);

	return rst;
}

static SANE_Status device_open(struct usb_device *dev)
{
	usb_dev_handle *myhandle = NULL;
	SANE_Status rst = SANE_STATUS_INVAL;
	
	/* if some device is already opened, close it */
	if (device->usb->handle != NULL)
		device_close(device->usb->handle);
	
	if (dev != NULL)
	{
		/* open device */
		myhandle = usb_open(dev);
	
		if (myhandle != NULL)
		{
			if (usb_set_configuration(myhandle, 0x01) == 0)
			{
				/* Using usb_claim_interface is mandatory to use usb_bulk_write */
				if (usb_claim_interface(myhandle, 0x00) == 0)
				{
					usb_set_altinterface(myhandle, 0x00);
					device->usb->handle = myhandle;
					rst = SANE_STATUS_GOOD;
				}
			}
			if (rst == SANE_STATUS_INVAL)
				usb_close(myhandle);
		}
	}

	DBG(DBG_FNC, "> device_open(0x%04x:0x%04x): %i\n", scanner->descriptor.idVendor, scanner->descriptor.idProduct, rst);

	return rst;
}

static SANE_Status device_close(usb_dev_handle *usb_handle)
{
	if (usb_handle != NULL)
	{
		usb_release_interface(usb_handle, 0);
		usb_close(usb_handle);
		usb_handle = NULL;
	}
	
	return SANE_STATUS_GOOD;
}

static void show_app_header(SANE_Int do_exit)
{
	SANE_Int dl = DBG_LEVEL;

	/* Change debug level to show app header */
	if (DBG_LEVEL < 1)
		DBG_LEVEL = 1;

	DBG(DBG_VRB, "hp3900 v%s - Realtek RTS8822 chipset based scanners controller\n", BACKEND_VRSN);
	DBG(DBG_VRB, "Coded by %s - %s\n", BACKEND_AUTHOR, BACKEND_EMAIL);
	DBG(DBG_VRB, "Web: %s\n", BACKEND_URL);
	DBG(DBG_VRB, "License: %s\n\n", BACKEND_LICENSE);

	/* Restore debug level */
	DBG_LEVEL = dl;

	if (do_exit != 0)
		exit(0);
}

static void show_version(SANE_Int do_exit)
{
	SANE_Int dl = DBG_LEVEL;

	/* Change debug level to show app version */
	if (DBG_LEVEL < 1)
		DBG_LEVEL = 1;

	DBG(DBG_VRB, "%s\n", BACKEND_VRSN);

	/* Restore debug level */
	DBG_LEVEL = dl;

	if (do_exit != 0)
		exit(0);
}

static SANE_Status Parse_args(st_device *dev, SANE_Int argc, char *argv[], struct params *data)
{
	SANE_Int C;
	SANE_Byte Setting = 0;
	SANE_Int Value = 0;
	double dbValue = 0;
	char *op = NULL;
	double fvalue = 0;

	if (argc > 1)
	{
		C = 1;
		while (C < argc)
		{
			op = argv[C];
			if (op != NULL)
			{
				/* Is option or data setting? */
				if (Setting > 0)
				{
					switch (Setting)
					{
						case 1:
							if (inches == SANE_TRUE)
							{
								fvalue = atof(op);
								data->coords.left = ceil(fvalue * 25.4);
							} else data->coords.left = atoi(op);
							break;
						case 2:
							if (inches == SANE_TRUE)
							{
								fvalue = atof(op);
								data->coords.width = ceil(fvalue * 25.4);
							} else data->coords.width = atoi(op);
							break;
						case 3:
							if (inches == SANE_TRUE)
							{
								fvalue = atof(op);
								data->coords.top = ceil(fvalue * 25.4);
							} else data->coords.top = atoi(op);
							break;
						case 4:
							if (inches == SANE_TRUE)
							{
								fvalue = atof(op);
								data->coords.height = ceil(fvalue * 25.4);
							} else data->coords.height = atoi(op);
							break;
						case 5:
							Value = atoi(op);
							switch(Value)
							{
								/* Only 8 and 16 bit allowed until debugging windows driver */
								/*case 1:
									data->colormode = CM_LINEART;
									data->depth = Value;
									break;*/
								case 8:
								/*case 12:*/
								case 16:
									if (data->colormode == CM_LINEART)
										data->colormode = CM_COLOR;
									data->depth = Value;
									break;
								default:
									if (data->colormode == CM_LINEART)
										data->colormode = CM_COLOR;
									data->depth = 8;
									break;
							}
							break;
						case 6:
							Value = atoi(op);
							if ((Value < ST_NORMAL)||(Value > ST_NEG))
								Value = ST_NORMAL;
							data->scantype = Value;
							break;
						case 7: /* --res */
							Value = atoi(op);
							/*if ((Value < 10)||(Value > 2400))
								Value = 100;*/
							data->resolution_x = Value;
							data->resolution_y = Value;
							break;
						case 8: /* --resx */
							Value = atoi(op);
							if ((Value < 10)||(Value > 9999))
								Value = 100;
							data->resolution_x = Value;
							break;
						case 9: /* --resy */
							Value = atoi(op);
							if ((Value < 10)||(Value > 9999))
								Value = 100;
							data->resolution_y = Value;
							break;
						case 10: /* --debug */
							Value = atoi(op);
							if (Value > 4)
								Value = 0;
							DBG_LEVEL = Value;
							break;
						case 11: /* --file */
							snprintf(File_Path, 512, "%s", op);
							break;
						case 12: /* --bus */
							ubus = atoi(op);
							break;
						case 13: /* --dev */
							udev = atoi(op);
							break;
						case 14: /* --color */
							Value = atoi(op);
							if ((Value < 0)||(Value > 3))
								Value = 0;
							switch(Value)
							{
								case 0: /* Color */
									data->colormode = CM_COLOR;
									cnv.colormode = -1;
									if (data->depth < 8)
										data->depth = 8;
									data->channel = 0;
									/* Disabled until debugging driver
									data->colormode = 3;
									if (data->depth < 8)
										data->depth = 8;
									*/
									break;
								case 1: /* Real Grayscale */
									data->colormode = CM_GRAY;
									data->channel = 1;
									cnv.colormode = -1;
									if (data->depth < 8)
										data->depth = 8;
									break;
								case 2: /* Lineart */
									#ifdef developing
									cnv.colormode = -1;
									data->colormode = CM_LINEART;
									data->depth = 8;
									data->channel = 1;
									#else
									data->colormode = CM_GRAY;
									data->depth = 8;
									cnv.colormode = CM_LINEART;
									cnv.depth = 1;
									cnv.real_depth = SANE_TRUE;
									#endif
									break;
								case 3: /* Emulated Grayscale */
									data->colormode = CM_COLOR;
									data->channel = 0;
									cnv.colormode = CM_GRAY;
									if (data->depth < 8)
										data->depth = 8;
									break;
							}
							break;
						case 15: /* gamma */
							dbValue = atof(op);
							Gamma_CheckConstrains(&dbValue);
							myGamma.correction[CL_RED]   = dbValue;
							myGamma.correction[CL_GREEN] = dbValue;
							myGamma.correction[CL_BLUE]  = dbValue;
							break;
						case 16: /* gammar */
							dbValue = atof(op);
							Gamma_CheckConstrains(&dbValue);
							myGamma.correction[CL_RED] = dbValue;
							break;
						case 17: /* gammag */
							dbValue = atof(op);
							Gamma_CheckConstrains(&dbValue);
							myGamma.correction[CL_GREEN] = dbValue;
							break;
						case 18: /* gammab */
							dbValue = atof(op);
							Gamma_CheckConstrains(&dbValue);
							myGamma.correction[CL_BLUE] = dbValue;
							break;
						case 19: /* --threshold */
							Value = atoi(op);
							if ((Value < 0)||(Value > 0x100))
								Value = 0x80;
							cnv.threshold = Value;
							break;
						case 29: /* --odtflb */
							Value = atoi(op);
							if (Value > 0)
								dev->options->overdrive_flb = Value;
							break;
						case 30: /* --odttma */
							Value = atoi(op);
							if (Value > 0)
								dev->options->overdrive_ta = Value;
							break;
						case 31: /* --model */
							Value = atoi(op);
							if ((Value >= 0)&&(Value < RTS_device_count()))
								Scanner_Model = Value;
							break;
						case 32: /* --usbtype */
							Value = atoi(op);
							if (Value == 0)
								dev->usb->type = USB11;
							else if (Value == 1)
								dev->usb->type = USB20;
							break;
					}
					Setting = 0;
				} else
				{
					/* Options */
					if (strcmp(op, "--help") == 0)
						show_help();
					else if (strcmp(op, "--inches") == 0)
						inches = SANE_TRUE;
					else if (strcmp(op, "--left") == 0)
						Setting = 1;
					else if (strcmp(op, "--width") == 0)
						Setting = 2;
					else if (strcmp(op, "--top") == 0)
						Setting = 3;
					else if (strcmp(op, "--height") == 0)
						Setting = 4;
					else if (strcmp(op, "--depth") == 0)
						Setting = 5;
					else if (strcmp(op, "--type") == 0)
						Setting = 6;
					else if (strcmp(op, "--nocal") == 0)
						dev->options->calibrate = SANE_FALSE;
					else if (strcmp(op, "--color") == 0)
						Setting = 14;
					else if (strcmp(op, "--res") == 0)
						Setting = 7;
					else  if (strcmp(op, "--resx") == 0)
						Setting = 8;
					else  if (strcmp(op, "--resy") == 0)
						Setting = 9;
					else  if (strcmp(op, "--debug") == 0)
						Setting = 10;
					else  if (strcmp(op, "--file") == 0)
						Setting = 11;
					else  if (strcmp(op, "--devs") == 0)
						device_usb_list();
					else  if (strcmp(op, "--bus") == 0)
						Setting = 12;
					else  if (strcmp(op, "--dev") == 0)
						Setting = 13;
					else  if (strcmp(op, "--verbose") == 0)
					{
						verbose = SANE_TRUE;
						if (DBG_LEVEL < 1)
							DBG_LEVEL = 1;
					}
					else  if (strcmp(op, "--about") == 0)
						show_app_header(1);
					else  if (strcmp(op, "--version") == 0)
						show_version(1);
					else  if (strcmp(op, "--gamma") == 0)
						Setting = 15;
					else  if (strcmp(op, "--gammar") == 0)
						Setting = 16;
					else  if (strcmp(op, "--gammag") == 0)
						Setting = 17;
					else  if (strcmp(op, "--gammab") == 0)
						Setting = 18;
					else  if (strcmp(op, "--threshold") == 0)
						Setting = 19;
					else  if (strcmp(op, "--chipset") == 0)
						app_operation = OP_CHIPSET;
					else  if (strcmp(op, "--dbgimage") == 0)
						dev->options->dbg_image = SANE_TRUE;
					else  if (strcmp(op, "--scanwhiteboard") == 0)
						dev->options->wht_board = SANE_TRUE;
					else  if (strcmp(op, "--chipset-reset") == 0)
						app_operation = OP_RESET;
					else  if (strcmp(op, "--disablefixedpwm") == 0)
						dev->options->fixed_pwm = SANE_FALSE;
					else  if (strcmp(op, "--help-debug") == 0)
						show_help_debug();
					else  if (strcmp(op, "--nowarmup") == 0)
						dev->options->do_warmup = SANE_FALSE;
					else  if (strcmp(op, "--preview") == 0)
						dev->status->preview = SANE_TRUE;
					else  if (strcmp(op, "--odtflb") == 0)
						Setting = 29;
					else  if (strcmp(op, "--odttma") == 0)
						Setting = 30;
					else  if (strcmp(op, "--model") == 0)
						Setting = 31;
					else  if (strcmp(op, "--usbtype") == 0)
						Setting = 32;
					else  if (strcmp(op, "--negative") == 0)
						cnv.negative = SANE_TRUE;
					else  if (strcmp(op, "--gamma-off") == 0)
						dev->options->use_gamma = SANE_FALSE;
					else  if (strcmp(op, "--real-depth") == 0)
						cnv.real_depth = SANE_TRUE;
					else  if (strcmp(op, "--buttons") == 0)
						app_operation = OP_BUTTONS;
					else if (strcmp (op, "--qbuttons") == 0)
						app_operation = OP_QBUTTONS;
					else  if (strcmp(op, "--scancount") == 0)
						app_operation = OP_SCANCNT;
					else  if (strcmp(op, "--nowshd") == 0)
						dev->options->shd_white = SANE_FALSE;
					else  if (strcmp(op, "--nobshd") == 0)
						dev->options->shd_black = SANE_FALSE;
					else  if (strcmp(op, "--noshd") == 0)
					{
						dev->options->shd_black = SANE_FALSE;
						dev->options->shd_white = SANE_FALSE;
					}
					else
					{
						DBG(DBG_ERR, "Unknown option '%s'. Try --help for more information.\n", op);
						exit(0);
					}
				}
			}
			C++;
		}
	}

	return SANE_STATUS_GOOD;
}

static void show_help()
{
	/* Show application header */
	if (DBG_LEVEL < 1) DBG_LEVEL = 1;

	show_app_header(0);

	DBG(DBG_VRB, "Usage: hp3900 [options]\n\n");
	DBG(DBG_VRB, "Arguments:\n");
	DBG(DBG_VRB, "--help        : Shows this help.\n");
	DBG(DBG_VRB, "--help-debug  : Shows debugging parameters.\n");
	DBG(DBG_VRB, "--devs        : Shows devices recognized by this controller.\n");
	DBG(DBG_VRB, "--bus    <num>: Sets device's usb bus number.\n");
	DBG(DBG_VRB, "--dev    <num>: Sets usb device number.\n");
	DBG(DBG_VRB, "--chipset     : Shows chipset model id and name\n");
	DBG(DBG_VRB, "--scancount   : Shows the number of scans made by scanner\n");
	DBG(DBG_VRB, "--inches      : Set coordinates in inches unit instead of milimiters.\n");
	DBG(DBG_VRB, "--left   <num>: Sets image left coord.\n");
	DBG(DBG_VRB, "--width  <num>: Sets image width to scan.\n");
	DBG(DBG_VRB, "--top    <num>: Sets image top coord.\n");
	DBG(DBG_VRB, "--height <num>: Sets image height to scan.\n");
	DBG(DBG_VRB, "--depth  <num>: Sets image depth.\n");
	DBG(DBG_VRB, "                Posible values are: 8 and 16\n");
	DBG(DBG_VRB, "                By default, 8 bits mode is used\n");
	DBG(DBG_VRB, "--nowarmup    : Skip lamp warmup process.\n");
	DBG(DBG_VRB, "--nocal       : Disables calibration process.\n");
	DBG(DBG_VRB, "--gamma  <num>: Sets gamma correction of three colour channels.\n");
	DBG(DBG_VRB, "                Posible values are between 0.30 and 3.00.\n");
	DBG(DBG_VRB, "                By default, 1.00 value is used\n");
	DBG(DBG_VRB, "--gammar <num>: Sets gamma correction in red colour channel.\n");
	DBG(DBG_VRB, "--gammag <num>: Sets gamma correction in green colour channel.\n");
	DBG(DBG_VRB, "--gammab <num>: Sets gamma correction in blue colour channel.\n");
	DBG(DBG_VRB, "--gamma-off   : Disables gamma correction.\n");
	DBG(DBG_VRB, "--type   <num>: Sets scan type. Should be the first argument\n");
	DBG(DBG_VRB, "                Posible values are:\n");
	DBG(DBG_VRB, "                1 : Normal scan (default)\n");
	DBG(DBG_VRB, "                2 : Slide scan\n");
	DBG(DBG_VRB, "                3 : Negative scan\n");
	DBG(DBG_VRB, "--color  <num>: Sets scan color\n");
	DBG(DBG_VRB, "                Posible values are:\n");
	DBG(DBG_VRB, "                0 : Colour (default)\n");
	DBG(DBG_VRB, "                1 : Gray\n");
	DBG(DBG_VRB, "                2 : Lineart\n");
	DBG(DBG_VRB, "                3 : Emulated Gray\n");
	DBG(DBG_VRB, "--threshold % : Sets threshold when scanning in lineart mode\n");
	DBG(DBG_VRB, "                Posible values are from 0 to 255\n");
	DBG(DBG_VRB, "--negative    : Image colours will be inverted.\n");
	DBG(DBG_VRB, "                In negative scans, this will be done by default.\n");
	DBG(DBG_VRB, "                Setting this option in negative scans will return a positive image.\n");
	DBG(DBG_VRB, "--res    <num>: Sets image horizontal and vertical resolution.\n");
	DBG(DBG_VRB, "                Any value is allowed between 10 and 2400\n");
	DBG(DBG_VRB, "                Standard values are: 100, 200, 300, 600, 1200 and 2400 dpi\n");
	DBG(DBG_VRB, "                Default resolution is 100 dpi\n");
	DBG(DBG_VRB, "--resx   <num>: Sets image horizontal resolution\n");
	DBG(DBG_VRB, "--resy   <num>: Sets image vertical resolution\n");
	DBG(DBG_VRB, "--file  <path>: Path where saving scanned image\n");
	DBG(DBG_VRB, "                Image is saved in TIFF format\n");
	DBG(DBG_VRB, "                By default file name is './image.tiff'\n");
	DBG(DBG_VRB, "--version     : Shows application version\n");
	DBG(DBG_VRB, "--verbose     : Shows scanning status messages\n");
	DBG(DBG_VRB, "--about       : Show application's information\n");

	DBG(DBG_VRB, "\n");

	exit(SANE_STATUS_GOOD);
}

static void show_help_debug()
{
	/* Show application header */
	if (DBG_LEVEL < 1) DBG_LEVEL = 1;

	show_app_header(0);

	DBG(DBG_VRB, "Debugging functions:\n");
	DBG(DBG_VRB, "--buttons        : Lets to test device's buttons.\n");
	DBG(DBG_VRB, "--qbuttons       : Wait for button press, then report and exit.\n");
	DBG(DBG_VRB, "--chipset-reset  : Resets chipset data\n");
	DBG(DBG_VRB, "--debug  <num>   : Shows debugging messages. Posible values are:\n");
	DBG(DBG_VRB, "                   0: Only errors (by default)\n");
	DBG(DBG_VRB, "                   1: Equal to verbose mode\n");
	DBG(DBG_VRB, "                   2: Functions and parameters\n");
	DBG(DBG_VRB, "                   3: USB ctl transfers\n");
	DBG(DBG_VRB, "                   4: USB bulk transfers\n");
	DBG(DBG_VRB, "                      Caution: Level 4 may create huge logs (about 40 MB in preview scans)\n");
	DBG(DBG_VRB, "--disablefixedpwm: Disables fixed pulse-width modulation.\n");
	DBG(DBG_VRB, "--preview        : performs a preview scan.\n");
	DBG(DBG_VRB, "--nowshd         : Disables white shading correction.\n");
	DBG(DBG_VRB, "--nobshd         : Disables black shading correction.\n");
	DBG(DBG_VRB, "--noshd          : Disables black and white shading correction.\n");

	if (RTS_device_count() > 0)
	{
		int a;

		DBG(DBG_VRB, "--model <num>    : Lets to test device behaviour with other supported models.\n");
		DBG(DBG_VRB, "                   Supported models are:\n");

		for (a = 0; a < RTS_device_count(); a++)
		{
			char *sname = RTS_device_name(a);

			if (sname != NULL)
			{
				char *vendor = RTS_device_vendor(a);

				if (vendor != NULL)
				{
					char *product = RTS_device_product(a);

					if (product != NULL)
					{
						DBG(DBG_VRB, "                   %2i: %8s = %s %s\n", a, sname, vendor, product);
						free (product);
					}

					free (vendor);
				}

				free (sname);
			}
		}
	}

	DBG(DBG_VRB, "--odtflb <secs>  : Sets Overdrive time in seconds for reflective lamp. Default 10\n");
	DBG(DBG_VRB, "--odttma  <secs> : Sets Overdrive time in seconds for tma lamp. Default 10\n");
	DBG(DBG_VRB, "--real-depth     : Sends the given depth to the chip instead of 16 bits.\n");
	DBG(DBG_VRB, "--dbgimage       : Saves all images taken in calibration process.\n");
	DBG(DBG_VRB, "--scanwhiteboard : Scans the black and white pattern located beyond the top of image.\n");
	DBG(DBG_VRB, "--usbtype <num>  : Lets to test device behaviour in other usb versions.\n");
	DBG(DBG_VRB, "                   Supported types are:\n");
	DBG(DBG_VRB, "                   0: USB1.1\n");
	DBG(DBG_VRB, "                   1: USB2.0\n");

	DBG(DBG_VRB, "\nUse these functions carefully !!\n");
	DBG(DBG_VRB, "\n");

	exit(SANE_STATUS_GOOD);
}

/** SEC: USB related functions */

static void device_usb_list()
{
	struct usb_bus    *bus;
	struct usb_device *dev;

	SANE_Int count = 0;

	if (DBG_LEVEL < 1) DBG_LEVEL = 1;

	usb_find_busses();
	usb_find_devices();

	/* Searching devices */
	for (bus = usb_get_busses(); bus ; bus = bus->next)
		for (dev = bus->devices; dev; dev = dev->next)
		{
			SANE_Int device  = RTS_device_get(dev->descriptor.idProduct, dev->descriptor.idVendor);

			if (device != -1)
			{
				char *vendor  = RTS_device_vendor (device);
				char *product = RTS_device_product(device);

				count++;

				DBG(DBG_VRB, "- Bus %s Dev %s: ID %04x:%04x %s %s\n",
					bus->dirname,
					dev->filename,
					dev->descriptor.idVendor,
					dev->descriptor.idProduct,
					(vendor  == NULL)? "": vendor,
					(product == NULL)? "": product);

				if (vendor != NULL)
					free (vendor);

				if (product != NULL)
					free (product);
			}
		}

	DBG(DBG_VRB, "Found %i devices\n", count);

	if (count == 0)
	{
		DBG(DBG_VRB, "This doesn't mean that your scanner doesn't work with this controller.\n");
		DBG(DBG_VRB, "If your scanner has a compatible chipset, you could try to emulate any other\n");
		DBG(DBG_VRB, "supported scanner using parameter --model (see hp3900 --help-debug) and setting\n");
		DBG(DBG_VRB, "with --bus and --dev, your scanner's bus and device taken from lsusb command.\n");
	}
	
	exit(SANE_STATUS_GOOD);
}

static struct usb_device *device_usb_get_first()
{
	struct usb_bus    *bus;
	struct usb_device *dev;

	usb_find_busses();
	usb_find_devices();

	DBG(DBG_VRB, "- Looking for some available device...\n");
	
	/* Searching devices */
	for (bus = usb_get_busses(); bus ; bus = bus->next)
		for (dev = bus->devices; dev; dev = dev->next)
			if (RTS_device_get(dev->descriptor.idProduct, dev->descriptor.idVendor) != -1)
				return dev;

	/* No devices found*/
	return NULL;
}

static struct usb_device *device_usb_get(SANE_Int ibus, SANE_Int idev)
{
	struct usb_bus    *bus;
	struct usb_device *dev;

	usb_find_busses();
	usb_find_devices();

	/* Searching devices */
	for (bus = usb_get_busses(); bus ; bus = bus->next)
		for (dev = bus->devices; dev; dev = dev->next)
			if ((atoi(bus->dirname) == ibus)&&(atoi(dev->filename) == idev))
				return dev;

	/* No devices found*/
	return NULL;
}

/** SEC: TIFF image format
 * This image format will be used to store received data from scanner
 */

static TIFF *TIFF_Open(char *sFile, SANE_Int width, SANE_Int height, SANE_Int depth, SANE_Int colortype, SANE_Int resolution_x, SANE_Int resolution_y, SANE_Int channels)
{
	/* Define an image */
	TIFF *image;

	DBG(DBG_FNC, "> TIFF_Open(file, width=%i, height=%i, depth=%i, colortype=%i, res_x=%i, res_y=%i, channels=%i)\n",
	    width, height, depth, colortype, resolution_x, resolution_y, channels);

	/* Open the TIFF file */
	if ((image = TIFFOpen(sFile, "w")) != NULL)
	{
		SANE_Int ct = (colortype == CM_COLOR) ? PHOTOMETRIC_RGB: PHOTOMETRIC_MINISBLACK;

		/* We need to set some values for basic tags before we can add any data */
		TIFFSetField(image, TIFFTAG_IMAGEWIDTH, width);
		TIFFSetField(image, TIFFTAG_IMAGELENGTH, height);
		TIFFSetField(image, TIFFTAG_BITSPERSAMPLE, depth);
		TIFFSetField(image, TIFFTAG_SAMPLESPERPIXEL, channels);

		TIFFSetField(image, TIFFTAG_PHOTOMETRIC, ct);
		TIFFSetField(image, TIFFTAG_FILLORDER, FILLORDER_MSB2LSB);
		TIFFSetField(image, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

		TIFFSetField(image, TIFFTAG_XRESOLUTION, (double)resolution_x);
		TIFFSetField(image, TIFFTAG_YRESOLUTION, (double)resolution_y);
		TIFFSetField(image, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);
	}

	return image;
}

static SANE_Status TIFF_Add(TIFF *image, SANE_Byte *buffer, SANE_Int size)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	if ((image != NULL) && (buffer != NULL))
	{
		/* write the information to the file */
		TIFFWriteRawStrip(image, 0, buffer, size);
		rst = SANE_STATUS_GOOD;
	}

	return rst;
}

static void TIFF_Fill(TIFF *image, SANE_Int size)
{
	if ((size > 0)&&(image != NULL))
	{
		SANE_Byte *buffer = (SANE_Byte *) malloc(size * sizeof(SANE_Byte));

		if (buffer != NULL)
		{
			bzero(buffer, size * sizeof(SANE_Byte));
			TIFFWriteRawStrip(image, 0, buffer, size);
			free(buffer);
		}
	}
}

static void TIFF_Close(TIFF *image)
{
  /* Close the file */
	if (image != NULL)
	{
		TIFFClose(image);
		image = NULL;
	}
}

/** SEC: Gamma */

static SANE_Status Gamma_CreateTable(SANE_Byte *table, double gamma, SANE_Int depth)
{
	/* For better performance we create here neccesary gamma values
	   instead of calculating them for each given pixel

	   Thanks to Michael Mather for his code to generate gamma values in 16 bit depth
	*/

	SANE_Int a, b;
	double value, c;
	SANE_Int count, chn_size;

	/*#define QandD 1*/
	#if QandD
		SANE_Int black_level = 0x20;
	#endif

	if (table == NULL)
		return SANE_STATUS_INVAL;

	Gamma_CheckConstrains(&gamma);

	count = (1 << depth);
	chn_size = (depth + 7) / 8;
	c = 0;
	for (a = 0; a < count; a++)
	{
		#if QandD
			/* Quick and dirty way to adjust black levels. */
			value = (c - black_level) / (count - 1 - black_level);
			if (value < 0)
				value = 0;
		#else
			value = (c / (count - 1));
		#endif

		c++;
		value = pow(value, (1 / gamma));
		value = value * (count - 1 );

		b = (SANE_Int) value;
		if (b > (count - 1))
			b = (count - 1);
				else if (b < 0)
					b = 0;

		data_lsb_set(table + (a * chn_size), b, chn_size);
	}

	return SANE_STATUS_GOOD;
}

static void Gamma_Init()
{
	SANE_Int a;
	SANE_Int size = (((1 << GAMMA_DEPTH) * GAMMA_DEPTH) + 7) / 8;

	/* Delete previous gamma tables if exist */
	Gamma_Free();

	/* Create gamma tables */
	for (a = 0; a < 3; a++)
	{
		myGamma.table[a] = (SANE_Byte *) malloc(size * sizeof(SANE_Byte));
		if (myGamma.table[a] != NULL)
		{
			if (myGamma.correction[a] <= 0)
				myGamma.correction[a] = GAMMA_DEFAULT;

			Gamma_CreateTable(myGamma.table[a], myGamma.correction[a], GAMMA_DEPTH);

			/*if (device->options->calibrate != SANE_FALSE)*/
			RTS_gamma_alloc(device, a, GAMMA_DEPTH, myGamma.table[a]);
		}
	}
}

static void Gamma_CheckConstrains(double *gamma)
{
	if (*gamma < 0.10)
		*gamma = 0.10;
			else if (*gamma > 9.00)
				*gamma = 9.00;
}

static void Gamma_Free()
{
	SANE_Int a;

	/* Destroy gamma tables */
	for (a = 0; a < 3; a++)
	{
		if (myGamma.table[a] != NULL)
		{
			free(myGamma.table[a]);
			myGamma.table[a] = NULL;
		}
	}
}


/** SEC: Converters
 * This section tries to provide image convertion capabilities until
 * driver is debugged at all
 */

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

static void Color_to_Gray(SANE_Byte *buffer, SANE_Int size, SANE_Int depth)
{
	/* converts 3 color channel into 1 gray channel of specified bit depth */

	if (buffer != NULL)
	{
		SANE_Int c, chn, chn_size;
		SANE_Byte *ptr_src = NULL;
		SANE_Byte *ptr_dst = NULL;
		float data, chn_data;
		float coef[3] = {0.299, 0.587, 0.114}; /* coefficients per channel /1000 */

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

			ptr_src += 3 * chn_size;
			ptr_dst += chn_size;
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
				if ( (fromBufferPos_i < size) && (buffer[fromBufferPos_i] > threshold) )
					toBufferByte |= (1u << bitPos_i);

				fromBufferPos_i++;
			}

			buffer[toBufferPos_i] = toBufferByte;
			toBufferPos_i++;
		}
	}
}

static void Color_Negative(SANE_Byte *buffer, SANE_Int size, SANE_Int depth)
{
	if (buffer != NULL)
	{
		SANE_Int a, value;
		SANE_Int max_value = (1 << depth) - 1;
		SANE_Int chn_size = (depth + 7) / 8;
		SANE_Byte *ptr;

		ptr = buffer;

		for (a = 0; a < size / chn_size; a++)
		{
			value = data_lsb_get(ptr, chn_size);
			data_lsb_set(ptr, max_value - value, chn_size);

			ptr += chn_size;
		}
	}
}

static void show_RTS_chip_name(SANE_Int do_exit)
{
	char *name;
	SANE_Int dl = DBG_LEVEL;

	/* Change debug level to show app header */
	if (DBG_LEVEL < 1)
		DBG_LEVEL = 1;

	name = (char *)malloc(255 * sizeof(char));

	if (name != NULL)
	{
		if (RTS_chip_name(device, name, 255) != SANE_STATUS_INVAL)
			DBG(DBG_VRB, "%i : %s\n", RTS_chip_id(device), name);
				else DBG(DBG_VRB, "-1 : unknown\n");
		free(name);
	} else DBG(DBG_VRB, "-1 : unknown\n");

	/* Restore debug level */
	DBG_LEVEL = dl;

	if (do_exit != 0)
		exit(0);
}

static void Silent_Compile()
{
	/*
		There are some functions in hp3900_rts8822.c that aren't used yet.
		To avoid compilation warnings we will use them here
	*/

	SANE_Byte a = 1;

	if (a == 0)
	{
		RTS_btn_order(NULL, 0);
		RTS_btn_name(NULL, 0);
		Gamma_GetTables(NULL, NULL);
		RTS_gamma_depth_set (NULL, 0, 0);
		RTS_device_supported(-1);
		RTS_device_usb_vendor(-1);
		RTS_device_usb_product(-1);
	}
}

static SANE_Status Get_Image(struct params *data)
{
	SANE_Byte *Line;

	DBG(DBG_VRB, "- Getting image...\n");

	Line = (SANE_Byte *) malloc(bytesperline * sizeof(SANE_Byte));
	if (Line != NULL)
	{
		SANE_Int transferred, counter;
		SANE_Int Bytes_per_channel = (data->depth > 8) ? 2: 1;
		SANE_Int Channels_per_dot = (data->colormode != CM_COLOR) ? 1: 3;
		TIFF *image;

		/* create image file */
		image = TIFF_Open(File_Path,
		                  data->coords.width,
		                  data->coords.height,
		                  (cnv.depth != -1)? cnv.depth : data->depth,
		                  (cnv.colormode != -1)? cnv.colormode: data->colormode,
		                  data->resolution_x,
		                  data->resolution_y,
		                  (cnv.colormode == CM_GRAY)? 1: Channels_per_dot);

		counter = 0;
		transferred = 0;
		do
		{
			RTS_scanner_read(device, bytesperline, Line, &transferred);

			if (transferred > 0)
			{
				counter += data->coords.width;

				/*We only add desired line size */
				if (data->colormode == CM_LINEART)
				{
					TIFF_Add(image, Line, min(transferred, min(transferred, data->coords.width / 8)));
				} else
				{
					SANE_Int size = min(transferred, data->coords.width * (Bytes_per_channel * Channels_per_dot));

					/* if we are scanning negatives, let's invert colors */
					if (data->scantype == ST_NEG)
					{
						if (cnv.negative == SANE_FALSE)
							Color_Negative(Line, size, data->depth);
					} else if (cnv.negative != SANE_FALSE)
							Color_Negative(Line, size, data->depth);

					/* emulating depth */
					if (cnv.depth != -1)
					{
						switch (cnv.depth)
						{
							/* depth 1 is treated separately */
							/*case 12: in the future*/
							case 8:
								Depth_16_to_8(Line, size, Line);
								size /= 2;
								break;
						}
					}

					/* Lets do neccesary convertions */
					switch(cnv.colormode)
					{
						case CM_GRAY:
							Color_to_Gray(Line, size, (cnv.depth != -1)? cnv.depth: data->depth);
							size /= 3;
							break;
						case CM_LINEART:
							{
								SANE_Int rest = size % 8;
								Gray_to_Lineart(Line, size, cnv.threshold);

								size /= 8;
								if (rest > 0)
									size++;
							}
							break;
					}

					/* Save image */
					TIFF_Add(image, Line, size);
				}
			}
		} while (transferred != 0);

		/* Close image */
		TIFF_Fill(image, (data->coords.width * data->coords.height) - counter);
		TIFF_Close(image);

		free(Line);
	}

	RTS_scanner_stop(device, SANE_TRUE);

	return SANE_STATUS_GOOD;
}

static void Set_Coordinates(SANE_Int scantype, SANE_Int resolution, struct st_coords *coords)
{
	struct st_coords *limits = RTS_area_get(device, scantype);

	if (coords->left == -1)
		coords->left = 0;

	if (coords->width == -1)
		coords->width = limits->width;

	if (coords->top == -1)
		coords->top = 0;

	if (coords->height == -1)
		coords->height = limits->height;

	coords->left   = MM_TO_PIXEL(coords->left, resolution);
	coords->width  = MM_TO_PIXEL(coords->width, resolution);
	coords->top    = MM_TO_PIXEL(coords->top, resolution);
	coords->height = MM_TO_PIXEL(coords->height, resolution);

	RTS_area_check(device, resolution, scantype, coords);
}

static void show_args(SANE_Int argc, char *argv[])
{
	/* This function shows application params */

	SANE_Int a;

	DBG(DBG_FNC, "hp3900 v%s :", BACKEND_VRSN);

	for (a = 0; a < argc; a++)
	{
		DBG(DBG_FNC, "%s ", argv[a]);
	}

	DBG(DBG_FNC, "\n");
}

/* next is a simple hack to avoid blocking getc */
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>


static SANE_Int kbhit(void)
{
  int			cnt = 0;
  int			error;
  static struct termios	Otty, Ntty;

  tcgetattr(0, &Otty);
  Ntty = Otty;

  Ntty.c_iflag 		= 0;	   /* input mode	*/
  Ntty.c_oflag 		= 0;	   /* output mode	*/
  Ntty.c_lflag 	       &= ~ICANON; /* raw mode 		*/
  Ntty.c_cc[VMIN] 	= 1;	   /* minimum chars to wait for */
  Ntty.c_cc[VTIME] 	= 1;   /* minimum wait time	*/

  if (0 == (error = tcsetattr(0, TCSANOW, &Ntty))) {
    struct timeval	tv;
    error     += ioctl(0, FIONREAD, &cnt);
    error     += tcsetattr(0, TCSANOW, &Otty);
    tv.tv_sec  = 0;
    tv.tv_usec = 100;   /* insert a minimal delay */
    select(1, NULL, NULL, NULL, &tv);
  }

  return (error == 0 ? cnt : -1 );
}
/* ******************************* */

static void query_buttons()
{
	if (RTS_btn_count(device) > 0)
	{
		SANE_Int c;
		SANE_Int released_total;
		SANE_Int status_last = -1, released_last = -1;
		SANE_Int status_current, released_current;

		RTS_chip_reset (device);

		c = 0;
		released_total = 0;

		/* Infinite loop */
		while (0 == 0)
		{
			/* read status from scanner */
			status_current = RTS_btn_status(device);
			released_current = RTS_btn_released(device);

			if ((status_current != status_last)||((released_current != released_last)))
			{
				/* update status, first register */
				status_last = status_current;
				released_last = released_current;

				released_total = released_total | released_current;

				/* check for all buttons depressed */
				if ((status_current == 0x3f)&&(released_current==0))
				{
					c++;

					/* check to see if we've been here before, i.e. exit on second button release */
					if (c > 1)
					{
						printf("%d\n", released_total);
						break;
					}
				}
			}
		}
	} else DBG (DBG_ERR, "- This device doesn't support any button\n");
}

static void show_buttons()
{
	if (RTS_btn_count(device) > 0)
	{
		SANE_Int a, b;
		SANE_Int status_last = -1, released_last = -1;
		SANE_Int status_current, released_current;

		/* to get buttons count we need config to be initialized */
		DBG(DBG_ERR, "- This device supports %i buttons\n", RTS_btn_count(device));

		RTS_chip_reset(device);

		b = 0;

		/* Infinite loop */
		while(kbhit() == 0)
		{
			/* remember commands every 20 lines */
			if (b == 0)
			{
				DBG(DBG_ERR, "- Press device's buttons to test them or ANY KEY to exit.\n");
				b = 20;
			}

			/* read status from scanner */
			status_current   = RTS_btn_status(device);
			released_current = RTS_btn_released(device);

			if ((status_current != status_last)||((released_current != released_last)))
			{
				/* update status, first register */
				status_last = status_current;
				released_last = released_current;
				DBG(DBG_ERR, "Reg[0xe968]: 0x%02x: ", status_current);

				for (a = 0; a < 8; a++)
					DBG(DBG_ERR, "%i ", ((status_current & (0x80 >> a)) != 0)? 1: 0);

				/* second register */
				DBG(DBG_ERR, "; Reg[0xe96a]: 0x%02x: ", released_current);

				for (a = 0; a < 8; a++)
					DBG(DBG_ERR, "%i ", ((released_current & (0x80 >> a)) != 0)? 1: 0);

				DBG(DBG_ERR, "\n");
				b--;
			}
		}
	} else DBG(DBG_ERR, "- This device doesn't support any button\n");
}

static void show_scancount(void)
{
	/* shows number of scans made by scanner */
	SANE_Int sc = RTS_scan_count_get(device);

	DBG(DBG_VRB, "Number of scans made by this scanner: ");
	DBG(DBG_ERR, "%i\n", sc);
}

static SANE_Status make_scan(struct params *data)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	/* set coordinates*/
	Set_Coordinates(data->scantype, data->resolution_x, &data->coords);

	/* send scanning parameters to device */
	DBG(DBG_VRB, "- Setting parameters...\n");

	if (RTS_scanner_setup(device, data) == SANE_STATUS_GOOD)
	{
		/* start scanning process */
		DBG(DBG_VRB, "- Starting scanning process...\n");

		/* stop previus scans */
		RTS_scanner_stop(device, SANE_TRUE);

		/* the big party starts now!!! */
		if (RTS_scanner_start(device) == SANE_STATUS_GOOD)
		{
			/* ok, retrieve image from scanner : ) */
			rst = SANE_STATUS_GOOD;
			Get_Image(data);
		} else DBG(DBG_ERR, "ERROR: Could not start scanning correctly\n");
	} else DBG(DBG_ERR, "ERROR: Parameters could not be set correctly\n");

	return rst;
}

static SANE_Status proof(st_device *dev)
{
	SANE_Byte *b = calloc(1, 1000);
	int a;

	if (RTS_dma_read(dev, /*dmacs*/ 0, /*segment*/ 0, /*size*/128, b) == SANE_STATUS_GOOD)
		RTS_usb_buffer_show(-1, b, 128);

	for (a = 0; a < 128; a++)
	{
		b[a] = a;
	}

	if (RTS_dma_write(dev, /*dmacs*/0x14, /*segment*/ 0, /*size*/128, b) == SANE_STATUS_GOOD)
	{
		printf("\n");
		if (RTS_dma_read(dev, /*dmacs*/ 0x14, /*segment*/ 0, /*size*/128, b) == SANE_STATUS_GOOD)
			RTS_usb_buffer_show(-1, b, 128);
	}
	return SANE_STATUS_GOOD;
}
