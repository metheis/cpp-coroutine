// Server side C/C++ program to demonstrate Socket programming
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <concepts>
#include <coroutine>
#include <exception>
#include <iostream>
#define PORT 8080

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
	int server_fd, new_socket, valread;
	struct sockaddr_in address;
	int opt = 1;
	int addrlen = sizeof(address);
	std::coroutine_handle<ReturnObject3::promise_type> h;
	char buffer[1024] = {0};

	// Creating socket file descriptor
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
	{
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	// Forcefully attaching socket to the port 8080
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
				   &opt, sizeof(opt)))
	{
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);

	std::coroutine_handle<ReturnObject3::promise_type> h2 = counter();
	// print return object
	unsigned char *p1 = (unsigned char *)h2.address();
	int j;
	std::cout << "server instantiated handle: " << std::endl;
	for (j = 0; j < sizeof(std::coroutine_handle<ReturnObject3::promise_type>); j++)
	{
		printf("%02hhX ", p1[j]);
	}
	printf("\n");

	// Forcefully attaching socket to the port 8080
	if (bind(server_fd, (struct sockaddr *)&address,
			 sizeof(address)) < 0)
	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
	if (listen(server_fd, 3) < 0)
	{
		perror("listen");
		exit(EXIT_FAILURE);
	}
	if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
							 (socklen_t *)&addrlen)) < 0)
	{
		perror("accept");
		exit(EXIT_FAILURE);
	}
	valread = read(new_socket, buffer, 1024);
	printf("read() read %d bytes\n", valread);
	if (sizeof(std::coroutine_handle<ReturnObject3::promise_type>) == valread)
	{

		h = std::coroutine_handle<ReturnObject3::promise_type>::from_address(&buffer);
		ReturnObject3::promise_type &promise = h.promise();
		// print out what was received
		unsigned char *p = (unsigned char *)h.address();
		int i;
		std::cout << "server received handle: " << std::endl;
		for (i = 0; i < sizeof(std::coroutine_handle<ReturnObject3::promise_type>); i++)
		{
			printf("%02hhX ", p[i]);
		}
		printf("\n");
		//h.resume();
		// std::cout << "counter: " << promise.value_ << std::endl;
		// h();
		// ReturnObject3::promise_type &promise = h.promise();
		// std::cout << "counter: " << promise.value_ << std::endl;
		// h();
		// std::cout << "counter: " << promise.value_ << std::endl;
		// h.destroy();
	}
	else
	{
		printf("read() did not read the right number of bytes");
	}
	return 0;
}
