#include <server/misc_utils.h>

#include <common/types.h>

#include <pthread.h>
#include <signal.h>

YAIC_NAMESPACE

void MiscUtils::blockSignals()
{
    sigset_t set;

    sigfillset(&set);
    pthread_sigmask(SIG_SETMASK, &set, NULL);
}

END_NAMESPACE
