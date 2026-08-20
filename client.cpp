// Standard C library includes for assertions, integers, utility, string
// manipulation, standard I/O, and error numbers.
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// POSIX operating system API (read, write, close)
#include <unistd.h>

// Socket and networking includes (internet operations, socket definitions, IP
// protocols)
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <sys/socket.h>

// C++ standard library includes for strings and dynamic arrays (vectors)
#include <string>
#include <vector>

// Use the standard namespace to avoid typing std:: everywhere
using namespace std;

enum {
    TAG_NIL = 0,    // nil (like null)
    TAG_ERR = 1,    // error code + msg
    TAG_STR = 2,    // string
    TAG_INT = 3,    // int64
    TAG_DBL = 4,    // double
    TAG_ARR = 5,    // array
};

// Helper function to print a simple message to standard error
static void msg(const char *msg) { fprintf(stderr, "%s\n", msg); }

// Helper function to print an error message including the current errno, then
// terminate the program
static void die(const char *msg) {
  int err = errno;
  fprintf(stderr, "[%d] %s\n", err, msg);
  abort();
}

// Helper function to read exactly 'n' bytes from the file descriptor 'fd' into
// 'buf'. It loops until all 'n' bytes are read, handling partial reads
// automatically.
static int32_t read_full(int fd, uint8_t *buf, size_t n) {
  while (n > 0) {
    ssize_t rv = read(fd, buf, n); // Perform the read system call
    if (rv <= 0) {
      return -1; // error, or unexpected EOF (end of file)
    }
    assert((size_t)rv <= n); // Ensure we didn't read more bytes than requested
    n -= (size_t)rv;         // Decrease the remaining bytes to read
    buf += rv;               // Advance the buffer pointer
  }
  return 0; // Success
}

// Helper function to write exactly 'n' bytes to the file descriptor 'fd' from
// 'buf'. It loops until all 'n' bytes are written, handling partial writes
// automatically.
static int32_t write_all(int fd, const uint8_t *buf, size_t n) {
  while (n > 0) {
    ssize_t rv = write(fd, buf, n); // Perform the write system call
    if (rv <= 0) {
      return -1; // error
    }
    assert((size_t)rv <= n); // Ensure we didn't write more bytes than requested
    n -= (size_t)rv;         // Decrease the remaining bytes to write
    buf += rv;               // Advance the buffer pointer
  }
  return 0; // Success
}

// Helper function to append raw byte data to the end of a vector buffer
static void buf_append(vector<uint8_t> &buf, const uint8_t *data, size_t len) {
  buf.insert(buf.end(), data, data + len);
}

// Maximum message size allowed (32 Megabytes). This is a sanity check to
// prevent huge allocations.
const size_t k_max_msg = 32 << 20; // likely larger than the kernel buffer

// Function to send a request to the server in the format:
// TotalLength + NumStrings + Len1 + Str1 + Len2 + Str2 ...
static int32_t send_req(int fd, const vector<string> &cmd) {
  uint32_t len = 4;
  for (const string &s : cmd) {
    len += 4 + s.size();
  }
  if (len > k_max_msg) {
    return -1;
  }

  vector<uint8_t> wbuf(4 + len);
  memcpy(wbuf.data(), &len, 4); // assume little endian
  uint32_t n = cmd.size();
  memcpy(wbuf.data() + 4, &n, 4);
  size_t cur = 8;
  for (const string &s : cmd) {
    uint32_t p = (uint32_t)s.size();
    memcpy(wbuf.data() + cur, &p, 4);
    memcpy(wbuf.data() + cur + 4, s.data(), s.size());
    cur += 4 + s.size();
  }
  return write_all(fd, wbuf.data(), 4 + len);
}

static int32_t on_response(const uint8_t *data, size_t size) {
  if (size < 1) {
    msg("bad response");
    return -1;
  }
  switch (data[0]) {
  case TAG_NIL:
    printf("(nil)\n");
    return 1;
  case TAG_ERR: {
    if (size < 1 + 8) return -1;
    int32_t code = 0;
    uint32_t len = 0;
    memcpy(&code, &data[1], 4);
    memcpy(&len, &data[1 + 4], 4);
    if (size < 1 + 8 + len) return -1;
    printf("(err) %d %.*s\n", code, len, &data[1 + 8]);
    return 1 + 8 + len;
  }
  case TAG_STR: {
    if (size < 1 + 4) return -1;
    uint32_t len = 0;
    memcpy(&len, &data[1], 4);
    if (size < 1 + 4 + len) return -1;
    printf("(str) %.*s\n", len, &data[1 + 4]);
    return 1 + 4 + len;
  }
  case TAG_INT: {
    if (size < 1 + 8) return -1;
    int64_t val = 0;
    memcpy(&val, &data[1], 8);
    printf("(integer) %ld\n", val);
    return 1 + 8;
  }
  case TAG_ARR: {
    if (size < 1 + 4) return -1;
    uint32_t len = 0;
    memcpy(&len, &data[1], 4);
    printf("(arr) len=%u\n", len);
    size_t arr_bytes = 1 + 4;
    for (uint32_t i = 0; i < len; ++i) {
      int32_t rv = on_response(&data[arr_bytes], size - arr_bytes);
      if (rv < 0) return rv;
      arr_bytes += (size_t)rv;
    }
    printf("(arr) end\n");
    return (int32_t)arr_bytes;
  }
  }
  msg("bad response");
  return -1;
}

// Function to read a complete response from the server.
static int32_t read_res(int fd) {
  // 1. Read the 4-byte total length header
  char rbuf[4];
  errno = 0;
  int32_t err = read_full(fd, (uint8_t *)rbuf, 4);
  if (err) {
    if (errno == 0) {
      msg("EOF");
    } else {
      msg("read() error");
    }
    return err;
  }

  uint32_t len = 0;
  memcpy(&len, rbuf, 4); // assume little endian
  if (len > k_max_msg) {
    msg("too long");
    return -1;
  }

  // 2. Read the reply body
  vector<uint8_t> body(len);
  err = read_full(fd, body.data(), len);
  if (err) {
    msg("read() error");
    return err;
  }

  // 3. Parse the TLV response
  int32_t rv = on_response(body.data(), len);
  if (rv > 0 && (size_t)rv != len) {
    msg("bad response");
    return -1;
  }
  return 0;
}

int main(int argc, char **argv) {
  // Create a TCP socket using IPv4
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    die("socket()");
  }

  // Define the server address to connect to (127.0.0.1 on port 1234)
  struct sockaddr_in addr = {};
  addr.sin_family = AF_INET;
  addr.sin_port = ntohs(1234);
  addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK); // 127.0.0.1
  int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
  if (rv) {
    die("connect");
  }

  // Parse command line arguments into our command vector
  vector<string> cmd;
  for (int i = 1; i < argc; ++i) {
    cmd.push_back(argv[i]);
  }

  // Send request
  int32_t err = send_req(fd, cmd);
  if (err) {
    goto L_DONE;
  }

  // Read response
  err = read_res(fd);
  if (err) {
    goto L_DONE;
  }

L_DONE:
  close(fd);
  return 0;
}