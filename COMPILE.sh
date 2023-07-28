#!/bin/bash

# read version of hp3900-series
. ./scripts/version.sh

# read config
. ./scripts/config.sh

# read colors
. ./scripts/colors.sh

COMPILE_HEADER=1
COMPILE_RESULT=0

QUIET=0

# functions
function header()
{
	if test $COMPILE_HEADER -eq "1"
	then
		# clear screen
		clear

		echo -en $LIGHT_GRAY"hp3900-series $version - COMPILER\n"
		echo "Created by $AUTHOR <$EMAIL>"
		echo -en $GRAY"\n"
	fi
}

function compile_help()
{
	# show installer header
	header

	echo
	echo "$0 [arguments]"
	echo
	echo -en $LIGHT_GRAY"Arguments:\n\n"
	echo -en $LIGHT_GRAY"--type <#>: "$GRAY"Selects application to compile. Posible values are:\n"
	echo "       1: hp3900-series backend"
	echo "       2: SANE project"
	echo "       3: Both projects"
	echo
	echo -en $LIGHT_GRAY"--noheader: "$GRAY"Hides application's header\n"
	echo -en $LIGHT_GRAY"--quiet   : "$GRAY"Hides less important messages\n"

	exit 0
}

function compile_select()
{
	if test $compile_type -eq "-1";
	then
		compile_type=2; # by default SANE Backend

		echo
		echo -en $GRAY"Select application you wish to compile:\n"
		echo "1 - Stand-alone application"
		echo "2 - SANE Backend"
		echo
		echo -en $LIGHT_GRAY"Compile type [$compile_type]: "$GRAY
		read ins_type

		if test ! -z $ins_type;
		then
			if test $ins_type -gt "0";
			then
				if test $ins_type -lt "3";
				then
					compile_type=$ins_type
				fi
			fi
		fi
	fi
}

function stdalone_compile()
{
	# get current directory
	CURDIR=$PWD

	# check if file stdcompile exists
	if !(test -e $UPD_PATH_SRC_STDALONE/stdcompile.sh);
	then
		echo -en $WARNING"Sources not found. Must download first...\n"$GRAY
		./UPDATE.sh --type 1 --noheader
		if test $? -ne "0";
		then
			# update failed, exit
			echo -en $ERROR"Sources couldn't be downloaded. Aborting compile process ...\n"
			exit 1
		fi
	fi

	if test $QUIET -eq "0";
	then
		echo
		echo -en $LIGHT_GRAY"- Starting compile process of stand-alone application...\n"$GRAY
	fi

	# lets move to working directory
	cd $UPD_PATH_SRC_STDALONE

	# compile
	if test $QUIET -eq "0";
	then
		echo -en $ACTION"Compiling ...\n"
	fi

	./stdcompile.sh > /dev/null

	if test $? -eq "0";
	then
		# return to root directory
		cd $CURDIR

		# generate destination path where copying files
		INS_PATH=$INS_PATH_BIN_STDALONE/others

		# delete all files at others directory
		rm -rf $INS_PATH/* > /dev/null

		# copy generated files to binaries directory
		if test $QUIET -eq "0";
		then
			echo -en $ACTION"Copying files to $INS_PATH ...\n"
		fi

		cp $UPD_PATH_SRC_STDALONE/hp3900 $INS_PATH/ > /dev/null
		cp $UPD_PATH_SRC_STDALONE/stdalone/hp3900.1 $INS_PATH/ > /dev/null
		cp $UPD_PATH_SRC_STDALONE/stdalone/hp3900.rules $INS_PATH/ > /dev/null
		cp $UPD_PATH_SRC_STDALONE/stdalone/hp3900.kmdr $INS_PATH/ > /dev/null

		# delete binary
		rm $UPD_PATH_SRC_STDALONE/hp3900 > /dev/null

		if test $QUIET -eq "0";
		then
			echo -en $LIGHT_GRAY"- Compilation ends succesfully...\n"$GRAY
		fi
	else
		COMPILE_RESULT=1
		echo -en $ERROR"Stand-alone compilation failed! ...\n"

		# return to root directory
		cd $CURDIR
	fi
}

function sane_compile()
{
	# get current directory
	CURDIR=$PWD

	# check if log directory exists
	if !(test -e "./log");
	then
		mkdir ./log
	else
		if test -e ./log/$CMP_LOG_SANE;
		then
			rm -f ./log/$CMP_LOG_SANE > /dev/null
		fi
	fi

	# check if file sane sources exist
	if !(test -e $UPD_PATH_SRC_SANE/configure);
	then
		echo -en $WARNING"Sources not found. Must download first...\n"$GRAY
		./UPDATE.sh --type 2 --noheader
		if test $? -ne "0";
		then
			# update failed, exit
			echo -en $ERROR"Sources couldn't be downloaded. Aborting compile process ...\n"
			exit 1
		fi
	fi

	if test $QUIET -eq "0";
	then
		echo
		echo -en $LIGHT_GRAY"- Starting compile process of SANE backend...\n"$GRAY
	fi

	# lets move to working directory
	cd $UPD_PATH_SRC_SANE

	# configure
	con=0

	if test $QUIET -eq "0";
	then
		echo -en $ACTION"Configuring compilation...\n"
	fi

	export BACKENDS=hp3900
	./configure --disable-translations --disable-latex --disable-warnings --prefix=/usr --sysconfdir=/etc --localstatedir=/var 2>/dev/null > $CURDIR/log/$CMP_LOG_SANE

	# look for usb.h
	cat $CURDIR/log/$CMP_LOG_SANE | grep -E "for.usb\.h.+yes" > /dev/null

	if test $? -ne "0"
	then
		echo -en $WARNING"usb.h couldn't be detected in your system. This header is needed to access USB devices. Although compilation can continue, backend won't be able to use USB scanners. To solve this problem, install libusb-dev package. In Debian compatible distros (ubuntu, knoppix..) it can be installed typing as root 'apt-get install libusb-dev'. In other distros like SuSE, package's name is libusb-devel and can be installed using Yast2\n\n"

		con=1
		echo -en $LIGHT_GRAY"Do you want to continue compiling backend? [N/y]:"$GRAY
		read ct

		if test ! -z $ct;
		then
			if test $ct = "y";
			then
				con=0
			fi
		fi
	fi

	# compile
	if test $con -eq "0";
	then
		if test $QUIET -eq "0";
		then
			echo -en $ACTION"Compiling...\n"
		fi

		make >> $CURDIR/log/$CMP_LOG_SANE 2>> $CURDIR/log/$CMP_LOG_SANE

		if test $? -eq "0";
		then
			# return to root directory
			cd $CURDIR

			# generate destination path where copying files
			INS_PATH=$INS_PATH_BIN_SANE/others

			# delete all files at others directory
			rm -rf $INS_PATH/* > /dev/null

			# copy generated files to binaries directory
			if test $QUIET -eq "0";
			then
				echo -en $ACTION"Copying files into $INS_PATH ...\n"
			fi

			cp $UPD_PATH_SRC_SANE/tools/udev/libsane.rules $INS_PATH > /dev/null
			cp $UPD_PATH_SRC_SANE/doc/sane-hp3900.5 $INS_PATH  > /dev/null
			cp $UPD_PATH_SRC_SANE/backend/libsane-hp3900.la $INS_PATH > /dev/null
			cp $UPD_PATH_SRC_SANE/backend/.libs/libsane-hp3900.so.*.*.* $INS_PATH  > /dev/null
			cp $UPD_PATH_SRC_SANE/backend/hp3900.conf $INS_PATH  > /dev/null

			# clear sources
			cd $UPD_PATH_SRC_SANE
			make --quiet distclean > /dev/null 2> /dev/null

			# return to root directory
			cd $CURDIR

			if test $QUIET -eq "0";
			then
				echo -en $LIGHT_GRAY"- Compilation ends succesfully...\n"$GRAY
				echo -en $LIGHT_GRAY"- When installation asks you about your distro, select 'Others (compile)' to install this binary\n"$GRAY
			fi
		else
			COMPILE_RESULT=1

			echo -en $ERROR"Compilation failed! ...\n"
			echo -en $ERROR"Check '$CMP_LOG_SANE' located at '$CURDIR/log'.\n"
			echo -en $ERROR"If you consider necessary, send log to project administrator at <$EMAIL>\n"

			# clear sources
			make --quiet distclean > /dev/null 2> /dev/null

			# return to root directory
			cd $CURDIR
		fi
	else
		COMPILE_RESULT=1
		echo -en $ERROR"Compilation canceled! ...\n"

		# return to root directory
		cd $CURDIR
	fi
}

# parse arguments
setting=0

while (( $# ))
do
	if test $setting -gt 0;
	then
		# an argument setting
		case $setting in
		1) # type
			if test $1 -gt "0";
			then
				if test $1 -lt "4";
				then
					compile_type=$1
				fi
			fi;;
		esac

		# reset setting status
		setting=0;
	else
		# another argument
		case $1 in
		--help )
			compile_help;;
		--quiet )
			QUIET=1;;
		--type )
			setting=1;;
		--noheader )
			COMPILE_HEADER=0;;
		*)
			echo "Unknown '$1' option. Type $0 --help"
			exit 1;;
		esac
	fi

	shift
done


# show installer header
header

# user must select application to install
compile_select

case $compile_type in
1) # hp3900
	stdalone_compile;;
2) # sane
	sane_compile;;
3) # both
	stdalone_compile
	sane_compile;;
esac

# exit setup
exit $COMPILE_RESULT
