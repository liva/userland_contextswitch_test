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

void interrupt(pid_t pid, uint64_t handler) {
  struct user_regs_struct data;
  exit_if_error(ptrace(PTRACE_GETREGS, pid, NULL, &data), "getregs");

  long rip = data.rip;
  data.rip = handler;
  data.rsp -= sizeof(long);
  
  write_w(pid, (void *)data.rsp, rip);

  exit_if_error(ptrace(PTRACE_SETREGS, pid, NULL, &data), "getregs");
}

void stop_tracee(pid_t pid) {
  int status;
  exit_if_error(kill(pid, SIGSTOP), "kill");

  while(true) {
    exit_if_error(waitpid(pid, &status, 0), "waitpid");
    
    if (WIFSTOPPED(status)) {
      if (WSTOPSIG(status) == SIGSTOP) {
	// the signal has been properly delivered.
	break;
      }
    } else {
      std::cout << "could not stop tracee process." << std::endl;
      exit(1);
    }

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
    exit_if_error(execl("test", "test", NULL), "execl");
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

  exit_if_error(waitpid(pid, &status, 0), "waitpid");

  uint64_t handler;
  if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP) {
    struct user_regs_struct data;
    exit_if_error(ptrace(PTRACE_GETREGS, pid, NULL, &data), "getregs");
    
    std::cout << "handler address is " << std::hex << data.rax << "." << std::endl;
    handler = data.rax;
  } else {
    std::cout << "please set handler address" << std::endl;
    exit(1);
  }

  continue_tracee(pid);

  // interrupt 5 times with 4 seconds interval
  for(int i = 0; i < 5; i++) {
    // to ignore all signals and continue, invoke PTRACE_CONT periodically
    for(int j = 0; j < 4; j++) {
      if (waitpid(pid, &status, WNOHANG) != 0) {
	continue_tracee(pid);
      }
      sleep(1);
    }
    
    stop_tracee(pid);

    interrupt(pid, handler);
    continue_tracee(pid);
  }

  while(true) {
    exit_if_error(waitpid(pid, &status, 0), "waitpid");
    if (WIFEXITED(status)) {
      return 0;
    } else if (WIFSIGNALED(status)) {
      return 1;
    } else {
      continue_tracee(pid);
    }
  }
}
