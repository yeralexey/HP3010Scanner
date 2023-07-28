#!/bin/bash

# read version of hp3900-series
. ./scripts/version.sh

# read config
. ./scripts/config.sh

# read colors
. ./scripts/colors.sh

UPDATE_HEADER=1
UPDATE_RESULT=0

# functions
function header()
{
	if test $UPDATE_HEADER -eq "1"
	then
		# clear screen
		clear

		echo -en $LIGHT_GRAY"hp3900-series $version - UPDATER\n"
		echo "Created by $AUTHOR <$EMAIL>"
		echo -en $GRAY"\n"
	fi
}

function update_help()
{
	# show installer header
	header

	echo
	echo "$0 [arguments]"
	echo
	echo -en $LIGHT_GRAY"Arguments:\n\n"
	echo -en $LIGHT_GRAY"--type <#>: "$GRAY"Selects application to update. Posible values are:\n"
	echo "       1: hp3900-series backend"
	echo "       2: SANE project"
	echo "       3: Both projects"
	echo
	echo -en $LIGHT_GRAY"--noheader: "$GRAY"Hides application's header\n"
	exit 0
}

function update_select()
{
	if test $update_type -eq "-1";
	then
		update_type=3; # by default SANE Backend

		echo
		echo -en $GRAY"Select project sources you wish to update:\n"
		echo "       1: hp3900-series backend"
		echo "       2: SANE project"
		echo "       3: Both projects"
		echo
		echo -en $LIGHT_GRAY"Update selection [$update_type]: "$GRAY
		read ins_type

		if test ! -z $ins_type
		then
			if test $ins_type -gt "0";
			then
				if test $ins_type -lt "4";
				then
					update_type=$ins_type
				fi
			fi
		fi
	fi
}

function sane_update()
{
	# get current directory
	CURDIR=$PWD

	# hp3900 sources must exist to patch sane sources so lets check that condition
	if !(test -e $UPD_PATH_SRC_STDALONE);
	then
		echo -en $WARNING"hp3900-series sources must exist to patch SANE sources ...\n"
		./UPDATE.sh --type 1 --noheader
		if test $? -ne "0"
		then
			echo -en $ERROR"Aborting SANE update\n"
			exit 1
		fi
	fi

	# delete temp directory
	if test -e $UPD_PATH_TMP_SANE
	then
		rm -rf $UPD_PATH_TMP_SANE
	fi

	# create an empty temp directory and enter into it
	mkdir $UPD_PATH_TMP_SANE
	cd $UPD_PATH_TMP_SANE

	# download sources using git (must be installed)
	echo -en $ACTION"Downloading SANE project sources from GIT...\n"
	git clone git://git.debian.org/sane/sane-backends.git > /dev/null 2> /dev/null

	if test $? -ne "0";
	then
			# return to root directory
		cd $CURDIR

		UPDATE_RESULT=1

		echo -en $ERROR"Some error caused GIT command execution. Are you sure you've installed GIT client?\n"

		# delete temp directory if has been created
		if test -e $UPD_PATH_TMP_SANE;
		then
			rm -rf $UPD_PATH_TMP_SANE
		fi
	else
		# return to root directory
		cd $CURDIR

		# patch sane sources
		echo -en $ACTION"Patching SANE sources with hp3900-patcher ...\n"
		$UPD_PATH_SRC_STDALONE/patcher/hp3900-patcher --from $UPD_PATH_SRC_STDALONE/ --sane $UPD_PATH_TMP_SANE/sane-backends

		# remove old directory
		if test -e $UPD_PATH_SRC_SANE;
		then
			rm -rf $UPD_PATH_SRC_SANE
		fi

		# rename downloaded directory
		mv $UPD_PATH_TMP_SANE/sane-backends ./src/
		rm -rf $UPD_PATH_TMP_SANE

		# show version retrieved
		./INFO.sh --noheader --type 3

		echo -en $LIGHT_GRAY"- Update succesfull...\n"$GRAY
	fi
}

function stdalone_update()
{
	# get current directory
	CURDIR=$PWD

	# delete temp directory
	if test -e $UPD_PATH_TMP_STDALONE;
	then
		rm -rf $UPD_PATH_TMP_STDALONE
	fi

	mkdir $UPD_PATH_TMP_STDALONE

	cd $UPD_PATH_TMP_STDALONE

	echo -en $ACTION"Downloading hp3900-series backend sources via GIT ...\n"

	# download sources using cvs (must be installed)
	git clone git://git.code.sf.net/p/hp3900-series/code hp3900-series > /dev/null 2> /dev/null

	if test $? -ne "0";
	then
		cd $CURDIR

		UPDATE_RESULT=1
		echo -en $ERROR"Some error caused GIT command execution. Are you sure you've installed GIT client?\n"

		# delete temp directory if has been created
		if test -e $UPD_PATH_TMP_STDALONE;
		then
			rm -rf $UPD_PATH_TMP_STDALONE
		fi
	else
		cd $CURDIR
		mv $UPD_PATH_TMP_STDALONE/hp3900-series/* $UPD_PATH_TMP_STDALONE
		rm -rf $UPD_PATH_TMP_STDALONE/hp3900-series

		# update installer scripts
		echo -en $ACTION"Updating installer scripts ...\n"

		if test -e $UPD_PATH_SRC_STDALONE/package ;
		then
			cp -f $UPD_PATH_SRC_STDALONE/package/*.* $CURDIR
			if test -e $UPD_PATH_SRC_STDALONE/package/scripts ;
			then
				cp -f $UPD_PATH_SRC_STDALONE/package/scripts/*.* $CURDIR/scripts/
			fi
		fi

		# We need to compile patcher
		echo -en $ACTION"Compiling hp3900-series patcher ...\n"

		cd $UPD_PATH_TMP_STDALONE/patcher
		./compile.sh > /dev/null

		if test $? -eq "0";
		then
			# return to root directory
			cd $CURDIR

			# remove old directory
			if test -e $UPD_PATH_SRC_STDALONE;
			then
				rm -rf $UPD_PATH_SRC_STDALONE
			fi

			# rename downloaded directory
			mv $UPD_PATH_TMP_STDALONE $UPD_PATH_SRC_STDALONE

			# show version retrieved
			./INFO.sh --noheader --type 2

			echo -en $LIGHT_GRAY"- Update succesfull...\n"$GRAY
		else
			UPDATE_RESULT=1
			echo -en $ERROR"Some error compiling patcher.\n"

			# return to root directory
			cd $CURDIR

			# delete temp directory
			rm -rf $UPD_PATH_TMP_STDALONE
		fi
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
					update_type=$1
				fi
			fi;;
		esac

		# reset setting status
		setting=0;
	else
		# another argument
		case $1 in
		--help )
			update_help;;
		--type )
			setting=1;;
		--noheader )
			UPDATE_HEADER=0;;
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
update_select

case $update_type in
1) # hp3900
	stdalone_update;;
2) # sane
	sane_update;;
3) # both
	stdalone_update
	sane_update;;
esac

# exit setup
exit $UPDATE_RESULT
