#include <iostream>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <cstdint>
#include <sys/wait.h>
#include <cstdio>
#include <cerrno>
#include <cstdlib>
#include <unistd.h>

void exit_if_error(int rval, const char *msg) {
  if (rval < 0) {
    perror(msg);
    exit(1);
  }
}

uint64_t read_w(pid_t pid, void *addr) {
  errno = 0;
  long rval = ptrace(PTRACE_PEEKTEXT, pid, addr, 0);
  if (rval == -1 && errno != 0) {
    perror("read_w");
    exit(1);
  }
  return static_cast<uint64_t>(rval);
}

void write_w(pid_t pid, void *addr, uint64_t data) {
  exit_if_error(ptrace(PTRACE_POKETEXT, pid, addr, reinterpret_cast<void *>(data)), "write_w");
}

void continue_tracee(pid_t pid) {
  exit_if_error(ptrace(PTRACE_CONT, pid, NULL, NULL), "cont");
}

uint64_t handler = 0;
bool interrupt_disable = false;

void interrupt(pid_t pid) {
  if (handler == 0) {
    std::cout << "please initialize handler before enable interrupt." << std::endl;
    exit(1);
  }

  struct user_regs_struct data;
  exit_if_error(ptrace(PTRACE_GETREGS, pid, NULL, &data), "getregs");

  long rip = data.rip;
  data.rip = handler;
  data.rsp -= sizeof(long);
  
  write_w(pid, (void *)data.rsp, rip);

  exit_if_error(ptrace(PTRACE_SETREGS, pid, NULL, &data), "getregs");
}

void handle_hypercall(pid_t pid, int status) {
  if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP) {
    struct user_regs_struct data;
    exit_if_error(ptrace(PTRACE_GETREGS, pid, NULL, &data), "getregs");
    
    switch(data.rax) {
    case 1:
      // set handler address
      std::cout << "handler address is " << std::hex << data.rbx << "." << std::endl;
      handler = data.rbx;
      break;
    case 2:
      // disable interrupt
      interrupt_disable = true;
      break;
    case 3:
      // enable interrupt
      interrupt_disable = false;
      break;
    default:
      std::cout << "unknown hypercall" << std::endl;
      exit(1);
    }
  } else if (WIFEXITED(status)) {
    exit(0);
  } else if (WIFSIGNALED(status)) {
    exit(1);
  } else {
    // ignore other signals
  }
}

void stop_tracee(pid_t pid) {
  int status;
  exit_if_error(kill(pid, SIGSTOP), "kill");

  while(true) {
    exit_if_error(waitpid(pid, &status, 0), "waitpid");

    if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGSTOP) {
      // the signal has been properly delivered.
      break;
    }

    handle_hypercall(pid, status);

    continue_tracee(pid);
  }
}


int main(int argc, const char **argv) {
  pid_t pid;
  int status;
  if ((pid = fork()) == 0) {
    // run tracee process
    exit_if_error(ptrace(PTRACE_TRACEME, 0, NULL, NULL), "traceme");

    // execl will invoke SIGTRAP and send it to the parent process
    exit_if_error(execl("worker", "worker", NULL), "execl");
  }

  // parent process (tracer)

  exit_if_error(pid, "fork");

  exit_if_error(waitpid(pid, &status, 0), "waitpid");
  
  if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP) {
    // must be SIGTRAP due to execl
  } else {
    std::cout << "unknown error (no SIGTRAP)" << std::endl;
    exit(1);
  }
  continue_tracee(pid);

  static const int kTimeSlice = 30;
  int timeout = kTimeSlice;
  while(true) {
    if (waitpid(pid, &status, WNOHANG) != 0) {
      // handle signals if tracee has been stopped.
      handle_hypercall(pid, status);
      continue_tracee(pid);
    }

    usleep(100 * 1000); // 100ms sleep

    if (timeout > 0) {
      timeout--;
    }
    
    if (timeout == 0 && !interrupt_disable) {
      stop_tracee(pid);
      interrupt(pid);
      continue_tracee(pid);
      timeout = kTimeSlice;
    }
  }
}
