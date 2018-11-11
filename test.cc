#include <iostream>
#include <unistd.h>

extern "C" {
  void handler();
  volatile int number = 0;
}

void show() {
  for(int i = 0; i < 30; i++) {
    std::cout << number << std::endl;
    sleep(1);
    asm volatile("":::"memory");
  }
}

int main(int argc, const char **argv) {
  asm volatile("int $3;"::"a"(handler));
  show();
  return 0;
}
