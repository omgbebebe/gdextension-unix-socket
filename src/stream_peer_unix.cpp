#include "stream_peer_unix.h"

#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/core/property_info.hpp"
#include <godot_cpp/core/class_db.hpp>

#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace godot;

Error _get_data(void *user, uint8_t *p_buffer, int p_bytes) {
  return ((StreamPeerUnix *)user)->get_data(p_buffer, p_bytes);
}

Error _get_partial_data(void *user, uint8_t *p_buffer, int p_bytes,
                        int *r_received) {
  return ((StreamPeerUnix *)user)
      ->get_partial_data(p_buffer, p_bytes, r_received);
}

Error _put_data(void *user, const uint8_t *p_data, int p_bytes) {
  return ((StreamPeerUnix *)user)->put_data(p_data, p_bytes);
}

Error _put_partial_data(void *user, const uint8_t *p_data, int p_bytes,
                        int *r_sent) {
  return ((StreamPeerUnix *)user)->put_partial_data(p_data, p_bytes, r_sent);
}

int _get_available_bytes(const void *user) {
  return ((StreamPeerUnix *)user)->get_available_bytes();
}

Error StreamPeerUnix::get_data(uint8_t *p_buffer, int p_bytes) {
  ERR_FAIL_COND_V(!is_open(), ERR_UNCONFIGURED);
  ERR_FAIL_COND_V(p_bytes < 0, ERR_INVALID_PARAMETER);
  Error error = OK;
  int result = recv(socketfd, p_buffer, p_bytes, MSG_WAITALL | MSG_NOSIGNAL);
  if (result < 0) {
    error = ERR_FILE_CANT_READ;
  } else if (result == 0 && p_bytes > 0) {
    close();
    error = ERR_FILE_EOF;
  } else if (result != p_bytes) {
    error = FAILED;
  }
  return error;
}

Error StreamPeerUnix::get_partial_data(uint8_t *p_buffer, int p_bytes,
                                       int *r_received) {
  *r_received = 0;
  ERR_FAIL_COND_V(!is_open(), ERR_UNCONFIGURED);
  ERR_FAIL_COND_V(p_bytes < 0, ERR_INVALID_PARAMETER);
  int result = recv(socketfd, p_buffer, p_bytes, MSG_NOSIGNAL);
  if (result < 0) {
    if (errno == EBADF || errno == EPIPE)
      close();
    return ERR_FILE_CANT_READ;
  } else if (result == 0 && p_bytes > 0) {
    close();
    return ERR_FILE_EOF;
  }
  *r_received = result;
  return OK;
}

Error StreamPeerUnix::put_data(const uint8_t *p_data, int p_bytes) {
  ERR_FAIL_COND_V(!is_open(), ERR_UNCONFIGURED);
  Error error = OK;
  int sent = 0;
  while (error == OK && sent < p_bytes) {
    int written;
    error = put_partial_data(p_data, p_bytes - sent, &written);
    p_data += written;
    sent += written;
  }
  return error;
}

Error StreamPeerUnix::put_partial_data(const uint8_t *p_data, int p_bytes,
                                       int *r_sent) {
  *r_sent = 0;
  ERR_FAIL_COND_V(!is_open(), ERR_UNCONFIGURED);
  int result = send(socketfd, p_data, p_bytes, MSG_NOSIGNAL);
  if (result < 0) {
    if (errno == EBADF || errno == EPIPE)
      close();
    return ERR_FILE_CANT_WRITE;
  }
  if (result == 0 && p_bytes > 0) {
    close();
    return ERR_FILE_EOF;
  }
  *r_sent = result;
  return OK;
}

int StreamPeerUnix::get_available_bytes() {
  ERR_FAIL_COND_V(!is_open(), -1);
  unsigned long available;
  ioctl(socketfd, FIONREAD, &available);
  return available;
}

int StreamPeerUnix::open(const String path) {
  ERR_FAIL_COND_V(is_open(), ERR_ALREADY_IN_USE);

  socketfd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (socketfd < 0) {
    return ERR_CANT_CREATE;
  }

  const char *path_string = path.ascii().get_data();
  server_address.sun_family = AF_UNIX;
  strcpy(server_address.sun_path, path_string);

  if (::connect(socketfd, (struct sockaddr *)&server_address,
                sizeof(server_address)) < 0) {
    socketfd = -1;
    return ERR_CANT_CONNECT;
  }

  if (!blocking)
    fcntl(socketfd, F_SETFL, O_NONBLOCK);

  this->path = path;

  return OK;
}

String StreamPeerUnix::get_path() { return path; }

bool StreamPeerUnix::is_open() { return socketfd >= 0; }

void StreamPeerUnix::close() {
  if (is_open()) {
    ::close(socketfd);
    server_address = {};
    socketfd = -1;
    path = String();
  }
}

void StreamPeerUnix::set_blocking_mode(bool value) {
  ERR_FAIL_COND(is_open());
  blocking = value;
}

bool StreamPeerUnix::is_blocking_mode_enabled() { return blocking; }

StreamPeerUnix::~StreamPeerUnix() { close(); }

void StreamPeerUnix::_bind_methods() {
  ClassDB::bind_method(D_METHOD("get_blocking_mode"),
                       &StreamPeerUnix::is_blocking_mode_enabled);
  ClassDB::bind_method(D_METHOD("set_blocking_mode", "value"),
                       &StreamPeerUnix::set_blocking_mode);
  // ADD_PROPERTY(PropertyInfo(bool))

  ClassDB::bind_method(D_METHOD("open", "path"), &StreamPeerUnix::open);
  ClassDB::bind_method(D_METHOD("get_path"), &StreamPeerUnix::get_path);
  ClassDB::bind_method(D_METHOD("is_open"), &StreamPeerUnix::is_open);
  ClassDB::bind_method(D_METHOD("close"), &StreamPeerUnix::close);
};
