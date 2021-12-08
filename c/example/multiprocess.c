#include <stdio.h>
#include <syslog.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

unsigned long limit = 50;
unsigned long num_child = 0;

void sig_block(int sig);
void sig_unblock(int sig);
void sig_catch(int sig, void (*f)());
void sig_uncatch(int sig);
void sig_pause(void);
void sig_ignore(int sig);

void show_status(void)
{
    syslog(LOG_INFO, "status: %ld/%ld", num_child, limit);
}

void sigterm()
{
    _exit(0);
}

void sigchld()
{
    int wstat;
    int pid;
    while ((pid = waitpid(-1, &wstat, WNOHANG)) > 0)
    {
        if (num_child)
            --num_child;
        show_status();
    }
}

void usage(char *prog)
{
    printf("Usage:\n");
    printf("%s -c 100 program\n", prog);
    printf("Params:\n");
    printf("-c  max number of child");
    _exit(100);
}

int main(int argc, char **argv)
{
    int opt;
    char *program;
    pid_t pid_child = 0;

    while ((opt = getopt(argc, argv, "c:h")) != EOF)
    {
        switch (opt)
        {
        case 'c':
            limit = atoi(optarg);
            break;
        case 'h':
        default:
            usage(argv[0]);
        }
    }

    argc -= optind;
    argv += optind;
    program = *argv++;
    printf("program: %s\n", program);

    if (!program || !*program)
        usage(argv[0]);

    openlog(argv[0], LOG_PID | LOG_NDELAY, LOG_MAIL);

    sig_block(SIGCHLD);
    sig_catch(SIGCHLD, sigchld);
    sig_catch(SIGTERM, sigterm);
    sig_ignore(SIGPIPE);

    syslog(LOG_INFO, "start loop...");
    for (;;)
    {
        while (num_child >= limit)
            sig_pause();

        ++num_child;
        show_status();

        pid_child = fork();
        switch (pid_child)
        {
        case -1:
            syslog(LOG_ERR, "unable to fork: %s", strerror(errno));
            --num_child;
            break;
        case 0:
            // 子进程
            sig_uncatch(SIGCHLD);
            sig_unblock(SIGCHLD);
            sig_uncatch(SIGTERM);
            sig_uncatch(SIGPIPE);
            sleep(10);
            printf("child exit.\n");
            _exit(250);
        }
    }
}

/********************************************************
 * signal
 *******************************************************/
#include <signal.h>

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
void sig_catch(int sig, void (*f)())
{
    struct sigaction sa;
    sa.sa_handler = f;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(sig, &sa, (struct sigaction *)0);
}
void sig_uncatch(int sig)
{
    sig_catch(sig, SIG_DFL);
}
void sig_pause(void)
{
    sigset_t ss;
    sigemptyset(&ss);
    sigsuspend(&ss);
}
void sig_ignore(int sig)
{
    sig_catch(sig, SIG_IGN);
}
