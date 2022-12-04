#include <SPI.h>
#include <Ethernet.h>

#include "private.h"

#define HTTP_SERVER_PORT 80
#define MAX_RESPONSE_LEN 2048
#define MAX_REQUEST_BODY 2048
#define MAX_HEADERS 10

#define PIN_RX 16
#define PIN_TX 19
#define PIN_CS 17
#define PIN_SCK 18

// http stuff


namespace EthHTTPServer {
  char buffer[4096];

  static EthernetServer server(80);

  struct http_header {
    char name[64];
    char data[256];
  };

  struct http_request {
    bool valid = true;
    int content_length = 0;
    int num_headers = 0;
    char method[16];
    char protocol[16];
    char target[128];
    http_header headers[MAX_HEADERS];
    char body[MAX_REQUEST_BODY];
  };

  struct http_response {
    int code = 200;
    char code_msg[32];
    char content_type[32] = "text/plain; charset=utf-8";
    http_header headers[MAX_HEADERS];
    char body[MAX_RESPONSE_LEN];
    int num_headers = 0;
  };


  enum parse_stage {
    method,
    target,
    protocol,
    headers,
    body
  };

  int find_char(char* data, int len, char c) {
    int i = 0;
    while (i < len && data[i] != c) i++;
    return i;
  }

  int find_endline(char* data, int len) {
    return find_char(data, len, '\n');
  }

  int find_whitespace(char* data, int len) {
    int i = 0;
    while (i < len) {
      char c = data[i];
      if (c == ' ' || c == '\n')
        break;
      i++;
    }
    return i;
  }

  // Copy with null terminator added on
  void cpy(char* dest, char* src, int count) {
    memcpy(dest, src, count);
    dest[count] = '\0';
  }

  int read_all(EthernetClient* client, char* buffer, int max) {
    int avail = client->available();
    int len = max < avail ? max : avail;
    int i = 0;
    for (; i < len; i++) buffer[i] = client->read();
    return i > 0 ? i + 1 : 0;
  }

  http_request parse_request(EthernetClient* client) {
    int len = read_all(client, buffer, 4096);

    http_request req;
    parse_stage stage = parse_stage::method;

    int cursor = 0;
    int header_i = 0;
    while (cursor < len) {
      char* data = buffer + cursor;
      int remaining = len - cursor;

      if (stage < parse_stage::headers) {
        int space_i = find_whitespace(data, remaining);

        char* holster;
        parse_stage next;
        switch (stage) {
          case parse_stage::method:
            holster = req.method;
            next = parse_stage::target;
            break;
          case parse_stage::target:
            holster = req.target;
            next = parse_stage::protocol;
            break;
          case parse_stage::protocol:
            holster = req.protocol;
            next = parse_stage::headers;
            break;
        }

        cpy(holster, data, space_i);
        cursor += space_i + 1;
        stage = next;
      }
      else if (stage == parse_stage::headers && header_i < MAX_HEADERS) {
        // skip headers lol (read until we can an empty line)
        int ll = find_endline(data, remaining);
        int name_end = find_char(data, remaining, ':');

        if (name_end + 2 < ll) {
          http_header* header = &req.headers[header_i];
          cpy(header->name, data, name_end);
          cpy(header->data, data + name_end + 2, ll - (name_end + 2));

          if (!strcmp(header->name, "Content-Length")) {
            char* end;
            req.content_length = strtol(header->data, &end, 10);
          }

          req.num_headers++;
          header_i++;
        }

        cursor += ll + 1;
        // between headers and body there will be a blank line.
        if (ll < 2) {
          stage = parse_stage::body;
        }
      }
      else {
        // body parsing
        memcpy(req.body, data, find_endline(data, remaining));
        break;
      }
    }
    return req;
  }

  void write_buffer(EthernetClient* client) {
    int i = 0;
    do {
      client->write(buffer[i]);
      i++;
    } while (buffer[i] != '\0');
  }

  void send_response(EthernetClient* client, const http_response* resp) {
    static char const* format = 
          "HTTP/1.1 %d %s\n"
          "Content-Type: %s\n"
          "Content-Length: %d\n"
          "%s"
          "\n"
          "%s";
    static char const* header_format = "%s: %s\n";
    static char header_buffer[2048];
    static char scratch[512];

    int offset;    
    for (int i = 0; i < resp->num_headers; i++) {
      sprintf(scratch, header_format, resp->headers[i].name, resp->headers[i].data);
      strcpy(header_buffer + offset, scratch);
      offset += strlen(scratch);
    }
    
    sprintf(
      buffer,
      format,
      resp->code,
      resp->code_msg,
      resp->content_type,
      strlen(resp->body),
      header_buffer,
      resp->body
    );
    write_buffer(client);
  }

  // user api

  void setup() {
    // SPI
    bool ok = SPI.setRX(PIN_RX);
    ok &= SPI.setTX(PIN_TX);
    ok &= SPI.setCS(PIN_CS);
    ok &= SPI.setSCK(PIN_SCK);

    if (ok) {
      // true for hardware cs
      SPI.begin(true);
    } else {
      Serial.println("FUCKKKKK");
    }

    // Ethernet interface
    Ethernet.init(PIN_CS);
    Ethernet.begin(mac, ip);
  }

  void run() {
    static const http_response test_response {
      200,
      "OK",
      "text/plain; charset=utf-8",  
      {},
      "Hello World!"      
    };

    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println("Ethernet not found.");
      return;
    }

    EthernetClient client = server.available();
    if (client) {
      Serial.println("Start Request");
      digitalWrite(LED_BUILTIN, HIGH);

      http_request req = parse_request(&client);
      Serial.printf("%s %s %s\n", req.method, req.target, req.protocol);
      for (int i = 0; i < req.num_headers; i++) {
        Serial.printf("%s: %s\n", req.headers[i].name, req.headers[i].data);
      }
      if (req.content_length)
        Serial.println(req.body);

      
      send_response(&client, &test_response);

      digitalWrite(LED_BUILTIN, LOW);
    }
  }
}




