#!/bin/bash

source ../../helper_functions.bash

if test -x ./pass_argument_MPIRUN ; then
	MPIRUN=`./pass_argument_MPIRUN`
else
	exit 1
fi

rm -fr TRACE.* *.mpits set-0

TRACE=${0/\.sh/}

EXTRAE_CONFIG_FILE=extrae.xml ${MPIRUN} -np 1 ./trace-ldpreload.sh ./mpi_barrier_c

../../../../src/merger/mpi2prv -f TRACE.mpits -o ${TRACE}.prv

# Actual comparison
CheckEntryInPCF ${TRACE}.pcf MPI_Init
CheckEntryInPCF ${TRACE}.pcf MPI_Barrier
CheckEntryInPCF ${TRACE}.pcf MPI_Finalize

NumberEntriesInPRV ${TRACE}.prv 50000003 31
if [[ "${?}" -ne 1 ]] ; then
	die "There must be only one entry to MPI_Init"
fi

NumberEntriesInPRV ${TRACE}.prv 50000003 32
if [[ "${?}" -ne 1 ]] ; then
	die "There must be only one entry to MPI_Finalize"
fi

NumberEntriesInPRV ${TRACE}.prv 50000003 0
if [[ "${?}" -ne 2 ]] ; then
	die "There must be only two exits (one per MPI_Init and another per MPI_Finalize)"
fi

NumberEntriesInPRV ${TRACE}.prv 50000002 8
if [[ "${?}" -ne 1 ]] ; then
	die "There must be only one entry to MPI_Barrier"
fi

NumberEntriesInPRV ${TRACE}.prv 50000002 0
if [[ "${?}" -ne 1 ]] ; then
	die "There must be only one collective exit"
fi

rm -fr ${TRACE}.??? set-0 TRACE.*

exit 0
