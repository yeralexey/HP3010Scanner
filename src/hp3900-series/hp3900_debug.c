/* HP Scanjet 3900 series - Debugging functions for standalone

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

#ifndef HP3900DBG
#define HP3900DBG

/* debugging level messages */
#define DBG_ERR             0x00 /* Only important errors         */
#define DBG_VRB             0x01 /* verbose messages              */
#define DBG_FNC             0x02 /* Function names and parameters */
#define DBG_CTL             0x03 /* USB Ctl data                  */
#define DBG_BLK             0x04 /* USB Bulk data                 */

#include <stdarg.h>
#ifdef HAVE_TIFFIO_H
#include <tiffio.h> /* dbg_tiff_save */
#endif

/* headers */

static char *dbg_scantype    (SANE_Int type);
static void  dbg_scanmodes   (st_device *dev);
static void  dbg_checkshading(struct st_check_shading *shading);
static void  dbg_motorcurves (st_device *dev);
static void  dbg_motormoves  (st_device *dev);
static void  dbg_hwdcfg      (struct st_hwdconfig *params);
static void  dbg_ScanParams  (struct st_scanparams *params);
static void  dbg_calibtable  (struct st_gain_offset *params);
static char *dbg_colour      (SANE_Int colour);
static void  dbg_motorcfg    (struct st_motorcfg *motorcfg);
static void  dbg_buttons     (struct st_buttons *buttons);
static void  dbg_sensor      (struct st_sensorcfg *sensor);
static void  dbg_timing      (struct st_timing *mt);
static void  dbg_sensorclock (struct st_cph *cph);
static void  dbg_tiff_save   (char *sFile, SANE_Int width, SANE_Int height, SANE_Int depth, SANE_Int colortype, SANE_Int res_x, SANE_Int res_y, SANE_Byte *buffer, SANE_Int size);
static void  dbg_autoref     (struct st_scanparams *scancfg, SANE_Byte *pattern, SANE_Int ser1, SANE_Int ser2, SANE_Int ler);

#ifdef STANDALONE

/* implementation */

int DBG_LEVEL = 0;

static void DBG(int level, const char *msg, ...)
{
	va_list ap;
	va_start (ap, msg);

	if (level <= DBG_LEVEL)
		vfprintf(stderr, msg, ap);

	va_end (ap);
}

#endif

/* debugging functions */
static char *dbg_scantype(SANE_Int type)
{
	switch(type)
	{
		case ST_NORMAL:
			return "ST_NORMAL";
			break;
		case ST_TA:
			return "ST_TA";
			break;
		case ST_NEG:
			return "ST_NEG";
			break;
		default:
			return "Unknown";
			break;
	}
}

static void dbg_sensorclock(struct st_cph *cph)
{
	if (cph != NULL)
	{
		DBG(DBG_FNC, " -> cph->p1 = %f\n", cph->p1);
		DBG(DBG_FNC, " -> cph->p2 = %f\n", cph->p2);
		DBG(DBG_FNC, " -> cph->ps = %i\n", cph->ps);
		DBG(DBG_FNC, " -> cph->ge = %i\n", cph->ge);
		DBG(DBG_FNC, " -> cph->go = %i\n", cph->go);
	} else DBG(DBG_FNC, " -> cph is NULL\n");
}

static void dbg_timing(struct st_timing *mt)
{
	if (mt != NULL)
	{
		DBG(DBG_FNC, " -> mt->cdss[0]   = %i\n", _B0(mt->cdss[0]));
		DBG(DBG_FNC, " -> mt->cdsc[0]   = %i\n", _B0(mt->cdsc[0]));
		DBG(DBG_FNC, " -> mt->cdss[1]   = %i\n", _B0(mt->cdss[1]));
		DBG(DBG_FNC, " -> mt->cdsc[1]   = %i\n", _B0(mt->cdsc[1]));
		DBG(DBG_FNC, " -> mt->cnpp      = %i\n", _B0(mt->cnpp));
		DBG(DBG_FNC, " -> mt->cvtrp0    = %i\n", _B0(mt->cvtrp[0]));
		DBG(DBG_FNC, " -> mt->cvtrp1    = %i\n", _B0(mt->cvtrp[1]));
		DBG(DBG_FNC, " -> mt->cvtrp2    = %i\n", _B0(mt->cvtrp[2]));
		DBG(DBG_FNC, " -> mt->cvtrfpw   = %i\n", _B0(mt->cvtrfpw));
		DBG(DBG_FNC, " -> mt->cvtrbpw   = %i\n", _B0(mt->cvtrbpw));
		DBG(DBG_FNC, " -> mt->cvtrw     = %i\n", _B0(mt->cvtrw));
		DBG(DBG_FNC, " -> mt->clamps    = 0x%08x\n", mt->clamps);
		DBG(DBG_FNC, " -> mt->clampe    = 0x%08x\n", mt->clampe);
		DBG(DBG_FNC, " -> mt->adcclkp0  = %f\n", mt->adcclkp[0]);
		DBG(DBG_FNC, " -> mt->adcclkp1  = %f\n", mt->adcclkp[1]);
		DBG(DBG_FNC, " -> mt->adcclkp2e = %i\n", mt->adcclkp2e);
		DBG(DBG_FNC, " -> mt->cphbp2s   = %i\n", mt->cphbp2s);
		DBG(DBG_FNC, " -> mt->cphbp2e   = %i\n", mt->cphbp2e);
	} else DBG(DBG_FNC, " -> mt is NULL\n");
}

static void dbg_checkshading(struct st_check_shading *shading)
{
	if (shading != NULL)
	{
		DBG(DBG_FNC, " -> shading->table_count = %i\n", shading->table_count);
		DBG(DBG_FNC, " -> shading->edb0        = %i\n", shading->edb0);
		DBG(DBG_FNC, " -> shading->ee04        = %i\n", shading->ee04);
		DBG(DBG_FNC, " -> shading->ee0c        = %i\n", shading->ee0c);
		DBG(DBG_FNC, " -> shading->ee14        = %i\n", shading->ee14);
		DBG(DBG_FNC, " -> shading->ee08        = %i\n", shading->ee08);
		DBG(DBG_FNC, " -> shading->ee01        = %i\n", shading->ee01);
		DBG(DBG_FNC, " -> shading->ee10        = %i\n", shading->ee10);
		DBG(DBG_FNC, " -> Table sizes: %i, %i, %i, %i\n", shading->sh_table_size[0], shading->sh_table_size[1], shading->sh_table_size[2], shading->sh_table_size[3]);
	} else DBG(DBG_FNC, " -> shading is NULL\n");
}

static void dbg_sensor(struct st_sensorcfg *sensor)
{
	if (sensor != NULL)
	{
		DBG(DBG_FNC, " -> type, name, res , {chn_color }, {chn_gray}, {rgb_order }, line_dist, evnodd_dist\n");
		DBG(DBG_FNC, " -> ----, ----, --- , {--, --, --}, {--, --  }, {--, --, --}, ---------, -----------\n");
		DBG(DBG_FNC, " -> %4i, %4i, %4i, {%2i, %2i, %2i}, {%2i, %2i  }, {%2i, %2i, %2i}, %9i, %11i\n",
		    sensor->type, sensor->name, sensor->resolution, sensor->channel_color[0],
		    sensor->channel_color[1], sensor->channel_color[2], sensor->channel_gray[0],
		    sensor->channel_gray[1], sensor->rgb_order[0], sensor->rgb_order[1],
		    sensor->rgb_order[2], sensor->line_distance, sensor->evenodd_distance);
	} else DBG(DBG_FNC, " -> sensor is NULL\n");
}

static void dbg_buttons(struct st_buttons *buttons)
{
	if (buttons != NULL)
	{
		DBG(DBG_FNC, " -> count, btn1, btn2, btn3, btn4, btn5, btn6\n");
		DBG(DBG_FNC, " -> -----, ----, ----, ----, ----, ----, ----\n");
		DBG(DBG_FNC, " -> %5i, %4i, %4i, %4i, %4i, %4i, %4i\n",
		    buttons->count, buttons->mask[0][0], buttons->mask[1][0], buttons->mask[2][0],
		    buttons->mask[3][0], buttons->mask[4][0], buttons->mask[5][0]);
	} else DBG(DBG_FNC, " -> buttons is NULL\n");
}

static void dbg_scanmodes(st_device *dev)
{
	if (dev->scanmodes_count > 0)
	{
		SANE_Int a;
		struct st_scanmode *reg;

		DBG(DBG_FNC, " -> ##, ST       , CM        , RES , TM, CV, SR, CLK, CTPC  , BKS , STT, DML, {   Exposure times     }, { Max exposure times   }, MP , MExp16, MExpF, MExp, MRI, MSI, MMTIR, MMTIRH, SK\n");
		DBG(DBG_FNC, " -> --, ---------, ----------, --- , --, --, --, ---, ------, ----, ---, ---, {------  ------  ------}, {------  ------  ------}, ---, ------, -----, ----, ---, ---, -----, ------, --\n");
		for (a = 0; a < dev->scanmodes_count; a++)
		{
			reg = dev->scanmodes[a];
			if (reg != NULL)
			{
				DBG(DBG_FNC, " -> %2i, %9s, %10s, %4i, %2i, %2i, %2i, %3i, %6i, %4i, %3i, %3i, {%6i, %6i, %6i}, {%6i, %6i, %6i}, %3i, %6i, %5i, %4i, %3i, %3i, %5i, %6i, %2i\n",
				    a, dbg_scantype(reg->scantype), dbg_colour(reg->colormode), reg->resolution, reg->timing,
				    reg->motorcurve, reg->samplerate, reg->systemclock, reg->ctpc,
				    reg->motorbackstep, reg->scanmotorsteptype, reg->dummyline,
				    reg->expt[0], reg->expt[1], reg->expt[2],
				    reg->mexpt[0], reg->mexpt[1], reg->mexpt[2], reg->motorplus,
				    reg->multiexposurefor16bitmode, reg->multiexposureforfullspeed, reg->multiexposure,
				    reg->mri, reg->msi, reg->mmtir, reg->mmtirh, reg->skiplinecount);
			}
		}
	}
}

static void dbg_motorcurves(st_device *dev)
{
	if (dev->mtrsetting != NULL)
	{
		struct st_motorcurve *mtc;
		SANE_Int a = 0;

		while (a < dev->mtrsetting_count)
		{
			DBG(DBG_FNC, " -> Motorcurve %2i: ", a);
			mtc = dev->mtrsetting[a];
			if (mtc != NULL)
			{
				DBG(DBG_FNC, "mri=%i msi=%i skip=%i bckstp=%i\n", mtc->mri, mtc->msi, mtc->skiplinecount, mtc->motorbackstep);
				if (mtc->curve_count > 0)
				{
					char *sdata = (char *)malloc(256);
					if (sdata != NULL)
					{
						char *sline = (char *)malloc(256);
						if (sline != NULL)
						{
							SANE_Int count;
							struct st_curve *crv;

							DBG(DBG_FNC, " ->  ##, dir, type      , count, from, to  , steps\n");
							DBG(DBG_FNC, " ->  --, ---, ----------, -----, ----, ----, -----\n");

							count = 0;
							while (count < mtc->curve_count)
							{
								memset(sline, 0, 256);

								snprintf(sdata, 256, " ->  %02i, ", count);
								strcat(sline, sdata);

								crv = mtc->curve[count];
								if (crv != NULL)
								{
									if (crv->crv_speed == ACC_CURVE)
										strcat(sline, "ACC, ");
											else strcat(sline, "DEC, ");

									switch(crv->crv_type)
									{
										case CRV_NORMALSCAN: strcat(sline, "NORMALSCAN, "); break;
										case CRV_PARKHOME  : strcat(sline, "PARKHOME  , "); break;
										case CRV_SMEARING  : strcat(sline, "SMEARING  , "); break;
										case CRV_BUFFERFULL: strcat(sline, "BUFFERFULL, "); break;
										default:
											snprintf(sdata, 256, "unknown %2i, ", crv->crv_type);
											strcat(sline, sdata);
											break;
									}

									snprintf(sdata, 256, "%5i, ", crv->step_count);
									strcat(sline, sdata);
									if (crv->step_count > 0)
									{
										SANE_Int stpcount = 0;

										snprintf(sdata, 256, "%4i, %4i| ", crv->step[0], crv->step[crv->step_count -1]);
										strcat(sline, sdata);

										while (stpcount < crv->step_count)
										{
											if (stpcount == 10)
											{
												strcat(sline, "...");
												break;
											}
											if (stpcount > 0)
												strcat(sline, ", ");

											snprintf(sdata, 256, "%4i", crv->step[stpcount]);
											strcat(sline, sdata);

											stpcount++;
										}
										strcat(sline, "\n");
									} else strcat(sline, "NONE\n");
								} else strcat(sline, "NULL ...\n");

								DBG(DBG_FNC, "%s", sline);

								count++;
							}

							free(sline);
						}
						free(sdata);
					}
				}
			} else DBG(DBG_FNC, "NULL\n");
			a++;
		}
	}
}

static void dbg_motormoves(st_device *dev)
{
	if (dev->motormove_count > 0)
	{
		SANE_Int a;
		struct st_motormove *reg;

		DBG(DBG_FNC, " -> ##, CLK, CTPC, STT, CV\n");
		DBG(DBG_FNC, " -> --, ---, ----, ---, --\n");
		for (a = 0; a < dev->motormove_count; a++)
		{
			reg = dev->motormove[a];
			if (reg != NULL)
			{
				DBG(DBG_FNC, " -> %2i, %3i, %4i, %3i, %2i\n",
				    a, reg->systemclock, reg->ctpc,
				    reg->scanmotorsteptype, reg->motorcurve);
			}
		}
	}
}

static void dbg_hwdcfg(struct st_hwdconfig *params)
{
	if (params != NULL)
	{
		DBG(DBG_FNC, " -> Low level config:\n");
		DBG(DBG_FNC, " -> startpos              = %i\n", params->startpos);
		DBG(DBG_FNC, " -> arrangeline           = %s\n", (params->arrangeline == FIX_BY_SOFT)? "FIX_BY_SOFT": (params->arrangeline == FIX_BY_HARD)? "FIX_BY_HARD" : "FIX_BY_NONE");
		DBG(DBG_FNC, " -> scantype              = %s\n", dbg_scantype(params->scantype));
		DBG(DBG_FNC, " -> compression           = %i\n", params->compression);
		DBG(DBG_FNC, " -> use_gamma             = %i\n", params->use_gamma);
		DBG(DBG_FNC, " -> gamma_depth           = %i\n", params->gamma_depth);
		DBG(DBG_FNC, " -> white_shading         = %i\n", params->white_shading);
		DBG(DBG_FNC, " -> black_shading         = %i\n", params->black_shading);
		DBG(DBG_FNC, " -> unk3                  = %i\n", params->unk3);
		DBG(DBG_FNC, " -> motorplus             = %i\n", params->motorplus);
		DBG(DBG_FNC, " -> static_head           = %i\n", params->static_head);
		DBG(DBG_FNC, " -> motor_direction       = %s\n", (params->motor_direction == MTR_FORWARD)? "FORWARD": "BACKWARD");
		DBG(DBG_FNC, " -> dummy_scan            = %i\n", params->dummy_scan);
		DBG(DBG_FNC, " -> highresolution        = %i\n", params->highresolution);
		DBG(DBG_FNC, " -> sensorevenodddistance = %i\n", params->sensorevenodddistance);
		DBG(DBG_FNC, " -> calibrate             = %i\n", params->calibrate);
	}
}

static void dbg_ScanParams(struct st_scanparams *params)
{
	if (params != NULL)
	{
		DBG(DBG_FNC, " -> Scan params:\n");
		DBG(DBG_FNC, " -> colormode        = %s\n", dbg_colour(params->colormode));
		DBG(DBG_FNC, " -> depth            = %i\n", params->depth);
		DBG(DBG_FNC, " -> samplerate       = %i\n", params->samplerate);
		DBG(DBG_FNC, " -> timing           = %i\n", params->timing);
		DBG(DBG_FNC, " -> channel          = %i\n", params->channel);
		DBG(DBG_FNC, " -> sensorresolution = %i\n", params->sensorresolution);
		DBG(DBG_FNC, " -> resolution_x     = %i\n", params->resolution_x);
		DBG(DBG_FNC, " -> resolution_y     = %i\n", params->resolution_y);
		DBG(DBG_FNC, " -> left             = %i\n", params->coord.left);
		DBG(DBG_FNC, " -> width            = %i\n", params->coord.width);
		DBG(DBG_FNC, " -> top              = %i\n", params->coord.top);
		DBG(DBG_FNC, " -> height           = %i\n", params->coord.height);
		DBG(DBG_FNC, " -> shadinglength    = %i\n", params->shadinglength);
		DBG(DBG_FNC, " -> v157c            = %i\n", params->v157c);
		DBG(DBG_FNC, " -> bytesperline     = %i\n", params->bytesperline);
		DBG(DBG_FNC, " -> expt             = %i\n", params->expt);
		DBG(DBG_FNC, " *> origin_x         = %i\n", params->origin_x);
		DBG(DBG_FNC, " *> origin_y         = %i\n", params->origin_y);
		DBG(DBG_FNC, " *> scantype         = %s\n", dbg_scantype(params->scantype));
	}
}

static void dbg_calibtable(struct st_gain_offset *params)
{
	if (params != NULL)
	{
		DBG(DBG_FNC, " -> Calib table:\n");
		DBG(DBG_FNC, " -> type     R     G     B\n");
		DBG(DBG_FNC, " -> -----   ---   ---   ---\n");
		DBG(DBG_FNC, " -> edcg1 = %3i , %3i , %3i\n", params->edcg1[0], params->edcg1[1], params->edcg1[2]);
		DBG(DBG_FNC, " -> edcg2 = %3i , %3i , %3i\n", params->edcg2[0], params->edcg2[1], params->edcg2[2]);
		DBG(DBG_FNC, " -> odcg1 = %3i , %3i , %3i\n", params->odcg1[0], params->odcg1[1], params->odcg1[2]);
		DBG(DBG_FNC, " -> odcg2 = %3i , %3i , %3i\n", params->odcg2[0], params->odcg2[1], params->odcg2[2]);
		DBG(DBG_FNC, " -> pag   = %3i , %3i , %3i\n", params->pag[0], params->pag[1], params->pag[2]);
		DBG(DBG_FNC, " -> vgag1 = %3i , %3i , %3i\n", params->vgag1[0], params->vgag1[1], params->vgag1[2]);
		DBG(DBG_FNC, " -> vgag2 = %3i , %3i , %3i\n", params->vgag2[0], params->vgag2[1], params->vgag2[2]);
	}
}

static char *dbg_colour(SANE_Int colour)
{
	switch(colour)
	{
		case CM_COLOR:
			return "CM_COLOR";
			break;
		case CM_GRAY:
			return "CM_GRAY";
			break;
		case CM_LINEART:
			return "CM_LINEART";
			break;
		default:
			return "Unknown";
			break;
	}
}

static void dbg_motorcfg(struct st_motorcfg *motorcfg)
{
	if (motorcfg != NULL)
	{
		DBG(DBG_FNC, " -> type, res , freq, speed, base, high, park, change\n");
		DBG(DBG_FNC, " -> ----, --- , ----, -----, ----, ----, ----, ------\n");
		DBG(DBG_FNC, " -> %4i, %4i, %4i, %5i, %4i, %4i, %4i, %6i\n",
		    motorcfg->type, motorcfg->resolution, motorcfg->pwmfrequency,
		    motorcfg->basespeedpps, motorcfg->basespeedmotormove, motorcfg->highspeedmotormove,
		    motorcfg->parkhomemotormove, motorcfg->changemotorcurrent);
	}
}

static void dbg_tiff_save(char *sFile, SANE_Int width, SANE_Int height, SANE_Int depth, SANE_Int colortype, SANE_Int res_x, SANE_Int res_y, SANE_Byte *buffer, SANE_Int size)
{
	#ifdef HAVE_TIFFIO_H
	if (buffer != NULL)
	{
		char *path = getenv("HOME");

		if (path != NULL)
		{
			char filename[512];
			TIFF *image;

			if (snprintf(filename, 512, "%s/%s", path, sFile) > 0)
			{
				/* Open the TIFF file */
				if ((image = TIFFOpen(filename, "w")) != NULL)
				{
					char desc[256];

					SANE_Int spp = (colortype == CM_GRAY)? 1: 3;
					SANE_Int ct = (colortype == CM_GRAY) ? PHOTOMETRIC_MINISBLACK: PHOTOMETRIC_RGB;

					snprintf(desc, 256, "Created with hp3900 %s", BACKEND_VRSN);

					/* We need to set some values for basic tags before we can add any data */
					TIFFSetField(image, TIFFTAG_IMAGEWIDTH, width);
					TIFFSetField(image, TIFFTAG_IMAGELENGTH, height);
					TIFFSetField(image, TIFFTAG_BITSPERSAMPLE, depth);
					TIFFSetField(image, TIFFTAG_SAMPLESPERPIXEL, spp);

					TIFFSetField(image, TIFFTAG_PHOTOMETRIC, ct);
					TIFFSetField(image, TIFFTAG_FILLORDER, FILLORDER_MSB2LSB);
					TIFFSetField(image, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

					TIFFSetField(image, TIFFTAG_XRESOLUTION, (double)res_x);
					TIFFSetField(image, TIFFTAG_YRESOLUTION, (double)res_y);
					TIFFSetField(image, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);
					TIFFSetField(image, TIFFTAG_IMAGEDESCRIPTION, desc);

					/* Write the information to the file*/
					TIFFWriteRawStrip(image, 0, buffer, size);
					TIFFClose(image);
				} else DBG(DBG_ERR, "- dbg_tiff_save: Error opening filename: %s\n", filename);
			} else DBG(DBG_ERR, "- dbg_tiff_save: Error generating filename\n");
		} else DBG(DBG_ERR, "- dbg_tiff_save: Enviroment HOME variable does not exist\n");
	}
	#else
	/* silent gcc */
	sFile = sFile;
	width = width;
	height = height;
	depth = depth;
	colortype = colortype;
	res_x = res_x;
	res_y = res_y;
	buffer = buffer;
	size = size;

	DBG(DBG_ERR, "- dbg_tiff_save: tiffio not supported\n");
	#endif
}

static void dbg_autoref(struct st_scanparams *scancfg, SANE_Byte *pattern, SANE_Int ser1, SANE_Int ser2, SANE_Int ler)
{
	/* this function generates post-autoref.tiff */
	SANE_Byte *img = malloc(sizeof(SANE_Byte) * (scancfg->coord.width * scancfg->coord.height * 3));

	if (img != NULL)
	{
		SANE_Int c, value;

		/* generate image from 1 gray channel to 3 color channels */
		for (c = 0; c < (scancfg->coord.width * scancfg->coord.height); c++)
		{
			value = *(pattern + c);
			*(img + (3 * c)) = value;
			*(img + (3 * c) + 1) = value;
			*(img + (3 * c) + 2) = value;
		}

		for (c = 0; c < scancfg->coord.height; c++)
		{
			/* line for first SER */
			if (c < (ler + 5))
			{
				*(img + (scancfg->coord.width * c * 3) + (3 * ser1)) = 0;
				*(img + (scancfg->coord.width * c * 3) + (3 * ser1) + 1) = 255;
				*(img + (scancfg->coord.width * c * 3) + (3 * ser1) + 2) = 0;
			}

			/* line for second SER */
			if (c > (ler - 5))
			{
				*(img + (scancfg->coord.width * c * 3) + (3 * ser2)) = 90;
				*(img + (scancfg->coord.width * c * 3) + (3 * ser2) + 1) = 90;
				*(img + (scancfg->coord.width * c * 3) + (3 * ser2) + 2) = 255;
			}

			/* vertical lines of the pointer */
			if ((c > (ler - 5))&&(c < (ler + 5)))
			{
				if ((ser2 - 5) >= 0)
				{
					*(img + (scancfg->coord.width * c * 3) + (3 * (ser2 - 5))) = 255;
					*(img + (scancfg->coord.width * c * 3) + (3 * (ser2 - 5)) + 1) = 255;
					*(img + (scancfg->coord.width * c * 3) + (3 * (ser2 - 5)) + 2) = 0;
				}

				if ((ser2 + 5) < scancfg->coord.width)
				{
					*(img + (scancfg->coord.width * c * 3) + (3 * (ser2 + 5))) = 255;
					*(img + (scancfg->coord.width * c * 3) + (3 * (ser2 + 5)) + 1) = 255;
					*(img + (scancfg->coord.width * c * 3) + (3 * (ser2 + 5)) + 2) = 0;
				}
			}
		}

		/* line for first LER */
		for (c = 0; c < scancfg->coord.width; c++)
		{
			if ((c > (ser1 - 5))&&(c < (ser2 + 5)))
			{
				if (c != (ser2 - 5))
				{
					*(img + (scancfg->coord.width * ler * 3) + (3 * c)) = 255;
					*(img + (scancfg->coord.width * ler * 3) + (3 * c) + 1) = 90;
					*(img + (scancfg->coord.width * ler * 3) + (3 * c) + 2) = 90;
				}

				/* horizontal lines of the pointer */
				if ((c > (ser2 - 5))&&(c < (ser2 + 5)))
				{
					if ((ler - 5) >= 0)
					{
						*(img + (scancfg->coord.width * (ler - 5) * 3) + (3 * c)) = 255;
						*(img + (scancfg->coord.width * (ler - 5) * 3) + (3 * c) + 1) = 255;
						*(img + (scancfg->coord.width * (ler - 5) * 3) + (3 * c) + 2) = 0;
					}

					if ((ler + 5) < scancfg->coord.height)
					{
						*(img + (scancfg->coord.width * (ler + 5) * 3) + (3 * c)) = 255;
						*(img + (scancfg->coord.width * (ler + 5) * 3) + (3 * c) + 1) = 255;
						*(img + (scancfg->coord.width * (ler + 5) * 3) + (3 * c) + 2) = 0;
					}
				}
			}
		}

		dbg_tiff_save("post-autoref.tiff", scancfg->coord.width, scancfg->coord.height, 8, CM_COLOR, scancfg->resolution_x, scancfg->resolution_y, img, scancfg->coord.height * scancfg->coord.width * 3);

		/* free generated image */
		free(img);
	}
}

#endif
