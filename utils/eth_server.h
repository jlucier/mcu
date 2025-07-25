#include <SPI.h>
#include <Ethernet.h>

#ifndef SERVER_PORT
#define SERVER_PORT 80
#endif

#ifndef MAX_RESPONSE_LEN
#define MAX_RESPONSE_LEN 2048
#endif

#ifndef MAX_REQUEST_BODY
#define MAX_REQUEST_BODY 2048
#endif

#ifndef MAX_ROUTE_TARGET
#define MAX_ROUTE_TARGET 64
#endif

#ifndef DISABLE_HEADERS
#ifndef MAX_HEADERS 
#define MAX_HEADERS 10
#endif
#endif

#ifndef MAX_ROUTES
#define MAX_ROUTES 32
#endif

#ifndef ETH_HTTP_BUFFER_SIZE
#define ETH_HTTP_BUFFER_SIZE 4096
#endif

namespace EthHTTPServer {
    char buffer[ETH_HTTP_BUFFER_SIZE];

    static EthernetServer server(SERVER_PORT);

    struct http_header {
        char name[64];
        char data[256];
    };

    struct http_request {
        bool valid = true;
        int content_length = 0;
        char method[8];
        char protocol[8];
        char target[MAX_ROUTE_TARGET];
    #ifndef DISABLE_HEADERS
        int num_headers = 0;
        http_header headers[MAX_HEADERS];
    #endif
        char body[MAX_REQUEST_BODY];
    };

    struct http_response {
        int code = 200;
        char code_msg[16] = "Success";
        char content_type[32] = "text/plain; charset=utf-8";
        char body[MAX_RESPONSE_LEN];
        #ifndef DISABLE_HEADERS
        http_header headers[MAX_HEADERS];
        int num_headers = 0;
        #endif
    };

    using route_func_t = http_response (*)(const http_request&);

    struct route_t {
        route_func_t func = nullptr;
        char target[MAX_ROUTE_TARGET];
    };

    struct route_table_t {
        int num_routes = 0;
        route_t not_found;
        route_t routes[MAX_ROUTES];
    };

    static route_table_t route_table;

    enum parse_stage {
        method,
        target,
        protocol,
        headers,
        body
    };

    // response helper
    http_response empty_response(int code, const char* code_message) {
        http_response resp;
        resp.code = code;
        strcpy(resp.code_msg, code_message);
        return resp;
    }

    // helpers

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

    /*
     * Copy with null terminator added on
     */
    void cpy(char* dest, char* src, int count) {
        memcpy(dest, src, count);
        dest[count] = '\0';
    }

    int read_all(EthernetClient& client, char* buffer, int max) {
        int avail = client.available();
        int len = max < avail ? max : avail;
        int i = 0;
        for (; i < len; i++) buffer[i] = client.read();
        return i > 0 ? i + 1 : 0;
    }

    http_request parse_request(EthernetClient& client) {
        int len = read_all(client, buffer, ETH_HTTP_BUFFER_SIZE);

        http_request req;
        parse_stage stage = parse_stage::method;

        int cursor = 0;
        int header_i = 0;
        Serial.println("Parsing request...");
        Serial.println(buffer);
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
            else if (stage == parse_stage::headers
            #ifndef DISABLE_HEADERS
                && header_i < MAX_HEADERS) {
            #else
                ) {
            #endif
                // skip headers lol (read until we can an empty line)
                int ll = find_endline(data, remaining);
                int name_end = find_char(data, remaining, ':');

                if (name_end + 2 < ll) {
            #ifndef DISABLE_HEADERS
                    http_header* header = &req.headers[header_i];
            #else
                    static http_header _header;
                    http_header* header = &_header;
            #endif
                    cpy(header->name, data, name_end);
                    cpy(header->data, data + name_end + 2, ll - (name_end + 2));

                    if (!strcmp(header->name, "Content-Length")) {
                        char* end;
                        req.content_length = strtol(header->data, &end, 10);
                    }

            #ifndef DISABLE_HEADERS
                    req.num_headers++;
                    header_i++;
            #endif
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
        Serial.print("Finished. Target: ");
        Serial.println(req.target);
        return req;
    }

    const route_t* match_route(const http_request& req) {
        Serial.print("Matching route for target: ");
        
        for (int i = 0; i < route_table.num_routes; i++) {
            const route_t* r = &route_table.routes[i];

            if (!strcmp(r->target, req.target))
                return r;
        }

        return nullptr;
    }

    void send_response(EthernetClient& client, const http_response& resp) {
        static char const* format =
            "HTTP/1.1 %d %s\n"
            "Content-Type: %s\n"
            "Content-Length: %d\n"
            "%s"
            "\n"
            "%s";
        #ifdef DISABLE_HEADERS
        static char header_buffer[1] = {0}; // empty string if headers are disabled
        #else
        static char const* header_format = "%s: %s\n";
        static char header_buffer[2048];
        static char scratch[512];

        int offset;
        for (int i = 0; i < resp.num_headers; i++) {
            sprintf(scratch, header_format, resp.headers[i].name, resp.headers[i].data);
            strcpy(header_buffer + offset, scratch);
            offset += strlen(scratch);
        }
        #endif

        sprintf(
            buffer,
            format,
            resp.code,
            resp.code_msg,
            resp.content_type,
            strlen(resp.body),
            header_buffer,
            resp.body
        );
        int i = 0;
        do {
            client.write(buffer[i]);
            i++;
        } while (buffer[i] != '\0');
    }

    http_response default_not_found(const http_request& req) {
        return empty_response(404, "Not found");
    }

    // user api

    #ifdef BOARD_IS_PICO
    struct EthServerConfig {
        byte mac[12] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
        long unsigned int ip = 0;
        unsigned int pin_rx = 4;
        unsigned int pin_tx = 3;
        unsigned int pin_cs = 5;
        unsigned int pin_sck = 2;
    };

    void setup(const EthServerConfig& config) {
        Serial.println("Server init");
        // SPI
        bool ok = SPI.setRX(config.pin_rx);
        ok &= SPI.setTX(config.pin_tx);
        ok &= SPI.setCS(config.pin_cs);
        ok &= SPI.setSCK(config.pin_sck);

        if (ok) {
            // true for hardware cs
            SPI.begin(true);
        } else {
            Serial.println("SPI Failed");
        }
        Ethernet.init(config.pin_cs);

        // Ethernet interface
        if (config.ip) {
            Ethernet.begin(config.mac, config.ip);
        } else {
            Ethernet.begin(config.mac);
        }

        if (Ethernet.hardwareStatus() == EthernetNoHardware) {
            Serial.println("Ethernet not found.");
        } else {
            Serial.println("Ethernet found.");
            Serial.println(Ethernet.localIP());
        }
    }
    #else
    void setup(const byte* mac, unsigned long ip = 0) {
        SPI.begin();
        if (ip) {
            Ethernet.begin(mac, ip);
        } else {
            Ethernet.begin(mac);
        }

        if (Ethernet.hardwareStatus() == EthernetNoHardware) {
            Serial.println("Ethernet not found.");
        } else {
            Serial.println("Ethernet found.");
            Serial.println(Ethernet.localIP());
        }
    }

    #endif

    void add_endpoint(const char* target, route_func_t func) {
        route_t& new_route = route_table.routes[route_table.num_routes];
        strcpy(new_route.target, target);
        new_route.func = func;
        route_table.num_routes++;
    }

    void run() {
        if (Ethernet.hardwareStatus() == EthernetNoHardware) {
            Serial.println("Ethernet not found.");
            return;
        }

        EthernetClient client = server.available();
        if (client) {
            digitalWrite(LED_BUILTIN, HIGH);

            http_request req = parse_request(client);
            const route_t* r = match_route(req);

            if (r) {
                send_response(client, r->func(req));
            } else if (route_table.not_found.func) {
                send_response(client, route_table.not_found.func(req));
            } else {
                send_response(client, default_not_found(req));
            }

            Serial.println("Request handled.");
            digitalWrite(LED_BUILTIN, LOW);
        }
    }
}
