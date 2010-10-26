#
# This file is automatically generated by the Extrae instrumentation Makefile.
# Edit with caution
#

if (${?EXTRAE_HOME}) then

	# Read configuration variables (if available)
	if (! -f ${EXTRAE_HOME}/etc/extrae-vars.csh) then
		echo "Error! Unable to locate ${EXTRAE_HOME}/etc/extrae-vars.sh"
		echo "Dying..."
		break
	else
		source ${EXTRAE_HOME}/etc/extrae-vars.csh
	endif

	if (${?LD_LIBRARY_PATH}) then
		setenv LD_LIBRARY_PATH ${EXTRAE_HOME}/lib:${LD_LIBRARY_PATH}
	else
		setenv LD_LIBRARY_PATH ${EXTRAE_HOME}/lib
	endif

	if (${?MPI_HOME}) then
		if (! -d ${MPI_HOME}/lib ) then
			echo "Unable to find libmpi library directory!"
		else
			setenv LD_LIBRARY_PATH ${MPI_HOME}/lib:${LD_LIBRARY_PATH}
		endif
	endif

	if (${?PACX_HOME}) then
		if (! -d ${PACX_HOME}/lib ) then
			echo "Unable to find libpacx library directory!"
		else
			setenv LD_LIBRARY_PATH ${PACX_HOME}/lib:${LD_LIBRARY_PATH}
		endif
	endif

	if (${?LIBXML2_HOME}) then
		if (! -d ${LIBXML2_HOME}/lib ) then
			echo "Unable to find libxml2 library directory!"
		else
			setenv LD_LIBRARY_PATH ${LIBXML2_HOME}/lib:${LD_LIBRARY_PATH}
		endif
	endif

	if (${?PAPI_HOME}) then
		if (! -d ${PAPI_HOME}/lib ) then
			echo "Unable to find PAPI library directory!"
		else
			setenv LD_LIBRARY_PATH ${PAPI_HOME}/lib:${LD_LIBRARY_PATH}
		endif
	endif

	if (${?DYNINST_HOME}) then
		if (! -d ${DYNINST_HOME}/lib ) then
			echo "Unable to find DynInst library directory!"
		else
			if (! -f ${DYNINST_HOME}/lib/libdyninstAPI_RT.so.1 ) then
				echo "Unable to find libdyninstAPI_RT.so.1 in the Dyninst library directory!"
			else
				setenv LD_LIBRARY_PATH ${DYNINST_HOME}/lib:${LD_LIBRARY_PATH}
				setenv DYNINSTAPI_RT_LIB ${DYNINST_HOME}/lib/libdyninstAPI_RT.so.1
			endif
		endif
	endif

else
	echo "You have to define EXTRAE_HOME to run this script"
endif

