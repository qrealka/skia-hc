/*
 * Copyright 2014 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/ptrace.h>
#include <sys/syscall.h>
#include <sys/user.h>
#include <sys/types.h>
#include <sys/wait.h>

////////////////////////////seccomp_bpf.h///////////////////////////////////////
/*
 * seccomp example for x86 (32-bit and 64-bit) with BPF macros
 *
 * Copyright (c) 2012 The Chromium OS Authors <chromium-os-dev@chromium.org>
 * Authors:
 *  Will Drewry <wad@chromium.org>
 *  Kees Cook <keescook@chromium.org>
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef _SECCOMP_BPF_H_
#define _SECCOMP_BPF_H_

#define _GNU_SOURCE 1
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include <sys/prctl.h>
#ifndef PR_SET_NO_NEW_PRIVS
# define PR_SET_NO_NEW_PRIVS 38
#endif

#include <linux/unistd.h>
#include <linux/audit.h>
#include <linux/filter.h>
#include <linux/seccomp.h>

#define syscall_nr (offsetof(struct seccomp_data, nr))
#define arch_nr (offsetof(struct seccomp_data, arch))
#define arg_offset_0 (offsetof(struct seccomp_data, args[0]))
#define arg_offset_1 (offsetof(struct seccomp_data, args[1]))
#define arg_offset_2 (offsetof(struct seccomp_data, args[2]))
#define arg_offset_3 (offsetof(struct seccomp_data, args[3]))
#define arg_offset_4 (offsetof(struct seccomp_data, args[4]))
#define arg_offset_5 (offsetof(struct seccomp_data, args[5]))

#if defined(__x86_64__)
#define SECCOMP_REG(_ctx, _reg) ((_ctx)->uc_mcontext.gregs[(_reg)])
#define SECCOMP_RESULT(_ctx)    SECCOMP_REG(_ctx, REG_RAX)
#define SECCOMP_SYSCALL(_ctx)   SECCOMP_REG(_ctx, REG_RAX)
#define SECCOMP_IP(_ctx)        SECCOMP_REG(_ctx, REG_RIP)
#define SECCOMP_PARM1(_ctx)     SECCOMP_REG(_ctx, REG_RDI)
#define SECCOMP_PARM2(_ctx)     SECCOMP_REG(_ctx, REG_RSI)
#define SECCOMP_PARM3(_ctx)     SECCOMP_REG(_ctx, REG_RDX)
#define SECCOMP_PARM4(_ctx)     SECCOMP_REG(_ctx, REG_R10)
#define SECCOMP_PARM5(_ctx)     SECCOMP_REG(_ctx, REG_R8)
#define SECCOMP_PARM6(_ctx)     SECCOMP_REG(_ctx, REG_R9)
# define REG_SYSCALL  REG_RAX
# define ARCH_NR  AUDIT_ARCH_X86_64
#else
# warning "Platform does not support seccomp filter yet"
# define REG_SYSCALL  0
# define ARCH_NR  0
#endif

#define VALIDATE_ARCHITECTURE \
  BPF_STMT(BPF_LD+BPF_W+BPF_ABS, arch_nr), \
  BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, ARCH_NR, 1, 0), \
  BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_KILL)

#define EXAMINE_SYSCALL \
  BPF_STMT(BPF_LD+BPF_W+BPF_ABS, syscall_nr)

#define ALLOW_SYSCALL(name) \
  BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, __NR_##name, 0, 1), \
  BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_ALLOW)

#define TRACE_OPENS_FOR_READS_ONLY(name, arg_index) \
  BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, __NR_##name, 0, 5), \
  BPF_STMT(BPF_LD+BPF_W+BPF_ABS, arg_offset_##arg_index), \
  BPF_STMT(BPF_ALU+BPF_AND+BPF_K, O_ACCMODE), \
  BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, O_RDONLY, 0, 1), \
  BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_TRACE), \
  BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_KILL)

#define TRACE_SYSCALL(name) \
  BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, __NR_##name, 0, 1), \
  BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_TRACE)

#define KILL_PROCESS \
  BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_KILL)

#define ALLOW_ALL \
  BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_ALLOW)

#define TRACE_ALL \
  BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_TRACE)

#endif /* _SECCOMP_BPF_H_ */

////////////////////////////////////////////////////////////////////////////////

#include <iostream>

using namespace std;

static bool install_syscall_filter() {
    struct sock_filter filter[] = {
        VALIDATE_ARCHITECTURE,
        /* Grab the system call number. */
        EXAMINE_SYSCALL,
        /* List allowed syscalls. */


        // ALLOW_SYSCALL(accept),
        // ALLOW_SYSCALL(accept4),
        // ALLOW_SYSCALL(access),
        // ALLOW_SYSCALL(acct),
        // ALLOW_SYSCALL(add_key),
        // ALLOW_SYSCALL(adjtimex),
        // ALLOW_SYSCALL(afs_syscall),
        // ALLOW_SYSCALL(alarm),
        // ALLOW_SYSCALL(arch_prctl),
        // ALLOW_SYSCALL(bind),
        // ALLOW_SYSCALL(brk),
        // ALLOW_SYSCALL(capget),
        // ALLOW_SYSCALL(capset),
        // ALLOW_SYSCALL(chdir),
        // ALLOW_SYSCALL(chmod),
        // ALLOW_SYSCALL(chown),
        // ALLOW_SYSCALL(chroot),
        // ALLOW_SYSCALL(clock_adjtime),
        // ALLOW_SYSCALL(clock_getres),
        ALLOW_SYSCALL(clock_gettime),
        // ALLOW_SYSCALL(clock_nanosleep),
        // ALLOW_SYSCALL(clock_settime),
        // ALLOW_SYSCALL(clone),
        // ALLOW_SYSCALL(close),
        // ALLOW_SYSCALL(connect),
        // ALLOW_SYSCALL(creat),
        // ALLOW_SYSCALL(create_module),
        // ALLOW_SYSCALL(delete_module),
        // ALLOW_SYSCALL(dup),
        // ALLOW_SYSCALL(dup2),
        // ALLOW_SYSCALL(dup3),
        // ALLOW_SYSCALL(epoll_create),
        // ALLOW_SYSCALL(epoll_create1),
        // ALLOW_SYSCALL(epoll_ctl),
        // ALLOW_SYSCALL(epoll_ctl_old),
        // ALLOW_SYSCALL(epoll_pwait),
        // ALLOW_SYSCALL(epoll_wait),
        // ALLOW_SYSCALL(epoll_wait_old),
        // ALLOW_SYSCALL(eventfd),
        // ALLOW_SYSCALL(eventfd2),
        // ALLOW_SYSCALL(execve),
        ALLOW_SYSCALL(exit),
        // ALLOW_SYSCALL(exit_group),
        // ALLOW_SYSCALL(faccessat),
        // ALLOW_SYSCALL(fadvise64),
        // ALLOW_SYSCALL(fallocate),
        // ALLOW_SYSCALL(fanotify_init),
        // ALLOW_SYSCALL(fanotify_mark),
        // ALLOW_SYSCALL(fchdir),
        // ALLOW_SYSCALL(fchmod),
        // ALLOW_SYSCALL(fchmodat),
        // ALLOW_SYSCALL(fchown),
        // ALLOW_SYSCALL(fchownat),
        // ALLOW_SYSCALL(fcntl),
        // ALLOW_SYSCALL(fdatasync),
        // ALLOW_SYSCALL(fgetxattr),
        // ALLOW_SYSCALL(finit_module),
        // ALLOW_SYSCALL(flistxattr),
        // ALLOW_SYSCALL(flock),
        // ALLOW_SYSCALL(fork),
        // ALLOW_SYSCALL(fremovexattr),
        // ALLOW_SYSCALL(fsetxattr),
        // ALLOW_SYSCALL(fstat),
        // ALLOW_SYSCALL(fstatfs),
        // ALLOW_SYSCALL(fsync),
        // ALLOW_SYSCALL(ftruncate),
        // ALLOW_SYSCALL(futex),
        ALLOW_SYSCALL(futimesat),
        ALLOW_SYSCALL(get_kernel_syms),
        ALLOW_SYSCALL(get_mempolicy),
        ALLOW_SYSCALL(get_robust_list),
        ALLOW_SYSCALL(get_thread_area),
        ALLOW_SYSCALL(getcpu),
        ALLOW_SYSCALL(getcwd),
        // ALLOW_SYSCALL(getdents),
        // ALLOW_SYSCALL(getdents64),
        // ALLOW_SYSCALL(getegid),
        // ALLOW_SYSCALL(geteuid),
        // ALLOW_SYSCALL(getgid),
        ALLOW_SYSCALL(getgroups),
        ALLOW_SYSCALL(getitimer),
        ALLOW_SYSCALL(getpeername),
        ALLOW_SYSCALL(getpgid),
        ALLOW_SYSCALL(getpgrp),
        ALLOW_SYSCALL(getpid),
        ALLOW_SYSCALL(getpmsg),
        ALLOW_SYSCALL(getppid),
        ALLOW_SYSCALL(getpriority),
        ALLOW_SYSCALL(getresgid),
        ALLOW_SYSCALL(getresuid),
        ALLOW_SYSCALL(getrlimit),
        ALLOW_SYSCALL(getrusage),
        ALLOW_SYSCALL(getsid),
        ALLOW_SYSCALL(getsockname),
        ALLOW_SYSCALL(getsockopt),
        ALLOW_SYSCALL(gettid),
        ALLOW_SYSCALL(gettimeofday),
        // ALLOW_SYSCALL(getuid),
        // ALLOW_SYSCALL(getxattr),
        // ALLOW_SYSCALL(init_module),
        // ALLOW_SYSCALL(inotify_add_watch),
        // ALLOW_SYSCALL(inotify_init),
        // ALLOW_SYSCALL(inotify_init1),
        // ALLOW_SYSCALL(inotify_rm_watch),
        // ALLOW_SYSCALL(io_cancel),
        // ALLOW_SYSCALL(io_destroy),
        // ALLOW_SYSCALL(io_getevents),
        // ALLOW_SYSCALL(io_setup),
        // ALLOW_SYSCALL(io_submit),
        // ALLOW_SYSCALL(ioctl),
        // ALLOW_SYSCALL(ioperm),
        // ALLOW_SYSCALL(iopl),
        // ALLOW_SYSCALL(ioprio_get),
        // ALLOW_SYSCALL(ioprio_set),
        // ALLOW_SYSCALL(kcmp),
        // ALLOW_SYSCALL(kexec_load),
        // ALLOW_SYSCALL(keyctl),
        // ALLOW_SYSCALL(kill),
        // ALLOW_SYSCALL(lchown),
        // ALLOW_SYSCALL(lgetxattr),
        // ALLOW_SYSCALL(link),
        // ALLOW_SYSCALL(linkat),
        // ALLOW_SYSCALL(listen),
        // ALLOW_SYSCALL(listxattr),
        // ALLOW_SYSCALL(llistxattr),
        // ALLOW_SYSCALL(lookup_dcookie),
        // ALLOW_SYSCALL(lremovexattr),
        // ALLOW_SYSCALL(lseek),
        // ALLOW_SYSCALL(lsetxattr),
        // ALLOW_SYSCALL(lstat),
        // ALLOW_SYSCALL(madvise),
        // ALLOW_SYSCALL(mbind),
        // ALLOW_SYSCALL(migrate_pages),
        // ALLOW_SYSCALL(mincore),
        // ALLOW_SYSCALL(mkdir),
        // ALLOW_SYSCALL(mkdirat),
        // ALLOW_SYSCALL(mknod),
        // ALLOW_SYSCALL(mknodat),
        // ALLOW_SYSCALL(mlock),
        // ALLOW_SYSCALL(mlockall),
        // ALLOW_SYSCALL(mmap),
        // ALLOW_SYSCALL(modify_ldt),
        // ALLOW_SYSCALL(mount),
        // ALLOW_SYSCALL(move_pages),
        // ALLOW_SYSCALL(mprotect),
        // ALLOW_SYSCALL(mq_getsetattr),
        // ALLOW_SYSCALL(mq_notify),
        // ALLOW_SYSCALL(mq_open),
        // ALLOW_SYSCALL(mq_timedreceive),
        // ALLOW_SYSCALL(mq_timedsend),
        // ALLOW_SYSCALL(mq_unlink),
        ALLOW_SYSCALL(mremap),
        // ALLOW_SYSCALL(msgctl),
        // ALLOW_SYSCALL(msgget),
        // ALLOW_SYSCALL(msgrcv),
        // ALLOW_SYSCALL(msgsnd),
        // ALLOW_SYSCALL(msync),
        // ALLOW_SYSCALL(munlock),
        // ALLOW_SYSCALL(munlockall),
        // ALLOW_SYSCALL(munmap),
        // ALLOW_SYSCALL(name_to_handle_at),
        // ALLOW_SYSCALL(nanosleep),
        // ALLOW_SYSCALL(newfstatat),
        // ALLOW_SYSCALL(nfsservctl),
        ALLOW_SYSCALL(open),
        // ALLOW_SYSCALL(open_by_handle_at),
        // ALLOW_SYSCALL(openat),
        // ALLOW_SYSCALL(pause),
        // ALLOW_SYSCALL(perf_event_open),
        // ALLOW_SYSCALL(personality),
        // ALLOW_SYSCALL(pipe),
        // ALLOW_SYSCALL(pipe2),
        // ALLOW_SYSCALL(pivot_root),
        // ALLOW_SYSCALL(poll),
        // ALLOW_SYSCALL(ppoll),
        // ALLOW_SYSCALL(prctl),
        // ALLOW_SYSCALL(pread64),
        // ALLOW_SYSCALL(preadv),
        // ALLOW_SYSCALL(prlimit64),
        // ALLOW_SYSCALL(process_vm_readv),
        // ALLOW_SYSCALL(process_vm_writev),
        // ALLOW_SYSCALL(pselect6),
        // ALLOW_SYSCALL(ptrace),
        // ALLOW_SYSCALL(putpmsg),
        // ALLOW_SYSCALL(pwrite64),
        // ALLOW_SYSCALL(pwritev),
        // ALLOW_SYSCALL(query_module),
        // ALLOW_SYSCALL(quotactl),
        // ALLOW_SYSCALL(read),
        // ALLOW_SYSCALL(readahead),
        ALLOW_SYSCALL(readlink),
        // ALLOW_SYSCALL(readlinkat),
        // ALLOW_SYSCALL(readv),
        // ALLOW_SYSCALL(reboot),
        // ALLOW_SYSCALL(recvfrom),
        // ALLOW_SYSCALL(recvmmsg),
        // ALLOW_SYSCALL(recvmsg),
        // ALLOW_SYSCALL(remap_file_pages),
        // ALLOW_SYSCALL(removexattr),
        // ALLOW_SYSCALL(rename),
        // ALLOW_SYSCALL(renameat),
        // ALLOW_SYSCALL(renameat2),
        // ALLOW_SYSCALL(request_key),
        // ALLOW_SYSCALL(restart_syscall),
        // ALLOW_SYSCALL(rmdir),
        // ALLOW_SYSCALL(rt_sigaction),
        // ALLOW_SYSCALL(rt_sigpending),
        // ALLOW_SYSCALL(rt_sigprocmask),
        // ALLOW_SYSCALL(rt_sigqueueinfo),
        // ALLOW_SYSCALL(rt_sigreturn),
        // ALLOW_SYSCALL(rt_sigsuspend),
        // ALLOW_SYSCALL(rt_sigtimedwait),
        // ALLOW_SYSCALL(rt_tgsigqueueinfo),
        // ALLOW_SYSCALL(sched_get_priority_max),
        // ALLOW_SYSCALL(sched_get_priority_min),
        // ALLOW_SYSCALL(sched_getaffinity),
        // ALLOW_SYSCALL(sched_getattr),
        // ALLOW_SYSCALL(sched_getparam),
        // ALLOW_SYSCALL(sched_getscheduler),
        // ALLOW_SYSCALL(sched_rr_get_interval),
        // ALLOW_SYSCALL(sched_setaffinity),
        // ALLOW_SYSCALL(sched_setattr),
        // ALLOW_SYSCALL(sched_setparam),
        // ALLOW_SYSCALL(sched_setscheduler),
        // ALLOW_SYSCALL(sched_yield),
        // ALLOW_SYSCALL(seccomp),
        // ALLOW_SYSCALL(security),
        // ALLOW_SYSCALL(select),
        // ALLOW_SYSCALL(semctl),
        // ALLOW_SYSCALL(semget),
        // ALLOW_SYSCALL(semop),
        // ALLOW_SYSCALL(semtimedop),
        // ALLOW_SYSCALL(sendfile),
        // ALLOW_SYSCALL(sendmmsg),
        // ALLOW_SYSCALL(sendmsg),
        // ALLOW_SYSCALL(sendto),
        // ALLOW_SYSCALL(set_mempolicy),
        // ALLOW_SYSCALL(set_robust_list),
        // ALLOW_SYSCALL(set_thread_area),
        // ALLOW_SYSCALL(set_tid_address),
        // ALLOW_SYSCALL(setdomainname),
        // ALLOW_SYSCALL(setfsgid),
        // ALLOW_SYSCALL(setfsuid),
        // ALLOW_SYSCALL(setgid),
        // ALLOW_SYSCALL(setgroups),
        // ALLOW_SYSCALL(sethostname),
        // ALLOW_SYSCALL(setitimer),
        // ALLOW_SYSCALL(setns),
        // ALLOW_SYSCALL(setpgid),
        // ALLOW_SYSCALL(setpriority),
        // ALLOW_SYSCALL(setregid),
        // ALLOW_SYSCALL(setresgid),
        // ALLOW_SYSCALL(setresuid),
        // ALLOW_SYSCALL(setreuid),
        // ALLOW_SYSCALL(setrlimit),
        // ALLOW_SYSCALL(setsid),
        // ALLOW_SYSCALL(setsockopt),
        // ALLOW_SYSCALL(settimeofday),
        // ALLOW_SYSCALL(setuid),
        // ALLOW_SYSCALL(setxattr),
        // ALLOW_SYSCALL(shmat),
        // ALLOW_SYSCALL(shmctl),
        // ALLOW_SYSCALL(shmdt),
        // ALLOW_SYSCALL(shmget),
        // ALLOW_SYSCALL(shutdown),
        // ALLOW_SYSCALL(sigaltstack),
        // ALLOW_SYSCALL(signalfd),
        // ALLOW_SYSCALL(signalfd4),
        // ALLOW_SYSCALL(socket),
        // ALLOW_SYSCALL(socketpair),
        // ALLOW_SYSCALL(splice),
        // ALLOW_SYSCALL(stat),
        // ALLOW_SYSCALL(statfs),
        // ALLOW_SYSCALL(swapoff),
        // ALLOW_SYSCALL(swapon),
        // ALLOW_SYSCALL(symlink),
        // ALLOW_SYSCALL(symlinkat),
        // ALLOW_SYSCALL(sync),
        // ALLOW_SYSCALL(sync_file_range),
        // ALLOW_SYSCALL(syncfs),
        // ALLOW_SYSCALL(sysfs),
        // ALLOW_SYSCALL(sysinfo),
        // ALLOW_SYSCALL(syslog),
        // ALLOW_SYSCALL(tee),
        // ALLOW_SYSCALL(tgkill),
        // ALLOW_SYSCALL(time),
        // ALLOW_SYSCALL(timer_create),
        // ALLOW_SYSCALL(timer_delete),
        // ALLOW_SYSCALL(timer_getoverrun),
        // ALLOW_SYSCALL(timer_gettime),
        // ALLOW_SYSCALL(timer_settime),
        // ALLOW_SYSCALL(timerfd_create),
        // ALLOW_SYSCALL(timerfd_gettime),
        // ALLOW_SYSCALL(timerfd_settime),
        // ALLOW_SYSCALL(times),
        // ALLOW_SYSCALL(tkill),
        // ALLOW_SYSCALL(truncate),
        // ALLOW_SYSCALL(tuxcall),
        // ALLOW_SYSCALL(umask),
        // ALLOW_SYSCALL(umount2),
        // ALLOW_SYSCALL(uname),
        // ALLOW_SYSCALL(unlink),
        // ALLOW_SYSCALL(unlinkat),
        // ALLOW_SYSCALL(unshare),
        // ALLOW_SYSCALL(uselib),
        // ALLOW_SYSCALL(ustat),
        // ALLOW_SYSCALL(utime),
        // ALLOW_SYSCALL(utimensat),
        // ALLOW_SYSCALL(utimes),
        // ALLOW_SYSCALL(vfork),
        // ALLOW_SYSCALL(vhangup),
        // ALLOW_SYSCALL(vmsplice),
        // ALLOW_SYSCALL(vserver),
        // ALLOW_SYSCALL(wait4),
        // ALLOW_SYSCALL(waitid),
        // ALLOW_SYSCALL(write),
        // ALLOW_SYSCALL(writev),

        ALLOW_SYSCALL(exit_group),
        ALLOW_SYSCALL(exit),
        ALLOW_SYSCALL(stat),
        ALLOW_SYSCALL(fstat),
        ALLOW_SYSCALL(read),
        ALLOW_SYSCALL(write),
        ALLOW_SYSCALL(getdents),
        ALLOW_SYSCALL(close),
        ALLOW_SYSCALL(mmap),
        ALLOW_SYSCALL(mprotect),
        ALLOW_SYSCALL(munmap),
        ALLOW_SYSCALL(brk),
        ALLOW_SYSCALL(futex),
        ALLOW_SYSCALL(lseek),
        ALLOW_SYSCALL(set_tid_address),
        ALLOW_SYSCALL(set_robust_list),
        ALLOW_SYSCALL(rt_sigaction),
        ALLOW_SYSCALL(rt_sigprocmask),
        ALLOW_SYSCALL(getrlimit),
        ALLOW_SYSCALL(arch_prctl),
        ALLOW_SYSCALL(access),
        TRACE_SYSCALL(execve),
        TRACE_OPENS_FOR_READS_ONLY(open, 1),
        TRACE_OPENS_FOR_READS_ONLY(openat, 2),
        // TRACE_ALL,
        KILL_PROCESS,
    };
    struct sock_fprog prog = {
        sizeof(filter)/sizeof(filter[0]),
        filter,
    };

    // Lock down the app so that it can't get new privs, such as setuid.
    // Calling this is a requirement for an unprivileged process to use mode
    // 2 seccomp filters, ala SECCOMP_MODE_FILTER, otherwise we'd have to be
    // root.
    if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0)) {
        perror("prctl(NO_NEW_PRIVS)");
        goto failed;
    }
    // Now call seccomp and restrict the system calls that can be made to only
    // the ones in the provided filter list.
    if (prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &prog)) {
        perror("prctl(SECCOMP)");
        goto failed;
    }
    return true;

failed:
    if (errno == EINVAL) {
        fprintf(stderr, "SECCOMP_FILTER is not available. :(\n");
    }
    return false;
}

static void setLimits() {
     struct rlimit n;

     // Limit to 5 seconds of CPU.
     n.rlim_cur = 5;
     n.rlim_max = 5;
     if (setrlimit(RLIMIT_CPU, &n)) {
         perror("setrlimit(RLIMIT_CPU)");
     }

     // Limit to 150M of Address space.
     n.rlim_cur = 150000000;
     n.rlim_max = 150000000;
     if (setrlimit(RLIMIT_AS, &n)) {
         perror("setrlimit(RLIMIT_CPU)");
     }
 }


int do_child(int argc, char **argv) {

    char *args[argc+1];

    memcpy(args, argv, argc * sizeof(char *));
    args[argc] = NULL;

    if (ptrace(PTRACE_TRACEME, 0, 0, 0)) {
        perror("ptrace");
        exit(-1);
    }
    kill(getpid(), SIGSTOP);

    setLimits();
    if (!install_syscall_filter()) {
        return -1;
    }

    (void)execvp(args[0], args);
    // if execvp returns, we couldn't run the child.  Probably
    // because the compile failed.  Let's kill ourselves so the
    // parent sees the signal and exits appropriately.
    kill(getpid(), SIGKILL);
    return -1;
}

// read_string copies a null-terminated string out
// of the child's address space, one character at a time.
// It allocates memory and returns it to the caller;
// it is the caller's responsibility to free it.
char *read_string(pid_t child, unsigned long addr) {
#define INITIAL_ALLOCATION 4096
    char *val = (char *) malloc(INITIAL_ALLOCATION);
    size_t allocated = INITIAL_ALLOCATION;
    size_t read = 0;
    unsigned long tmp;

    while (1) {
        if (read + sizeof tmp > allocated) {
            allocated *= 2;
            val = (char *) realloc(val, allocated);
        }

        tmp = ptrace(PTRACE_PEEKDATA, child, addr + read);
        if (errno != 0) {
            val[read] = 0;
            break;
        }
        memcpy(val + read, &tmp, sizeof tmp);
        if (memchr(&tmp, 0, sizeof tmp) != NULL) {
            break;
        }
        read += sizeof tmp;
    }
    return val;
}


int do_trace(pid_t child, char *allowed_exec) {
    int status;
    waitpid(child, &status, 0);
    ptrace(PTRACE_SETOPTIONS, child, 0, PTRACE_O_TRACEEXEC | PTRACE_O_TRACESECCOMP);
    ptrace(PTRACE_CONT, child, 0, 0);

#define CHILD_FAIL(message) \
    perror(message); \
    kill(child, SIGKILL); \
    exit(-1)

    while(1) {
        waitpid(child, &status, 0);
        if (WIFEXITED(status)) {
            return 0;
        }
        if (WIFSIGNALED(status)) {
            fprintf(stderr, "TERMSIG = %d = %s\n", WTERMSIG(status),
                    strsignal(WTERMSIG(status)));
            return 1;
        }

        if (status>>8 == (SIGTRAP | (PTRACE_EVENT_SECCOMP<<8))) {
            struct user_regs_struct regs;
            if(ptrace(PTRACE_GETREGS, child, NULL, &regs)) {
                  perror("The child failed...");
                  exit(-1);
            }

            int syscall = regs.orig_rax;
            if (syscall == SYS_execve) {
                char *name = read_string( child, regs.rdi );
                if (strcmp(name, allowed_exec)) {
                    fprintf(stderr, "\n%s\n", name);
                    fprintf(stderr, "\n%s\n", allowed_exec);
                    CHILD_FAIL( "Invalid exec." );
                }
                free(name);
            } else if (syscall == SYS_open) {
                char *name = read_string( child, regs.rdi );
                if (NULL != strstr(name, "..")) {
                    CHILD_FAIL( "No relative paths..." );
                }
                int flags = regs.rsi;
                if (O_RDONLY != (flags & O_ACCMODE)) {
                            CHILD_FAIL( "No writing to files..." );
                }
                const char *allowed_prefixes[] = {
                    "./",
                    "/usr/share/fonts",
                    "/etc/ld.so.cache",
                    "/lib/",
                    "/usr/lib/",
                    "skia.conf",
                    "/skia_build/inout/"
                };
                bool okay = false;
                unsigned int N = sizeof(allowed_prefixes) / sizeof(allowed_prefixes[0]);
                for (unsigned int i = 0 ; i < N ; i++) {
                    if (!strncmp(allowed_prefixes[i], name,
                                 strlen(allowed_prefixes[i]))) {
                        okay = true;
                        break;
                    }
                }
                if (!okay) {
                    fprintf(stderr, "\n%s\n", name);
                    CHILD_FAIL( "Invalid open." );
                }
                free(name);
            } else if (syscall == SYS_openat) {
                char *name = read_string( child, regs.rsi );
                if (NULL != strstr(name, "..")) {
                    CHILD_FAIL( "No relative paths..." );
                }
                int flags = regs.rdx;
                if (O_RDONLY != (flags & O_ACCMODE)) {
                            CHILD_FAIL( "No writing to files..." );
                }
                if (strncmp(name, "/usr/share/fonts", strlen("/usr/share/fonts"))) {
                    CHILD_FAIL( "Invalid openat." );
                }
                free(name);
            } else {
                // this should never happen, but if we're in TRACE_ALL
                // mode for debugging, this lets me print out what system
                // calls are happening unexpectedly.
                cout << "WEIRD SYSTEM CALL: " << syscall << endl;
            }
        }
        ptrace(PTRACE_CONT, child, 0, 0);

    }
    return 0;
}

int main(int argc, char** argv) {
    pid_t child = fork();

    if (child == 0) {
        return do_child(argc-1, argv+1);
    } else {
        return do_trace(child, argv[1]);
    }
}
