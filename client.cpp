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

  char wbuf[4 + k_max_msg];
  memcpy(&wbuf[0], &len, 4); // assume little endian
  uint32_t n = cmd.size();
  memcpy(&wbuf[4], &n, 4);
  size_t cur = 8;
  for (const string &s : cmd) {
    uint32_t p = (uint32_t)s.size();
    memcpy(&wbuf[cur], &p, 4);
    memcpy(&wbuf[cur + 4], s.data(), s.size());
    cur += 4 + s.size();
  }
  return write_all(fd, (const uint8_t *)wbuf, 4 + len);
}

// Function to read a complete response from the server.
static int32_t read_res(int fd) {
  // 1. Read the 4-byte total length header
  char rbuf[4 + k_max_msg];
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
  err = read_full(fd, (uint8_t *)&rbuf[4], len);
  if (err) {
    msg("read() error");
    return err;
  }

  // 3. Extract status code and print the result
  uint32_t rescode = 0;
  if (len < 4) {
    msg("bad response");
    return -1;
  }
  memcpy(&rescode, &rbuf[4], 4);
  printf("server says: [%u] %.*s\n", rescode, len - 4, &rbuf[8]);
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