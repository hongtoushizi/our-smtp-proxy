#ifndef __LOG_H
#define __LOG_H

#define MAIN_THREAD_LOG(...)      mylog("[MAIN THREAD] " __VA_ARGS__);
#define CLIENT_THREAD_LOG(...)    mylog("[CLIENT THREAD] " __VA_ARGS__);
#define TP_LOG(...)               mylog("[TP] " __VA_ARGS__);
#define TP_CONN_LOG(...)          mylog("[TP CONN] " __VA_ARGS__);
#define TP_CONFIG_LOG(...)        mylog("[TP CONFIG] " __VA_ARGS__);

#define LOG_FILE "proxy.log"

void mylog(const char *format, ...);

#endif