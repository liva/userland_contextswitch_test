#include <iostream>
#include <unistd.h>
#include <cstdint>

struct context {
  void *rip;
  uint8_t *stack;
} next_worker;

uint8_t worker2_stack[4096];

extern "C" void handler();

volatile int number = 0;

void worker1() {
  for(int i = 0; i < 15; i++) {
    asm volatile("int $3;"::"a"(2));
    std::cout << number << std::endl;
    asm volatile("int $3;"::"a"(3));
    number++;
    sleep(1);
    asm volatile("":::"memory");
  }
  exit(0);
}

void worker2() {
  for(int i = 0; i < 15; i++) {
    asm volatile("int $3;"::"a"(2));
    std::cout << number << std::endl;
    asm volatile("int $3;"::"a"(3));
    number+=100;
    sleep(1);
    asm volatile("":::"memory");
  }
  exit(0);
}

int main(int argc, const char **argv) {
  asm volatile("int $3;"::"a"(1), "b"(handler));
  next_worker.rip = reinterpret_cast<void *>(worker2);
  next_worker.stack = worker2_stack + sizeof(worker2_stack);
  worker1();
  return 0;
}
