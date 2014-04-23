/*****************************************************************************\
 *                        ANALYSIS PERFORMANCE TOOLS                         *
 *                                   Extrae                                  *
 *              Instrumentation package for parallel applications            *
 *****************************************************************************
 *     ___     This library is free software; you can redistribute it and/or *
 *    /  __         modify it under the terms of the GNU LGPL as published   *
 *   /  /  _____    by the Free Software Foundation; either version 2.1      *
 *  /  /  /     \   of the License, or (at your option) any later version.   *
 * (  (  ( B S C )                                                           *
 *  \  \  \_____/   This library is distributed in hope that it will be      *
 *   \  \__         useful but WITHOUT ANY WARRANTY; without even the        *
 *    \___          implied warranty of MERCHANTABILITY or FITNESS FOR A     *
 *                  PARTICULAR PURPOSE. See the GNU LGPL for more details.   *
 *                                                                           *
 * You should have received a copy of the GNU Lesser General Public License  *
 * along with this library; if not, write to the Free Software Foundation,   *
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA          *
 * The GNU LEsser General Public License is contained in the file COPYING.   *
 *                                 ---------                                 *
 *   Barcelona Supercomputing Center - Centro Nacional de Supercomputacion   *
\*****************************************************************************/

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- *\
 | @file: $HeadURL$
 | @last_commit: $Date$
 | @version:     $Revision$
\* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#include "common.h"

static char UNUSED rcsid[] = "$Id$";

#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_STDIO_H
# include <stdio.h>
#endif
#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif
#ifdef HAVE_UCONTEXT_H
# include <ucontext.h>
#endif
#ifdef HAVE_SIGNAL_H
# include <signal.h>
#endif

#include "sampling.h"
#include "trace_macros.h"
#include "threadid.h"
#include "wrapper.h"

#if defined(SAMPLING_SUPPORT)
int SamplingSupport = FALSE;
int SamplingRunning = FALSE;
int EnabledSampling = FALSE;
#endif

static struct sigaction signalaction;

int isSamplingEnabled(void)
{
#if defined(SAMPLING_SUPPORT)
	return EnabledSampling;
#else
	return FALSE;
#endif
}

void setSamplingEnabled (int enabled)
{
#if !defined(SAMPLING_SUPPORT)
	UNREFERENCED_PARAMETER(enabled);
#else
	EnabledSampling = (enabled != FALSE);
#endif
}


void Extrae_SamplingHandler (void* address)
{
	if (tracejant && isSamplingEnabled() && !Backend_inInstrumentation(THREADID))
	{
		UINT64 temps = Clock_getCurrentTime_nstore();
		SAMPLE_EVENT_HWC (temps, SAMPLING_EV, (unsigned long long) address);
		trace_callers (temps, 6, CALLER_SAMPLING);
	}
}

#if defined(IS_BGP_MACHINE) || defined(IS_BGQ_MACHINE)
/* BG/P  & BG/Q */
# if __WORDSIZE == 32
#  define UCONTEXT_REG(uc, reg) ((uc)->uc_mcontext.uc_regs->gregs[reg])
# else
#  define UCONTEXT_REG(uc, reg) ((uc)->uc_mcontext.gp_regs[reg])
# endif
# define PPC_REG_PC 32
#elif defined(IS_BGL_MACHINE)
# error "Don't know how to access the PC! Check if it is like BG/P"
#endif

static unsigned long long Sampling_variability;
static struct itimerval SamplingPeriod_base;
static struct itimerval SamplingPeriod;
static int SamplingClockType;

static void PrepareNextAlarm (void)
{
	/* Set next timer! */
	if (Sampling_variability > 0)
	{
		long int r = random();
		unsigned long long v = r%(Sampling_variability);
		unsigned long long s, us;

		us = (v + SamplingPeriod_base.it_value.tv_usec) % 1000000;
		s = (v + SamplingPeriod_base.it_value.tv_usec) / 1000000 + SamplingPeriod_base.it_interval.tv_sec;

		SamplingPeriod.it_interval.tv_sec = 0;
		SamplingPeriod.it_interval.tv_usec = 0;
		SamplingPeriod.it_value.tv_usec = us;
		SamplingPeriod.it_value.tv_sec = s;
	}
	else
		SamplingPeriod = SamplingPeriod_base;

	setitimer (SamplingClockType ,&SamplingPeriod, NULL);
}

static void TimeSamplingHandler (int sig, siginfo_t *siginfo, void *context)
{
	caddr_t pc;
#if defined(OS_FREEBSD) || defined(OS_DARWIN)
	ucontext_t *uc = (ucontext_t *) context;
#else
	struct ucontext *uc = (struct ucontext *) context;
	struct sigcontext *sc = (struct sigcontext *) &uc->uc_mcontext;
#endif

	UNREFERENCED_PARAMETER(sig);
	UNREFERENCED_PARAMETER(siginfo);

#if defined(IS_BGP_MACHINE) || defined(IS_BGQ_MACHINE)
	pc = (caddr_t)UCONTEXT_REG(uc, PPC_REG_PC);
#elif defined(OS_LINUX)
# if defined(ARCH_IA32) && !defined(ARCH_IA32_x64)
  pc = (caddr_t)sc->eip;
# elif defined(ARCH_IA32) && defined(ARCH_IA32_x64)
	pc = (caddr_t)sc->rip;
# elif defined(ARCH_IA64)
	pc = (caddr_t)sc->sc_ip;
# elif defined(ARCH_PPC)
	pc = (caddr_t)sc->regs->nip;
# elif defined(ARCH_ARM)
	pc = (caddr_t)sc->arm_pc;
# else
#  error "Don't know how to get the PC for this architecture in Linux!"
# endif
#elif defined(OS_FREEBSD)
# if defined(ARCH_IA32) && !defined(ARCH_IA32_x64)
	pc = (caddr_t)(uc->uc_mcontext.mc_eip);
# elif defined (ARCH_IA32) && defined(ARCH_IA32_x64)
	pc = (caddr_t)(uc->uc_mcontext.mc_rip);
# else
#  error "Don't know how to get the PC for this architecture in FreeBSD!"
# endif
#elif defined(OS_DARWIN)
# if defined(ARCH_IA32) && !defined(ARCH_IA32_x64)
	pc = (caddr_t)((uc->uc_mcontext)->__ss.__eip);
# elif defined (ARCH_IA32) && defined(ARCH_IA32_x64)
	pc = (caddr_t)((uc->uc_mcontext)->__ss.__rip);
# else
#  error "Don't know how to get the PC for this architecture in Darwin!"
# endif
#else
# error "Don't know how to get the PC for this OS!"
#endif

	Extrae_SamplingHandler ((void*) pc);

	PrepareNextAlarm ();
}


void setTimeSampling (unsigned long long period, unsigned long long variability, int sampling_type)
{
	int signum;
	int ret;

	memset (&signalaction, 0, sizeof(signalaction));

	ret = sigemptyset(&signalaction.sa_mask);
	if (ret != 0)
	{
		fprintf (stderr, PACKAGE_NAME": Error! Sampling error: %s\n", strerror(ret));
		return;
	}

	if (sampling_type == SAMPLING_TIMING_VIRTUAL)
	{
		SamplingClockType = ITIMER_VIRTUAL;
		signum = SIGVTALRM;
	}
	else if (sampling_type == SAMPLING_TIMING_PROF)
	{
		SamplingClockType = ITIMER_PROF;
		signum = SIGPROF;
	}
	else
	{
		SamplingClockType = ITIMER_REAL;
		signum = SIGALRM;
	}

	ret = sigaddset(&signalaction.sa_mask, signum);
	if (ret != 0)
	{
		fprintf (stderr, PACKAGE_NAME": Error! Sampling error: %s\n", strerror(ret));
		return;
	}

	if (variability > period)
	{
		fprintf (stderr, PACKAGE_NAME": Error! Sampling variability can't be higher than sampling period\n");
		variability = 0;
	}

	/* The period and variability are given in nanoseconds */
	period = (period - variability) / 1000; /* We well afterwards add the variability, this is the base */
	variability = variability / 1000;
 
	SamplingPeriod_base.it_interval.tv_sec = 0;
	SamplingPeriod_base.it_interval.tv_usec = 0;
	SamplingPeriod_base.it_value.tv_sec = period / 1000000;
	SamplingPeriod_base.it_value.tv_usec = period % 1000000;

	signalaction.sa_sigaction = TimeSamplingHandler;
	signalaction.sa_flags = SA_SIGINFO | SA_RESTART;

	ret = sigaction (signum, &signalaction, NULL);
	if (ret != 0)
	{
		fprintf (stderr, PACKAGE_NAME": Error! Sampling error: %s\n", strerror(ret));
		return;
	}

	if (variability >= RAND_MAX)
	{
		fprintf (stderr, PACKAGE_NAME": Error! Sampling variability is too high (%llu microseconds). Setting to %llu microseconds.\n", variability, (unsigned long long) RAND_MAX);
		Sampling_variability = RAND_MAX;
	}
	else
		Sampling_variability = 2*variability;

	SamplingRunning = TRUE;

	PrepareNextAlarm ();
}


void setTimeSampling_postfork (void)
{
	int signum;
	int ret;

	if (EnabledSampling)
	{
		memset (&signalaction, 0, sizeof(signalaction));

		ret = sigemptyset(&signalaction.sa_mask);
		if (ret != 0)
		{
			fprintf (stderr, PACKAGE_NAME": Error! Sampling error: %s\n", strerror(ret));
			return;
		}

		if (SamplingClockType == ITIMER_VIRTUAL)
			signum = SIGVTALRM;
		else if (SamplingClockType == ITIMER_PROF)
			signum = SIGPROF;
		else
			signum = SIGALRM;

		ret = sigaddset(&signalaction.sa_mask, signum);
		if (ret != 0)
		{
			fprintf (stderr, PACKAGE_NAME": Error! Sampling error: %s\n", strerror(ret));
			return;
		}
	
		signalaction.sa_sigaction = TimeSamplingHandler;
		signalaction.sa_flags = SA_SIGINFO | SA_RESTART;
	
		ret = sigaction (signum, &signalaction, NULL);
		if (ret != 0)
		{
			fprintf (stderr, PACKAGE_NAME": Error! Sampling error: %s\n", strerror(ret));
			return;
		}

		SamplingRunning = TRUE;
	
		PrepareNextAlarm ();
	}
}

void unsetTimeSampling (void)
{
	if (SamplingRunning)
	{
		int ret, signum;

		if (SamplingClockType == ITIMER_VIRTUAL)
			signum = SIGVTALRM;
		else if (SamplingClockType == ITIMER_PROF)
			signum = SIGPROF;
		else
			signum = SIGALRM;

		ret = sigdelset (&signalaction.sa_mask, signum);
		if (ret != 0)
			fprintf (stderr, PACKAGE_NAME": Error Sampling error: %s\n", strerror(ret));

		SamplingRunning = FALSE;
	}
}
