/*

hp3900-patcher v0.4 - CVS SANE patcher for hp3900 project
Copyright (C) 2007  Jonathan Bravo Lopez

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#define TITLE    "hp3900-patcher"
#define VERSION  "0.4"
#define INFO     "CVS SANE patcher for hp3900-series project"
#define LICENSE  "General Public License (GPL)"
#define URL      "http://jkdsoftware.dyndns.org"
#define AUTHOR   "Jonathan Bravo Lopez"
#define EMAIL    "jkdsoft@gmail.com"

#define TRUE  1
#define FALSE 0

#define BACKEND  "hp3900"

char SANE_Path[512];
char FROM_Path[512];
char confext[10] = {0};
int  verbose = 0;

static int ContentExists(char *path, char *text);
static int FileExists(char *path);
static int Parse_args(int argc, char *argv[]);
static int Set_AUTHOR(void);
static int Set_dllconf(void);
static int CopyFile(char *filename, char *FROM_Path, char *TO_Path);
static int CopyFiles(void);
static int Set_Makefile1(void);
static int Set_configure(void);
static int Set_configurein(void);
/*static int Set_unsupporteddesc(void);*/
static int Set_DocMakefile(void);
static int Set_Saneman(void);
static int Set_usermap(void);
static int show_about(int bexit);
static void show_help(void);

static int substr(char *sub, char *line);
static int lowercase(char *dest, char *source, int size);
static int trim(char *dest, char *source, int dsize);
static int ltrim(char *dest, char *source, int dsize);
static int rtrim(char *dest, char *source, int dsize);
static int get_word(char *word, char *ftext, int fsize, int dsize, int cut);
static int ldel(char *dest, char *source, int count);

int main(int argc, char *argv[])
{
	bzero(SANE_Path, 512);
	bzero(FROM_Path, 512);

	strcpy(confext, ".conf.in");

	/* Parse arguments*/
	Parse_args(argc, argv);

	if (FROM_Path[0] == '\0')
		strcpy(FROM_Path, "./");

	if (SANE_Path[0] == '\0')
	{
		printf("- %s: No SANE path set. Exiting...\n", TITLE);
		return -1;
	}

	/* patching file AUTHORS */
	Set_AUTHOR();
	/* patching file backend/dll.conf */
	Set_dllconf();
	/*
	patching file backend/hp3900.c
	patching file backend/hp3900.conf.in
	patching file backend/hp3900_config.c
	patching file backend/hp3900_debug.c
	patching file backend/hp3900_rts8822.c
	patching file backend/hp3900_sane.c
	patching file backend/hp3900_types.c
	patching file backend/hp3900_usb.c
	patching file doc/descriptions-external/hp3900.desc
	patching file doc/sane-hp3900.man
	*/
	CopyFiles();
	/* patching file backend/Makefile.in */
	Set_Makefile1();
	/* patching file configure */
	Set_configure();
	/* patching file configure.in */
	Set_configurein();
	/* patching file doc/descriptions/unsupported.desc */
	/*Set_unsupporteddesc();*/
	/* patching file doc/Makefile.in */
	Set_DocMakefile();
	/* patching file doc/sane.man */
	Set_Saneman();
	/* patching file tools/hotplug/libsane.usermap */
	Set_usermap();
	return 0;
}

static int ContentExists(char *path, char *text)
{
	char *buffer = malloc(512);
	int rst = FALSE;

	if (buffer != NULL)
	{
		snprintf(buffer, 512, "grep \"%s\" %s > /dev/null", text, path);
		rst = (system(buffer) == 0)? TRUE: FALSE;

		free(buffer);
	}

	return rst;
}

static int FileExists(char *path)
{
	int rst = 0;
	FILE *fd;

	fd = fopen(path, "r");
	if (fd != NULL)
	{
		fclose(fd);
		rst = 0;
	} else rst = -1;
	return rst;
}

static int Parse_args(int argc, char *argv[])
{
	int C;
	char Setting = 0;
	char *op = NULL;
	
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
							strcpy(SANE_Path, op);
							if (SANE_Path[strlen(SANE_Path) - 1] != '/')
								strcat(SANE_Path, "/");
							break;
						case 2:
							strcpy(FROM_Path, op);
							if (FROM_Path[strlen(FROM_Path) - 1] != '/')
								strcat(FROM_Path, "/");
							break;
					}
					Setting = 0;
				} else
				{
					/* Options */
					if (strcmp(op, "--sane") == 0)
						Setting = 1;
					else if (strcmp(op, "--from") == 0)
						Setting = 2;
					else if (strcmp(op, "--oldconf") == 0)
						strcpy(confext, ".conf");
					else if (strcmp(op, "--verbose") == 0)
						verbose = 1;
					else if (strcmp(op, "--help") == 0)
						show_help();
					else if (strcmp(op, "--about") == 0)
						show_about(1);
					else if (strcmp(op, "--version") == 0)
					{
						printf("%s\n", VERSION);
						exit(0);
					}
				}
			}
			C++;
		}
	}

	return 0;
}

static int Set_AUTHOR()
{
	char path[1024];
	char cmd[1024];
	char data[1024];
	FILE *fs, *fd;
	int status = 0;
	int Phase = 0;
	int bline = 0;

	snprintf(path, 1023, "%sAUTHORS", SANE_Path);
	if (FileExists(path) != 0)
	{
		printf("- %s: I can't find AUTHORS file in '%s'\n", TITLE, SANE_Path);
		return -1;
	}

	if (ContentExists(path, AUTHOR) == FALSE)
	{
		fs = fopen(path, "r");
		if (fs == NULL)
		{
			printf("- %s: I can't open AUTHORS file in '%s'\n", TITLE, SANE_Path);
			return -1;
		}

		fd = fopen("patch.tmp", "w");
		if (fd == NULL)
		{
			printf("- %s: I can't open temporal file\n", TITLE);
			fclose(fs);
			return -1;
		}

		if (verbose != 0)
			printf("- %s: Patching AUTHORS file in '%s'\n", TITLE, SANE_Path);

		while (!feof(fs))
		{
			if (fgets(cmd, 1024, fs) != NULL)
			{
				switch(Phase)
				{
					case 0: /* We don't know where we are */
						if ((substr("Backends:", cmd) == 0)&&((status & 0x01) == 0))
							Phase = 1;
						else if ((substr("Email addresses:", cmd) == 0)&&((status & 0x02) == 0))
							Phase = 2;
						break;
					case 1: /* Backends group */
						ltrim(data, cmd, 1024);
						if (data[0] == '\n')
						{
							if (bline > 0)
							{
								/* This is the end of the group so we must write backend author here */
								if (snprintf(data, 1024, " %s:        %s\n", BACKEND, AUTHOR) > 0)
								{
									fputs(data, fd);
									status = status | 1;
									Phase = 0;
									bline = 0;
								}
							} else bline++;
						} else
						{
							if (substr(":", data) > 0)
							{
								char a[1024];
								int b;
								get_word(data, data, 1024, 1024, 0);
								if (data[strlen(data) - 1] == ':')
									data[strlen(data) - 1] = '\0';
								lowercase(data, data, 1024);
								lowercase(a, BACKEND, 1024);
								if (strcmp(data, a) != 0)
								{
									b = strlen(data);
									b = (strlen(a) < b) ? strlen(a) : b;
									if (strncmp(data, a, b) > 0)
									{
										if (snprintf(data, 1024, " %s:        %s\n", BACKEND, AUTHOR) > 0)
										{
											fputs(data, fd);
											status = status | 1;
											Phase = 0;
											bline = 0;
										}
									}
								} else
								{
									/* This group is already patched */
									status = status | 1;
									Phase = 0;
									bline = 0;
								}
							}
						}
						break;
					case 2: /* Email group */
						ltrim(data, cmd, 1024);
						if (data[0] == '\n')
						{
							if (bline > 0)
							{
								/* This is the end of the group so we must write email here */
								if (snprintf(data, 1024, "%s <%s>\n", AUTHOR, EMAIL) > 0)
								{
									fputs(data, fd);
									status = status | 2;
									Phase = 0;
									bline = 0;
								}
							} else bline++;
						} else
						{
							if (substr("<", data) > 0)
							{
								char a[1024];
								int b;
								get_word(data, data, 1024, 1024, 0);
								get_word(a, AUTHOR, 1024, 1024, 0);
								if (data[strlen(data) - 1] == '.')
									data[strlen(data) - 1] = '\0';
								lowercase(data, data, 1024);
								lowercase(a, a, 1024);
								if (strcmp(data, a) != 0)
								{
									b = strlen(data);
									b = (strlen(a) < b) ? strlen(a) : b;
									if (strncmp(data, a, b) > 0)
									{
										if (snprintf(data, 1024, "%s <%s>\n", AUTHOR, EMAIL) > 0)
										{
											fputs(data, fd);
											status = status | 2;
											Phase = 0;
											bline = 0;
										}
									}
								} else
								{
									/* This group is already patched */
									status = status | 2;
									Phase = 0;
									bline = 0;
								}
							}
						}
						break;
				}
				fputs(cmd, fd);
			}
		}
		fclose(fs);

		if ((status & 1) == 0)
		{
			fputs("Backends:\n", fd);
			if (snprintf(data, 1024, " %s:        %s\n", BACKEND, AUTHOR) > 0)
				fputs(data, fd);
		}

		if ((status & 2) == 0)
		{
			fputs("Email addresses:\n", fd);
			if (snprintf(data, 1024, "%s <%s>\n", AUTHOR, EMAIL) > 0)
				fputs(data, fd);
		}

		fclose(fd);

		if (snprintf(cmd, 1024, "mv patch.tmp %s", path) > 0)
			system(cmd);
	} else printf("- %s: 'AUTHORS' file already patched. Skipping ...\n", TITLE);

	return 0;
}

static int Set_dllconf()
{
	char path[1024];
	char cmd[1024];
	char data[1024];
	FILE *fs, *fd;
	int status = 0;
	int Phase = 0;

	snprintf(path, 1023, "%sbackend/dll%s", SANE_Path, confext);
	if (FileExists(path) != 0)
	{
		printf("- %s: I can't find dll%s file in '%sbackend/'\n", TITLE, confext, SANE_Path);
		return -1;
	}

	if (ContentExists(path, BACKEND) == FALSE)
	{
		fs = fopen(path, "r");
		if (fs == NULL)
		{
			printf("- %s: I can't open dll%s file in '%sbackend/'\n", TITLE, confext, SANE_Path);
			return -1;
		}

		fd = fopen("patch.tmp", "w");
		if (fd == NULL)
		{
			printf("- %s: I can't open temporal file\n", TITLE);
			fclose(fs);
			return -1;
		}

		if (verbose != 0)
			printf("- %s: Patching 'dll%s' file in '%sbackend/'\n", TITLE, confext, SANE_Path);

		while (!feof(fs))
		{
			if (fgets(cmd, 1024, fs) != NULL)
			{
				switch(Phase)
				{
					case 0: /* We don't know where we are */
						if ((substr("abaton", cmd) == 0)&&((status & 0x01) == 0))
							Phase = 1;
						break;
					case 1: /* Backends group */
						ltrim(data, cmd, 1024);
						if (data[0] == '\n')
						{
							/* This is the end of the group so we must write backend author here */
							if (snprintf(data, 1024, "%s\n", BACKEND) > 0)
							{
								fputs(data, fd);
								status = status | 1;
								Phase = 0;
							}
						} else
						{
								char a[1024];
								int b;
								get_word(data, data, 1024, 1024, 0);
								if (data[0] == '#')
									ldel(data, data, 1);
								lowercase(data, data, 1024);
								lowercase(a, BACKEND, 1024);
								if (strcmp(data, a) != 0)
								{
									b = strlen(data);
									b = (strlen(a) < b) ? strlen(a) : b;
									if (strncmp(data, a, b) > 0)
									{
										if (snprintf(data, 1024, "%s\n", BACKEND) > 0)
										{
											fputs(data, fd);
											status = status | 1;
											Phase = 0;
										}
									}
								} else
								{
									/* This group is already patched */
									status = status | 1;
									Phase = 0;
								}
						}
						break;
				}
				fputs(cmd, fd);
			}
		}

		fclose(fs);

		if (status == 0)
		{
			if (snprintf(data, 1024, "%s\n", BACKEND) > 0)
				fputs(data, fd);
		}

		fclose(fd);

		if (snprintf(cmd, 1024, "mv patch.tmp %s", path) > 0)
			system(cmd);
	} else printf("- %s: 'dll.conf' file already patched. Skipping ...\n", TITLE);

	return 0;
}

static int CopyFile(char *filename, char *FROM_Path, char *TO_Path)
{
	char path[1024];
	char cmd[1024];
	int rst = -1;

	snprintf(path, 1023, "%s%s", FROM_Path, filename);
	if (FileExists(path) == 0)
	{
		snprintf(cmd, 1023, "cp %s %s", path, TO_Path);
		if (system(cmd) == 0)
			rst = 0;
		printf("- %s: Copying %s to %s\n", TITLE, filename, TO_Path);
	} else printf("- %s: I can't find '%s' file in '%s'\n", TITLE, filename, FROM_Path);

	return rst;
}

static int CopyFiles()
{
	char frompath[1024];
	char topath[1024];
	char file[1024];

	snprintf(frompath, 1023, "%s", FROM_Path);
	snprintf(topath, 1023, "%sbackend/", SANE_Path);
	CopyFile("hp3900.c", frompath, topath);
	CopyFile("hp3900_config.c", frompath, topath);
	CopyFile("hp3900_debug.c", frompath, topath);
	CopyFile("hp3900_rts8822.c", frompath, topath);
	CopyFile("hp3900_sane.c", frompath, topath);
	CopyFile("hp3900_types.c", frompath, topath);
	CopyFile("hp3900_usb.c", frompath, topath);

	snprintf(frompath, 1023, "%ssane/", FROM_Path);
	snprintf(file, 1023, "%shp3900%s", topath, confext);
	CopyFile("hp3900.conf.in", frompath, file);

	snprintf(topath, 1023, "%sdoc/descriptions/", SANE_Path);
	CopyFile("hp3900.desc", frompath, topath);

	snprintf(topath, 1023, "%sdoc/", SANE_Path);
	CopyFile("sane-hp3900.man", frompath, topath);

	return 0;
}

static int Set_Makefile1()
{
	char path[1024];
	char cmd[1024];
	char data[1024];
	int rst = 0;
	FILE *fs, *fd;
	int status = 0;
	int Phase = 0;

	snprintf(path, 1023, "%sbackend/Makefile.in", SANE_Path);
	if (FileExists(path) != 0)
	{
		printf("- %s: I can't find Makefile.in file in '%sbackend/'\n", TITLE, SANE_Path);
		return -1;
	}

	if (ContentExists(path, "hp3900.c") == FALSE)
	{
		fs = fopen(path, "r");
		if (fs == NULL)
		{
			printf("- %s: I can't open Makefile.in file in '%sbackend/'\n", TITLE, SANE_Path);
			return -1;
		}

		fd = fopen("patch.tmp", "w");
		if (fd == NULL)
		{
			printf("- %s: I can't open temporal file\n", TITLE);
			fclose(fs);
			return -1;
		}

		if (verbose != 0)
			printf("- %s: Patching 'Makefile.in' file in '%sbackend/'\n", TITLE, SANE_Path);

		while (!feof(fs))
		{
			if (fgets(cmd, 1024, fs) != NULL)
			{
				switch(Phase)
				{
					case 0: /* We don't know where we are */
						if ((substr("DISTFILES =", cmd) == 0)&&((status & 0x01) == 0))
							Phase = 1;
						else if ((substr("libsane-abaton.la", cmd) == 0)&&((status & 0x02) == 0))
							Phase = 2;
						break;
					case 1: /* Backends group */
						strcpy(data, cmd);
						if (data[strlen(data) - 1] == '\n')
							data[strlen(data) - 1] = '\0';

						trim(data, data, 1024);
						if (data[strlen(data) - 1] != '\\')
						{
							char file[1024];

							/* This is the end of the group so we must write backend's files here */
							if (cmd[strlen(cmd) - 1] == '\n')
								cmd[strlen(cmd) - 1] = '\0';
							rtrim(cmd, cmd, 1024);
							strcat(cmd, " \\\n");
							fputs(cmd, fd);
							snprintf(file, 1023, "  hp3900.c hp3900%s hp3900_config.c hp3900_debug.c hp3900_rts8822.c \\\n", confext);
							fputs(file, fd);
							fputs("  hp3900_sane.c hp3900_types.c hp3900_usb.c\n", fd);
							cmd[0] = '\0';
							Phase = 0;
							status = status | 1;
						} else
						{
							char a[1024];
							int b;
							get_word(data, data, 1024, 1024, 0);
							lowercase(data, data, 1024);
							lowercase(a, BACKEND, 1024);
							if (substr(a, data) < 0)
							{
								char file[1024];
								b = strlen(data);
								b = (strlen(a) < b) ? strlen(a) : b;
								if (strncmp(data, a, b) > 0)
								{
									snprintf(file, 1023, "  hp3900.c hp3900%s hp3900_config.c hp3900_debug.c hp3900_rts8822.c \\\n", confext);
									fputs(file, fd);
									fputs("  hp3900_sane.c hp3900_types.c hp3900_usb.c \\\n", fd);
									status = status | 1;
									Phase = 0;
								}
							} else
							{
								/* This group is already patched */
								status = status | 1;
								Phase = 0;
							}
						}
						break;
					case 2: /* libraries group */
						ltrim(data, cmd, 1024);
						if (data[0] == '\n')
						{
							/* This is the end of the group so we must write email here */
							fputs("libsane-hp3900.la: ../sanei/sanei_usb.lo\n", fd);
							status = status | 2;
							Phase = 0;
						} else
						{
							if (substr(":", data) > 0)
							{
								char a[1024];
								int b;
								get_word(data, data, 1024, 1024, 0);
								strcpy(a, "libsane-hp3900.la");
								if (data[strlen(data) - 1] == ':')
									data[strlen(data) - 1] = '\0';
								lowercase(data, data, 1024);
								if (strcmp(data, a) != 0)
								{
									b = strlen(data);
									b = (strlen(a) < b) ? strlen(a) : b;
									if (strncmp(data, a, b) > 0)
									{
										fputs("libsane-hp3900.la: ../sanei/sanei_usb.lo\n", fd);
										status = status | 2;
										Phase = 0;
									}
								} else
								{
									/* This group is already patched */
									status = status | 2;
									Phase = 0;
								}
							}
						}
						break;
				}
				if (strlen(cmd) > 0)
					fputs(cmd, fd);
			}
		}
		fclose(fs);

		if ((status & 1) == 0)
		{
			printf("- %s: Error addind files to DISTFILES in '%s'\n", TITLE, path);
			rst = -1;
		}

		if ((status & 2) == 0)
		{
			printf("- %s: Error addind libraries in '%s'\n", TITLE, path);
			rst = -1;
		}

		fclose(fd);

		if (snprintf(cmd, 1024, "mv patch.tmp %s", path) > 0)
			system(cmd);
	} else printf("- %s: 'Makefile.in' file already patched. Skipping ...\n", TITLE);

	return rst;
}

static int Set_configure()
{
	char path[1024];
	char cmd[1024];
	char data[1024];
	char data2[1024];
	int rst = 0;
	FILE *fs, *fd;
	int a;

	snprintf(path, 1023, "%sconfigure", SANE_Path);
	if (FileExists(path) != 0)
	{
		printf("- %s: I can't find configure file in '%s'\n", TITLE, SANE_Path);
		return -1;
	}

	if (ContentExists(path, BACKEND) == FALSE)
	{
		fs = fopen(path, "r");
		if (fs == NULL)
		{
			printf("- %s: I can't open configure file in '%s/'\n", TITLE, SANE_Path);
			return -1;
		}

		fd = fopen("patch.tmp", "w");
		if (fd == NULL)
		{
			printf("- %s: I can't open temporal file\n", TITLE);
			fclose(fs);
			return -1;
		}

		if (verbose != 0)
			printf("- %s: Patching 'configure' file in '%s'\n", TITLE, SANE_Path);

		while (!feof(fs))
		{
			if (fgets(cmd, 1024, fs) != NULL)
			{
				if (substr("BACKENDS=\"abaton", cmd) >= 0)
				{
					/* Already patched ? */
					if (substr(BACKEND, cmd) == -1)
					{
						a = substr("\"", cmd);
						strncpy(data2, cmd, 1024);
						ldel(data, cmd, a + 1);
						if (snprintf(cmd, 1024, "    BACKENDS=\"%s %s", BACKEND, data) == 0)
						{
							/* Some error. Restore line */
							strncpy(cmd, data2, 1024);
							rst = -1;
						}
					} else rst = 0;
				}
				if (strlen(cmd) > 0)
					fputs(cmd, fd);
			}
		}
		fclose(fs);

		fclose(fd);

		if (snprintf(cmd, 1024, "mv patch.tmp %s", path) > 0)
		{
			system(cmd);
			if (snprintf(cmd, 1024, "chmod +x %s", path) > 0)
				system(cmd);
					else printf("- %s: Warning! I can't set x mode to '%s'\n", TITLE, path);
		}

		if (rst == -1)
			printf("- %s: I can't add BACKEND in '%s'\n", TITLE, path);
	} else printf("- %s: 'configure' file already patched. Skipping ...\n", TITLE);

	return rst;
}

static int Set_configurein()
{
	char path[1024];
	char cmd[1024];
	char data[1024];
	char data2[1024];
	int rst = 0;
	FILE *fs, *fd;
	int a;

	snprintf(path, 1023, "%sconfigure.in", SANE_Path);
	if (FileExists(path) != 0)
	{
		printf("- %s: I can't find configure.in file in '%s'\n", TITLE, SANE_Path);
		return -1;
	}

	if (ContentExists(path, BACKEND) == FALSE)
	{
		fs = fopen(path, "r");
		if (fs == NULL)
		{
			printf("- %s: I can't open configure.in file in '%s/'\n", TITLE, SANE_Path);
			return -1;
		}

		fd = fopen("patch.tmp", "w");
		if (fd == NULL)
		{
			printf("- %s: I can't open temporal file\n", TITLE);
			fclose(fs);
			return -1;
		}

		if (verbose != 0)
			printf("- %s: Patching 'configure.in' file in '%s'\n", TITLE, SANE_Path);

		while (!feof(fs))
		{
			if (fgets(cmd, 1024, fs) != NULL)
			{
				if (substr("BACKENDS=\"abaton", cmd) >= 0)
				{
					/* Already patched ? */
					if (substr(BACKEND, cmd) == -1)
					{
						a = substr("\"", cmd);
						strncpy(data2, cmd, 1024);
						ldel(data, cmd, a + 1);
						if (snprintf(cmd, 1024, "    BACKENDS=\"%s %s", BACKEND, data) == 0)
						{
							/* Some error. Restore line */
							strncpy(cmd, data2, 1024);
							rst = -1;
						}
					}
				}
				if (strlen(cmd) > 0)
					fputs(cmd, fd);
			}
		}
		fclose(fs);
		fclose(fd);

		if (snprintf(cmd, 1024, "mv patch.tmp %s", path) > 0)
			system(cmd);

		if (rst == -1)
			printf("- %s: I can't add BACKEND in '%s'\n", TITLE, path);
	} else printf("- %s: 'configure.in' file already patched. Skipping ...\n", TITLE);

	return rst;
}

/*static int Set_unsupporteddesc()
{
	char path[1024];
	char cmd[1024];
	char data[1024];
	FILE *fs, *fd;
	int Phase = 0;
	int bline = 0;

	snprintf(path, 1023, "%sdoc/descriptions/unsupported.desc", SANE_Path);
	if (FileExists(path) != 0)
	{
		printf("- Error: I can't find unsupported.desc file in '%sdoc/descriptions/'\n", SANE_Path);
		return -1;
	}

	fs = fopen(path, "r");
	if (fs == NULL)
	{
		printf("- Error: I can't open unsupported.desc file in '%sdoc/descriptions/'\n", SANE_Path);
		return -1;
	}

	fd = fopen("patch.tmp", "w");
	if (fd == NULL)
	{
		printf("- Error: I can't open temporal file\n");
		fclose(fs);
		return -1;
	}

	if (verbose != 0)
		printf("- Patching 'unsupported.desc' file in '%s'\n", SANE_Path);

	while (!feof(fs))
	{
		if (fgets(cmd, 1024, fs) != NULL)
		{
			switch(Phase)
			{
				case 0: *//* We don't know where we are */
/*					if (substr("ScanJet 3970", cmd) > 0)
						Phase = 1;
					else if (substr("ScanJet 4070", cmd) > 0)
						Phase = 1;
					break;
				case 1: *//* Remove group */
/*					ltrim(data, cmd, 1024);
					if (data[0] == '\n')
					{
						bline = 1;
						Phase = 0;
					} else
					{
						if (substr(":model", data) == 0)
						{
							if ((substr("ScanJet 3970", data) == -1)&&
									(substr("ScanJet 4070", data) == -1))
										Phase = 0;
						}
					}
					break;
			}
			if (Phase == 0)
			{
				if (bline == 0)
					fputs(cmd, fd);
						else bline = 0;
			}
		}
	}
	fclose(fs);
	fclose(fd);

	if (snprintf(cmd, 1024, "mv patch.tmp %s", path) > 0)
		system(cmd);

	return 0;
}
*/

static int Set_DocMakefile()
{
	char path[1024];
	char cmd[1024];
	char data[1024];
	char data2[1024];
	int rst = 0;
	FILE *fs, *fd;
	int status = 0;
	int a;

	snprintf(path, 1023, "%sdoc/Makefile.in", SANE_Path);
	if (FileExists(path) != 0)
	{
		printf("- %s: I can't find Makefile.in file in '%sdoc/'\n", TITLE, SANE_Path);
		return -1;
	}

	if (ContentExists(path, BACKEND) == FALSE)
	{
		fs = fopen(path, "r");
		if (fs == NULL)
		{
			printf("- %s: I can't open Makefile.in file in '%sdoc/'\n", TITLE, SANE_Path);
			return -1;
		}

		fd = fopen("patch.tmp", "w");
		if (fd == NULL)
		{
			printf("- %s: I can't open temporal file\n", TITLE);
			fclose(fs);
			return -1;
		}

		if (verbose != 0)
			printf("- %s: Patching 'Makefile.in' file in '%sdoc/'\n", TITLE, SANE_Path);

		while (!feof(fs))
		{
			if (fgets(cmd, 1024, fs) != NULL)
			{
				if ((substr("SECT5", cmd) >= 0) && ((status & 0x01) == 0))
				{
					/* Already patched ? */
					if (substr(BACKEND, cmd) == -1)
					{
						a = substr("=", cmd);
						if (a > 0)
						{
							strncpy(data2, cmd, 1024);
							ldel(data, cmd, a + 1);
							if (snprintf(cmd, 1024, "SECT5	= sane-%s.5 %s", BACKEND, data) == 0)
							{
								/* Some error. Restore line */
								strncpy(cmd, data2, 1024);
								rst = -1;
							} else status = status | 1;
						}
					} else status = status | 1;
				} else if ((substr("DISTFILES", cmd) >= 0) && ((status & 0x02) == 0))
				{
					/* Already patched ? */
					if (substr(BACKEND, cmd) == -1)
					{
						a = substr("=", cmd);
						if (a > 0)
						{
							strncpy(data2, cmd, 1024);
							ldel(data, cmd, a + 1);
							if (snprintf(cmd, 1024, "DISTFILES	= sane-%s.man %s", BACKEND, data) == 0)
							{
								/* Some error. Restore line */
								strncpy(cmd, data2, 1024);
								rst = -1;
							} else status = status | 2;
						}
					} else status = status | 2;
				}

				if (strlen(cmd) > 0)
					fputs(cmd, fd);
			}
		}
		fclose(fs);

		fclose(fd);

		if (snprintf(cmd, 1024, "mv patch.tmp %s", path) > 0)
			system(cmd);

		if (rst == -1)
			printf("- %s: I can't add BACKEND in '%s'\n", TITLE, path);
	} else printf("- %s: 'doc/Makefile.in' file already patched. Skipping ...\n", TITLE);

	return rst;
}

static int Set_Saneman()
{
	char path[1024];
	char cmd[1024];
	char data[1024];
	int rst = 0;
	FILE *fs, *fd;
	int status = 0;
	int Phase = 0;
	int bline = 3;

	snprintf(path, 1023, "%sdoc/sane.man", SANE_Path);
	if (FileExists(path) != 0)
	{
		printf("- %s: I can't find sane.man file in '%sdoc/'\n", TITLE, SANE_Path);
		return -1;
	}

	if (ContentExists(path, BACKEND) == FALSE)
	{
		fs = fopen(path, "r");
		if (fs == NULL)
		{
			printf("- %s: I can't open sane.man file in '%sdoc/'\n", TITLE, SANE_Path);
			return -1;
		}

		fd = fopen("patch.tmp", "w");
		if (fd == NULL)
		{
			printf("- %s: I can't open temporal file\n", TITLE);
			fclose(fs);
			return -1;
		}

		if (verbose != 0)
			printf("- %s: Patching 'sane.man' file in '%sdoc/'\n", TITLE, SANE_Path);

		while (!feof(fs))
		{
			if (fgets(cmd, 1024, fs) != NULL)
			{
				switch(Phase)
				{
					case 0: /* We don't know where we are */
						if ((substr(".SH \"BACKENDS FOR SCANNERS\"", cmd) == 0)&&((status & 0x01) == 0))
							Phase = 1;
						else if ((substr(".SH \"SEE ALSO\"", cmd) == 0)&&((status & 0x02) == 0))
							Phase = 2;
						break;
					case 1: /* Backends group */
						trim(data, cmd, 1024);
						if (substr(".B ", data) == 0)
						{
							if (substr(BACKEND, data) < 0)
							{
								char a[1024];
								int b;
								ldel(data, data, 2);
								ltrim(data, data, 1024);
								get_word(data, data, 1024, 1024, 0);
								lowercase(data, data, 1024);
								strncpy(a, BACKEND, 1024);
								lowercase(a, a, 1024);
								b = strlen(data);
								b = (strlen(a) < b) ? strlen(a) : b;
								if (strncmp(data, a, b) > 0)
								{
									fputs(".B hp3900\n", fd);
									fputs("The SANE backend for the Hewlett-Packard ScanJet 3900 series. See\n", fd);
									fputs(".BR sane-hp3900 (5)\n", fd);
									fputs("for details.\n", fd);
									fputs(".TP\n", fd);
									status = status | 1;
									Phase = 0;
								}
							} else
							{
								/* Already patched */
								status = status | 1;
								Phase = 0;
							}
						} else if (substr(".PP", data) == 0)
						{
							/* End of group, add here */
							fputs(".TP\n", fd);
							fputs(".B hp3900\n", fd);
							fputs("The SANE backend for the Hewlett-Packard ScanJet 3900 series. See\n", fd);
							fputs(".BR sane-hp3900 (5)\n", fd);
							fputs("for details.\n", fd);
							status = status | 1;
							Phase = 0;
						}
						break;
					case 2: /* libraries group */
						trim(data, cmd, 1024);
						if (substr(".BR", data) == 0)
						{
							if (substr(BACKEND, data) < 0)
							{
								if (bline == 0)
								{
									char a[1024];
									int b;
									ldel(data, data, 3);
									trim(data, data, 1024);
									get_word(data, data, 1024, 1024, 0);
									lowercase(data, data, 1024);
									snprintf(a, 1024, "sane-%s", BACKEND);
									lowercase(a, a, 1024);
									b = strlen(data);
									b = (strlen(a) < b) ? strlen(a) : b;
									if (strncmp(data, a, b) > 0)
									{
										fputs(".BR sane-hp3900 (5),\n", fd);
										status = status | 2;
										Phase = 0;
									}
								} else bline--;
							} else
							{
								/* Already patched */
								status = status | 2;
								Phase = 0;
							}
						} else if (substr(".SH AUTHOR", data) == 0)
						{
							/* End of group, add here */
							fputs(".BR sane-hp3900 (5),\n\n", fd);
							status = status | 2;
							Phase = 0;
						}
						break;
				}
				if (strlen(cmd) > 0)
					fputs(cmd, fd);
			}
		}
		fclose(fs);

		if ((status & 1) == 0)
		{
			printf("- %s: Error editing BACKEND info in '%s'\n", TITLE, path);
			rst = -1;
		}

		if ((status & 2) == 0)
		{
			printf("- %s: Error 2 editing '%s'\n", TITLE, path);
			rst = -1;
		}

		fclose(fd);

		if (snprintf(cmd, 1024, "mv patch.tmp %s", path) > 0)
			system(cmd);
	} else printf("- %s: 'doc/sane.man' file already patched. Skipping ...\n", TITLE);

	return rst;
}

static int Set_usermap()
{
	char path[1024];
	char cmd[1024];
	int rst = 0;
	FILE *fs, *fd;

	snprintf(path, 1023, "%stools/hotplug/libsane.usermap", SANE_Path);
	if (FileExists(path) != 0)
	{
		printf("- %s: I can't find libsane.usermap file in '%stools/hotplug/'\n", TITLE, SANE_Path);
		return -1;
	}

	fs = fopen(path, "r");
	if (fs == NULL)
	{
		printf("- %s: I can't open libsane.usermap file in '%stools/hotplug/'\n", TITLE, SANE_Path);
		return -1;
	}

	fd = fopen("patch.tmp", "w");
	if (fd == NULL)
	{
		printf("- %s: I can't open temporal file\n", TITLE);
		fclose(fs);
		return -1;
	}

	if (verbose != 0)
		printf("- %s: Patching 'libsane.usermap' file in '%stools/hotplug/'\n", TITLE, SANE_Path);

	while (!feof(fs))
	{
		if (fgets(cmd, 1024, fs) != NULL)
		{
			if (substr("# Hewlett-Packard|ScanJet 4100C", cmd) >= 0)
			{
				fputs("# Hewlett-Packard|ScanJet 3970c\n", fd);
				fputs("libusbscanner             0x0003      0x03f0   0x2305    0x0000       0x0000       0x00         0x00            0x00            0x00            0x00               0x00               0x00000000\n", fd);
				fputs("# Hewlett-Packard|ScanJet 4070 Photosmart\n", fd);
				fputs("libusbscanner             0x0003      0x03f0   0x2405    0x0000       0x0000       0x00         0x00            0x00            0x00            0x00               0x00               0x00000000\n", fd);
				fputs("# Hewlett-Packard|ScanJet 4370\n", fd);
				fputs("libusbscanner             0x0003      0x03f0   0x4105    0x0000       0x0000       0x00         0x00            0x00            0x00            0x00               0x00               0x00000000\n", fd);
				fputs("# Hewlett-Packard|ScanJet G3010\n", fd);
				fputs("libusbscanner             0x0003      0x03f0   0x4205    0x0000       0x0000       0x00         0x00            0x00            0x00            0x00               0x00               0x00000000\n", fd);
				fputs("# UMAX|Astra 4900\n", fd);
				fputs("libusbscanner             0x0003      0x06dc   0x0020    0x0000       0x0000       0x00         0x00            0x00            0x00            0x00               0x00               0x00000000\n", fd);
			}

			if (strlen(cmd) > 0)
				fputs(cmd, fd);
		}
	}
	fclose(fs);
	fclose(fd);

	if (snprintf(cmd, 1024, "mv patch.tmp %s", path) > 0)
		system(cmd);

	if (rst == -1)
		printf("- %s: I can't add scanner to '%s'\n", TITLE, path);

	return rst;
}

static int substr(char *sub, char *line)
{
	int rst = -1; /* Resultado por defecto */
	int sublen, txtlen, pos, subpos;
	char *subcadena = NULL;
	char *cadena = NULL;

	/* Verificamos que los parámetros están hinstanciados */
	if ((sub == NULL) || (line == NULL))
		return -1;

	/* Obtenemos sus longitudes */
	sublen = strlen(sub);
	txtlen = strlen(line);

	/* Si la subcadena es más grande que la cadena, la respuesta es obvia */
	if (sublen > txtlen)
		return -1;

	/* Creamos variables temporales ... */
	subcadena = (char *) malloc((1 + sublen) * sizeof(char));
	if (subcadena == NULL)
		return -1;

	cadena = (char *) malloc((1 + txtlen) * sizeof(char));
	if (cadena == NULL)
	{
		free(subcadena);
		return -1;
	}

	/*... con el contenido de la cadena y subcadena... */
	strcpy(subcadena, sub);
	strcpy(cadena, line);

	/* ...para pasar su contenido a minúsculas sin alterar los originales */
	lowercase(subcadena, subcadena, sublen + 1);
	lowercase(cadena, cadena, txtlen + 1);

	pos = 0;
	subpos = 0;

	/* Recorremos la cadena en busca de la subcadena, letra a letra */
	while ((pos <= (txtlen - sublen)) && (rst == -1))
	{
		/* Si una letra de la subcadena coincide con la de la cadena ... */
		if (*(cadena + pos + subpos) == *(subcadena + subpos))
		{
			/* Pasamos a la siguiente letra de la subcadena */
			subpos++;
			/* Si hemos verificado toda la subcadena correctamente... */
			if (subpos == sublen)
				rst = pos; /* Subcadena encontrada! */
		} else
		{
			/* De lo contrario empezamos de nuevo */
			subpos = 0;
			pos++;
		}
	}

	/* Liberamos la memoria */
	free(subcadena);
	free(cadena);

	/* y devolvemos resultado */
	return rst;
}

static int lowercase(char *dest, char *source, int size)
{
	char *pointer1, *pointer2;

	pointer1 = source;
	pointer2 = dest;

	if ((pointer1 == NULL)||(pointer2 == NULL))
		return -1;

	while (*pointer1 != '\0')
	{
		size--;
		if (size == 0)
		{
			*pointer2 = '\0';
			break;
		} else *pointer2 = tolower(*pointer1);

		pointer2++;
		pointer1++;
	}

	return 0;
}

static int ltrim(char *dest, char *source, int dsize)
{
	/* Deletes spaces on the left of a string */

	char *pointer;

	if ((dest == NULL)||(source == NULL))
		return -1;

	pointer = source;
	while (*pointer == ' ')
		pointer++;

	strncpy(dest, pointer, dsize);

	return 0;
}


static int rtrim(char *dest, char *source, int dsize)
{
	/* Deletes spaces on the right of a string */

	if ((dest == NULL)||(source == NULL))
		return -1;

	strncpy(dest, source, dsize);

	while (strlen(dest) > 0)
		if (dest[strlen(dest) - 1] == ' ')
			dest[strlen(dest) - 1] = '\0';
				else break;

	return 0;
}

static int trim(char *dest, char *source, int dsize)
{
	/* Deletes spaces on the right and on the left of a string */

	int rst = -1;

	if ((dest == NULL)||(source == NULL))
		return -1;

	if (ltrim(dest, source, dsize) == 0)
		if (rtrim(dest, dest, dsize) == 0)
			rst = 0;

	return rst;
}

static int ldel(char *dest, char *source, int count)
{
	/* Deletes count chars in a source string and saves it in dest */

	int txtlen;

	if ((dest == NULL)||(source == NULL))
		return -1;

	txtlen = strlen(source);
	if (txtlen < count)
		count = txtlen;

	strcpy(dest, source + count);

	return 0;
}

static int get_word(char *word, char *ftext, int fsize, int dsize, int cut)
{
	/* Gets a word from ftext and saves it in word */

	int fpos, dpos, found, tam;

	if ((word == NULL)||(ftext == NULL))
		return -1;

	fpos = 0;
	dpos = 0;
	tam = 0;
	found = 0;

	if (dsize > 1)
	{
		while ((fpos < fsize)&&(found == 0))
		{
			if ((ftext[fpos] != ' ')&&(ftext[fpos] != '\n'))
			{
				tam++;
				*(word + dpos) = *(ftext + fpos);
				if ((dpos + 1) < dsize)
					dpos++;
						else found = 1;
			} else found = 1;
			fpos++;
		}
		*(word + dpos) = '\0';
		if (cut != 0)
			strncpy(ftext, ftext + tam, fsize);
	} else *word = '\0';

	return tam;
}

static int show_about(int bexit)
{
	/* Shows info about this application */

	printf("%s v%s - %s\n", TITLE, VERSION, INFO);
	printf("Coded by %s <%s>\n", AUTHOR, EMAIL);
	printf("License: %s\n", LICENSE);
	printf("URL: %s\n\n", URL);

	if (bexit != 0)
		exit(0);

	return 0;
}

static void show_help()
{
	/* Shows application's help */
	show_about(0);

	printf("Usage %s <options>\n\n", TITLE);
	printf("Options:\n");
	printf("  --about      : Shows application's info\n");
	printf("  --from <path>: Sets folder where hp3900 project files are allocated\n");
	printf("  --help       : Shows this help\n");
	printf("  --oldconf    : Uses '.conf' extension instead of '.conf.in'\n");
	printf("  --sane <path>: Sets SANE folder to patch\n");
	printf("  --verbose    : Shows status messages while patching files\n");
	printf("  --version    : Shows application version\n");
	printf("\n");

	exit(0);
}
