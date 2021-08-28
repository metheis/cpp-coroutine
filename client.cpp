// Client side C/C++ program for sockets
#include <concepts>
#include <coroutine>
#include <exception>
#include <iostream>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#define PORT 8080

// from corodemo.cc
template <typename PromiseType>
struct GetPromise
{
  PromiseType *p_;
  bool await_ready() { return false; } // says yes call await_suspend
  bool await_suspend(std::coroutine_handle<PromiseType> h)
  {
    p_ = &h.promise();
    return false; // says no don't suspend coroutine after all
  }
  PromiseType *await_resume() { return p_; }
};

// from corodemo.cc
struct ReturnObject3
{
  struct promise_type
  {
    unsigned value_;

    ReturnObject3 get_return_object()
    {
      return ReturnObject3{
          .h_ = std::coroutine_handle<promise_type>::from_promise(*this)};
    }
    std::suspend_always initial_suspend() { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void unhandled_exception() {}
  };

  std::coroutine_handle<promise_type> h_;
  operator std::coroutine_handle<promise_type>() const { return h_; }
};

// from corodemo.cc
ReturnObject3
counter()
{
  auto pp = co_await GetPromise<ReturnObject3::promise_type>{};
  printf("hello world\n");
  // for (unsigned i = 0;; ++i) {
  //   pp->value_ = i;
  //   co_await std::suspend_always{};
  // }
}

int main(int argc, char const *argv[])
{
  int sock = 0, valread, ret;
  struct sockaddr_in serv_addr;
  const char *hello = "Hello from client";
  char buffer[1024] = {0};
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    printf("\n Socket creation error \n");
    return -1;
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT);

  // Convert IPv4 and IPv6 addresses from text to binary form
  if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
  {
    printf("\nInvalid address/ Address not supported \n");
    return -1;
  }

  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
  {
    printf("\nConnection Failed \n");
    return -1;
  }
  std::cout << "size of handle: " << sizeof(std::coroutine_handle<ReturnObject3::promise_type>) << std::endl;
  std::coroutine_handle<ReturnObject3::promise_type> h = counter();
  // print return object
  unsigned char *p1 = (unsigned char *)h.address();
  int j;
  std::cout << "original handle: " << std::endl;
  for (j = 0; j < sizeof(std::coroutine_handle<ReturnObject3::promise_type>); j++)
  {
    printf("%02hhX ", p1[j]);
  }
  printf("\n");
  // ReturnObject3::promise_type &promise = h.promise();
  // std::cout << "counter: " << promise.value_ << std::endl;
  if ((ret = write(sock, h.address(), sizeof(std::coroutine_handle<ReturnObject3::promise_type>))) == -1)
  {
    perror("write() error");
  }
  printf("write() wrote %d bytes\n", ret);
  // print out what was sent
  unsigned char *p = (unsigned char *)h.address();
  int i;
  std::cout << "sent handle: " << std::endl;
  for (i = 0; i < sizeof(std::coroutine_handle<ReturnObject3::promise_type>); i++)
  {
    printf("%02hhX ", p[i]);
  }
  printf("\n");
  // h.destroy();
  // printf("routine message sent\n");
  return 0;
}
