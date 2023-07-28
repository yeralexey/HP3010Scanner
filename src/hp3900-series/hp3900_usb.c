/* HP Scanjet 3900 series - USB layer

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

#ifndef HP3900USB
#define HP3900USB

/* USB layer commands */
static SANE_Int    RTS_usb_ctl_read     (st_device *, SANE_Int address, SANE_Byte *buffer, SANE_Int size, SANE_Int index);
static SANE_Int    RTS_usb_ctl_write    (st_device *, SANE_Int address, SANE_Byte *buffer, SANE_Int size, SANE_Int index);
static SANE_Int    RTS_usb_blk_read     (st_device *, SANE_Byte *buffer, size_t size);
static SANE_Status RTS_usb_blk_write    (st_device *, SANE_Byte *buffer, SANE_Int size);
static void        RTS_usb_buffer_show  (SANE_Int level, SANE_Byte *buffer, SANE_Int size);

/* Higher level commands*/
static SANE_Status RTS_ctl_iread_byte   (st_device *, SANE_Int address, SANE_Byte *data, SANE_Int index);
static SANE_Status RTS_ctl_iread_word   (st_device *, SANE_Int address, SANE_Int *data, SANE_Int index);
static SANE_Status RTS_ctl_iread_int    (st_device *, SANE_Int address, SANE_Int *data, SANE_Int index);
static SANE_Status RTS_ctl_iread_buffer (st_device *, SANE_Int address, SANE_Byte *buffer, SANE_Int size, SANE_Int index);

static SANE_Status RTS_ctl_iwrite_byte  (st_device *, SANE_Int address, SANE_Byte data, SANE_Int index1, SANE_Int index2);
static SANE_Status RTS_ctl_iwrite_word  (st_device *, SANE_Int address, SANE_Int data, SANE_Int index);
static SANE_Status RTS_ctl_iwrite_int   (st_device *, SANE_Int address, SANE_Int data, SANE_Int index);
static SANE_Status RTS_ctl_iwrite_buffer(st_device *, SANE_Int address, SANE_Byte *buffer, SANE_Int size, SANE_Int index);

static SANE_Status RTS_ctl_read_byte    (st_device *, SANE_Int address, SANE_Byte *data);
static SANE_Status RTS_ctl_read_word    (st_device *, SANE_Int address, SANE_Int *data);
static SANE_Status RTS_ctl_read_int     (st_device *, SANE_Int address, SANE_Int *data);
static SANE_Status RTS_ctl_read_buffer  (st_device *, SANE_Int address, SANE_Byte *buffer, SANE_Int size);

static SANE_Status RTS_ctl_write_byte   (st_device *, SANE_Int address, SANE_Byte data);
static SANE_Status RTS_ctl_write_word   (st_device *, SANE_Int address, SANE_Int data);
/*static SANE_Status RTS_ctl_write_int  (st_device *, SANE_Int address, SANE_Int data);*/
static SANE_Status RTS_ctl_write_buffer (st_device *, SANE_Int address, SANE_Byte *buffer, SANE_Int size);

static SANE_Status RTS_blk_read         (st_device *, SANE_Int buffer_size, SANE_Byte *buffer, SANE_Int *transfered);
static SANE_Status RTS_blk_write        (st_device *, SANE_Int buffer_size, SANE_Byte *buffer, SANE_Int *transfered);


/* Implementation */

static SANE_Status RTS_ctl_iwrite_byte(st_device *dev, SANE_Int address, SANE_Byte data, SANE_Int index1, SANE_Int index2)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	if (dev != NULL)
	{
		SANE_Byte buffer[2] = {0x00, 0x00};
		
		if (RTS_usb_ctl_read(dev, address + 1, buffer, 0x02, index1) == 2)
		{
			buffer[1] = (buffer[0] & 0xff);
			buffer[0] = (data & 0xff);
			
			if (RTS_usb_ctl_write(dev, address, buffer, 0x02, index2) == 2)
				rst = SANE_STATUS_GOOD;
		}
	}

	return rst;
}

static SANE_Status RTS_ctl_iwrite_word(st_device *dev, SANE_Int address, SANE_Int data, SANE_Int index)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	if (dev != NULL)
	{
		SANE_Byte buffer[2];
		
		buffer[0] = (data & 0xff);
		buffer[1] = ((data >> 8) & 0xff);
			
		if (RTS_usb_ctl_write(dev, address, buffer, 0x02, index) == 2)
			rst = SANE_STATUS_GOOD;
	}

	return rst;
}

static SANE_Status RTS_ctl_iwrite_int(st_device *dev, SANE_Int address, SANE_Int data, SANE_Int index)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	if (dev != NULL)
	{
		SANE_Byte buffer[4];
		
		buffer[0] = (data & 0xff);
		buffer[1] = ((data >> 8) & 0xff);
		buffer[2] = ((data >> 16) & 0xff);
		buffer[3] = ((data >> 24) & 0xff);
			
		if (RTS_usb_ctl_write(dev, address, buffer, 0x04, index) == 4)
			rst = SANE_STATUS_GOOD;
	}

	return rst;
}

static SANE_Status RTS_ctl_iwrite_buffer(st_device *dev, SANE_Int address, SANE_Byte *buffer, SANE_Int size, SANE_Int index)
{
	SANE_Status ret = SANE_STATUS_INVAL;

	if (dev != NULL)
	{
		if (RTS_usb_ctl_write(dev, address, buffer, size, index) == size)
			ret = SANE_STATUS_GOOD;
	}

	return ret;
}

static SANE_Status RTS_ctl_iread_byte(st_device *dev, SANE_Int address, SANE_Byte *data, SANE_Int index)
{
	SANE_Status ret = SANE_STATUS_INVAL;

	if ((dev != NULL) && (data != NULL))
	{
		SANE_Byte buffer[2] = { 0x00, 0x00 };

		if (RTS_usb_ctl_read(dev, address, buffer, 0x02, index) == 2)
		{
			*data = (SANE_Byte)(buffer[0] & 0xff);
			ret = SANE_STATUS_GOOD;
		}
	}

	return ret;
}

static SANE_Status RTS_ctl_iread_word(st_device *dev, SANE_Int address, SANE_Int *data, SANE_Int index)
{
	SANE_Status ret = SANE_STATUS_INVAL;

	if ((dev != NULL) && (data != NULL))
	{
		SANE_Byte buffer[2] = { 0x00, 0x00 };

		if (RTS_usb_ctl_read(dev, address, buffer, 0x02, index) == 2)
		{
			*data = ((buffer[1] << 8) & 0xffff) + (buffer[0] & 0xff);
			ret = SANE_STATUS_GOOD;
		}
	}

	return ret;
}

static SANE_Status RTS_ctl_iread_int(st_device *dev, SANE_Int address, SANE_Int *data, SANE_Int index)
{
	SANE_Status ret = SANE_STATUS_INVAL;

	if ((dev != NULL) && (data != NULL))
	{
		SANE_Byte buffer[4] = { 0x00, 0x00, 0x00, 0x00 };

		*data = 0;
		if (RTS_usb_ctl_read(dev, address, buffer, 0x04, index) == 4)
		{
			SANE_Int C;

			for (C = 3; C >= 0; C--)
				*data = ((*data << 8) + (buffer[C] & 0xff)) & 0xffffffff;

			ret = SANE_STATUS_GOOD;
		}
	}

	return ret;
}

static SANE_Status RTS_ctl_iread_buffer(st_device *dev, SANE_Int address, SANE_Byte *buffer, SANE_Int size, SANE_Int index)
{
	SANE_Status ret = SANE_STATUS_INVAL;

	if ((dev != NULL) && (buffer != NULL))
		if (RTS_usb_ctl_read(dev, address, buffer, size, index) == size)
			ret = SANE_STATUS_GOOD;

	return ret;
}

static SANE_Status RTS_ctl_write_byte(st_device *dev, SANE_Int address, SANE_Byte data)
{
	return RTS_ctl_iwrite_byte(dev, address, data, 0x100, 0);
}

static SANE_Status RTS_ctl_write_word(st_device *dev, SANE_Int address, SANE_Int data)
{
	return RTS_ctl_iwrite_word(dev, address, data, 0);
}

/*static SANE_Status RTS_ctl_write_int(st_device *dev, SANE_Int address, SANE_Int data)
{
	return RTS_ctl_iwrite_int(usb_handle, address, data, 0);
}*/

static SANE_Status RTS_ctl_write_buffer(st_device *dev, SANE_Int address, SANE_Byte *buffer, SANE_Int size)
{
	return RTS_ctl_iwrite_buffer(dev, address, buffer, size, 0);
}

static SANE_Status RTS_ctl_read_byte(st_device *dev, SANE_Int address, SANE_Byte *data)
{
	return RTS_ctl_iread_byte(dev, address, data, 0x100);
}

static SANE_Status RTS_ctl_read_word(st_device *dev, SANE_Int address, SANE_Int *data)
{
	return RTS_ctl_iread_word(dev, address, data, 0x100);
}

static SANE_Status RTS_ctl_read_int(st_device *dev, SANE_Int address, SANE_Int *data)
{
	return RTS_ctl_iread_int(dev, address, data, 0x100);
}

static SANE_Status RTS_ctl_read_buffer(st_device *dev, SANE_Int address, SANE_Byte *buffer, SANE_Int size)
{
	return RTS_ctl_iread_buffer(dev, address, buffer, size, 0x100);
}

static SANE_Status RTS_blk_read(st_device *dev, SANE_Int buffer_size, SANE_Byte *buffer, SANE_Int *transfered)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "+ RTS_blk_read(buffer_size=%i):\n", buffer_size);

	if (transfered != NULL)
		*transfered = 0;

	if ((dev != NULL) && (buffer != NULL))
	{
		SANE_Int iTransferSize, iBytesToTransfer, iPos, iBytesTransfered;

		iBytesToTransfer = buffer_size;
		iPos = 0;
		rst = SANE_STATUS_GOOD;
		iBytesTransfered = 0;

		iTransferSize = min(buffer_size, dev->chipset->dma.transfer_size);
		
		do
		{
			iTransferSize = min(iTransferSize, iBytesToTransfer);

			iBytesTransfered = RTS_usb_blk_read(dev, &buffer[iPos], iTransferSize);
			if (iBytesTransfered < 0)
			{
				rst = SANE_STATUS_INVAL;
				break;
			} else
			{
				if (transfered != NULL)
					*transfered += iBytesTransfered;
			}
			iPos += iTransferSize;
			iBytesToTransfer -= iTransferSize;
		} while (iBytesToTransfer > 0);
	}

	DBG(DBG_FNC, "- RTS_blk_read: %i\n", rst);

	return rst;
}

static SANE_Status RTS_blk_write(st_device *dev, SANE_Int buffer_size, SANE_Byte *buffer, SANE_Int *transfered)
{
	SANE_Status rst = SANE_STATUS_INVAL;

	DBG(DBG_FNC, "+ RTS_blk_write(buffer_size=%i, buffer):\n", buffer_size);

	if (transfered != NULL)
		*transfered = 0;

	if ((dev != NULL) && (buffer != NULL))
	{
		SANE_Int iTransferSize, iBytesToTransfer, iPos;

		iBytesToTransfer = buffer_size;
		iPos = 0;
		rst = SANE_STATUS_GOOD;

		iTransferSize = min(buffer_size, dev->chipset->dma.transfer_size);

		do
		{
			iTransferSize = min(iTransferSize, iBytesToTransfer);

			if (RTS_usb_blk_write(dev, &buffer[iPos], iTransferSize) != SANE_STATUS_GOOD)
			{
				rst = SANE_STATUS_INVAL;
				break;
			} else
			{
				if (transfered != NULL)
					*transfered += iTransferSize;
			}
			iPos += iTransferSize;
			iBytesToTransfer -= iTransferSize;
		} while (iBytesToTransfer > 0);
	}

	DBG(DBG_FNC, "- RTS_blk_write: %i\n", rst);

	return rst;
}

static SANE_Status RTS_usb_blk_write(st_device *dev, SANE_Byte *buffer, SANE_Int size)
{
	SANE_Status rst = SANE_STATUS_INVAL;
	
	if ((dev != NULL) && (buffer != NULL))
	{
		dev->usb->dataline_count++;
		DBG(DBG_CTL, "%06i BLK DO: %i. bytes\n", dev->usb->dataline_count, size);
		RTS_usb_buffer_show(4, buffer, size);
		
		#ifdef STANDALONE
			if (dev->usb->handle != NULL)
				if (usb_bulk_write(dev->usb->handle, dev->usb->blk_write_ep, (char *)buffer, size, dev->usb->timeout) == size)
					rst = SANE_STATUS_GOOD;
		#else
			if (dev->usb->handle != -1)
			{
				size_t mysize = size;
				if (sanei_usb_write_bulk(dev->usb->handle, buffer, &mysize) == SANE_STATUS_GOOD)
					rst = SANE_STATUS_GOOD;
			}
		#endif
	}
	
	if (rst != SANE_STATUS_GOOD)
		DBG(DBG_CTL, "             : RTS_usb_blk_write error\n");
	
	return rst;
}

static SANE_Int RTS_usb_blk_read(st_device *dev, SANE_Byte *buffer, size_t size)
{
	SANE_Int rst = -1;

	if ((dev != NULL) && (buffer != NULL))
	{
		dev->usb->dataline_count++;
		DBG(DBG_CTL, "%06i BLK DI: Buffer length = %lu. bytes\n", dev->usb->dataline_count, (u_long) size);
		
		#ifdef STANDALONE
			if (dev->usb->handle != NULL)
				rst = usb_bulk_read(dev->usb->handle, dev->usb->blk_read_ep, (char *)buffer, size, dev->usb->timeout);
		#else
			if (dev->usb->handle != -1)
				if (sanei_usb_read_bulk(dev->usb->handle, buffer, &size) == SANE_STATUS_GOOD)
					rst = size;
		#endif
	}
	
	if (rst < 0)
		DBG(DBG_CTL, "             : RTS_usb_blk_read error\n");
			else RTS_usb_buffer_show(4, buffer, rst);
	
	return rst;
}

static SANE_Int RTS_usb_ctl_write(st_device *dev, SANE_Int address, SANE_Byte *buffer, SANE_Int size, SANE_Int index)
{
	SANE_Int rst = -1;

	if (dev != NULL)
	{
		dev->usb->dataline_count++;
		DBG(DBG_CTL, "%06i CTL DO: 40 04 %04x %04x %04x\n",
				dev->usb->dataline_count,
				address & 0xffff,
				index,
				size);
		RTS_usb_buffer_show(DBG_CTL, buffer, size);

		#ifdef STANDALONE
			if (dev->usb->handle != NULL)
				rst = usb_control_msg(dev->usb->handle,
															0x40,                  /* Request type */
															0x04,                  /* Request      */
															address,               /* Value        */
															index,                 /* Index        */
															(char *)buffer,        /* Buffer       */
															size,                  /* Size         */
															dev->usb->timeout);
		#else
			if (dev->usb->handle != -1)
			{
				if (sanei_usb_control_msg(dev->usb->handle,
																		0x40,            /* Request type */
																		0x04,            /* Request      */
																		address,         /* Value        */
																		index,           /* Index        */
																		size,            /* Size         */
																		buffer)          /* Buffer       */
							== SANE_STATUS_GOOD)
						rst = size;
							else rst = -1;
			}
		#endif
	}

	if (rst < 0)
		DBG(DBG_CTL, "             : Error, returned %i\n", rst);
	
	return rst;
}

static SANE_Int RTS_usb_ctl_read(st_device *dev, SANE_Int address, SANE_Byte *buffer, SANE_Int size, SANE_Int index)
{
	SANE_Int rst = -1;

	if (dev != NULL)
	{
		dev->usb->dataline_count++;
		DBG(DBG_CTL, "%06i CTL DI: c0 04 %04x %04x %04x\n",
				dev->usb->dataline_count,
				address & 0xffff,
				index,
				size);
			
		#ifdef STANDALONE
			if (dev->usb->handle != NULL)
				rst = usb_control_msg(dev->usb->handle,
															0xc0,                 /* Request type */
															0x04,                 /* Request      */
															address,              /* Value        */
															index,                /* Index        */
															(char *)buffer,       /* Buffer       */
															size,                 /* Size         */
															dev->usb->timeout);
		#else
			if (dev->usb->handle != -1)
			{
				if (sanei_usb_control_msg(dev->usb->handle,
																		0xc0,           /* Request type */
																		0x04,           /* Request      */
																		address,        /* Value        */
																		index,          /* Index        */
																		size,           /* Size         */
																		buffer)         /* Buffer       */
							== SANE_STATUS_GOOD)
						rst = size;
							else rst = -1;
			}
		#endif
	}

	if (rst < 0)
		DBG(DBG_CTL, "             : Error, returned %i\n", rst);
			else RTS_usb_buffer_show(DBG_CTL, buffer, rst);

	return rst;
}

static void RTS_usb_buffer_show(SANE_Int level, SANE_Byte *buffer, SANE_Int size)
{
	if (DBG_LEVEL >= level)
	{
		char *sline = NULL;
		char *sdata = NULL;
		SANE_Int cont, data, offset = 0, col = 0;

		if ((size > 0) && (buffer != NULL))
		{
			sline = (char *)malloc(256);
			if (sline != NULL)
			{
				sdata = (char *)malloc(256);
				if (sdata != NULL)
				{
					bzero(sline, 256);
					for (cont = 0; cont < size; cont++)
					{
						if (col == 0)
						{
							if (cont == 0)
								snprintf(sline, 255, "           BF: ");
									else snprintf(sline, 255, "               ");
						}
						data = (buffer[cont] & 0xff);
						snprintf(sdata, 255, "%02x ", data);
						sline = strcat(sline, sdata);
						col++;
						offset++;
						if (col == 8)
						{
							col = 0;
							snprintf(sdata, 255, " : %i\n", offset - 8);
							sline = strcat(sline, sdata);
							DBG(level, "%s", sline);
							bzero(sline, 256);
						}
					}
					if (col > 0)
					{
						for (cont = col; cont < 8; cont++)
						{
							snprintf(sdata, 255, "-- ");
							sline = strcat(sline, sdata);
							offset++;
						}
						snprintf(sdata, 255, " : %i\n", offset - 8);
						sline = strcat(sline, sdata);
						DBG(level, "%s", sline);
						bzero(sline, 256);
					}
					free(sdata);
				}
				free(sline);
			}
		} else DBG(level, "           BF: Empty buffer\n");
	}
}

#endif /* HP3900USB */
