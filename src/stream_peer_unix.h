#ifndef STREAM_UNIX_H
#define STREAM_UNIX_H

#include <sys/un.h>

#include <godot_cpp/classes/stream_peer.hpp>
#include <godot_cpp/classes/stream_peer_extension.hpp>

#include <godot_cpp/core/binder_common.hpp>

using namespace godot;

Error _get_data(void *, uint8_t *, int);
Error _get_partial_data(void *, uint8_t *, int, int *);
Error _put_data(void *, const uint8_t *, int);
Error _put_partial_data(void *, const uint8_t *, int, int *);
int _get_available_bytes(const void *);

class StreamPeerUnix : public StreamPeerExtension {
  GDCLASS(StreamPeerUnix, StreamPeerExtension)

  bool blocking;

protected:
  static void _bind_methods();
  int socketfd = -1;
  String path;

  struct sockaddr_un server_address;

public:
  virtual Error get_data(uint8_t *p_buffer, int p_bytes);
  virtual Error get_partial_data(uint8_t *p_buffer, int p_bytes,
                                 int *r_received);
  virtual Error put_data(const uint8_t *p_data, int p_bytes);
  virtual Error put_partial_data(const uint8_t *p_data, int p_bytes,
                                 int *r_sent);
  virtual int get_available_bytes();

  int open(const String path);
  String get_path();
  bool is_open();
  void close();

  void set_blocking_mode(bool value);
  bool is_blocking_mode_enabled();

  ~StreamPeerUnix();
};

#endif // STREAM_UNIX_H
