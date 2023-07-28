#!/bin/bash

# read version of hp3900-series
. ./scripts/version.sh

# read config
. ./scripts/config.sh

# read colors
. ./scripts/colors.sh

PATCH_HEADER=1
PATCH_RESULT=0

# functions
function header()
{
	if test $PATCH_HEADER -eq "1"
	then
		# clear screen
		clear

		echo -en $LIGHT_GRAY"hp3900-series $version - PATCHER\n"
		echo "Created by $AUTHOR <$EMAIL>"
		echo -en $GRAY"\n"
	fi
}

function patch_help()
{
	# show installer header
	header

	echo
	echo "$0 [arguments]"
	echo
	echo -en $LIGHT_GRAY"Arguments:\n\n"
	echo -en $LIGHT_GRAY"--noheader: "$GRAY"Hides application's header\n"
	exit 0
}

function sane_patch()
{
	# get current directory
	CURDIR=$PWD

	# hp3900 sources must exist to patch sane sources so lets check that condition
	if !(test -e $UPD_PATH_SRC_STDALONE);
	then
		echo -en $ERROR"hp3900-series does NOT exist!. Aborting SANE patch\n"
		exit 1
	fi

	# sane sources must exist
	if !(test -e $UPD_PATH_SRC_SANE);
	then
		echo -en $ERROR"hp3900-series does NOT exist!. Aborting SANE patch\n"
		exit 1
	fi

	# hp3900 patcher must exist. If don't, compile it
	if !(test -e $UPD_PATH_SRC_STDALONE/patcher/hp3900-patcher);
	then
		# We need to compile patcher
		echo -en $ACTION"Compiling hp3900-series patcher ...\n"

		cd $UPD_PATH_SRC_STDALONE/patcher
		./compile.sh > /dev/null

		if test $? -eq "0";
		then
			# return to root directory
			cd $CURDIR
		else
			# return to root directory
			cd $CURDIR

			echo -en $ERROR"Some error compiling patcher.\n"
			exit 1
		fi
	fi

	# lets patch sane sources
	echo -en $ACTION"Patching SANE sources with hp3900-patcher ...\n"
	$UPD_PATH_SRC_STDALONE/patcher/hp3900-patcher --from $UPD_PATH_SRC_STDALONE/ --sane $UPD_PATH_SRC_SANE > /dev/null
}

# parse arguments
setting=0

while (( $# ))
do
	if test $setting -gt 0;
	then
		# an argument setting
		#case $setting in
		#1) # type
		#	if test $1 -gt "0";
		#	then
		#		if test $1 -lt "4";
		#		then
		#			patch_type=$1
		#		fi
		#	fi;;
		#esac

		# reset setting status
		setting=0;
	else
		# another argument
		case $1 in
		--help )
			patch_help;;
		--noheader )
			PATCH_HEADER=0;;
		*)
			echo "Unknown '$1' option. Type $0 --help"
			exit 1;;
		esac
	fi

	shift
done

# show patcher header
header

# patch
sane_patch

# exit setup
exit $PATCH_RESULT
