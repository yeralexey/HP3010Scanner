#!/bin/bash

# read version of hp3900-series
. ./scripts/version.sh

# read config
. ./scripts/config.sh

# read colors
. ./scripts/colors.sh

INFO_HEADER=1
INFO_RESULT=0
INFO_QUIET=0

# functions
function header()
{
	if test $INFO_HEADER -eq "1"
	then
		# clear screen
		clear

		echo -en $LIGHT_GRAY"hp3900-series $version - INFO\n"
		echo "Created by $AUTHOR <$EMAIL>"
		echo -en $GRAY"\n"
	fi
}

function info_help()
{
	# show installer header
	header

	echo
	echo "$0 [arguments]"
	echo
	echo -en $LIGHT_GRAY"Arguments:\n\n"
	echo -en $LIGHT_GRAY"--type <#>: "$GRAY"Selects information type. Posible values are:\n"
	echo "       0: All"
	echo "       1: hp3900-series binary version"
	echo "       2: hp3900-series sources version (if exists)"
	echo "       3: SANE project sources version (if exists)"
	echo
	echo -en $LIGHT_GRAY"--noheader: "$GRAY"Hides application's header\n"
	echo -en $LIGHT_GRAY"--quiet   : "$GRAY"Hides extra information\n"
	exit 0
}

function info_select()
{
	if test $info_type -eq "-1";
	then
		info_type=3; # by default SANE Backend

		echo
		echo -en $GRAY"Select information you wish to show:\n"
		echo "       0: All"
		echo "       1: hp3900-series/SANE binary version"
		echo "       2: hp3900-series sources version"
		echo "       3: SANE project sources version"
		echo
		echo -en $LIGHT_GRAY"Info selection [info_type]: "$GRAY
		read ins_type

		if test $ins_type -gt "-1";
		then
			if test $ins_type -lt "4";
			then
				info_type=$ins_type
			fi
		fi
	fi
}

function hp3900_bin_version()
{
	if test $INFO_QUIET -eq 0;
	then
		echo -en $INFO"hp3900/SANE binary version   :$GRAY "
	fi

	echo $version_bin
}

function hp3900_src_version()
{
	myversion='Unknown'

	if test -e $UPD_PATH_SRC_STDALONE/hp3900.c;
	then
		sline=$(cat $UPD_PATH_SRC_STDALONE/hp3900.c|grep -m 1 BACKEND_VRSN|cut -f2 -d'"')
		if test -n $sline
		then
			myversion=$sline
		fi
	fi

	if test $INFO_QUIET -eq 0;
	then
		echo -en $INFO"hp3900-series sources version:$GRAY "
	fi

	echo $myversion
}

function sane_src_version()
{
	myversion='Unknown'

	if test -e $UPD_PATH_SRC_SANE/configure;
	then
		sline=$(cat $UPD_PATH_SRC_SANE/configure|grep -m1 PACKAGE_VERSION=|cut -f2 -d"'")
		if test -n $sline
		then
			myversion=$sline
		fi
	fi

	if test $INFO_QUIET -eq 0;
	then
		echo -en $INFO"SANE sources version         :$GRAY "
	fi

	echo $myversion
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
			if test $1 -gt "-1";
			then
				if test $1 -lt "4";
				then
					info_type=$1
				fi
			fi;;
		esac

		# reset setting status
		setting=0;
	else
		# another argument
		case $1 in
		--help )
			info_help;;
		--type )
			setting=1;;
		--noheader )
			INFO_HEADER=0;;
		--quiet )
			INFO_QUIET=1;;
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
info_select

case $info_type in
0) # All information
	hp3900_bin_version
	hp3900_src_version
	sane_src_version;;
1) # hp3900 binary version
	hp3900_bin_version;;
2) # hp3900 sources version
	hp3900_src_version;;
3) # sane sources version
	sane_src_version;;
esac

# exit setup
exit $INFO_RESULT
