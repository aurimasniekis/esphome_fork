#include "socket.h"
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

namespace esphome {
namespace socket {

std::string format_sockaddr(const struct sockaddr_storage &storage) {
  if (storage.ss_family == AF_INET) {
    const struct sockaddr_in *addr = reinterpret_cast<const struct sockaddr_in *>(&storage);
    char buf[INET_ADDRSTRLEN];
    const char *ret = inet_ntop(AF_INET, &addr->sin_addr, buf, sizeof(buf));
    if (ret == NULL)
      return {};
    return std::string{buf};
  } else if (storage.ss_family == AF_INET6) {
    const struct sockaddr_in6 *addr = reinterpret_cast<const struct sockaddr_in6 *>(&storage);
    char buf[INET6_ADDRSTRLEN];
    const char *ret = inet_ntop(AF_INET6, &addr->sin6_addr, buf, sizeof(buf));
    if (ret == NULL)
      return {};
    return std::string{buf};
  }
  return {};
}

class SocketImplSocket : public Socket {
 public:
  SocketImplSocket(int fd) : Socket(), fd_(fd) {}
  ~SocketImplSocket() override {
    if (!closed_) {
      close();
    }
  }
  std::unique_ptr<Socket> accept(struct sockaddr *addr, socklen_t *addrlen) override {
    int fd = ::accept(fd_, addr, addrlen);
    if (fd == -1)
      return {};
    return std::unique_ptr<SocketImplSocket>{new SocketImplSocket(fd)};
  }
  int bind(const struct sockaddr *addr, socklen_t addrlen) override {
    return ::bind(fd_, addr, addrlen);
  }
  int close() override {
    int ret = ::close(fd_);
    closed_ = true;
    return ret;
  }
  int connect(const std::string &address) override {
    // TODO
    return 0;
  }
  int connect(const struct sockaddr *addr, socklen_t addrlen) override {
    return ::connect(fd_, addr, addrlen);
  }
  int shutdown(int how) override {
    return ::shutdown(fd_, how);
  }

  int getpeername(struct sockaddr *addr, socklen_t *addrlen) override {
    return ::getpeername(fd_, addr, addrlen);
  }
  std::string getpeername() override {
    struct sockaddr_storage storage;
    socklen_t len = sizeof(storage);
    int err = this->getpeername((struct sockaddr *) &storage, &len);
    if (err != 0)
      return {};
    return format_sockaddr(storage);
  }
  int getsockname(struct sockaddr *addr, socklen_t *addrlen) override {
    return ::getsockname(fd_, addr, addrlen);
  }
  std::string getsockname() override {
    struct sockaddr_storage storage;
    socklen_t len = sizeof(storage);
    int err = this->getsockname((struct sockaddr *) &storage, &len);
    if (err != 0)
      return {};
    return format_sockaddr(storage);
  }
  int getsockopt(int level, int optname, void *optval, socklen_t *optlen) override {
    return ::getsockopt(fd_, level, optname, optval, optlen);
  }
  int setsockopt(int level, int optname, const void *optval, socklen_t optlen) override {
    return ::setsockopt(fd_, level, optname, optval, optlen);
  }
  int listen(int backlog) override {
    return ::listen(fd_, backlog);
  }
  // virtual ssize_t readv(const struct iovec *iov, int iovcnt) = 0;
  ssize_t read(void *buf, size_t len) override {
    return ::read(fd_, buf, len);
  }
  // virtual ssize_t writev(const struct iovec *iov, int iovcnt) = 0;
  ssize_t write(const void *buf, size_t len) override {
    return ::write(fd_, buf, len);
  }
  int setblocking(bool blocking) override {
    int fl = ::fcntl(fd_, F_GETFL, 0);
    ::fcntl(fd_, F_SETFL, fl | O_NONBLOCK);
    return 0;
  }
 protected:
  int fd_;
  bool closed_ = false;
};

std::unique_ptr<Socket> socket(int domain, int type, int protocol) {
  int ret = ::socket(domain, type, protocol);
  if (ret == -1)
    return nullptr;
  return std::unique_ptr<Socket>{new SocketImplSocket(ret)};
}

}  // namespace socket
}  // namespace esphome