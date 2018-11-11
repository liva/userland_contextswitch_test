#include <iostream>
#include <unistd.h>
#include <cstdint>

void setup_handler(void (*func)()) {
  asm volatile("int $3;"::"a"(1), "b"(func));
}

void enable_interrupt() {
  asm volatile("int $3;"::"a"(3));
}

void disable_interrupt() {
  asm volatile("int $3;"::"a"(2));
}

volatile int number = 0;

void worker1() {
  for(int i = 0; i < 15; i++) {
    disable_interrupt();
    std::cout << number << std::endl;
    enable_interrupt();
    number++;
    sleep(1);
    asm volatile("":::"memory");
  }
  exit(0);
}

void worker2() {
  for(int i = 0; i < 15; i++) {
    disable_interrupt();
    std::cout << number << std::endl;
    enable_interrupt();
    number+=100;
    sleep(1);
    asm volatile("":::"memory");
  }
  exit(0);
}

struct context {
  void *rip;
  uint8_t *stack;
} next_worker;
uint8_t worker2_stack[4096];

extern "C" void handler();

int main(int argc, const char **argv) {
  setup_handler(handler);
  enable_interrupt();
  next_worker.rip = reinterpret_cast<void *>(worker2);
  next_worker.stack = worker2_stack + sizeof(worker2_stack);
  worker1();
  return 0;
}
