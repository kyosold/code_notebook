#include <signal.h>

void sig_catch(int sig,
               void (*f)())
{
    struct sigaction sa;
    sa.sa_handler = f;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(sig, &sa, (struct sigaction *)0);
}

void sig_block(int sig)
{
    sigset_t ss;
    sigemptyset(&ss);
    sigaddset(&ss, sig);
    sigprocmask(SIG_BLOCK, &ss, (sigset_t *)0);
}

void sig_unblock(int sig)
{
    sigset_t ss;
    sigemptyset(&ss);
    sigaddset(&ss, sig);
    sigprocmask(SIG_UNBLOCK, &ss, (sigset_t *)0);
}

void sig_blocknone()
{
    sigset_t ss;
    sigemptyset(&ss);
    sigprocmask(SIG_SETMASK, &ss, (sigset_t *)0);
}

void sig_pause()
{
    sigset_t ss;
    sigemptyset(&ss);
    sigsuspend(&ss);
}

void sig_bugcatch(void (*f)())
{
    sig_catch(SIGILL, f);
    sig_catch(SIGABRT, f);
    sig_catch(SIGFPE, f);
    sig_catch(SIGBUS, f);
    sig_catch(SIGSEGV, f);
#ifdef SIGSYS
    sig_catch(SIGSYS, f);
#endif
#ifdef SIGEMT
    sig_catch(SIGEMT, f);
#endif
}
